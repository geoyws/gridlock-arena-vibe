# Gridlock Arena - Unlimited RPG Adventure

## 🎮 Game Features

- **Unlimited World Generation**: Procedurally generated chunks with infinite exploration
- **Player Controls**: WASD movement with optimized speed (10 moves/second)
- **Monster AI**: Slower movement (5 moves/second) for better gameplay balance
- **Dynamic Spawning**: Monsters, powerups, and landmines spawn automatically in chunks
- **Real-time Combat**: Battle system with health/power mechanics
- **Power-ups**: Damage boost, health restoration, and speed multipliers
- **Audio Support**: Sound effects for all game actions

## 🎵 Adding Sound Effects

The game supports audio! To add sound effects:

1. **Create sound files** in WAV format (recommended for raylib)
2. **Place them in the `sounds/` directory**
3. **Uncomment the sound loading lines** in `main.c`

### Required Sound Files:
```
sounds/
├── move.wav        # Player movement sound
├── battle.wav      # Battle/combat sound
├── powerup.wav     # Powerup pickup sound
├── damage.wav      # Taking damage sound
├── death.wav       # Player death sound
└── victory.wav     # Monster defeat sound
```

### Sound Setup:
1. Find or create short WAV sound effects
2. Place them in the `sounds/` folder
3. In `main.c`, uncomment these lines in the main function:

```c
// Load sounds (you'll need to add .wav files to the project)
sounds[0] = LoadSound("sounds/move.wav");        // Player movement
sounds[1] = LoadSound("sounds/battle.wav");      // Battle sound
sounds[2] = LoadSound("sounds/powerup.wav");     // Powerup pickup
sounds[3] = LoadSound("sounds/damage.wav");      // Taking damage
sounds[4] = LoadSound("sounds/death.wav");       // Death sound
sounds[5] = LoadSound("sounds/victory.wav");     // Victory sound
```

And uncomment the sound playing lines throughout the code.

## 🚀 How to Play

1. **Compile**: `clang main.c libraylib.a -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo -o arena`
2. **Run**: `./arena`
3. **Controls**:
   - **WASD**: Move player
   - **R**: Restart when game over
   - **ESC**: Quit

## 🏗️ Technical Features

- **Chunk-based World**: 32×32 cell chunks with LRU cache
- **Movement Cooldowns**: Prevents 60 FPS spam movement
- **Dynamic Loading**: Chunks load/unload based on player distance
- **Procedural Generation**: Random content in each chunk
- **Raylib Graphics**: Hardware-accelerated rendering
- **Audio Integration**: Ready for sound effects

## 🎯 Gameplay Balance

- **Player**: Moves every 6 frames (10/sec), speed boost available
- **Monsters**: Move every 12 frames (5/sec), easier to catch
- **Spawn Rates**: 3-6 monsters, 1-2 powerups, 1-3 landmines per chunk
- **Combat**: Real-time damage exchange with health bars

Enjoy your unlimited RPG adventure! 🗡️⚔️
