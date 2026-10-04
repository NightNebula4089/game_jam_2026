#pragma once
#include <vita2d.h>

// A recurring "episode": a heartbeat that squeezes the vision mask in, then
// the view blurs and doubles for a moment before clearing up again.

// Seconds between episodes. Change freely, also at runtime (e.g. by health).
extern float episodeIntervalSeconds;

// Anything that should stop the heartbeat gets its own reason bit, so several
// can pause at once and the heartbeat only continues when all have resumed.
enum EpisodePauseReason {
    EPISODE_PAUSE_DIALOGUE = 1 << 0,
};
void episodePause(unsigned int reason);   // freeze timers; the effect holds where it is
void episodeResume(unsigned int reason);  // continue exactly where it stopped
bool episodePaused();

void episodeUpdate(bool alive);   // call once per frame while playing
void episodeReset();              // restart the interval (e.g. on a new game)
bool episodeJustStarted();        // true only on the frame an episode begins
float episodePulse();             // 0..1 heartbeat strength this frame
float episodeBlur();              // 0..1 blur strength this frame
float episodeVisionScale();       // multiply the vision radius by this (< 1 on a heartbeat)

// Offscreen texture to draw the scene into while blurring (created on first use)
vita2d_texture *episodeTarget();
// Draw the offscreen scene to the screen, blurred by episodeBlur()
void episodeDrawBlurred();
