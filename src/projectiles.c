#include "types.h"
#include "globals.h"
#include <stdlib.h>
#include <math.h>

void spawnProjectile(int x, int y, float dx, float dy, int type, int damage)
{
  if (projectileCount >= MAX_PROJECTILES)
    return;

  Projectile *proj = &projectiles[projectileCount++];
  proj->x = x;
  proj->y = y;
  proj->dx = dx;
  proj->dy = dy;
  proj->type = type;
  proj->alive = 1;
  proj->damage = damage;
  proj->speed = 2; // Projectiles move 2 cells per frame

  // Set effect and range based on type
  if (type == 0)
  {
    proj->effect = 1;     // Lightning = stun
    proj->maxRange = 200; // Lightning travels far
  }
  else if (type == 1)
  {
    proj->effect = 2;     // Fireball = DoT
    proj->maxRange = 150; // Fireballs travel medium distance
  }
  else
  {
    proj->effect = 0;    // Arrow = no effect
    proj->maxRange = 80; // Arrows have limited range
  }
  proj->range = 0; // Start with zero distance traveled
}

void updateProjectiles()
{
  for (int i = projectileCount - 1; i >= 0; i--)
  {
    if (!projectiles[i].alive)
      continue;

    // Move projectile with precise float directions
    projectiles[i].x += projectiles[i].dx * projectiles[i].speed;
    projectiles[i].y += projectiles[i].dy * projectiles[i].speed;

    // Update range traveled (calculate actual distance moved)
    float distanceMoved = sqrt(projectiles[i].dx * projectiles[i].dx * projectiles[i].speed * projectiles[i].speed +
                               projectiles[i].dy * projectiles[i].dy * projectiles[i].speed * projectiles[i].speed);
    projectiles[i].range += (int)distanceMoved;

    // Check if projectile exceeded its maximum range
    if (projectiles[i].range >= projectiles[i].maxRange)
    {
      projectiles[i].alive = 0;
      continue;
    }

    // Check collision with player
    if (abs(projectiles[i].x - player.x) <= 1 && abs(projectiles[i].y - player.y) <= 1)
    {
      if (!player.invulnerabilityTimer)
      {
        player.health -= projectiles[i].damage;
        PlaySound(sounds[0]);             // Fight sound
        player.invulnerabilityTimer = 60; // 1 second invulnerability

        // Apply projectile effects
        if (projectiles[i].effect == 1) // Stun (lightning)
        {
          player.stunTimer = 60; // 1 second stun
        }
        else if (projectiles[i].effect == 2) // DoT (fire)
        {
          player.dotTimer = 180;                        // 3 seconds DoT
          player.dotDamage = projectiles[i].damage / 3; // Damage over 3 ticks
        }
      }
      projectiles[i].alive = 0;
      continue;
    }

    // Check collision with monsters
    for (int j = 0; j < monsterCount; j++)
    {
      if (!monsters[j].alive)
        continue;

      if (abs(projectiles[i].x - monsters[j].x) <= 1 && abs(projectiles[i].y - monsters[j].y) <= 1)
      {
        // Damage the monster
        monsters[j].health -= projectiles[i].damage;

        // Apply projectile effects to monster
        if (projectiles[i].effect == 1) // Stun (lightning)
        {
          monsters[j].stunTimer = 60; // 1 second stun
        }
        else if (projectiles[i].effect == 2) // DoT (fire)
        {
          monsters[j].dotTimer = 180;                        // 3 seconds DoT
          monsters[j].dotDamage = projectiles[i].damage / 3; // Damage over 3 ticks
        }

        // Check if monster died
        if (monsters[j].health <= 0)
        {
          monsters[j].alive = 0;
          // Award experience to player
          player.experience += monsters[j].power * 10;

          // Check for level up
          if (player.experience >= player.experienceToNext)
          {
            player.level++;
            player.experience -= player.experienceToNext;
            player.experienceToNext = player.level * 100;
            player.maxHealth += 20;
            player.health = player.maxHealth;
            player.power += 2;
          }
        }

        // Remove projectile after hitting
        projectiles[i].alive = 0;
        break; // Exit monster loop since projectile is dead
      }
    }

    // Despawn if too far
    float dx = projectiles[i].x - player.x;
    float dy = projectiles[i].y - player.y;
    if (sqrt(dx * dx + dy * dy) > 50)
    {
      projectiles[i].alive = 0;
    }
  }

  // Remove dead projectiles
  for (int i = projectileCount - 1; i >= 0; i--)
  {
    if (!projectiles[i].alive)
    {
      projectiles[i] = projectiles[projectileCount - 1];
      projectileCount--;
    }
  }
}

void drawProjectiles()
{
  for (int i = 0; i < projectileCount; i++)
  {
    if (!projectiles[i].alive)
      continue;

    int textureIndex = 6 + projectiles[i].type; // 6=lightning, 7=fireball, 8=arrow
    if (projectiles[i].type == 2)
      textureIndex = 8; // Arrow uses texture 8

    // Draw all projectiles at 10x10 pixels (half tile size)
    Rectangle sourceRect = {0, 0, textures[textureIndex].width, textures[textureIndex].height};
    Rectangle destRect = {
        projectiles[i].x * CELL_SIZE - camera.target.x + WINDOW_SIZE / 2,
        projectiles[i].y * CELL_SIZE - camera.target.y + WINDOW_SIZE / 2,
        CELL_SIZE / 2.0f, // 10x10 pixels
        CELL_SIZE / 2.0f  // 10x10 pixels
    };
    Vector2 origin = {CELL_SIZE / 4.0f, CELL_SIZE / 4.0f}; // Center the smaller sprite
    DrawTexturePro(textures[textureIndex], sourceRect, destRect, origin, 0.0f, WHITE);
  }
}
