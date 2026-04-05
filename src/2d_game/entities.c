#include "entities.h"
#include "systems.h"
#include <math.h>

/*
 * ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

// Get the visual offset for a room in the grid
static Vector2 GetRoomOffset(int roomX, int roomY) {
    return (Vector2){
        roomX * ROOM_WIDTH,
        roomY * ROOM_HEIGHT
    };
}

float Clamp(float v, float min, float max) {
    return v < min ? min : (v > max ? max : v);
}

float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

float NormalizeAngle(float angle) {
    while (angle > PI) angle -= 2.0f * PI;
    while (angle < -PI) angle += 2.0f * PI;
    return angle;
}

Vector2 Vec2Add(Vector2 a, Vector2 b) {
    return (Vector2){ a.x + b.x, a.y + b.y };
}

Vector2 Vec2Sub(Vector2 a, Vector2 b) {
    return (Vector2){ a.x - b.x, a.y - b.y };
}

Vector2 Vec2Scale(Vector2 v, float s) {
    return (Vector2){ v.x * s, v.y * s };
}

float Vec2Length(Vector2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

Vector2 Vec2Normalize(Vector2 v) {
    float len = Vec2Length(v);
    if (len > 0.0f) {
        return (Vector2){ v.x / len, v.y / len };
    }
    return (Vector2){ 0.0f, 0.0f };
}

float Vec2Distance(Vector2 a, Vector2 b) {
    return Vec2Length(Vec2Sub(b, a));
}

/*
 * ============================================================================
 * COLLISION & PHYSICS
 * ============================================================================
 */

bool CircleCollision(Vector2 p1, float r1, Vector2 p2, float r2) {
    return Vec2Distance(p1, p2) < (r1 + r2);
}

void ResolveCircleCollision(Vector2 *pos1, float r1, Vector2 *vel1, 
                            Vector2 *pos2, float r2, Vector2 *vel2) {
    Vector2 delta = Vec2Sub(*pos2, *pos1);
    float dist = Vec2Length(delta);
    
    if (dist < r1 + r2 && dist > 0.0f) {
        Vector2 normal = Vec2Normalize(delta);
        float overlap = (r1 + r2) - dist;
        
        // Push entities apart equally
        Vector2 separation = Vec2Scale(normal, overlap * 0.5f);
        *pos1 = Vec2Sub(*pos1, separation);
        *pos2 = Vec2Add(*pos2, separation);
    }
}

void ApplyKnockback(Vector2 *knockback, Vector2 direction, float strength) {
    Vector2 impulse = Vec2Scale(Vec2Normalize(direction), strength);
    *knockback = Vec2Add(*knockback, impulse);
}

/*
 * ============================================================================
 * PLAYER SYSTEM
 * ============================================================================
 */

void PlayerInit(void) {
    playerSpawnPoint = (Vector2){ 
        ROOM_X + ROOM_WIDTH * 0.5f, 
        ROOM_Y + ROOM_HEIGHT * 0.5f 
    };
    
    player.pos = playerSpawnPoint;
    player.velocity = (Vector2){ 0.0f, 0.0f };
    player.knockback = (Vector2){ 0.0f, 0.0f };
    player.health = PLAYER_MAX_HEALTH;
    player.maxHealth = PLAYER_MAX_HEALTH;
    player.facing = 0;
    player.hasSword = false;
    player.isAttacking = false;
    player.attackTimer = 0.0f;
    player.attackCooldown = 0.0f;
    player.swordAngle = 0.0f;
    player.animTimer = 0.0f;
    player.animFrame = 0;
    player.moving = false;
    player.invulnTimer = 0.0f;
    player.radius = PLAYER_SIZE * 0.85f;
}

void PlayerRespawn(void) {
    // Reset position and physics
    player.pos = playerSpawnPoint;
    player.velocity = (Vector2){ 0.0f, 0.0f };
    player.knockback = (Vector2){ 0.0f, 0.0f };
    player.health = PLAYER_MAX_HEALTH;
    
    // Respawn sword if player had it
    if (player.hasSword) {
        sword.active = true;
        sword.pos = swordSpawnPoint;
        sword.respawnTimer = 0.0f;
        ParticlesBurst(swordSpawnPoint, 15, (Color){ 255, 220, 100, 255 }, 
                      PTYPE_SPARK, 80.0f, 150.0f);
    }
    
    // Reset combat state
    player.hasSword = false;
    player.isAttacking = false;
    player.attackTimer = 0.0f;
    player.attackCooldown = 0.0f;
    player.invulnTimer = 1.0f;
    
    ParticlesBurst(player.pos, 20, (Color){ 100, 200, 255, 200 }, 
                  PTYPE_SPARK, 100.0f, 200.0f);
}

void PlayerUpdate(float dt) {
    // Update timers
    if (player.attackCooldown > 0.0f) player.attackCooldown -= dt;
    if (player.invulnTimer > 0.0f) player.invulnTimer -= dt;
    
    // Handle attack animation
    if (player.isAttacking) {
        player.attackTimer += dt;
        if (player.attackTimer >= SWORD_SWING_DURATION) {
            player.isAttacking = false;
            player.attackTimer = 0.0f;
        }
        
        // Animate sword swing from -90° to +90°
        float progress = player.attackTimer / SWORD_SWING_DURATION;
        player.swordAngle = -90.0f + progress * 180.0f;
    }
    
    // Handle movement input
    Vector2 input = { 0.0f, 0.0f };
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) input.x -= 1.0f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input.x += 1.0f;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) input.y -= 1.0f;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) input.y += 1.0f;
    
    player.moving = (input.x != 0.0f || input.y != 0.0f);
    
    if (player.moving) {
        // Normalize diagonal movement
        input = Vec2Normalize(input);
        player.velocity = Vec2Scale(input, PLAYER_SPEED);
        
        // Update facing direction (prioritize horizontal movement)
        if (fabsf(input.x) > fabsf(input.y)) {
            player.facing = input.x > 0.0f ? 0 : 2;
        } else {
            player.facing = input.y > 0.0f ? 1 : 3;
        }
        
        // Update animation
        player.animTimer += dt;
        if (player.animTimer >= ANIM_SPEED) {
            player.animTimer -= ANIM_SPEED;
            player.animFrame = (player.animFrame + 1) % 4;
        }
    } else {
        player.velocity = (Vector2){ 0.0f, 0.0f };
        player.animFrame = 0;
        player.animTimer = 0.0f;
    }
    
    // Apply knockback physics
    player.pos = Vec2Add(player.pos, Vec2Scale(player.knockback, dt));
    player.knockback = Vec2Scale(player.knockback, 1.0f - (KNOCKBACK_DECAY * dt));
    
    // Stop small knockback values
    if (Vec2Length(player.knockback) < 10.0f) {
        player.knockback = (Vector2){ 0.0f, 0.0f };
    }
    
    // Apply movement
    player.pos = Vec2Add(player.pos, Vec2Scale(player.velocity, dt));
    
    // Keep player within room bounds
    player.pos.x = Clamp(player.pos.x, ROOM_X + player.radius, 
                         ROOM_X + ROOM_WIDTH - player.radius);
    player.pos.y = Clamp(player.pos.y, ROOM_Y + player.radius, 
                         ROOM_Y + ROOM_HEIGHT - player.radius);
    
    // Handle sword attack input
    if ((IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) && 
        player.hasSword && player.attackCooldown <= 0.0f && !player.isAttacking) {
        
        player.isAttacking = true;
        player.attackTimer = 0.0f;
        player.attackCooldown = SWORD_ATTACK_COOLDOWN;
        player.swordAngle = -90.0f;
        
        ParticlesBurst(player.pos, 5, (Color){ 200, 200, 255, 200 }, 
                      PTYPE_SPARK, 50.0f, 100.0f);
        
        // Check for enemy hits (only in current room)
        bool hitEnemy = false;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            Enemy *e = &enemies[i];
            if (!e->active) continue;
            if (e->roomX != roomSystem.currentRoomX || 
                e->roomY != roomSystem.currentRoomY) continue;
            
            float dist = Vec2Distance(player.pos, e->pos);
            if (dist < SWORD_ATTACK_RANGE) {
                hitEnemy = true;
                e->health -= SWORD_DAMAGE;
                
                Vector2 knockbackDir = Vec2Sub(e->pos, player.pos);
                ApplyKnockback(&e->knockback, knockbackDir, 
                              KNOCKBACK_STRENGTH * SWORD_KNOCKBACK_MULTIPLIER);
                
                ParticlesBurst(e->pos, 15, (Color){ 200, 50, 50, 255 }, 
                              PTYPE_BLOOD, 80.0f, 150.0f);
                
                if (e->health <= 0.0f) {
                    e->active = false;
                    ParticlesBurst(e->pos, 30, (Color){ 200, 50, 50, 255 }, 
                                  PTYPE_BLOOD, 100.0f, 250.0f);
                }
            }
        }
        
        if (hitEnemy) {
            PlaySound(hitSound);
        }
    }
    
    // Handle sword pickup (only in current room)
    if (!player.hasSword && sword.active) {
        if (sword.roomX == roomSystem.currentRoomX && 
            sword.roomY == roomSystem.currentRoomY) {
            if (Vec2Distance(player.pos, sword.pos) < SWORD_PICKUP_RADIUS) {
                player.hasSword = true;
                sword.active = false;
                sword.respawnTimer = 0.0f;
                ParticlesBurst(sword.pos, 20, (Color){ 255, 220, 100, 255 }, 
                              PTYPE_SPARK, 100.0f, 200.0f);
            }
        }
    }
    
    // Handle death
    if (player.health <= 0.0f) {
        PlayerRespawn();
    }
}

void PlayerDraw(Texture2D spriteSheet) {
    Vector2 roomOffset = GetRoomOffset(roomSystem.currentRoomX, 
                                       roomSystem.currentRoomY);
    
    // Animation frames (from sprite sheet)
    Rectangle frames[4] = {
        { 1.0f, 2.0f, 13.0f, 13.0f },
        { 17.0f, 2.0f, 13.0f, 13.0f },
        { 33.0f, 2.0f, 13.0f, 13.0f },
        { 49.0f, 2.0f, 13.0f, 13.0f }
    };
    
    int frameIdx = player.animFrame % 4;
    Rectangle src = frames[frameIdx];
    
    // Flip sprite for left-facing
    if (player.facing == 2) {
        src.width *= -1.0f;
    }
    
    Rectangle dst = {
        player.pos.x + roomOffset.x,
        player.pos.y + roomOffset.y,
        PLAYER_SIZE * 2.0f,
        PLAYER_SIZE * 2.0f
    };
    Vector2 origin = { PLAYER_SIZE, PLAYER_SIZE };
    
    // Apply invulnerability flash effect
    Color tint = WHITE;
    if (player.invulnTimer > 0.0f) {
        float alpha = sinf(gameTime * 30.0f) * 0.5f + 0.5f;
        tint = Fade(WHITE, alpha);
    }
    
    DrawTexturePro(spriteSheet, src, dst, origin, 0.0f, tint);
    
    // Draw sword if equipped
    if (player.hasSword) {
        float baseAngle = player.facing * 90.0f;
        float angle = baseAngle;
        
        if (player.isAttacking) {
            angle = baseAngle + player.swordAngle;
        }
        
        Vector2 swordOffset = {
            cosf(angle * DEG2RAD) * (PLAYER_SIZE * 0.8f),
            sinf(angle * DEG2RAD) * (PLAYER_SIZE * 0.8f)
        };
        Vector2 swordPos = Vec2Add(player.pos, swordOffset);
        swordPos.x += roomOffset.x;
        swordPos.y += roomOffset.y;
        
        // Draw sword blade
        Rectangle swordRect = { swordPos.x, swordPos.y, 8.0f, 35.0f };
        Vector2 swordOrigin = { 4.0f, 0.0f };
        DrawRectanglePro(swordRect, swordOrigin, angle, 
                        (Color){ 180, 180, 200, 255 });
        
        // Draw sword highlight
        DrawRectanglePro(
            (Rectangle){ swordPos.x + 2.0f, swordPos.y, 2.0f, 35.0f },
            swordOrigin,
            angle,
            Fade(WHITE, 0.6f)
        );
    }
}

/*
 * ============================================================================
 * ENEMY SYSTEM
 * ============================================================================
 */

void EnemyInit(Enemy *e, Vector2 pos, int index, int totalEnemies) {
    e->pos = pos;
    e->velocity = (Vector2){ 0.0f, 0.0f };
    e->knockback = (Vector2){ 0.0f, 0.0f };
    e->health = ENEMY_HEALTH;
    e->maxHealth = ENEMY_HEALTH;
    e->facing = 0;
    e->active = true;
    e->isAttacking = false;
    e->attackTimer = 0.0f;
    e->attackCooldown = 0.0f;
    e->animTimer = 0.0f;
    e->animFrame = 0;
    e->targetPos = e->pos;
    e->stateTimer = -((float)index * 0.5f);  // Stagger AI updates
    e->radius = ENEMY_SIZE * 1.2f;
    
    // Initialize orbiting behavior
    e->preferredAngle = (2.0f * PI / totalEnemies) * index;
    e->circleRadius = ENEMY_PLAYER_MIN_DISTANCE + 10.0f;
}

void EnemyUpdate(Enemy *e, float dt) {
    if (!e->active) return;
    
    // Only update enemies in current room
    if (e->roomX != roomSystem.currentRoomX || 
        e->roomY != roomSystem.currentRoomY) {
        return;
    }
    
    // Update timers
    if (e->attackCooldown > 0.0f) e->attackCooldown -= dt;
    e->stateTimer += dt;
    
    // Skip update during initial delay
    if (e->stateTimer < 0.0f) return;
    
    // Calculate distance to player
    Vector2 toPlayer = Vec2Sub(player.pos, e->pos);
    float distToPlayer = Vec2Length(toPlayer);
    
    // Update orbit angle with slight variation
    float orbitSpeed = 0.5f + sinf(e->stateTimer * 0.3f) * 0.2f;
    e->preferredAngle += dt * orbitSpeed;
    
    // Calculate ideal orbital position
    Vector2 idealPos = {
        player.pos.x + cosf(e->preferredAngle) * e->circleRadius,
        player.pos.y + sinf(e->preferredAngle) * e->circleRadius
    };
    
    Vector2 toIdeal = Vec2Sub(idealPos, e->pos);
    float distToIdeal = Vec2Length(toIdeal);
    
    // Move toward ideal position
    if (distToIdeal > 5.0f) {
        Vector2 dir = Vec2Normalize(toIdeal);
        e->velocity = Vec2Scale(dir, ENEMY_SPEED);
        
        // Update facing direction
        if (fabsf(dir.x) > fabsf(dir.y)) {
            e->facing = dir.x > 0.0f ? 0 : 2;
        } else {
            e->facing = dir.y > 0.0f ? 1 : 3;
        }
        
        // Update animation
        e->animTimer += dt;
        if (e->animTimer >= ANIM_SPEED) {
            e->animTimer -= ANIM_SPEED;
            e->animFrame = (e->animFrame + 1) % 4;
        }
    } else {
        e->velocity = (Vector2){ 0.0f, 0.0f };
        e->animFrame = 0;
    }
    
    // Apply knockback physics
    e->pos = Vec2Add(e->pos, Vec2Scale(e->knockback, dt));
    e->knockback = Vec2Scale(e->knockback, 1.0f - (KNOCKBACK_DECAY * dt));
    
    if (Vec2Length(e->knockback) < 10.0f) {
        e->knockback = (Vector2){ 0.0f, 0.0f };
    }
    
    // Apply movement
    e->pos = Vec2Add(e->pos, Vec2Scale(e->velocity, dt));
    
    // Enemy separation (only with enemies in same room)
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *other = &enemies[i];
        if (other == e || !other->active) continue;
        if (other->roomX != e->roomX || other->roomY != e->roomY) continue;
        
        float dist = Vec2Distance(e->pos, other->pos);
        if (dist < ENEMY_SEPARATION_RADIUS && dist > 0.0f) {
            Vector2 away = Vec2Sub(e->pos, other->pos);
            Vector2 separation = Vec2Scale(Vec2Normalize(away), 
                                          (ENEMY_SEPARATION_RADIUS - dist) * 0.5f);
            e->pos = Vec2Add(e->pos, separation);
        }
    }
    
    // Attack player if in range
    if (distToPlayer < ENEMY_ATTACK_RANGE) {
        if (e->attackCooldown <= 0.0f && player.invulnTimer <= 0.0f) {
            player.health -= ENEMY_DAMAGE;
            e->attackCooldown = ENEMY_ATTACK_COOLDOWN;
            
            Vector2 knockbackDir = Vec2Sub(player.pos, e->pos);
            ApplyKnockback(&player.knockback, knockbackDir, 
                          KNOCKBACK_STRENGTH * PLAYER_KNOCKBACK_RESISTANCE);
            
            ParticlesBurst(player.pos, 10, (Color){ 200, 50, 50, 255 }, 
                          PTYPE_BLOOD, 60.0f, 120.0f);
        }
    }
    
    // Keep enemy within room bounds
    e->pos.x = Clamp(e->pos.x, ROOM_X + e->radius, 
                     ROOM_X + ROOM_WIDTH - e->radius);
    e->pos.y = Clamp(e->pos.y, ROOM_Y + e->radius, 
                     ROOM_Y + ROOM_HEIGHT - e->radius);
}

void EnemyDraw(Enemy *e, Texture2D spriteSheet) {
    if (!e->active) return;
    
    Vector2 roomOffset = GetRoomOffset(e->roomX, e->roomY);
    
    // Animation frames (from sprite sheet)
    Rectangle frames[4] = {
        { 1.0f, 2.0f, 13.0f, 13.0f },
        { 17.0f, 2.0f, 13.0f, 13.0f },
        { 33.0f, 2.0f, 13.0f, 13.0f },
        { 49.0f, 2.0f, 13.0f, 13.0f }
    };
    
    int frameIdx = e->animFrame % 4;
    Rectangle src = frames[frameIdx];
    
    // Flip sprite for left-facing
    if (e->facing == 2) {
        src.width *= -1.0f;
    }
    
    Rectangle dst = {
        e->pos.x + roomOffset.x,
        e->pos.y + roomOffset.y,
        ENEMY_SIZE * 2.0f,
        ENEMY_SIZE * 2.0f
    };
    Vector2 origin = { ENEMY_SIZE, ENEMY_SIZE };
    
    // Draw enemy with red tint
    DrawTexturePro(spriteSheet, src, dst, origin, 0.0f, 
                  (Color){ 255, 100, 100, 255 });
    
    // Draw health bar
    float healthRatio = e->health / e->maxHealth;
    float barWidth = 50.0f;
    float barHeight = 6.0f;
    Vector2 barPos = { 
        e->pos.x + roomOffset.x - barWidth * 0.5f, 
        e->pos.y + roomOffset.y - ENEMY_SIZE - 15.0f 
    };
    
    DrawRectangle((int)barPos.x, (int)barPos.y, (int)barWidth, (int)barHeight, 
                 (Color){ 50, 50, 50, 200 });
    DrawRectangle((int)barPos.x, (int)barPos.y, (int)(barWidth * healthRatio), 
                 (int)barHeight, (Color){ 200, 50, 50, 255 });
}

/*
 * ============================================================================
 * SWORD SYSTEM
 * ============================================================================
 */

void SwordInit(void) {
    swordSpawnPoint = (Vector2){ 
        ROOM_X + ROOM_WIDTH * 0.3f, 
        ROOM_Y + ROOM_HEIGHT * 0.3f 
    };
    
    sword.pos = swordSpawnPoint;
    sword.active = true;
    sword.bobPhase = 0.0f;
    sword.spinAngle = 0.0f;
    sword.respawnTimer = 0.0f;
    sword.roomX = roomSystem.currentRoomX;
    sword.roomY = roomSystem.currentRoomY;
}

void SwordUpdate(float dt) {
    if (sword.active) {
        // Animate floating effect
        sword.bobPhase += dt * 3.0f;
        sword.spinAngle += dt * 90.0f;
        if (sword.spinAngle >= 360.0f) sword.spinAngle -= 360.0f;
    }
}

void SwordDraw(void) {
    if (!sword.active) return;
    
    Vector2 roomOffset = GetRoomOffset(sword.roomX, sword.roomY);
    
    // Calculate bobbing offset
    float bobOffset = sinf(sword.bobPhase) * 8.0f;
    Vector2 drawPos = { 
        sword.pos.x + roomOffset.x, 
        sword.pos.y + roomOffset.y + bobOffset 
    };
    
    // Draw glow effect
    DrawCircle((int)drawPos.x, (int)drawPos.y, 25.0f, 
              Fade((Color){ 255, 220, 100, 100 }, 0.4f));
    DrawCircle((int)drawPos.x, (int)drawPos.y, 15.0f, 
              Fade((Color){ 255, 220, 100, 100 }, 0.6f));
    
    // Draw spinning sword
    Rectangle swordRect = { drawPos.x, drawPos.y, 8.0f, 35.0f };
    Vector2 origin = { 4.0f, 17.5f };
    DrawRectanglePro(swordRect, origin, sword.spinAngle, 
                    (Color){ 200, 200, 220, 255 });
    
    // Draw sword highlight
    DrawRectanglePro(
        (Rectangle){ drawPos.x + 2.0f, drawPos.y, 2.0f, 35.0f },
        origin,
        sword.spinAngle,
        Fade(WHITE, 0.8f)
    );
}