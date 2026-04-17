#ifndef PALETTE_H
#define PALETTE_H

#include "raylib.h"

/*
 * ============================================================================
 * DUCK QUEST — Unified 8-bit Palette
 * ============================================================================
 *
 * A small, hand-tuned 16-color palette (DawnBringer-inspired) used everywhere
 * menus, HUD and overlays need color. Keeping every UI color inside this
 * palette is what makes the game feel artistically harmonic rather than
 * random-hex-soup. Gameplay entities (player, enemies, boss) keep their
 * existing tuned palettes — this header is for UI / screens / overlays.
 *
 * Values are all fully opaque. Call PAL_ALPHA(c, a) to apply opacity.
 *
 * Usage:
 *     DrawRectangle(x, y, w, h, PAL_INK);
 *     DrawRectangle(x, y, w, h, PAL_ALPHA(PAL_INK, 180));
 * ============================================================================
 */

#define PAL_RGB(r, g, b)      ((Color){ (r), (g), (b), 255 })
#define PAL_ALPHA(c, a)       ((Color){ (c).r, (c).g, (c).b, (unsigned char)(a) })

// Neutrals — frames, text, panels
#define PAL_INK               PAL_RGB( 18,  14,  24)   // darkest — outlines, shadows
#define PAL_NIGHT             PAL_RGB( 28,  22,  38)   // panel fill
#define PAL_DUSK              PAL_RGB( 50,  42,  66)   // panel border
#define PAL_STONE             PAL_RGB( 96,  88, 110)   // disabled text, inactive
#define PAL_ASH               PAL_RGB(156, 150, 166)   // secondary text
#define PAL_BONE              PAL_RGB(230, 222, 210)   // primary text
#define PAL_SNOW              PAL_RGB(250, 248, 240)   // highlight text

// Warms — gold, heart, fire, warning
#define PAL_GOLD              PAL_RGB(240, 200,  70)   // gold, coins, titles
#define PAL_AMBER             PAL_RGB(210, 140,  50)   // secondary gold
#define PAL_EMBER             PAL_RGB(200,  85,  55)   // fire, danger-warm
#define PAL_BLOOD             PAL_RGB(200,  55,  55)   // hearts, damage
#define PAL_WINE              PAL_RGB(120,  35,  50)   // dark red, gameover tint

// Cools — ice, magic, shop, UI accents
#define PAL_SKY               PAL_RGB(100, 180, 220)   // shop, shield, info
#define PAL_TIDE              PAL_RGB( 55, 110, 165)   // darker blue
#define PAL_MOSS              PAL_RGB(100, 180, 120)   // success, sanctuary
#define PAL_IVY               PAL_RGB( 55, 110,  80)   // dark green
#define PAL_VIOLET            PAL_RGB(150, 100, 190)   // rare / magic

#endif // PALETTE_H
