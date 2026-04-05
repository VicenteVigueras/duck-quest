#ifndef TYPES_H
#define TYPES_H

#include "raylib.h"

/*
 * ============================================================================
 * SCREEN & ROOM CONSTANTS
 * ============================================================================
 */

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define ROOM_WIDTH 1000.0f
#define ROOM_HEIGHT 600.0f
#define ROOM_X ((SCREEN_WIDTH - ROOM_WIDTH) * 0.5f)
#define ROOM_Y ((SCREEN_HEIGHT - ROOM_HEIGHT) * 0.5f)

/*
 * ============================================================================
 * ROOM SYSTEM CONSTANTS
 * ============================================================================
 */

#define MAX_ROOMS_X 2
#define MAX_ROOMS_Y 2
#define DOORWAY_WIDTH 80.0f
#define DOORWAY_HEIGHT 80.0f
#define TRANSITION_DURATION 1.5f    // Seconds for room transition animation
#define TRANSITION_PAUSE 0.2f       // Brief pause before transition starts

/*
 * ============================================================================
 * PLAYER CONSTANTS
 * ============================================================================
 */

#define PLAYER_SPEED 300.0f
#define PLAYER_SIZE 40.0f
#define PLAYER_MAX_HEALTH 100.0f
#define PLAYER_KNOCKBACK_RESISTANCE 0.5f

/*
 * ============================================================================
 * ENEMY CONSTANTS
 * ============================================================================
 */

#define MAX_ENEMIES 10
#define ENEMY_SPEED 80.0f
#define ENEMY_SIZE 32.0f
#define ENEMY_HEALTH 50.0f
#define ENEMY_ATTACK_RANGE 45.0f
#define ENEMY_ATTACK_COOLDOWN 1.2f
#define ENEMY_DAMAGE 15.0f
#define ENEMY_SEPARATION_RADIUS 60.0f
#define ENEMY_PLAYER_MIN_DISTANCE 35.0f

/*
 * ============================================================================
 * SWORD CONSTANTS
 * ============================================================================
 */

#define SWORD_SIZE 30.0f
#define SWORD_PICKUP_RADIUS 40.0f
#define SWORD_ATTACK_RANGE 100.0f
#define SWORD_ATTACK_COOLDOWN 0.5f
#define SWORD_DAMAGE 25.0f
#define SWORD_SWING_DURATION 0.3f
#define SWORD_RESPAWN_TIME 3.0f
#define SWORD_KNOCKBACK_MULTIPLIER 2.5f

/*
 * ============================================================================
 * PARTICLE SYSTEM CONSTANTS
 * ============================================================================
 */

#define MAX_PARTICLES 512
#define PARTICLE_LIFE 0.8f

/*
 * ============================================================================
 * ANIMATION & PHYSICS CONSTANTS
 * ============================================================================
 */

#define ANIM_SPEED 0.1f
#define KNOCKBACK_STRENGTH 400.0f
#define KNOCKBACK_DECAY 6.0f

/*
 * ============================================================================
 * ENUMERATIONS
 * ============================================================================
 */

typedef enum {
    PTYPE_BLOOD,
    PTYPE_SPARK,
    PTYPE_DUST
} ParticleType;

typedef enum {
    DIR_NORTH = 0,
    DIR_EAST = 1,
    DIR_SOUTH = 2,
    DIR_WEST = 3
} Direction;

/*
 * ============================================================================
 * PARTICLE SYSTEM STRUCTURES
 * ============================================================================
 */

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    float size;
    Color color;
    ParticleType type;
} Particle;

typedef struct {
    Particle pool[MAX_PARTICLES];
    int count;
} ParticleSystem;

/*
 * ============================================================================
 * ENTITY STRUCTURES
 * ============================================================================
 */

typedef struct {
    // Position & movement
    Vector2 pos;
    Vector2 velocity;
    Vector2 knockback;
    float radius;
    
    // Health
    float health;
    float maxHealth;
    
    // Animation
    int facing;                 // 0=right, 1=down, 2=left, 3=up
    float animTimer;
    int animFrame;
    bool moving;
    
    // Combat
    bool hasSword;
    bool isAttacking;
    float attackTimer;
    float attackCooldown;
    float swordAngle;
    float invulnTimer;
} Player;

typedef struct {
    // Position & movement
    Vector2 pos;
    Vector2 velocity;
    Vector2 knockback;
    float radius;
    
    // Room assignment
    int roomX;
    int roomY;
    
    // Health
    float health;
    float maxHealth;
    
    // Animation
    int facing;                 // 0=right, 1=down, 2=left, 3=up
    float animTimer;
    int animFrame;
    
    // State
    bool active;
    bool isAttacking;
    float attackTimer;
    float attackCooldown;
    
    // AI behavior
    Vector2 targetPos;
    float stateTimer;
    float preferredAngle;       // For orbiting behavior
    float circleRadius;
} Enemy;

typedef struct {
    Vector2 pos;
    int roomX;
    int roomY;
    bool active;
    float bobPhase;
    float spinAngle;
    float respawnTimer;
} Sword;

/*
 * ============================================================================
 * ROOM SYSTEM STRUCTURES
 * ============================================================================
 */

typedef struct {
    Vector2 center;
    Direction direction;
    bool active;
} Doorway;

typedef struct {
    int x;                      // Grid position X
    int y;                      // Grid position Y
    Color tint;                 // Visual variation
    Doorway doorways[4];        // North, East, South, West
    bool visited;
} Room;

typedef struct {
    Room rooms[MAX_ROOMS_X][MAX_ROOMS_Y];
    
    // Current room tracking
    int currentRoomX;
    int currentRoomY;
    int previousRoomX;          // For transition rendering
    int previousRoomY;
    
    // Transition state
    bool isTransitioning;
    float transitionTimer;
    float pauseTimer;
    Direction transitionDirection;
    Vector2 transitionOffset;
    Vector2 cameraOffset;       // For smooth scrolling
} RoomSystem;

/*
 * ============================================================================
 * GLOBAL STATE (extern declarations)
 * ============================================================================
 */

extern Player player;
extern Enemy enemies[MAX_ENEMIES];
extern Sword sword;
extern ParticleSystem particles;
extern RoomSystem roomSystem;

extern float gameTime;
extern Vector2 playerSpawnPoint;
extern Vector2 swordSpawnPoint;

extern Sound hitSound;
extern Music backgroundMusic;

extern int currentWave;
extern int enemiesInWave;
extern bool waveComplete;
extern float waveTransitionTimer;

#endif // TYPES_H