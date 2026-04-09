#include "raylib.h"
#include "rlgl.h"
#include "types.h"
#include "utils.h"
#include "entities.h"
#include "systems.h"
#include "dungeon.h"
#include "renderer.h"
#include "combat.h"
#include "items.h"
#include "boss.h"
#include "minimap.h"
#include <string.h>
#include <time.h>

/*
 * ============================================================================
 * GLOBAL STATE DEFINITIONS
 * ============================================================================
 */

Player player;
Enemy enemies[MAX_ENEMIES];
Sword sword;
ParticleSystem particles;
Dungeon dungeon;
Projectile projectiles[MAX_PROJECTILES];
WorldItem worldItems[MAX_WORLD_ITEMS];
Boss boss;
ScreenShake screenShake;
GameStateManager gameState;

float gameTime;
float hitFreezeTimer;
Vector2 playerSpawnPoint;

float musicVolume = 0.5f;
float sfxVolume = 0.7f;
float screenOffsetX = 0.0f;
float screenOffsetY = 0.0f;

Sound hitSound;
Sound hitSoundSlay;
Music backgroundMusic;
Music titleMusic;
Music gameplayMusic;
Music bossMusic;
float musicFadeTimer;
bool bossRoomActive = false;
float bossMusicFadeTimer = 0.0f;

/*
 * ============================================================================
 * AUDIO HELPERS
 * ============================================================================
 */

static void PlayHitSound(Sound sound) {
    float pitch = (float)GetRandomValue(92, 108) / 100.0f;
    float volume = (float)GetRandomValue(85, 100) / 100.0f;
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
    // Clear all state
    memset(enemies, 0, sizeof(enemies));
    memset(projectiles, 0, sizeof(projectiles));
    memset(&boss, 0, sizeof(boss));
    memset(&screenShake, 0, sizeof(screenShake));
    particles.count = 0;
    hitFreezeTimer = 0;

    // Generate dungeon
    unsigned int seed = (unsigned int)time(NULL);
    DungeonGenerate(&dungeon, seed);

    // Init player
    PlayerInit();

    // Place sword in start room
    SwordInit(dungeon.startRoomId);

    // Init items
    ItemsInit();

    // Setup shop room
    for (int i = 0; i < dungeon.roomCount; i++) {
        if (dungeon.rooms[i].type == ROOM_SHOP) {
            ShopRoomSetup(i);
        }
        if (dungeon.rooms[i].type == ROOM_TREASURE) {
            // Place a guaranteed upgrade in treasure room
            Vector2 pos = { ROOM_X + ROOM_WIDTH * 0.5f, ROOM_Y + ROOM_HEIGHT * 0.5f };
            ItemType treasureItem;
            int roll = GetRandomValue(0, 2);
            if (roll == 0) treasureItem = ITEM_HEART_CONTAINER;
            else if (roll == 1) treasureItem = ITEM_DAMAGE_UP;
            else treasureItem = ITEM_SPEED_UP;
            ItemSpawn(treasureItem, pos, i, 0);
        }
    }

    // Place a key in one of the mid-distance combat rooms
    for (int i = 1; i < dungeon.roomCount; i++) {
        if (dungeon.rooms[i].type == ROOM_COMBAT && dungeon.rooms[i].distance >= 2) {
            // This room will drop a key when cleared (spawn key at center)
            Vector2 pos = { ROOM_X + ROOM_WIDTH * 0.6f, ROOM_Y + ROOM_HEIGHT * 0.6f };
            ItemSpawn(ITEM_KEY, pos, i, 0);
            break;
        }
    }

    // Place rubber ducks randomly in some rooms (~1 in 3 combat rooms gets one)
    for (int i = 1; i < dungeon.roomCount; i++) {
        if (dungeon.rooms[i].type == ROOM_COMBAT && GetRandomValue(0, 2) == 0) {
            float duckX = ROOM_X + ROOM_WIDTH * (0.2f + (float)GetRandomValue(0, 60) / 100.0f);
            float duckY = ROOM_Y + ROOM_HEIGHT * (0.2f + (float)GetRandomValue(0, 60) / 100.0f);
            Vector2 duckPos = { duckX, duckY };
            ItemSpawn(ITEM_RUBBER_DUCK, duckPos, i, 0);
        }
    }

    // Reset room name flash tracking
    ResetRoomNames();

    // Reset boss music state
    bossRoomActive = false;
    bossMusicFadeTimer = 0.0f;
    StopMusicStream(bossMusic);
    SetMusicVolume(gameplayMusic, musicVolume);

    // Game state
    gameState.current = STATE_GAMEPLAY;
    gameState.stateTimer = 0;
}

/*
 * ============================================================================
 * MAIN GAME LOOP
 * ============================================================================
 */

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "DUCK QUEST — Into the Dungeon");
    SetTargetFPS(144);

    InitAudioDevice();

    // Load assets (use relative paths via working directory)
    Texture2D playerSheet = LoadTexture("assets/sprites/player/player.png");
    Texture2D enemySheet = LoadTexture("assets/sprites/player/enemy.png");
    hitSound = LoadSound("assets/sounds/hit_sound.mp3");
    hitSoundSlay = LoadSound("assets/sounds/hit_sound_slay.mp3");

    // Load music tracks
    titleMusic = LoadMusicStream("assets/sounds/pixel_drift.mp3");
    titleMusic.looping = true;
    gameplayMusic = LoadMusicStream("assets/sounds/castle.mp3");
    gameplayMusic.looping = true;
    bossMusic = LoadMusicStream("assets/sounds/boss_lair.mp3");
    bossMusic.looping = true;
    musicFadeTimer = 0.0f;
    bossMusicFadeTimer = 0.0f;
    bossRoomActive = false;

    backgroundMusic = titleMusic;
    SetMusicVolume(titleMusic, musicVolume);
    SetMusicVolume(gameplayMusic, 0.0f);
    SetMusicVolume(bossMusic, 0.0f);
    PlayMusicStream(titleMusic);

    // Disable ESC as exit key (we use ESC for pause)
    SetExitKey(0);

    gameTime = 0.0f;
    gameState.current = STATE_TITLE;
    gameState.stateTimer = 0;

    (void)PlayHitSound; // Suppress unused warning; called via PlaySound directly
    (void)enemySheet;   // Enemies now drawn programmatically

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;
        gameTime += dt;

        // Compute centering offset for any window size
        int winW = GetScreenWidth();
        int winH = GetScreenHeight();
        screenOffsetX = (winW - SCREEN_WIDTH) * 0.5f;
        screenOffsetY = (winH - SCREEN_HEIGHT) * 0.5f;
        if (screenOffsetX < 0) screenOffsetX = 0;
        if (screenOffsetY < 0) screenOffsetY = 0;

        UpdateMusicStream(titleMusic);
        UpdateMusicStream(gameplayMusic);
        UpdateMusicStream(bossMusic);

        // Crossfade between title and gameplay music
        if (musicFadeTimer > 0.0f) {
            musicFadeTimer -= dt;
            if (musicFadeTimer < 0.0f) musicFadeTimer = 0.0f;
            float fadeDuration = 1.5f;
            float t = 1.0f - (musicFadeTimer / fadeDuration); // 0→1 over fade
            if (t > 1.0f) t = 1.0f;
            // Crossfade: title fades out, gameplay fades in
            SetMusicVolume(titleMusic, musicVolume * (1.0f - t));
            SetMusicVolume(gameplayMusic, musicVolume * t);
            if (musicFadeTimer <= 0.0f) {
                StopMusicStream(titleMusic);
                SetMusicVolume(gameplayMusic, musicVolume);
            }
        }

        // Boss music crossfade (gameplay <-> boss)
        if (bossMusicFadeTimer > 0.0f) {
            bossMusicFadeTimer -= dt;
            if (bossMusicFadeTimer < 0.0f) bossMusicFadeTimer = 0.0f;
            float fadeDuration = 2.0f;
            float t = 1.0f - (bossMusicFadeTimer / fadeDuration);
            if (t > 1.0f) t = 1.0f;
            if (bossRoomActive) {
                // Fade gameplay out, boss in
                SetMusicVolume(gameplayMusic, musicVolume * (1.0f - t));
                SetMusicVolume(bossMusic, musicVolume * t);
            } else {
                // Fade boss out, gameplay in
                SetMusicVolume(bossMusic, musicVolume * (1.0f - t));
                SetMusicVolume(gameplayMusic, musicVolume * t);
                if (bossMusicFadeTimer <= 0.0f) {
                    StopMusicStream(bossMusic);
                }
            }
        }

        // Check if we need to transition to/from boss music
        if (gameState.current == STATE_GAMEPLAY && musicFadeTimer <= 0.0f) {
            DungeonRoom *musicRoom = DungeonGetCurrentRoom();
            bool inBossRoom = (musicRoom->type == ROOM_BOSS && !dungeon.isTransitioning);
            if (inBossRoom && !bossRoomActive) {
                bossRoomActive = true;
                PlayMusicStream(bossMusic);
                SetMusicVolume(bossMusic, 0.0f);
                bossMusicFadeTimer = 2.0f;
            } else if (!inBossRoom && bossRoomActive) {
                bossRoomActive = false;
                bossMusicFadeTimer = 2.0f;
            }
        }

        // Global volume controls (work in any state)
        {
            float volStep = 0.05f;
            bool volChanged = false;
            if (IsKeyPressed(KEY_EQUAL) || IsKeyPressedRepeat(KEY_EQUAL)) { // + key
                musicVolume += volStep;
                if (musicVolume > 1.0f) musicVolume = 1.0f;
                volChanged = true;
            }
            if (IsKeyPressed(KEY_MINUS) || IsKeyPressedRepeat(KEY_MINUS)) { // - key
                musicVolume -= volStep;
                if (musicVolume < 0.0f) musicVolume = 0.0f;
                volChanged = true;
            }

            if (volChanged && musicFadeTimer <= 0.0f && bossMusicFadeTimer <= 0.0f) {
                if (gameState.current == STATE_TITLE) {
                    SetMusicVolume(titleMusic, musicVolume);
                } else if (bossRoomActive) {
                    SetMusicVolume(bossMusic, musicVolume);
                } else {
                    SetMusicVolume(gameplayMusic, musicVolume);
                }
            }
            if (IsKeyPressed(KEY_RIGHT_BRACKET) || IsKeyPressedRepeat(KEY_RIGHT_BRACKET)) { // ] for SFX+
                sfxVolume += volStep;
                if (sfxVolume > 1.0f) sfxVolume = 1.0f;
            }
            if (IsKeyPressed(KEY_LEFT_BRACKET) || IsKeyPressedRepeat(KEY_LEFT_BRACKET)) { // [ for SFX-
                sfxVolume -= volStep;
                if (sfxVolume < 0.0f) sfxVolume = 0.0f;
            }
        }

        /*
         * ====================================================================
         * STATE MACHINE UPDATE
         * ====================================================================
         */

        switch (gameState.current) {
            case STATE_TITLE: {
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                    GameInit();
                    gameState.current = STATE_GAMEPLAY;
                    // Crossfade from title to gameplay music
                    SeekMusicStream(gameplayMusic, 0.0f);
                    SetMusicVolume(gameplayMusic, 0.0f);
                    PlayMusicStream(gameplayMusic);
                    musicFadeTimer = 1.5f; // 1.5s crossfade
                    // Reset boss music state
                    bossRoomActive = false;
                    bossMusicFadeTimer = 0.0f;
                }
                break;
            }

            case STATE_GAMEPLAY: {
                // Hit freeze (skip updates for impact feel)
                if (hitFreezeTimer > 0) {
                    hitFreezeTimer -= dt;
                    break;
                }

                // Pause
                if (IsKeyPressed(KEY_ESCAPE)) {
                    gameState.current = STATE_PAUSE;
                    break;
                }

                // Restart
                if (IsKeyPressed(KEY_R)) {
                    GameInit();
                    break;
                }

                // DEBUG: Teleport to boss room
                if (IsKeyPressed(KEY_P)) {
                    int bossId = dungeon.bossRoomId;
                    // Unlock all boss doors so we can enter
                    for (int d = 0; d < 4; d++) {
                        dungeon.rooms[bossId].doorLocked[d] = false;
                        int nid = dungeon.rooms[bossId].connections[d];
                        if (nid >= 0) {
                            for (int d2 = 0; d2 < 4; d2++) {
                                if (dungeon.rooms[nid].connections[d2] == bossId) {
                                    dungeon.rooms[nid].doorLocked[d2] = false;
                                }
                            }
                        }
                    }
                    // Give player a key and sword just in case
                    player.hasSword = true;
                    player.inventory.keys++;
                    // Move to boss room
                    dungeon.currentRoomId = bossId;
                    dungeon.rooms[bossId].visited = true;
                    player.pos = (Vector2){ ROOM_X + ROOM_WIDTH * 0.5f, ROOM_Y + ROOM_HEIGHT * 0.7f };
                    // Init boss if not already
                    if (!boss.active && boss.phase != BOSS_DEAD) {
                        BossInit(bossId);
                    }
                    break;
                }

                // Update game systems
                if (!dungeon.isTransitioning) {
                    PlayerUpdate(dt);
                    DungeonCheckDoorways();
                }
                DungeonUpdate(dt);

                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (enemies[i].active) {
                        EnemyUpdate(&enemies[i], dt);
                    }
                }

                BossUpdate(dt);
                SwordUpdate(dt);
                ProjectilesUpdate(dt);
                ItemsUpdate(dt);
                ParticlesUpdate(dt);
                ScreenShakeUpdate(dt);

                // Track rooms explored
                DungeonRoom *curRoom = DungeonGetCurrentRoom();
                if (curRoom->visited) {
                    int explored = 0;
                    for (int i = 0; i < dungeon.roomCount; i++) {
                        if (dungeon.rooms[i].visited) explored++;
                    }
                    player.inventory.roomsExplored = explored;
                }

                // Check victory (boss dead and all rooms cleared, or boss dead)
                if (boss.phase == BOSS_DEAD) {
                    gameState.stateTimer += dt;
                    if (gameState.stateTimer > 2.0f) {
                        gameState.current = STATE_VICTORY;
                        gameState.stateTimer = 0;
                    }
                }

                // Check death
                if (player.health <= 0) {
                    gameState.stateTimer += dt;
                    if (gameState.stateTimer > 1.0f) {
                        gameState.current = STATE_GAME_OVER;
                        gameState.stateTimer = 0;
                    }
                }

                break;
            }

            case STATE_PAUSE: {
                if (IsKeyPressed(KEY_ESCAPE)) {
                    gameState.current = STATE_GAMEPLAY;
                }
                // Volume controlled by global +/-/[/] keys above
                break;
            }

            case STATE_GAME_OVER:
            case STATE_VICTORY: {
                if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER)) {
                    gameState.current = STATE_TITLE;
                    gameState.stateTimer = 0;
                    // Switch back to title music
                    StopMusicStream(gameplayMusic);
                    StopMusicStream(bossMusic);
                    bossRoomActive = false;
                    bossMusicFadeTimer = 0.0f;
                    musicFadeTimer = 0.0f;
                    SeekMusicStream(titleMusic, 0.0f);
                    SetMusicVolume(titleMusic, musicVolume);
                    PlayMusicStream(titleMusic);
                }
                break;
            }
        }

        /*
         * ====================================================================
         * RENDER
         * ====================================================================
         */

        BeginDrawing();
        ClearBackground((Color){ 0, 0, 0, 255 }); // Black letterbox

        // Center all content for any window size
        rlPushMatrix();
        rlTranslatef(screenOffsetX, screenOffsetY, 0);

        switch (gameState.current) {
            case STATE_TITLE: {
                TitleScreenDraw();
                break;
            }

            case STATE_GAMEPLAY:
            case STATE_PAUSE: {
                // Dark background for game area
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 12, 10, 18, 255 });

                // Screen shake offset
                Vector2 shakeOffset = ScreenShakeGetOffset();
                rlPushMatrix();
                rlTranslatef(shakeOffset.x, shakeOffset.y, 0);

                // Draw current room
                DungeonRoom *room = DungeonGetCurrentRoom();
                DrawRoomBackground(room, ROOM_X, ROOM_Y);
                DrawRoomWalls(room, ROOM_X, ROOM_Y);
                DrawRoomDoorways(room, ROOM_X, ROOM_Y);
                DrawRoomDecorations(room, ROOM_X, ROOM_Y);
                DrawTorches(room, ROOM_X, ROOM_Y);

                // Draw world objects
                SwordDraw();
                ItemsDraw();
                ProjectilesDraw();
                ParticlesDraw();

                // Draw enemies
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (enemies[i].active) {
                        EnemyDraw(&enemies[i]);
                    }
                }

                // Draw boss
                BossDraw();

                // Draw player
                PlayerDraw(playerSheet);

                rlPopMatrix();

                // HUD (not affected by shake)
                HUDDraw();
                DrawRoomNameFlash();

                // Fade transition overlay
                if (dungeon.fadeAlpha > 0.01f) {
                    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                                 (Color){ 0, 0, 0, (unsigned char)(dungeon.fadeAlpha * 255) });
                }

                // Pause overlay
                if (gameState.current == STATE_PAUSE) {
                    PauseScreenDraw();
                }

                break;
            }

            case STATE_GAME_OVER: {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 12, 10, 18, 255 });
                DungeonRoom *room = DungeonGetCurrentRoom();
                DrawRoomBackground(room, ROOM_X, ROOM_Y);
                DrawRoomWalls(room, ROOM_X, ROOM_Y);
                GameOverScreenDraw();
                break;
            }

            case STATE_VICTORY: {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 12, 10, 18, 255 });
                DungeonRoom *room = DungeonGetCurrentRoom();
                DrawRoomBackground(room, ROOM_X, ROOM_Y);
                DrawRoomWalls(room, ROOM_X, ROOM_Y);
                VictoryScreenDraw();
                break;
            }
        }

        // Scanline overlay for 8-bit CRT feel
        DrawScanlineOverlay();

        rlPopMatrix(); // End centering offset
        EndDrawing();
    }

    // Cleanup
    UnloadMusicStream(titleMusic);
    UnloadMusicStream(gameplayMusic);
    UnloadMusicStream(bossMusic);
    UnloadSound(hitSound);
    UnloadSound(hitSoundSlay);
    UnloadTexture(playerSheet);
    UnloadTexture(enemySheet);

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
