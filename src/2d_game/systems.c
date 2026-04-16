#include "systems.h"
#include "renderer.h"
#include "dungeon.h"
#include "minimap.h"
#include "utils.h"
#include <math.h>
#include <string.h>

/*
 * ============================================================================
 * PARTICLES
 * ============================================================================
 */

void ParticlesUpdate(float dt) {
    int writeIdx = 0;
    for (int i = 0; i < particles.count; i++) {
        Particle *p = &particles.pool[i];
        p->life -= dt;
        if (p->life <= 0.0f) continue;

        p->vel.y += 300.0f * dt;
        p->vel.x *= 0.96f;
        p->vel.y *= 0.96f;
        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;

        particles.pool[writeIdx++] = *p;
    }
    particles.count = writeIdx;
}

void ParticlesDraw(void) {
    for (int i = 0; i < particles.count; i++) {
        Particle *p = &particles.pool[i];
        float lifeRatio = p->life / p->maxLife;
        Color col = Fade(p->color, lifeRatio);
        int sz = (int)(p->size * (0.5f + lifeRatio * 0.5f) * 2.0f);
        if (sz < 2) sz = 2;
        DrawRectangle((int)(p->pos.x - sz / 2), (int)(p->pos.y - sz / 2), sz, sz, col);
    }
}

/*
 * ============================================================================
 * HUD — Hearts, Keys, Coins, Minimap
 * ============================================================================
 */

#define HUD_PADDING 20.0f

// Timer for the "NEED KEY!" bump feedback
static float lockedDoorMsgTimer = 0.0f;

void SystemsShowLockedDoorMsg(void) {
    lockedDoorMsgTimer = 1.2f;
}

// True while any entrance to the boss room is still key-locked.
static bool BossLairStillLocked(void) {
    int bid = dungeon.bossRoomId;
    if (bid < 0 || bid >= dungeon.roomCount) return false;
    DungeonRoom *br = &dungeon.rooms[bid];
    for (int d = 0; d < 4; d++) {
        if (br->connections[d] >= 0 && br->doorLocked[d]) return true;
    }
    return false;
}

// Banner that guides the player to the overarching goal
static void DrawGlobalObjective(void) {
    // Hide while inside the boss room — local objective already handles it
    if (dungeon.currentRoomId == dungeon.bossRoomId) return;
    // Hide once the boss is defeated
    if (boss.phase == BOSS_DEAD) return;

    const char *msg;
    Color col;

    if (!BossLairStillLocked() || player.inventory.keys > 0) {
        msg = "KEY FOUND  -  FIND THE BOSS LAIR";
        col = (Color){ 120, 220, 140, 230 };
    } else {
        msg = "FIND THE KEY TO THE BOSS LAIR";
        float pulse = sinf(gameTime * 2.5f) * 0.25f + 0.75f;
        col = (Color){ 240, 210, 90, (unsigned char)(230 * pulse) };
    }

    int msgW = MeasureText(msg, 16);
    int x = (SCREEN_WIDTH - msgW) / 2;
    int y = (int)(ROOM_Y - 38);
    // Shadow for readability
    DrawText(msg, x + 2, y + 2, 16, (Color){ 0, 0, 0, 160 });
    DrawText(msg, x, y, 16, col);
}

// Draws the "NEED KEY!" flash when the player bumps a locked door
static void DrawLockedDoorMsg(void) {
    if (lockedDoorMsgTimer <= 0.0f) return;
    lockedDoorMsgTimer -= GetFrameTime();

    float t = lockedDoorMsgTimer > 1.0f ? 1.0f : lockedDoorMsgTimer;
    float alpha = t;
    float floatUp = (1.2f - lockedDoorMsgTimer) * 20.0f;

    const char *msg = "NEED  KEY!";
    int fontSize = 36;
    int msgW = MeasureText(msg, fontSize);
    int x = (SCREEN_WIDTH - msgW) / 2;
    int y = (int)(SCREEN_HEIGHT * 0.5f - 120 - floatUp);

    DrawText(msg, x + 3, y + 3, fontSize, Fade((Color){ 0, 0, 0, 200 }, alpha));
    DrawText(msg, x, y, fontSize, Fade((Color){ 255, 210, 80, 255 }, alpha));

    // Sub-hint
    const char *hint = "Find a key in the dungeon to unlock this door";
    int hintW = MeasureText(hint, 14);
    DrawText(hint, (SCREEN_WIDTH - hintW) / 2 + 2, y + 44, 14,
             Fade((Color){ 0, 0, 0, 180 }, alpha));
    DrawText(hint, (SCREEN_WIDTH - hintW) / 2, y + 42, 14,
             Fade((Color){ 240, 220, 180, 255 }, alpha));
}

static void DrawRoomObjective(void) {
    DungeonRoom *room = DungeonGetCurrentRoom();
    const char *msg = NULL;
    Color msgColor = { 160, 160, 170, 140 };

    switch (room->type) {
        case ROOM_COMBAT: {
            if (!room->cleared) {
                int alive = 0;
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (enemies[i].active && enemies[i].roomId == dungeon.currentRoomId) alive++;
                }
                msg = TextFormat("DEFEAT ALL ENEMIES  -  %d remaining", alive);
                msgColor = (Color){ 200, 100, 100, 150 };
            } else {
                msg = "ROOM CLEARED  -  Explore";
                msgColor = (Color){ 100, 180, 100, 120 };
            }
            break;
        }
        case ROOM_SHOP:
            msg = "SHOP  -  Walk into items to buy";
            msgColor = (Color){ 100, 180, 220, 140 };
            break;
        case ROOM_TREASURE:
            msg = "TREASURE  -  Claim your reward";
            msgColor = (Color){ 220, 180, 50, 140 };
            break;
        case ROOM_BOSS:
            if (boss.active && boss.phase != BOSS_DEAD) {
                msg = "BOSS  -  Defeat THE MAD DUCK";
                msgColor = (Color){ 220, 60, 60, 160 };
            } else if (boss.phase == BOSS_DEAD) {
                msg = "BOSS DEFEATED";
                msgColor = (Color){ 100, 200, 100, 140 };
            }
            break;
        case ROOM_START:
            msg = "SANCTUARY  -  Explore the dungeon";
            msgColor = (Color){ 100, 200, 150, 120 };
            break;
        default:
            msg = "EXPLORE";
            break;
    }

    if (msg) {
        int msgW = MeasureText(msg, 13);
        DrawText(msg, (SCREEN_WIDTH - msgW) / 2, (int)(ROOM_Y - 18), 13, msgColor);
    }
}

void HUDDraw(void) {
    // Hearts
    int totalHearts = 4 + player.inventory.heartContainers;
    float heartX = HUD_PADDING;
    float heartY = HUD_PADDING;
    float heartScale = 3.0f;
    float heartSpacing = 7.0f * heartScale + 4.0f;

    for (int i = 0; i < totalHearts; i++) {
        float heartHP = player.health - (float)i * HEART_HP;
        float fill;
        if (heartHP >= HEART_HP) fill = 1.0f;
        else if (heartHP > 0.0f) fill = heartHP / HEART_HP;
        else fill = 0.0f;

        // Low health pulse
        Color heartColor = { 220, 50, 50, 255 };
        if (player.health <= HEART_HP && fill > 0) {
            float pulse = sinf(gameTime * 8.0f) * 0.3f + 0.7f;
            heartColor.r = (unsigned char)(220 * pulse);
        }

        DrawPixelHeart(heartX + i * heartSpacing, heartY, heartScale, fill, heartColor);
    }

    // Keys
    float itemY = heartY + 7 * heartScale + 8;
    if (player.inventory.keys > 0 || 1) {
        DrawPixelKey(HUD_PADDING, itemY, 2.0f, (Color){ 230, 200, 50, 255 });
        DrawText(TextFormat("x%d", player.inventory.keys),
                (int)(HUD_PADDING + 20), (int)(itemY + 2), 16,
                (Color){ 230, 200, 50, 255 });
    }

    // Coins
    float coinX = HUD_PADDING + 60;
    DrawPixelCoin(coinX, itemY, 1.5f, (Color){ 230, 200, 50, 255 });
    DrawText(TextFormat("x%d", player.inventory.coins),
            (int)(coinX + 16), (int)(itemY + 2), 16,
            (Color){ 230, 200, 50, 255 });

    // Shield timer
    if (player.shieldTimer > 0) {
        DrawText(TextFormat("SHIELD %.0f", player.shieldTimer),
                (int)(HUD_PADDING), (int)(itemY + 22), 14,
                (Color){ 80, 160, 255, 200 });
    }

    // Immunity timer
    if (player.immunityTimer > 0) {
        float imY = player.shieldTimer > 0 ? itemY + 38 : itemY + 22;
        float cycle = gameTime * 6.0f;
        Color imColor = {
            (unsigned char)(sinf(cycle) * 80 + 175),
            (unsigned char)(sinf(cycle + 2.094f) * 80 + 175),
            (unsigned char)(sinf(cycle + 4.189f) * 80 + 175), 220
        };
        DrawText(TextFormat("STAR %.0f", player.immunityTimer),
                (int)(HUD_PADDING), (int)(imY), 14, imColor);
    }

    // Global and room objectives
    DrawGlobalObjective();
    DrawRoomObjective();

    // "NEED KEY!" flash when bumping a locked door
    DrawLockedDoorMsg();

    // ===== Bottom status bar panel (organized, padded, good contrast) =====
    {
        float panelW = 300.0f;
        float panelH = 52.0f;
        float panelX = SCREEN_WIDTH - panelW - 12.0f;
        float panelY = SCREEN_HEIGHT - panelH - 8.0f;

        // Dark background panel with border for contrast
        DrawRectangle((int)panelX, (int)panelY, (int)panelW, (int)panelH,
                     (Color){ 12, 10, 18, 200 });
        DrawRectangleLinesEx((Rectangle){ panelX, panelY, panelW, panelH },
                            1, (Color){ 60, 56, 70, 180 });

        float innerPad = 10.0f;
        float cx = panelX + innerPad;
        float cy = panelY + 8.0f;

        // Row 1: Pause icon + "ESC Pause" | Music/SFX hint
        int p = 2;
        Color iconCol = { 200, 200, 210, 200 };
        Color textCol = { 170, 170, 180, 180 };
        Color labelCol = { 130, 130, 140, 150 };

        // Pause icon (two bars)
        DrawRectangle((int)cx, (int)(cy + 1), 2*p, 3*p, iconCol);
        DrawRectangle((int)(cx + 3*p), (int)(cy + 1), 2*p, 3*p, iconCol);
        DrawText("ESC Pause", (int)(cx + 6*p + 4), (int)(cy), 12, textCol);

        // Volume controls hint (right side of row 1)
        float volHintX = panelX + panelW - innerPad - 120;
        DrawText("+/- Music", (int)volHintX, (int)(cy), 11, labelCol);

        // Row 2: SFX hint + FPS
        float row2Y = cy + 20.0f;
        DrawText("[/] SFX", (int)volHintX, (int)(row2Y), 11, labelCol);

        // Music volume indicator
        {
            float barX = cx;
            float barW = 80.0f;
            float barH = 6.0f;
            DrawText("Vol", (int)barX, (int)(row2Y), 11, labelCol);
            float bx2 = barX + 24;
            DrawRectangle((int)bx2, (int)(row2Y + 2), (int)barW, (int)barH,
                         (Color){ 30, 28, 40, 200 });
            DrawRectangle((int)bx2, (int)(row2Y + 2), (int)(barW * musicVolume), (int)barH,
                         (Color){ 100, 180, 220, 200 });
        }

        // FPS (right-aligned in panel)
        int fps = GetFPS();
        Color fpsColor = fps >= 120 ? (Color){ 100, 255, 100, 255 }
                       : fps >= 60  ? (Color){ 255, 220, 100, 255 }
                       : (Color){ 255, 100, 100, 255 };
        const char *fpsText = TextFormat("%d FPS", fps);
        int fpsW = MeasureText(fpsText, 12);
        DrawText(fpsText, (int)(panelX + panelW - innerPad - fpsW), (int)(row2Y), 12,
                Fade(fpsColor, 0.6f));
    }

    // Minimap
    MinimapDraw();
}

/*
 * ============================================================================
 * ROOM NAME FLASH
 * ============================================================================
 */

static float roomNameTimer = 0.0f;
static int lastRoomId = -1;
static bool roomNameShown[MAX_DUNGEON_ROOMS] = {0};

void ResetRoomNames(void) {
    roomNameTimer = 0.0f;
    lastRoomId = -1;
    memset(roomNameShown, 0, sizeof(roomNameShown));
}

void DrawRoomNameFlash(void) {
    if (dungeon.currentRoomId != lastRoomId) {
        lastRoomId = dungeon.currentRoomId;
        // Only show name on first visit
        if (!roomNameShown[dungeon.currentRoomId]) {
            roomNameShown[dungeon.currentRoomId] = true;
            roomNameTimer = 2.0f;
        }
    }

    if (roomNameTimer <= 0.0f) return;
    roomNameTimer -= GetFrameTime();

    float alpha = roomNameTimer > 1.0f ? 1.0f : roomNameTimer;
    DungeonRoom *room = DungeonGetCurrentRoom();

    const char *name = "";
    Color nameColor = { 255, 255, 255, 255 };
    switch (room->type) {
        case ROOM_START:    name = "SANCTUARY"; nameColor = (Color){ 100, 200, 150, 255 }; break;
        case ROOM_COMBAT:   name = "COMBAT"; nameColor = (Color){ 200, 80, 80, 255 }; break;
        case ROOM_TREASURE: name = "TREASURE"; nameColor = (Color){ 220, 180, 50, 255 }; break;
        case ROOM_SHOP:     name = "SHOP"; nameColor = (Color){ 100, 180, 220, 255 }; break;
        case ROOM_BOSS:     name = "BOSS LAIR"; nameColor = (Color){ 220, 50, 50, 255 }; break;
        default: break;
    }

    int fontSize = 36;
    int textW = MeasureText(name, fontSize);
    DrawText(name, (SCREEN_WIDTH - textW) / 2, SCREEN_HEIGHT / 2 - 80,
            fontSize, Fade(nameColor, alpha));
}

/*
 * ============================================================================
 * GAME STATE SCREENS
 * ============================================================================
 */

void TitleScreenDraw(void) {
    // Background
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 15, 12, 20, 255 });

    // Animated background tiles (extra row/col to cover scroll gap)
    int scrollOff = (int)fmodf(gameTime * 10.0f, 20.0f);
    for (int x = 0; x < SCREEN_WIDTH / 20 + 2; x++) {
        for (int y = 0; y < SCREEN_HEIGHT / 20 + 2; y++) {
            bool light = ((x + y) % 2 == 0);
            Color c = light ? (Color){ 18, 15, 25, 255 } : (Color){ 15, 12, 20, 255 };
            DrawRectangle(x * 20 - scrollOff, y * 20 - scrollOff, 20, 20, c);
        }
    }

    // Title
    const char *title = "DUCK QUEST";
    int titleSize = 72;
    int titleW = MeasureText(title, titleSize);
    float titleY = 120;

    // Title shadow
    DrawText(title, (SCREEN_WIDTH - titleW) / 2 + 3, (int)titleY + 3, titleSize,
            (Color){ 0, 0, 0, 150 });
    // Title main
    float titlePulse = sinf(gameTime * 2.0f) * 5.0f;
    DrawText(title, (SCREEN_WIDTH - titleW) / 2, (int)(titleY + titlePulse), titleSize,
            (Color){ 255, 220, 100, 255 });

    // Subtitle
    const char *sub = "Into the Dungeon";
    int subW = MeasureText(sub, 24);
    DrawText(sub, (SCREEN_WIDTH - subW) / 2, (int)(titleY + 78), 24,
            (Color){ 180, 170, 160, 200 });

    // Early Access disclaimer
    const char *earlyAccess = "EARLY ACCESS BUILD  -  Pre-Alpha v0.1";
    int eaW = MeasureText(earlyAccess, 16);
    DrawText(earlyAccess, (SCREEN_WIDTH - eaW) / 2, (int)(titleY + 110), 16,
            (Color){ 200, 150, 50, 180 });

    // Start prompt
    float promptAlpha = sinf(gameTime * 3.0f) * 0.3f + 0.7f;
    const char *prompt = "Press ENTER to start";
    int promptW = MeasureText(prompt, 22);
    DrawText(prompt, (SCREEN_WIDTH - promptW) / 2, 310, 22,
            Fade((Color){ 200, 200, 200, 255 }, promptAlpha));

    // Controls hint
    const char *controls = "WASD - Move   |   SPACE - Attack   |   ESC - Pause";
    int ctrlW = MeasureText(controls, 16);
    DrawText(controls, (SCREEN_WIDTH - ctrlW) / 2, 345, 16,
            (Color){ 120, 120, 130, 150 });

    // Developer notes / changelog panel
    float panelX = SCREEN_WIDTH * 0.5f - 250;
    float panelY = 390;
    float panelW = 500;
    float panelH = 280;

    DrawRectangle((int)panelX, (int)panelY, (int)panelW, (int)panelH,
                 (Color){ 10, 8, 16, 200 });
    DrawRectangleLinesEx((Rectangle){ panelX, panelY, panelW, panelH },
                        1, (Color){ 60, 55, 70, 150 });

    DrawText("DEVELOPER NOTES", (int)(panelX + 12), (int)(panelY + 10), 16,
            (Color){ 200, 180, 100, 220 });

    DrawText("This build is an early prototype. Expect rough edges,",
            (int)(panelX + 12), (int)(panelY + 32), 13, (Color){ 160, 155, 150, 170 });
    DrawText("placeholder assets, and unbalanced gameplay. Your feedback",
            (int)(panelX + 12), (int)(panelY + 48), 13, (Color){ 160, 155, 150, 170 });
    DrawText("helps shape the final experience. Thank you for playing!",
            (int)(panelX + 12), (int)(panelY + 64), 13, (Color){ 160, 155, 150, 170 });

    float clY = panelY + 90;
    DrawText("PATCH NOTES  -  v0.1.0", (int)(panelX + 12), (int)clY, 14,
            (Color){ 180, 160, 80, 200 });
    clY += 22;

    const char *notes[] = {
        "> Procedural dungeon generation (10 rooms per run)",
        "> 5 room types: Sanctuary, Combat, Treasure, Shop, Boss",
        "> 4 enemy types: Slime, Bat, Skeleton, Turret",
        "> Boss fight: The Mad Duck (2 phases, 3 attack patterns)",
        "> Item system: hearts, upgrades, keys, coins, bombs, shields",
        "> Shop with purchasable items",
        "> Minimap with fog of war",
        "> Screen shake, hit freeze, particle effects",
        "> Heart-based HUD with key/coin counters",
    };
    int noteCount = sizeof(notes) / sizeof(notes[0]);
    for (int i = 0; i < noteCount; i++) {
        DrawText(notes[i], (int)(panelX + 16), (int)clY, 12, (Color){ 140, 140, 145, 160 });
        clY += 16;
    }
}

void PauseScreenDraw(void) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 0, 0, 0, 150 });

    const char *title = "PAUSED";
    int titleW = MeasureText(title, 48);
    DrawText(title, (SCREEN_WIDTH - titleW) / 2, 160, 48, (Color){ 220, 220, 220, 255 });

    const char *hint = "Press ESC to resume";
    int hintW = MeasureText(hint, 20);
    float alpha = sinf(gameTime * 3.0f) * 0.3f + 0.7f;
    DrawText(hint, (SCREEN_WIDTH - hintW) / 2, 220, 20,
            Fade((Color){ 200, 200, 200, 255 }, alpha));

    // Audio settings
    float panelX = SCREEN_WIDTH * 0.5f - 160;
    float panelY = 270;
    float panelW = 320;
    float panelH = 120;

    DrawRectangle((int)panelX, (int)panelY, (int)panelW, (int)panelH,
                 (Color){ 10, 8, 16, 200 });
    DrawRectangleLinesEx((Rectangle){ panelX, panelY, panelW, panelH },
                        1, (Color){ 60, 55, 70, 150 });

    DrawText("AUDIO SETTINGS", (int)(panelX + 12), (int)(panelY + 10), 16,
            (Color){ 200, 180, 100, 220 });

    // Music volume
    float barX = panelX + 120;
    float barW = 160;
    float barH = 14;
    float musicY = panelY + 38;
    DrawText("Music", (int)(panelX + 16), (int)(musicY), 16, (Color){ 180, 180, 180, 220 });
    DrawRectangle((int)barX, (int)musicY, (int)barW, (int)barH, (Color){ 30, 28, 40, 200 });
    DrawRectangle((int)barX, (int)musicY, (int)(barW * musicVolume), (int)barH,
                 (Color){ 100, 180, 220, 220 });
    DrawText(TextFormat("%d%%", (int)(musicVolume * 100)),
            (int)(barX + barW + 8), (int)musicY, 14, (Color){ 160, 160, 160, 200 });

    // SFX volume
    float sfxY = panelY + 62;
    DrawText("SFX", (int)(panelX + 16), (int)(sfxY), 16, (Color){ 180, 180, 180, 220 });
    DrawRectangle((int)barX, (int)sfxY, (int)barW, (int)barH, (Color){ 30, 28, 40, 200 });
    DrawRectangle((int)barX, (int)sfxY, (int)(barW * sfxVolume), (int)barH,
                 (Color){ 100, 180, 220, 220 });
    DrawText(TextFormat("%d%%", (int)(sfxVolume * 100)),
            (int)(barX + barW + 8), (int)sfxY, 14, (Color){ 160, 160, 160, 200 });

    // Controls hint
    DrawText("+/-: Music volume   [/]: SFX volume",
            (int)(panelX + 16), (int)(panelY + 90), 12, (Color){ 120, 120, 130, 150 });

    // Show enlarged minimap
    MinimapDraw();
}

void GameOverScreenDraw(void) {
    // Red-tinted overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 80, 10, 10, 180 });

    const char *title = "YOU DIED";
    int titleW = MeasureText(title, 64);
    DrawText(title, (SCREEN_WIDTH - titleW) / 2 + 3, 203, 64, (Color){ 0, 0, 0, 150 });
    DrawText(title, (SCREEN_WIDTH - titleW) / 2, 200, 64, (Color){ 200, 50, 50, 255 });

    // Stats
    float statsY = 320;
    const char *stats[] = {
        TextFormat("Rooms Explored: %d", player.inventory.roomsExplored),
        TextFormat("Enemies Slain: %d", player.inventory.enemiesKilled),
        TextFormat("Coins Collected: %d", player.inventory.coins),
    };
    for (int i = 0; i < 3; i++) {
        int sw = MeasureText(stats[i], 20);
        DrawText(stats[i], (SCREEN_WIDTH - sw) / 2, (int)(statsY + i * 30), 20,
                (Color){ 200, 180, 180, 220 });
    }

    float alpha = sinf(gameTime * 3.0f) * 0.3f + 0.7f;
    const char *hint = "Press R to restart";
    int hintW = MeasureText(hint, 22);
    DrawText(hint, (SCREEN_WIDTH - hintW) / 2, 450, 22,
            Fade((Color){ 200, 200, 200, 255 }, alpha));
}

void VictoryScreenDraw(void) {
    // Gold overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 40, 35, 10, 150 });

    const char *title = "DUNGEON CLEARED!";
    int titleW = MeasureText(title, 56);
    float pulse = sinf(gameTime * 2.0f) * 5.0f;
    DrawText(title, (SCREEN_WIDTH - titleW) / 2 + 3, (int)(183 + pulse), 56, (Color){ 0, 0, 0, 150 });
    DrawText(title, (SCREEN_WIDTH - titleW) / 2, (int)(180 + pulse), 56,
            (Color){ 255, 220, 100, 255 });

    // Stats
    float statsY = 300;
    const char *stats[] = {
        TextFormat("Rooms Explored: %d / %d", player.inventory.roomsExplored, dungeon.roomCount),
        TextFormat("Enemies Slain: %d", player.inventory.enemiesKilled),
        TextFormat("Coins Collected: %d", player.inventory.coins),
    };
    for (int i = 0; i < 3; i++) {
        int sw = MeasureText(stats[i], 22);
        DrawText(stats[i], (SCREEN_WIDTH - sw) / 2, (int)(statsY + i * 35), 22,
                (Color){ 230, 220, 180, 240 });
    }

    float alpha = sinf(gameTime * 3.0f) * 0.3f + 0.7f;
    const char *hint = "Press R to play again";
    int hintW = MeasureText(hint, 22);
    DrawText(hint, (SCREEN_WIDTH - hintW) / 2, 480, 22,
            Fade((Color){ 200, 200, 200, 255 }, alpha));
}
