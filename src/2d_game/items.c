#include "items.h"
#include "utils.h"
#include "renderer.h"
#include "dungeon.h"
#include <math.h>
#include <string.h>

static float shopMessageTimer = 0.0f;
static Vector2 shopMessagePos = {0};

void ItemsInit(void) {
    memset(worldItems, 0, sizeof(WorldItem) * MAX_WORLD_ITEMS);
    shopMessageTimer = 0.0f;
}

void ItemSpawn(ItemType type, Vector2 pos, int roomId, int price) {
    for (int i = 0; i < MAX_WORLD_ITEMS; i++) {
        if (!worldItems[i].active && !worldItems[i].collected) {
            worldItems[i].type = type;
            worldItems[i].pos = pos;
            worldItems[i].roomId = roomId;
            worldItems[i].active = true;
            worldItems[i].collected = false;
            worldItems[i].bobPhase = gameTime + (float)i;
            worldItems[i].price = price;
            // Pop/jump effect for dropped items (not shop items)
            if (price == 0 && (type == ITEM_COIN || type == ITEM_HEART ||
                type == ITEM_DAMAGE_UP || type == ITEM_SPEED_UP || type == ITEM_RUBBER_DUCK)) {
                worldItems[i].spawnVelY = -120.0f - (float)GetRandomValue(0, 60);
                worldItems[i].spawnGround = pos.y;
                worldItems[i].pos.y -= 5.0f; // Start slightly above
            } else {
                worldItems[i].spawnVelY = 0.0f;
                worldItems[i].spawnGround = pos.y;
            }
            return;
        }
    }
}

static void ApplyItem(ItemType type) {
    switch (type) {
        case ITEM_HEART:
            player.health += HEART_HP;
            if (player.health > player.maxHealth) player.health = player.maxHealth;
            break;
        case ITEM_HEART_CONTAINER:
            player.inventory.heartContainers++;
            player.maxHealth += HEART_HP;
            player.health += HEART_HP;
            break;
        case ITEM_DAMAGE_UP:
            player.inventory.damageBonus += 10.0f;
            break;
        case ITEM_SPEED_UP:
            player.inventory.speedBonus += 40.0f;
            break;
        case ITEM_KEY:
            player.inventory.keys++;
            break;
        case ITEM_COIN:
            player.inventory.coins++;
            break;
        case ITEM_SHIELD:
            player.shieldTimer = 8.0f;
            break;
        case ITEM_BOMB: {
            // Damage all enemies in current room
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].active && enemies[i].roomId == dungeon.currentRoomId) {
                    enemies[i].health -= 40.0f;
                    enemies[i].flashTimer = 0.2f;
                    if (enemies[i].health <= 0.0f) {
                        enemies[i].active = false;
                        player.inventory.enemiesKilled++;
                    }
                }
            }
            // Explosion particles
            for (int i = 0; i < 40; i++) {
                float angle = ((float)GetRandomValue(0, 628)) / 100.0f;
                float speed = 100.0f + (float)GetRandomValue(0, 200);
                Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
                if (particles.count < MAX_PARTICLES) {
                    Particle *pt = &particles.pool[particles.count++];
                    pt->pos = player.pos;
                    pt->vel = vel;
                    pt->life = 0.5f;
                    pt->maxLife = 0.5f;
                    pt->size = 4.0f;
                    pt->color = (Color){ 255, 150, 50, 255 };
                    pt->type = PTYPE_SPARK;
                }
            }
            break;
        }
        case ITEM_RUBBER_DUCK:
            player.immunityTimer = 5.0f; // 5 seconds of immunity
            break;
        default: break;
    }
}

void ItemsUpdate(float dt) {
    if (shopMessageTimer > 0.0f) shopMessageTimer -= dt;
    for (int i = 0; i < MAX_WORLD_ITEMS; i++) {
        WorldItem *item = &worldItems[i];
        if (!item->active || item->collected) continue;

        // Spawn pop physics
        if (item->spawnVelY != 0.0f) {
            item->spawnVelY += 400.0f * dt; // Gravity
            item->pos.y += item->spawnVelY * dt;
            if (item->pos.y >= item->spawnGround) {
                item->pos.y = item->spawnGround;
                item->spawnVelY = 0.0f;
            }
        }

        if (item->roomId != dungeon.currentRoomId) continue;

        float dist = Vec2Distance(player.pos, item->pos);
        if (dist < ITEM_PICKUP_RADIUS + player.radius) {
            // Check if it's a shop item
            if (item->price > 0) {
                if (player.inventory.coins >= item->price) {
                    player.inventory.coins -= item->price;
                } else {
                    // Show "not enough coins" message
                    shopMessageTimer = 1.5f;
                    shopMessagePos = item->pos;
                    continue;
                }
            }

            ApplyItem(item->type);
            item->active = false;
            item->collected = true;

            // Pickup particles
            Color pickupColor = { 255, 255, 100, 255 };
            for (int j = 0; j < 10; j++) {
                float angle = ((float)GetRandomValue(0, 628)) / 100.0f;
                float speed = 40.0f + (float)GetRandomValue(0, 60);
                Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
                if (particles.count < MAX_PARTICLES) {
                    Particle *pt = &particles.pool[particles.count++];
                    pt->pos = item->pos;
                    pt->vel = vel;
                    pt->life = 0.3f;
                    pt->maxLife = 0.3f;
                    pt->size = 2.0f;
                    pt->color = pickupColor;
                    pt->type = PTYPE_SPARK;
                }
            }

            // No sound on pickup — hit sound reserved for combat only
        }
    }
}

static const char *GetItemName(ItemType type) {
    switch (type) {
        case ITEM_HEART:           return "HEART";
        case ITEM_HEART_CONTAINER: return "HEART+";
        case ITEM_DAMAGE_UP:       return "ATK UP";
        case ITEM_SPEED_UP:        return "SPD UP";
        case ITEM_KEY:             return "KEY";
        case ITEM_COIN:            return "COIN";
        case ITEM_SHIELD:          return "SHIELD";
        case ITEM_BOMB:            return "BOMB";
        case ITEM_RUBBER_DUCK:     return "RUBBER DUCK";
        default:                   return "";
    }
}

static const char *GetItemDesc(ItemType type) {
    switch (type) {
        case ITEM_HEART:           return "Restores 1 heart";
        case ITEM_HEART_CONTAINER: return "+1 max heart";
        case ITEM_DAMAGE_UP:       return "+10 attack damage";
        case ITEM_SPEED_UP:        return "+40 movement speed";
        case ITEM_KEY:             return "Opens locked doors";
        case ITEM_COIN:            return "Currency";
        case ITEM_SHIELD:          return "Blocks hits for 8s";
        case ITEM_BOMB:            return "Damages all enemies";
        case ITEM_RUBBER_DUCK:     return "5s attack immunity";
        default:                   return "";
    }
}

void ItemsDraw(void) {
    int currentRoom = dungeon.currentRoomId;
    DungeonRoom *curRoom = DungeonGetCurrentRoom();

    for (int i = 0; i < MAX_WORLD_ITEMS; i++) {
        WorldItem *item = &worldItems[i];
        if (!item->active || item->collected || item->roomId != currentRoom) continue;

        float bobOffset = sinf(gameTime * ITEM_BOB_SPEED + item->bobPhase) * 5.0f;
        // Don't bob while still in spawn jump
        if (item->spawnVelY != 0.0f) bobOffset = 0.0f;
        float drawY = item->pos.y + bobOffset;

        // Draw shadow (pixel rectangle)
        DrawRectangle((int)(item->pos.x - 8), (int)(item->pos.y + 10), 16, 4,
                     (Color){ 0, 0, 0, 60 });

        // Draw item icon
        DrawItemIcon(item->type, item->pos.x - 14, drawY - 14, 4.5f);

        // Shop items: show name, description, and price
        if (item->price > 0) {
            const char *name = GetItemName(item->type);
            const char *desc = GetItemDesc(item->type);
            int nameW = MeasureText(name, 13);
            int descW = MeasureText(desc, 10);

            // Item name
            DrawText(name, (int)(item->pos.x - nameW / 2), (int)(item->pos.y - 32), 13,
                    (Color){ 220, 210, 180, 200 });
            // Description
            DrawText(desc, (int)(item->pos.x - descW / 2), (int)(item->pos.y - 20), 10,
                    (Color){ 160, 155, 150, 150 });

            // Price
            const char *priceText = TextFormat("%d", item->price);
            int tw = MeasureText(priceText, 14);
            DrawText(priceText, (int)(item->pos.x - tw / 2), (int)(item->pos.y + 20), 14,
                    (Color){ 230, 200, 50, 255 });
            DrawPixelCoin(item->pos.x + tw / 2 + 2, item->pos.y + 20, 1.2f,
                         (Color){ 230, 200, 50, 255 });
        }
        // Treasure room items: show name
        else if (curRoom->type == ROOM_TREASURE && item->type != ITEM_COIN) {
            const char *name = GetItemName(item->type);
            const char *desc = GetItemDesc(item->type);
            int nameW = MeasureText(name, 13);
            int descW = MeasureText(desc, 10);
            DrawText(name, (int)(item->pos.x - nameW / 2), (int)(item->pos.y + 18), 13,
                    (Color){ 220, 180, 50, 180 });
            DrawText(desc, (int)(item->pos.x - descW / 2), (int)(item->pos.y + 32), 10,
                    (Color){ 160, 155, 150, 130 });
        }
    }

    // "Not enough coins" floating message
    if (shopMessageTimer > 0.0f) {
        float alpha = shopMessageTimer > 1.0f ? 1.0f : shopMessageTimer;
        float floatUp = (1.5f - shopMessageTimer) * 30.0f;
        const char *msg = "Not enough coins!";
        int msgW = MeasureText(msg, 18);
        DrawText(msg, (int)(shopMessagePos.x - msgW / 2),
                (int)(shopMessagePos.y - 30 - floatUp), 18,
                Fade((Color){ 220, 80, 80, 255 }, alpha));
    }
}

void ItemDropFromEnemy(Vector2 pos, int roomId) {
    int roll = GetRandomValue(0, 99);

    if (roll < 35) {
        ItemSpawn(ITEM_COIN, pos, roomId, 0);
    } else if (roll < 45) {
        ItemSpawn(ITEM_HEART, pos, roomId, 0);
    } else if (roll < 48) {
        // Rare stat drops
        if (GetRandomValue(0, 1) == 0) {
            ItemSpawn(ITEM_DAMAGE_UP, pos, roomId, 0);
        } else {
            ItemSpawn(ITEM_SPEED_UP, pos, roomId, 0);
        }
    }
    // 52% chance: no drop
}

void ShopRoomSetup(int roomId) {
    float baseX = ROOM_X + ROOM_WIDTH * 0.25f;
    float y = ROOM_Y + ROOM_HEIGHT * 0.55f;
    float spacing = ROOM_WIDTH * 0.25f;

    ItemType shopItems[3] = { ITEM_HEART, ITEM_KEY, ITEM_BOMB };
    int prices[3] = { 3, 2, 3 };

    // Randomize one slot to be a stat upgrade
    int specialSlot = GetRandomValue(0, 2);
    if (GetRandomValue(0, 1) == 0) {
        shopItems[specialSlot] = ITEM_DAMAGE_UP;
        prices[specialSlot] = 5;
    } else {
        shopItems[specialSlot] = ITEM_SPEED_UP;
        prices[specialSlot] = 4;
    }

    for (int i = 0; i < 3; i++) {
        Vector2 pos = { baseX + spacing * i, y };
        ItemSpawn(shopItems[i], pos, roomId, prices[i]);
    }
}
