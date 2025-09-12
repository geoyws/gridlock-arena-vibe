#include "types.h"
#include "globals.h"
#include <stdlib.h>
#include <math.h>

// Function prototype for drawMinimap (defined in world.c)
void drawMinimap();

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

  // Controls display
  DrawText("WASD: Move | SPACE: Arrow | SHIFT: Rush | H: Heal | M: Minimap", 10, WINDOW_SIZE - 25, 14, WHITE);

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

  // Ability cooldowns
  if (player.jumpSmashCooldown > 0)
    DrawText(TextFormat("1: JUMP (%d)", player.jumpSmashCooldown / 60), 550, 8, 14, RED);
  else
    DrawText("1: JUMP", 550, 8, 14, GREEN);

  if (player.rushCooldown > 0)
    DrawText(TextFormat("2: RUSH (%d)", player.rushCooldown / 60), 550, 28, 14, RED);
  else
    DrawText("2: RUSH", 550, 28, 14, GREEN);

  if (player.healCooldown > 0)
    DrawText(TextFormat("3: HEAL (%d)", player.healCooldown / 60), 550, 48, 14, RED);
  else
    DrawText("3: HEAL", 550, 48, 14, GREEN);

  // Status effects
  if (player.stunTimer > 0)
    DrawText("STUNNED", 350, 8, 16, YELLOW);
  if (player.dotTimer > 0)
    DrawText("BURNING", 350, 28, 16, RED);
  if (player.speedBoostTimer > 0)
    DrawText("RUSHING!", 350, 48, 16, SKYBLUE);

  // Draw minimap (moved to bottom right)
  drawMinimap();

  // Game over
  if (!player.alive)
  {
    DrawText("GAME OVER", WINDOW_SIZE / 2 - 100, WINDOW_SIZE / 2, 40, RED);
    DrawText("Press R to restart", WINDOW_SIZE / 2 - 100, WINDOW_SIZE / 2 + 50, 20, WHITE);
  }
}
