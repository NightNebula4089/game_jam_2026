#include "sfx.h"
#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

static const int SAMPLE_RATE = 48000;
static const int GRAIN       = 256;     // stereo frames per output call (~5 ms)
static const int MAX_VOICES  = 8;       // sounds that can play at once

static const char *SFX_PATHS[SFX_COUNT] = {
    "app0:/assets/sfx/footstep_0.wav",
    "app0:/assets/sfx/footstep_1.wav",
    "app0:/assets/sfx/footstep_2.wav",
    "app0:/assets/sfx/footstep_3.wav",
    "app0:/assets/sfx/footstep_4.wav",
    "app0:/assets/sfx/hurt.wav",
};

struct Sound {
    int16_t *samples = nullptr;   // interleaved L/R
    int frames = 0;
};

struct Voice {
    const Sound *sound = nullptr; // nullptr = free
    int pos = 0;                  // next frame to play
    int volume = 0;               // 0..256
};

static Sound sounds[SFX_COUNT];
static Voice voices[MAX_VOICES];
static SceKernelLwMutexWork voiceLock;
static SceUID thread = -1;
static int port = -1;
static volatile bool running = false;

static uint32_t readU32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint16_t readU16(const uint8_t *p) { return p[0] | (p[1] << 8); }

// Load a 48 kHz 16-bit stereo PCM WAV. Leaves the sound empty on any mismatch.
static void loadWav(Sound &sound, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = (uint8_t *)malloc(size);
    if (!data || fread(data, 1, size, f) != (size_t)size || size < 12 ||
        memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "WAVE", 4) != 0) {
        free(data);
        fclose(f);
        return;
    }
    fclose(f);

    bool formatOk = false;
    long off = 12;
    while (off + 8 <= size) {
        uint32_t len = readU32(data + off + 4);
        const uint8_t *body = data + off + 8;
        if (memcmp(data + off, "fmt ", 4) == 0 && len >= 16) {
            formatOk = readU16(body) == 1 && readU16(body + 2) == 2 &&
                       readU32(body + 4) == SAMPLE_RATE && readU16(body + 14) == 16;
        } else if (memcmp(data + off, "data", 4) == 0 && formatOk) {
            if (len > (uint32_t)(size - off - 8)) len = size - off - 8;
            sound.frames = len / 4;
            sound.samples = (int16_t *)malloc(sound.frames * 4);
            if (sound.samples) memcpy(sound.samples, body, sound.frames * 4);
            else sound.frames = 0;
            break;
        }
        off += 8 + len + (len & 1);
    }
    free(data);
}

static int mixerThread(SceSize, void *) {
    static int16_t out[2][GRAIN * 2];
    int buf = 0;
    int32_t mix[GRAIN * 2];

    while (running) {
        memset(mix, 0, sizeof(mix));
        sceKernelLockLwMutex(&voiceLock, 1, nullptr);
        for (Voice &v : voices) {
            if (!v.sound) continue;
            int n = v.sound->frames - v.pos;
            if (n > GRAIN) n = GRAIN;
            const int16_t *src = v.sound->samples + v.pos * 2;
            for (int i = 0; i < n * 2; ++i) mix[i] += (src[i] * v.volume) >> 8;
            v.pos += n;
            if (v.pos >= v.sound->frames) v.sound = nullptr;
        }
        sceKernelUnlockLwMutex(&voiceLock, 1);

        for (int i = 0; i < GRAIN * 2; ++i) {
            int32_t s = mix[i];
            out[buf][i] = s > 32767 ? 32767 : s < -32768 ? -32768 : s;
        }
        sceAudioOutOutput(port, out[buf]);   // blocks until the port wants more
        buf ^= 1;
    }
    sceAudioOutOutput(port, nullptr);        // wait for the last buffer to finish
    return 0;
}

void sfxInit() {
    for (int i = 0; i < SFX_COUNT; ++i) loadWav(sounds[i], SFX_PATHS[i]);

    port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, GRAIN, SAMPLE_RATE, SCE_AUDIO_OUT_MODE_STEREO);
    if (port < 0) return;
    int vol[2] = {SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB};
    sceAudioOutSetVolume(port, (SceAudioOutChannelFlag)(SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH), vol);

    sceKernelCreateLwMutex(&voiceLock, "sfx_voices", 0, 0, nullptr);
    running = true;
    thread = sceKernelCreateThread("sfx_mixer", mixerThread, 0x10000100, 0x4000, 0, 0, nullptr);
    if (thread < 0) { running = false; return; }
    sceKernelStartThread(thread, 0, nullptr);
}

void sfxShutdown() {
    if (thread >= 0) {
        running = false;
        sceKernelWaitThreadEnd(thread, nullptr, nullptr);
        sceKernelDeleteThread(thread);
        sceKernelDeleteLwMutex(&voiceLock);
        thread = -1;
    }
    if (port >= 0) { sceAudioOutReleasePort(port); port = -1; }
    for (Sound &s : sounds) { free(s.samples); s.samples = nullptr; s.frames = 0; }
}

void sfxPlay(Sfx sound, float volume) {
    if (!running || sound < 0 || sound >= SFX_COUNT || !sounds[sound].samples) return;
    int vol = (int)(volume * 256.0f);
    vol = vol < 0 ? 0 : vol > 256 ? 256 : vol;

    sceKernelLockLwMutex(&voiceLock, 1, nullptr);
    // Use a free voice, or steal the one that has played the longest
    Voice *pick = &voices[0];
    for (Voice &v : voices) {
        if (!v.sound) { pick = &v; break; }
        if (v.pos > pick->pos) pick = &v;
    }
    pick->sound = &sounds[sound];
    pick->pos = 0;
    pick->volume = vol;
    sceKernelUnlockLwMutex(&voiceLock, 1);
}

void sfxFootstep() {
    static int last = -1;
    int i = rand() % 5;
    if (i == last) i = (i + 1) % 5;   // never the same step twice in a row
    last = i;
    sfxPlay((Sfx)(SFX_FOOTSTEP_0 + i), 0.6f);
}
