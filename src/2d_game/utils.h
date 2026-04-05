#ifndef UTILS_H
#define UTILS_H

#include "raylib.h"
#include <math.h>

/*
 * ============================================================================
 * MATH UTILITIES (static inline for zero-overhead)
 * ============================================================================
 */

static inline float Clamp(float v, float mn, float mx) {
    return v < mn ? mn : (v > mx ? mx : v);
}

static inline float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

static inline float NormalizeAngle(float angle) {
    while (angle > PI) angle -= 2.0f * PI;
    while (angle < -PI) angle += 2.0f * PI;
    return angle;
}

static inline Vector2 Vec2Add(Vector2 a, Vector2 b) {
    return (Vector2){ a.x + b.x, a.y + b.y };
}

static inline Vector2 Vec2Sub(Vector2 a, Vector2 b) {
    return (Vector2){ a.x - b.x, a.y - b.y };
}

static inline Vector2 Vec2Scale(Vector2 v, float s) {
    return (Vector2){ v.x * s, v.y * s };
}

static inline float Vec2Length(Vector2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

static inline Vector2 Vec2Normalize(Vector2 v) {
    float len = Vec2Length(v);
    if (len > 0.0f) return (Vector2){ v.x / len, v.y / len };
    return (Vector2){ 0.0f, 0.0f };
}

static inline float Vec2Distance(Vector2 a, Vector2 b) {
    return Vec2Length(Vec2Sub(b, a));
}

/*
 * ============================================================================
 * COLLISION & PHYSICS UTILITIES
 * ============================================================================
 */

static inline bool CircleCollision(Vector2 p1, float r1, Vector2 p2, float r2) {
    return Vec2Distance(p1, p2) < (r1 + r2);
}

static inline void ResolveCircleCollision(Vector2 *pos1, float r1,
                                           Vector2 *pos2, float r2) {
    Vector2 delta = Vec2Sub(*pos2, *pos1);
    float dist = Vec2Length(delta);
    if (dist < r1 + r2 && dist > 0.0f) {
        Vector2 normal = Vec2Normalize(delta);
        float overlap = (r1 + r2) - dist;
        Vector2 separation = Vec2Scale(normal, overlap * 0.5f);
        *pos1 = Vec2Sub(*pos1, separation);
        *pos2 = Vec2Add(*pos2, separation);
    }
}

static inline void ApplyKnockback(Vector2 *knockback, Vector2 direction, float strength) {
    Vector2 impulse = Vec2Scale(Vec2Normalize(direction), strength);
    *knockback = Vec2Add(*knockback, impulse);
}

/*
 * ============================================================================
 * SIMPLE HASH FOR DETERMINISTIC RANDOMNESS
 * ============================================================================
 */

static inline unsigned int HashTile(unsigned int seed, int x, int y) {
    unsigned int h = seed;
    h ^= (unsigned int)x * 374761393u;
    h ^= (unsigned int)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

/*
 * ============================================================================
 * EASING FUNCTIONS
 * ============================================================================
 */

static inline float EaseCubicInOut(float t) {
    return t < 0.5f
        ? 4.0f * t * t * t
        : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

static inline float EaseOutBack(float t) {
    float c1 = 1.70158f;
    float c3 = c1 + 1.0f;
    return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
}

#endif // UTILS_H
