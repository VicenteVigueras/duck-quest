#include "systems.h"
#include "renderer.h"
#include "dungeon.h"
#include "minimap.h"
#include "palette.h"
#include "utils.h"
#include <math.h>
#include <string.h>

/*
 * ============================================================================
 * UI primitives — kept local to systems.c so menu styling is uniform.
 *
 * A menu panel is: a solid dark fill, a solid dusk-colored border, and a
 * single-pixel ink outline. No alpha blending. No anti-aliased corners.
 * Every menu in the game uses the same one. That consistency is the whole
 * point.
 * ============================================================================
 */
static void DrawPanel(int x, int y, int w, int h) {
    // Outer ink border (the hard edge)
    DrawRectangle(x - 1, y - 1, w + 2, h + 2, PAL_INK);
    // Panel fill
    DrawRectangle(x, y, w, h, PAL_NIGHT);
    // Inner highlight band (top) — sells depth in 8-bit
    DrawRectangle(x, y, w, 1, PAL_DUSK);
    DrawRectangle(x, y + 1, 1, h - 1, PAL_DUSK);
    // Inner shadow band (bottom/right)
    DrawRectangle(x, y + h - 1, w, 1, PAL_INK);
    DrawRectangle(x + w - 1, y, 1, h, PAL_INK);
}

// Horizontal 1-pixel rule with two palette shades, NES-style.
static void DrawHRule(int x, int y, int w) {
    DrawRectangle(x, y, w, 1, PAL_DUSK);
    DrawRectangle(x, y + 1, w, 1, PAL_INK);
}

// Centered pixel-text helper — most menus center everything horizontally.
static void DrawCenteredText(const char *text, int y, int size, Color color) {
    int w = MeasureText(text, size);
    DrawPixelText(text, (SCREEN_WIDTH - w) / 2, y, size, color);
}

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
    // Quantize particle size to discrete 2-pixel steps and use alpha in
    // coarse quartiles instead of a smooth fade. Both changes make particles
    // read as pixel art rather than dots from a modern engine.
    for (int i = 0; i < particles.count; i++) {
        Particle *p = &particles.pool[i];
        float lifeRatio = p->life / p->maxLife;
        if (lifeRatio < 0.0f) lifeRatio = 0.0f;

        // 4-stage alpha quantization
        unsigned char a;
        if      (lifeRatio > 0.75f) a = p->color.a;
        else if (lifeRatio > 0.50f) a = (unsigned char)(p->color.a * 0.8f);
        else if (lifeRatio > 0.25f) a = (unsigned char)(p->color.a * 0.55f);
        else                        a = (unsigned char)(p->color.a * 0.3f);
        Color col = { p->color.r, p->color.g, p->color.b, a };

        // Quantize size to multiples of 2
        int sz = (int)(p->size * (0.5f + lifeRatio * 0.5f) * 2.0f);
        sz = (sz / 2) * 2;
        if (sz < 2) sz = 2;
        DrawRectangle((int)(p->pos.x) - sz / 2, (int)(p->pos.y) - sz / 2, sz, sz, col);
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
        col = PAL_MOSS;
    } else {
        msg = "FIND THE KEY TO THE BOSS LAIR";
        // Integer-stepped pulse (no sub-pixel alpha wobble) for 8-bit feel
        int blink = ((int)(gameTime * 2.0f)) & 1;
        col = blink ? PAL_GOLD : PAL_AMBER;
    }

    DrawCenteredText(msg, (int)(ROOM_Y - 38), 16, col);
}

// Draws the "NEED KEY!" flash when the player bumps a locked door
static void DrawLockedDoorMsg(void) {
    if (lockedDoorMsgTimer <= 0.0f) return;
    lockedDoorMsgTimer -= GetFrameTime();

    // Integer-stepped float-up (pixel-snapped, no sub-pixel crawling)
    int floatUp = (int)((1.2f - lockedDoorMsgTimer) * 20.0f);
    unsigned char alpha = (unsigned char)(255 * (lockedDoorMsgTimer > 1.0f ? 1.0f : lockedDoorMsgTimer));

    int y = (int)(SCREEN_HEIGHT * 0.5f - 120 - floatUp);
    DrawCenteredText("NEED  KEY!", y, 36, PAL_ALPHA(PAL_GOLD, alpha));
    DrawCenteredText("Find a key in the dungeon to unlock this door",
                     y + 44, 14, PAL_ALPHA(PAL_BONE, alpha));
}

static void DrawRoomObjective(void) {
    DungeonRoom *room = DungeonGetCurrentRoom();
    const char *msg = NULL;
    Color msgColor = PAL_ASH;

    switch (room->type) {
        case ROOM_COMBAT: {
            if (!room->cleared) {
                int alive = 0;
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (enemies[i].active && enemies[i].roomId == dungeon.currentRoomId) alive++;
                }
                msg = TextFormat("DEFEAT ALL ENEMIES  -  %d remaining", alive);
                msgColor = PAL_EMBER;
            } else {
                msg = "ROOM CLEARED  -  Explore";
                msgColor = PAL_MOSS;
            }
            break;
        }
        case ROOM_SHOP:
            msg = "SHOP  -  Walk into items to buy";
            msgColor = PAL_SKY;
            break;
        case ROOM_TREASURE:
            msg = "TREASURE  -  Claim your reward";
            msgColor = PAL_GOLD;
            break;
        case ROOM_BOSS:
            if (boss.active && boss.phase != BOSS_DEAD) {
                msg = "BOSS  -  Defeat THE MAD DUCK";
                msgColor = PAL_BLOOD;
            } else if (boss.phase == BOSS_DEAD) {
                msg = "BOSS DEFEATED";
                msgColor = PAL_MOSS;
            }
            break;
        case ROOM_START:
            msg = "SANCTUARY  -  Explore the dungeon";
            msgColor = PAL_MOSS;
            break;
        default:
            msg = "EXPLORE";
            break;
    }

    if (msg) {
        DrawCenteredText(msg, (int)(ROOM_Y - 18), 13, msgColor);
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
    DrawPixelKey(HUD_PADDING, itemY, 2.0f, PAL_GOLD);
    DrawPixelText(TextFormat("x%d", player.inventory.keys),
                  HUD_PADDING + 20, itemY + 2, 16, PAL_GOLD);

    // Coins
    float coinX = HUD_PADDING + 60;
    DrawPixelCoin(coinX, itemY, 1.5f, PAL_GOLD);
    DrawPixelText(TextFormat("x%d", player.inventory.coins),
                  coinX + 16, itemY + 2, 16, PAL_GOLD);

    // Shield timer
    if (player.shieldTimer > 0) {
        DrawPixelText(TextFormat("SHIELD %.0f", player.shieldTimer),
                      HUD_PADDING, itemY + 22, 14, PAL_SKY);
    }

    // Immunity timer — palette-cycle through 3 harmonic colors, integer-stepped
    if (player.immunityTimer > 0) {
        float imY = player.shieldTimer > 0 ? itemY + 38 : itemY + 22;
        int cycle = ((int)(gameTime * 4.0f)) % 3;
        Color imColor = cycle == 0 ? PAL_GOLD
                       : cycle == 1 ? PAL_SKY
                       : PAL_VIOLET;
        DrawPixelText(TextFormat("STAR %.0f", player.immunityTimer),
                      HUD_PADDING, imY, 14, imColor);
    }

    // Global and room objectives
    DrawGlobalObjective();
    DrawRoomObjective();

    // "NEED KEY!" flash when bumping a locked door
    DrawLockedDoorMsg();

    // ===== Bottom status bar panel (solid colors, pixel borders) =====
    {
        int panelW = 300;
        int panelH = 52;
        int panelX = SCREEN_WIDTH - panelW - 12;
        int panelY = SCREEN_HEIGHT - panelH - 8;

        DrawPanel(panelX, panelY, panelW, panelH);

        int innerPad = 10;
        int cx = panelX + innerPad;
        int cy = panelY + 8;

        // Row 1: Pause icon + "ESC Pause" label
        int p = 2;
        // Pause icon (two solid bars, full opacity)
        DrawRectangle(cx,         cy + 1, 2*p, 3*p, PAL_BONE);
        DrawRectangle(cx + 3*p,   cy + 1, 2*p, 3*p, PAL_BONE);
        DrawPixelText("ESC Pause", cx + 6*p + 4, cy, 12, PAL_ASH);

        // Volume hint (right of row 1)
        int volHintX = panelX + panelW - innerPad - 120;
        DrawPixelText("+/- Music", volHintX, cy, 11, PAL_STONE);

        // Row 2: SFX hint + music bar + FPS
        int row2Y = cy + 20;
        DrawPixelText("[/] SFX", volHintX, row2Y, 11, PAL_STONE);

        // Music volume indicator — pipped bar (NES-style), 10 cells
        {
            int barX = cx;
            DrawPixelText("Vol", barX, row2Y, 11, PAL_STONE);
            int pipX = barX + 24;
            int pipY = row2Y + 3;
            int pipSize = 6;
            int pipGap = 2;
            int lit = (int)(musicVolume * 10.0f + 0.5f);
            for (int i = 0; i < 10; i++) {
                int x = pipX + i * (pipSize + pipGap);
                DrawRectangle(x, pipY, pipSize, pipSize,
                              i < lit ? PAL_SKY : PAL_DUSK);
            }
        }

        // FPS (right-aligned, palette-bucketed)
        int fps = GetFPS();
        Color fpsColor = fps >= 120 ? PAL_MOSS
                       : fps >= 60  ? PAL_GOLD
                                    : PAL_BLOOD;
        const char *fpsText = TextFormat("%d FPS", fps);
        int fpsW = MeasureText(fpsText, 12);
        DrawPixelText(fpsText, panelX + panelW - innerPad - fpsW, row2Y, 12, fpsColor);
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

    unsigned char alpha = (unsigned char)(255 * (roomNameTimer > 1.0f ? 1.0f : roomNameTimer));
    DungeonRoom *room = DungeonGetCurrentRoom();

    const char *name = "";
    Color nameColor = PAL_BONE;
    switch (room->type) {
        case ROOM_START:    name = "SANCTUARY"; nameColor = PAL_MOSS;  break;
        case ROOM_COMBAT:   name = "COMBAT";    nameColor = PAL_EMBER; break;
        case ROOM_TREASURE: name = "TREASURE";  nameColor = PAL_GOLD;  break;
        case ROOM_SHOP:     name = "SHOP";      nameColor = PAL_SKY;   break;
        case ROOM_BOSS:     name = "BOSS LAIR"; nameColor = PAL_BLOOD; break;
        default: break;
    }

    DrawCenteredText(name, SCREEN_HEIGHT / 2 - 80, 36, PAL_ALPHA(nameColor, alpha));
}

/*
 * ============================================================================
 * GAME STATE SCREENS
 * ============================================================================
 */

void TitleScreenDraw(void) {
    // Solid background from palette
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, PAL_INK);

    // Parallax scrolling checkerboard — two palette shades only, pixel-snapped
    int scrollOff = ((int)(gameTime * 10.0f)) % 20;
    for (int x = 0; x < SCREEN_WIDTH / 20 + 2; x++) {
        for (int y = 0; y < SCREEN_HEIGHT / 20 + 2; y++) {
            bool light = ((x + y) & 1) == 0;
            DrawRectangle(x * 20 - scrollOff, y * 20 - scrollOff, 20, 20,
                          light ? PAL_NIGHT : PAL_INK);
        }
    }

    // Title — integer-stepped bob (no float wobble), chunky outline
    int titleSize = 72;
    int titleBob = (((int)(gameTime * 2.0f)) & 1) ? 0 : 2;
    int titleY = 120 + titleBob;

    // Extra-thick 3px title outline (drawn as a stack of DrawPixelText calls)
    int titleW = MeasureText("DUCK QUEST", titleSize);
    int tx = (SCREEN_WIDTH - titleW) / 2;
    for (int ox = -2; ox <= 2; ox++) for (int oy = -2; oy <= 2; oy++) {
        if (ox == 0 && oy == 0) continue;
        DrawText("DUCK QUEST", tx + ox, titleY + oy, titleSize, PAL_INK);
    }
    DrawText("DUCK QUEST", tx, titleY, titleSize, PAL_GOLD);

    // Subtitle
    DrawCenteredText("Into the Dungeon", titleY + 78, 24, PAL_ASH);

    // Build stamp
    DrawCenteredText("EARLY ACCESS BUILD  -  Pre-Alpha v0.1",
                     titleY + 110, 16, PAL_AMBER);

    // Start prompt — integer blink, not smooth alpha
    int blink = ((int)(gameTime * 2.0f)) & 1;
    DrawCenteredText("Press ENTER to start", 310, 22,
                     blink ? PAL_SNOW : PAL_STONE);

    // Controls hint
    DrawCenteredText("WASD - Move   |   SPACE - Attack   |   ESC - Pause",
                     345, 16, PAL_STONE);

    // Developer notes panel
    int panelX = SCREEN_WIDTH / 2 - 250;
    int panelY = 390;
    int panelW = 500;
    int panelH = 280;

    DrawPanel(panelX, panelY, panelW, panelH);

    DrawPixelText("DEVELOPER NOTES", panelX + 12, panelY + 10, 16, PAL_GOLD);
    DrawHRule(panelX + 12, panelY + 30, panelW - 24);

    DrawPixelText("This build is an early prototype. Expect rough edges,",
                  panelX + 12, panelY + 38, 13, PAL_ASH);
    DrawPixelText("placeholder assets, and unbalanced gameplay. Your feedback",
                  panelX + 12, panelY + 54, 13, PAL_ASH);
    DrawPixelText("helps shape the final experience. Thank you for playing!",
                  panelX + 12, panelY + 70, 13, PAL_ASH);

    int clY = panelY + 96;
    DrawPixelText("PATCH NOTES  -  v0.1.0", panelX + 12, clY, 14, PAL_AMBER);
    DrawHRule(panelX + 12, clY + 16, panelW - 24);
    clY += 24;

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
        DrawPixelText(notes[i], panelX + 16, clY, 12, PAL_STONE);
        clY += 16;
    }
}

/*
 * Draws a pipped audio bar (10 cells), same visual vocabulary as the HUD bar.
 * Pure 8-bit look — no gradient fill, no alpha, just on/off cells.
 */
static void DrawPippedBar(int x, int y, float fill01) {
    int pipSize = 12;
    int pipGap = 2;
    int lit = (int)(fill01 * 10.0f + 0.5f);
    for (int i = 0; i < 10; i++) {
        int px = x + i * (pipSize + pipGap);
        DrawRectangle(px, y, pipSize, pipSize, i < lit ? PAL_SKY : PAL_DUSK);
        // tiny inner highlight on lit pips for pop
        if (i < lit) {
            DrawRectangle(px, y, pipSize, 2, PAL_BONE);
        }
    }
}

void PauseScreenDraw(void) {
    // Dithered dim overlay: checkerboard of ink pixels, no alpha blending.
    for (int y = 0; y < SCREEN_HEIGHT; y += 2) {
        for (int x = ((y / 2) & 1) ? 0 : 1; x < SCREEN_WIDTH; x += 2) {
            DrawRectangle(x, y, 1, 1, PAL_INK);
        }
    }

    // PAUSED banner
    for (int ox = -2; ox <= 2; ox++) for (int oy = -2; oy <= 2; oy++) {
        if (ox == 0 && oy == 0) continue;
        DrawText("PAUSED", (SCREEN_WIDTH - MeasureText("PAUSED", 48)) / 2 + ox,
                 160 + oy, 48, PAL_INK);
    }
    DrawText("PAUSED", (SCREEN_WIDTH - MeasureText("PAUSED", 48)) / 2,
             160, 48, PAL_BONE);

    int blink = ((int)(gameTime * 2.0f)) & 1;
    DrawCenteredText("Press ESC to resume", 220, 20, blink ? PAL_SNOW : PAL_STONE);

    // Audio settings panel
    int panelX = SCREEN_WIDTH / 2 - 180;
    int panelY = 270;
    int panelW = 360;
    int panelH = 150;
    DrawPanel(panelX, panelY, panelW, panelH);

    DrawPixelText("AUDIO SETTINGS", panelX + 12, panelY + 10, 16, PAL_GOLD);
    DrawHRule(panelX + 12, panelY + 30, panelW - 24);

    int musicY = panelY + 42;
    DrawPixelText("Music", panelX + 16, musicY + 2, 14, PAL_BONE);
    DrawPippedBar(panelX + 80, musicY, musicVolume);
    DrawPixelText(TextFormat("%d%%", (int)(musicVolume * 100)),
                  panelX + 80 + 140 + 8, musicY + 2, 14, PAL_ASH);

    int sfxY = musicY + 32;
    DrawPixelText("SFX", panelX + 16, sfxY + 2, 14, PAL_BONE);
    DrawPippedBar(panelX + 80, sfxY, sfxVolume);
    DrawPixelText(TextFormat("%d%%", (int)(sfxVolume * 100)),
                  panelX + 80 + 140 + 8, sfxY + 2, 14, PAL_ASH);

    DrawPixelText("+/-: Music   [/]: SFX",
                  panelX + 16, panelY + panelH - 22, 12, PAL_STONE);

    // Show enlarged minimap
    MinimapDraw();
}

/*
 * Shared helper: a stats panel centered on screen. Used by both the
 * GameOver and Victory screens so their framing is visually identical.
 */
static void DrawStatsPanel(int topY, Color accent) {
    int panelX = SCREEN_WIDTH / 2 - 220;
    int panelY = topY;
    int panelW = 440;
    int panelH = 140;
    DrawPanel(panelX, panelY, panelW, panelH);

    DrawPixelText("RUN SUMMARY", panelX + 14, panelY + 10, 16, accent);
    DrawHRule(panelX + 14, panelY + 30, panelW - 28);

    int labelX = panelX + 20;
    int valueX = panelX + panelW - 20;
    int rowY = panelY + 42;
    int rowH = 24;

    const char *labels[] = { "Rooms Explored", "Enemies Slain", "Coins Collected" };
    int values[] = {
        player.inventory.roomsExplored,
        player.inventory.enemiesKilled,
        player.inventory.coins,
    };
    for (int i = 0; i < 3; i++) {
        DrawPixelText(labels[i], labelX, rowY + i * rowH, 16, PAL_ASH);
        const char *valStr = (i == 0)
            ? TextFormat("%d / %d", values[i], dungeon.roomCount)
            : TextFormat("%d", values[i]);
        int vw = MeasureText(valStr, 16);
        DrawPixelText(valStr, valueX - vw, rowY + i * rowH, 16, PAL_BONE);
    }
}

void GameOverScreenDraw(void) {
    // Dithered wine overlay — every other row, solid palette color, no alpha.
    for (int y = 0; y < SCREEN_HEIGHT; y += 2) {
        DrawRectangle(0, y, SCREEN_WIDTH, 1, PAL_WINE);
    }

    // Extra-chunky outlined title
    int titleW = MeasureText("YOU DIED", 64);
    int tx = (SCREEN_WIDTH - titleW) / 2;
    int ty = 180;
    for (int ox = -2; ox <= 2; ox++) for (int oy = -2; oy <= 2; oy++) {
        if (ox == 0 && oy == 0) continue;
        DrawText("YOU DIED", tx + ox, ty + oy, 64, PAL_INK);
    }
    DrawText("YOU DIED", tx, ty, 64, PAL_BLOOD);

    DrawStatsPanel(290, PAL_BLOOD);

    int blink = ((int)(gameTime * 2.0f)) & 1;
    DrawCenteredText("Press R to restart", 480, 22, blink ? PAL_SNOW : PAL_STONE);
}

void VictoryScreenDraw(void) {
    // Dithered gold overlay — warm but restrained
    for (int y = 0; y < SCREEN_HEIGHT; y += 2) {
        DrawRectangle(0, y, SCREEN_WIDTH, 1, PAL_AMBER);
    }
    for (int y = 1; y < SCREEN_HEIGHT; y += 2) {
        DrawRectangle(0, y, SCREEN_WIDTH, 1, PAL_INK);
    }

    // Integer bob, chunky outline
    int titleBob = (((int)(gameTime * 2.0f)) & 1) ? 0 : 2;
    int titleW = MeasureText("DUNGEON CLEARED!", 56);
    int tx = (SCREEN_WIDTH - titleW) / 2;
    int ty = 170 + titleBob;
    for (int ox = -2; ox <= 2; ox++) for (int oy = -2; oy <= 2; oy++) {
        if (ox == 0 && oy == 0) continue;
        DrawText("DUNGEON CLEARED!", tx + ox, ty + oy, 56, PAL_INK);
    }
    DrawText("DUNGEON CLEARED!", tx, ty, 56, PAL_GOLD);

    DrawStatsPanel(280, PAL_GOLD);

    int blink = ((int)(gameTime * 2.0f)) & 1;
    DrawCenteredText("Press R to play again", 480, 22, blink ? PAL_SNOW : PAL_STONE);
}
