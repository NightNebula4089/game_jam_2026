#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <vita2d.h>
#include <sstream>
#include <vector>
#include <psp2/appmgr.h>
#include <player.h>
#include <cmath>
#include <cstdio>
#include <dialogue.h>
#include <algorithm>
#include <config.h>


void Dialogue::animateDialogue(Dialogue &dialogue, vita2d_pgf *font, vita2d_texture *background, int color,float x,float y,int n) {
    if (!dialogue.active) return;

    // Update the timer
    dialogue.timer++;

    // Calculate how many characters should be visible based on the timer and charsPerSecond
    int charsToShow = (dialogue.timer * dialogue.charsPerSecond) / 60; // Assuming 60 FPS

    // Update visibleChars but do not exceed the length of the text
    dialogue.visibleChars = std::min(charsToShow, static_cast<int>(dialogue.text[n].length()));

    // Draw the background if provided
    if (background) {
        vita2d_draw_texture(background, x, y);
    }

    // Draw the text with only the visible characters
    std::string visibleText = dialogue.text[n].substr(0, dialogue.visibleChars);
    vita2d_pgf_draw_text(font, x + 10, y + 50, color, 1.0f, visibleText.c_str());
}

void Dialogue::update(const std::vector<std::string> &newText) {
    text = newText;
    index = 0;
    visibleChars = 0;
    timer = 0;
    if(!active) {
        active = true;
    }
}


