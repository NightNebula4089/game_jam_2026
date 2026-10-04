#ifndef WORLD_H
#define WORLD_H

#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <vita2d.h>
#include <sstream>
#include <vector>
#include <psp2/appmgr.h>
#include <dialogue.h>
#include <cstdio>
#include <scene.h>
#include <string>

enum GameStateEnum {
    MainMenuState,
    Playing,
    Paused,
    GameOver,
    Exit
};

struct GameState {
    GameStateEnum state;
    bool holeOpen = false, 
    gameWon = false;

    explicit GameState(GameStateEnum state)
        :  state(state) {};
};

#endif