#include <audio.h>

bool SoundManager::init() {
    if (engine.init(
            SoLoud::Soloud::CLIP_ROUNDOFF,
            SoLoud::Soloud::VITA_HOMEBREW,
            44100,
            2048,
            2
        ) != 0) {
        return false;
    }

    const char *glitchPaths[MENU_GLITCH_COUNT] = {
        "app0:assets/audio/glitch/glitch_crackle.wav",
    };

    for (int i = 0; i < MENU_GLITCH_COUNT; ++i) {
        menuLoaded[i] = menuGlitches[i].load(glitchPaths[i]) == 0;
    }
    footstepsLoaded = footsteps.load("app0:assets/audio/Steps_wood-003.wav") == 0;
    heartbeatLoaded = heartbeat.load("app0:assets/audio/heart_beat_human_a_slow.wav") == 0;
    ventLoaded = ventAirflow.load("app0:assets/audio/Vent_airflow.wav") == 0;
    deathLoaded = deathSound.load("app0:assets/audio/death_sound.wav") == 0;
    engine.setGlobalVolume(1.0f);
    return true;
}

void SoundManager::shutdown() {
    engine.deinit();
}

void SoundManager::updateMenu(bool active) {
    if (!active) {
        if (menuActive && engine.isValidVoiceHandle(menuVoice)) {
            engine.stop(menuVoice);
        }
        menuActive = false;
        menuTimer = 0;
        return;
    }

    if (!menuActive) {
        menuActive = true;
        menuIndex = 0;
        menuTimer = MENU_GLITCH_INTERVAL_FRAMES;
        if (menuLoaded[menuIndex]) {
            menuVoice = engine.play(menuGlitches[menuIndex]);
        }
        return;
    }

    if (--menuTimer <= 0) {
        menuTimer = MENU_GLITCH_INTERVAL_FRAMES;
        if (engine.isValidVoiceHandle(menuVoice)) {
            engine.stop(menuVoice);
        }
        if (menuLoaded[menuIndex]) {
            menuVoice = engine.play(menuGlitches[menuIndex]);
        }
    }
}

void SoundManager::updateDead(bool isDead) {
    if (isDead && !dead && deathLoaded) {
        engine.play(deathSound, 1.0f);
    }
    dead = isDead;
}

void SoundManager::updateGameplay(bool walking) {
    if (walking && footstepsLoaded) {
        if (++footstepTimer >= 18) {
            footstepTimer = 0;
            engine.play(footsteps, 0.65f);
        }
    } else {
        footstepTimer = 0;
    }

    if (heartbeatActiveTimer > 0) {
        --heartbeatActiveTimer;
        if (heartbeatActiveTimer == 0 && engine.isValidVoiceHandle(heartbeatVoice)) {
            engine.stop(heartbeatVoice);
        }
    } else if (heartbeatTimer > 0) {
        --heartbeatTimer;
    } else {
        if (heartbeatLoaded) {
            heartbeatVoice = engine.play(heartbeat, 0.8f);
        }
        heartbeatActiveTimer = HEARTBEAT_ACTIVE_FRAMES;
        heartbeatTimer = HEARTBEAT_INTERVAL_FRAMES;
    }
}

void SoundManager::playDeathSound(){
    if(dead) engine.play(deathSound, 1.0f);
    dead = false;
}

void SoundManager::playVentAirflow() {
    if (ventLoaded) engine.play(ventAirflow, 0.8f);
}