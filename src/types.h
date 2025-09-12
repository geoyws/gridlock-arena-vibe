#ifndef TYPES_H
#define TYPES_H

#include "raylib.h"

// Game constants
#define WORLD_SIZE 100
#define CELL_SIZE 20
#define WINDOW_SIZE 800
#define MAX_MONSTERS 2000
#define MAX_POWERUPS 50
#define MAX_LANDMINES 100
#define MAX_PROJECTILES 100

// Chunk system constants
#define CHUNK_SIZE 32
#define CHUNK_CELL_SIZE (CHUNK_SIZE * CELL_SIZE)
#define MAX_LOADED_CHUNKS 625  // 25x25 grid of chunks around player
#define CHUNK_LOAD_DISTANCE 12 // Much larger loading distance to prevent chunk unloading

// Enums
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

// Data structures
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

  // Player special abilities
  int jumpSmashCooldown;
  int rushCooldown;
  int healCooldown;
  int arrowCooldown;

  // Last movement direction for abilities
  int lastDirX;
  int lastDirY;

  // Intended movement direction (what keys are being pressed)
  int intendedDirX;
  int intendedDirY;

  // Status effects
  int stunTimer;       // Lightning stun
  int dotTimer;        // Fire damage over time
  int dotDamage;       // Damage per tick
  int speedBoostTimer; // Rush speed boost duration
  int deathTimer;      // Auto-restart timer after death
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

typedef struct
{
  int x, y;
  float dx, dy; // Changed to float for precise direction tracking
  int type;     // 0=lightning, 1=fireball, 2=arrow
  int alive;
  int damage;
  int speed;
  int effect;   // 0=none, 1=stun, 2=dot
  int range;    // Distance traveled
  int maxRange; // Maximum range before disappearing
} Projectile;

#endif // TYPES_H
