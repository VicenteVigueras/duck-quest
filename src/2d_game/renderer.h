#ifndef RENDERER_H
#define RENDERER_H

#include "types.h"

void DrawRoomBackground(DungeonRoom *room, float offsetX, float offsetY);
void DrawRoomWalls(DungeonRoom *room, float offsetX, float offsetY);
void DrawRoomDoorways(DungeonRoom *room, float offsetX, float offsetY);
void DrawRoomDecorations(DungeonRoom *room, float offsetX, float offsetY);
void DrawTorches(DungeonRoom *room, float offsetX, float offsetY);
void DrawPixelHeart(float x, float y, float scale, float fillRatio, Color color);
void DrawPixelKey(float x, float y, float scale, Color color);
void DrawPixelCoin(float x, float y, float scale, Color color);
void DrawItemIcon(ItemType type, float x, float y, float scale);
void DrawPixelText(const char *text, float x, float y, int size, Color color);
void DrawScanlineOverlay(void);

#endif // RENDERER_H
