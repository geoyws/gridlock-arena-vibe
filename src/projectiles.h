#ifndef PROJECTILES_H
#define PROJECTILES_H

#include "types.h"

// Function declarations for projectile management
void spawnProjectile(int x, int y, float dx, float dy, int type, int damage);
void updateProjectiles();
void drawProjectiles();

#endif
