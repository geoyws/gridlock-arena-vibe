#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WORLD_SIZE 100
#define CELL_SIZE 20
#define WINDOW_SIZE 800
#define MAX_MONSTERS 50
#define MAX_POWERUPS 20
#define MAX_LANDMINES 30

// Chunk system constants
#define CHUNK_SIZE 32
#define CHUNK_CELL_SIZE (CHUNK_SIZE * CELL_SIZE)
#define MAX_LOADED_CHUNKS 25  // 5x5 grid of chunks around player
#define CHUNK_LOAD_DISTANCE 2 // Load chunks within 2 chunks of player

// Chunk system structures
typedef struct
{
  int chunkX, chunkY; // Chunk coordinates
  int loaded;         // Whether this chunk is currently loaded
  int lastAccess;     // Frame counter for LRU cache
  // Terrain data could be added here (biomes, elevation, etc.)
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
void drawWorld();
void drawUI();
WorldPosition worldToChunk(int worldX, int worldY);
int getChunkIndex(int chunkX, int chunkY);
int loadChunk(int chunkX, int chunkY);
void unloadChunkEntities(int chunkIndex);
void generateChunkContent(int chunkX, int chunkY);
void updateChunks();

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
  player.x = 0; // Start at world origin
  player.y = 0;
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

  // Load initial chunks around player
  updateChunks();
}

void spawnMonster()
{
  if (monsterCount >= MAX_MONSTERS)
    return;

  Character *monster = &monsters[monsterCount++];
  strcpy(monster->name, "Monster");
  monster->x = rand() % WORLD_SIZE;
  monster->y = rand() % WORLD_SIZE;
  monster->health = 20 + rand() % 30; // 20-50 health
  monster->maxHealth = monster->health;
  monster->power = 3 + rand() % 5;          // 3-7 power
  monster->textureIndex = 1 + (rand() % 4); // Random monster texture
  monster->alive = 1;
  monster->speed = 1;
  monster->speedMultiplier = 1.0f;
  monster->damageMultiplier = 1.0f;
  monster->powerupTimer = 0;
}

void spawnPowerup()
{
  if (powerupCount >= MAX_POWERUPS)
    return;

  Powerup *powerup = &powerups[powerupCount++];
  powerup->x = rand() % WORLD_SIZE;
  powerup->y = rand() % WORLD_SIZE;
  powerup->type = rand() % POWERUP_COUNT;
  powerup->active = 1;
}

void spawnLandmine()
{
  if (landmineCount >= MAX_LANDMINES)
    return;

  Landmine *landmine = &landmines[landmineCount++];
  landmine->x = rand() % WORLD_SIZE;
  landmine->y = rand() % WORLD_SIZE;
  landmine->damage = 10 + rand() % 20; // 10-30 damage
  landmine->active = 1;
}

// Chunk system utility functions
WorldPosition worldToChunk(int worldX, int worldY)
{
  WorldPosition pos;
  pos.x = worldX;
  pos.y = worldY;
  pos.chunkX = worldX / CHUNK_SIZE;
  pos.chunkY = worldY / CHUNK_SIZE;
  pos.localX = worldX % CHUNK_SIZE;
  pos.localY = worldY % CHUNK_SIZE;
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
    loadedChunks[oldestIndex].lastAccess = GetFrameTime() * 1000; // Current time in ms
    return 1;
  }

  // Load new chunk
  loadedChunks[loadedChunkCount].chunkX = chunkX;
  loadedChunks[loadedChunkCount].chunkY = chunkY;
  loadedChunks[loadedChunkCount].loaded = 1;
  loadedChunks[loadedChunkCount].lastAccess = GetFrameTime() * 1000;
  loadedChunkCount++;

  // Generate content for this chunk
  generateChunkContent(chunkX, chunkY);

  return 1;
}

void unloadChunkEntities(int chunkIndex)
{
  int chunkX = loadedChunks[chunkIndex].chunkX;
  int chunkY = loadedChunks[chunkIndex].chunkY;

  // Remove monsters in this chunk
  for (int i = 0; i < monsterCount; i++)
  {
    WorldPosition pos = worldToChunk(monsters[i].x, monsters[i].y);
    if (pos.chunkX == chunkX && pos.chunkY == chunkY)
    {
      // Remove this monster by swapping with the last one
      monsters[i] = monsters[--monsterCount];
      i--; // Recheck this index
    }
  }

  // Remove powerups in this chunk
  for (int i = 0; i < powerupCount; i++)
  {
    WorldPosition pos = worldToChunk(powerups[i].x, powerups[i].y);
    if (pos.chunkX == chunkX && pos.chunkY == chunkY)
    {
      powerups[i] = powerups[--powerupCount];
      i--;
    }
  }

  // Remove landmines in this chunk
  for (int i = 0; i < landmineCount; i++)
  {
    WorldPosition pos = worldToChunk(landmines[i].x, landmines[i].y);
    if (pos.chunkX == chunkX && pos.chunkY == chunkY)
    {
      landmines[i] = landmines[--landmineCount];
      i--;
    }
  }
}

void generateChunkContent(int chunkX, int chunkY)
{
  // Generate monsters for this chunk
  int monstersInChunk = 3 + rand() % 4; // 3-6 monsters per chunk
  for (int i = 0; i < monstersInChunk; i++)
  {
    if (monsterCount >= MAX_MONSTERS)
      break;

    Character *monster = &monsters[monsterCount++];
    monster->x = chunkX * CHUNK_SIZE + rand() % CHUNK_SIZE;
    monster->y = chunkY * CHUNK_SIZE + rand() % CHUNK_SIZE;
    monster->health = 20 + rand() % 30; // 20-50 health
    monster->maxHealth = monster->health;
    monster->power = 3 + rand() % 5;        // 3-8 power
    monster->textureIndex = 1 + rand() % 5; // Random monster texture
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

  // Generate powerups for this chunk
  int powerupsInChunk = 1 + rand() % 2; // 1-2 powerups per chunk
  for (int i = 0; i < powerupsInChunk; i++)
  {
    if (powerupCount >= MAX_POWERUPS)
      break;

    Powerup *powerup = &powerups[powerupCount++];
    powerup->x = chunkX * CHUNK_SIZE + rand() % CHUNK_SIZE;
    powerup->y = chunkY * CHUNK_SIZE + rand() % CHUNK_SIZE;
    powerup->type = rand() % POWERUP_COUNT;
    powerup->active = 1;
  }

  // Generate landmines for this chunk
  int landminesInChunk = 1 + rand() % 3; // 1-3 landmines per chunk
  for (int i = 0; i < landminesInChunk; i++)
  {
    if (landmineCount >= MAX_LANDMINES)
      break;

    Landmine *landmine = &landmines[landmineCount++];
    landmine->x = chunkX * CHUNK_SIZE + rand() % CHUNK_SIZE;
    landmine->y = chunkY * CHUNK_SIZE + rand() % CHUNK_SIZE;
    landmine->damage = 10 + rand() % 20;
    landmine->active = 1;
  }
}

void updateChunks()
{
  // Get player's chunk position
  WorldPosition playerPos = worldToChunk(player.x, player.y);

  // Load chunks around player
  for (int dx = -CHUNK_LOAD_DISTANCE; dx <= CHUNK_LOAD_DISTANCE; dx++)
  {
    for (int dy = -CHUNK_LOAD_DISTANCE; dy <= CHUNK_LOAD_DISTANCE; dy++)
    {
      loadChunk(playerPos.chunkX + dx, playerPos.chunkY + dy);
    }
  }

  // Update access times for loaded chunks
  for (int i = 0; i < loadedChunkCount; i++)
  {
    loadedChunks[i].lastAccess = GetFrameTime() * 1000;
  }
}

void updatePlayer()
{
  // Update movement cooldown
  if (player.movementCooldown > 0)
  {
    player.movementCooldown--;
    return; // Don't process movement input while on cooldown
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

  // No boundary checks for unlimited world

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

    // Keep in bounds
    if (newX >= 0 && newX < WORLD_SIZE && newY >= 0 && newY < WORLD_SIZE)
    {
      monsters[i].x = newX;
      monsters[i].y = newY;
      // Set cooldown after moving (longer when in combat)
      monsters[i].movementCooldown = monsters[i].isInCombat ? 24 : 12; // 50% slower when fighting
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

      // Per-tick damage
      int playerDamage = (int)(player.power * player.damageMultiplier * 0.5f); // Reduced damage per tick
      int monsterDamage = (int)(monsters[i].power * monsters[i].damageMultiplier * 0.5f);

      monsters[i].health -= playerDamage;
      player.health -= monsterDamage;

      // Play battle sound (only once per combat tick)
      // if (sounds[1].frameCount > 0 && !player.isInCombat) PlaySound(sounds[1]);

      if (monsters[i].health <= 0)
      {
        monsters[i].alive = 0;
        // Award experience to player
        player.experience += monsters[i].power * 10;
        // Play victory sound
        // if (sounds[5].frameCount > 0) PlaySound(sounds[5]);

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
        // if (sounds[4].frameCount > 0) PlaySound(sounds[4]);
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
      }

      powerups[i].active = 0;
      // Play powerup sound
      // if (sounds[2].frameCount > 0) PlaySound(sounds[2]);
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
      // if (sounds[3].frameCount > 0) PlaySound(sounds[3]);
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
}

void drawWorld()
{
  // Get camera bounds to determine what to draw
  int camLeft = (camera.target.x - WINDOW_SIZE / 2) / CELL_SIZE - 1;
  int camRight = (camera.target.x + WINDOW_SIZE / 2) / CELL_SIZE + 1;
  int camTop = (camera.target.y - WINDOW_SIZE / 2) / CELL_SIZE - 1;
  int camBottom = (camera.target.y + WINDOW_SIZE / 2) / CELL_SIZE + 1;

  // Draw grid within camera bounds
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
    if (landmines[i].active && landmines[i].x >= 0 && landmines[i].y >= 0)
    {
      DrawCircle(landmines[i].x * CELL_SIZE + CELL_SIZE / 2,
                 landmines[i].y * CELL_SIZE + CELL_SIZE / 2,
                 CELL_SIZE / 4, RED);
    }
  }

  // Draw powerups (only those in loaded chunks)
  for (int i = 0; i < powerupCount; i++)
  {
    if (powerups[i].active && powerups[i].x >= 0 && powerups[i].y >= 0)
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
      }
      DrawCircle(powerups[i].x * CELL_SIZE + CELL_SIZE / 2,
                 powerups[i].y * CELL_SIZE + CELL_SIZE / 2,
                 CELL_SIZE / 3, color);
    }
  }

  // Draw monsters (only those in loaded chunks)
  for (int i = 0; i < monsterCount; i++)
  {
    if (monsters[i].alive && monsters[i].x >= 0 && monsters[i].y >= 0)
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

  // Draw player
  if (player.alive)
  {
    DrawTextureEx(textures[player.textureIndex],
                  (Vector2){player.x * CELL_SIZE, player.y * CELL_SIZE},
                  0, CELL_SIZE / 72.0f, WHITE);

    // Health bar
    DrawRectangle(player.x * CELL_SIZE, player.y * CELL_SIZE - 5,
                  CELL_SIZE, 3, RED);
    DrawRectangle(player.x * CELL_SIZE, player.y * CELL_SIZE - 5,
                  (float)player.health / player.maxHealth * CELL_SIZE, 3, GREEN);
  }
}

void drawUI()
{
  // Draw UI background
  DrawRectangle(0, 0, WINDOW_SIZE, 100, Fade(BLACK, 0.7f));

  // Player stats
  DrawText(TextFormat("Health: %d/%d", player.health, player.maxHealth), 10, 10, 20, WHITE);
  DrawText(TextFormat("Power: %d", (int)(player.power * player.damageMultiplier)), 10, 30, 20, WHITE);
  DrawText(TextFormat("Level: %d", player.level), 10, 50, 20, WHITE);
  DrawText(TextFormat("XP: %d/%d", player.experience, player.experienceToNext), 10, 70, 20, WHITE);

  // Powerup status
  if (player.powerupTimer > 0)
  {
    const char *powerupName;
    switch (player.activePowerup)
    {
    case POWERUP_DOUBLE_DAMAGE:
      powerupName = "DOUBLE DAMAGE";
      break;
    case POWERUP_DOUBLE_HEALTH:
      powerupName = "FULL HEALTH";
      break;
    case POWERUP_DOUBLE_SPEED:
      powerupName = "DOUBLE SPEED";
      break;
    }
    DrawText(TextFormat("%s (%d)", powerupName, player.powerupTimer / 60), 200, 10, 20, YELLOW);
  }

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

  // Load sounds (you'll need to add .wav files to the project)
  // sounds[0] = LoadSound("sounds/move.wav");        // Player movement
  // sounds[1] = LoadSound("sounds/battle.wav");      // Battle sound
  // sounds[2] = LoadSound("sounds/powerup.wav");     // Powerup pickup
  // sounds[3] = LoadSound("sounds/damage.wav");      // Taking damage
  // sounds[4] = LoadSound("sounds/death.wav");       // Death sound
  // sounds[5] = LoadSound("sounds/victory.wav");     // Victory sound

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
      updateChunks(); // Update chunk loading/unloading

      // No periodic spawning - chunks handle content generation
    }
    else
    {
      // Restart on R key
      if (IsKeyPressed(KEY_R))
      {
        initGame();
        spawnTimer = 0;
      }
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

  // Cleanup
  for (int i = 0; i < 6; i++)
  {
    UnloadTexture(textures[i]);
  }
  // for (int i = 0; i < 6; i++)
  // {
  //   UnloadSound(sounds[i]);
  // }
  CloseAudioDevice();
  CloseWindow();
  return 0;
}
