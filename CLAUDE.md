# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
make          # Compile
make run      # Compile and run
make clean    # Remove build artifacts
make rebuild  # Clean + recompile
```

**Compiler:** GCC, C99, `-Wall -Wextra -O2`
**Platform:** macOS (links against Raylib + CoreVideo, IOKit, Cocoa, GLUT, OpenGL)
**Output:** `build/2d_game/2d_game_v_2`

No automated tests — run `make run` and test manually (WASD move, SPACE attack, R restart).

## Architecture

The game is a 2D action prototype ("Duck Quest") built in C with [Raylib](https://www.raylib.com/).

### Source files (`src/2d_game/`)

| File | Responsibility |
|------|---------------|
| `types.h` | All structs, enums, and game constants (single source of truth) |
| `main.c` | Window/audio init, asset loading, game loop, render orchestration |
| `entities.c/h` | Player, enemies, sword — update logic and collision |
| `systems.c/h` | Particles, UI, wave spawning, room transitions |

### Game loop (`main.c`)
Each frame: process input → call `PlayerUpdate` / `EnemyUpdate` / `SwordUpdate` / `ParticlesUpdate` / `RoomSystemUpdate` → render everything → draw UI overlay.

### Key systems

- **Room system** — 2×2 grid (4 rooms, each 1000×600 units). Camera smoothly translates during doorway transitions (1.5 s).
- **Wave system** — Wave N spawns N enemies (max 10 waves, max 10 active enemies). Tracked per room.
- **Particle pool** — Fixed 512-slot pool; types: Blood, Spark, Dust.
- **Combat** — Circle-based collision. Sword: 25 dmg, 100-unit range, 0.5 s cooldown. Enemies: 15 dmg, 45-unit range, 1.2 s cooldown. Knockback with velocity decay.

### Assets
```
assets/sounds/   pixel_drift.mp3 (BGM), hit_sound.wav (SFX)
assets/sprites/player/   player.png, enemy.png (spritesheets)
```

> **Note:** The Makefile currently hard-codes absolute paths for the build output directory (`/Users/vicentevigueras/Developer/game_poc/build/...`). Asset paths in code may also be absolute — check before adding new assets.
