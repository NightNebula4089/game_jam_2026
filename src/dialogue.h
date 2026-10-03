#ifndef DIALOGUE_H
#define DIALOGUE_H

#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <vita2d.h>
#include <sstream>
#include <vector>
#include <psp2/appmgr.h>
#include <player.h>
#include <cmath>
#include <cstdio>
#include <config.h>

struct Dialogue {
    std::vector<std::string> text;

    int visibleChars; // Number of characters currently visible
    int timer;

    int charsPerSecond; // How many characters to reveal per second

    bool active; // Is the dialogue currently active?
    bool triggered; // Has an automatic scene dialogue already been triggered?

    float x; // X position of the dialogue interaction

    float y; // Y position of the dialogue interaction

    int index; // Index of the current dialogue text being displayed

    bool interactable; // CDoes the player have to interact with something to trigger the dialogue?

    Dialogue(const std::vector<std::string> &text,float x,bool interactable)
        : text(text), visibleChars(0), timer(0), charsPerSecond(30), active(false), triggered(false), x(x), interactable(interactable), index(0) {}

    void update(const std::vector<std::string> &newText);
    void addText(const std::string &newText) {
        text.push_back(newText);
    };
    void animateDialogue(Dialogue &dialogue, vita2d_pgf *font, vita2d_texture *background, int color, float x, float y,int n); // Animate the dialogue text based on the timer and charsPerSecond

};

#endif



