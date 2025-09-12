#include "types.h"
#include "globals.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

// Function prototypes for functions called before definition
void initGame();
void updateChunks();

void restartGame()
{
  // Reset all game state first
  projectileCount = 0;
  monsterCount = 0;
  powerupCount = 0;
  landmineCount = 0;
  loadedChunkCount = 0;

  // Clear all arrays to prevent stale data issues
  memset(monsters, 0, sizeof(monsters));
  memset(powerups, 0, sizeof(powerups));
  memset(landmines, 0, sizeof(landmines));
  memset(projectiles, 0, sizeof(projectiles));

  // Clear all chunks
  for (int i = 0; i < MAX_LOADED_CHUNKS; i++)
  {
    loadedChunks[i].loaded = 0;
  }

  // Reinitialize game
  initGame();
}

void initGame()
{
  // Initialize player
  strcpy(player.name, "Knight");
  player.x = 1; // Start slightly away from origin to avoid spawn conflicts
  player.y = 1;
  player.health = 100;
  player.maxHealth = 100;
  player.power = 8;
  player.textureIndex = 0;
  player.alive = 1;
  player.speed = 1;
  player.speedMultiplier = 1.0f;
  player.damageMultiplier = 1.0f;
  player.powerupTimer = 0;
  player.movementCooldown = 0;
  player.level = 1;
  player.experience = 0;
  player.experienceToNext = 100;
  player.isInCombat = 0;
  player.invulnerabilityTimer = 180; // 3 seconds at 60 FPS

  // Initialize player abilities and status
  player.jumpSmashCooldown = 0;
  player.rushCooldown = 0;
  player.healCooldown = 0;
  player.arrowCooldown = 0;
  player.lastDirX = 0;
  player.lastDirY = -1; // Default to up
  player.intendedDirX = 0;
  player.intendedDirY = -1;
  player.stunTimer = 0;
  player.dotTimer = 0;
  player.dotDamage = 0;
  player.speedBoostTimer = 0;

  // Initialize camera
  camera.target = (Vector2){player.x * CELL_SIZE, player.y * CELL_SIZE};
  camera.offset = (Vector2){WINDOW_SIZE / 2, WINDOW_SIZE / 2};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  // Initialize chunk system
  loadedChunkCount = 0;
  monsterCount = 0;
  powerupCount = 0;
  landmineCount = 0;

  // Load initial chunks around player (more aggressively)
  for (int i = 0; i < 3; i++) // Load chunks multiple times to ensure they're ready
  {
    updateChunks();
  }

  // Initial monster spawn for immediate gameplay
  for (int i = 0; i < 15; i++) // Spawn 15 monsters initially
  {
    if (monsterCount < MAX_MONSTERS)
    {
      Character *monster = &monsters[monsterCount++];
      strcpy(monster->name, "Monster");

      // Spawn closer initially: 5-50 units from player
      int distance = 5 + rand() % 45;
      float angle = (rand() % 360) * DEG2RAD;
      monster->x = player.x + (int)(cos(angle) * distance);
      monster->y = player.y + (int)(sin(angle) * distance);

      monster->health = 20 + rand() % 30;
      monster->maxHealth = monster->health;
      monster->power = 3 + rand() % 5;
      monster->textureIndex = 1 + (rand() % 5); // Random monster texture (1-5: dragon, goblin, ogre, troll, wizard)
      monster->alive = 1;
      monster->speed = 1;
      monster->speedMultiplier = 1.0f;
      monster->damageMultiplier = 1.0f;
      monster->powerupTimer = 0;
      monster->movementCooldown = 0;
      monster->level = 1;
      monster->experience = 0;
      monster->experienceToNext = 0;
      monster->isInCombat = 0;
      monster->invulnerabilityTimer = 0;
    }
  }
}

void checkCollisions()
{
  // Reset combat flags
  player.isInCombat = 0;
  for (int i = 0; i < monsterCount; i++)
  {
    if (monsters[i].alive)
    {
      monsters[i].isInCombat = 0;
    }
  }

  // Check player vs monsters (adjacent cells)
  for (int i = 0; i < monsterCount; i++)
  {
    if (!monsters[i].alive)
      continue;

    // Check if monster is adjacent to player (including diagonally)
    int dx = abs(player.x - monsters[i].x);
    int dy = abs(player.y - monsters[i].y);

    if (dx <= 1 && dy <= 1 && !(dx == 0 && dy == 0))
    {
      // Adjacent combat!
      player.isInCombat = 1;
      monsters[i].isInCombat = 1;

      // Per-tick damage (only if player is not invulnerable)
      int playerDamage = (int)(player.power * player.damageMultiplier * 0.5f); // Reduced damage per tick
      int monsterDamage = (int)(monsters[i].power * monsters[i].damageMultiplier * 0.5f);

      // Troll gang damage multiplier
      if (monsters[i].textureIndex == 4) // Troll
      {
        int nearbyTrolls = 0;
        for (int j = 0; j < monsterCount; j++)
        {
          if (i != j && monsters[j].alive && monsters[j].textureIndex == 4)
          {
            int tdx = abs(monsters[i].x - monsters[j].x);
            int tdy = abs(monsters[i].y - monsters[j].y);
            if (tdx <= 2 && tdy <= 2) // Within 2 units
            {
              nearbyTrolls++;
            }
          }
        }
        // Damage multiplier: 2x per nearby troll
        monsterDamage *= (1 << nearbyTrolls); // 2^nearbyTrolls
      }

      monsters[i].health -= playerDamage;

      // Only damage player if not invulnerable
      if (player.invulnerabilityTimer <= 0)
      {
        player.health -= monsterDamage;
      }

      // Play battle sound (only once per combat tick)
      if (sounds[0].frameCount > 0 && !player.isInCombat)
        PlaySound(sounds[0]);

      if (monsters[i].health <= 0)
      {
        monsters[i].alive = 0;
        // Award experience to player
        player.experience += monsters[i].power * 10;
        // Play victory sound
        if (sounds[4].frameCount > 0)
          PlaySound(sounds[4]);

        // Check for level up
        if (player.experience >= player.experienceToNext)
        {
          player.level++;
          player.experience -= player.experienceToNext;
          player.experienceToNext = player.level * 100; // Next level requires more XP
          player.maxHealth += 20;
          player.health = player.maxHealth; // Full heal on level up
          player.power += 2;
        }
      }
      if (player.health <= 0)
      {
        player.alive = 0;
        // Play death sound
        if (sounds[3].frameCount > 0)
          PlaySound(sounds[3]);
      }
    }
  }

  // Check player vs powerups
  for (int i = 0; i < powerupCount; i++)
  {
    if (!powerups[i].active)
      continue;

    if (player.x == powerups[i].x && player.y == powerups[i].y)
    {
      // Apply powerup
      player.activePowerup = powerups[i].type;
      player.powerupTimer = 300; // 5 seconds at 60 FPS

      switch (powerups[i].type)
      {
      case POWERUP_DOUBLE_DAMAGE:
        player.damageMultiplier = 2.0f;
        break;
      case POWERUP_DOUBLE_HEALTH:
        player.health = player.maxHealth;
        break;
      case POWERUP_DOUBLE_SPEED:
        player.speedMultiplier = 2.0f;
        break;
      default:
        break;
      }

      powerups[i].active = 0;
      // Play powerup sound
      if (sounds[1].frameCount > 0)
        PlaySound(sounds[1]);
    }
  }

  // Check player vs landmines
  for (int i = 0; i < landmineCount; i++)
  {
    if (!landmines[i].active)
      continue;

    if (player.x == landmines[i].x && player.y == landmines[i].y)
    {
      player.health -= landmines[i].damage;
      landmines[i].active = 0;
      // Play damage sound
      if (sounds[2].frameCount > 0)
        PlaySound(sounds[2]);
    }
  }
}

void updatePowerups()
{
  if (player.powerupTimer > 0)
  {
    player.powerupTimer--;
    if (player.powerupTimer == 0)
    {
      // Reset powerups
      player.speedMultiplier = 1.0f;
      player.damageMultiplier = 1.0f;
    }
  }

  // Despawn powerups that are too far from player
  const int DESPAWN_DISTANCE = 300; // Despawn powerups more than 300 units away

  for (int i = powerupCount - 1; i >= 0; i--)
  {
    int dx = powerups[i].x - player.x;
    int dy = powerups[i].y - player.y;
    float distance = sqrt(dx * dx + dy * dy);

    if (distance > DESPAWN_DISTANCE)
    {
      // Remove this powerup by moving the last powerup to this position
      powerups[i] = powerups[powerupCount - 1];
      powerupCount--;
    }
  }
}

void updateLandmines()
{
  // Despawn landmines that are too far from player
  const int DESPAWN_DISTANCE = 300; // Despawn landmines more than 300 units away

  for (int i = landmineCount - 1; i >= 0; i--)
  {
    int dx = landmines[i].x - player.x;
    int dy = landmines[i].y - player.y;
    float distance = sqrt(dx * dx + dy * dy);

    if (distance > DESPAWN_DISTANCE)
    {
      // Remove this landmine by moving the last landmine to this position
      landmines[i] = landmines[landmineCount - 1];
      landmineCount--;
    }
  }
}
