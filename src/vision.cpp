#include "vision.h"
#include <vita2d.h>
#include <math.h>

static const int   SCREEN_W = 960;
static const int   SCREEN_H = 544;
static const float MASK_PX  = 512.0f;   // size of vision_mask.png
static const float R_MAX    = 1500.0f;  // outer radius at full health (edges barely dim)
static const float R_MIN    = 340.0f;   // outer radius near death (clear hole is 35% of this)

static vita2d_texture *mask = nullptr;
static bool  loadTried = false;
static float shownR = R_MAX;
static int   frameCount = 0;

static void block(float x, float y, float w, float h) {
    if (w > 0 && h > 0) vita2d_draw_rectangle(x, y, w, h, RGBA8(0, 0, 0, 255));
}

void drawVision(float cx, float cy, float healthFrac) {
    if (!loadTried) {                       // loaded once, on first use
        mask = vita2d_load_PNG_file("app0:/assets/ui/vision_mask.png");
        loadTried = true;
    }
    if (healthFrac < 0) healthFrac = 0;
    if (healthFrac > 1) healthFrac = 1;

    // Vision stays wide at first, then closes in faster as health runs out
    float target = R_MIN + (R_MAX - R_MIN) * healthFrac * healthFrac;
    shownR += (target - shownR) * 0.05f;    // glide toward the target, no sudden jumps

    // Slow "heartbeat" pulse that speeds up as health drops
    ++frameCount;
    float pulse = 1.0f + 0.02f * sinf(frameCount * (0.05f + 0.10f * (1.0f - healthFrac)));
    float R = shownR * pulse;

    float x0 = cx - R, y0 = cy - R, x1 = cx + R, y1 = cy + R;

    if (mask) {
        float s = (2.0f * R) / MASK_PX;
        vita2d_draw_texture_scale(mask, x0, y0, s, s);
    }
    // The mask only covers a square around the player: fill the rest with solid black
    block(0, 0, SCREEN_W, y0);                 // above
    block(0, y1, SCREEN_W, SCREEN_H - y1);     // below
    block(0, y0, x0, y1 - y0);                 // left
    block(x1, y0, SCREEN_W - x1, y1 - y0);     // right
}