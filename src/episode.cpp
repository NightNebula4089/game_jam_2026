#include "episode.h"
#include <math.h>

static const int   SCREEN_W = 960;
static const int   SCREEN_H = 544;
static const int   EPISODE_FRAMES   = 100;      // how long one episode lasts
static const int   BLUR_START       = 10;       // blur begins just after the first heartbeat
static const float BLUR_RADIUS      = 3.5f;     // px spread of the blur copies at full strength
static const float DOUBLE_VISION    = 8.0f;     // px drift of the ghost image at full strength
static const float HEARTBEAT_SQUEEZE = 0.22f;   // how far a heartbeat closes the vision mask (0..1)
static const float PI = 3.14159265f;

float episodeIntervalSeconds = 2.0f;           // TESTING: set to 10.0f

static int intervalTimer = 0;
static int episodeFrame  = EPISODE_FRAMES;      // >= EPISODE_FRAMES means no episode running
static vita2d_texture *target = nullptr;
static unsigned int pauseReasons = 0;

// Gaussian bump centred on frame c
static float beat(float t, float c, float width) {
    float d = (t - c) / width;
    return expf(-d * d);
}

void episodePause(unsigned int reason)  { pauseReasons |= reason; }
void episodeResume(unsigned int reason) { pauseReasons &= ~reason; }
bool episodePaused()                    { return pauseReasons != 0; }

void episodeUpdate(bool alive) {
    if (episodePaused()) return;          // paused time does not count
    if (!alive) { episodeFrame = EPISODE_FRAMES; return; }
    if (episodeFrame < EPISODE_FRAMES) ++episodeFrame;
    if (++intervalTimer >= (int)(episodeIntervalSeconds * 60.0f)) {
        intervalTimer = 0;
        episodeFrame  = 0;
    }
}

void episodeReset() {
    intervalTimer = 0;
    episodeFrame  = EPISODE_FRAMES;
}

float episodePulse() {
    if (episodeFrame >= EPISODE_FRAMES) return 0.0f;
    float t = (float)episodeFrame;
    // "lub-dub": a strong beat then a softer one
    float p = beat(t, 4.0f, 3.5f) + 0.7f * beat(t, 18.0f, 3.5f);
    return p > 1.0f ? 1.0f : p;
}

float episodeVisionScale() {
    return 1.0f - HEARTBEAT_SQUEEZE * episodePulse();
}

float episodeBlur() {
    if (episodeFrame < BLUR_START || episodeFrame >= EPISODE_FRAMES) return 0.0f;
    float u = (float)(episodeFrame - BLUR_START) / (EPISODE_FRAMES - BLUR_START);
    return sinf(PI * u);   // ease in, ease out
}

vita2d_texture *episodeTarget() {
    if (!target) {
        target = vita2d_create_empty_texture_rendertarget(SCREEN_W, SCREEN_H, SCE_GXM_TEXTURE_FORMAT_A8B8G8R8);
    }
    return target;
}

void episodeDrawBlurred() {
    if (!target) return;
    float b = episodeBlur();

    vita2d_draw_texture(target, 0, 0);

    // Ring of faint offset copies smears the image
    float r = BLUR_RADIUS * b;
    unsigned int a = (unsigned int)(255 * 0.12f * b);
    for (int i = 0; i < 8; ++i) {
        float ang = i * (PI / 4.0f) + episodeFrame * 0.05f;
        vita2d_draw_texture_tint(target, r * cosf(ang), r * sinf(ang), RGBA8(255, 255, 255, a));
    }

    // Swaying ghost image for double vision
    float drift = DOUBLE_VISION * b * sinf(episodeFrame * 0.12f);
    vita2d_draw_texture_tint(target, drift, 0, RGBA8(255, 255, 255, (unsigned int)(255 * 0.18f * b)));

    // Dim slightly while vision is lost
    vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, RGBA8(0, 0, 0, (unsigned int)(30 * b)));
}

