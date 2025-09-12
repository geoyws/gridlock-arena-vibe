#include "types.h"
#include "globals.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Function prototypes for functions called before definition
void unloadChunkEntities(int chunkIndex);
void generateChunkContent(int chunkX, int chunkY);
void generateChunkTerrain(int chunkX, int chunkY);

// World position conversion
WorldPosition worldToChunk(int worldX, int worldY)
{
  WorldPosition pos;
  pos.x = worldX;
  pos.y = worldY;

  // Handle negative coordinates properly
  if (worldX < 0)
  {
    pos.chunkX = (worldX - CHUNK_SIZE + 1) / CHUNK_SIZE;
    pos.localX = (worldX % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
  }
  else
  {
    pos.chunkX = worldX / CHUNK_SIZE;
    pos.localX = worldX % CHUNK_SIZE;
  }

  if (worldY < 0)
  {
    pos.chunkY = (worldY - CHUNK_SIZE + 1) / CHUNK_SIZE;
    pos.localY = (worldY % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
  }
  else
  {
    pos.chunkY = worldY / CHUNK_SIZE;
    pos.localY = worldY % CHUNK_SIZE;
  }

  return pos;
}

// Chunk management
int getChunkIndex(int chunkX, int chunkY)
{
  for (int i = 0; i < loadedChunkCount; i++)
  {
    if (loadedChunks[i].chunkX == chunkX && loadedChunks[i].chunkY == chunkY)
    {
      return i;
    }
  }
  return -1;
}

int loadChunk(int chunkX, int chunkY)
{
  // Check if chunk is already loaded
  if (getChunkIndex(chunkX, chunkY) != -1)
    return 1;

  // If we're at max capacity, unload the least recently used chunk
  if (loadedChunkCount >= MAX_LOADED_CHUNKS)
  {
    int oldestIndex = 0;
    int oldestAccess = loadedChunks[0].lastAccess;

    for (int i = 1; i < loadedChunkCount; i++)
    {
      if (loadedChunks[i].lastAccess < oldestAccess)
      {
        oldestAccess = loadedChunks[i].lastAccess;
        oldestIndex = i;
      }
    }

    // Remove entities in the chunk being unloaded
    unloadChunkEntities(oldestIndex);

    // Replace the oldest chunk
    loadedChunks[oldestIndex].chunkX = chunkX;
    loadedChunks[oldestIndex].chunkY = chunkY;
    loadedChunks[oldestIndex].loaded = 1;
    loadedChunks[oldestIndex].lastAccess = (int)(GetTime() * 1000);

    // Generate content for the new chunk
    generateChunkContent(chunkX, chunkY);

    return 1;
  }

  // Load new chunk
  loadedChunks[loadedChunkCount].chunkX = chunkX;
  loadedChunks[loadedChunkCount].chunkY = chunkY;
  loadedChunks[loadedChunkCount].loaded = 1;
  loadedChunks[loadedChunkCount].lastAccess = (int)(GetTime() * 1000);
  loadedChunkCount++;

  // Generate content for this chunk
  generateChunkContent(chunkX, chunkY);

  return 1;
}

void unloadChunkEntities(int chunkIndex)
{
  int chunkX = loadedChunks[chunkIndex].chunkX;
  int chunkY = loadedChunks[chunkIndex].chunkY;

  // Remove monsters in this chunk (but keep ones near player)
  for (int i = 0; i < monsterCount; i++)
  {
    WorldPosition pos = worldToChunk(monsters[i].x, monsters[i].y);
    if (pos.chunkX == chunkX && pos.chunkY == chunkY)
    {
      float dx = monsters[i].x - player.x;
      float dy = monsters[i].y - player.y;
      float distance = sqrt(dx * dx + dy * dy);

      if (distance > 200)
      {
        monsters[i] = monsters[--monsterCount];
        i--;
      }
    }
  }

  // Remove powerups in this chunk (but keep ones near player)
  for (int i = 0; i < powerupCount; i++)
  {
    WorldPosition pos = worldToChunk(powerups[i].x, powerups[i].y);
    if (pos.chunkX == chunkX && pos.chunkY == chunkY)
    {
      float dx = powerups[i].x - player.x;
      float dy = powerups[i].y - player.y;
      float distance = sqrt(dx * dx + dy * dy);

      if (distance > 200)
      {
        powerups[i] = powerups[--powerupCount];
        i--;
      }
    }
  }

  // Remove landmines in this chunk (but keep ones near player)
  for (int i = 0; i < landmineCount; i++)
  {
    WorldPosition pos = worldToChunk(landmines[i].x, landmines[i].y);
    if (pos.chunkX == chunkX && pos.chunkY == chunkY)
    {
      float dx = landmines[i].x - player.x;
      float dy = landmines[i].y - player.y;
      float distance = sqrt(dx * dx + dy * dy);

      if (distance > 200)
      {
        landmines[i] = landmines[--landmineCount];
        i--;
      }
    }
  }
}

void generateChunkTerrain(int chunkX, int chunkY)
{
  int chunkIndex = getChunkIndex(chunkX, chunkY);
  if (chunkIndex == -1)
    return;

  // Use chunk coordinates as seed for consistent terrain generation
  srand(chunkX * 10000 + chunkY);

  for (int x = 0; x < CHUNK_SIZE; x++)
  {
    for (int y = 0; y < CHUNK_SIZE; y++)
    {
      // Generate terrain based on noise-like patterns
      int worldX = chunkX * CHUNK_SIZE + x;
      int worldY = chunkY * CHUNK_SIZE + y;

      // Create varied terrain using multiple noise layers
      float noise1 = sin(worldX * 0.01f) * cos(worldY * 0.01f);
      float noise2 = sin(worldX * 0.05f + worldY * 0.03f) * 0.5f;
      float noise3 = (rand() % 100) / 100.0f * 0.3f;

      float combinedNoise = noise1 + noise2 + noise3;

      // Determine terrain type based on noise
      if (combinedNoise > 0.8f)
        loadedChunks[chunkIndex].terrain[x][y] = 1; // Mountain
      else if (combinedNoise > 0.3f)
        loadedChunks[chunkIndex].terrain[x][y] = 2; // Tree
      else if (combinedNoise > -0.2f)
        loadedChunks[chunkIndex].terrain[x][y] = 0; // Grass
      else if (combinedNoise > -0.8f)
        loadedChunks[chunkIndex].terrain[x][y] = 3; // Lake
      else
        loadedChunks[chunkIndex].terrain[x][y] = 4; // Sea
    }
  }
}

void generateChunkContent(int chunkX, int chunkY)
{
  // Generate terrain first
  generateChunkTerrain(chunkX, chunkY);

  // Get chunk index for terrain access
  int chunkIndex = getChunkIndex(chunkX, chunkY);
  if (chunkIndex == -1)
    return;

  // Generate entities based on terrain
  for (int x = 0; x < CHUNK_SIZE; x++)
  {
    for (int y = 0; y < CHUNK_SIZE; y++)
    {
      int worldX = chunkX * CHUNK_SIZE + x;
      int worldY = chunkY * CHUNK_SIZE + y;
      int terrainType = loadedChunks[chunkIndex].terrain[x][y];

      // Spawn entities based on terrain type and random chance
      if (rand() % 100 < 2) // 2% chance per cell
      {
        if (terrainType == 0 || terrainType == 2) // Grass or trees - spawn monsters
        {
          if (monsterCount < MAX_MONSTERS)
          {
            monsters[monsterCount].x = worldX;
            monsters[monsterCount].y = worldY;
            monsters[monsterCount].health = 20 + rand() % 30;
            monsters[monsterCount].maxHealth = monsters[monsterCount].health;
            monsters[monsterCount].power = 3 + rand() % 5;
            monsters[monsterCount].textureIndex = 1 + (rand() % 5);
            monsters[monsterCount].alive = 1;
            monsters[monsterCount].speed = 1;
            monsters[monsterCount].speedMultiplier = 1.0f;
            monsters[monsterCount].damageMultiplier = 1.0f;
            monsters[monsterCount].powerupTimer = 0;
            monsters[monsterCount].movementCooldown = 0;
            monsters[monsterCount].level = 1;
            monsters[monsterCount].experience = 0;
            monsters[monsterCount].experienceToNext = 100;
            monsters[monsterCount].isInCombat = 0;
            monsters[monsterCount].invulnerabilityTimer = 0;
            monsters[monsterCount].stunTimer = 0;
            monsters[monsterCount].dotTimer = 0;
            monsters[monsterCount].dotDamage = 0;
            monsters[monsterCount].speedBoostTimer = 0;
            monsters[monsterCount].deathTimer = 0;
            monsterCount++;
          }
        }
        else if (terrainType == 1) // Mountains - spawn powerups
        {
          if (powerupCount < MAX_POWERUPS)
          {
            powerups[powerupCount].x = worldX;
            powerups[powerupCount].y = worldY;
            powerups[powerupCount].type = rand() % POWERUP_COUNT;
            powerups[powerupCount].active = 1;
            powerupCount++;
          }
        }
        else if (terrainType == 3 || terrainType == 4) // Lakes or seas - spawn landmines
        {
          if (landmineCount < MAX_LANDMINES)
          {
            landmines[landmineCount].x = worldX;
            landmines[landmineCount].y = worldY;
            landmines[landmineCount].damage = 15 + rand() % 10;
            landmines[landmineCount].active = 1;
            landmineCount++;
          }
        }
      }
    }
  }
}

void updateChunks()
{
  // Load chunks around player
  int playerChunkX = player.x / CHUNK_SIZE;
  int playerChunkY = player.y / CHUNK_SIZE;

  for (int dx = -CHUNK_LOAD_DISTANCE; dx <= CHUNK_LOAD_DISTANCE; dx++)
  {
    for (int dy = -CHUNK_LOAD_DISTANCE; dy <= CHUNK_LOAD_DISTANCE; dy++)
    {
      loadChunk(playerChunkX + dx, playerChunkY + dy);
    }
  }

  // Update last access times for loaded chunks
  for (int i = 0; i < loadedChunkCount; i++)
  {
    if (loadedChunks[i].loaded)
    {
      loadedChunks[i].lastAccess = (int)(GetTime() * 1000);
    }
  }
}

void ensureNearbyMonsters()
{
  int playerChunkX = player.x / CHUNK_SIZE;
  int playerChunkY = player.y / CHUNK_SIZE;

  // Count monsters in nearby chunks
  int nearbyMonsterCount = 0;
  for (int i = 0; i < monsterCount; i++)
  {
    if (monsters[i].alive)
    {
      int monsterChunkX = monsters[i].x / CHUNK_SIZE;
      int monsterChunkY = monsters[i].y / CHUNK_SIZE;
      int dx = abs(monsterChunkX - playerChunkX);
      int dy = abs(monsterChunkY - playerChunkY);

      if (dx <= 2 && dy <= 2) // Within 2 chunks
      {
        nearbyMonsterCount++;
      }
    }
  }

  // Spawn more monsters if needed
  while (nearbyMonsterCount < 50 && monsterCount < MAX_MONSTERS)
  {
    // Find a random position in nearby chunks
    int offsetX = (rand() % 5) - 2; // -2 to +2 chunks
    int offsetY = (rand() % 5) - 2;
    int spawnChunkX = playerChunkX + offsetX;
    int spawnChunkY = playerChunkY + offsetY;

    // Ensure chunk is loaded
    loadChunk(spawnChunkX, spawnChunkY);

    // Find a suitable spawn position
    for (int attempts = 0; attempts < 10; attempts++)
    {
      int localX = rand() % CHUNK_SIZE;
      int localY = rand() % CHUNK_SIZE;
      int worldX = spawnChunkX * CHUNK_SIZE + localX;
      int worldY = spawnChunkY * CHUNK_SIZE + localY;

      // Check distance from player
      float dx = worldX - player.x;
      float dy = worldY - player.y;
      float distance = sqrt(dx * dx + dy * dy);

      if (distance > 50 && distance < 300) // Not too close, not too far
      {
        // Check if position is free
        int positionFree = 1;
        for (int j = 0; j < monsterCount; j++)
        {
          if (monsters[j].alive &&
              abs(monsters[j].x - worldX) <= 1 &&
              abs(monsters[j].y - worldY) <= 1)
          {
            positionFree = 0;
            break;
          }
        }

        if (positionFree)
        {
          monsters[monsterCount].x = worldX;
          monsters[monsterCount].y = worldY;
          monsters[monsterCount].health = 20 + rand() % 30;
          monsters[monsterCount].maxHealth = monsters[monsterCount].health;
          monsters[monsterCount].power = 3 + rand() % 5;
          monsters[monsterCount].textureIndex = 1 + (rand() % 5);
          monsters[monsterCount].alive = 1;
          monsters[monsterCount].speed = 1;
          monsters[monsterCount].speedMultiplier = 1.0f;
          monsters[monsterCount].damageMultiplier = 1.0f;
          monsters[monsterCount].powerupTimer = 0;
          monsters[monsterCount].movementCooldown = 0;
          monsters[monsterCount].level = 1;
          monsters[monsterCount].experience = 0;
          monsters[monsterCount].experienceToNext = 100;
          monsters[monsterCount].isInCombat = 0;
          monsters[monsterCount].invulnerabilityTimer = 0;
          monsters[monsterCount].stunTimer = 0;
          monsters[monsterCount].dotTimer = 0;
          monsters[monsterCount].dotDamage = 0;
          monsters[monsterCount].speedBoostTimer = 0;
          monsters[monsterCount].deathTimer = 0;
          monsterCount++;
          nearbyMonsterCount++;
          break;
        }
      }
    }
  }
}

void ensureNearbyPowerups()
{
  int playerChunkX = player.x / CHUNK_SIZE;
  int playerChunkY = player.y / CHUNK_SIZE;

  // Count powerups in nearby chunks
  int nearbyPowerupCount = 0;
  for (int i = 0; i < powerupCount; i++)
  {
    if (powerups[i].active)
    {
      int powerupChunkX = powerups[i].x / CHUNK_SIZE;
      int powerupChunkY = powerups[i].y / CHUNK_SIZE;
      int dx = abs(powerupChunkX - playerChunkX);
      int dy = abs(powerupChunkY - playerChunkY);

      if (dx <= 2 && dy <= 2)
      {
        nearbyPowerupCount++;
      }
    }
  }

  // Spawn more powerups if needed
  while (nearbyPowerupCount < 5 && powerupCount < MAX_POWERUPS)
  {
    int offsetX = (rand() % 5) - 2;
    int offsetY = (rand() % 5) - 2;
    int spawnChunkX = playerChunkX + offsetX;
    int spawnChunkY = playerChunkY + offsetY;

    loadChunk(spawnChunkX, spawnChunkY);

    for (int attempts = 0; attempts < 10; attempts++)
    {
      int localX = rand() % CHUNK_SIZE;
      int localY = rand() % CHUNK_SIZE;
      int worldX = spawnChunkX * CHUNK_SIZE + localX;
      int worldY = spawnChunkY * CHUNK_SIZE + localY;

      float dx = worldX - player.x;
      float dy = worldY - player.y;
      float distance = sqrt(dx * dx + dy * dy);

      if (distance > 100 && distance < 400)
      {
        int positionFree = 1;
        for (int j = 0; j < powerupCount; j++)
        {
          if (powerups[j].active &&
              abs(powerups[j].x - worldX) <= 2 &&
              abs(powerups[j].y - worldY) <= 2)
          {
            positionFree = 0;
            break;
          }
        }

        if (positionFree)
        {
          powerups[powerupCount].x = worldX;
          powerups[powerupCount].y = worldY;
          powerups[powerupCount].type = rand() % POWERUP_COUNT;
          powerups[powerupCount].active = 1;
          powerupCount++;
          nearbyPowerupCount++;
          break;
        }
      }
    }
  }
}

void ensureNearbyLandmines()
{
  int playerChunkX = player.x / CHUNK_SIZE;
  int playerChunkY = player.y / CHUNK_SIZE;

  // Count landmines in nearby chunks
  int nearbyLandmineCount = 0;
  for (int i = 0; i < landmineCount; i++)
  {
    if (landmines[i].active)
    {
      int landmineChunkX = landmines[i].x / CHUNK_SIZE;
      int landmineChunkY = landmines[i].y / CHUNK_SIZE;
      int dx = abs(landmineChunkX - playerChunkX);
      int dy = abs(landmineChunkY - playerChunkY);

      if (dx <= 2 && dy <= 2)
      {
        nearbyLandmineCount++;
      }
    }
  }

  // Spawn more landmines if needed
  while (nearbyLandmineCount < 10 && landmineCount < MAX_LANDMINES)
  {
    int offsetX = (rand() % 5) - 2;
    int offsetY = (rand() % 5) - 2;
    int spawnChunkX = playerChunkX + offsetX;
    int spawnChunkY = playerChunkY + offsetY;

    loadChunk(spawnChunkX, spawnChunkY);

    for (int attempts = 0; attempts < 10; attempts++)
    {
      int localX = rand() % CHUNK_SIZE;
      int localY = rand() % CHUNK_SIZE;
      int worldX = spawnChunkX * CHUNK_SIZE + localX;
      int worldY = spawnChunkY * CHUNK_SIZE + localY;

      float dx = worldX - player.x;
      float dy = worldY - player.y;
      float distance = sqrt(dx * dx + dy * dy);

      if (distance > 150 && distance < 500)
      {
        int positionFree = 1;
        for (int j = 0; j < landmineCount; j++)
        {
          if (landmines[j].active &&
              abs(landmines[j].x - worldX) <= 3 &&
              abs(landmines[j].y - worldY) <= 3)
          {
            positionFree = 0;
            break;
          }
        }

        if (positionFree)
        {
          landmines[landmineCount].x = worldX;
          landmines[landmineCount].y = worldY;
          landmines[landmineCount].damage = 15 + rand() % 10;
          landmines[landmineCount].active = 1;
          landmineCount++;
          nearbyLandmineCount++;
          break;
        }
      }
    }
  }
}

void drawWorld()
{
  // Get camera bounds to determine what to draw
  int camLeft = (camera.target.x - WINDOW_SIZE / 2) / CELL_SIZE - 1;
  int camRight = (camera.target.x + WINDOW_SIZE / 2) / CELL_SIZE + 1;
  int camTop = (camera.target.y - WINDOW_SIZE / 2) / CELL_SIZE - 1;
  int camBottom = (camera.target.y + WINDOW_SIZE / 2) / CELL_SIZE + 1;

  // Draw terrain
  for (int worldX = camLeft; worldX <= camRight; worldX++)
  {
    for (int worldY = camTop; worldY <= camBottom; worldY++)
    {
      WorldPosition pos = worldToChunk(worldX, worldY);
      int chunkIndex = getChunkIndex(pos.chunkX, pos.chunkY);

      if (chunkIndex != -1)
      {
        int terrainType = loadedChunks[chunkIndex].terrain[pos.localX][pos.localY];
        Color terrainColor;

        switch (terrainType)
        {
        case 0:
          terrainColor = (Color){80, 140, 80, 220};
          break; // Grass
        case 1:
          terrainColor = (Color){140, 110, 80, 220};
          break; // Mountain
        case 2:
          terrainColor = (Color){50, 110, 50, 220};
          break; // Tree
        case 3:
          terrainColor = (Color){80, 140, 200, 220};
          break; // Lake
        case 4:
          terrainColor = (Color){50, 80, 140, 220};
          break; // Sea
        default:
          terrainColor = (Color){80, 140, 80, 220};
          break; // Default
        }

        DrawRectangle(worldX * CELL_SIZE, worldY * CELL_SIZE, CELL_SIZE, CELL_SIZE, terrainColor);
      }
      else
      {
        DrawRectangle(worldX * CELL_SIZE, worldY * CELL_SIZE, CELL_SIZE, CELL_SIZE, (Color){80, 140, 80, 220});
      }
    }
  }

  // Draw grid
  for (int x = camLeft; x <= camRight; x++)
  {
    Vector2 start = {x * CELL_SIZE, camTop * CELL_SIZE};
    Vector2 end = {x * CELL_SIZE, camBottom * CELL_SIZE};
    DrawLineV(start, end, LIGHTGRAY);
  }
  for (int y = camTop; y <= camBottom; y++)
  {
    Vector2 start = {camLeft * CELL_SIZE, y * CELL_SIZE};
    Vector2 end = {camRight * CELL_SIZE, y * CELL_SIZE};
    DrawLineV(start, end, LIGHTGRAY);
  }

  // Draw landmines
  for (int i = 0; i < landmineCount; i++)
  {
    if (landmines[i].active)
    {
      DrawCircle(landmines[i].x * CELL_SIZE + CELL_SIZE / 2,
                 landmines[i].y * CELL_SIZE + CELL_SIZE / 2,
                 CELL_SIZE / 4, RED);
    }
  }

  // Draw powerups
  for (int i = 0; i < powerupCount; i++)
  {
    if (powerups[i].active)
    {
      Color color;
      switch (powerups[i].type)
      {
      case POWERUP_DOUBLE_DAMAGE:
        color = ORANGE;
        break;
      case POWERUP_DOUBLE_HEALTH:
        color = GREEN;
        break;
      case POWERUP_DOUBLE_SPEED:
        color = BLUE;
        break;
      default:
        color = WHITE;
        break;
      }
      DrawCircle(powerups[i].x * CELL_SIZE + CELL_SIZE / 2,
                 powerups[i].y * CELL_SIZE + CELL_SIZE / 2,
                 CELL_SIZE / 3, color);
    }
  }

  // Draw monsters
  for (int i = 0; i < monsterCount; i++)
  {
    if (monsters[i].alive)
    {
      float screenX = monsters[i].x * CELL_SIZE - camera.target.x + camera.offset.x;
      float screenY = monsters[i].y * CELL_SIZE - camera.target.y + camera.offset.y;

      if (screenX >= -CELL_SIZE && screenX <= WINDOW_SIZE + CELL_SIZE &&
          screenY >= -CELL_SIZE && screenY <= WINDOW_SIZE + CELL_SIZE)
      {
        DrawTextureEx(textures[monsters[i].textureIndex],
                      (Vector2){monsters[i].x * CELL_SIZE, monsters[i].y * CELL_SIZE},
                      0, CELL_SIZE / 72.0f, WHITE);

        // Health bar
        DrawRectangle(monsters[i].x * CELL_SIZE, monsters[i].y * CELL_SIZE - 5,
                      CELL_SIZE, 3, RED);
        DrawRectangle(monsters[i].x * CELL_SIZE, monsters[i].y * CELL_SIZE - 5,
                      (float)monsters[i].health / monsters[i].maxHealth * CELL_SIZE, 3, GREEN);
      }
    }
  }

  // Draw player
  if (player.alive)
  {
    Color playerColor = WHITE;
    if (player.invulnerabilityTimer > 0)
    {
      if ((player.invulnerabilityTimer / 10) % 2 == 0)
      {
        playerColor = YELLOW;
      }
      else
      {
        playerColor = Fade(WHITE, 0.3f);
      }
    }

    DrawTextureEx(textures[player.textureIndex],
                  (Vector2){player.x * CELL_SIZE, player.y * CELL_SIZE},
                  0, CELL_SIZE / 72.0f, playerColor);

    // Health bar
    DrawRectangle(player.x * CELL_SIZE, player.y * CELL_SIZE - 5,
                  CELL_SIZE, 3, RED);
    DrawRectangle(player.x * CELL_SIZE, player.y * CELL_SIZE - 5,
                  (float)player.health / player.maxHealth * CELL_SIZE, 3, GREEN);
  }
}

void drawMinimap()
{
  int minimapSize = 120;
  int minimapX = WINDOW_SIZE - minimapSize - 10;
  int minimapY = WINDOW_SIZE - minimapSize - 10;
  int minimapScale = 3;

  // Draw minimap background
  DrawRectangle(minimapX, minimapY, minimapSize, minimapSize, (Color){0, 0, 0, 180});
  DrawRectangleLines(minimapX, minimapY, minimapSize, minimapSize, WHITE);

  // Calculate minimap bounds
  int minimapCenterX = player.x;
  int minimapCenterY = player.y;
  int minimapHalfSize = (minimapSize / 2) * minimapScale;

  int minimapLeft = minimapCenterX - minimapHalfSize;
  int minimapRight = minimapCenterX + minimapHalfSize;
  int minimapTop = minimapCenterY - minimapHalfSize;
  int minimapBottom = minimapCenterY + minimapHalfSize;

  // Draw terrain on minimap
  for (int worldX = minimapLeft; worldX <= minimapRight; worldX += minimapScale)
  {
    for (int worldY = minimapTop; worldY <= minimapBottom; worldY += minimapScale)
    {
      WorldPosition pos = worldToChunk(worldX, worldY);
      int chunkIndex = getChunkIndex(pos.chunkX, pos.chunkY);

      if (chunkIndex != -1)
      {
        int terrainType = loadedChunks[chunkIndex].terrain[pos.localX][pos.localY];
        Color terrainColor;

        switch (terrainType)
        {
        case 0:
          terrainColor = (Color){80, 140, 80, 255};
          break;
        case 1:
          terrainColor = (Color){140, 110, 80, 255};
          break;
        case 2:
          terrainColor = (Color){50, 110, 50, 255};
          break;
        case 3:
          terrainColor = (Color){80, 140, 200, 255};
          break;
        case 4:
          terrainColor = (Color){50, 80, 140, 255};
          break;
        default:
          terrainColor = (Color){80, 140, 80, 255};
          break;
        }

        int minimapPixelX = minimapX + ((worldX - minimapLeft) / minimapScale);
        int minimapPixelY = minimapY + ((worldY - minimapTop) / minimapScale);

        if (minimapPixelX >= minimapX && minimapPixelX < minimapX + minimapSize &&
            minimapPixelY >= minimapY && minimapPixelY < minimapY + minimapSize)
        {
          DrawRectangle(minimapPixelX, minimapPixelY, 1, 1, terrainColor);
        }
      }
    }
  }

  // Draw player on minimap
  int playerMinimapX = minimapX + (minimapSize / 2);
  int playerMinimapY = minimapY + (minimapSize / 2);
  DrawRectangle(playerMinimapX - 1, playerMinimapY - 1, 3, 3, YELLOW);

  // Draw monsters on minimap
  for (int i = 0; i < monsterCount; i++)
  {
    int monsterMinimapX = minimapX + (minimapSize / 2) + ((monsters[i].x - player.x) / minimapScale);
    int monsterMinimapY = minimapY + (minimapSize / 2) + ((monsters[i].y - player.y) / minimapScale);

    if (monsterMinimapX >= minimapX && monsterMinimapX < minimapX + minimapSize &&
        monsterMinimapY >= minimapY && monsterMinimapY < minimapY + minimapSize)
    {
      DrawRectangle(monsterMinimapX, monsterMinimapY, 1, 1, RED);
    }
  }

  // Draw powerups on minimap
  for (int i = 0; i < powerupCount; i++)
  {
    int powerupMinimapX = minimapX + (minimapSize / 2) + ((powerups[i].x - player.x) / minimapScale);
    int powerupMinimapY = minimapY + (minimapSize / 2) + ((powerups[i].y - player.y) / minimapScale);

    if (powerupMinimapX >= minimapX && powerupMinimapX < minimapX + minimapSize &&
        powerupMinimapY >= minimapY && powerupMinimapY < minimapY + minimapSize)
    {
      DrawRectangle(powerupMinimapX, powerupMinimapY, 1, 1, GREEN);
    }
  }

  // Draw landmines on minimap
  for (int i = 0; i < landmineCount; i++)
  {
    int landmineMinimapX = minimapX + (minimapSize / 2) + ((landmines[i].x - player.x) / minimapScale);
    int landmineMinimapY = minimapY + (minimapSize / 2) + ((landmines[i].y - player.y) / minimapScale);

    if (landmineMinimapX >= minimapX && landmineMinimapX < minimapX + minimapSize &&
        landmineMinimapY >= minimapY && landmineMinimapY < minimapY + minimapSize)
    {
      DrawRectangle(landmineMinimapX, landmineMinimapY, 1, 1, ORANGE);
    }
  }
}
