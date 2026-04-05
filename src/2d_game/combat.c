#include "combat.h"
#include "utils.h"
#include "items.h"
#include <math.h>
#include <string.h>

/*
 * ============================================================================
 * SCREEN SHAKE
 * ============================================================================
 */

void ShakeScreen(float intensity, float duration, float frequency) {
    if (intensity > screenShake.intensity) {
        screenShake.intensity = intensity;
    }
    screenShake.duration = duration;
    screenShake.frequency = frequency;
}

void ScreenShakeUpdate(float dt) {
    if (screenShake.duration > 0.0f) {
        screenShake.duration -= dt;
        screenShake.intensity *= 0.92f;
        if (screenShake.duration <= 0.0f) {
            screenShake.intensity = 0.0f;
        }
    }
}

Vector2 ScreenShakeGetOffset(void) {
    if (screenShake.duration <= 0.0f) return (Vector2){ 0, 0 };
    float sx = sinf(gameTime * screenShake.frequency) * screenShake.intensity;
    float sy = cosf(gameTime * screenShake.frequency * 1.3f) * screenShake.intensity;
    return (Vector2){ sx, sy };
}

/*
 * ============================================================================
 * PROJECTILE SYSTEM
 * ============================================================================
 */

void ProjectileSpawn(Vector2 pos, Vector2 vel, float radius, float damage,
                     bool friendly, int roomId, Color color) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) {
            projectiles[i].pos = pos;
            projectiles[i].vel = vel;
            projectiles[i].radius = radius;
            projectiles[i].damage = damage;
            projectiles[i].lifetime = 3.0f;
            projectiles[i].active = true;
            projectiles[i].friendly = friendly;
            projectiles[i].roomId = roomId;
            projectiles[i].color = color;
            projectiles[i].rotation = 0.0f;
            projectiles[i].rotSpeed = 360.0f;
            return;
        }
    }
}

void ProjectilesUpdate(float dt) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *p = &projectiles[i];
        if (!p->active) continue;

        p->pos = Vec2Add(p->pos, Vec2Scale(p->vel, dt));
        p->rotation += p->rotSpeed * dt;
        p->lifetime -= dt;

        if (p->lifetime <= 0.0f) {
            p->active = false;
            continue;
        }

        // Bounds check (within room)
        float wt = WALL_THICKNESS * TILE_SIZE;
        if (p->pos.x < ROOM_X + wt || p->pos.x > ROOM_X + ROOM_WIDTH - wt ||
            p->pos.y < ROOM_Y + wt || p->pos.y > ROOM_Y + ROOM_HEIGHT - wt) {
            p->active = false;
            ParticleSystem *ps = &particles;
            (void)ps;
            continue;
        }

        // Collision with player (enemy projectiles)
        if (!p->friendly && p->roomId == dungeon.currentRoomId) {
            if (Vec2Distance(p->pos, player.pos) < p->radius + player.radius) {
                CombatApplyDamageToPlayer(p->damage, p->pos);
                p->active = false;
                continue;
            }
        }

        // Collision with enemies (friendly projectiles)
        if (p->friendly) {
            for (int j = 0; j < MAX_ENEMIES; j++) {
                Enemy *e = &enemies[j];
                if (!e->active || e->roomId != p->roomId) continue;
                if (Vec2Distance(p->pos, e->pos) < p->radius + e->radius) {
                    CombatApplyDamageToEnemy(e, p->damage, p->pos);
                    p->active = false;
                    break;
                }
            }
        }
    }
}

void ProjectilesDraw(void) {
    int currentRoom = dungeon.currentRoomId;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *p = &projectiles[i];
        if (!p->active || p->roomId != currentRoom) continue;

        // Square pixel projectile
        int sz = (int)(p->radius * 2.0f);
        DrawRectangle((int)(p->pos.x - p->radius), (int)(p->pos.y - p->radius),
                     sz, sz, p->color);
        // Inner highlight
        int inner = (int)(p->radius * 0.8f);
        DrawRectangle((int)(p->pos.x - inner / 2), (int)(p->pos.y - inner / 2),
                     inner, inner, (Color){ 255, 255, 255, 100 });
    }
}

/*
 * ============================================================================
 * DAMAGE APPLICATION
 * ============================================================================
 */

void CombatApplyDamageToEnemy(Enemy *e, float damage, Vector2 from) {
    e->health -= damage;
    e->flashTimer = 0.1f;

    Vector2 knockDir = Vec2Sub(e->pos, from);
    if (e->type != ENEMY_TURRET) {
        ApplyKnockback(&e->knockback, knockDir, KNOCKBACK_STRENGTH * SWORD_KNOCKBACK_MULTIPLIER);
    } else {
        ApplyKnockback(&e->knockback, knockDir, KNOCKBACK_STRENGTH * 0.3f);
    }

    // Particles
    Color bloodColor;
    switch (e->type) {
        case ENEMY_SLIME:    bloodColor = (Color){ 80, 200, 80, 255 }; break;
        case ENEMY_BAT:      bloodColor = (Color){ 150, 50, 150, 255 }; break;
        case ENEMY_SKELETON: bloodColor = (Color){ 200, 195, 180, 255 }; break;
        default:             bloodColor = (Color){ 200, 50, 50, 255 }; break;
    }

    // Use direct particle spawning
    for (int i = 0; i < 10; i++) {
        float angle = ((float)GetRandomValue(0, 628)) / 100.0f;
        float speed = 60.0f + (float)GetRandomValue(0, 100);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        float size = 2.0f + (float)GetRandomValue(0, 30) / 10.0f;
        float life = 0.3f + (float)GetRandomValue(0, 40) / 100.0f;

        if (particles.count < MAX_PARTICLES) {
            Particle *pt = &particles.pool[particles.count++];
            pt->pos = e->pos;
            pt->vel = vel;
            pt->life = life;
            pt->maxLife = life;
            pt->size = size;
            pt->color = bloodColor;
            pt->type = PTYPE_BLOOD;
        }
    }

    ShakeScreen(1.5f, 0.08f, 40.0f);
    hitFreezeTimer = 0.02f;

    if (e->health <= 0.0f) {
        e->active = false;
        player.inventory.enemiesKilled++;
        // Death burst
        for (int i = 0; i < 20; i++) {
            float angle = ((float)GetRandomValue(0, 628)) / 100.0f;
            float speed = 80.0f + (float)GetRandomValue(0, 150);
            Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
            float size = 2.0f + (float)GetRandomValue(0, 40) / 10.0f;
            float life = 0.4f + (float)GetRandomValue(0, 40) / 100.0f;

            if (particles.count < MAX_PARTICLES) {
                Particle *pt = &particles.pool[particles.count++];
                pt->pos = e->pos;
                pt->vel = vel;
                pt->life = life;
                pt->maxLife = life;
                pt->size = size;
                pt->color = bloodColor;
                pt->type = PTYPE_BLOOD;
            }
        }
        ShakeScreen(2.5f, 0.12f, 45.0f);

        // Drop items
        ItemDropFromEnemy(e->pos, e->roomId);
    }

    SetSoundVolume(hitSound, sfxVolume);
    PlaySound(hitSound);
}

void CombatApplyDamageToPlayer(float damage, Vector2 from) {
    if (player.invulnTimer > 0.0f) return;
    if (player.shieldTimer > 0.0f) {
        player.shieldTimer -= 0.5f;
        ShakeScreen(1.0f, 0.06f, 35.0f);
        return;
    }

    player.health -= damage;
    player.flashTimer = 0.12f;
    player.invulnTimer = 0.6f;

    Vector2 knockDir = Vec2Sub(player.pos, from);
    ApplyKnockback(&player.knockback, knockDir, KNOCKBACK_STRENGTH * PLAYER_KNOCKBACK_RESISTANCE);

    // Blood particles
    for (int i = 0; i < 8; i++) {
        float angle = ((float)GetRandomValue(0, 628)) / 100.0f;
        float speed = 50.0f + (float)GetRandomValue(0, 80);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        if (particles.count < MAX_PARTICLES) {
            Particle *pt = &particles.pool[particles.count++];
            pt->pos = player.pos;
            pt->vel = vel;
            pt->life = 0.4f;
            pt->maxLife = 0.4f;
            pt->size = 3.0f;
            pt->color = (Color){ 200, 50, 50, 255 };
            pt->type = PTYPE_BLOOD;
        }
    }

    ShakeScreen(2.0f, 0.1f, 45.0f);
}
