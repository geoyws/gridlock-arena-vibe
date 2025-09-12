#ifndef WORLD_H
#define WORLD_H

#include "types.h"

// Function declarations for world management
void updateChunks();
void ensureNearbyMonsters();
void ensureNearbyPowerups();
void ensureNearbyLandmines();
void drawWorld();
void drawMinimap();
WorldPosition worldToChunk(int worldX, int worldY);
int getChunkIndex(int chunkX, int chunkY);
int loadChunk(int chunkX, int chunkY);
void unloadChunkEntities(int chunkIndex);
void generateChunkContent(int chunkX, int chunkY);
void generateChunkTerrain(int chunkX, int chunkY);

#endif
