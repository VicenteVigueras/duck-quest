#include "minimap.h"
#include "utils.h"

#define MINIMAP_ROOM_SIZE 14
#define MINIMAP_GAP 3
#define MINIMAP_PADDING 20

void MinimapDraw(void) {
    // Position in top-right corner
    float baseX = SCREEN_WIDTH - MINIMAP_PADDING - DUNGEON_GRID_SIZE * (MINIMAP_ROOM_SIZE + MINIMAP_GAP);
    float baseY = MINIMAP_PADDING;

    // Semi-transparent background
    float bgW = DUNGEON_GRID_SIZE * (MINIMAP_ROOM_SIZE + MINIMAP_GAP) + 10;
    float bgH = DUNGEON_GRID_SIZE * (MINIMAP_ROOM_SIZE + MINIMAP_GAP) + 10;
    DrawRectangle((int)(baseX - 5), (int)(baseY - 5), (int)bgW, (int)bgH,
                 (Color){ 10, 10, 15, 150 });
    DrawRectangleLinesEx(
        (Rectangle){ baseX - 5, baseY - 5, bgW, bgH },
        1, (Color){ 80, 80, 100, 150 });

    for (int i = 0; i < dungeon.roomCount; i++) {
        DungeonRoom *room = &dungeon.rooms[i];
        if (!room->visited) continue;

        float rx = baseX + room->gridX * (MINIMAP_ROOM_SIZE + MINIMAP_GAP);
        float ry = baseY + room->gridY * (MINIMAP_ROOM_SIZE + MINIMAP_GAP);

        Color roomColor;
        if (i == dungeon.currentRoomId) {
            roomColor = (Color){ 255, 255, 255, 255 };
        } else {
            switch (room->type) {
                case ROOM_START:    roomColor = (Color){ 100, 200, 100, 200 }; break;
                case ROOM_COMBAT:   roomColor = room->cleared
                                        ? (Color){ 80, 80, 100, 180 }
                                        : (Color){ 180, 80, 80, 200 }; break;
                case ROOM_TREASURE: roomColor = (Color){ 220, 180, 50, 220 }; break;
                case ROOM_SHOP:     roomColor = (Color){ 100, 180, 220, 200 }; break;
                case ROOM_BOSS:     roomColor = (Color){ 200, 50, 50, 230 }; break;
                default:            roomColor = (Color){ 80, 80, 80, 150 }; break;
            }
        }

        DrawRectangle((int)rx, (int)ry, MINIMAP_ROOM_SIZE, MINIMAP_ROOM_SIZE, roomColor);

        // Draw connections
        for (int d = 0; d < 4; d++) {
            int nid = room->connections[d];
            if (nid < 0 || !dungeon.rooms[nid].visited) continue;

            float cx = rx + MINIMAP_ROOM_SIZE / 2;
            float cy = ry + MINIMAP_ROOM_SIZE / 2;
            float nx = cx, ny = cy;
            int half = (MINIMAP_ROOM_SIZE + MINIMAP_GAP) / 2;

            switch (d) {
                case DIR_NORTH: ny -= half; break;
                case DIR_EAST:  nx += half; break;
                case DIR_SOUTH: ny += half; break;
                case DIR_WEST:  nx -= half; break;
            }

            DrawLine((int)cx, (int)cy, (int)nx, (int)ny, (Color){ 120, 120, 140, 150 });
        }
    }
}
