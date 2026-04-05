#ifndef TYPES_H
#define TYPES_H

#include "raylib.h"
#include <stdbool.h>

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

#define TILE_SIZE 20
#define TILES_X ((int)(ROOM_WIDTH / TILE_SIZE))
#define TILES_Y ((int)(ROOM_HEIGHT / TILE_SIZE))

/*
 * ============================================================================
 * DUNGEON CONSTANTS
 * ============================================================================
 */

#define MAX_DUNGEON_ROOMS 16
#define DUNGEON_GRID_SIZE 7
#define TARGET_ROOM_COUNT 10
#define DOORWAY_WIDTH 80.0f
#define DOORWAY_HEIGHT 80.0f
#define WALL_THICKNESS 3           // In tiles
#define TRANSITION_DURATION 0.4f
#define TRANSITION_PAUSE 0.1f

/*
 * ============================================================================
 * PLAYER CONSTANTS
 * ============================================================================
 */

#define PLAYER_SPEED 280.0f
#define PLAYER_SIZE 32.0f
#define PLAYER_MAX_HEALTH 100.0f
#define PLAYER_KNOCKBACK_RESISTANCE 0.5f
#define HEART_HP 25.0f
#define PLAYER_INVULN_TIME 1.0f

/*
 * ============================================================================
 * ENEMY CONSTANTS
 * ============================================================================
 */

#define MAX_ENEMIES 32
#define MAX_ENEMIES_PER_ROOM 8
#define ENEMY_SEPARATION_RADIUS 50.0f

// Slime
#define SLIME_SPEED 60.0f
#define SLIME_SIZE 28.0f
#define SLIME_HEALTH 30.0f
#define SLIME_DAMAGE 10.0f
#define SLIME_ATTACK_RANGE 40.0f
#define SLIME_ATTACK_COOLDOWN 1.5f

// Bat
#define BAT_SPEED 150.0f
#define BAT_SIZE 22.0f
#define BAT_HEALTH 15.0f
#define BAT_DAMAGE 8.0f
#define BAT_ATTACK_RANGE 30.0f
#define BAT_ATTACK_COOLDOWN 1.0f

// Skeleton
#define SKELETON_SPEED 70.0f
#define SKELETON_SIZE 42.0f
#define SKELETON_HEALTH 75.0f
#define SKELETON_DAMAGE 20.0f
#define SKELETON_ATTACK_RANGE 200.0f
#define SKELETON_ATTACK_COOLDOWN 2.0f

// Turret
#define TURRET_SIZE 28.0f
#define TURRET_HEALTH 80.0f
#define TURRET_DAMAGE 12.0f
#define TURRET_ATTACK_COOLDOWN 1.5f

/*
 * ============================================================================
 * SWORD / COMBAT CONSTANTS
 * ============================================================================
 */

#define SWORD_SIZE 30.0f
#define SWORD_PICKUP_RADIUS 40.0f
#define SWORD_ATTACK_RANGE 70.0f
#define SWORD_ATTACK_COOLDOWN 0.35f
#define SWORD_BASE_DAMAGE 25.0f
#define SWORD_SWING_DURATION 0.25f
#define SWORD_KNOCKBACK_MULTIPLIER 2.5f

/*
 * ============================================================================
 * PROJECTILE CONSTANTS
 * ============================================================================
 */

#define MAX_PROJECTILES 32
#define PROJECTILE_SPEED 250.0f
#define BONE_PROJECTILE_SPEED 200.0f

/*
 * ============================================================================
 * ITEM CONSTANTS
 * ============================================================================
 */

#define MAX_WORLD_ITEMS 32
#define MAX_SHOP_SLOTS 3
#define ITEM_PICKUP_RADIUS 35.0f
#define ITEM_BOB_SPEED 3.0f

/*
 * ============================================================================
 * BOSS CONSTANTS
 * ============================================================================
 */

#define BOSS_MAX_HEALTH 300.0f
#define BOSS_SIZE 60.0f
#define BOSS_CHARGE_SPEED 500.0f
#define BOSS_CONTACT_DAMAGE 25.0f

/*
 * ============================================================================
 * PARTICLE & PHYSICS CONSTANTS
 * ============================================================================
 */

#define MAX_PARTICLES 512
#define PARTICLE_LIFE 0.8f
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

typedef enum {
    STATE_TITLE,
    STATE_GAMEPLAY,
    STATE_PAUSE,
    STATE_GAME_OVER,
    STATE_VICTORY
} GameState;

typedef enum {
    ROOM_NONE = 0,
    ROOM_START,
    ROOM_COMBAT,
    ROOM_TREASURE,
    ROOM_SHOP,
    ROOM_BOSS
} RoomType;

typedef enum {
    ENEMY_NONE = 0,
    ENEMY_SLIME,
    ENEMY_BAT,
    ENEMY_SKELETON,
    ENEMY_TURRET
} EnemyType;

typedef enum {
    ITEM_NONE = 0,
    ITEM_HEART,
    ITEM_HEART_CONTAINER,
    ITEM_DAMAGE_UP,
    ITEM_SPEED_UP,
    ITEM_KEY,
    ITEM_COIN,
    ITEM_SHIELD,
    ITEM_BOMB
} ItemType;

typedef enum {
    BOSS_IDLE,
    BOSS_PHASE_1,
    BOSS_PHASE_2,
    BOSS_CHARGING,
    BOSS_SHOOTING,
    BOSS_SLAMMING,
    BOSS_SUMMONING,
    BOSS_DYING,
    BOSS_DEAD
} BossPhase;

/*
 * ============================================================================
 * PARTICLE SYSTEM
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
 * SCREEN SHAKE
 * ============================================================================
 */

typedef struct {
    float intensity;
    float duration;
    float frequency;
} ScreenShake;

/*
 * ============================================================================
 * PLAYER INVENTORY
 * ============================================================================
 */

typedef struct {
    int coins;
    int keys;
    float damageBonus;
    float speedBonus;
    int heartContainers;       // Each adds HEART_HP max health
    int enemiesKilled;
    int roomsExplored;
} PlayerInventory;

/*
 * ============================================================================
 * ENTITY STRUCTURES
 * ============================================================================
 */

typedef struct {
    Vector2 pos;
    Vector2 velocity;
    Vector2 knockback;
    float radius;

    float health;
    float maxHealth;

    int facing;                 // 0=right, 1=down, 2=left, 3=up
    float animTimer;
    int animFrame;
    bool moving;

    bool hasSword;
    bool isAttacking;
    float attackTimer;
    float attackCooldown;
    float swordAngle;
    float invulnTimer;
    float flashTimer;          // White flash on damage
    float shieldTimer;         // Shield powerup

    PlayerInventory inventory;
} Player;

typedef struct {
    Vector2 pos;
    Vector2 velocity;
    Vector2 knockback;
    float radius;

    int roomId;                // Which dungeon room this enemy belongs to
    EnemyType type;

    float health;
    float maxHealth;

    int facing;
    float animTimer;
    int animFrame;

    bool active;
    bool isAttacking;
    float attackTimer;
    float attackCooldown;
    float flashTimer;

    // AI behavior
    Vector2 targetPos;
    float stateTimer;
    float preferredAngle;
    float circleRadius;
    float hopTimer;            // For slime hop animation
    float sineOffset;          // For bat sine wave
    float aimAngle;            // For turret rotation
} Enemy;

typedef struct {
    Vector2 pos;
    int roomId;
    bool active;
    float bobPhase;
    float spinAngle;
} Sword;

/*
 * ============================================================================
 * PROJECTILE SYSTEM
 * ============================================================================
 */

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float radius;
    float damage;
    float lifetime;
    bool active;
    bool friendly;             // false = enemy, true = player
    int roomId;
    Color color;
    float rotation;
    float rotSpeed;
} Projectile;

/*
 * ============================================================================
 * ITEM SYSTEM
 * ============================================================================
 */

typedef struct {
    ItemType type;
    Vector2 pos;
    int roomId;
    bool active;
    bool collected;
    float bobPhase;
    int price;                 // 0 = free pickup, >0 = shop item
    float spawnVelY;           // Upward velocity for spawn pop
    float spawnGround;         // Ground Y for bounce landing
} WorldItem;

typedef struct {
    ItemType item;
    int price;
    bool sold;
    Vector2 pos;
} ShopSlot;

/*
 * ============================================================================
 * DUNGEON SYSTEM
 * ============================================================================
 */

typedef struct {
    int id;
    int gridX, gridY;
    RoomType type;
    bool visited;
    bool cleared;
    bool doorsLocked;          // Locked during combat
    int connections[4];        // Room IDs: [N, E, S, W], -1 = no connection
    bool doorLocked[4];        // Key-locked doors
    int enemyCount;
    unsigned int seed;         // For deterministic decorations
    int distance;              // From start room (for difficulty scaling)
} DungeonRoom;

typedef struct {
    DungeonRoom rooms[MAX_DUNGEON_ROOMS];
    int roomCount;
    int currentRoomId;
    int previousRoomId;
    int startRoomId;
    int bossRoomId;

    // Transition state
    bool isTransitioning;
    float transitionTimer;
    float transitionPhase;     // 0 = fade out, 1 = fade in
    Direction transitionDirection;
    Vector2 cameraOffset;
    float fadeAlpha;           // For fade transition
} Dungeon;

/*
 * ============================================================================
 * BOSS
 * ============================================================================
 */

typedef struct {
    Vector2 pos;
    Vector2 velocity;
    Vector2 knockback;
    float health;
    float maxHealth;
    float radius;
    BossPhase phase;
    float stateTimer;
    float attackCooldown;
    int currentPattern;
    float flashTimer;
    float animTimer;
    int animFrame;
    bool active;
    Vector2 chargeTarget;      // Where boss is charging to
    float chargeWindup;        // Telegraph timer
    int minionCount;
} Boss;

/*
 * ============================================================================
 * GAME STATE MANAGER
 * ============================================================================
 */

typedef struct {
    GameState current;
    GameState previous;
    float transitionAlpha;
    float stateTimer;          // Time in current state
} GameStateManager;

/*
 * ============================================================================
 * SPAWN TEMPLATES
 * ============================================================================
 */

typedef struct {
    EnemyType types[MAX_ENEMIES_PER_ROOM];
    int count;
} SpawnTemplate;

/*
 * ============================================================================
 * GLOBAL STATE (extern declarations)
 * ============================================================================
 */

extern Player player;
extern Enemy enemies[MAX_ENEMIES];
extern Sword sword;
extern ParticleSystem particles;
extern Dungeon dungeon;
extern Projectile projectiles[MAX_PROJECTILES];
extern WorldItem worldItems[MAX_WORLD_ITEMS];
extern Boss boss;
extern ScreenShake screenShake;
extern GameStateManager gameState;

extern float gameTime;
extern float hitFreezeTimer;
extern Vector2 playerSpawnPoint;

extern float musicVolume;
extern float sfxVolume;
extern float screenOffsetX;    // For centering in fullscreen
extern float screenOffsetY;

extern Sound hitSound;
extern Music backgroundMusic;
extern Music titleMusic;
extern Music gameplayMusic;
extern float musicFadeTimer;   // Crossfade timer (>0 = fading)

#endif // TYPES_H
