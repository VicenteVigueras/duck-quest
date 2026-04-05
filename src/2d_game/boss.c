#include "boss.h"
#include "combat.h"
#include "utils.h"
#include <math.h>
#include <string.h>

void BossInit(int roomId) {
    memset(&boss, 0, sizeof(Boss));
    boss.pos = (Vector2){ ROOM_X + ROOM_WIDTH * 0.5f, ROOM_Y + ROOM_HEIGHT * 0.35f };
    boss.health = BOSS_MAX_HEALTH;
    boss.maxHealth = BOSS_MAX_HEALTH;
    boss.radius = BOSS_SIZE;
    boss.phase = BOSS_PHASE_1;
    boss.active = true;
    boss.stateTimer = 1.5f; // Initial delay before first attack
    boss.attackCooldown = 0.0f;
    boss.currentPattern = 0;
    boss.minionCount = 0;
    (void)roomId;
}

static void BossSpawnMinions(void) {
    if (boss.minionCount >= 4) return;

    for (int i = 0; i < 2; i++) {
        int slot = -1;
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!enemies[j].active) { slot = j; break; }
        }
        if (slot < 0) break;

        float angle = ((float)GetRandomValue(0, 628)) / 100.0f;
        Vector2 pos = Vec2Add(boss.pos, (Vector2){ cosf(angle) * 100.0f, sinf(angle) * 100.0f });

        Enemy *e = &enemies[slot];
        memset(e, 0, sizeof(Enemy));
        e->pos = pos;
        e->active = true;
        e->roomId = dungeon.currentRoomId;
        e->type = ENEMY_SLIME;
        e->health = SLIME_HEALTH;
        e->maxHealth = SLIME_HEALTH;
        e->radius = SLIME_SIZE;
        e->circleRadius = 60.0f;

        boss.minionCount++;
    }
}

void BossUpdate(float dt) {
    if (!boss.active) return;

    // Flash timer
    if (boss.flashTimer > 0) boss.flashTimer -= dt;

    // Animation
    boss.animTimer += dt;

    // Phase transition
    if (boss.health <= BOSS_MAX_HEALTH * 0.5f && boss.phase == BOSS_PHASE_1) {
        boss.phase = BOSS_PHASE_2;
        ShakeScreen(10.0f, 0.5f, 50.0f);
        // Red flash particles
        for (int i = 0; i < 30; i++) {
            float angle = ((float)GetRandomValue(0, 628)) / 100.0f;
            float speed = 100.0f + (float)GetRandomValue(0, 150);
            if (particles.count < MAX_PARTICLES) {
                Particle *pt = &particles.pool[particles.count++];
                pt->pos = boss.pos;
                pt->vel = (Vector2){ cosf(angle) * speed, sinf(angle) * speed };
                pt->life = 0.6f;
                pt->maxLife = 0.6f;
                pt->size = 4.0f;
                pt->color = (Color){ 255, 50, 50, 255 };
                pt->type = PTYPE_SPARK;
            }
        }
    }

    // Death check
    if (boss.health <= 0.0f && boss.phase != BOSS_DYING && boss.phase != BOSS_DEAD) {
        boss.phase = BOSS_DYING;
        boss.stateTimer = 2.0f;
        ShakeScreen(15.0f, 1.0f, 55.0f);
        return;
    }

    // Death animation
    if (boss.phase == BOSS_DYING) {
        boss.stateTimer -= dt;
        boss.flashTimer = 0.05f; // Rapid flash
        if (boss.stateTimer <= 0.0f) {
            boss.phase = BOSS_DEAD;
            boss.active = false;
            // Massive explosion
            for (int i = 0; i < 60; i++) {
                float angle = ((float)GetRandomValue(0, 628)) / 100.0f;
                float speed = 50.0f + (float)GetRandomValue(0, 250);
                if (particles.count < MAX_PARTICLES) {
                    Particle *pt = &particles.pool[particles.count++];
                    pt->pos = boss.pos;
                    pt->vel = (Vector2){ cosf(angle) * speed, sinf(angle) * speed };
                    pt->life = 0.8f;
                    pt->maxLife = 0.8f;
                    pt->size = 3.0f + (float)GetRandomValue(0, 40) / 10.0f;
                    pt->color = (Color){
                        (unsigned char)(200 + GetRandomValue(0, 55)),
                        (unsigned char)(50 + GetRandomValue(0, 150)),
                        (unsigned char)GetRandomValue(20, 80),
                        255
                    };
                    pt->type = PTYPE_SPARK;
                }
            }
            // Drop key + hearts
            ItemType drops[] = { ITEM_KEY, ITEM_HEART_CONTAINER, ITEM_HEART };
            for (int i = 0; i < 3; i++) {
                Vector2 dropPos = Vec2Add(boss.pos, (Vector2){ (float)(i - 1) * 40.0f, 20.0f });
                for (int j = 0; j < MAX_WORLD_ITEMS; j++) {
                    if (!worldItems[j].active && !worldItems[j].collected) {
                        worldItems[j].type = drops[i];
                        worldItems[j].pos = dropPos;
                        worldItems[j].roomId = dungeon.currentRoomId;
                        worldItems[j].active = true;
                        worldItems[j].collected = false;
                        worldItems[j].bobPhase = gameTime + (float)j;
                        worldItems[j].price = 0;
                        break;
                    }
                }
            }
            return;
        }
        return;
    }

    // Knockback
    boss.pos = Vec2Add(boss.pos, Vec2Scale(boss.knockback, dt));
    boss.knockback = Vec2Scale(boss.knockback, 1.0f - (KNOCKBACK_DECAY * 0.5f * dt));
    if (Vec2Length(boss.knockback) < 5.0f) boss.knockback = (Vector2){ 0, 0 };

    // Keep in bounds
    float wt = WALL_THICKNESS * TILE_SIZE;
    boss.pos.x = Clamp(boss.pos.x, ROOM_X + wt + boss.radius, ROOM_X + ROOM_WIDTH - wt - boss.radius);
    boss.pos.y = Clamp(boss.pos.y, ROOM_Y + wt + boss.radius, ROOM_Y + ROOM_HEIGHT - wt - boss.radius);

    // Attack pattern logic
    float cooldownMult = (boss.phase == BOSS_PHASE_2) ? 0.6f : 1.0f;

    boss.stateTimer -= dt;
    if (boss.stateTimer > 0.0f) return;

    // Cycle through patterns
    switch (boss.currentPattern % 3) {
        case 0: { // CHARGE
            if (boss.chargeWindup <= 0.0f) {
                // Telegraph
                boss.chargeWindup = 0.8f * cooldownMult;
                boss.chargeTarget = player.pos;
                boss.flashTimer = boss.chargeWindup;
            } else {
                boss.chargeWindup -= dt;
                if (boss.chargeWindup <= 0.0f) {
                    // Execute charge
                    Vector2 dir = Vec2Normalize(Vec2Sub(boss.chargeTarget, boss.pos));
                    boss.velocity = Vec2Scale(dir, BOSS_CHARGE_SPEED);
                    boss.stateTimer = 0.4f;

                    // After charge, move to next pattern
                    boss.currentPattern++;
                    boss.chargeWindup = 0.0f;

                    // Damage player if close
                    if (Vec2Distance(boss.pos, player.pos) < boss.radius + player.radius + 20.0f) {
                        CombatApplyDamageToPlayer(BOSS_CONTACT_DAMAGE, boss.pos);
                    }
                }
                return;
            }
            break;
        }
        case 1: { // BONE SPRAY
            int numBones = 5;
            float baseAngle = atan2f(player.pos.y - boss.pos.y, player.pos.x - boss.pos.x);
            float spread = 0.8f; // radians total spread

            for (int i = 0; i < numBones; i++) {
                float angle = baseAngle - spread / 2.0f + spread * ((float)i / (numBones - 1));
                Vector2 vel = { cosf(angle) * BONE_PROJECTILE_SPEED, sinf(angle) * BONE_PROJECTILE_SPEED };
                ProjectileSpawn(boss.pos, vel, 6.0f, 15.0f, false, dungeon.currentRoomId,
                              (Color){ 200, 195, 180, 255 });
            }
            ShakeScreen(3.0f, 0.1f, 30.0f);
            boss.currentPattern++;
            boss.stateTimer = 1.5f * cooldownMult;
            break;
        }
        case 2: { // SLAM - ring of projectiles
            // Jump to center
            boss.pos = (Vector2){ ROOM_X + ROOM_WIDTH * 0.5f, ROOM_Y + ROOM_HEIGHT * 0.5f };
            int numProjectiles = 8;
            for (int i = 0; i < numProjectiles; i++) {
                float angle = (2.0f * PI / numProjectiles) * i;
                Vector2 vel = { cosf(angle) * PROJECTILE_SPEED, sinf(angle) * PROJECTILE_SPEED };
                ProjectileSpawn(boss.pos, vel, 7.0f, 12.0f, false, dungeon.currentRoomId,
                              (Color){ 200, 50, 50, 255 });
            }
            ShakeScreen(8.0f, 0.3f, 50.0f);
            boss.currentPattern++;
            boss.stateTimer = 2.0f * cooldownMult;

            // Phase 2: summon minions every other slam
            if (boss.phase == BOSS_PHASE_2 && (boss.currentPattern / 3) % 2 == 0) {
                BossSpawnMinions();
            }
            break;
        }
    }

    // Apply velocity (from charge)
    boss.pos = Vec2Add(boss.pos, Vec2Scale(boss.velocity, dt));
    boss.velocity = Vec2Scale(boss.velocity, 0.9f);
    if (Vec2Length(boss.velocity) < 10.0f) boss.velocity = (Vector2){ 0, 0 };

    // Contact damage
    if (Vec2Distance(boss.pos, player.pos) < boss.radius + player.radius) {
        CombatApplyDamageToPlayer(BOSS_CONTACT_DAMAGE * 0.5f, boss.pos);
    }
}

void BossDraw(void) {
    if (!boss.active && boss.phase != BOSS_DYING) return;

    // Boss body
    Color bodyColor = (boss.phase == BOSS_PHASE_2 || boss.phase == BOSS_DYING)
        ? (Color){ 180, 50, 50, 255 }
        : (Color){ 200, 195, 180, 255 };

    if (boss.flashTimer > 0.0f) {
        bodyColor = WHITE;
    }

    float bx = boss.pos.x;
    float by = boss.pos.y;
    float r = boss.radius;

    // Shadow
    DrawEllipse((int)bx, (int)(by + r * 0.8f), r * 1.2f, r * 0.4f,
               (Color){ 0, 0, 0, 80 });

    // Phase 2 glow
    if (boss.phase == BOSS_PHASE_2) {
        float pulse = sinf(gameTime * 4.0f) * 0.3f + 0.7f;
        DrawCircle((int)bx, (int)by, r * 1.3f * pulse,
                  (Color){ 200, 30, 30, (unsigned char)(40 * pulse) });
    }

    // Body (torso rectangle)
    DrawRectangle((int)(bx - r * 0.6f), (int)(by - r * 0.4f),
                 (int)(r * 1.2f), (int)(r * 1.0f), bodyColor);

    // Head
    DrawCircle((int)bx, (int)(by - r * 0.6f), r * 0.45f, bodyColor);

    // Crown
    Color crownColor = { 220, 180, 50, 255 };
    float crownY = by - r * 1.0f;
    DrawRectangle((int)(bx - r * 0.35f), (int)crownY, (int)(r * 0.7f), (int)(r * 0.15f), crownColor);
    // Crown spikes
    for (int i = 0; i < 3; i++) {
        float sx = bx - r * 0.25f + (float)i * r * 0.25f;
        DrawRectangle((int)(sx - 2), (int)(crownY - r * 0.2f), 5, (int)(r * 0.2f), crownColor);
    }

    // Eyes (red in phase 2)
    Color eyeColor = (boss.phase == BOSS_PHASE_2) ? (Color){ 255, 50, 50, 255 } : (Color){ 40, 30, 30, 255 };
    DrawCircle((int)(bx - r * 0.15f), (int)(by - r * 0.6f), 4, eyeColor);
    DrawCircle((int)(bx + r * 0.15f), (int)(by - r * 0.6f), 4, eyeColor);

    // Arms
    float armWave = sinf(gameTime * 3.0f) * 5.0f;
    DrawRectangle((int)(bx - r * 0.9f), (int)(by - r * 0.2f + armWave), (int)(r * 0.3f), (int)(r * 0.6f), bodyColor);
    DrawRectangle((int)(bx + r * 0.6f), (int)(by - r * 0.2f - armWave), (int)(r * 0.3f), (int)(r * 0.6f), bodyColor);

    // Health bar
    float barW = 200.0f;
    float barH = 12.0f;
    float barX = bx - barW / 2;
    float barY = by - r - 35.0f;
    float healthRatio = boss.health / boss.maxHealth;
    if (healthRatio < 0) healthRatio = 0;

    DrawRectangle((int)barX, (int)barY, (int)barW, (int)barH, (Color){ 30, 30, 30, 200 });
    Color hpColor = (boss.phase == BOSS_PHASE_2)
        ? (Color){ 200, 50, 50, 255 }
        : (Color){ 200, 160, 50, 255 };
    DrawRectangle((int)barX, (int)barY, (int)(barW * healthRatio), (int)barH, hpColor);
    DrawRectangleLinesEx((Rectangle){ barX, barY, barW, barH }, 1, (Color){ 100, 100, 100, 200 });

    const char *bossName = "BONE KING";
    int nameW = MeasureText(bossName, 14);
    DrawText(bossName, (int)(bx - nameW / 2), (int)(barY - 16), 14, (Color){ 220, 200, 180, 200 });
}
