#include "dungeon.h"
#include "utils.h"
#include "combat.h"
#include "items.h"
#include "boss.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
 * ============================================================================
 * DUNGEON GENERATION — Random Walk Algorithm
 * ============================================================================
 */

static int gridMap[DUNGEON_GRID_SIZE][DUNGEON_GRID_SIZE]; // -1 = empty, else room id

static unsigned int GenRand(unsigned int *state) {
    *state = *state * 1103515245u + 12345u;
    return (*state >> 16) & 0x7FFF;
}

void DungeonGenerate(Dungeon *dun, unsigned int seed) {
    memset(dun, 0, sizeof(Dungeon));
    memset(gridMap, -1, sizeof(gridMap));

    unsigned int rng = seed;

    // Place start room at center of grid
    int cx = DUNGEON_GRID_SIZE / 2;
    int cy = DUNGEON_GRID_SIZE / 2;

    dun->rooms[0].id = 0;
    dun->rooms[0].gridX = cx;
    dun->rooms[0].gridY = cy;
    dun->rooms[0].type = ROOM_START;
    dun->rooms[0].seed = GenRand(&rng);
    dun->rooms[0].distance = 0;
    for (int d = 0; d < 4; d++) dun->rooms[0].connections[d] = -1;
    for (int d = 0; d < 4; d++) dun->rooms[0].doorLocked[d] = false;
    gridMap[cx][cy] = 0;
    dun->roomCount = 1;
    dun->startRoomId = 0;

    // Direction offsets: N, E, S, W
    int dx[4] = { 0, 1, 0, -1 };
    int dy[4] = { -1, 0, 1, 0 };
    int opposite[4] = { DIR_SOUTH, DIR_WEST, DIR_NORTH, DIR_EAST };

    // Random walk to place rooms
    int curX = cx, curY = cy;
    int attempts = 0;
    while (dun->roomCount < TARGET_ROOM_COUNT && attempts < 200) {
        attempts++;
        int dir = GenRand(&rng) % 4;
        int nx = curX + dx[dir];
        int ny = curY + dy[dir];

        if (nx < 0 || nx >= DUNGEON_GRID_SIZE || ny < 0 || ny >= DUNGEON_GRID_SIZE) continue;

        if (gridMap[nx][ny] == -1) {
            // Place new room
            int id = dun->roomCount;
            dun->rooms[id].id = id;
            dun->rooms[id].gridX = nx;
            dun->rooms[id].gridY = ny;
            dun->rooms[id].seed = GenRand(&rng);
            for (int d = 0; d < 4; d++) dun->rooms[id].connections[d] = -1;
            for (int d = 0; d < 4; d++) dun->rooms[id].doorLocked[d] = false;
            gridMap[nx][ny] = id;

            // Connect to the room we walked from
            int fromId = gridMap[curX][curY];
            dun->rooms[fromId].connections[dir] = id;
            dun->rooms[id].connections[opposite[dir]] = fromId;

            dun->roomCount++;
        }

        curX = nx;
        curY = ny;
    }

    // Also connect any adjacent rooms that aren't connected yet
    for (int i = 0; i < dun->roomCount; i++) {
        int gx = dun->rooms[i].gridX;
        int gy = dun->rooms[i].gridY;
        for (int d = 0; d < 4; d++) {
            if (dun->rooms[i].connections[d] != -1) continue;
            int ax = gx + dx[d];
            int ay = gy + dy[d];
            if (ax >= 0 && ax < DUNGEON_GRID_SIZE && ay >= 0 && ay < DUNGEON_GRID_SIZE) {
                if (gridMap[ax][ay] != -1) {
                    int adjId = gridMap[ax][ay];
                    // Only connect if this would make sense (50% chance for extra connections)
                    if (GenRand(&rng) % 2 == 0) {
                        dun->rooms[i].connections[d] = adjId;
                        dun->rooms[adjId].connections[opposite[d]] = i;
                    }
                }
            }
        }
    }

    // BFS to compute distances from start
    int queue[MAX_DUNGEON_ROOMS];
    int qHead = 0, qTail = 0;
    for (int i = 0; i < dun->roomCount; i++) dun->rooms[i].distance = -1;
    dun->rooms[0].distance = 0;
    queue[qTail++] = 0;
    int farthestId = 0;
    int maxDist = 0;
    while (qHead < qTail) {
        int rid = queue[qHead++];
        for (int d = 0; d < 4; d++) {
            int nid = dun->rooms[rid].connections[d];
            if (nid >= 0 && dun->rooms[nid].distance == -1) {
                dun->rooms[nid].distance = dun->rooms[rid].distance + 1;
                queue[qTail++] = nid;
                if (dun->rooms[nid].distance > maxDist) {
                    maxDist = dun->rooms[nid].distance;
                    farthestId = nid;
                }
            }
        }
    }

    // Assign room types
    dun->bossRoomId = farthestId;
    dun->rooms[farthestId].type = ROOM_BOSS;

    // Find rooms for treasure and shop (mid-distance from start)
    bool assignedTreasure = false;
    bool assignedShop = false;
    for (int i = 1; i < dun->roomCount; i++) {
        if (i == farthestId) continue;
        if (!assignedTreasure && dun->rooms[i].distance >= 2 && dun->rooms[i].distance <= maxDist / 2 + 1) {
            dun->rooms[i].type = ROOM_TREASURE;
            assignedTreasure = true;
        } else if (!assignedShop && dun->rooms[i].distance >= 1 && assignedTreasure) {
            dun->rooms[i].type = ROOM_SHOP;
            assignedShop = true;
        }
    }

    // Everything else is combat
    for (int i = 1; i < dun->roomCount; i++) {
        if (dun->rooms[i].type == ROOM_NONE) {
            dun->rooms[i].type = ROOM_COMBAT;
        }
    }

    // Lock boss room door (the connection leading to the boss room)
    for (int d = 0; d < 4; d++) {
        int nid = dun->rooms[farthestId].connections[d];
        if (nid >= 0) {
            // Find which direction from neighbor leads to boss
            for (int d2 = 0; d2 < 4; d2++) {
                if (dun->rooms[nid].connections[d2] == farthestId) {
                    dun->rooms[nid].doorLocked[d2] = true;
                    break;
                }
            }
            dun->rooms[farthestId].doorLocked[d] = true;
            break; // Only lock one entrance
        }
    }

    // Initialize state
    dun->currentRoomId = 0;
    dun->previousRoomId = 0;
    dun->rooms[0].visited = true;
    dun->isTransitioning = false;
    dun->transitionTimer = 0.0f;
    dun->fadeAlpha = 0.0f;
    dun->cameraOffset = (Vector2){ 0.0f, 0.0f };
}

int DungeonGetRoomAtGrid(int gx, int gy) {
    if (gx < 0 || gx >= DUNGEON_GRID_SIZE || gy < 0 || gy >= DUNGEON_GRID_SIZE) return -1;
    return gridMap[gx][gy];
}

DungeonRoom* DungeonGetCurrentRoom(void) {
    return &dungeon.rooms[dungeon.currentRoomId];
}

DungeonRoom* DungeonGetRoom(int id) {
    if (id < 0 || id >= dungeon.roomCount) return NULL;
    return &dungeon.rooms[id];
}

Vector2 DungeonGetCameraOffset(void) {
    return dungeon.cameraOffset;
}

/*
 * ============================================================================
 * ROOM TRANSITIONS (Fade-based)
 * ============================================================================
 */

void DungeonCheckDoorways(void) {
    if (dungeon.isTransitioning) return;

    DungeonRoom *room = DungeonGetCurrentRoom();
    if (room->doorsLocked) return;

    float pr = player.radius;

    for (int d = 0; d < 4; d++) {
        int nextId = room->connections[d];
        if (nextId < 0) continue;

        bool collision = false;
        float doorCenterX = ROOM_X + ROOM_WIDTH * 0.5f;
        float doorCenterY = ROOM_Y + ROOM_HEIGHT * 0.5f;
        float wt = WALL_THICKNESS * TILE_SIZE;
        // Trigger transition when player walks into the wall zone through the doorway opening
        float triggerMargin = wt * 0.5f;

        switch (d) {
            case DIR_NORTH:
                collision = (player.pos.y <= ROOM_Y + wt + pr - triggerMargin &&
                            player.pos.x >= doorCenterX - DOORWAY_WIDTH * 0.5f &&
                            player.pos.x <= doorCenterX + DOORWAY_WIDTH * 0.5f);
                break;
            case DIR_SOUTH:
                collision = (player.pos.y >= ROOM_Y + ROOM_HEIGHT - wt - pr + triggerMargin &&
                            player.pos.x >= doorCenterX - DOORWAY_WIDTH * 0.5f &&
                            player.pos.x <= doorCenterX + DOORWAY_WIDTH * 0.5f);
                break;
            case DIR_WEST:
                collision = (player.pos.x <= ROOM_X + wt + pr - triggerMargin &&
                            player.pos.y >= doorCenterY - DOORWAY_HEIGHT * 0.5f &&
                            player.pos.y <= doorCenterY + DOORWAY_HEIGHT * 0.5f);
                break;
            case DIR_EAST:
                collision = (player.pos.x >= ROOM_X + ROOM_WIDTH - wt - pr + triggerMargin &&
                            player.pos.y >= doorCenterY - DOORWAY_HEIGHT * 0.5f &&
                            player.pos.y <= doorCenterY + DOORWAY_HEIGHT * 0.5f);
                break;
        }

        if (collision) {
            // Check if door is key-locked
            if (room->doorLocked[d]) {
                if (player.inventory.keys > 0) {
                    player.inventory.keys--;
                    room->doorLocked[d] = false;
                    // Also unlock from the other side
                    for (int d2 = 0; d2 < 4; d2++) {
                        if (dungeon.rooms[nextId].connections[d2] == room->id) {
                            dungeon.rooms[nextId].doorLocked[d2] = false;
                            break;
                        }
                    }
                } else {
                    // Bounce player back
                    float pushback = 5.0f;
                    switch (d) {
                        case DIR_NORTH: player.pos.y += pushback; break;
                        case DIR_SOUTH: player.pos.y -= pushback; break;
                        case DIR_WEST:  player.pos.x += pushback; break;
                        case DIR_EAST:  player.pos.x -= pushback; break;
                    }
                    return;
                }
            }

            // Start transition
            dungeon.previousRoomId = dungeon.currentRoomId;
            dungeon.currentRoomId = nextId;
            dungeon.isTransitioning = true;
            dungeon.transitionTimer = 0.0f;
            dungeon.transitionPhase = 0; // Fade out
            dungeon.transitionDirection = (Direction)d;
            dungeon.fadeAlpha = 0.0f;

            // Reposition player at opposite side of new room (inside walls)
            {
                float wallInset = WALL_THICKNESS * TILE_SIZE + player.radius + 15.0f;
                switch (d) {
                    case DIR_NORTH: player.pos.y = ROOM_Y + ROOM_HEIGHT - wallInset; break;
                    case DIR_EAST:  player.pos.x = ROOM_X + wallInset; break;
                    case DIR_SOUTH: player.pos.y = ROOM_Y + wallInset; break;
                    case DIR_WEST:  player.pos.x = ROOM_X + ROOM_WIDTH - wallInset; break;
                }
            }

            dungeon.rooms[nextId].visited = true;

            // Spawn enemies if room not cleared
            if (!dungeon.rooms[nextId].cleared && dungeon.rooms[nextId].type == ROOM_COMBAT) {
                DungeonSpawnRoomEnemies(nextId);
            }
            if (!dungeon.rooms[nextId].cleared && dungeon.rooms[nextId].type == ROOM_BOSS) {
                BossInit(nextId);
            }

            break;
        }
    }
}

void DungeonUpdate(float dt) {
    if (dungeon.isTransitioning) {
        dungeon.transitionTimer += dt;
        float halfDur = TRANSITION_DURATION * 0.5f;

        if (dungeon.transitionTimer < halfDur) {
            // Fade out
            dungeon.fadeAlpha = dungeon.transitionTimer / halfDur;
        } else {
            // Fade in
            dungeon.fadeAlpha = 1.0f - (dungeon.transitionTimer - halfDur) / halfDur;
        }

        if (dungeon.transitionTimer >= TRANSITION_DURATION) {
            dungeon.isTransitioning = false;
            dungeon.fadeAlpha = 0.0f;

            // Lock doors if entering combat room with enemies
            DungeonRoom *room = DungeonGetCurrentRoom();
            if (room->type == ROOM_COMBAT && !room->cleared) {
                room->doorsLocked = true;
            }
            if (room->type == ROOM_BOSS && !room->cleared) {
                room->doorsLocked = true;
            }
        }
    }

    // Check if current room combat is finished
    DungeonRoom *room = DungeonGetCurrentRoom();
    if (room->doorsLocked && (room->type == ROOM_COMBAT || room->type == ROOM_BOSS)) {
        bool allDead = true;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active && enemies[i].roomId == room->id) {
                allDead = false;
                break;
            }
        }
        // For boss room, also check boss
        if (room->type == ROOM_BOSS && boss.active) {
            allDead = false;
        }

        if (allDead) {
            room->doorsLocked = false;
            room->cleared = true;
        }
    }

    dungeon.cameraOffset = (Vector2){ 0.0f, 0.0f };
}

/*
 * ============================================================================
 * ENEMY SPAWN TEMPLATES
 * ============================================================================
 */

void DungeonSpawnRoomEnemies(int roomId) {
    DungeonRoom *room = &dungeon.rooms[roomId];
    if (room->cleared) return;

    int dist = room->distance;
    EnemyType types[MAX_ENEMIES_PER_ROOM];
    int count = 0;

    if (dist <= 1) {
        // Easy: 2-3 slimes
        count = 2 + (room->seed % 2);
        for (int i = 0; i < count; i++) types[i] = ENEMY_SLIME;
    } else if (dist <= 3) {
        // Medium: slimes and a skeleton
        count = 3 + (room->seed % 2);
        types[0] = ENEMY_SLIME;
        types[1] = ENEMY_SLIME;
        types[2] = ENEMY_SKELETON;
        if (count > 3) types[3] = ENEMY_SLIME;
    } else {
        // Hard: skeletons, slimes, maybe turret
        count = 4 + (room->seed % 2);
        if (count > MAX_ENEMIES_PER_ROOM) count = MAX_ENEMIES_PER_ROOM;
        types[0] = ENEMY_SKELETON;
        types[1] = ENEMY_SKELETON;
        types[2] = ENEMY_SLIME;
        types[3] = ENEMY_SLIME;
        if (count > 4) types[4] = ENEMY_TURRET;
        if (count > 5) types[5] = ENEMY_SKELETON;
    }

    room->enemyCount = count;

    for (int i = 0; i < count; i++) {
        // Find free slot
        int slot = -1;
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!enemies[j].active) { slot = j; break; }
        }
        if (slot < 0) break;

        float angle = (2.0f * PI / count) * i;
        float spawnRadius = 200.0f;
        Vector2 pos = {
            ROOM_X + ROOM_WIDTH * 0.5f + cosf(angle) * spawnRadius,
            ROOM_Y + ROOM_HEIGHT * 0.5f + sinf(angle) * spawnRadius
        };

        Enemy *e = &enemies[slot];
        memset(e, 0, sizeof(Enemy));
        e->pos = pos;
        e->active = true;
        e->roomId = roomId;
        e->type = types[i];
        e->facing = 0;
        e->stateTimer = -((float)i * 0.3f);
        e->preferredAngle = angle;
        e->sineOffset = (float)(room->seed + i) * 0.7f;

        switch (types[i]) {
            case ENEMY_SLIME:
                e->health = SLIME_HEALTH;
                e->maxHealth = SLIME_HEALTH;
                e->radius = SLIME_SIZE;
                e->circleRadius = 60.0f;
                break;
            case ENEMY_BAT:
                e->health = BAT_HEALTH;
                e->maxHealth = BAT_HEALTH;
                e->radius = BAT_SIZE;
                break;
            case ENEMY_SKELETON:
                e->health = SKELETON_HEALTH;
                e->maxHealth = SKELETON_HEALTH;
                e->radius = SKELETON_SIZE;
                break;
            case ENEMY_TURRET:
                e->health = TURRET_HEALTH;
                e->maxHealth = TURRET_HEALTH;
                e->radius = TURRET_SIZE;
                e->pos = (Vector2){
                    ROOM_X + ROOM_WIDTH * (0.2f + (float)(room->seed % 60) / 100.0f),
                    ROOM_Y + ROOM_HEIGHT * (0.2f + (float)((room->seed / 7) % 60) / 100.0f)
                };
                break;
            default: break;
        }
    }
}

/*
 * ============================================================================
 * DUNGEON DRAW — Renders the current room
 * ============================================================================
 */

void DungeonDraw(void) {
    // Drawing is handled by renderer.c via main.c orchestration
}
