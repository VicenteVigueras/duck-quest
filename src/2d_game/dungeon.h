#ifndef DUNGEON_H
#define DUNGEON_H

#include "types.h"

void DungeonGenerate(Dungeon *dun, unsigned int seed);
void DungeonUpdate(float dt);
void DungeonDraw(void);
DungeonRoom* DungeonGetCurrentRoom(void);
DungeonRoom* DungeonGetRoom(int id);
void DungeonCheckDoorways(void);
Vector2 DungeonGetCameraOffset(void);
void DungeonSpawnRoomEnemies(int roomId);
int DungeonGetRoomAtGrid(int gx, int gy);

#endif // DUNGEON_H
