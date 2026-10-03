#ifndef SCENE_H
#define SCENE_H

#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <vita2d.h>
#include <sstream>
#include <vector>
#include <psp2/appmgr.h>
#include <dialogue.h>
#include <cstdio>
#include <string>

struct Interactable {
    std::string name;
    float x;
    float y;
    float width;
    float height;
    float range; // Range within which the player can interact with this object
    Dialogue dialogue; // Dialogue associated with this interactable
    std::string spritePath; // Path to the sprite representing the interactable
    vita2d_texture *texture;
    bool active; // Is the interactable currently active
    bool pickable; // Can the player collect this interactable?

    Interactable(const std::string &name, float x, float y, float width, float height, const Dialogue &dialogue, float range, const std::string &spritePath, bool pickable = false)
        : name(name), x(x), y(y), width(width), height(height), dialogue(dialogue), range(range), spritePath(spritePath), texture(nullptr), active(false), pickable(pickable) {}
};

struct Scene {
    std::string name;
    std::vector<Dialogue> dialogues;
    std::string backgroundImagePath;
    bool parallaxEnabled = false; // Enable or disable parallax scrolling
    std::vector<std::string> backgroundLayers; // Background layers associated with this interactable
    std::vector<vita2d_texture *> backgroundTextures;
    vita2d_texture *backgroundTexture = nullptr;
    std::vector<Interactable> interactables;
    int animFrame = 0;
    int animTimer = 0;
    int idleTimer = 0;
    bool playIdleAnimation = false;
    bool wasWalking = false;
    float left_border; // minimum x positiqon for the player
    float right_border; // maximum x position for the player

    Scene(const std::string &name, const std::string &backgroundImagePath,float left_border, float right_border)
        : name(name), backgroundImagePath(backgroundImagePath), left_border(left_border), right_border(right_border) {}

    void addDialogue(const Dialogue &dialogue);
    void setParallaxEnabled();
    void addBackgroundLayer(const std::string &layerPath);
    void unsetParallaxEnabled();
    void releaseBackgroundTextures();
    void addInteractable(const Interactable &interactable);
    void drawScene(Character &player,vita2d_texture *idleTexture,
                   vita2d_texture *walkTexture, float cameraX, float groundY,
                   vita2d_pgf *font, vita2d_texture *crossButton);

};

#endif

