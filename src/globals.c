#include "globals.h"

// Global game state
Character player;
Character monsters[MAX_MONSTERS];
Powerup powerups[MAX_POWERUPS];
Landmine landmines[MAX_LANDMINES];
Projectile projectiles[MAX_PROJECTILES];
Chunk loadedChunks[MAX_LOADED_CHUNKS];
int loadedChunkCount = 0;
int monsterCount = 0;
int powerupCount = 0;
int landmineCount = 0;
int projectileCount = 0;

// Assets
Texture2D textures[10]; // Player + monsters + powerups + landmines
Sound sounds[10];       // Various sound effects
Camera2D camera = {0};
