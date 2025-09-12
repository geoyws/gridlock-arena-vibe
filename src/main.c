#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Include our modular headers
#include "types.h"
#include "globals.h"
#include "world.h"
#include "projectiles.h"
#include "player.h"
#include "monsters.h"
#include "ui.h"
#include "game.h"

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
  textures[6] = LoadTexture("emojis/26a1.png");  // Lightning bolt
  textures[7] = LoadTexture("emojis/1f525.png"); // Fireball
  textures[8] = LoadTexture("emojis/27a1.png");  // Arrow (right arrow)

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
      updateProjectiles();
      updateChunks(); // Update chunk loading/unloading

      // Ensure monsters are nearby
      ensureNearbyMonsters();

      // Ensure powerups are nearby
      ensureNearbyPowerups();

      // Ensure landmines are nearby
      ensureNearbyLandmines();
    }

    // Auto-restart after death (after a short delay)
    if (!player.alive && player.deathTimer <= 0)
    {
      player.deathTimer = 180; // 3 second delay before auto-restart
    }
    if (!player.alive && player.deathTimer > 0)
    {
      player.deathTimer--;
      if (player.deathTimer <= 0)
      {
        restartGame();
        spawnTimer = 0;
      }
    }

    // Manual restart on R key (works anytime)
    if (IsKeyPressed(KEY_R))
    {
      restartGame();
      spawnTimer = 0;
    }

    // Draw
    BeginDrawing();
    ClearBackground(WHITE);

    BeginMode2D(camera);
    drawWorld();
    drawProjectiles();
    EndMode2D();

    drawUI();

    EndDrawing();
  }

  // Cleanup
  for (int i = 0; i < 9; i++)
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
