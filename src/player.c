#include "types.h"
#include "globals.h"
#include <stdlib.h>
#include <math.h>

// Function prototype for spawnProjectile (defined in projectiles.c)
void spawnProjectile(int x, int y, float dx, float dy, int type, int damage);

void updatePlayer()
{
  // Update status effects
  if (player.stunTimer > 0)
  {
    player.stunTimer--;
    return; // Can't move or act while stunned
  }

  if (player.dotTimer > 0)
  {
    player.dotTimer--;
    if (player.dotTimer % 60 == 0) // Every second
    {
      player.health -= player.dotDamage;
      if (player.health <= 0)
      {
        player.alive = 0;
        PlaySound(sounds[3]); // Death sound
      }
    }
  }

  if (player.speedBoostTimer > 0)
  {
    player.speedBoostTimer--;
  } // Update ability cooldowns
  if (player.jumpSmashCooldown > 0)
    player.jumpSmashCooldown--;
  if (player.rushCooldown > 0)
    player.rushCooldown--;
  if (player.healCooldown > 0)
    player.healCooldown--;
  if (player.arrowCooldown > 0)
    player.arrowCooldown--;

  // Update movement cooldown
  if (player.movementCooldown > 0)
  {
    player.movementCooldown--;
  }

  // Update invulnerability timer
  if (player.invulnerabilityTimer > 0)
  {
    player.invulnerabilityTimer--;
  }

  // Don't process movement input while on cooldown
  if (player.movementCooldown > 0)
  {
    return;
  }

  // Handle input
  int moved = 0;

  // Track intended direction from key presses
  player.intendedDirX = 0;
  player.intendedDirY = 0;

  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
  {
    player.intendedDirY = -1;
  }
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
  {
    player.intendedDirY = 1;
  }
  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
  {
    player.intendedDirX = -1;
  }
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
  {
    player.intendedDirX = 1;
  }

  // If no intended direction, keep last direction
  if (player.intendedDirX == 0 && player.intendedDirY == 0)
  {
    player.intendedDirX = player.lastDirX;
    player.intendedDirY = player.lastDirY;
  }

  // Calculate current speed multiplier (includes rush boost)
  float currentSpeedMultiplier = player.speedMultiplier;
  if (player.speedBoostTimer > 0)
  {
    currentSpeedMultiplier *= 2.0f; // 2x speed during rush
  }

  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
  {
    player.y -= player.speed * currentSpeedMultiplier;
    player.lastDirX = 0;
    player.lastDirY = -1;
    moved = 1;
  }
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
  {
    player.y += player.speed * currentSpeedMultiplier;
    player.lastDirX = 0;
    player.lastDirY = 1;
    moved = 1;
  }
  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
  {
    player.x -= player.speed * currentSpeedMultiplier;
    player.lastDirX = -1;
    player.lastDirY = 0;
    moved = 1;
  }
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
  {
    player.x += player.speed * currentSpeedMultiplier;
    player.lastDirX = 1;
    player.lastDirY = 0;
    moved = 1;
  }

  // If player moved, set cooldown (longer when in combat)
  if (moved)
  {
    player.movementCooldown = player.isInCombat ? 12 : 6; // 50% slower when fighting
    // Play movement sound
    // if (sounds[0].frameCount > 0) PlaySound(sounds[0]);
  }

  // Handle abilities
  if (IsKeyPressed(KEY_ONE) && player.jumpSmashCooldown <= 0)
  {
    // Jump smash: jump forward and AoE damage
    int jumpDistance = 3;
    int dirX = player.intendedDirX;
    int dirY = player.intendedDirY;

    // If no intended direction, default to up
    if (dirX == 0 && dirY == 0)
    {
      dirX = 0;
      dirY = -1;
    }

    int jumpX = player.x + dirX * jumpDistance;
    int jumpY = player.y + dirY * jumpDistance;

    // Move player
    player.x = jumpX;
    player.y = jumpY;

    // AoE damage to nearby monsters
    for (int i = 0; i < monsterCount; i++)
    {
      if (!monsters[i].alive)
        continue;
      float dx = monsters[i].x - player.x;
      float dy = monsters[i].y - player.y;
      if (sqrt(dx * dx + dy * dy) <= 2) // Within 2 units
      {
        monsters[i].health -= player.power * 2;
        if (monsters[i].health <= 0)
        {
          monsters[i].alive = 0;
          player.experience += 10;
        }
      }
    }

    player.jumpSmashCooldown = 180; // 3 seconds
    PlaySound(sounds[0]);
  }

  if (IsKeyPressed(KEY_TWO) && player.rushCooldown <= 0)
  {
    // Rush: temporary speed boost
    player.speedBoostTimer = 180; // 3 seconds of 2x speed
    player.rushCooldown = 600;    // 10 seconds cooldown
    PlaySound(sounds[0]);
  }

  if (IsKeyPressed(KEY_THREE) && player.healCooldown <= 0)
  {
    // Full heal
    player.health = player.maxHealth;
    player.healCooldown = 1800; // 30 seconds
    PlaySound(sounds[1]);       // Powerup sound
  }

  // Arrow shooting (hold space)
  if (IsKeyDown(KEY_SPACE) && player.arrowCooldown <= 0)
  {
    float arrowDx = player.intendedDirX;
    float arrowDy = player.intendedDirY;

    // If no intended direction, default to up
    if (arrowDx == 0 && arrowDy == 0)
    {
      arrowDx = 0;
      arrowDy = -1;
    }

    if (arrowDx != 0 || arrowDy != 0)
    {
      // Normalize the direction for consistent arrow speed
      float length = sqrt(arrowDx * arrowDx + arrowDy * arrowDy);
      if (length > 0)
      {
        arrowDx /= length;
        arrowDy /= length;
      }

      spawnProjectile(player.x, player.y, arrowDx, arrowDy, 2, player.power / 2);
      player.arrowCooldown = 60; // Once per second
    }
  }

  // No bounds checking - unlimited world exploration!

  // Update camera to follow player
  camera.target = (Vector2){player.x * CELL_SIZE, player.y * CELL_SIZE};
}
