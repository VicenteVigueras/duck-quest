#ifndef SYSTEMS_H
#define SYSTEMS_H

#include "types.h"

void ParticleSpawn(Vector2 pos, Vector2 vel, float life, float size, 
                   Color color, ParticleType type);
void ParticlesUpdate(float dt);
void ParticlesDraw(void);
void ParticlesBurst(Vector2 pos, int count, Color color, ParticleType type, 
                    float speedMin, float speedMax);

void UIDrawHealthBar(void);
void UIDrawWaveInfo(void);
void UIDrawControls(void);
void UIDrawRoomIndicator(void);
void UIDrawFPS(void);
void UIDrawCenterMessage(const char *title, const char *subtitle, const char *hint,
                         Color titleColor, int titleSize);
void UIDraw(void);

void StartWave(int wave);

void RoomSystemInit(void);
void RoomSystemUpdate(float dt);
void RoomSystemDraw(void);
Room* GetCurrentRoom(void);
void CheckDoorwayCollision(void);
Vector2 GetCameraOffset(void);

#endif // SYSTEMS_H