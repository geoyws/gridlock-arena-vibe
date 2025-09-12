#include "types.h"
#include "globals.h"
#include <stdlib.h>
#include <math.h>

// Function prototype for spawnProjectile (defined in projectiles.c)
void spawnProjectile(int x, int y, float dx, float dy, int type, int damage);

void updateMonsters()
{
  for (int i = 0; i < monsterCount; i++)
  {
    if (!monsters[i].alive)
      continue;

    // Update movement cooldown
    if (monsters[i].movementCooldown > 0)
    {
      monsters[i].movementCooldown--;
      continue; // Don't move while on cooldown
    }

    // Special behaviors based on monster type
    int newX = monsters[i].x;
    int newY = monsters[i].y;

    if (monsters[i].textureIndex == 4) // Troll (1f47f.png) - move fast towards player
    {
      // Calculate direction towards player
      float dx = player.x - monsters[i].x;
      float dy = player.y - monsters[i].y;
      float dist = sqrt(dx * dx + dy * dy);
      if (dist > 0)
      {
        // Move 2 cells towards player (faster)
        newX += (int)(dx / dist * 2);
        newY += (int)(dy / dist * 2);
      }
      monsters[i].movementCooldown = monsters[i].isInCombat ? 12 : 6; // Faster movement
    }
    else if (monsters[i].textureIndex == 5 || monsters[i].textureIndex == 1) // Wizard (1f9d9.png) or Dragon (1f409.png) - ranged attacks
    {
      // Occasionally shoot projectiles
      if (rand() % 100 < 30) // 30% chance per frame (increased for better visibility)
      {
        // Calculate precise direction towards player
        float dx = player.x - monsters[i].x;
        float dy = player.y - monsters[i].y;
        float dist = sqrt(dx * dx + dy * dy);

        if (dist > 0)
        {
          // Normalize direction and multiply by projectile speed for consistent movement
          float normalizedDx = dx / dist;
          float normalizedDy = dy / dist;

          int type = (monsters[i].textureIndex == 5) ? 0 : 1; // 0=lightning for wizard, 1=fireball for dragon
          spawnProjectile(monsters[i].x, monsters[i].y, normalizedDx, normalizedDy, type, monsters[i].power);
        }
      }

      // Run away from player instead of random movement
      float dx = monsters[i].x - player.x; // Reverse direction - away from player
      float dy = monsters[i].y - player.y;
      float dist = sqrt(dx * dx + dy * dy);
      if (dist > 0)
      {
        // Move away from player
        newX += (int)(dx / dist * 1);
        newY += (int)(dy / dist * 1);
      }

      monsters[i].movementCooldown = monsters[i].isInCombat ? 18 : 9; // Slightly faster when running away
    }
    else // Other monsters - random movement
    {
      // Random movement
      Direction dir = rand() % 4;
      switch (dir)
      {
      case UP:
        newY--;
        break;
      case DOWN:
        newY++;
        break;
      case LEFT:
        newX--;
        break;
      case RIGHT:
        newX++;
        break;
      }
      monsters[i].movementCooldown = monsters[i].isInCombat ? 24 : 12; // 50% slower when fighting
    }

    // No bounds checking - unlimited world!
    monsters[i].x = newX;
    monsters[i].y = newY;
  }

  // Despawn monsters that are too far from player
  const int DESPAWN_DISTANCE = 300;  // Very large despawn distance for maximum stability
  const int MIN_NEARBY_MONSTERS = 6; // Keep at least 6 monsters nearby (consistent with spawn logic)

  int nearbyMonsterCount = 0;
  for (int i = 0; i < monsterCount; i++)
  {
    if (!monsters[i].alive)
      continue;

    float dx = monsters[i].x - player.x;
    float dy = monsters[i].y - player.y;
    float distance = sqrt(dx * dx + dy * dy);

    if (distance <= DESPAWN_DISTANCE)
    {
      nearbyMonsterCount++;
    }
  }

  // Despawn distant monsters if we have enough nearby ones
  if (nearbyMonsterCount >= MIN_NEARBY_MONSTERS)
  {
    for (int i = monsterCount - 1; i >= 0; i--)
    {
      if (!monsters[i].alive)
        continue;

      float dx = monsters[i].x - player.x;
      float dy = monsters[i].y - player.y;
      float distance = sqrt(dx * dx + dy * dy);

      if (distance > DESPAWN_DISTANCE)
      {
        // Remove this monster by moving the last monster to this position
        monsters[i] = monsters[monsterCount - 1];
        monsterCount--;
      }
    }
  }
}
