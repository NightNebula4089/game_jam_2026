#ifndef MENU_H
#define MENU_H

#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <vita2d.h>
#include <sstream>
#include <vector>
#include <psp2/appmgr.h>
#include <string>

struct MainMenu {

    std ::vector<std::string> options;
    int selectedOption;
    std::vector<vita2d_texture *> backgroundTextures;
    int backgroundIndex;
    int backgroundTimer;

    MainMenu(const std::vector<std::string> &options)
        : options(options), selectedOption(0), backgroundIndex(0), backgroundTimer(0) {}
    
    void loadTextures();
    void releaseTextures();
    int updateState(SceCtrlData &pad);
    void updateMenuSelection(int direction);
    void draw(vita2d_font *font);

};

#endif