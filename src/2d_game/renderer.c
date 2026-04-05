#include "renderer.h"
#include "utils.h"
#include <math.h>

/*
 * ============================================================================
 * NES-INSPIRED COLOR PALETTE
 * ============================================================================
 */

static const Color PAL_FLOOR_DARK  = { 38, 34, 50, 255 };
static const Color PAL_FLOOR_LIGHT = { 46, 42, 58, 255 };
static const Color PAL_WALL_BASE   = { 68, 62, 76, 255 };
static const Color PAL_WALL_HI     = { 88, 82, 96, 255 };
static const Color PAL_WALL_SHADOW = { 28, 26, 36, 255 };
static const Color PAL_DOOR_FRAME  = { 95, 85, 65, 255 };
static const Color PAL_DOOR_OPEN   = { 18, 16, 26, 255 };
static const Color PAL_DOOR_LOCKED = { 130, 50, 50, 255 };

// Room type palettes
static const Color PAL_TREASURE_FLOOR = { 45, 40, 55, 255 };
static const Color PAL_SHOP_FLOOR     = { 42, 38, 52, 255 };
static const Color PAL_BOSS_FLOOR     = { 50, 30, 35, 255 };
static const Color PAL_BOSS_FLOOR_LT  = { 58, 36, 42, 255 };
static const Color PAL_START_FLOOR    = { 40, 42, 55, 255 };

/*
 * ============================================================================
 * TILE-BASED FLOOR RENDERING
 * ============================================================================
 */

void DrawRoomBackground(DungeonRoom *room, float offsetX, float offsetY) {
    Color floorDark, floorLight;

    switch (room->type) {
        case ROOM_TREASURE:
            floorDark  = PAL_TREASURE_FLOOR;
            floorLight = (Color){ floorDark.r + 8, floorDark.g + 8, floorDark.b + 8, 255 };
            break;
        case ROOM_SHOP:
            floorDark  = PAL_SHOP_FLOOR;
            floorLight = (Color){ floorDark.r + 6, floorDark.g + 6, floorDark.b + 6, 255 };
            break;
        case ROOM_BOSS:
            floorDark  = PAL_BOSS_FLOOR;
            floorLight = PAL_BOSS_FLOOR_LT;
            break;
        case ROOM_START:
            floorDark  = PAL_START_FLOOR;
            floorLight = (Color){ floorDark.r + 8, floorDark.g + 8, floorDark.b + 8, 255 };
            break;
        default:
            floorDark  = PAL_FLOOR_DARK;
            floorLight = PAL_FLOOR_LIGHT;
            break;
    }

    int startTileX = WALL_THICKNESS;
    int startTileY = WALL_THICKNESS;
    int endTileX = TILES_X - WALL_THICKNESS;
    int endTileY = TILES_Y - WALL_THICKNESS;

    for (int tx = startTileX; tx < endTileX; tx++) {
        for (int ty = startTileY; ty < endTileY; ty++) {
            bool light = ((tx + ty) % 2 == 0);
            Color c = light ? floorLight : floorDark;

            // Add subtle variation using hash
            unsigned int h = HashTile(room->seed, tx, ty);
            if (h % 20 == 0) {
                // Slightly darker tile (stain)
                c.r = (unsigned char)(c.r > 6 ? c.r - 6 : 0);
                c.g = (unsigned char)(c.g > 6 ? c.g - 6 : 0);
                c.b = (unsigned char)(c.b > 6 ? c.b - 6 : 0);
            }

            DrawRectangle(
                (int)(offsetX + tx * TILE_SIZE),
                (int)(offsetY + ty * TILE_SIZE),
                TILE_SIZE, TILE_SIZE, c
            );

            // Draw cracks on some tiles (pixel-art style)
            if (h % 30 == 1) {
                float crackX = offsetX + tx * TILE_SIZE + (h % 10) + 3;
                float crackY = offsetY + ty * TILE_SIZE + ((h / 7) % 8) + 3;
                Color crk = { 30, 26, 40, 100 };
                DrawRectangle((int)crackX, (int)crackY, 4, 2, crk);
                DrawRectangle((int)(crackX + 3), (int)(crackY + 2), 2, 3, crk);
                if (h % 60 == 1) {
                    DrawRectangle((int)(crackX + 5), (int)(crackY + 1), 3, 2, crk);
                }
            }
        }
    }
}

/*
 * ============================================================================
 * WALL RENDERING
 * ============================================================================
 */

void DrawRoomWalls(DungeonRoom *room, float offsetX, float offsetY) {
    (void)room; // Seed used for future wall variation
    int wt = WALL_THICKNESS * TILE_SIZE;

    // Draw solid wall blocks
    // Top wall
    DrawRectangle((int)offsetX, (int)offsetY, (int)ROOM_WIDTH, wt, PAL_WALL_BASE);
    // Bottom wall
    DrawRectangle((int)offsetX, (int)(offsetY + ROOM_HEIGHT - wt), (int)ROOM_WIDTH, wt, PAL_WALL_BASE);
    // Left wall
    DrawRectangle((int)offsetX, (int)offsetY, wt, (int)ROOM_HEIGHT, PAL_WALL_BASE);
    // Right wall
    DrawRectangle((int)(offsetX + ROOM_WIDTH - wt), (int)offsetY, wt, (int)ROOM_HEIGHT, PAL_WALL_BASE);

    // Inner edge highlights (bevel effect)
    int bevel = 2;
    // Top inner edge
    DrawRectangle((int)offsetX, (int)(offsetY + wt - bevel), (int)ROOM_WIDTH, bevel, PAL_WALL_SHADOW);
    // Bottom inner edge
    DrawRectangle((int)offsetX, (int)(offsetY + ROOM_HEIGHT - wt), (int)ROOM_WIDTH, bevel, PAL_WALL_HI);
    // Left inner edge
    DrawRectangle((int)(offsetX + wt - bevel), (int)offsetY, bevel, (int)ROOM_HEIGHT, PAL_WALL_SHADOW);
    // Right inner edge
    DrawRectangle((int)(offsetX + ROOM_WIDTH - wt), (int)offsetY, bevel, (int)ROOM_HEIGHT, PAL_WALL_HI);

    // Outer highlight (top-left light source)
    DrawRectangle((int)offsetX, (int)offsetY, (int)ROOM_WIDTH, bevel, PAL_WALL_HI);
    DrawRectangle((int)offsetX, (int)offsetY, bevel, (int)ROOM_HEIGHT, PAL_WALL_HI);

    // Wall texture: brick pattern
    for (int tx = 0; tx < TILES_X; tx++) {
        for (int ty = 0; ty < WALL_THICKNESS; ty++) {
            // Top wall bricks
            if ((tx + ty * 3) % 5 == 0) {
                DrawRectangleLines(
                    (int)(offsetX + tx * TILE_SIZE + 1),
                    (int)(offsetY + ty * TILE_SIZE + 1),
                    TILE_SIZE - 2, TILE_SIZE - 2,
                    (Color){ PAL_WALL_SHADOW.r, PAL_WALL_SHADOW.g, PAL_WALL_SHADOW.b, 60 }
                );
            }
            // Bottom wall bricks
            if ((tx + ty * 3 + 2) % 5 == 0) {
                DrawRectangleLines(
                    (int)(offsetX + tx * TILE_SIZE + 1),
                    (int)(offsetY + (TILES_Y - WALL_THICKNESS + ty) * TILE_SIZE + 1),
                    TILE_SIZE - 2, TILE_SIZE - 2,
                    (Color){ PAL_WALL_SHADOW.r, PAL_WALL_SHADOW.g, PAL_WALL_SHADOW.b, 60 }
                );
            }
        }
        // Left/Right wall bricks
        for (int ty = WALL_THICKNESS; ty < TILES_Y - WALL_THICKNESS; ty++) {
            if ((tx + ty * 2) % 6 == 0 && tx < WALL_THICKNESS) {
                DrawRectangleLines(
                    (int)(offsetX + tx * TILE_SIZE + 1),
                    (int)(offsetY + ty * TILE_SIZE + 1),
                    TILE_SIZE - 2, TILE_SIZE - 2,
                    (Color){ PAL_WALL_SHADOW.r, PAL_WALL_SHADOW.g, PAL_WALL_SHADOW.b, 60 }
                );
            }
            if ((tx + ty * 2 + 1) % 6 == 0 && tx >= TILES_X - WALL_THICKNESS) {
                DrawRectangleLines(
                    (int)(offsetX + tx * TILE_SIZE + 1),
                    (int)(offsetY + ty * TILE_SIZE + 1),
                    TILE_SIZE - 2, TILE_SIZE - 2,
                    (Color){ PAL_WALL_SHADOW.r, PAL_WALL_SHADOW.g, PAL_WALL_SHADOW.b, 60 }
                );
            }
        }
    }
}

/*
 * ============================================================================
 * DOORWAYS
 * ============================================================================
 */

void DrawRoomDoorways(DungeonRoom *room, float offsetX, float offsetY) {
    int wt = WALL_THICKNESS * TILE_SIZE;
    float doorW = DOORWAY_WIDTH;
    float doorH = DOORWAY_HEIGHT;
    int p = 3; // Pixel block size

    for (int d = 0; d < 4; d++) {
        if (room->connections[d] < 0) continue;

        bool locked = room->doorLocked[d] || room->doorsLocked;
        Color doorBg = locked ? PAL_DOOR_LOCKED : PAL_DOOR_OPEN;
        Color frame = PAL_DOOR_FRAME;
        Color frameHi = { frame.r + 25, frame.g + 25, frame.b + 25, 255 };
        Color frameSh = { frame.r / 2, frame.g / 2, frame.b / 2, 255 };
        Color doorPanel = locked ? (Color){ 100, 40, 40, 255 } : (Color){ 75, 60, 45, 255 };
        Color doorPanelHi = { (unsigned char)(doorPanel.r + 20), (unsigned char)(doorPanel.g + 20), (unsigned char)(doorPanel.b + 20), 255 };

        float dx = 0, dy = 0, dw = 0, dh = 0;

        switch (d) {
            case DIR_NORTH:
                dx = offsetX + ROOM_WIDTH * 0.5f - doorW * 0.5f;
                dy = offsetY; dw = doorW; dh = (float)wt; break;
            case DIR_SOUTH:
                dx = offsetX + ROOM_WIDTH * 0.5f - doorW * 0.5f;
                dy = offsetY + ROOM_HEIGHT - wt; dw = doorW; dh = (float)wt; break;
            case DIR_WEST:
                dx = offsetX;
                dy = offsetY + ROOM_HEIGHT * 0.5f - doorH * 0.5f;
                dw = (float)wt; dh = doorH; break;
            case DIR_EAST:
                dx = offsetX + ROOM_WIDTH - wt;
                dy = offsetY + ROOM_HEIGHT * 0.5f - doorH * 0.5f;
                dw = (float)wt; dh = doorH; break;
        }

        // Dark opening behind door
        DrawRectangle((int)dx, (int)dy, (int)dw, (int)dh, doorBg);

        if (d == DIR_NORTH || d == DIR_SOUTH) {
            // Frame pillars
            DrawRectangle((int)(dx - 2*p), (int)dy, 2*p, (int)dh, frame);
            DrawRectangle((int)(dx - 2*p), (int)dy, p, (int)dh, frameHi);
            DrawRectangle((int)(dx + dw), (int)dy, 2*p, (int)dh, frame);
            DrawRectangle((int)(dx + dw + p), (int)dy, p, (int)dh, frameSh);
            // Arch top/bottom
            DrawRectangle((int)dx, (int)dy, (int)dw, p, frameHi);
            DrawRectangle((int)dx, (int)(dy + dh - p), (int)dw, p, frameSh);

            if (!locked) {
                // Open door panels (pushed aside)
                int panelW = (int)(dw * 0.2f);
                DrawRectangle((int)dx, (int)dy + p, panelW, (int)dh - 2*p, doorPanel);
                DrawRectangle((int)dx, (int)dy + p, p, (int)dh - 2*p, doorPanelHi);
                DrawRectangle((int)(dx + dw - panelW), (int)dy + p, panelW, (int)dh - 2*p, doorPanel);
            } else {
                // Closed door with panels and bars
                int halfW = (int)(dw / 2);
                DrawRectangle((int)dx, (int)dy + p, halfW, (int)dh - 2*p, doorPanel);
                DrawRectangle((int)dx, (int)dy + p, p, (int)dh - 2*p, doorPanelHi);
                DrawRectangle((int)(dx + halfW), (int)dy + p, halfW, (int)dh - 2*p, doorPanel);
                // Center seam
                DrawRectangle((int)(dx + halfW - 1), (int)dy + p, 2, (int)dh - 2*p, frameSh);
                // Cross bars
                int barY = (int)dy + (int)dh / 3;
                DrawRectangle((int)dx + p, barY, (int)dw - 2*p, p, (Color){ 130, 110, 80, 255 });
                barY = (int)dy + (int)dh * 2 / 3;
                DrawRectangle((int)dx + p, barY, (int)dw - 2*p, p, (Color){ 130, 110, 80, 255 });
                // Lock icon
                DrawRectangle((int)(dx + dw/2 - p), (int)(dy + dh/2 - p), 2*p, 2*p, (Color){ 200, 180, 50, 255 });
            }
        } else {
            // Side doors (West/East)
            DrawRectangle((int)dx, (int)(dy - 2*p), (int)dw, 2*p, frame);
            DrawRectangle((int)dx, (int)(dy - 2*p), (int)dw, p, frameHi);
            DrawRectangle((int)dx, (int)(dy + dh), (int)dw, 2*p, frame);
            DrawRectangle((int)dx, (int)(dy + dh + p), (int)dw, p, frameSh);
            // Top/bottom arch
            DrawRectangle((int)dx, (int)dy, (int)dw, p, frameHi);
            DrawRectangle((int)dx, (int)(dy + dh - p), (int)dw, p, frameSh);

            if (!locked) {
                int panelH = (int)(dh * 0.2f);
                DrawRectangle((int)dx + p, (int)dy, (int)dw - 2*p, panelH, doorPanel);
                DrawRectangle((int)dx + p, (int)(dy + dh - panelH), (int)dw - 2*p, panelH, doorPanel);
            } else {
                int halfH = (int)(dh / 2);
                DrawRectangle((int)dx + p, (int)dy, (int)dw - 2*p, halfH, doorPanel);
                DrawRectangle((int)dx + p, (int)dy, (int)dw - 2*p, p, doorPanelHi);
                DrawRectangle((int)dx + p, (int)(dy + halfH), (int)dw - 2*p, halfH, doorPanel);
                DrawRectangle((int)dx + p, (int)(dy + halfH - 1), (int)dw - 2*p, 2, frameSh);
                // Cross bars
                int barX = (int)dx + (int)dw / 3;
                DrawRectangle(barX, (int)dy + p, p, (int)dh - 2*p, (Color){ 130, 110, 80, 255 });
                barX = (int)dx + (int)dw * 2 / 3;
                DrawRectangle(barX, (int)dy + p, p, (int)dh - 2*p, (Color){ 130, 110, 80, 255 });
                DrawRectangle((int)(dx + dw/2 - p), (int)(dy + dh/2 - p), 2*p, 2*p, (Color){ 200, 180, 50, 255 });
            }
        }
    }
}

/*
 * ============================================================================
 * ROOM DECORATIONS
 * ============================================================================
 */

static void DrawSkullPile(float x, float y) {
    int p = 3; // Pixel size for 8-bit look
    Color bone = { 200, 195, 180, 255 };
    Color boneDark = { 150, 145, 130, 255 };
    Color boneHi = { 220, 215, 200, 255 };
    Color eye = { 30, 28, 40, 255 };

    // Skull 1 (pixelated square head)
    DrawRectangle((int)(x - 2*p), (int)(y - 2*p), 4*p, 4*p, bone);
    DrawRectangle((int)(x - 2*p), (int)(y - 2*p), 4*p, p, boneHi); // highlight top
    DrawRectangle((int)(x - p), (int)(y - p), p, p, eye);  // Left eye
    DrawRectangle((int)(x + p), (int)(y - p), p, p, eye);  // Right eye
    DrawRectangle((int)(x), (int)(y + p), p, p, boneDark);  // Nose/teeth

    // Skull 2 (smaller, offset)
    DrawRectangle((int)(x + 3*p), (int)(y), 3*p, 3*p, boneDark);
    DrawRectangle((int)(x + 4*p), (int)(y + p), p, p, eye);

    // Bone cross below
    DrawRectangle((int)(x - 2*p), (int)(y + 3*p), 5*p, p, bone);
    DrawRectangle((int)(x), (int)(y + 2*p), p, 3*p, boneDark);
}

static void DrawBrokenPillar(float x, float y) {
    int p = 3;
    Color stone = { 80, 76, 88, 255 };
    Color stoneHi = { 100, 96, 108, 255 };
    Color stoneSh = { 55, 52, 62, 255 };

    // Base slab
    DrawRectangle((int)(x - 3*p), (int)(y + 4*p), 6*p, 2*p, stoneSh);
    // Pillar body
    DrawRectangle((int)(x - 2*p), (int)(y - 1*p), 4*p, 5*p, stone);
    // Left highlight edge
    DrawRectangle((int)(x - 2*p), (int)(y - 1*p), p, 5*p, stoneHi);
    // Right shadow edge
    DrawRectangle((int)(x + p), (int)(y - 1*p), p, 5*p, stoneSh);
    // Broken jagged top
    DrawRectangle((int)(x - 2*p), (int)(y - 3*p), 3*p, 2*p, stone);
    DrawRectangle((int)(x - 2*p), (int)(y - 3*p), 3*p, p, stoneHi);
    DrawRectangle((int)(x + p), (int)(y - 2*p), p, p, stone);
}

static void DrawCobweb(float x, float y) {
    int p = 3;
    Color web = { 180, 180, 180, 45 };
    Color webDark = { 160, 160, 160, 30 };
    // Pixel-art corner web: L-shaped strands
    DrawRectangle((int)x, (int)y, 7*p, p, web);
    DrawRectangle((int)x, (int)y, p, 7*p, web);
    DrawRectangle((int)(x + p), (int)(y + p), 4*p, p, webDark);
    DrawRectangle((int)(x + p), (int)(y + p), p, 4*p, webDark);
    // Diagonal dot trail
    DrawRectangle((int)(x + 2*p), (int)(y + 2*p), p, p, webDark);
    DrawRectangle((int)(x + 3*p), (int)(y + 3*p), p, p, webDark);
}

static void DrawTreasureSparkle(float x, float y, float time) {
    int p = 3;
    float alpha = sinf(time * 4.0f + x * 0.1f) * 0.5f + 0.5f;
    Color sparkle = { 255, 220, 100, (unsigned char)(alpha * 180.0f) };
    // 4-pointed star (pixel cross)
    DrawRectangle((int)x, (int)(y - p), p, 3*p, sparkle);
    DrawRectangle((int)(x - p), (int)y, 3*p, p, sparkle);
}

static void DrawShopCounter(float x, float y) {
    int p = 4;
    Color wood = { 120, 80, 50, 255 };
    Color woodDark = { 90, 60, 38, 255 };
    Color woodHi = { 150, 100, 65, 255 };
    int w = 50 * p; // Counter width in pixels
    // Counter top surface
    DrawRectangle((int)x, (int)(y - 5*p), w, 5*p, wood);
    // Top highlight
    DrawRectangle((int)x, (int)(y - 5*p), w, p, woodHi);
    // Front panel
    DrawRectangle((int)x, (int)y, w, 3*p, woodDark);
    // Legs
    DrawRectangle((int)(x + p), (int)(y + 3*p), 2*p, 3*p, woodDark);
    DrawRectangle((int)(x + w - 3*p), (int)(y + 3*p), 2*p, 3*p, woodDark);
    // Front detail lines
    DrawRectangle((int)(x + 8*p), (int)(y + p), p, p, wood);
    DrawRectangle((int)(x + 16*p), (int)(y + p), p, p, wood);
    DrawRectangle((int)(x + 24*p), (int)(y + p), p, p, wood);
}

void DrawRoomDecorations(DungeonRoom *room, float offsetX, float offsetY) {
    unsigned int seed = room->seed;
    float innerX = offsetX + WALL_THICKNESS * TILE_SIZE + 10;
    float innerY = offsetY + WALL_THICKNESS * TILE_SIZE + 10;
    float innerW = ROOM_WIDTH - WALL_THICKNESS * TILE_SIZE * 2 - 20;
    float innerH = ROOM_HEIGHT - WALL_THICKNESS * TILE_SIZE * 2 - 20;

    switch (room->type) {
        case ROOM_COMBAT: {
            // Skull piles
            int numSkulls = 2 + seed % 3;
            for (int i = 0; i < numSkulls; i++) {
                unsigned int h = HashTile(seed, i, 0);
                float sx = innerX + (h % (int)innerW);
                float sy = innerY + ((h / 100) % (int)innerH);
                DrawSkullPile(sx, sy);
            }
            // Broken pillars
            if (seed % 3 == 0) {
                unsigned int h = HashTile(seed, 10, 10);
                DrawBrokenPillar(innerX + (h % (int)(innerW * 0.6f)) + innerW * 0.2f,
                                innerY + ((h / 50) % (int)(innerH * 0.6f)) + innerH * 0.2f);
            }
            // Cobwebs in corners
            DrawCobweb(offsetX + WALL_THICKNESS * TILE_SIZE + 2,
                      offsetY + WALL_THICKNESS * TILE_SIZE + 2);
            if (seed % 2 == 0) {
                DrawCobweb(offsetX + ROOM_WIDTH - WALL_THICKNESS * TILE_SIZE - 22,
                          offsetY + WALL_THICKNESS * TILE_SIZE + 2);
            }
            break;
        }
        case ROOM_TREASURE: {
            // Sparkle effects
            for (int i = 0; i < 8; i++) {
                unsigned int h = HashTile(seed + 500, i, 0);
                float sx = innerX + (h % (int)innerW);
                float sy = innerY + ((h / 100) % (int)innerH);
                DrawTreasureSparkle(sx, sy, gameTime);
            }
            // Golden trim on floor edges
            Color gold = { 180, 150, 50, 80 };
            int wt = WALL_THICKNESS * TILE_SIZE;
            DrawRectangle((int)(offsetX + wt), (int)(offsetY + wt), (int)(ROOM_WIDTH - wt * 2), 2, gold);
            DrawRectangle((int)(offsetX + wt), (int)(offsetY + ROOM_HEIGHT - wt - 2), (int)(ROOM_WIDTH - wt * 2), 2, gold);
            DrawRectangle((int)(offsetX + wt), (int)(offsetY + wt), 2, (int)(ROOM_HEIGHT - wt * 2), gold);
            DrawRectangle((int)(offsetX + ROOM_WIDTH - wt - 2), (int)(offsetY + wt), 2, (int)(ROOM_HEIGHT - wt * 2), gold);
            break;
        }
        case ROOM_SHOP: {
            // Counter near top
            DrawShopCounter(offsetX + ROOM_WIDTH * 0.5f - 100,
                          offsetY + ROOM_HEIGHT * 0.3f);
            break;
        }
        case ROOM_BOSS: {
            // Corner pillars (8-bit styled)
            int p = 4;
            float pillarInset = 80.0f;
            float ppx[4] = { innerX + pillarInset, innerX + innerW - pillarInset,
                            innerX + pillarInset, innerX + innerW - pillarInset };
            float ppy[4] = { innerY + pillarInset, innerY + pillarInset,
                            innerY + innerH - pillarInset, innerY + innerH - pillarInset };
            for (int i = 0; i < 4; i++) {
                Color pBase = { 65, 40, 45, 255 };
                Color pHi = { 90, 58, 62, 255 };
                Color pSh = { 45, 28, 32, 255 };
                // Pillar body
                DrawRectangle((int)(ppx[i] - 3*p), (int)(ppy[i] - 4*p), 6*p, 8*p, pBase);
                // Left highlight
                DrawRectangle((int)(ppx[i] - 3*p), (int)(ppy[i] - 4*p), p, 8*p, pHi);
                // Top highlight
                DrawRectangle((int)(ppx[i] - 3*p), (int)(ppy[i] - 4*p), 6*p, p, pHi);
                // Right shadow
                DrawRectangle((int)(ppx[i] + 2*p), (int)(ppy[i] - 4*p), p, 8*p, pSh);
                // Bottom shadow
                DrawRectangle((int)(ppx[i] - 3*p), (int)(ppy[i] + 3*p), 6*p, p, pSh);
                // Cap (wider top)
                DrawRectangle((int)(ppx[i] - 4*p), (int)(ppy[i] - 5*p), 8*p, p, pHi);
            }
            break;
        }
        case ROOM_START: {
            // Pixelated spawn pad (diamond pattern)
            int p = 4;
            float cx = offsetX + ROOM_WIDTH * 0.5f;
            float cy = offsetY + ROOM_HEIGHT * 0.5f;
            Color padBase = { 50, 55, 70, 100 };
            Color padEdge = { 70, 75, 90, 130 };
            // Diamond shape using pixel rows
            int widths[] = { 2, 4, 6, 8, 8, 8, 6, 4, 2 };
            int rows = 9;
            for (int r = 0; r < rows; r++) {
                int w = widths[r];
                float rx = cx - w * p * 0.5f;
                float ry = cy - (rows / 2) * p + r * p;
                Color c = (r == 0 || r == rows - 1 || w == 8) ? padEdge : padBase;
                DrawRectangle((int)rx, (int)ry, w * p, p, c);
            }
            // Inner cross mark
            DrawRectangle((int)(cx - p), (int)(cy - 2*p), 2*p, 4*p, padEdge);
            DrawRectangle((int)(cx - 2*p), (int)(cy - p), 4*p, 2*p, padEdge);
            break;
        }
        default: break;
    }
}

/*
 * ============================================================================
 * ANIMATED TORCHES
 * ============================================================================
 */

void DrawTorches(DungeonRoom *room, float offsetX, float offsetY) {
    if (room->type == ROOM_START) return;

    int wt = WALL_THICKNESS * TILE_SIZE;
    unsigned int seed = room->seed;
    int p = 3; // Pixel size

    int numTorches = 4 + seed % 3;
    for (int i = 0; i < numTorches; i++) {
        unsigned int h = HashTile(seed + 777, i, 0);
        float tx, ty;

        int wall = i % 4;
        float pos = 0.2f + (float)(h % 60) / 100.0f;

        switch (wall) {
            case 0: tx = offsetX + ROOM_WIDTH * pos; ty = offsetY + wt - 2; break;
            case 1: tx = offsetX + ROOM_WIDTH * pos; ty = offsetY + ROOM_HEIGHT - wt + 2; break;
            case 2: tx = offsetX + wt - 2; ty = offsetY + ROOM_HEIGHT * pos; break;
            default: tx = offsetX + ROOM_WIDTH - wt + 2; ty = offsetY + ROOM_HEIGHT * pos; break;
        }

        // Wall bracket (pixel mount)
        DrawRectangle((int)(tx - p), (int)(ty - p), 2*p, p, (Color){ 80, 65, 50, 255 });
        DrawRectangle((int)(tx - p), (int)(ty - p), p, p, (Color){ 100, 85, 65, 255 });

        // Candle body
        Color wax = { 200, 190, 170, 255 };
        Color waxHi = { 220, 215, 200, 255 };
        DrawRectangle((int)(tx - p), (int)(ty - 4*p), 2*p, 3*p, wax);
        DrawRectangle((int)(tx - p), (int)(ty - 4*p), p, 3*p, waxHi); // highlight

        // Wick
        DrawRectangle((int)(tx), (int)(ty - 5*p), p, p, (Color){ 60, 50, 40, 255 });

        // Animated flame (pixel blocks)
        float flicker = sinf(gameTime * 8.0f + (float)i * 2.3f) * 0.3f + 0.7f;
        float flicker2 = sinf(gameTime * 12.0f + (float)i * 1.7f) * 0.2f + 0.8f;

        Color flameOuter = {
            (unsigned char)(255.0f * flicker),
            (unsigned char)(150.0f * flicker2),
            30, 220
        };
        Color flameInner = { 255, (unsigned char)(230.0f * flicker), 80, 255 };
        Color flameTip = { 255, 255, (unsigned char)(150.0f * flicker), 200 };

        // Outer flame (wider at base)
        DrawRectangle((int)(tx - p), (int)(ty - 7*p), 2*p, 2*p, flameOuter);
        // Inner bright core
        DrawRectangle((int)(tx), (int)(ty - 7*p), p, p, flameInner);
        // Tip (flickers between 1-2 pixels high)
        if (flicker > 0.6f) {
            DrawRectangle((int)(tx), (int)(ty - 8*p), p, p, flameTip);
        }

        // Pixel glow (square, subtle)
        int glowSz = (int)(4*p * flicker);
        DrawRectangle((int)(tx - glowSz/2), (int)(ty - 6*p - glowSz/2), glowSz, glowSz,
                     (Color){ 255, 180, 50, (unsigned char)(18.0f * flicker) });
    }
}

/*
 * ============================================================================
 * PIXEL ART ICONS
 * ============================================================================
 */

void DrawPixelHeart(float x, float y, float scale, float fillRatio, Color color) {
    // 7x7 pixel heart pattern
    static const int heart[7][7] = {
        {0,1,1,0,1,1,0},
        {1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1},
        {0,1,1,1,1,1,0},
        {0,0,1,1,1,0,0},
        {0,0,0,1,0,0,0},
    };

    Color empty = { color.r / 3, color.g / 3, color.b / 3, 200 };
    int totalPixels = 0, filledPixels = 0;

    // Count total pixels
    for (int py = 0; py < 7; py++)
        for (int px = 0; px < 7; px++)
            if (heart[py][px]) totalPixels++;

    int fillCount = (int)(fillRatio * totalPixels + 0.5f);

    for (int py = 0; py < 7; py++) {
        for (int px = 0; px < 7; px++) {
            if (!heart[py][px]) continue;
            bool filled = (filledPixels < fillCount);
            filledPixels++;
            Color c = filled ? color : empty;
            DrawRectangle(
                (int)(x + px * scale),
                (int)(y + py * scale),
                (int)scale, (int)scale, c
            );
        }
    }
}

void DrawPixelKey(float x, float y, float scale, Color color) {
    // Key head (circle)
    DrawCircle((int)(x + 2 * scale), (int)(y + 2 * scale), scale * 2, color);
    DrawCircle((int)(x + 2 * scale), (int)(y + 2 * scale), scale, (Color){ 0, 0, 0, 150 });
    // Key shaft
    DrawRectangle((int)(x + 3 * scale), (int)(y + 1.5f * scale), (int)(scale * 4), (int)scale, color);
    // Key teeth
    DrawRectangle((int)(x + 6 * scale), (int)(y + 2.5f * scale), (int)scale, (int)scale, color);
    DrawRectangle((int)(x + 5 * scale), (int)(y + 2.5f * scale), (int)scale, (int)(scale * 0.7f), color);
}

void DrawPixelCoin(float x, float y, float scale, Color color) {
    float s = scale;
    Color dark = {
        (unsigned char)(color.r * 0.6f),
        (unsigned char)(color.g * 0.6f),
        (unsigned char)(color.b * 0.6f), 255
    };
    Color hi = {
        (unsigned char)(color.r + (255 - color.r) / 3),
        (unsigned char)(color.g + (255 - color.g) / 3),
        (unsigned char)(color.b + (255 - color.b) / 3), 255
    };
    // Square coin body
    DrawRectangle((int)(x + s), (int)y, (int)(4*s), (int)(6*s), color);
    DrawRectangle((int)x, (int)(y + s), (int)(6*s), (int)(4*s), color);
    // Highlight top-left
    DrawRectangle((int)(x + s), (int)y, (int)(4*s), (int)s, hi);
    DrawRectangle((int)x, (int)(y + s), (int)s, (int)(3*s), hi);
    // Shadow bottom-right
    DrawRectangle((int)(x + s), (int)(y + 5*s), (int)(4*s), (int)s, dark);
    DrawRectangle((int)(x + 5*s), (int)(y + s), (int)s, (int)(3*s), dark);
    // Center mark (C shape)
    DrawRectangle((int)(x + 2*s), (int)(y + 2*s), (int)(2*s), (int)s, dark);
    DrawRectangle((int)(x + 2*s), (int)(y + 3*s), (int)s, (int)s, dark);
}

void DrawScanlineOverlay(void) {
    // Subtle CRT scanline effect for 8-bit feel
    for (int y = 0; y < SCREEN_HEIGHT; y += 3) {
        DrawRectangle(0, y, SCREEN_WIDTH, 1, (Color){ 0, 0, 0, 18 });
    }
    // Slight vignette on edges
    DrawRectangle(0, 0, SCREEN_WIDTH, 2, (Color){ 0, 0, 0, 30 });
    DrawRectangle(0, SCREEN_HEIGHT - 2, SCREEN_WIDTH, 2, (Color){ 0, 0, 0, 30 });
    DrawRectangle(0, 0, 2, SCREEN_HEIGHT, (Color){ 0, 0, 0, 30 });
    DrawRectangle(SCREEN_WIDTH - 2, 0, 2, SCREEN_HEIGHT, (Color){ 0, 0, 0, 30 });
}

void DrawItemIcon(ItemType type, float x, float y, float scale) {
    switch (type) {
        case ITEM_HEART:
            DrawPixelHeart(x, y, scale, 1.0f, (Color){ 220, 50, 50, 255 });
            break;
        case ITEM_HEART_CONTAINER:
            DrawPixelHeart(x, y, scale, 1.0f, (Color){ 255, 200, 50, 255 });
            break;
        case ITEM_DAMAGE_UP: {
            Color red = { 220, 60, 60, 255 };
            // Upward arrow
            DrawRectangle((int)(x + 2*scale), (int)(y + 2*scale), (int)(3*scale), (int)(4*scale), red);
            DrawRectangle((int)(x + 1*scale), (int)(y + 2*scale), (int)(5*scale), (int)(2*scale), red);
            DrawRectangle((int)(x + 2*scale), (int)(y + 1*scale), (int)(3*scale), (int)scale, red);
            DrawRectangle((int)(x + 3*scale), (int)y, (int)scale, (int)scale, red);
            break;
        }
        case ITEM_SPEED_UP: {
            Color blue = { 60, 120, 220, 255 };
            DrawRectangle((int)(x + 2*scale), (int)(y + 2*scale), (int)(3*scale), (int)(4*scale), blue);
            DrawRectangle((int)(x + 1*scale), (int)(y + 2*scale), (int)(5*scale), (int)(2*scale), blue);
            DrawRectangle((int)(x + 2*scale), (int)(y + 1*scale), (int)(3*scale), (int)scale, blue);
            DrawRectangle((int)(x + 3*scale), (int)y, (int)scale, (int)scale, blue);
            break;
        }
        case ITEM_KEY:
            DrawPixelKey(x, y, scale, (Color){ 230, 200, 50, 255 });
            break;
        case ITEM_COIN:
            DrawPixelCoin(x, y, scale, (Color){ 230, 200, 50, 255 });
            break;
        case ITEM_SHIELD: {
            Color shieldC = { 80, 160, 220, 255 };
            DrawRectangle((int)(x + 1*scale), (int)y, (int)(5*scale), (int)(4*scale), shieldC);
            DrawRectangle((int)(x + 2*scale), (int)(y + 4*scale), (int)(3*scale), (int)(2*scale), shieldC);
            DrawRectangle((int)(x + 3*scale), (int)(y + 6*scale), (int)scale, (int)scale, shieldC);
            break;
        }
        case ITEM_BOMB: {
            Color bombC = { 60, 60, 60, 255 };
            DrawCircle((int)(x + 3*scale), (int)(y + 4*scale), 3*scale, bombC);
            DrawRectangle((int)(x + 2.5f*scale), (int)y, (int)scale, (int)(2*scale), (Color){160, 120, 80, 255});
            // Fuse spark
            float spark = sinf(gameTime * 10.0f) * 0.5f + 0.5f;
            DrawCircle((int)(x + 3*scale), (int)y, scale * (0.5f + spark), (Color){255, 200, 50, (unsigned char)(200*spark)});
            break;
        }
        case ITEM_RUBBER_DUCK: {
            // Small red rubber duck (8-bit pixel art)
            float s = scale;
            Color duckBody = { 220, 50, 50, 255 };
            Color duckBeak = { 255, 180, 50, 255 };
            Color duckEye = { 20, 20, 20, 255 };
            Color duckWing = { 180, 40, 40, 255 };
            Color duckHi = { 255, 100, 100, 255 };
            // Body (round blob)
            DrawRectangle((int)(x + 1*s), (int)(y + 3*s), (int)(5*s), (int)(4*s), duckBody);
            DrawRectangle((int)(x + 0*s), (int)(y + 4*s), (int)(7*s), (int)(2*s), duckBody);
            // Head
            DrawRectangle((int)(x + 1*s), (int)(y + 1*s), (int)(3*s), (int)(3*s), duckBody);
            // Highlight
            DrawRectangle((int)(x + 1*s), (int)(y + 1*s), (int)(2*s), (int)s, duckHi);
            // Beak (pointing right)
            DrawRectangle((int)(x + 4*s), (int)(y + 2*s), (int)(2*s), (int)s, duckBeak);
            // Eye
            DrawRectangle((int)(x + 2*s), (int)(y + 2*s), (int)s, (int)s, duckEye);
            // Wing
            DrawRectangle((int)(x + 2*s), (int)(y + 4*s), (int)(3*s), (int)(2*s), duckWing);
            // Tail
            DrawRectangle((int)(x + 0*s), (int)(y + 3*s), (int)s, (int)(2*s), duckBody);
            // Star sparkle (immunity indicator)
            float sparkle = sinf(gameTime * 6.0f) * 0.5f + 0.5f;
            DrawRectangle((int)(x + 3*s), (int)(y - 1*s), (int)s, (int)(3*s),
                         (Color){ 255, 255, 100, (unsigned char)(200 * sparkle) });
            DrawRectangle((int)(x + 2*s), (int)(y + 0*s), (int)(3*s), (int)s,
                         (Color){ 255, 255, 100, (unsigned char)(200 * sparkle) });
            break;
        }
        default: break;
    }
}
