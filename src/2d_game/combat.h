#ifndef COMBAT_H
#define COMBAT_H

#include "types.h"

void ShakeScreen(float intensity, float duration, float frequency);
void ScreenShakeUpdate(float dt);
Vector2 ScreenShakeGetOffset(void);

void ProjectileSpawn(Vector2 pos, Vector2 vel, float radius, float damage,
                     bool friendly, int roomId, Color color);
void ProjectilesUpdate(float dt);
void ProjectilesDraw(void);

void CombatApplyDamageToEnemy(Enemy *e, float damage, Vector2 from);
void CombatApplyDamageToPlayer(float damage, Vector2 from);

#endif // COMBAT_H
