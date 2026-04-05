#include "raylib.h"
#include "rlgl.h"
#include "types.h"
#include "entities.h"
#include "systems.h"

/*
 * ============================================================================
 * GLOBAL STATE DEFINITIONS
 * ============================================================================
 */

Player player;
Enemy enemies[MAX_ENEMIES];
Sword sword;
ParticleSystem particles;
RoomSystem roomSystem;

float gameTime;
Vector2 playerSpawnPoint;
Vector2 swordSpawnPoint;

Sound hitSound;
Music backgroundMusic;

int currentWave;
int enemiesInWave;
bool waveComplete;
float waveTransitionTimer;


/*
 * ============================================================================
 * AUDIO HELPERS
 * ============================================================================
 */

// Realistic hit playback with slight randomness (safe + non-breaking)
static void PlayHitSound(Sound sound)
{
    float pitch = (float)GetRandomValue(92,108) / 100.0f;
    float volume = (float)GetRandomValue(85,100) / 100.0f;

    SetSoundPitch(sound, pitch);
    SetSoundVolume(sound, volume);
    PlaySound(sound);
}


/*
 * ============================================================================
 * GAME INITIALIZATION
 * ============================================================================
 */

static void GameInit(void) {
    PlayerInit();
    SwordInit();
    RoomSystemInit();
    
    particles.count = 0;
    
    currentWave = 0;
    waveComplete = false;
    waveTransitionTimer = 0.0f;
    
    StartWave(1);
}

/*
 * ============================================================================
 * MAIN GAME LOOP
 * ============================================================================
 */

int main(void) {
    // Initialize window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "DUCK QUEST");
    SetTargetFPS(144);
    
    // Initialize audio
    InitAudioDevice();
    
    // Load assets
    Texture2D playerSheet = LoadTexture("/Users/vicentevigueras/Developer/game_poc/assets/sprites/player/player.png");
    Texture2D enemySheet = LoadTexture("/Users/vicentevigueras/Developer/game_poc/assets/sprites/player/enemy.png");
    hitSound = LoadSound("/Users/vicentevigueras/Developer/game_poc/assets/sounds/hit_sound.wav");
    backgroundMusic = LoadMusicStream("/Users/vicentevigueras/Developer/game_poc/assets/sounds/pixel_drift.mp3");
    
    // Configure music
    backgroundMusic.looping = true;
    SetMusicVolume(backgroundMusic, 0.5f);
    PlayMusicStream(backgroundMusic);
    
    // Initialize game state
    GameInit();
    gameTime = 0.0f;
    
    // Main game loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;  // Cap delta time to prevent spiral of death
        gameTime += dt;
        
        // Handle restart
        if (IsKeyPressed(KEY_R)) {
            GameInit();
        }
        
        // Update music
        UpdateMusicStream(backgroundMusic);
        
        // Check wave completion
        bool allDead = true;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                allDead = false;
                break;
            }
        }
        
        if (allDead && !waveComplete) {
            waveComplete = true;
            waveTransitionTimer = 0.0f;
        }
        
        // Auto-advance to next wave
        if (waveComplete && currentWave < 10) {
            waveTransitionTimer += dt;
            if (waveTransitionTimer >= 3.0f) {
                StartWave(currentWave + 1);
            }
        }
        
        // Update game systems
        PlayerUpdate(dt);
        CheckDoorwayCollision();
        
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                EnemyUpdate(&enemies[i], dt);
            }
        }
        
        SwordUpdate(dt);
        ParticlesUpdate(dt);
        RoomSystemUpdate(dt);
        
        // Render
        BeginDrawing();
        ClearBackground((Color){ 20, 25, 35, 255 });
        
        // Apply camera offset for room transitions
        Vector2 camOffset = GetCameraOffset();
        
        rlPushMatrix();
        rlTranslatef(camOffset.x, camOffset.y, 0.0f);
        
        // Draw world
        RoomSystemDraw();
        SwordDraw();
        ParticlesDraw();
        
        // Draw enemies with shadows
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!enemies[i].active) continue;
            
            float roomOffsetX = enemies[i].roomX * ROOM_WIDTH;
            float roomOffsetY = enemies[i].roomY * ROOM_HEIGHT;
            
            // Draw shadow
            DrawCircle(
                (int)(enemies[i].pos.x + roomOffsetX),
                (int)(enemies[i].pos.y + roomOffsetY + 3),
                ENEMY_SIZE * 0.8f,
                Fade(BLACK, 0.3f)
            );
            
            EnemyDraw(&enemies[i], enemySheet);
        }
        
        // Draw player with shadow
        float playerRoomOffsetX = roomSystem.currentRoomX * ROOM_WIDTH;
        float playerRoomOffsetY = roomSystem.currentRoomY * ROOM_HEIGHT;
        
        DrawCircle(
            (int)(player.pos.x + playerRoomOffsetX),
            (int)(player.pos.y + playerRoomOffsetY + 3),
            PLAYER_SIZE * 0.8f,
            Fade(BLACK, 0.3f)
        );
        
        PlayerDraw(playerSheet);
        
        rlPopMatrix();
        
        // Draw UI (not affected by camera)
        UIDraw();
        
        EndDrawing();
    }
    
    // Cleanup
    UnloadMusicStream(backgroundMusic);
    UnloadSound(hitSound);
    UnloadTexture(playerSheet);
    UnloadTexture(enemySheet);
    
    CloseAudioDevice();
    CloseWindow();
    
    return 0;
}
