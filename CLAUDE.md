# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
make          # Compile
make run      # Compile and run (from repo root — assets loaded via relative paths)
make clean    # Remove build artifacts
make rebuild  # Clean + recompile
```

**Compiler:** GCC, C99, `-Wall -Wextra -O2`
**Platform:** macOS (links against Raylib + CoreVideo, IOKit, Cocoa, GLUT, OpenGL)
**Output:** `build/2d_game/duck_quest`

No automated tests — run `make run` and test manually:
- **WASD** move, **SPACE** attack, **ESC** pause, **R** restart, **ENTER** start from title

## Architecture

The game is a 2D dungeon crawler ("Duck Quest") built in C with [Raylib](https://www.raylib.com/). It features procedurally generated dungeon layouts with room-based exploration, multiple enemy types, items/progression, and a boss fight.

### Source files (`src/2d_game/`)

| File | Responsibility |
|------|---------------|
| `types.h` | All structs, enums, and game constants (single source of truth) |
| `utils.h` | Static inline math, collision, physics, hashing, easing utilities |
| `main.c` | Window/audio init, game state machine, game loop, render orchestration |
| `entities.c/h` | Player, 4 enemy types (slime/bat/skeleton/turret), sword — update & draw |
| `systems.c/h` | Particles, HUD (hearts/keys/coins), game state screens (title/pause/gameover/victory) |
| `dungeon.c/h` | Procedural dungeon generation (random walk), room transitions, doorway logic |
| `renderer.c/h` | 8-bit tile rendering, walls, doorways, decorations, torches, pixel-art item icons |
| `combat.c/h` | Screen shake, hit flash/freeze, projectile system, damage application |
| `items.c/h` | Item spawning, pickup, drops, shop room, inventory effects |
| `boss.c/h` | Boss AI ("Bone King") — 2 phases, 3 attack patterns, minion spawning |
| `minimap.c/h` | Minimap overlay with fog of war |

### Game loop (`main.c`)
State machine: TITLE → GAMEPLAY → PAUSE / GAME_OVER / VICTORY.
Each gameplay frame: process input → PlayerUpdate → DungeonCheckDoorways → DungeonUpdate → EnemyUpdate (×32) → BossUpdate → SwordUpdate → ProjectilesUpdate → ItemsUpdate → ParticlesUpdate → ScreenShakeUpdate → render room + entities + HUD.

### Key systems

- **Dungeon generation** — Random walk on 7×7 grid → ~10 rooms. Types: Start, Combat, Treasure, Shop, Boss. BFS assigns difficulty by distance. Boss room is farthest from start.
- **Room system** — Flat array of `DungeonRoom` with `connections[4]` graph. Fade transitions (0.4s). Doors lock during combat, unlock when cleared. Key-locked boss door.
- **Combat** — Screen shake + hit flash (0.1s white) + hit freeze (0.03s pause). Circle-based collision. 4 enemy types with distinct AI. Projectile system (32 pool) for skeleton/turret/boss.
- **Items** — 8 types: Heart, Heart Container, Damage Up, Speed Up, Key, Coin, Shield, Bomb. Enemy drops (35% coin, 10% heart, 3% stat). Shop room with 3 priced items. Treasure rooms with guaranteed upgrades.
- **Boss** — "Bone King", 300 HP, 2 phases. Phase 1: Charge/Bone Spray/Slam cycle. Phase 2 (<50% HP): faster + bat minion summons. Death: freeze → flash → 60-particle explosion → victory.
- **HUD** — Pixel-art hearts (25 HP each), key/coin counters, minimap (top-right, fog of war), room name flash on entry.
- **Particle pool** — Fixed 512-slot pool; types: Blood, Spark, Dust.
- **Rendering** — NES-inspired palette, tile-based checkerboard floors, beveled brick walls, animated torches, programmatic 8-bit enemy sprites (no sprite sheets for enemies).

### Assets
```
assets/sounds/   pixel_drift.mp3 (BGM), hit_sound.wav (SFX)
assets/sprites/player/   player.png (player spritesheet, 13×13 frames)
```

> **Note:** Assets are loaded via relative paths. Run the game from the repository root directory.
