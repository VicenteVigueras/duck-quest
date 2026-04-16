#include "entities.h"
#include "combat.h"
#include "dungeon.h"
#include "items.h"
#include "utils.h"
#include <math.h>
#include <string.h>

/*
 * ============================================================================
 * PLAYER SYSTEM
 * ============================================================================
 */

void PlayerInit(void) {
    memset(&player, 0, sizeof(Player));
    playerSpawnPoint = (Vector2){
        ROOM_X + ROOM_WIDTH * 0.5f,
        ROOM_Y + ROOM_HEIGHT * 0.5f
    };

    player.pos = playerSpawnPoint;
    player.health = PLAYER_MAX_HEALTH;
    player.maxHealth = PLAYER_MAX_HEALTH;
    player.radius = PLAYER_SIZE * 0.85f;
    player.hasSword = false;
    player.inventory.heartContainers = 0;
    player.inventory.coins = 0;
    player.inventory.keys = 0;
    player.inventory.damageBonus = 0;
    player.inventory.speedBonus = 0;
    player.inventory.enemiesKilled = 0;
    player.inventory.roomsExplored = 1;
}

void PlayerRespawn(void) {
    player.pos = playerSpawnPoint;
    player.velocity = (Vector2){ 0, 0 };
    player.knockback = (Vector2){ 0, 0 };
    player.health = player.maxHealth;
    player.isAttacking = false;
    player.attackTimer = 0;
    player.attackCooldown = 0;
    player.invulnTimer = PLAYER_INVULN_TIME;

    // Respawn particles
    for (int i = 0; i < 20; i++) {
        float angle = ((float)GetRandomValue(0, 628)) / 100.0f;
        float speed = 80.0f + (float)GetRandomValue(0, 120);
        if (particles.count < MAX_PARTICLES) {
            Particle *pt = &particles.pool[particles.count++];
            pt->pos = player.pos;
            pt->vel = (Vector2){ cosf(angle) * speed, sinf(angle) * speed };
            pt->life = 0.5f;
            pt->maxLife = 0.5f;
            pt->size = 3.0f;
            pt->color = (Color){ 100, 200, 255, 200 };
            pt->type = PTYPE_SPARK;
        }
    }
}

void PlayerUpdate(float dt) {
    // Timers
    if (player.attackCooldown > 0) player.attackCooldown -= dt;
    if (player.invulnTimer > 0) player.invulnTimer -= dt;
    if (player.flashTimer > 0) player.flashTimer -= dt;
    if (player.shieldTimer > 0) player.shieldTimer -= dt;
    if (player.immunityTimer > 0) player.immunityTimer -= dt;

    // Attack animation
    if (player.isAttacking) {
        player.attackTimer += dt;
        if (player.attackTimer >= SWORD_SWING_DURATION) {
            player.isAttacking = false;
            player.attackTimer = 0;
        }
        float progress = player.attackTimer / SWORD_SWING_DURATION;
        player.swordAngle = -90.0f + progress * 180.0f;
    }

    // Movement input
    Vector2 input = { 0, 0 };
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) input.x -= 1.0f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input.x += 1.0f;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) input.y -= 1.0f;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) input.y += 1.0f;

    player.moving = (input.x != 0 || input.y != 0);
    float speed = PLAYER_SPEED + player.inventory.speedBonus;

    if (player.moving) {
        input = Vec2Normalize(input);
        player.velocity = Vec2Scale(input, speed);

        if (fabsf(input.x) > fabsf(input.y)) {
            player.facing = input.x > 0 ? 0 : 2;
        } else {
            player.facing = input.y > 0 ? 1 : 3;
        }

        player.animTimer += dt;
        if (player.animTimer >= ANIM_SPEED) {
            player.animTimer -= ANIM_SPEED;
            player.animFrame = (player.animFrame + 1) % 4;
        }
    } else {
        player.velocity = (Vector2){ 0, 0 };
        player.animFrame = 0;
        player.animTimer = 0;
    }

    // Knockback
    player.pos = Vec2Add(player.pos, Vec2Scale(player.knockback, dt));
    player.knockback = Vec2Scale(player.knockback, 1.0f - KNOCKBACK_DECAY * dt);
    if (Vec2Length(player.knockback) < 10.0f) player.knockback = (Vector2){ 0, 0 };

    // Movement
    player.pos = Vec2Add(player.pos, Vec2Scale(player.velocity, dt));

    // Room bounds (accounting for walls, but allow passage through doorways)
    {
        float wt = WALL_THICKNESS * TILE_SIZE;
        float minX = ROOM_X + wt + player.radius;
        float maxX = ROOM_X + ROOM_WIDTH - wt - player.radius;
        float minY = ROOM_Y + wt + player.radius;
        float maxY = ROOM_Y + ROOM_HEIGHT - wt - player.radius;

        DungeonRoom *room = DungeonGetCurrentRoom();
        float doorHalfW = DOORWAY_WIDTH * 0.5f;
        float doorHalfH = DOORWAY_HEIGHT * 0.5f;
        float cx = ROOM_X + ROOM_WIDTH * 0.5f;
        float cy = ROOM_Y + ROOM_HEIGHT * 0.5f;

        bool inDoorX = (player.pos.x > cx - doorHalfW && player.pos.x < cx + doorHalfW);
        bool inDoorY = (player.pos.y > cy - doorHalfH && player.pos.y < cy + doorHalfH);

        // Allow walking INTO doorways — key-locked doors still let the player
        // approach so DungeonCheckDoorways can consume a key / show "NEED KEY!".
        // doorsLocked (combat-lock) remains a hard wall.
        if (room->connections[DIR_NORTH] >= 0 && !room->doorsLocked && inDoorX) {
            minY = ROOM_Y + player.radius;
        }
        if (room->connections[DIR_SOUTH] >= 0 && !room->doorsLocked && inDoorX) {
            maxY = ROOM_Y + ROOM_HEIGHT - player.radius;
        }
        if (room->connections[DIR_WEST] >= 0 && !room->doorsLocked && inDoorY) {
            minX = ROOM_X + player.radius;
        }
        if (room->connections[DIR_EAST] >= 0 && !room->doorsLocked && inDoorY) {
            maxX = ROOM_X + ROOM_WIDTH - player.radius;
        }

        player.pos.x = Clamp(player.pos.x, minX, maxX);
        player.pos.y = Clamp(player.pos.y, minY, maxY);
    }

    // Sword attack
    if ((IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) &&
        player.hasSword && player.attackCooldown <= 0 && !player.isAttacking) {

        player.isAttacking = true;
        player.attackTimer = 0;
        player.attackCooldown = SWORD_ATTACK_COOLDOWN;
        player.swordAngle = -90.0f;

        float damage = SWORD_BASE_DAMAGE + player.inventory.damageBonus;

        // Swing particles
        for (int i = 0; i < 5; i++) {
            float angle = ((float)GetRandomValue(0, 628)) / 100.0f;
            float spd = 40.0f + (float)GetRandomValue(0, 60);
            if (particles.count < MAX_PARTICLES) {
                Particle *pt = &particles.pool[particles.count++];
                pt->pos = player.pos;
                pt->vel = (Vector2){ cosf(angle) * spd, sinf(angle) * spd };
                pt->life = 0.2f;
                pt->maxLife = 0.2f;
                pt->size = 2.0f;
                pt->color = (Color){ 200, 200, 255, 200 };
                pt->type = PTYPE_SPARK;
            }
        }

        // Hit enemies
        bool hitAny = false;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            Enemy *e = &enemies[i];
            if (!e->active || e->roomId != dungeon.currentRoomId) continue;

            float dist = Vec2Distance(player.pos, e->pos);
            if (dist < SWORD_ATTACK_RANGE + e->radius) {
                CombatApplyDamageToEnemy(e, damage, player.pos);
                hitAny = true;
            }
        }

        // Hit boss
        if (boss.active && boss.phase != BOSS_DYING && boss.phase != BOSS_DEAD) {
            float dist = Vec2Distance(player.pos, boss.pos);
            if (dist < SWORD_ATTACK_RANGE + boss.radius) {
                boss.health -= damage;
                boss.flashTimer = 0.1f;
                Vector2 kd = Vec2Sub(boss.pos, player.pos);
                ApplyKnockback(&boss.knockback, kd, KNOCKBACK_STRENGTH * 0.5f);
                ShakeScreen(6.0f, 0.2f, 45.0f);
                hitFreezeTimer = 0.04f;
                hitAny = true;

                for (int i = 0; i < 12; i++) {
                    float ang = ((float)GetRandomValue(0, 628)) / 100.0f;
                    float spd = 70.0f + (float)GetRandomValue(0, 120);
                    if (particles.count < MAX_PARTICLES) {
                        Particle *pt = &particles.pool[particles.count++];
                        pt->pos = boss.pos;
                        pt->vel = (Vector2){ cosf(ang) * spd, sinf(ang) * spd };
                        pt->life = 0.4f;
                        pt->maxLife = 0.4f;
                        pt->size = 3.0f;
                        pt->color = (Color){ 200, 195, 180, 255 };
                        pt->type = PTYPE_BLOOD;
                    }
                }
                SetSoundVolume(hitSound, sfxVolume);
                PlaySound(hitSound);
            }
        }

        if (hitAny && !boss.active) {
            SetSoundVolume(hitSound, sfxVolume);
            PlaySound(hitSound);
        }
    }

    // Sword pickup
    if (!player.hasSword && sword.active && sword.roomId == dungeon.currentRoomId) {
        if (Vec2Distance(player.pos, sword.pos) < SWORD_PICKUP_RADIUS) {
            player.hasSword = true;
            sword.active = false;
            for (int i = 0; i < 15; i++) {
                float angle = ((float)GetRandomValue(0, 628)) / 100.0f;
                float spd = 60.0f + (float)GetRandomValue(0, 100);
                if (particles.count < MAX_PARTICLES) {
                    Particle *pt = &particles.pool[particles.count++];
                    pt->pos = sword.pos;
                    pt->vel = (Vector2){ cosf(angle) * spd, sinf(angle) * spd };
                    pt->life = 0.4f;
                    pt->maxLife = 0.4f;
                    pt->size = 3.0f;
                    pt->color = (Color){ 255, 220, 100, 255 };
                    pt->type = PTYPE_SPARK;
                }
            }
        }
    }

    // Death
    if (player.health <= 0) {
        // Don't respawn, trigger game over via game state
        player.health = 0;
    }
}

void PlayerDraw(Texture2D spriteSheet) {
    float px = player.pos.x;
    float py = player.pos.y;

    // Shadow (pixel rectangle)
    DrawRectangle((int)(px - PLAYER_SIZE * 0.6f), (int)(py + PLAYER_SIZE * 0.55f),
                 (int)(PLAYER_SIZE * 1.2f), 4, (Color){ 0, 0, 0, 50 });

    // Immunity star overlay (rainbow cycling, like Mario star)
    if (player.immunityTimer > 0) {
        float cycle = gameTime * 8.0f;
        // Cycling rainbow colors
        unsigned char ir = (unsigned char)(sinf(cycle) * 127.0f + 128.0f);
        unsigned char ig = (unsigned char)(sinf(cycle + 2.094f) * 127.0f + 128.0f);
        unsigned char ib = (unsigned char)(sinf(cycle + 4.189f) * 127.0f + 128.0f);
        float pulse = sinf(gameTime * 12.0f) * 0.3f + 0.7f;
        int glowSz = (int)(PLAYER_SIZE * 1.5f * pulse);
        // Outer glow
        DrawCircle((int)px, (int)py, (float)glowSz,
                  (Color){ ir, ig, ib, (unsigned char)(50 * pulse) });
        // Inner glow ring
        DrawCircleLines((int)px, (int)py, PLAYER_SIZE * 1.1f,
                       (Color){ ir, ig, ib, (unsigned char)(180 * pulse) });
        // Sparkle particles around player
        for (int i = 0; i < 6; i++) {
            float angle = gameTime * 4.0f + (float)i * PI / 3.0f;
            float rad = PLAYER_SIZE * 1.3f + sinf(gameTime * 6.0f + (float)i) * 5.0f;
            float sx = px + cosf(angle) * rad;
            float sy = py + sinf(angle) * rad;
            unsigned char sa = (unsigned char)(200 * pulse);
            DrawRectangle((int)(sx - 2), (int)(sy - 2), 4, 4, (Color){ 255, 255, 200, sa });
        }
    }

    // Shield effect (pixel square outline)
    if (player.shieldTimer > 0) {
        float pulse = sinf(gameTime * 6.0f) * 0.2f + 0.8f;
        int shSz = (int)(PLAYER_SIZE * 1.2f * pulse);
        Color shC = { 80, 160, 255, (unsigned char)(100 * pulse) };
        DrawRectangleLinesEx((Rectangle){ px - shSz, py - shSz, shSz * 2.0f, shSz * 2.0f }, 2, shC);
    }

    // Sprite
    Rectangle frames[4] = {
        { 1, 2, 13, 13 }, { 17, 2, 13, 13 },
        { 33, 2, 13, 13 }, { 49, 2, 13, 13 }
    };
    int frameIdx = player.animFrame % 4;
    Rectangle src = frames[frameIdx];
    if (player.facing == 2) src.width *= -1;

    Rectangle dst = { px, py, PLAYER_SIZE * 2.0f, PLAYER_SIZE * 2.0f };
    Vector2 origin = { PLAYER_SIZE, PLAYER_SIZE };

    Color tint = WHITE;
    if (player.immunityTimer > 0) {
        // Rainbow tint cycle for star immunity
        float cycle = gameTime * 8.0f;
        tint = (Color){
            (unsigned char)(sinf(cycle) * 80.0f + 175.0f),
            (unsigned char)(sinf(cycle + 2.094f) * 80.0f + 175.0f),
            (unsigned char)(sinf(cycle + 4.189f) * 80.0f + 175.0f),
            255
        };
    } else if (player.flashTimer > 0) {
        tint = WHITE;
    } else if (player.invulnTimer > 0) {
        float alpha = sinf(gameTime * 30.0f) * 0.5f + 0.5f;
        tint = Fade(WHITE, alpha);
    }

    DrawTexturePro(spriteSheet, src, dst, origin, 0.0f, tint);

    // Sword rendering
    if (player.hasSword) {
        float baseAngle = (float)player.facing * 90.0f;
        float angle = baseAngle;
        if (player.isAttacking) angle = baseAngle + player.swordAngle;

        Vector2 swordOffset = {
            cosf(angle * DEG2RAD) * (PLAYER_SIZE * 0.8f),
            sinf(angle * DEG2RAD) * (PLAYER_SIZE * 0.8f)
        };
        Vector2 swordPos = Vec2Add(player.pos, swordOffset);

        // Sword trail when attacking
        if (player.isAttacking) {
            float trailAlpha = 1.0f - player.attackTimer / SWORD_SWING_DURATION;
            DrawRectanglePro(
                (Rectangle){ swordPos.x, swordPos.y, 10, 38 },
                (Vector2){ 5, 0 }, angle,
                Fade((Color){ 150, 180, 255, 100 }, trailAlpha * 0.5f)
            );
        }

        // Blade
        DrawRectanglePro(
            (Rectangle){ swordPos.x, swordPos.y, 8, 35 },
            (Vector2){ 4, 0 }, angle,
            (Color){ 180, 180, 200, 255 }
        );
        // Highlight
        DrawRectanglePro(
            (Rectangle){ swordPos.x + 2, swordPos.y, 2, 35 },
            (Vector2){ 4, 0 }, angle,
            Fade(WHITE, 0.6f)
        );
        // Guard
        DrawRectanglePro(
            (Rectangle){ swordPos.x, swordPos.y, 14, 4 },
            (Vector2){ 7, -1 }, angle,
            (Color){ 160, 140, 100, 255 }
        );
    }
}

/*
 * ============================================================================
 * ENEMY SYSTEM — Multi-type AI
 * ============================================================================
 */

static void UpdateSlime(Enemy *e, float dt) {
    e->hopTimer += dt;

    // Hop toward player in bursts
    float hopCycle = 1.3f;
    float hopDuration = 0.4f;
    float cyclePos = fmodf(e->hopTimer, hopCycle);

    if (cyclePos < hopDuration) {
        Vector2 toPlayer = Vec2Sub(player.pos, e->pos);
        Vector2 dir = Vec2Normalize(toPlayer);
        e->velocity = Vec2Scale(dir, SLIME_SPEED * 2.0f);

        if (fabsf(dir.x) > fabsf(dir.y)) {
            e->facing = dir.x > 0 ? 0 : 2;
        } else {
            e->facing = dir.y > 0 ? 1 : 3;
        }
    } else {
        e->velocity = (Vector2){ 0, 0 };
    }

    // Attack
    float dist = Vec2Distance(e->pos, player.pos);
    if (dist < SLIME_ATTACK_RANGE && e->attackCooldown <= 0) {
        CombatApplyDamageToPlayer(SLIME_DAMAGE, e->pos);
        e->attackCooldown = SLIME_ATTACK_COOLDOWN;
    }
}

static void UpdateBat(Enemy *e, float dt) {
    e->stateTimer += dt;

    // Sine-wave flight toward player
    Vector2 toPlayer = Vec2Sub(player.pos, e->pos);
    Vector2 dir = Vec2Normalize(toPlayer);

    // Perpendicular oscillation
    float sineVal = sinf(e->stateTimer * 4.0f + e->sineOffset) * 80.0f;
    Vector2 perp = { -dir.y, dir.x };

    e->velocity = Vec2Add(
        Vec2Scale(dir, BAT_SPEED * 0.7f),
        Vec2Scale(perp, sineVal)
    );

    if (fabsf(e->velocity.x) > fabsf(e->velocity.y)) {
        e->facing = e->velocity.x > 0 ? 0 : 2;
    } else {
        e->facing = e->velocity.y > 0 ? 1 : 3;
    }

    // Wing flap animation
    e->animTimer += dt;
    if (e->animTimer >= 0.08f) {
        e->animTimer -= 0.08f;
        e->animFrame = (e->animFrame + 1) % 2;
    }

    // Attack
    float dist = Vec2Distance(e->pos, player.pos);
    if (dist < BAT_ATTACK_RANGE && e->attackCooldown <= 0) {
        CombatApplyDamageToPlayer(BAT_DAMAGE, e->pos);
        e->attackCooldown = BAT_ATTACK_COOLDOWN;
    }
}

static void UpdateSkeleton(Enemy *e, float dt) {
    float dist = Vec2Distance(e->pos, player.pos);

    if (dist > SKELETON_ATTACK_RANGE * 0.5f) {
        // Walk toward player
        Vector2 dir = Vec2Normalize(Vec2Sub(player.pos, e->pos));
        e->velocity = Vec2Scale(dir, SKELETON_SPEED);

        if (fabsf(dir.x) > fabsf(dir.y)) {
            e->facing = dir.x > 0 ? 0 : 2;
        } else {
            e->facing = dir.y > 0 ? 1 : 3;
        }

        e->animTimer += dt;
        if (e->animTimer >= ANIM_SPEED) {
            e->animTimer -= ANIM_SPEED;
            e->animFrame = (e->animFrame + 1) % 4;
        }
    } else {
        e->velocity = (Vector2){ 0, 0 };
        e->animFrame = 0;
    }

    // Throw bone projectile
    if (dist < SKELETON_ATTACK_RANGE && e->attackCooldown <= 0) {
        Vector2 dir = Vec2Normalize(Vec2Sub(player.pos, e->pos));
        Vector2 vel = Vec2Scale(dir, BONE_PROJECTILE_SPEED);
        ProjectileSpawn(e->pos, vel, 5.0f, SKELETON_DAMAGE, false, e->roomId,
                       (Color){ 200, 195, 180, 255 });
        e->attackCooldown = SKELETON_ATTACK_COOLDOWN;
    }
}

static void UpdateTurret(Enemy *e, float dt) {
    (void)dt;
    // Rotate to face player
    Vector2 toPlayer = Vec2Sub(player.pos, e->pos);
    e->aimAngle = atan2f(toPlayer.y, toPlayer.x);

    // Fire
    if (e->attackCooldown <= 0) {
        Vector2 vel = Vec2Scale(Vec2Normalize(toPlayer), PROJECTILE_SPEED);
        ProjectileSpawn(e->pos, vel, 5.0f, TURRET_DAMAGE, false, e->roomId,
                       (Color){ 255, 100, 50, 255 });
        e->attackCooldown = TURRET_ATTACK_COOLDOWN;
    }

    e->velocity = (Vector2){ 0, 0 };
}

static void UpdateWizard(Enemy *e, float dt) {
    e->wizardFloatPhase += dt;
    e->wizardMouthTimer += dt;
    float dist = Vec2Distance(e->pos, player.pos);

    if (e->wizardCasting) {
        // Channeling a spell — stand still, arms raised
        e->wizardCastTimer -= dt;
        e->velocity = (Vector2){ 0, 0 };

        // Face the player while casting
        Vector2 toP = Vec2Sub(player.pos, e->pos);
        if (fabsf(toP.x) > fabsf(toP.y)) {
            e->facing = toP.x > 0 ? 0 : 2;
        } else {
            e->facing = toP.y > 0 ? 1 : 3;
        }

        // Channel particles (gathering energy at staff tip)
        if (particles.count < MAX_PARTICLES && GetRandomValue(0, 2) == 0) {
            float ang = ((float)GetRandomValue(0, 628)) / 100.0f;
            float rad = 20.0f + (float)GetRandomValue(0, 30);
            Particle *pt = &particles.pool[particles.count++];
            pt->pos = (Vector2){
                e->pos.x + cosf(ang) * rad,
                e->pos.y - e->radius * 0.5f + sinf(ang) * rad
            };
            // Converge toward wizard
            pt->vel = Vec2Scale(Vec2Sub(e->pos, pt->pos), 3.0f);
            pt->life = 0.3f;
            pt->maxLife = 0.3f;
            pt->size = 3.0f;
            pt->color = (Color){ 180, 80, 255, 220 };
            pt->type = PTYPE_SPARK;
        }

        if (e->wizardCastTimer <= 0.0f) {
            // Fire spell projectile at player
            Vector2 dir = Vec2Normalize(Vec2Sub(player.pos, e->pos));
            Vector2 vel = Vec2Scale(dir, WIZARD_SPELL_SPEED);
            ProjectileSpawn(e->pos, vel, 8.0f, WIZARD_DAMAGE, false, e->roomId,
                           (Color){ 160, 60, 220, 255 });

            // Cast burst particles (8-bit square sparks)
            for (int i = 0; i < 8; i++) {
                float ang = ((float)GetRandomValue(0, 628)) / 100.0f;
                float spd = 40.0f + (float)GetRandomValue(0, 80);
                if (particles.count < MAX_PARTICLES) {
                    Particle *pt = &particles.pool[particles.count++];
                    pt->pos = e->pos;
                    pt->vel = (Vector2){ cosf(ang) * spd, sinf(ang) * spd };
                    pt->life = 0.35f;
                    pt->maxLife = 0.35f;
                    pt->size = 3.0f;
                    pt->color = (Color){ 220, 150, 255, 255 };
                    pt->type = PTYPE_SPARK;
                }
            }

            ShakeScreen(1.5f, 0.08f, 35.0f);
            e->wizardCasting = false;
            e->attackCooldown = WIZARD_ATTACK_COOLDOWN;
        }
        return;
    }

    // Float around — slow wander with sine-wave motion
    Vector2 toPlayer = Vec2Sub(player.pos, e->pos);
    Vector2 dir = Vec2Normalize(toPlayer);
    float perpSine = sinf(e->wizardFloatPhase * 2.0f) * 60.0f;
    Vector2 perp = { -dir.y, dir.x };

    // Keep distance: if too close, back away; if far, approach
    float desiredDist = WIZARD_ATTACK_RANGE * 0.6f;
    float approachFactor = (dist > desiredDist) ? 1.0f : -0.5f;

    e->velocity = Vec2Add(
        Vec2Scale(dir, WIZARD_SPEED * approachFactor),
        Vec2Scale(perp, perpSine)
    );

    if (fabsf(dir.x) > fabsf(dir.y)) {
        e->facing = dir.x > 0 ? 0 : 2;
    } else {
        e->facing = dir.y > 0 ? 1 : 3;
    }

    // Walk animation
    e->animTimer += dt;
    if (e->animTimer >= ANIM_SPEED * 1.5f) {
        e->animTimer -= ANIM_SPEED * 1.5f;
        e->animFrame = (e->animFrame + 1) % 4;
    }

    // Start casting when in range
    if (dist < WIZARD_ATTACK_RANGE && e->attackCooldown <= 0) {
        e->wizardCasting = true;
        e->wizardCastTimer = WIZARD_CAST_TIME;
    }
}

void EnemyUpdate(Enemy *e, float dt) {
    if (!e->active) return;
    if (e->roomId != dungeon.currentRoomId) return;

    // Timers
    if (e->attackCooldown > 0) e->attackCooldown -= dt;
    if (e->flashTimer > 0) e->flashTimer -= dt;

    // Skip during initial delay
    if (e->stateTimer < 0) {
        e->stateTimer += dt;
        return;
    }

    // Type-specific AI
    switch (e->type) {
        case ENEMY_SLIME:    UpdateSlime(e, dt); break;
        case ENEMY_BAT:      UpdateBat(e, dt); break;
        case ENEMY_SKELETON: UpdateSkeleton(e, dt); break;
        case ENEMY_TURRET:   UpdateTurret(e, dt); break;
        case ENEMY_WIZARD:   UpdateWizard(e, dt); break;
        default: break;
    }

    // Knockback
    e->pos = Vec2Add(e->pos, Vec2Scale(e->knockback, dt));
    e->knockback = Vec2Scale(e->knockback, 1.0f - KNOCKBACK_DECAY * dt);
    if (Vec2Length(e->knockback) < 10.0f) e->knockback = (Vector2){ 0, 0 };

    // Movement
    e->pos = Vec2Add(e->pos, Vec2Scale(e->velocity, dt));

    // Separation
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *other = &enemies[i];
        if (other == e || !other->active || other->roomId != e->roomId) continue;
        float dist = Vec2Distance(e->pos, other->pos);
        if (dist < ENEMY_SEPARATION_RADIUS && dist > 0) {
            Vector2 away = Vec2Normalize(Vec2Sub(e->pos, other->pos));
            e->pos = Vec2Add(e->pos, Vec2Scale(away, (ENEMY_SEPARATION_RADIUS - dist) * 0.4f));
        }
    }

    // Bounds
    float wt = WALL_THICKNESS * TILE_SIZE;
    e->pos.x = Clamp(e->pos.x, ROOM_X + wt + e->radius, ROOM_X + ROOM_WIDTH - wt - e->radius);
    e->pos.y = Clamp(e->pos.y, ROOM_Y + wt + e->radius, ROOM_Y + ROOM_HEIGHT - wt - e->radius);
}

/*
 * ============================================================================
 * ENEMY DRAWING — Programmatic 8-bit sprites
 * ============================================================================
 */

static void DrawSlime(Enemy *e) {
    // p = pixel block size, scaled so the slime fills its radius
    float p = e->radius / 7.0f;

    float squish = 1.0f;
    float cyclePos = fmodf(e->hopTimer, 1.3f);
    if (cyclePos < 0.4f) {
        float t = cyclePos / 0.4f;
        squish = 1.0f - sinf(t * PI) * 0.25f;
    }

    Color body = (e->flashTimer > 0) ? WHITE : (Color){ 60, 180, 60, 255 };
    Color bodyDark = { (unsigned char)(body.r * 3/4), (unsigned char)(body.g * 3/4), (unsigned char)(body.b * 3/4), 255 };
    Color bodyHi = { (unsigned char)((body.r + 255)/2), (unsigned char)((body.g + 255)/2), (unsigned char)((body.b + 255)/2), 255 };

    float bx = e->pos.x;
    float by = e->pos.y;
    float hOff = (1.0f - squish) * 2.0f * p;

    // Shadow
    DrawRectangle((int)(bx - 4*p), (int)(by + 5*p), (int)(8*p), (int)p, (Color){ 0, 0, 0, 50 });

    // Body blob: stacked rows (wider in middle)
    DrawRectangle((int)(bx - 3*p), (int)(by + 3*p - hOff), (int)(6*p), (int)p, bodyDark);
    DrawRectangle((int)(bx - 5*p), (int)(by + 1*p - hOff), (int)(10*p), (int)(2*p), body);
    DrawRectangle((int)(bx - 6*p), (int)(by - 2*p - hOff), (int)(12*p), (int)(3*p), body);
    DrawRectangle((int)(bx - 5*p), (int)(by - 4*p - hOff), (int)(10*p), (int)(2*p), body);
    DrawRectangle((int)(bx - 3*p), (int)(by - 5*p - hOff), (int)(6*p), (int)p, body);

    // Highlight on upper-left
    DrawRectangle((int)(bx - 4*p), (int)(by - 3*p - hOff), (int)(2*p), (int)p, bodyHi);

    // Belly stripe
    DrawRectangle((int)(bx - 4*p), (int)(by - hOff), (int)(8*p), (int)p, bodyDark);

    // Eyes (square pixels)
    float eyeDir = (e->facing == 2) ? -1.0f : 1.0f;
    DrawRectangle((int)(bx - 3*p + eyeDir*p), (int)(by - 4*p - hOff), (int)(2*p), (int)(2*p), WHITE);
    DrawRectangle((int)(bx + 1*p + eyeDir*p), (int)(by - 4*p - hOff), (int)(2*p), (int)(2*p), WHITE);
    DrawRectangle((int)(bx - 2*p + eyeDir*2*p), (int)(by - 3*p - hOff), (int)p, (int)p, (Color){ 20, 20, 20, 255 });
    DrawRectangle((int)(bx + 2*p + eyeDir*2*p), (int)(by - 3*p - hOff), (int)p, (int)p, (Color){ 20, 20, 20, 255 });
}

static void DrawBat(Enemy *e) {
    Color body = (e->flashTimer > 0) ? WHITE : (Color){ 80, 50, 100, 255 };

    // Shadow
    DrawEllipse((int)e->pos.x, (int)(e->pos.y + e->radius), 8, 3, (Color){ 0, 0, 0, 40 });

    // Body
    DrawCircle((int)e->pos.x, (int)e->pos.y, 8, body);

    // Wings
    float wingAngle = e->animFrame == 0 ? 20.0f : -15.0f;
    float wy = e->pos.y + wingAngle * 0.1f;

    // Left wing
    Vector2 lw[3] = {
        { e->pos.x - 6, e->pos.y },
        { e->pos.x - 22, wy - 8 },
        { e->pos.x - 12, e->pos.y + 5 }
    };
    DrawTriangle(lw[0], lw[2], lw[1], body);

    // Right wing
    Vector2 rw[3] = {
        { e->pos.x + 6, e->pos.y },
        { e->pos.x + 22, wy - 8 },
        { e->pos.x + 12, e->pos.y + 5 }
    };
    DrawTriangle(rw[0], rw[1], rw[2], body);

    // Eyes
    DrawCircle((int)(e->pos.x - 3), (int)(e->pos.y - 2), 2, (Color){ 255, 50, 50, 255 });
    DrawCircle((int)(e->pos.x + 3), (int)(e->pos.y - 2), 2, (Color){ 255, 50, 50, 255 });
}

static void DrawSkeleton(Enemy *e) {
    Color bone = (e->flashTimer > 0) ? WHITE : (Color){ 200, 195, 180, 255 };
    Color boneDark = { bone.r * 3/4, bone.g * 3/4, bone.b * 3/4, 255 };

    float bx = e->pos.x;
    float by = e->pos.y;
    float s = SKELETON_SIZE / 30.0f; // Scale factor relative to original size

    // Shadow (pixel rectangle)
    DrawRectangle((int)(bx - 10*s), (int)(by + 18*s), (int)(20*s), (int)(4*s), (Color){ 0, 0, 0, 50 });

    // Legs
    float walkOffset = sinf(e->animFrame * PI * 0.5f) * 4.0f * s;
    DrawRectangle((int)(bx - 5*s), (int)(by + 5*s), (int)(4*s), (int)(14*s + walkOffset), boneDark);
    DrawRectangle((int)(bx + 2*s), (int)(by + 5*s), (int)(4*s), (int)(14*s - walkOffset), boneDark);

    // Torso
    DrawRectangle((int)(bx - 8*s), (int)(by - 10*s), (int)(16*s), (int)(18*s), bone);

    // Ribs
    for (int i = 0; i < 3; i++) {
        DrawRectangle((int)(bx - 6*s), (int)(by - 6*s + i * 5*s), (int)(12*s), (int)(2*s), boneDark);
    }

    // Head (square pixel skull)
    DrawRectangle((int)(bx - 7*s), (int)(by - 26*s), (int)(14*s), (int)(14*s), bone);
    // Top highlight
    DrawRectangle((int)(bx - 7*s), (int)(by - 26*s), (int)(14*s), (int)(2*s),
                 (Color){ (unsigned char)(bone.r + 20 > 255 ? 255 : bone.r + 20),
                          (unsigned char)(bone.g + 20 > 255 ? 255 : bone.g + 20),
                          (unsigned char)(bone.b + 20 > 255 ? 255 : bone.b + 20), 255 });
    // Eye sockets
    DrawRectangle((int)(bx - 5*s), (int)(by - 23*s), (int)(4*s), (int)(4*s), (Color){ 20, 20, 20, 255 });
    DrawRectangle((int)(bx + 1*s), (int)(by - 23*s), (int)(4*s), (int)(4*s), (Color){ 20, 20, 20, 255 });
    // Nose
    DrawRectangle((int)(bx - 1*s), (int)(by - 18*s), (int)(2*s), (int)(2*s), boneDark);
    // Jaw (separate block with gap)
    DrawRectangle((int)(bx - 6*s), (int)(by - 14*s), (int)(12*s), (int)(4*s), boneDark);
    // Teeth
    DrawRectangle((int)(bx - 4*s), (int)(by - 15*s), (int)(2*s), (int)(2*s), bone);
    DrawRectangle((int)(bx - 1*s), (int)(by - 15*s), (int)(2*s), (int)(2*s), bone);
    DrawRectangle((int)(bx + 2*s), (int)(by - 15*s), (int)(2*s), (int)(2*s), bone);

    // Arms
    float armSwing = sinf(e->stateTimer * 2.0f) * 4.0f * s;
    DrawRectangle((int)(bx - 13*s), (int)(by - 8*s + armSwing), (int)(5*s), (int)(14*s), boneDark);
    DrawRectangle((int)(bx + 8*s), (int)(by - 8*s - armSwing), (int)(5*s), (int)(14*s), boneDark);
}

static void DrawTurret(Enemy *e) {
    Color base = (e->flashTimer > 0) ? WHITE : (Color){ 100, 90, 80, 255 };
    Color barrel = (e->flashTimer > 0) ? WHITE : (Color){ 140, 130, 120, 255 };

    // Shadow
    DrawEllipse((int)e->pos.x, (int)(e->pos.y + 12), 14, 5, (Color){ 0, 0, 0, 50 });

    // Base
    DrawRectangle((int)(e->pos.x - 14), (int)(e->pos.y - 14), 28, 28, base);
    DrawRectangle((int)(e->pos.x - 12), (int)(e->pos.y - 12), 24, 24,
                 (Color){ base.r + 15, base.g + 15, base.b + 15, 255 });

    // Barrel (rotated toward player)
    float angle = e->aimAngle * RAD2DEG;
    DrawRectanglePro(
        (Rectangle){ e->pos.x, e->pos.y, 24, 6 },
        (Vector2){ 0, 3 }, angle, barrel
    );

    // Center
    DrawCircle((int)e->pos.x, (int)e->pos.y, 6, (Color){ 60, 50, 45, 255 });
    DrawCircle((int)e->pos.x, (int)e->pos.y, 3, (Color){ 200, 50, 50, 255 });
}

static void DrawWizard(Enemy *e) {
    float p = e->radius / 8.0f;
    float bx = e->pos.x;
    float by = e->pos.y;

    // Floating hover offset (bobbing up and down)
    float floatOff = sinf(e->wizardFloatPhase * 3.0f) * 6.0f;
    by += floatOff;

    Color body = (e->flashTimer > 0) ? WHITE : (Color){ 100, 50, 160, 255 };
    Color bodyDark = { (unsigned char)(body.r * 3/4), (unsigned char)(body.g * 3/4), (unsigned char)(body.b * 3/4), 255 };
    Color bodyHi = { (unsigned char)((body.r + 255)/2), (unsigned char)((body.g + 255)/2), (unsigned char)((body.b + 255)/2), 255 };
    Color robeColor = (e->flashTimer > 0) ? WHITE : (Color){ 60, 30, 100, 255 };
    Color hatColor = (e->flashTimer > 0) ? WHITE : (Color){ 80, 40, 130, 255 };
    Color starColor = { 255, 220, 100, 255 };
    Color skinColor = (e->flashTimer > 0) ? WHITE : (Color){ 160, 130, 180, 255 };

    // Shadow
    DrawRectangle((int)(bx - 5*p), (int)(e->pos.y + 7*p), (int)(10*p), (int)(2*p), (Color){ 0, 0, 0, 50 });

    // Casting: magic circle on ground (8-bit square ring)
    if (e->wizardCasting) {
        float progress = 1.0f - (e->wizardCastTimer / WIZARD_CAST_TIME);
        float pulse = sinf(gameTime * 10.0f) * 0.3f + 0.7f;
        unsigned char alpha = (unsigned char)(60.0f * progress * pulse);
        // Square magic circle (pixel-art style, not round)
        int ringSize = (int)(16*p * progress);
        DrawRectangleLinesEx(
            (Rectangle){ bx - ringSize, e->pos.y + 5*p - ringSize,
                        ringSize * 2.0f, ringSize * 2.0f },
            (int)(2*p), (Color){ 180, 80, 255, alpha });
        // Corner runes (small squares at corners)
        int rs = (int)(2*p);
        DrawRectangle((int)(bx - ringSize), (int)(e->pos.y + 5*p - ringSize), rs, rs,
                     (Color){ 220, 150, 255, alpha });
        DrawRectangle((int)(bx + ringSize - rs), (int)(e->pos.y + 5*p - ringSize), rs, rs,
                     (Color){ 220, 150, 255, alpha });
        DrawRectangle((int)(bx - ringSize), (int)(e->pos.y + 5*p + ringSize - rs), rs, rs,
                     (Color){ 220, 150, 255, alpha });
        DrawRectangle((int)(bx + ringSize - rs), (int)(e->pos.y + 5*p + ringSize - rs), rs, rs,
                     (Color){ 220, 150, 255, alpha });
    }

    // Robe (wide bottom — stacked pixel rows)
    DrawRectangle((int)(bx - 5*p), (int)(by + 1*p), (int)(10*p), (int)(2*p), robeColor);
    DrawRectangle((int)(bx - 6*p), (int)(by + 3*p), (int)(12*p), (int)(2*p), robeColor);
    DrawRectangle((int)(bx - 7*p), (int)(by + 5*p), (int)(14*p), (int)(2*p), robeColor);
    // Robe hem detail
    DrawRectangle((int)(bx - 7*p), (int)(by + 6*p), (int)(14*p), (int)p, bodyDark);
    // Robe belt/sash
    DrawRectangle((int)(bx - 5*p), (int)(by + 0*p), (int)(10*p), (int)p,
                 (Color){ 200, 160, 60, 255 });

    // Body/torso
    DrawRectangle((int)(bx - 4*p), (int)(by - 4*p), (int)(8*p), (int)(5*p), body);
    DrawRectangle((int)(bx - 3*p), (int)(by - 3*p), (int)(2*p), (int)(2*p), bodyHi);

    // Head (skin-colored, larger for expressive face)
    DrawRectangle((int)(bx - 4*p), (int)(by - 9*p), (int)(8*p), (int)(6*p), skinColor);
    // Head highlight
    DrawRectangle((int)(bx - 4*p), (int)(by - 9*p), (int)(8*p), (int)p,
                 (Color){ skinColor.r + 20 > 255 ? 255 : skinColor.r + 20,
                          skinColor.g + 20 > 255 ? 255 : skinColor.g + 20,
                          skinColor.b + 20 > 255 ? 255 : skinColor.b + 20, 255 });

    // Pointy hat (wizard hat! all square pixels)
    DrawRectangle((int)(bx - 5*p), (int)(by - 10*p), (int)(10*p), (int)(2*p), hatColor);
    DrawRectangle((int)(bx - 4*p), (int)(by - 12*p), (int)(8*p), (int)(2*p), hatColor);
    DrawRectangle((int)(bx - 3*p), (int)(by - 14*p), (int)(6*p), (int)(2*p), hatColor);
    DrawRectangle((int)(bx - 2*p), (int)(by - 16*p), (int)(4*p), (int)(2*p), hatColor);
    DrawRectangle((int)(bx - 1*p), (int)(by - 17*p), (int)(2*p), (int)(2*p), hatColor);
    // Hat brim (wide, square)
    DrawRectangle((int)(bx - 6*p), (int)(by - 10*p), (int)(12*p), (int)(2*p), hatColor);
    // Hat band
    DrawRectangle((int)(bx - 5*p), (int)(by - 10*p), (int)(10*p), (int)p,
                 (Color){ 200, 160, 60, 255 });
    // Star on hat (pulsing)
    float starPulse = sinf(gameTime * 4.0f) * 0.3f + 0.7f;
    DrawRectangle((int)(bx - p), (int)(by - 14*p), (int)(2*p), (int)p,
                 (Color){ starColor.r, starColor.g, starColor.b, (unsigned char)(255 * starPulse) });
    DrawRectangle((int)(bx - p/2), (int)(by - 15*p), (int)p, (int)(3*p),
                 (Color){ starColor.r, starColor.g, starColor.b, (unsigned char)(255 * starPulse) });

    // === EXPRESSIVE FACE ===
    float eyeDir = (e->facing == 2) ? -1.0f : 1.0f;
    bool casting = e->wizardCasting;

    // Eyebrows (angry when casting, relaxed when idle)
    Color browColor = { 60, 30, 80, 255 };
    if (casting) {
        // Angry V-shaped eyebrows
        DrawRectangle((int)(bx - 3*p + eyeDir*p), (int)(by - 8*p), (int)(2*p), (int)p, browColor);
        DrawRectangle((int)(bx - 2*p + eyeDir*p), (int)(by - 9*p), (int)p, (int)p, browColor);
        DrawRectangle((int)(bx + 1*p + eyeDir*p), (int)(by - 8*p), (int)(2*p), (int)p, browColor);
        DrawRectangle((int)(bx + 2*p + eyeDir*p), (int)(by - 9*p), (int)p, (int)p, browColor);
    } else {
        // Flat brows
        DrawRectangle((int)(bx - 3*p + eyeDir*p), (int)(by - 9*p), (int)(2*p), (int)p, browColor);
        DrawRectangle((int)(bx + 1*p + eyeDir*p), (int)(by - 9*p), (int)(2*p), (int)p, browColor);
    }

    // Eyes (big square pixels, squint when casting)
    Color eyeWhite = { 240, 240, 250, 255 };
    Color eyeColor = casting ? (Color){ 255, 80, 80, 255 } : (Color){ 180, 120, 255, 255 };
    int eyeH = casting ? (int)p : (int)(2*p);
    int eyeYoff = casting ? 0 : 0;
    // Eye whites
    DrawRectangle((int)(bx - 3*p + eyeDir*p), (int)(by - 8*p + eyeYoff), (int)(2*p), (int)(2*p), eyeWhite);
    DrawRectangle((int)(bx + 1*p + eyeDir*p), (int)(by - 8*p + eyeYoff), (int)(2*p), (int)(2*p), eyeWhite);
    // Irises (follow player direction, colored)
    DrawRectangle((int)(bx - 2*p + eyeDir*2*p), (int)(by - 7*p + eyeYoff), (int)p, eyeH, eyeColor);
    DrawRectangle((int)(bx + 2*p + eyeDir*2*p), (int)(by - 7*p + eyeYoff), (int)p, eyeH, eyeColor);
    // Pupils (tiny dark squares)
    if (!casting) {
        DrawRectangle((int)(bx - 2*p + eyeDir*2*p), (int)(by - 7*p), (int)p, (int)p, (Color){ 20, 10, 30, 255 });
        DrawRectangle((int)(bx + 2*p + eyeDir*2*p), (int)(by - 7*p), (int)p, (int)p, (Color){ 20, 10, 30, 255 });
    }
    // Eye glow when casting (pixel squares around eyes)
    if (casting) {
        float gp = sinf(gameTime * 8.0f) * 0.4f + 0.6f;
        Color glow = { 200, 100, 255, (unsigned char)(80 * gp) };
        DrawRectangle((int)(bx - 4*p + eyeDir*p), (int)(by - 9*p), (int)(4*p), (int)(4*p), glow);
        DrawRectangle((int)(bx + 0*p + eyeDir*p), (int)(by - 9*p), (int)(4*p), (int)(4*p), glow);
    }

    // Nose (small pixel)
    DrawRectangle((int)(bx + eyeDir*p), (int)(by - 5*p), (int)p, (int)p,
                 (Color){ skinColor.r - 20, skinColor.g - 20, skinColor.b - 20, 255 });

    // Mouth (changes expression!)
    if (casting) {
        // Open mouth — shouting incantation (flickers between shapes)
        int mouthFrame = (int)(e->wizardMouthTimer * 6.0f) % 3;
        if (mouthFrame == 0) {
            // Wide O shape
            DrawRectangle((int)(bx - 2*p), (int)(by - 4*p), (int)(4*p), (int)(2*p), (Color){ 40, 20, 30, 255 });
            DrawRectangle((int)(bx - p), (int)(by - 4*p), (int)(2*p), (int)p, (Color){ 180, 60, 60, 255 });
        } else if (mouthFrame == 1) {
            // Teeth bared
            DrawRectangle((int)(bx - 2*p), (int)(by - 4*p), (int)(4*p), (int)(2*p), (Color){ 40, 20, 30, 255 });
            DrawRectangle((int)(bx - p), (int)(by - 4*p), (int)(2*p), (int)p, (Color){ 230, 230, 230, 255 });
        } else {
            // Narrow slit
            DrawRectangle((int)(bx - 2*p), (int)(by - 4*p), (int)(4*p), (int)p, (Color){ 40, 20, 30, 255 });
        }
    } else {
        // Smug smirk (curved pixel line)
        DrawRectangle((int)(bx - p), (int)(by - 4*p), (int)(3*p), (int)p, (Color){ 60, 30, 40, 255 });
        DrawRectangle((int)(bx + p), (int)(by - 5*p), (int)p, (int)p, (Color){ 60, 30, 40, 255 });
    }

    // Beard (small pixel tuft)
    DrawRectangle((int)(bx - p), (int)(by - 3*p), (int)(2*p), (int)(2*p),
                 (Color){ 140, 140, 150, 200 });
    DrawRectangle((int)(bx), (int)(by - 1*p), (int)p, (int)p,
                 (Color){ 140, 140, 150, 180 });

    // Arms
    float armSwing;
    if (casting) {
        // Arms raised high for spell casting
        armSwing = -6.0f * p + sinf(gameTime * 10.0f) * 2.0f * p;
    } else {
        armSwing = sinf(e->wizardFloatPhase * 2.5f) * 3.0f * p;
    }
    // Left arm
    DrawRectangle((int)(bx - 7*p), (int)(by - 2*p + armSwing), (int)(3*p), (int)(5*p), body);
    // Right arm
    DrawRectangle((int)(bx + 4*p), (int)(by - 2*p - armSwing), (int)(3*p), (int)(5*p), body);
    // Hands (skin colored pixel squares)
    DrawRectangle((int)(bx - 7*p), (int)(by + 2*p + armSwing), (int)(2*p), (int)(2*p), skinColor);
    DrawRectangle((int)(bx + 5*p), (int)(by + 2*p - armSwing), (int)(2*p), (int)(2*p), skinColor);

    // Staff (in right hand, square pixel art)
    float staffX = bx + 6*p;
    float staffY = by - 6*p - armSwing;
    Color staffWood = { 140, 100, 60, 255 };
    Color staffWoodDk = { 110, 75, 45, 255 };
    DrawRectangle((int)(staffX), (int)(staffY), (int)p, (int)(14*p), staffWood);
    DrawRectangle((int)(staffX + p), (int)(staffY), (int)p, (int)(14*p), staffWoodDk);
    // Staff orb (square, pulsing)
    float orbPulse = sinf(gameTime * 5.0f) * 0.4f + 0.6f;
    int orbSz = (int)(3*p * orbPulse);
    Color orbColor = casting
        ? (Color){ 255, 80, 80, (unsigned char)(220 * orbPulse) }
        : (Color){ 180, 80, 255, (unsigned char)(200 * orbPulse) };
    Color orbInner = casting
        ? (Color){ 255, 200, 200, (unsigned char)(255 * orbPulse) }
        : (Color){ 220, 180, 255, (unsigned char)(255 * orbPulse) };
    DrawRectangle((int)(staffX + p/2 - orbSz/2), (int)(staffY - orbSz), orbSz, orbSz, orbColor);
    int innerSz = orbSz / 2;
    if (innerSz < 1) innerSz = 1;
    DrawRectangle((int)(staffX + p/2 - innerSz/2), (int)(staffY - orbSz + orbSz/4), innerSz, innerSz, orbInner);

    // Magical floating pixel squares around wizard
    for (int i = 0; i < 4; i++) {
        float angle = gameTime * 2.0f + (float)i * PI * 0.5f;
        float orbitR = 12.0f + sinf(gameTime * 3.0f + (float)i) * 4.0f;
        float px2 = bx + cosf(angle) * orbitR;
        float py2 = by - 4*p + sinf(angle) * orbitR;
        unsigned char sparkAlpha = (unsigned char)(150 + (int)(sinf(gameTime * 5.0f + (float)i * 2.0f) * 80.0f));
        int ss = (int)(p * 0.8f);
        if (ss < 2) ss = 2;
        DrawRectangle((int)(px2 - ss/2), (int)(py2 - ss/2), ss, ss,
                     (Color){ 200, 150, 255, sparkAlpha });
    }
}

void EnemyDraw(Enemy *e) {
    if (!e->active) return;
    if (e->roomId != dungeon.currentRoomId) return;

    switch (e->type) {
        case ENEMY_SLIME:    DrawSlime(e); break;
        case ENEMY_BAT:      DrawBat(e); break;
        case ENEMY_SKELETON: DrawSkeleton(e); break;
        case ENEMY_TURRET:   DrawTurret(e); break;
        case ENEMY_WIZARD:   DrawWizard(e); break;
        default: break;
    }

    // Health bar (only show if damaged)
    if (e->health < e->maxHealth) {
        float barW = 40.0f;
        float barH = 4.0f;
        float barX = e->pos.x - barW / 2;
        float barY = e->pos.y - e->radius - 12.0f;
        float ratio = e->health / e->maxHealth;
        if (ratio < 0) ratio = 0;

        DrawRectangle((int)barX, (int)barY, (int)barW, (int)barH, (Color){ 30, 30, 30, 180 });
        Color hpColor = ratio > 0.5f ? (Color){ 100, 220, 100, 255 }
                       : ratio > 0.25f ? (Color){ 220, 180, 50, 255 }
                       : (Color){ 220, 50, 50, 255 };
        DrawRectangle((int)barX, (int)barY, (int)(barW * ratio), (int)barH, hpColor);
    }
}

/*
 * ============================================================================
 * SWORD SYSTEM
 * ============================================================================
 */

void SwordInit(int roomId) {
    sword.pos = (Vector2){ ROOM_X + ROOM_WIDTH * 0.5f, ROOM_Y + ROOM_HEIGHT * 0.35f };
    sword.roomId = roomId;
    sword.active = true;
    sword.bobPhase = 0;
    sword.spinAngle = 0;
}

void SwordUpdate(float dt) {
    if (!sword.active) return;
    sword.bobPhase += dt * 3.0f;
    sword.spinAngle += dt * 90.0f;
    if (sword.spinAngle >= 360.0f) sword.spinAngle -= 360.0f;
}

void SwordDraw(void) {
    if (!sword.active) return;
    if (sword.roomId != dungeon.currentRoomId) return;

    int p = 3; // Pixel size
    float bobOffset = sinf(sword.bobPhase) * 6.0f;
    float dx = sword.pos.x;
    float dy = sword.pos.y + bobOffset;

    // Pixelated glow (pulsing square)
    float pulse = sinf(gameTime * 3.0f) * 0.3f + 0.7f;
    int glowSz = (int)(20.0f * pulse);
    DrawRectangle((int)(dx - glowSz), (int)(dy - glowSz), glowSz * 2, glowSz * 2,
                 (Color){ 255, 220, 100, (unsigned char)(20 * pulse) });

    // Shadow
    DrawRectangle((int)(dx - 6), (int)(sword.pos.y + 12), 12, p, (Color){ 0, 0, 0, 40 });

    // Pixel art sword (static, facing up)
    Color blade = { 200, 200, 220, 255 };
    Color bladeHi = { 230, 230, 245, 255 };
    Color guard = { 160, 140, 100, 255 };
    Color grip = { 100, 75, 55, 255 };
    Color pommel = { 180, 160, 80, 255 };

    // Blade
    DrawRectangle((int)(dx - p), (int)(dy - 6*p), 2*p, 8*p, blade);
    DrawRectangle((int)(dx),     (int)(dy - 6*p), p, 8*p, bladeHi); // highlight
    // Tip
    DrawRectangle((int)(dx - p/2), (int)(dy - 7*p), p, p, blade);
    // Guard
    DrawRectangle((int)(dx - 2*p), (int)(dy + 2*p), 4*p, p, guard);
    // Grip
    DrawRectangle((int)(dx - p/2), (int)(dy + 3*p), p, 2*p, grip);
    // Pommel
    DrawRectangle((int)(dx - p), (int)(dy + 5*p), 2*p, p, pommel);
}
