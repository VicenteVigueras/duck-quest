Game Summary: DUCK QUEST
What It Is
A 2D dungeon-crawler action game built in C using the Raylib graphics library. It features a top-down perspective with wave-based combat and multi-room exploration.

Core Gameplay
Combat: The player controls a character that fights off waves of enemies using a sword as the primary weapon
Waves: The game progresses through multiple waves (up to 10) of enemies that spawn and must be defeated to advance
Rooms: The game world consists of a 2x2 grid of interconnected rooms that players can explore and transition between via doorways
Sword Mechanics: A sword is a pickup item that spawns in rooms and provides a melee attack capability with a swing animation and cooldown system
Goal
Defeat all enemies in successive waves to progress through 10 waves. The game auto-advances to the next wave 3 seconds after all enemies are defeated.

Key Game Systems
Player System

Movement: 300 units/sec speed
Health: 100 HP max
Sword pickup and melee attack capability
Animation and knockback mechanics
Invulnerability timer after taking damage
Enemy System

Up to 10 enemies active at once
Pathfinding toward the player with AI behavior (orbiting/circling)
Attack range: 45 units, dealing 15 damage per hit
Health: 50 HP each
Knockback and separation mechanics to prevent clumping
Sword System

Attack range: 100 units
25 damage per hit with 0.5 second cooldown
Deals 2.5x knockback multiplier
3-second respawn time after pickup
Bob and spin animations while on ground
Room/Dungeon System

2x2 grid of interconnected rooms
Smooth camera transitions between rooms (1.5 seconds)
Doorway-based navigation
Room tinting for visual variation
Visited room tracking
Visual & Audio Systems

Particle effects (blood, sparks, dust)
Shadow rendering for entities
Background music (pixel_drift.mp3)
Hit sound effects with pitch/volume randomization
UI displays: health bar, wave info, controls, room indicator, FPS
Input Controls
Movement: Keyboard input (WASD or arrow keys implied)
Attack: Sword swing mechanics
Room navigation: Move through doorways
Restart: R key
The project is in active development with the developer taking more ownership after the initial AI-assisted prototyping phase. The latest build version is 2d_game_v_2.