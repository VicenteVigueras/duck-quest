#ifndef ENTITIES_H
#define ENTITIES_H

#include "types.h"

// Player
void PlayerInit(void);
void PlayerRespawn(void);
void PlayerUpdate(float dt);
void PlayerDraw(Texture2D spriteSheet);

// Enemies
void EnemyUpdate(Enemy *e, float dt);
void EnemyDraw(Enemy *e);

// Sword (pickup in start room)
void SwordInit(int roomId);
void SwordUpdate(float dt);
void SwordDraw(void);

#endif // ENTITIES_H
