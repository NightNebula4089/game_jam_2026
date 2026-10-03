#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <vita2d.h>
#include <sstream>
#include <vector>
#include <psp2/appmgr.h>
#include <string>
#include <menu.h>

static const int SCREEN_W = 960;
static const int SCREEN_H = 544;
static const unsigned int MENU_FONT_SIZE = 32;

static std::string backgroundPictures[] = {
    "app0:assets/sprites/backgrounds/main_menu/bg_4.png",
    "app0:assets/sprites/backgrounds/main_menu/bg_10.png",
    "app0:assets/sprites/backgrounds/main_menu/bg_11.png",
    "app0:assets/sprites/backgrounds/main_menu/bg_5.png",
    "app0:assets/sprites/backgrounds/main_menu/bg_14.png",
};

static int BACKGROUND_COUNT = sizeof(backgroundPictures) / sizeof(backgroundPictures[0]);

void MainMenu::updateMenuSelection(int direction) {
    selectedOption += direction;
    if (selectedOption < 0) {
        selectedOption = options.size() - 1; // Wrap around to the last option
    } else if (selectedOption >= options.size()) {
        selectedOption = 0; // Wrap around to the first option
    }
}

void MainMenu::loadTextures() {
	if (!backgroundTextures.empty()) return;
    for (const std::string &path : backgroundPictures) {
        vita2d_texture *texture = vita2d_load_PNG_file(path.c_str());
        if (texture) {
            backgroundTextures.push_back(texture);
        }
    }
}

void MainMenu::releaseTextures() {
    for (vita2d_texture *texture : backgroundTextures) {
        if (texture) {
            vita2d_free_texture(texture);
        }
    }
    backgroundTextures.clear();
}

int MainMenu::updateState(SceCtrlData &pad) {
    if (pad.buttons & SCE_CTRL_UP) {
        updateMenuSelection(-1);
    } else if (pad.buttons & SCE_CTRL_DOWN) {
        updateMenuSelection(1);
    } else if (pad.buttons & SCE_CTRL_CROSS) {
        return selectedOption; // Return the index of the selected option
    }
    return -1; // No selection made
}

void MainMenu::draw(vita2d_font *font) {
    if (!backgroundTextures.empty()) {
        vita2d_texture *background = backgroundTextures[backgroundIndex];
        const float scaleX = 960.0f / vita2d_texture_get_width(background);
        const float scaleY = 544.0f / vita2d_texture_get_height(background);
        vita2d_draw_texture_scale(background, 0.0f, 0.0f, scaleX, scaleY);

        if (++backgroundTimer >= 20) {
            backgroundTimer = 0;
            backgroundIndex = (backgroundIndex + 1) % BACKGROUND_COUNT;
        }
    }

    for (size_t i = 0; i < options.size(); ++i) {
        int color = i == static_cast<size_t>(selectedOption)
            ? RGBA8(255, 255, 0, 255)
            : RGBA8(255, 255, 255, 255);
        vita2d_font_draw_text(font, SCREEN_W / 2, SCREEN_H / 2 - static_cast<int>(options.size() * 25) + static_cast<int>(i) * 50,
                     color, MENU_FONT_SIZE, options[i].c_str());
    }
}




