#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define WORLD_SIZE 100
#define CELL_SIZE 20
#define WINDOW_SIZE 800
#define MAX_MONSTERS 2000
#define MAX_POWERUPS 50
#define MAX_LANDMINES 100

// Chunk system constants
#define CHUNK_SIZE 32
#define CHUNK_CELL_SIZE (CHUNK_SIZE * CELL_SIZE)
#define MAX_LOADED_CHUNKS 625  // 25x25 grid of chunks around player
#define CHUNK_LOAD_DISTANCE 12 // Much larger loading distance to prevent chunk unloading

// Chunk system structures
typedef struct
{
  int chunkX, chunkY;                  // Chunk coordinates
  int loaded;                          // Whether this chunk is currently loaded
  int lastAccess;                      // Frame counter for LRU cache
  int terrain[CHUNK_SIZE][CHUNK_SIZE]; // Terrain data for each cell
  // Terrain types: 0=grass, 1=mountain, 2=tree, 3=lake, 4=sea
} Chunk;

typedef struct
{
  int x, y;           // World coordinates
  int chunkX, chunkY; // Chunk coordinates
  int localX, localY; // Local coordinates within chunk
} WorldPosition;

// Function prototypes
void initGame();
void spawnMonster();
void spawnPowerup();
void spawnLandmine();
void updatePlayer();
void updateMonsters();
void checkCollisions();
void updatePowerups();
void updateLandmines();
void drawWorld();
void drawUI();
void drawMinimap();
WorldPosition worldToChunk(int worldX, int worldY);
int getChunkIndex(int chunkX, int chunkY);
int loadChunk(int chunkX, int chunkY);
void unloadChunkEntities(int chunkIndex);
void generateChunkContent(int chunkX, int chunkY);
void updateChunks();
void ensureNearbyMonsters();
void ensureNearbyPowerups();
void ensureNearbyLandmines();
void generateChunkTerrain(int chunkX, int chunkY);

typedef enum
{
  UP,
  DOWN,
  LEFT,
  RIGHT
} Direction;

typedef enum
{
  POWERUP_DOUBLE_DAMAGE,
  POWERUP_DOUBLE_HEALTH,
  POWERUP_DOUBLE_SPEED,
  POWERUP_COUNT
} PowerupType;

typedef struct
{
  char name[20];
  int x, y;
  int health;
  int maxHealth;
  int power;
  int textureIndex;
  int alive;
  int speed;
  float speedMultiplier;
  float damageMultiplier;
  int powerupTimer;
  PowerupType activePowerup;
  int movementCooldown;
  int level;
  int experience;
  int experienceToNext;
  int isInCombat;
  int invulnerabilityTimer; // 3 seconds of invulnerability after spawning
} Character;

typedef struct
{
  int x, y;
  PowerupType type;
  int active;
} Powerup;

typedef struct
{
  int x, y;
  int damage;
  int active;
} Landmine;

Character player;
Character monsters[MAX_MONSTERS];
Powerup powerups[MAX_POWERUPS];
Landmine landmines[MAX_LANDMINES];
Chunk loadedChunks[MAX_LOADED_CHUNKS];
int loadedChunkCount = 0;
int monsterCount = 0;
int powerupCount = 0;
int landmineCount = 0;

Texture2D textures[10]; // Player + monsters + powerups + landmines
Sound sounds[10];       // Various sound effects
Camera2D camera = {0};

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
}

void spawnMonster()
{
  if (monsterCount >= MAX_MONSTERS)
    return;

  Character *monster = &monsters[monsterCount++];
  strcpy(monster->name, "Monster");

  // Spawn within 50-100 cells of player for unlimited world
  int distance = 50 + rand() % 50;
  float angle = (rand() % 360) * DEG2RAD;
  monster->x = player.x + (int)(cos(angle) * distance);
  monster->y = player.y + (int)(sin(angle) * distance);

  monster->health = 20 + rand() % 30; // 20-50 health
  monster->maxHealth = monster->health;
  monster->power = 3 + rand() % 5;          // 3-7 power
  monster->textureIndex = 1 + (rand() % 4); // Random monster texture
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
  monster->invulnerabilityTimer = 0; // Monsters don't need invulnerability
}

void spawnPowerup()
{
  if (powerupCount >= MAX_POWERUPS)
    return;

  Powerup *powerup = &powerups[powerupCount++];

  // Spawn within 30-80 cells of player for unlimited world
  int distance = 30 + rand() % 50;
  float angle = (rand() % 360) * DEG2RAD;
  powerup->x = player.x + (int)(cos(angle) * distance);
  powerup->y = player.y + (int)(sin(angle) * distance);

  powerup->type = rand() % POWERUP_COUNT;
  powerup->active = 1;
}

void spawnLandmine()
{
  if (landmineCount >= MAX_LANDMINES)
    return;

  Landmine *landmine = &landmines[landmineCount++];

  // Spawn within 40-90 cells of player for unlimited world
  int distance = 40 + rand() % 50;
  float angle = (rand() % 360) * DEG2RAD;
  landmine->x = player.x + (int)(cos(angle) * distance);
  landmine->y = player.y + (int)(sin(angle) * distance);

  landmine->damage = 10 + rand() % 20; // 10-30 damage
  landmine->active = 1;
}

// Chunk system utility functions
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
    loadedChunks[oldestIndex].lastAccess = (int)(GetTime() * 1000); // Current time in ms

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
      // Check if monster is close to player - don't remove if nearby
      float dx = monsters[i].x - player.x;
      float dy = monsters[i].y - player.y;
      float distance = sqrt(dx * dx + dy * dy);

      if (distance > 200) // Only remove monsters more than 200 units from player
      {
        // Remove this monster by swapping with the last one
        monsters[i] = monsters[--monsterCount];
        i--; // Recheck this index
      }
    }
  }

  // Remove powerups in this chunk (but keep ones near player)
  for (int i = 0; i < powerupCount; i++)
  {
    WorldPosition pos = worldToChunk(powerups[i].x, powerups[i].y);
    if (pos.chunkX == chunkX && pos.chunkY == chunkY)
    {
      // Check if powerup is close to player - don't remove if nearby
      float dx = powerups[i].x - player.x;
      float dy = powerups[i].y - player.y;
      float distance = sqrt(dx * dx + dy * dy);

      if (distance > 200) // Only remove powerups more than 200 units from player
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
      // Check if landmine is close to player - don't remove if nearby
      float dx = landmines[i].x - player.x;
      float dy = landmines[i].y - player.y;
      float distance = sqrt(dx * dx + dy * dy);

      if (distance > 200) // Only remove landmines more than 200 units from player
      {
        landmines[i] = landmines[--landmineCount];
        i--;
      }
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

  // Generate monsters for this chunk with terrain-based spawning
  int monstersInChunk = 25 + rand() % 16; // 25-40 monsters per chunk for 20% screen coverage
  for (int i = 0; i < monstersInChunk; i++)
  {
    if (monsterCount >= MAX_MONSTERS)
      break;

    Character *monster = &monsters[monsterCount++];
    int attempts = 0;
    int localX, localY, worldX, worldY, terrainType;

    // Try to find a suitable spawn location based on terrain
    do
    {
      localX = rand() % CHUNK_SIZE;
      localY = rand() % CHUNK_SIZE;
      worldX = chunkX * CHUNK_SIZE + localX;
      worldY = chunkY * CHUNK_SIZE + localY;
      terrainType = loadedChunks[chunkIndex].terrain[localX][localY];
      attempts++;
    } while (((worldX == 0 && worldY == 0) || (worldX == player.x && worldY == player.y)) && attempts < 50); // Don't spawn on player start position

    monster->x = worldX;
    monster->y = worldY;

    // No bounds checking - unlimited world!

    // Set monster properties based on terrain type
    switch (terrainType)
    {
    case 0:                                   // Grass - standard monsters
      monster->health = 20 + rand() % 30;     // 20-50 health
      monster->power = 3 + rand() % 5;        // 3-8 power
      monster->textureIndex = 1 + rand() % 5; // Random monster texture
      break;
    case 1:                               // Mountains - stronger monsters
      monster->health = 30 + rand() % 40; // 30-70 health
      monster->power = 5 + rand() % 6;    // 5-11 power
      monster->textureIndex = 1;          // Dragon for mountains
      break;
    case 2:                               // Trees - agile monsters
      monster->health = 15 + rand() % 25; // 15-40 health
      monster->power = 4 + rand() % 4;    // 4-8 power
      monster->textureIndex = 2;          // Ghost for forests
      break;
    case 3:                               // Lakes - aquatic monsters
      monster->health = 25 + rand() % 35; // 25-60 health
      monster->power = 3 + rand() % 6;    // 3-9 power
      monster->textureIndex = 3;          // Special lake monster
      break;
    case 4:                               // Sea - powerful monsters
      monster->health = 35 + rand() % 45; // 35-80 health
      monster->power = 6 + rand() % 7;    // 6-13 power
      monster->textureIndex = 4;          // Powerful sea monster
      break;
    default:
      monster->health = 20 + rand() % 30;
      monster->power = 3 + rand() % 5;
      monster->textureIndex = 1 + rand() % 5;
      break;
    }

    monster->maxHealth = monster->health;
    monster->alive = 1;
    monster->speed = 1;
    monster->speedMultiplier = 1.0f;
    monster->damageMultiplier = 1.0f;
    monster->powerupTimer = 0;
    monster->movementCooldown = 0;
    monster->level = 1;
    monster->experience = 0;
    monster->experienceToNext = 0; // Monsters don't level up
    monster->isInCombat = 0;
    strcpy(monster->name, "Monster");
  }

  // Generate powerups for this chunk with terrain preferences
  int powerupsInChunk = 3 + rand() % 3; // 3-5 powerups per chunk
  for (int i = 0; i < powerupsInChunk; i++)
  {
    if (powerupCount >= MAX_POWERUPS)
      break;

    Powerup *powerup = &powerups[powerupCount++];
    int attempts = 0;
    int localX, localY, worldX, worldY, terrainType;

    // Try to find a suitable spawn location
    do
    {
      localX = rand() % CHUNK_SIZE;
      localY = rand() % CHUNK_SIZE;
      worldX = chunkX * CHUNK_SIZE + localX;
      worldY = chunkY * CHUNK_SIZE + localY;
      terrainType = loadedChunks[chunkIndex].terrain[localX][localY];
      attempts++;
    } while ((worldX == 0 && worldY == 0) && attempts < 50); // Don't spawn on player start position

    powerup->x = worldX;
    powerup->y = worldY;

    // No bounds checking - unlimited world!

    // Powerup type based on terrain (some terrains get better powerups)
    if (terrainType == 1) // Mountains - better powerups
    {
      powerup->type = rand() % POWERUP_COUNT; // All types possible
    }
    else if (terrainType == 4) // Sea - health powerups more likely
    {
      powerup->type = (rand() % 2 == 0) ? POWERUP_DOUBLE_HEALTH : rand() % POWERUP_COUNT;
    }
    else
    {
      powerup->type = rand() % POWERUP_COUNT; // Random
    }

    powerup->active = 1;
  }

  // Generate landmines for this chunk with terrain preferences
  int landminesInChunk = 3 + rand() % 4; // 3-6 landmines per chunk
  for (int i = 0; i < landminesInChunk; i++)
  {
    if (landmineCount >= MAX_LANDMINES)
      break;

    Landmine *landmine = &landmines[landmineCount++];
    int attempts = 0;
    int localX, localY, worldX, worldY, terrainType;

    // Try to find a suitable spawn location
    do
    {
      localX = rand() % CHUNK_SIZE;
      localY = rand() % CHUNK_SIZE;
      worldX = chunkX * CHUNK_SIZE + localX;
      worldY = chunkY * CHUNK_SIZE + localY;
      terrainType = loadedChunks[chunkIndex].terrain[localX][localY];
      attempts++;
    } while ((worldX == 0 && worldY == 0) && attempts < 50); // Don't spawn on player start position

    landmine->x = worldX;
    landmine->y = worldY;

    // No bounds checking - unlimited world!

    // Landmine damage based on terrain (dangerous areas have stronger mines)
    if (terrainType == 1) // Mountains - very dangerous
    {
      landmine->damage = 15 + rand() % 25; // 15-40 damage
    }
    else if (terrainType == 4) // Sea - dangerous
    {
      landmine->damage = 12 + rand() % 20; // 12-32 damage
    }
    else if (terrainType == 2) // Trees - moderately dangerous
    {
      landmine->damage = 10 + rand() % 15; // 10-25 damage
    }
    else
    {
      landmine->damage = 8 + rand() % 12; // 8-20 damage
    }

    landmine->active = 1;
  }
}

void generateChunkTerrain(int chunkX, int chunkY)
{
  // Find the chunk
  int chunkIndex = getChunkIndex(chunkX, chunkY);
  if (chunkIndex == -1)
    return;

  // Use chunk coordinates as seed for consistent generation
  srand(chunkX * 1000 + chunkY);

  // Generate base terrain with multiple noise layers
  for (int x = 0; x < CHUNK_SIZE; x++)
  {
    for (int y = 0; y < CHUNK_SIZE; y++)
    {
      // Multiple noise layers for more complex terrain
      int noise1 = (rand() % 100);
      int noise2 = (rand() % 100);
      int noise3 = (rand() % 100);
      int noise4 = (rand() % 100);

      // Distance from chunk center affects terrain distribution
      int centerX = CHUNK_SIZE / 2;
      int centerY = CHUNK_SIZE / 2;
      float distanceFromCenter = sqrt((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY));
      float maxDistance = sqrt(centerX * centerX + centerY * centerY);
      float centerFactor = distanceFromCenter / maxDistance;

      // Global biome influence based on chunk coordinates
      int biomeNoise = (abs(chunkX) + abs(chunkY)) % 100;

      // Generate terrain with varied probabilities based on biome
      if (biomeNoise < 20) // Forest biome
      {
        if (noise1 < 3) // 3% mountains
        {
          loadedChunks[chunkIndex].terrain[x][y] = 1; // Mountain
        }
        else if (noise2 < 25 && centerFactor < 0.8) // 25% trees
        {
          loadedChunks[chunkIndex].terrain[x][y] = 2; // Tree
        }
        else if (noise3 < 5) // 5% lakes
        {
          loadedChunks[chunkIndex].terrain[x][y] = 3; // Lake
        }
        else
        {
          loadedChunks[chunkIndex].terrain[x][y] = 0; // Grass
        }
      }
      else if (biomeNoise < 40) // Mountain biome
      {
        if (noise1 < 15) // 15% mountains
        {
          loadedChunks[chunkIndex].terrain[x][y] = 1; // Mountain
        }
        else if (noise2 < 8) // 8% trees
        {
          loadedChunks[chunkIndex].terrain[x][y] = 2; // Tree
        }
        else if (noise3 < 3) // 3% lakes
        {
          loadedChunks[chunkIndex].terrain[x][y] = 3; // Lake
        }
        else
        {
          loadedChunks[chunkIndex].terrain[x][y] = 0; // Grass
        }
      }
      else if (biomeNoise < 60) // Lake biome
      {
        if (noise1 < 2) // 2% mountains
        {
          loadedChunks[chunkIndex].terrain[x][y] = 1; // Mountain
        }
        else if (noise2 < 10) // 10% trees
        {
          loadedChunks[chunkIndex].terrain[x][y] = 2; // Tree
        }
        else if (noise3 < 20) // 20% lakes
        {
          loadedChunks[chunkIndex].terrain[x][y] = 3; // Lake
        }
        else if (noise4 < 5) // 5% sea
        {
          loadedChunks[chunkIndex].terrain[x][y] = 4; // Sea
        }
        else
        {
          loadedChunks[chunkIndex].terrain[x][y] = 0; // Grass
        }
      }
      else // Mixed biome
      {
        if (noise1 < 8) // 8% mountains
        {
          loadedChunks[chunkIndex].terrain[x][y] = 1; // Mountain
        }
        else if (noise2 < 15 && centerFactor < 0.7) // 15% trees
        {
          loadedChunks[chunkIndex].terrain[x][y] = 2; // Tree
        }
        else if (noise3 < 10 && centerFactor > 0.3) // 10% water
        {
          loadedChunks[chunkIndex].terrain[x][y] = 3; // Lake
        }
        else if (noise4 < 3) // 3% sea
        {
          loadedChunks[chunkIndex].terrain[x][y] = 4; // Sea
        }
        else
        {
          loadedChunks[chunkIndex].terrain[x][y] = 0; // Grass
        }
      }
    }
  }

  // Add some clustered features for more interesting terrain
  int clusterCount = rand() % 3; // 0-2 clusters per chunk
  for (int cluster = 0; cluster < clusterCount; cluster++)
  {
    int clusterX = rand() % CHUNK_SIZE;
    int clusterY = rand() % CHUNK_SIZE;
    int clusterSize = 2 + rand() % 4; // 2-5 tiles
    int clusterType = rand() % 4;     // 0=trees, 1=lakes, 2=mountains, 3=sea

    for (int dx = -clusterSize; dx <= clusterSize; dx++)
    {
      for (int dy = -clusterSize; dy <= clusterSize; dy++)
      {
        int nx = clusterX + dx;
        int ny = clusterY + dy;

        if (nx >= 0 && nx < CHUNK_SIZE && ny >= 0 && ny < CHUNK_SIZE)
        {
          float distance = sqrt(dx * dx + dy * dy);
          if (distance <= clusterSize && rand() % 100 < 70) // 70% chance within cluster
          {
            if (clusterType == 0 && loadedChunks[chunkIndex].terrain[nx][ny] == 0)
              loadedChunks[chunkIndex].terrain[nx][ny] = 2; // Trees
            else if (clusterType == 1 && loadedChunks[chunkIndex].terrain[nx][ny] == 0)
              loadedChunks[chunkIndex].terrain[nx][ny] = 3; // Lakes
            else if (clusterType == 2 && loadedChunks[chunkIndex].terrain[nx][ny] == 0)
              loadedChunks[chunkIndex].terrain[nx][ny] = 1; // Mountains
            else if (clusterType == 3 && loadedChunks[chunkIndex].terrain[nx][ny] == 0)
              loadedChunks[chunkIndex].terrain[nx][ny] = 4; // Sea
          }
        }
      }
    }
  }
}

void updateChunks()
{
  // Get player's chunk position
  WorldPosition playerPos = worldToChunk(player.x, player.y);

  // Load chunks around player - UNLIMITED exploration!
  for (int dx = -CHUNK_LOAD_DISTANCE; dx <= CHUNK_LOAD_DISTANCE; dx++)
  {
    for (int dy = -CHUNK_LOAD_DISTANCE; dy <= CHUNK_LOAD_DISTANCE; dy++)
    {
      int chunkX = playerPos.chunkX + dx;
      int chunkY = playerPos.chunkY + dy;

      // No bounds checking - unlimited world!
      loadChunk(chunkX, chunkY);
    }
  }

  // Update access times for chunks around player (not all chunks)
  int currentTime = (int)(GetTime() * 1000);
  for (int dx = -CHUNK_LOAD_DISTANCE; dx <= CHUNK_LOAD_DISTANCE; dx++)
  {
    for (int dy = -CHUNK_LOAD_DISTANCE; dy <= CHUNK_LOAD_DISTANCE; dy++)
    {
      int chunkX = playerPos.chunkX + dx;
      int chunkY = playerPos.chunkY + dy;
      int chunkIndex = getChunkIndex(chunkX, chunkY);
      if (chunkIndex != -1)
      {
        loadedChunks[chunkIndex].lastAccess = currentTime;
      }
    }
  }
}

void ensureNearbyMonsters()
{
  const int MIN_NEARBY_DISTANCE = 3;   // Reduced for more responsive spawning
  const int MAX_NEARBY_DISTANCE = 300; // Match despawn distance for consistency
  const int MIN_NEARBY_MONSTERS = 20;  // Much higher minimum for stable population
  const int MAX_SPAWN_PER_FRAME = 12;  // Allow more monsters to spawn quickly

  int nearbyMonsterCount = 0;

  // Count monsters within nearby range
  for (int i = 0; i < monsterCount; i++)
  {
    if (!monsters[i].alive)
      continue;

    float dx = monsters[i].x - player.x;
    float dy = monsters[i].y - player.y;
    float distance = sqrt(dx * dx + dy * dy);

    if (distance >= MIN_NEARBY_DISTANCE && distance <= MAX_NEARBY_DISTANCE)
    {
      nearbyMonsterCount++;
    }
  }

  // Spawn additional monsters if we don't have enough nearby
  if (nearbyMonsterCount < MIN_NEARBY_MONSTERS && monsterCount < MAX_MONSTERS)
  {
    int monstersToSpawn = MIN_NEARBY_MONSTERS - nearbyMonsterCount;
    if (monstersToSpawn > MAX_SPAWN_PER_FRAME)
      monstersToSpawn = MAX_SPAWN_PER_FRAME; // Limit spawning per frame

    // Always spawn at least 2 monsters if we have room and count is low
    if (monstersToSpawn == 0 && nearbyMonsterCount < MIN_NEARBY_MONSTERS - 3)
      monstersToSpawn = 2;

    // Sometimes spawn in small clusters (15% chance for more natural distribution)
    int spawnInCluster = (rand() % 100) < 15;
    if (spawnInCluster && monstersToSpawn > 1)
    {
      monstersToSpawn = 2; // Limit cluster size to 2 for natural feel
    }

    for (int i = 0; i < monstersToSpawn; i++)
    {
      if (monsterCount >= MAX_MONSTERS)
        break;

      Character *monster = &monsters[monsterCount++];

      // More natural spawning: vary distance and sometimes cluster
      float spawnX, spawnY;

      if (spawnInCluster && i > 0)
      {
        // Spawn near the previous monster in the cluster with natural variation
        Character *lastMonster = &monsters[monsterCount - 2];
        float clusterAngle = ((rand() % 360) + (rand() % 120 - 60)) * PI / 180.0f; // ±60 degree variation
        float clusterDistance = 3.0f + (rand() % 5) + (rand() % 100) / 100.0f;     // 3-8 units with fractional variation
        spawnX = lastMonster->x + cos(clusterAngle) * clusterDistance;
        spawnY = lastMonster->y + sin(clusterAngle) * clusterDistance;
      }
      else
      {
        // Normal spawning around player - use floating point for natural distribution
        // Add some randomness to angle and distance for more organic spawning
        float angle = ((rand() % 360) + (rand() % 60 - 30)) * PI / 180.0f; // ±30 degree variation
        float spawnDistance = MIN_NEARBY_DISTANCE + (rand() % (MAX_NEARBY_DISTANCE - MIN_NEARBY_DISTANCE));
        // Add some distance variation (±20%)
        spawnDistance *= 0.8f + (rand() % 40) / 100.0f;
        spawnX = player.x + cos(angle) * spawnDistance;
        spawnY = player.y + sin(angle) * spawnDistance;
      }

      // No bounds checking - unlimited world!
      // Convert to integer coordinates
      monster->x = (int)round(spawnX);
      monster->y = (int)round(spawnY);

      // Check if this position is too close to existing monsters (avoid overcrowding)
      bool positionValid = true;
      for (int j = 0; j < monsterCount - 1; j++)
      {
        if (!monsters[j].alive)
          continue;
        float dx = monster->x - monsters[j].x;
        float dy = monster->y - monsters[j].y;
        float distToOther = sqrt(dx * dx + dy * dy);
        if (distToOther < 3.0f) // Minimum 3 units between monsters
        {
          positionValid = false;
          break;
        }
      }

      // If position is invalid, try a few more times
      if (!positionValid)
      {
        monsterCount--;              // Remove the invalid monster
        if (i < monstersToSpawn - 1) // If we have attempts left
        {
          i--; // Retry this spawn
          continue;
        }
      }

      // Scale monster strength based on player level
      int levelBonus = player.level - 1;
      monster->health = 20 + rand() % 30 + levelBonus * 10;
      monster->maxHealth = monster->health;
      monster->power = 3 + rand() % 5 + levelBonus * 2;
      monster->level = 1 + levelBonus / 2; // Monsters get stronger with player level

      monster->textureIndex = 1 + rand() % 5;
      monster->alive = 1;
      monster->speed = 1;
      monster->speedMultiplier = 1.0f;
      monster->damageMultiplier = 1.0f;
      monster->powerupTimer = 0;
      monster->movementCooldown = 0;
      monster->experience = 0;
      monster->experienceToNext = 0;
      monster->isInCombat = 0;
      monster->invulnerabilityTimer = 0; // Monsters don't need invulnerability
      strcpy(monster->name, "Monster");
    }
  }
}

void ensureNearbyPowerups()
{
  const int MIN_NEARBY_DISTANCE = 5;  // Minimum distance for "nearby"
  const int MAX_NEARBY_DISTANCE = 50; // Maximum distance for "nearby"
  const int MIN_NEARBY_POWERUPS = 3;  // Minimum powerups within nearby range

  int nearbyPowerupCount = 0;

  // Count powerups within nearby range
  for (int i = 0; i < powerupCount; i++)
  {
    int dx = powerups[i].x - player.x;
    int dy = powerups[i].y - player.y;
    float distance = sqrt(dx * dx + dy * dy);

    if (distance >= MIN_NEARBY_DISTANCE && distance <= MAX_NEARBY_DISTANCE)
    {
      nearbyPowerupCount++;
    }
  }

  // Spawn additional powerups if we don't have enough nearby
  if (nearbyPowerupCount < MIN_NEARBY_POWERUPS && powerupCount < MAX_POWERUPS)
  {
    int powerupsToSpawn = MIN_NEARBY_POWERUPS - nearbyPowerupCount;
    if (powerupsToSpawn > 2)
      powerupsToSpawn = 2; // Don't spawn too many at once

    for (int i = 0; i < powerupsToSpawn; i++)
    {
      if (powerupCount >= MAX_POWERUPS)
        break;

      Powerup *powerup = &powerups[powerupCount++];

      // Spawn in a random direction from player
      float angle = (rand() % 360) * PI / 180.0f;
      int distance = MIN_NEARBY_DISTANCE + rand() % (MAX_NEARBY_DISTANCE - MIN_NEARBY_DISTANCE);

      int attempts = 0;
      const int MAX_ATTEMPTS = 10; // Prevent infinite loops

      do
      {
        powerup->x = player.x + cos(angle) * distance;
        powerup->y = player.y + sin(angle) * distance;

        // No bounds checking - unlimited world!

        attempts++;
        if (attempts >= MAX_ATTEMPTS)
          break;

        // Make sure we don't spawn on player
      } while ((abs(powerup->x - player.x) < 2 && abs(powerup->y - player.y) < 2) && attempts < MAX_ATTEMPTS);

      powerup->type = rand() % 3; // 0=health, 1=speed, 2=damage
      powerup->active = 1;
    }
  }
}

void ensureNearbyLandmines()
{
  const int MIN_NEARBY_DISTANCE = 8;  // Minimum distance for "nearby"
  const int MAX_NEARBY_DISTANCE = 60; // Maximum distance for "nearby" (landmines can be further)
  const int MIN_NEARBY_LANDMINES = 2; // Minimum landmines within nearby range

  int nearbyLandmineCount = 0;

  // Count landmines within nearby range
  for (int i = 0; i < landmineCount; i++)
  {
    int dx = landmines[i].x - player.x;
    int dy = landmines[i].y - player.y;
    float distance = sqrt(dx * dx + dy * dy);

    if (distance >= MIN_NEARBY_DISTANCE && distance <= MAX_NEARBY_DISTANCE)
    {
      nearbyLandmineCount++;
    }
  }

  // Spawn additional landmines if we don't have enough nearby
  if (nearbyLandmineCount < MIN_NEARBY_LANDMINES && landmineCount < MAX_LANDMINES)
  {
    int landminesToSpawn = MIN_NEARBY_LANDMINES - nearbyLandmineCount;
    if (landminesToSpawn > 1)
      landminesToSpawn = 1; // Don't spawn too many at once

    for (int i = 0; i < landminesToSpawn; i++)
    {
      if (landmineCount >= MAX_LANDMINES)
        break;

      Landmine *landmine = &landmines[landmineCount++];

      // Spawn in a random direction from player
      float angle = (rand() % 360) * PI / 180.0f;
      int distance = MIN_NEARBY_DISTANCE + rand() % (MAX_NEARBY_DISTANCE - MIN_NEARBY_DISTANCE);

      int attempts = 0;
      const int MAX_ATTEMPTS = 10; // Prevent infinite loops

      do
      {
        landmine->x = player.x + cos(angle) * distance;
        landmine->y = player.y + sin(angle) * distance;

        // No bounds checking - unlimited world!

        attempts++;
        if (attempts >= MAX_ATTEMPTS)
          break;

        // Make sure we don't spawn on player
      } while ((abs(landmine->x - player.x) < 2 && abs(landmine->y - player.y) < 2) && attempts < MAX_ATTEMPTS);

      landmine->active = 1;
    }
  }
}

void updatePlayer()
{
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
  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
  {
    player.y -= player.speed * player.speedMultiplier;
    moved = 1;
  }
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
  {
    player.y += player.speed * player.speedMultiplier;
    moved = 1;
  }
  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
  {
    player.x -= player.speed * player.speedMultiplier;
    moved = 1;
  }
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
  {
    player.x += player.speed * player.speedMultiplier;
    moved = 1;
  }

  // If player moved, set cooldown (longer when in combat)
  if (moved)
  {
    player.movementCooldown = player.isInCombat ? 12 : 6; // 50% slower when fighting
    // Play movement sound
    // if (sounds[0].frameCount > 0) PlaySound(sounds[0]);
  }

  // No bounds checking - unlimited world exploration!

  // Update camera to follow player
  camera.target = (Vector2){player.x * CELL_SIZE, player.y * CELL_SIZE};
}

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

    // Random movement
    Direction dir = rand() % 4;
    int newX = monsters[i].x;
    int newY = monsters[i].y;

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

    // No bounds checking - unlimited world!
    monsters[i].x = newX;
    monsters[i].y = newY;
    // Set cooldown after moving (longer when in combat)
    monsters[i].movementCooldown = monsters[i].isInCombat ? 24 : 12; // 50% slower when fighting
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

void drawWorld()
{
  // Get camera bounds to determine what to draw
  int camLeft = (camera.target.x - WINDOW_SIZE / 2) / CELL_SIZE - 1;
  int camRight = (camera.target.x + WINDOW_SIZE / 2) / CELL_SIZE + 1;
  int camTop = (camera.target.y - WINDOW_SIZE / 2) / CELL_SIZE - 1;
  int camBottom = (camera.target.y + WINDOW_SIZE / 2) / CELL_SIZE + 1;

  // No bounds checking - unlimited world exploration!

  // Draw terrain (uncommented with faded colors for better contrast)
  for (int worldX = camLeft; worldX <= camRight; worldX++)
  {
    for (int worldY = camTop; worldY <= camBottom; worldY++)
    {
      // No bounds checking - unlimited world!
      WorldPosition pos = worldToChunk(worldX, worldY);
      int chunkIndex = getChunkIndex(pos.chunkX, pos.chunkY);

      if (chunkIndex != -1)
      {
        int terrainType = loadedChunks[chunkIndex].terrain[pos.localX][pos.localY];
        Color terrainColor;

        switch (terrainType)
        {
        case 0:
          terrainColor = (Color){80, 140, 80, 220}; // Grass - slightly brighter green
          break;
        case 1:
          terrainColor = (Color){140, 110, 80, 220}; // Mountain - slightly brighter brown
          break;
        case 2:
          terrainColor = (Color){50, 110, 50, 220}; // Tree - slightly brighter dark green
          break;
        case 3:
          terrainColor = (Color){80, 140, 200, 220}; // Lake - slightly brighter blue
          break;
        case 4:
          terrainColor = (Color){50, 80, 140, 220}; // Sea - slightly brighter dark blue
          break;
        default:
          terrainColor = (Color){80, 140, 80, 220}; // Default to brighter grass
          break;
        }

        DrawRectangle(worldX * CELL_SIZE, worldY * CELL_SIZE, CELL_SIZE, CELL_SIZE, terrainColor);
      }
      else
      {
        // Draw default brighter grass terrain for unloaded chunks to prevent flashing
        DrawRectangle(worldX * CELL_SIZE, worldY * CELL_SIZE, CELL_SIZE, CELL_SIZE, (Color){80, 140, 80, 220});
      }
    }
  }

  // Draw grid within camera bounds (after terrain so it appears on top)
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

  // Draw landmines (only those in loaded chunks)
  for (int i = 0; i < landmineCount; i++)
  {
    if (landmines[i].active)
    {
      DrawCircle(landmines[i].x * CELL_SIZE + CELL_SIZE / 2,
                 landmines[i].y * CELL_SIZE + CELL_SIZE / 2,
                 CELL_SIZE / 4, RED);
    }
  }

  // Draw powerups (only those in loaded chunks)
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

  // Draw monsters (only those in loaded chunks)
  for (int i = 0; i < monsterCount; i++)
  {
    if (monsters[i].alive)
    {
      // Check if monster is in camera view
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
    // Invulnerability visual effect (flashing)
    Color playerColor = WHITE;
    if (player.invulnerabilityTimer > 0)
    {
      // Flash every 10 frames
      if ((player.invulnerabilityTimer / 10) % 2 == 0)
      {
        playerColor = YELLOW; // Bright yellow when flashing
      }
      else
      {
        playerColor = Fade(WHITE, 0.3f); // Semi-transparent when not flashing
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
  int minimapSize = 120; // Smaller minimap
  int minimapX = WINDOW_SIZE - minimapSize - 10;
  int minimapY = WINDOW_SIZE - minimapSize - 10; // Bottom right corner
  int minimapScale = 3;                          // Show 3x3 world cells per minimap pixel (zoomed out more)

  // Draw minimap background
  DrawRectangle(minimapX, minimapY, minimapSize, minimapSize, (Color){0, 0, 0, 180});
  DrawRectangleLines(minimapX, minimapY, minimapSize, minimapSize, WHITE);

  // Calculate minimap bounds (centered on player)
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
      // No bounds checking - unlimited world!
      WorldPosition pos = worldToChunk(worldX, worldY);
      int chunkIndex = getChunkIndex(pos.chunkX, pos.chunkY);

      if (chunkIndex != -1)
      {
        int terrainType = loadedChunks[chunkIndex].terrain[pos.localX][pos.localY];
        Color terrainColor;

        switch (terrainType)
        {
        case 0:
          terrainColor = (Color){80, 140, 80, 255}; // Grass - brighter green
          break;
        case 1:
          terrainColor = (Color){140, 110, 80, 255}; // Mountain - brighter brown
          break;
        case 2:
          terrainColor = (Color){50, 110, 50, 255}; // Tree - brighter dark green
          break;
        case 3:
          terrainColor = (Color){80, 140, 200, 255}; // Lake - brighter blue
          break;
        case 4:
          terrainColor = (Color){50, 80, 140, 255}; // Sea - brighter dark blue
          break;
        default:
          terrainColor = (Color){80, 140, 80, 255}; // Default to brighter grass
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

void drawUI()
{
  // Draw UI background (reduced height)
  DrawRectangle(0, 0, WINDOW_SIZE, 60, Fade(BLACK, 0.7f));

  // Left side - Player stats
  DrawText(TextFormat("HP: %d/%d", player.health, player.maxHealth), 10, 8, 18, WHITE);
  DrawText(TextFormat("PWR: %d", (int)(player.power * player.damageMultiplier)), 10, 28, 18, WHITE);
  DrawText(TextFormat("LVL: %d", player.level), 10, 48, 18, WHITE);

  // Middle left - XP and invulnerability
  DrawText(TextFormat("XP: %d/%d", player.experience, player.experienceToNext), 120, 8, 18, WHITE);
  if (player.invulnerabilityTimer > 0)
  {
    int secondsLeft = player.invulnerabilityTimer / 60;
    DrawText(TextFormat("INVUL (%ds)", secondsLeft + 1), 120, 28, 16, YELLOW);
  }

  // Middle right - Debug info
  DrawText(TextFormat("Monsters: %d", monsterCount), 280, 8, 16, YELLOW);
  DrawText(TextFormat("Powerups: %d", powerupCount), 280, 28, 16, GREEN);
  DrawText(TextFormat("Landmines: %d", landmineCount), 280, 48, 16, ORANGE);

  // Right side - Position and restart
  DrawText(TextFormat("POS: (%d,%d)", (int)player.x, (int)player.y), 450, 8, 16, WHITE);
  DrawText(player.alive ? "ALIVE" : "DEAD", 450, 28, 16, player.alive ? GREEN : RED);

  // Restart button (smaller)
  DrawRectangle(WINDOW_SIZE - 100, 8, 90, 25, Fade(BLUE, 0.8f));
  DrawRectangleLines(WINDOW_SIZE - 100, 8, 90, 25, WHITE);
  DrawText("RESTART (R)", WINDOW_SIZE - 95, 12, 14, WHITE);

  // Powerup status (if active)
  if (player.powerupTimer > 0)
  {
    const char *powerupName;
    switch (player.activePowerup)
    {
    case POWERUP_DOUBLE_DAMAGE:
      powerupName = "2x DAMAGE";
      break;
    case POWERUP_DOUBLE_HEALTH:
      powerupName = "FULL HP";
      break;
    case POWERUP_DOUBLE_SPEED:
      powerupName = "2x SPEED";
      break;
    default:
      powerupName = "POWERUP";
      break;
    }
    DrawText(TextFormat("%s (%d)", powerupName, player.powerupTimer / 60), 120, 48, 14, YELLOW);
  }

  // Draw minimap (moved to bottom right)
  drawMinimap();

  // Game over
  if (!player.alive)
  {
    DrawText("GAME OVER", WINDOW_SIZE / 2 - 100, WINDOW_SIZE / 2, 40, RED);
    DrawText("Press R to restart", WINDOW_SIZE / 2 - 100, WINDOW_SIZE / 2 + 50, 20, WHITE);
  }
}

int main()
{
  SetConfigFlags(FLAG_WINDOW_HIGHDPI);
  InitWindow(WINDOW_SIZE, WINDOW_SIZE, "Gridlock Arena - Player Control");
  InitAudioDevice(); // Initialize audio device
  SetTargetFPS(60);

  // Initialize random seed
  srand(time(NULL));

  // Load textures
  textures[0] = LoadTexture("emojis/2694.png");  // Knight (player)
  textures[1] = LoadTexture("emojis/1f409.png"); // Dragon
  textures[2] = LoadTexture("emojis/1f47a.png"); // Goblin
  textures[3] = LoadTexture("emojis/1f479.png"); // Ogre
  textures[4] = LoadTexture("emojis/1f47f.png"); // Troll
  textures[5] = LoadTexture("emojis/1f9d9.png"); // Wizard

  // Load sounds
  sounds[0] = LoadSound("sounds/fight.wav");    // Battle sound
  sounds[1] = LoadSound("sounds/powerup.wav");  // Powerup pickup
  sounds[2] = LoadSound("sounds/landmine.wav"); // Landmine explosion
  sounds[3] = LoadSound("sounds/death.wav");    // Death sound
  sounds[4] = LoadSound("sounds/victory.wav");  // Victory sound

  initGame();

  // No need for initial spawning - chunks will generate content

  int spawnTimer = 0;

  while (!WindowShouldClose())
  {
    // Update
    if (player.alive)
    {
      updatePlayer();
      updateMonsters();
      checkCollisions();
      updatePowerups();
      updateLandmines();
      updateChunks(); // Update chunk loading/unloading

      // Ensure monsters are nearby
      ensureNearbyMonsters();

      // Ensure powerups are nearby
      ensureNearbyPowerups();

      // Ensure landmines are nearby
      ensureNearbyLandmines();

      // Restart on R key (works anytime)
      if (IsKeyPressed(KEY_R))
      {
        initGame();
        spawnTimer = 0;
      }

      // Draw
      BeginDrawing();
      ClearBackground(WHITE);

      BeginMode2D(camera);
      drawWorld();
      EndMode2D();

      drawUI();

      EndDrawing();
    }
  }

  // Cleanup
  for (int i = 0; i < 6; i++)
  {
    UnloadTexture(textures[i]);
  }
  for (int i = 0; i < 5; i++)
  {
    UnloadSound(sounds[i]);
  }
  CloseAudioDevice();
  CloseWindow();
  return 0;
}
