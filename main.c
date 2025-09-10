#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define GRID_SIZE 5
#define CELL_SIZE 150
#define WINDOW_SIZE (GRID_SIZE * CELL_SIZE)

typedef enum
{
  UP,
  DOWN,
  LEFT,
  RIGHT
} Direction;

typedef struct
{
  char name[20];
  int x, y;
  Direction moves[10];
  int moveCount;
  int power;
  int textureIndex;
  int alive;
  int score;
} Creature;

Creature creatures[] = {
    {"Dragon", 0, 0, {0}, 0, 7, 0, 1, 0},
    {"Goblin", 0, 2, {0}, 0, 3, 1, 1, 0},
    {"Ogre", 2, 0, {0}, 0, 5, 2, 1, 0},
    {"Troll", 2, 2, {0}, 0, 4, 3, 1, 0},
    {"Wizard", 4, 1, {0}, 0, 6, 4, 1, 0}};

int creatureCount = sizeof(creatures) / sizeof(Creature);

Texture2D textures[6];                           // 5 creatures + 1 fencing
int battleLocations[GRID_SIZE][GRID_SIZE] = {0}; // Track where battles occur

void resetCreatures()
{
  // Reset all creatures to initial state
  creatures[0] = (Creature){"Dragon", 0, 0, {0}, 0, 7, 0, 1, 0};
  creatures[1] = (Creature){"Goblin", 0, 2, {0}, 0, 3, 1, 1, 0};
  creatures[2] = (Creature){"Ogre", 2, 0, {0}, 0, 5, 2, 1, 0};
  creatures[3] = (Creature){"Troll", 2, 2, {0}, 0, 4, 3, 1, 0};
  creatures[4] = (Creature){"Wizard", 4, 1, {0}, 0, 6, 4, 1, 0};

  // Clear battle locations
  memset(battleLocations, 0, sizeof(battleLocations));
}
void drawGrid()
{
  for (int i = 0; i <= GRID_SIZE; i++)
  {
    DrawLine(i * CELL_SIZE, 0, i * CELL_SIZE, WINDOW_SIZE, BLACK);
    DrawLine(0, i * CELL_SIZE, WINDOW_SIZE, i * CELL_SIZE, BLACK);
  }
}

void drawCreatures()
{
  // Draw battle locations first (fencing emoji)
  for (int y = 0; y < GRID_SIZE; y++)
  {
    for (int x = 0; x < GRID_SIZE; x++)
    {
      if (battleLocations[y][x])
      {
        int drawX = x * CELL_SIZE + CELL_SIZE / 2 - 75;
        int drawY = y * CELL_SIZE + CELL_SIZE / 2 - 75;
        DrawTextureEx(textures[5], (Vector2){drawX, drawY}, 0, 150.0 / 72.0, WHITE);
      }
    }
  }

  // Draw creatures
  for (int i = 0; i < creatureCount; i++)
  {
    if (creatures[i].alive)
    {
      int x = creatures[i].x * CELL_SIZE + CELL_SIZE / 2 - 75;
      int y = creatures[i].y * CELL_SIZE + CELL_SIZE / 2 - 75;
      DrawTextureEx(textures[creatures[i].textureIndex], (Vector2){x, y}, 0, 150.0 / 72.0, WHITE);
    }
  }
}

void moveCreature(Creature *c, int moveIndex)
{
  // Choose a random direction instead of following sequence
  Direction dir = (Direction)(rand() % 4); // 0=UP, 1=DOWN, 2=LEFT, 3=RIGHT

  switch (dir)
  {
  case UP:
    if (c->y > 0)
      c->y--;
    break;
  case DOWN:
    if (c->y < GRID_SIZE - 1)
      c->y++;
    break;
  case LEFT:
    if (c->x > 0)
      c->x--;
    break;
  case RIGHT:
    if (c->x < GRID_SIZE - 1)
      c->x++;
    break;
  }
}

void resolveBattles()
{
  int positions[GRID_SIZE][GRID_SIZE] = {0};
  int power[GRID_SIZE][GRID_SIZE] = {0};
  int count[GRID_SIZE][GRID_SIZE] = {0};

  // Clear previous battle locations
  memset(battleLocations, 0, sizeof(battleLocations));

  // Count creatures per position
  for (int i = 0; i < creatureCount; i++)
  {
    if (creatures[i].alive)
    {
      int x = creatures[i].x;
      int y = creatures[i].y;
      positions[y][x]++;
      power[y][x] += creatures[i].power;
      count[y][x]++;
    }
  }

  // Resolve battles
  for (int y = 0; y < GRID_SIZE; y++)
  {
    for (int x = 0; x < GRID_SIZE; x++)
    {
      if (count[y][x] > 1)
      {
        // Mark battle location
        battleLocations[y][x] = 1;

        // Battle
        int maxPower = 0;
        int winnerIndex = -1;
        for (int i = 0; i < creatureCount; i++)
        {
          if (creatures[i].alive && creatures[i].x == x && creatures[i].y == y)
          {
            if (creatures[i].power > maxPower)
            {
              maxPower = creatures[i].power;
              winnerIndex = i;
            }
          }
        }
        // Kill all except winner
        for (int i = 0; i < creatureCount; i++)
        {
          if (creatures[i].alive && creatures[i].x == x && creatures[i].y == y && i != winnerIndex)
          {
            creatures[i].alive = 0;
            if (winnerIndex != -1)
            {
              creatures[winnerIndex].score += creatures[i].power;
            }
          }
        }
      }
    }
  }
}

int main()
{
  SetConfigFlags(FLAG_WINDOW_HIGHDPI);
  InitWindow(WINDOW_SIZE, WINDOW_SIZE, "Gridlock Arena");
  SetTargetFPS(1); // Slow for simulation

  // Initialize random seed for randomized movement
  srand(time(NULL));

  // Load textures for emojis
  textures[0] = LoadTexture("emojis/1f409.png"); // Dragon
  textures[1] = LoadTexture("emojis/1f47a.png"); // Goblin
  textures[2] = LoadTexture("emojis/1f479.png"); // Ogre
  textures[3] = LoadTexture("emojis/1f47f.png"); // Troll
  textures[4] = LoadTexture("emojis/1f9d9.png"); // Wizard
  textures[5] = LoadTexture("emojis/1f93a.png"); // Fencing

  int turn = 0;
  int moveIndex = 0;

  while (!WindowShouldClose())
  {
    // Check for restart button click
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
      Vector2 mousePos = GetMousePosition();
      Rectangle restartButton = {WINDOW_SIZE - 120, 10, 100, 40};
      if (CheckCollisionPointRec(mousePos, restartButton))
      {
        resetCreatures();
        turn = 0;
        moveIndex = 0;
      }
    }

    // Move all creatures
    for (int i = 0; i < creatureCount; i++)
    {
      if (creatures[i].alive)
      {
        moveCreature(&creatures[i], moveIndex);
      }
    }

    // Resolve battles
    resolveBattles();

    // Draw
    BeginDrawing();
    ClearBackground(RAYWHITE);
    drawGrid();
    drawCreatures();

    // Draw restart button
    DrawRectangle(WINDOW_SIZE - 120, 10, 100, 40, LIGHTGRAY);
    DrawRectangleLines(WINDOW_SIZE - 120, 10, 100, 40, BLACK);
    DrawText("RESTART", WINDOW_SIZE - 110, 20, 20, BLACK);

    // Draw scores
    char scoreText[100];
    sprintf(scoreText, "Turn: %d", turn);
    DrawText(scoreText, 10, 10, 30, BLACK);

    for (int i = 0; i < creatureCount; i++)
    {
      sprintf(scoreText, "%s: %d", creatures[i].name, creatures[i].score);
      DrawText(scoreText, 10, 40 + i * 30, 30, creatures[i].alive ? BLACK : GRAY);
    }

    EndDrawing();

    turn++;
    moveIndex++;
  }

  // Unload textures
  for (int i = 0; i < 6; i++)
  {
    UnloadTexture(textures[i]);
  }
  CloseWindow();
  return 0;
}
