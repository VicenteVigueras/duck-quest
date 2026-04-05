#ifndef ENTITIES_H
#define ENTITIES_H

#include "types.h"

/*
 * ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

float Clamp(float v, float min, float max);
float Lerp(float a, float b, float t);
float NormalizeAngle(float angle);

Vector2 Vec2Add(Vector2 a, Vector2 b);
Vector2 Vec2Sub(Vector2 a, Vector2 b);
Vector2 Vec2Scale(Vector2 v, float s);
float Vec2Length(Vector2 v);
Vector2 Vec2Normalize(Vector2 v);
float Vec2Distance(Vector2 a, Vector2 b);

/*
 * ============================================================================
 * COLLISION & PHYSICS
 * ============================================================================
 */

bool CircleCollision(Vector2 p1, float r1, Vector2 p2, float r2);
void ResolveCircleCollision(Vector2 *pos1, float r1, Vector2 *vel1, 
                            Vector2 *pos2, float r2, Vector2 *vel2);
void ApplyKnockback(Vector2 *knockback, Vector2 direction, float strength);

/*
 * ============================================================================
 * PLAYER SYSTEM
 * ============================================================================
 */

void PlayerInit(void);
void PlayerRespawn(void);
void PlayerUpdate(float dt);
void PlayerDraw(Texture2D spriteSheet);

/*
 * ============================================================================
 * ENEMY SYSTEM
 * ============================================================================
 */

void EnemyInit(Enemy *e, Vector2 pos, int index, int totalEnemies);
void EnemyUpdate(Enemy *e, float dt);
void EnemyDraw(Enemy *e, Texture2D spriteSheet);

/*
 * ============================================================================
 * SWORD SYSTEM
 * ============================================================================
 */

void SwordInit(void);
void SwordUpdate(float dt);
void SwordDraw(void);


#endif // ENTITIES_H