#ifndef GLOBALS_H
#define GLOBALS_H

#include "types.h"

// Global game state
extern Character player;
extern Character monsters[MAX_MONSTERS];
extern Powerup powerups[MAX_POWERUPS];
extern Landmine landmines[MAX_LANDMINES];
extern Projectile projectiles[MAX_PROJECTILES];
extern Chunk loadedChunks[MAX_LOADED_CHUNKS];
extern int loadedChunkCount;
extern int monsterCount;
extern int powerupCount;
extern int landmineCount;
extern int projectileCount;

// Assets
extern Texture2D textures[10]; // Player + monsters + powerups + landmines
extern Sound sounds[10];       // Various sound effects
extern Camera2D camera;

#endif // GLOBALS_H
