#ifndef ITEMS_H
#define ITEMS_H

#include "types.h"

void ItemsInit(void);
void ItemSpawn(ItemType type, Vector2 pos, int roomId, int price);
void ItemsUpdate(float dt);
void ItemsDraw(void);
void ItemDropFromEnemy(Vector2 pos, int roomId);
void ShopRoomSetup(int roomId);

#endif // ITEMS_H
