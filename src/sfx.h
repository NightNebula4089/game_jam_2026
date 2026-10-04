#pragma once

// Tiny sound-effect player: WAV files (48 kHz, 16-bit, stereo) mixed on a
// background thread into one SceAudio port. Convert new sounds with:
//   ffmpeg -i in.ogg -ar 48000 -ac 2 -c:a pcm_s16le assets/sfx/out.wav

enum Sfx {
    SFX_FOOTSTEP_0,
    SFX_FOOTSTEP_1,
    SFX_FOOTSTEP_2,
    SFX_FOOTSTEP_3,
    SFX_FOOTSTEP_4,
    SFX_HURT,
    SFX_COUNT
};

void sfxInit();                              // load sounds and start the mixer
void sfxShutdown();                          // stop the mixer and free sounds
void sfxPlay(Sfx sound, float volume = 1.0f);
void sfxFootstep();                          // a random footstep variant
