#include "boss.h"
#include "combat.h"
#include "utils.h"
#include <math.h>
#include <string.h>

/*
 * ============================================================================
 * THE MAD DUCK — a comically enraged pixel-art waterfowl
 * ============================================================================
 */

void BossInit(int roomId) {
    memset(&boss, 0, sizeof(Boss));
    boss.pos = (Vector2){ ROOM_X + ROOM_WIDTH * 0.5f, ROOM_Y + ROOM_HEIGHT * 0.35f };
    boss.health = BOSS_MAX_HEALTH;
    boss.maxHealth = BOSS_MAX_HEALTH;
    boss.radius = BOSS_SIZE;
    boss.phase = BOSS_PHASE_1;
    boss.active = true;
    boss.stateTimer = 1.5f;
    boss.attackCooldown = 0.0f;
    boss.currentPattern = 0;
    boss.minionCount = 0;
    (void)roomId;
}

// Spawn a small burst of feather particles at a given position
static void SpawnFeathers(Vector2 origin, int count, float speedMin, float speedMax) {
    for (int i = 0; i < count; i++) {
        if (particles.count >= MAX_PARTICLES) break;
        float angle = ((float)GetRandomValue(0, 628)) / 100.0f;
        float speed = speedMin + (float)GetRandomValue(0, (int)(speedMax - speedMin));
        Particle *pt = &particles.pool[particles.count++];
        pt->pos = origin;
        pt->vel = (Vector2){ cosf(angle) * speed, sinf(angle) * speed - 40.0f };
        pt->life = 0.9f;
        pt->maxLife = 0.9f;
        pt->size = 3.0f + (float)GetRandomValue(0, 20) / 10.0f;
        // Mostly white/yellow feathers with an occasional orange one
        int tint = GetRandomValue(0, 2);
        if (tint == 0)      pt->color = (Color){ 255, 245, 210, 255 }; // cream
        else if (tint == 1) pt->color = (Color){ 250, 220, 100, 255 }; // duck yellow
        else                pt->color = (Color){ 240, 160,  60, 255 }; // orange
        pt->type = PTYPE_DUST;
    }
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
        e->type = ENEMY_BAT; // angry duckling swarm (reused bat AI, flaps fast)
        e->health = BAT_HEALTH;
        e->maxHealth = BAT_HEALTH;
        e->radius = BAT_SIZE;

        // Puff of feathers where the minion appears
        SpawnFeathers(pos, 6, 60.0f, 140.0f);

        boss.minionCount++;
    }
}

void BossUpdate(float dt) {
    if (!boss.active) return;

    // Only update boss when player is in the boss room
    if (boss.phase != BOSS_DYING && boss.phase != BOSS_DEAD &&
        dungeon.currentRoomId != dungeon.bossRoomId) return;

    if (boss.flashTimer > 0) boss.flashTimer -= dt;
    boss.animTimer += dt;

    // Phase transition — duck gets angrier, feathers ruffled
    if (boss.health <= BOSS_MAX_HEALTH * 0.5f && boss.phase == BOSS_PHASE_1) {
        boss.phase = BOSS_PHASE_2;
        ShakeScreen(10.0f, 0.5f, 50.0f);
        SpawnFeathers(boss.pos, 30, 80.0f, 220.0f);
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
        boss.flashTimer = 0.05f;
        if (boss.stateTimer <= 0.0f) {
            boss.phase = BOSS_DEAD;
            boss.active = false;
            // Feather-and-yolk explosion
            for (int i = 0; i < 60; i++) {
                if (particles.count >= MAX_PARTICLES) break;
                float angle = ((float)GetRandomValue(0, 628)) / 100.0f;
                float speed = 50.0f + (float)GetRandomValue(0, 250);
                Particle *pt = &particles.pool[particles.count++];
                pt->pos = boss.pos;
                pt->vel = (Vector2){ cosf(angle) * speed, sinf(angle) * speed };
                pt->life = 0.8f;
                pt->maxLife = 0.8f;
                pt->size = 3.0f + (float)GetRandomValue(0, 40) / 10.0f;
                int tint = GetRandomValue(0, 3);
                if (tint == 0)      pt->color = (Color){ 255, 245, 210, 255 };
                else if (tint == 1) pt->color = (Color){ 250, 220, 100, 255 };
                else if (tint == 2) pt->color = (Color){ 240, 160,  60, 255 };
                else                pt->color = (Color){ 255, 255, 255, 255 };
                pt->type = PTYPE_SPARK;
            }
            // Drop hearts + coins (no key — player already has boss access)
            ItemType drops[] = { ITEM_HEART_CONTAINER, ITEM_HEART, ITEM_COIN };
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

    switch (boss.currentPattern % 3) {
        case 0: { // WADDLE CHARGE
            if (boss.chargeWindup <= 0.0f) {
                boss.chargeWindup = 0.8f * cooldownMult;
                boss.chargeTarget = player.pos;
                boss.flashTimer = boss.chargeWindup;
            } else {
                boss.chargeWindup -= dt;
                if (boss.chargeWindup <= 0.0f) {
                    Vector2 dir = Vec2Normalize(Vec2Sub(boss.chargeTarget, boss.pos));
                    boss.velocity = Vec2Scale(dir, BOSS_CHARGE_SPEED);
                    boss.stateTimer = 0.4f;

                    boss.currentPattern++;
                    boss.chargeWindup = 0.0f;

                    // Feather trail
                    SpawnFeathers(boss.pos, 8, 40.0f, 140.0f);

                    if (Vec2Distance(boss.pos, player.pos) < boss.radius + player.radius + 20.0f) {
                        CombatApplyDamageToPlayer(BOSS_CONTACT_DAMAGE, boss.pos);
                    }
                }
                return;
            }
            break;
        }
        case 1: { // FEATHER SPRAY
            int numFeathers = 5;
            float baseAngle = atan2f(player.pos.y - boss.pos.y, player.pos.x - boss.pos.x);
            float spread = 0.8f;

            for (int i = 0; i < numFeathers; i++) {
                float angle = baseAngle - spread / 2.0f + spread * ((float)i / (numFeathers - 1));
                Vector2 vel = { cosf(angle) * BONE_PROJECTILE_SPEED, sinf(angle) * BONE_PROJECTILE_SPEED };
                ProjectileSpawn(boss.pos, vel, 6.0f, 15.0f, false, dungeon.currentRoomId,
                              (Color){ 255, 240, 200, 255 });
            }
            // Puff of feathers on spray
            SpawnFeathers(boss.pos, 10, 50.0f, 160.0f);
            ShakeScreen(3.0f, 0.1f, 30.0f);
            boss.currentPattern++;
            boss.stateTimer = 1.5f * cooldownMult;
            break;
        }
        case 2: { // EGG SLAM — ring of eggs
            boss.pos = (Vector2){ ROOM_X + ROOM_WIDTH * 0.5f, ROOM_Y + ROOM_HEIGHT * 0.5f };
            int numProjectiles = 8;
            for (int i = 0; i < numProjectiles; i++) {
                float angle = (2.0f * PI / numProjectiles) * i;
                Vector2 vel = { cosf(angle) * PROJECTILE_SPEED, sinf(angle) * PROJECTILE_SPEED };
                ProjectileSpawn(boss.pos, vel, 9.0f, 12.0f, false, dungeon.currentRoomId,
                              (Color){ 250, 240, 220, 255 });
            }
            // Shell-crack dust
            SpawnFeathers(boss.pos, 14, 60.0f, 180.0f);
            ShakeScreen(8.0f, 0.3f, 50.0f);
            boss.currentPattern++;
            boss.stateTimer = 2.0f * cooldownMult;

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

/*
 * ============================================================================
 * BOSS DRAW — Mad Duck, pure 8-bit, all squares
 * ============================================================================
 *
 * Sprite grid (11 wide x 13 tall big-pixels). '.' = empty.
 *   Legend: Y=body, O=beak/feet, K=eye-pupil, W=eye-white,
 *           C=crown, G=gem, S=shade, H=highlight
 * ============================================================================
 */

static const char MAD_DUCK_SPRITE[13][11] = {
    // row 0 : crown spikes
    { '.', '.', '.', 'C', '.', 'C', '.', 'C', '.', '.', '.' },
    // row 1 : crown band
    { '.', '.', 'C', 'C', 'C', 'G', 'C', 'C', 'C', '.', '.' },
    // row 2 : head top + highlight
    { '.', '.', 'H', 'Y', 'Y', 'Y', 'Y', 'Y', 'S', '.', '.' },
    // row 3 : head eyes row
    { '.', '.', 'Y', 'W', 'K', 'Y', 'W', 'K', 'Y', '.', '.' },
    // row 4 : head with beak
    { '.', '.', 'Y', 'Y', 'Y', 'Y', 'Y', 'O', 'O', 'O', '.' },
    // row 5 : lower head + beak
    { '.', '.', 'Y', 'Y', 'Y', 'Y', 'Y', 'O', 'O', '.', '.' },
    // row 6 : neck
    { '.', '.', '.', 'Y', 'Y', 'Y', 'Y', '.', '.', '.', '.' },
    // row 7 : body top (wings start)
    { '.', 'S', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', '.', '.' },
    // row 8 : body mid (widest) + tail
    { 'S', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', '.' },
    // row 9 : body mid
    { 'S', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', '.' },
    // row 10: body bottom
    { '.', 'S', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'S', '.', '.' },
    // row 11: body base
    { '.', '.', 'S', 'S', 'S', 'S', 'S', 'S', '.', '.', '.' },
    // row 12: feet
    { '.', '.', 'O', 'O', '.', '.', 'O', 'O', '.', '.', '.' },
};

void BossDraw(void) {
    if (!boss.active && boss.phase != BOSS_DYING) return;
    if (dungeon.currentRoomId != dungeon.bossRoomId) return;

    bool phase2 = (boss.phase == BOSS_PHASE_2 || boss.phase == BOSS_DYING);

    // Palette — four colors, full 8-bit feel
    Color bodyColor  = phase2 ? (Color){ 230, 110,  60, 255 } : (Color){ 250, 210,  60, 255 };
    Color bodyShade  = phase2 ? (Color){ 170,  60,  30, 255 } : (Color){ 200, 150,  30, 255 };
    Color bodyHi     = phase2 ? (Color){ 255, 180, 100, 255 } : (Color){ 255, 240, 130, 255 };
    Color beakColor  = (Color){ 255, 140,  40, 255 };
    Color eyeWhite   = (Color){ 245, 245, 245, 255 };
    Color eyePupil   = phase2 ? (Color){ 220,  30,  30, 255 } : (Color){  20,  20,  30, 255 };
    Color crownColor = (Color){ 240, 205,  70, 255 };
    Color gemColor   = (Color){ 230,  70,  90, 255 };

    // Flash white when taking damage
    if (boss.flashTimer > 0.0f) {
        bodyColor = bodyShade = bodyHi = WHITE;
    }

    float bx = boss.pos.x;
    float by = boss.pos.y;
    float r  = boss.radius;

    // Each big-pixel is a square; size scales with boss radius
    float pf = r * 0.18f;
    int   p  = (int)pf;
    if (p < 4) p = 4;

    // Sprite dimensions
    int sw = 11 * p;
    int sh = 13 * p;

    // Waddle: tiny whole-sprite hop (1 big-pixel up/down, toggles every step)
    int step = ((int)(boss.animTimer * 4.0f)) & 1;
    int hop  = step ? -p : 0;
    // Phase 2: angry shake (random horizontal jitter of 1 pixel)
    int shake = 0;
    if (phase2) shake = (GetRandomValue(0, 1) == 0) ? -1 : 1;

    // Top-left anchor so the duck is centered on (bx, by)
    int x0 = (int)(bx - sw * 0.5f) + shake;
    int y0 = (int)(by - sh * 0.5f) + hop;

    // Shadow (stays put while duck hops)
    DrawRectangle((int)(bx - sw * 0.5f + p), (int)(by + sh * 0.45f),
                  sw - 2 * p, (p < 4 ? 4 : p), (Color){ 0, 0, 0, 90 });

    // Phase 2 rage aura (still a rectangle to keep the 8-bit look)
    if (boss.phase == BOSS_PHASE_2) {
        int pulse = (int)(sinf(gameTime * 4.0f) * 2.0f);
        DrawRectangle(x0 - p - pulse, y0 - p - pulse,
                      sw + 2 * (p + pulse), sh + 2 * (p + pulse),
                      (Color){ 255, 80, 40, 30 });
    }

    // Eye pupil tracking (offset in big-pixels, clamped to ±1)
    int pupilDx = 0, pupilDy = 0;
    {
        Vector2 toPlayer = Vec2Sub(player.pos, boss.pos);
        if (fabsf(toPlayer.x) > 40.0f) pupilDx = (toPlayer.x > 0) ? 1 : -1;
        if (toPlayer.y > 40.0f) pupilDy = 1;
    }

    // Beak open/close (swap pupils with mouth open is already handled by frame)
    bool beakOpen = (sinf(boss.animTimer * 4.0f) > 0.3f) || boss.chargeWindup > 0.0f;

    // Wing flap — alternate which big-pixel row of the body is "shadowed"
    // to simulate a wing moving up/down.  Purely integer: no floats.
    int flapPhase = ((int)(boss.animTimer * (phase2 ? 10.0f : 6.0f))) & 1;

    for (int sy = 0; sy < 13; sy++) {
        for (int sx = 0; sx < 11; sx++) {
            char code = MAD_DUCK_SPRITE[sy][sx];
            if (code == '.') continue;

            Color c;
            switch (code) {
                case 'Y': c = bodyColor; break;
                case 'S': c = bodyShade; break;
                case 'H': c = bodyHi; break;
                case 'O': c = beakColor; break;
                case 'W': c = eyeWhite; break;
                case 'K': c = eyePupil; break;
                case 'C': c = crownColor; break;
                case 'G': c = gemColor; break;
                default:  continue;
            }

            int dx = sx, dy = sy;

            // Pupil tracking: shift pupil pixels by ±1 big-pixel within eye
            if (code == 'K') {
                // Left eye pupil is at col 4, right is at col 7
                // Keep pupil inside the eye white area (cols 3/6 are white)
                if (sx == 4 && pupilDx < 0) dx = 3;
                if (sx == 7 && pupilDx < 0) dx = 6;
                // pupilDx>0 means pupil shifts right; it's already at the right edge
                if (pupilDy > 0) dy = sy + (pupilDy > 0 ? 0 : 0); // keep same row
            }

            // Beak swap: when closed, make the O at (row4, col9) into body
            if (!beakOpen && sy == 4 && sx == 9) continue;
            if (!beakOpen && sy == 5 && sx == 8) continue;

            // Wing flap: alternate shade pixels at body sides to fake flapping
            if ((code == 'S') && (sy == 7 || sy == 8) && flapPhase == 1) {
                // Brighten to body color briefly so the wing "lifts"
                c = bodyColor;
            }
            if ((code == 'Y') && sy == 8 && (sx == 1 || sx == 9) && flapPhase == 0) {
                // Lower-sweep: show shade at wing tips on the off-beat
                c = bodyShade;
            }

            DrawRectangle(x0 + dx * p, y0 + dy * p, p, p, c);
        }
    }

    // Charge telegraph: three little red exclamation squares above crown
    if (boss.chargeWindup > 0.0f) {
        int cs = (int)(sinf(boss.animTimer * 25.0f) * p * 0.5f);
        Color angryRed = { 240, 70, 70, 255 };
        DrawRectangle(x0 + 2 * p + cs, y0 - 2 * p, p, p, angryRed);
        DrawRectangle(x0 + 5 * p,      y0 - 3 * p, p, p, angryRed);
        DrawRectangle(x0 + 8 * p - cs, y0 - 2 * p, p, p, angryRed);
    }

    // ---- Health bar + name (unchanged styling, pixel-clean) ----
    float barW = 220.0f;
    float barH = 12.0f;
    float barX = bx - barW / 2;
    float barY = by - r - 60.0f;
    float healthRatio = boss.health / boss.maxHealth;
    if (healthRatio < 0) healthRatio = 0;

    DrawRectangle((int)barX, (int)barY, (int)barW, (int)barH, (Color){ 30, 30, 30, 200 });
    Color hpColor = phase2 ? (Color){ 220, 60, 50, 255 } : (Color){ 240, 200, 70, 255 };
    DrawRectangle((int)barX, (int)barY, (int)(barW * healthRatio), (int)barH, hpColor);
    DrawRectangleLinesEx((Rectangle){ barX, barY, barW, barH }, 1, (Color){ 100, 100, 100, 200 });

    const char *bossName = phase2 ? "THE MAD DUCK  -  ENRAGED" : "THE MAD DUCK";
    int nameW = MeasureText(bossName, 14);
    DrawText(bossName, (int)(bx - nameW / 2), (int)(barY - 16), 14,
             (Color){ 255, 230, 180, 220 });
}
