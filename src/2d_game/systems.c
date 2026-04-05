#include "systems.h"
#include "entities.h"
#include <stdlib.h>
#include <math.h>

void ParticleSpawn(Vector2 pos, Vector2 vel, float life, float size, 
                   Color color, ParticleType type) {
    if (particles.count >= MAX_PARTICLES) return;
    
    Particle *p = &particles.pool[particles.count++];
    p->pos = pos;
    p->vel = vel;
    p->life = life;
    p->maxLife = life;
    p->size = size;
    p->color = color;
    p->type = type;
}

void ParticlesUpdate(float dt) {
    int writeIdx = 0;
    for (int i = 0; i < particles.count; i++) {
        Particle *p = &particles.pool[i];
        
        p->life -= dt;
        if (p->life <= 0.0f) continue;
        
        p->vel.y += 300.0f * dt;
        p->vel.x *= 0.96f;
        p->vel.y *= 0.96f;
        
        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;
        
        particles.pool[writeIdx++] = *p;
    }
    particles.count = writeIdx;
}

void ParticlesDraw(void) {
    for (int i = 0; i < particles.count; i++) {
        Particle *p = &particles.pool[i];
        float lifeRatio = p->life / p->maxLife;
        float alpha = lifeRatio;
        Color col = Fade(p->color, alpha);
        
        float drawSize = p->size * (0.5f + lifeRatio * 0.5f);
        DrawCircleV(p->pos, drawSize, col);
    }
}

void ParticlesBurst(Vector2 pos, int count, Color color, ParticleType type, 
                    float speedMin, float speedMax) {
    for (int i = 0; i < count; i++) {
        float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float speed = speedMin + ((float)rand() / RAND_MAX) * (speedMax - speedMin);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        float size = 2.0f + ((float)rand() / RAND_MAX) * 4.0f;
        float life = 0.3f + ((float)rand() / RAND_MAX) * 0.5f;
        ParticleSpawn(pos, vel, life, size, color, type);
    }
}

#define UI_PADDING 30.0f
#define UI_ELEMENT_SPACING 15.0f

void UIDrawHealthBar(void) {
    float healthRatio = player.health / player.maxHealth;
    
    Color barColor;
    if (healthRatio > 0.66f) {
        barColor = (Color){ 100, 255, 100, 255 };
    } else if (healthRatio > 0.33f) {
        barColor = (Color){ 255, 200, 100, 255 };
    } else {
        barColor = (Color){ 255, 100, 100, 255 };
    }
    
    float barWidth = 300.0f;
    float barHeight = 40.0f;
    float barX = UI_PADDING;
    float barY = UI_PADDING;
    
    DrawRectangle((int)(barX - 3), (int)(barY - 3), (int)(barWidth + 6), (int)(barHeight + 6), 
                 Fade(BLACK, 0.4f));
    
    DrawRectangle((int)barX, (int)barY, (int)barWidth, (int)barHeight, 
                 Fade((Color){ 20, 22, 28, 255 }, 0.85f));
    
    DrawRectangle((int)barX, (int)barY, (int)(barWidth * healthRatio), (int)barHeight, barColor);
    
    DrawRectangle((int)barX, (int)barY, (int)(barWidth * healthRatio), (int)(barHeight * 0.3f), 
                 Fade(WHITE, 0.15f));
    
    if (healthRatio < 0.2f) {
        float pulseAlpha = sinf(gameTime * 6.0f) * 0.4f + 0.3f;
        DrawRectangle((int)barX, (int)barY, (int)(barWidth * healthRatio), (int)barHeight, 
                     Fade(WHITE, pulseAlpha));
    }
    
    DrawRectangleLinesEx((Rectangle){ barX, barY, barWidth, barHeight }, 2.0f, 
                        Fade(barColor, 0.8f));
    
    const char *healthText = TextFormat("%d / %d", (int)player.health, (int)player.maxHealth);
    int textWidth = MeasureText(healthText, 22);
    DrawText(healthText, 
             (int)(barX + (barWidth - textWidth) * 0.5f), 
             (int)(barY + 9), 
             22, 
             WHITE);
    
    DrawText("HEALTH", 
             (int)barX, 
             (int)(barY - 20), 
             16, 
             Fade(WHITE, 0.6f));
}

void UIDrawWaveInfo(void) {
    int aliveCount = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) aliveCount++;
    }
    
    float x = UI_PADDING;
    float y = UI_PADDING + 40.0f + UI_ELEMENT_SPACING;
    
    const char *waveText = TextFormat("WAVE %d", currentWave);
    DrawText(waveText, (int)x, (int)y, 28, (Color){ 255, 220, 100, 255 });
    
    const char *enemyText = TextFormat("Enemies: %d / %d", aliveCount, enemiesInWave);
    DrawText(enemyText, (int)x, (int)(y + 32), 20, Fade(WHITE, 0.75f));
}

void UIDrawControls(void) {
    const char *controlsTitle = "CONTROLS";
    const char *moveText = "WASD - Move";
    const char *attackText = player.hasSword ? "SPACE - Attack" : "Find the sword!";
    
    float rightX = GetScreenWidth() - UI_PADDING;
    float y = UI_PADDING;
    
    int titleWidth = MeasureText(controlsTitle, 16);
    DrawText(controlsTitle, (int)(rightX - titleWidth), (int)y, 16, Fade(WHITE, 0.5f));
    y += 24;
    
    int moveWidth = MeasureText(moveText, 18);
    DrawText(moveText, (int)(rightX - moveWidth), (int)y, 18, Fade(WHITE, 0.7f));
    y += 24;
    
    int attackWidth = MeasureText(attackText, 18);
    Color attackColor = player.hasSword ? (Color){ 255, 220, 100, 255 } : Fade(WHITE, 0.5f);
    DrawText(attackText, (int)(rightX - attackWidth), (int)y, 18, attackColor);
}

void UIDrawRoomIndicator(void) {
    Room *currentRoom = GetCurrentRoom();
    const char *roomText = TextFormat("Room (%d, %d)", currentRoom->x, currentRoom->y);
    
    float x = UI_PADDING;
    float y = GetScreenHeight() - UI_PADDING - 20;
    
    DrawText(roomText, (int)x, (int)y, 20, Fade(WHITE, 0.4f));
}

void UIDrawFPS(void) {
    const char *fpsText = TextFormat("%d FPS", GetFPS());
    int fpsWidth = MeasureText(fpsText, 20);
    
    float x = GetScreenWidth() - UI_PADDING - fpsWidth;
    float y = GetScreenHeight() - UI_PADDING - 20;
    
    Color fpsColor;
    int fps = GetFPS();
    if (fps >= 120) {
        fpsColor = (Color){ 100, 255, 100, 255 };
    } else if (fps >= 60) {
        fpsColor = (Color){ 255, 220, 100, 255 };
    } else {
        fpsColor = (Color){ 255, 100, 100, 255 };
    }
    
    DrawText(fpsText, (int)x, (int)y, 20, Fade(fpsColor, 0.5f));
}

void UIDrawCenterMessage(const char *title, const char *subtitle, const char *hint, 
                         Color titleColor, int titleSize) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.5f));
    
    float centerY = screenH * 0.5f;
    
    int titleWidth = MeasureText(title, titleSize);
    int titleX = (screenW - titleWidth) / 2;
    
    DrawText(title, titleX + 2, (int)(centerY - 60 + 2), titleSize, Fade(BLACK, 0.5f));
    DrawText(title, titleX, (int)(centerY - 60), titleSize, titleColor);
    
    if (subtitle) {
        int subtitleWidth = MeasureText(subtitle, 28);
        DrawText(subtitle, 
                 (screenW - subtitleWidth) / 2, 
                 (int)(centerY), 
                 28, 
                 Fade(WHITE, 0.85f));
    }
    
    if (hint) {
        int hintWidth = MeasureText(hint, 20);
        DrawText(hint, 
                 (screenW - hintWidth) / 2, 
                 (int)(centerY + 50), 
                 20, 
                 Fade(WHITE, 0.6f + sinf(gameTime * 3.0f) * 0.2f));
    }
}

void UIDraw(void) {
    UIDrawHealthBar();
    UIDrawWaveInfo();
    UIDrawControls();
    UIDrawRoomIndicator();
    UIDrawFPS();
    
    bool allDead = true;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            allDead = false;
            break;
        }
    }
    
    if (allDead && waveComplete && currentWave < 10) {
        const char *title = TextFormat("WAVE %d COMPLETE!", currentWave);
        const char *subtitle = TextFormat("Starting Wave %d...", currentWave + 1);
        UIDrawCenterMessage(title, subtitle, NULL, (Color){ 100, 255, 100, 255 }, 48);
    }
    
    if (allDead && currentWave == 10) {
        UIDrawCenterMessage(
            "VICTORY!", 
            "All 10 waves defeated!", 
            "Press R to restart",
            (Color){ 255, 220, 100, 255 },
            64
        );
    }
}

void StartWave(int wave) {
    currentWave = wave;
    enemiesInWave = wave;
    waveComplete = false;
    waveTransitionTimer = 0.0f;
    
    player.pos = playerSpawnPoint;
    player.velocity = (Vector2){ 0.0f, 0.0f };
    player.knockback = (Vector2){ 0.0f, 0.0f };
    player.health = PLAYER_MAX_HEALTH;
    player.hasSword = false;
    player.isAttacking = false;
    player.attackTimer = 0.0f;
    player.attackCooldown = 0.0f;
    player.invulnTimer = 1.0f;
    
    sword.active = true;
    sword.pos = swordSpawnPoint;
    sword.respawnTimer = 0.0f;
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].active = false;
    }
    
    for (int i = 0; i < enemiesInWave; i++) {
        float angle = (2.0f * PI / enemiesInWave) * i;
        float radius = 250.0f;
        Vector2 pos = {
            playerSpawnPoint.x + cosf(angle) * radius,
            playerSpawnPoint.y + sinf(angle) * radius
        };
        EnemyInit(&enemies[i], pos, i, enemiesInWave);
        enemies[i].roomX = roomSystem.currentRoomX;
        enemies[i].roomY = roomSystem.currentRoomY;
    }
    
    ParticlesBurst(playerSpawnPoint, 30, (Color){ 100, 200, 255, 200 }, 
                  PTYPE_SPARK, 150.0f, 250.0f);
}

void RoomSystemInit(void) {
    for (int x = 0; x < MAX_ROOMS_X; x++) {
        for (int y = 0; y < MAX_ROOMS_Y; y++) {
            Room *room = &roomSystem.rooms[x][y];
            room->x = x;
            room->y = y;
            room->visited = false;
            
            int colorVariation = (x + y) % 3;
            if (colorVariation == 0) {
                room->tint = (Color){ 25, 30, 40, 255 };
            } else if (colorVariation == 1) {
                room->tint = (Color){ 20, 28, 38, 255 };
            } else {
                room->tint = (Color){ 22, 32, 42, 255 };
            }
            
            room->doorways[DIR_NORTH].center = (Vector2){ 
                ROOM_X + ROOM_WIDTH * 0.5f, 
                ROOM_Y 
            };
            room->doorways[DIR_NORTH].direction = DIR_NORTH;
            room->doorways[DIR_NORTH].active = (y > 0);
            
            room->doorways[DIR_EAST].center = (Vector2){ 
                ROOM_X + ROOM_WIDTH, 
                ROOM_Y + ROOM_HEIGHT * 0.5f 
            };
            room->doorways[DIR_EAST].direction = DIR_EAST;
            room->doorways[DIR_EAST].active = (x < MAX_ROOMS_X - 1);
            
            room->doorways[DIR_SOUTH].center = (Vector2){ 
                ROOM_X + ROOM_WIDTH * 0.5f, 
                ROOM_Y + ROOM_HEIGHT 
            };
            room->doorways[DIR_SOUTH].direction = DIR_SOUTH;
            room->doorways[DIR_SOUTH].active = (y < MAX_ROOMS_Y - 1);
            
            room->doorways[DIR_WEST].center = (Vector2){ 
                ROOM_X, 
                ROOM_Y + ROOM_HEIGHT * 0.5f 
            };
            room->doorways[DIR_WEST].direction = DIR_WEST;
            room->doorways[DIR_WEST].active = (x > 0);
        }
    }
    
    roomSystem.currentRoomX = 0;
    roomSystem.currentRoomY = 0;
    roomSystem.previousRoomX = 0;
    roomSystem.previousRoomY = 0;
    roomSystem.rooms[roomSystem.currentRoomX][roomSystem.currentRoomY].visited = true;
    
    roomSystem.isTransitioning = false;
    roomSystem.transitionTimer = 0.0f;
    roomSystem.pauseTimer = 0.0f;
    roomSystem.transitionOffset = (Vector2){ 0.0f, 0.0f };
    roomSystem.cameraOffset = (Vector2){ 0.0f, 0.0f };
}

Room* GetCurrentRoom(void) {
    return &roomSystem.rooms[roomSystem.currentRoomX][roomSystem.currentRoomY];
}

Vector2 GetCameraOffset(void) {
    return roomSystem.cameraOffset;
}

void CheckDoorwayCollision(void) {
    if (roomSystem.isTransitioning) return;
    
    Room *currentRoom = GetCurrentRoom();
    
    for (int i = 0; i < 4; i++) {
        Doorway *door = &currentRoom->doorways[i];
        if (!door->active) continue;
        
        float playerRadius = player.radius;
        bool collision = false;
        
        if (door->direction == DIR_NORTH) {
            if (player.pos.y <= ROOM_Y + playerRadius + 10.0f &&
                player.pos.x >= door->center.x - DOORWAY_WIDTH * 0.5f &&
                player.pos.x <= door->center.x + DOORWAY_WIDTH * 0.5f) {
                collision = true;
            }
        } else if (door->direction == DIR_SOUTH) {
            if (player.pos.y >= ROOM_Y + ROOM_HEIGHT - playerRadius - 10.0f &&
                player.pos.x >= door->center.x - DOORWAY_WIDTH * 0.5f &&
                player.pos.x <= door->center.x + DOORWAY_WIDTH * 0.5f) {
                collision = true;
            }
        } else if (door->direction == DIR_WEST) {
            if (player.pos.x <= ROOM_X + playerRadius + 10.0f &&
                player.pos.y >= door->center.y - DOORWAY_HEIGHT * 0.5f &&
                player.pos.y <= door->center.y + DOORWAY_HEIGHT * 0.5f) {
                collision = true;
            }
        } else if (door->direction == DIR_EAST) {
            if (player.pos.x >= ROOM_X + ROOM_WIDTH - playerRadius - 10.0f &&
                player.pos.y >= door->center.y - DOORWAY_HEIGHT * 0.5f &&
                player.pos.y <= door->center.y + DOORWAY_HEIGHT * 0.5f) {
                collision = true;
            }
        }
        
        if (collision) {
            roomSystem.previousRoomX = roomSystem.currentRoomX;
            roomSystem.previousRoomY = roomSystem.currentRoomY;
            
            roomSystem.isTransitioning = true;
            roomSystem.transitionTimer = 0.0f;
            roomSystem.pauseTimer = 0.0f;
            roomSystem.transitionDirection = door->direction;
            
            switch (door->direction) {
                case DIR_NORTH:
                    roomSystem.currentRoomY--;
                    player.pos.y = ROOM_Y + ROOM_HEIGHT - 60.0f;
                    break;
                case DIR_EAST:
                    roomSystem.currentRoomX++;
                    player.pos.x = ROOM_X + 60.0f;
                    break;
                case DIR_SOUTH:
                    roomSystem.currentRoomY++;
                    player.pos.y = ROOM_Y + 60.0f;
                    break;
                case DIR_WEST:
                    roomSystem.currentRoomX--;
                    player.pos.x = ROOM_X + ROOM_WIDTH - 60.0f;
                    break;
            }
            
            roomSystem.rooms[roomSystem.currentRoomX][roomSystem.currentRoomY].visited = true;
            
            ParticlesBurst(door->center, 20, (Color){ 100, 150, 255, 200 }, 
                          PTYPE_SPARK, 80.0f, 150.0f);
            
            break;
        }
    }
}

void RoomSystemUpdate(float dt) {
    Vector2 baseOffset = {
        -roomSystem.currentRoomX * ROOM_WIDTH,
        -roomSystem.currentRoomY * ROOM_HEIGHT
    };
    
    if (roomSystem.isTransitioning) {
        if (roomSystem.pauseTimer < TRANSITION_PAUSE) {
            roomSystem.pauseTimer += dt;
            roomSystem.cameraOffset.x = -roomSystem.previousRoomX * ROOM_WIDTH;
            roomSystem.cameraOffset.y = -roomSystem.previousRoomY * ROOM_HEIGHT;
            return;
        }
        
        roomSystem.transitionTimer += dt;
        
        float totalTime = TRANSITION_DURATION;
        float progress = roomSystem.transitionTimer / totalTime;
        if (progress > 1.0f) progress = 1.0f;
        
        float easedProgress = progress < 0.5f 
            ? 4.0f * progress * progress * progress
            : 1.0f - powf(-2.0f * progress + 2.0f, 3.0f) / 2.0f;
        
        Vector2 fromOffset = {
            -roomSystem.previousRoomX * ROOM_WIDTH,
            -roomSystem.previousRoomY * ROOM_HEIGHT
        };
        
        Vector2 toOffset = {
            -roomSystem.currentRoomX * ROOM_WIDTH,
            -roomSystem.currentRoomY * ROOM_HEIGHT
        };
        
        roomSystem.cameraOffset.x = fromOffset.x + (toOffset.x - fromOffset.x) * easedProgress;
        roomSystem.cameraOffset.y = fromOffset.y + (toOffset.y - fromOffset.y) * easedProgress;
        
        if (roomSystem.transitionTimer >= totalTime) {
            roomSystem.isTransitioning = false;
            roomSystem.transitionTimer = 0.0f;
            roomSystem.pauseTimer = 0.0f;
            roomSystem.cameraOffset = baseOffset;
        }
    } else {
        roomSystem.cameraOffset = baseOffset;
    }
}

static void DrawDoorway(Doorway *door) {
    Color doorColor = (Color){ 100, 120, 150, 255 };
    Color doorGlow = (Color){ 120, 150, 200, 100 };
    
    if (door->direction == DIR_NORTH || door->direction == DIR_SOUTH) {
        float x = door->center.x - DOORWAY_WIDTH * 0.5f;
        float y = door->center.y - 8.0f;
        
        DrawRectangle((int)(x - 5.0f), (int)(y - 2.0f), 
                     (int)(DOORWAY_WIDTH + 10.0f), 20, doorGlow);
        DrawRectangle((int)x, (int)y, (int)DOORWAY_WIDTH, 16, doorColor);
        DrawRectangle((int)x, (int)y, (int)DOORWAY_WIDTH, 4, Fade(WHITE, 0.3f));
    } else {
        float x = door->center.x - 8.0f;
        float y = door->center.y - DOORWAY_HEIGHT * 0.5f;
        
        DrawRectangle((int)(x - 2.0f), (int)(y - 5.0f), 20, 
                     (int)(DOORWAY_HEIGHT + 10.0f), doorGlow);
        DrawRectangle((int)x, (int)y, 16, (int)DOORWAY_HEIGHT, doorColor);
        DrawRectangle((int)x, (int)y, 4, (int)DOORWAY_HEIGHT, Fade(WHITE, 0.3f));
    }
}

void RoomSystemDraw(void) {
    for (int x = 0; x < MAX_ROOMS_X; x++) {
        for (int y = 0; y < MAX_ROOMS_Y; y++) {
            Room *room = &roomSystem.rooms[x][y];
            
            float roomPosX = ROOM_X + (x * ROOM_WIDTH);
            float roomPosY = ROOM_Y + (y * ROOM_HEIGHT);
            
            DrawRectangle((int)roomPosX, (int)roomPosY, (int)ROOM_WIDTH, 
                         (int)ROOM_HEIGHT, room->tint);
            
            DrawRectangleLinesEx(
                (Rectangle){ roomPosX - 4, roomPosY - 4, 
                            ROOM_WIDTH + 8, ROOM_HEIGHT + 8 },
                4.0f,
                (Color){ 60, 70, 90, 255 }
            );
            
            for (int i = 0; i < 4; i++) {
                Doorway *door = &room->doorways[i];
                if (!door->active) continue;
                
                Vector2 originalCenter = door->center;
                door->center.x += (roomPosX - ROOM_X);
                door->center.y += (roomPosY - ROOM_Y);
                DrawDoorway(door);
                door->center = originalCenter;
            }
        }
    }
}