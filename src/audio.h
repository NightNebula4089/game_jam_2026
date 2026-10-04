#ifndef AUDIO_H
#define AUDIO_H

#include <soloud.h>
#include <soloud_wav.h>

class SoundManager {
public:
    bool init();
    void shutdown();
    void updateMenu(bool active);
    void updateDead(bool isDead);
    void updateGameplay(bool walking);
    void playVentAirflow();
    void playDeathSound();

private:
    static const int MENU_GLITCH_COUNT = 1;
    static const int MENU_GLITCH_INTERVAL_FRAMES = 600;
    static const int HEARTBEAT_ACTIVE_FRAMES = 600;
    static const int HEARTBEAT_INTERVAL_FRAMES = 1800;

    SoLoud::Soloud engine;
    SoLoud::Wav menuGlitches[MENU_GLITCH_COUNT];
    SoLoud::Wav footsteps;
    SoLoud::Wav heartbeat;
    SoLoud::Wav ventAirflow;
    SoLoud::Wav deathSound;
    SoLoud::handle menuVoice = 0;
    SoLoud::handle heartbeatVoice = 0;
    bool menuLoaded[MENU_GLITCH_COUNT] = {};
    bool footstepsLoaded = false;
    bool heartbeatLoaded = false;
    bool ventLoaded = false;
    bool deathLoaded = false;
    bool dead = false;
    int menuIndex = 0;
    int menuTimer = 0;
    int footstepTimer = 0;
    int heartbeatTimer = HEARTBEAT_INTERVAL_FRAMES;
    int heartbeatActiveTimer = 0;
    bool menuActive = false;
};

#endif