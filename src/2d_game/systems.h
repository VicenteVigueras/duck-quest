#ifndef SYSTEMS_H
#define SYSTEMS_H

#include "types.h"

// Particles
void ParticlesUpdate(float dt);
void ParticlesDraw(void);

// UI / HUD
void HUDDraw(void);
void DrawRoomNameFlash(void);
void ResetRoomNames(void);

// Flashes a "NEED KEY!" message when the player bumps a key-locked door
void SystemsShowLockedDoorMsg(void);

// Game states
void TitleScreenDraw(void);
void PauseScreenDraw(void);
void GameOverScreenDraw(void);
void VictoryScreenDraw(void);

#endif // SYSTEMS_H
