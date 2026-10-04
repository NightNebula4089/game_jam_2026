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
#include <functional>
#include <string>

struct Scene;

struct Interactable {
    std::string name;
    float x;
    float y;
    float width;
    float height;
    float range;                 // how close the player must be (walking scenes)
    Dialogue dialogue;           // what is shown while interacting
    std::string spritePath;      // sprite drawn for this interactable ("" = invisible)
    vita2d_texture *texture;
    bool active;                 // set by main.cpp every frame
    bool pickable;               // goes into the inventory when interacted with
    Scene *scene;                // scene to enter after the dialogue finishes
    std::vector<std::string> openText;     // lines shown normally
    std::vector<std::string> closedText;   // lines shown when requiredItem is missing
    std::string requiredItem;              // "" = nothing required
    std::function<void()> onInteract;      // runs when the normal dialogue finishes
    bool runLocked;              // true while the "item missing" lines are showing
    bool enabled;                // disabled interactables are invisible and ignore input
    bool removeAfterDialogue;    // picked-up item is removed after its pickup message

    Interactable(const std::string &name, float x, float y, float width, float height,
                 const Dialogue &dialogue, float range, const std::string &spritePath,
                 bool pickable = false, Scene *scene = nullptr,
                 const std::vector<std::string> &closedText = {},
                 const std::string &requiredItem = "",
                 std::function<void()> onInteract = nullptr,
                 bool runLocked = false)
        : name(name), x(x), y(y), width(width), height(height), range(range),
          dialogue(dialogue), spritePath(spritePath), texture(nullptr), active(false),
          pickable(pickable), scene(scene), openText(dialogue.text),
          closedText(closedText), requiredItem(requiredItem),
          onInteract(onInteract), runLocked(runLocked), enabled(true),
          removeAfterDialogue(false) {}
};

struct Scene {
    std::string name;
    std::vector<Dialogue> dialogues;
    std::string backgroundImagePath;
    bool parallaxEnabled = false;       // scrolling layered background
    bool pointClickEnabled = false;     // touch-screen close-up instead of walking
    std::vector<std::string> backgroundLayers;
    std::vector<vita2d_texture *> backgroundTextures;
    vita2d_texture *backgroundTexture = nullptr;
    std::vector<Interactable> interactables;
    int animFrame = 0;
    int animTimer = 0;
    int idleTimer = 0;
    bool playIdleAnimation = false;
    bool wasWalking = false;
    bool wasDead = false;
    float left_border;                  // minimum x for the player
    float right_border;                 // maximum x for the player
    float entryX = 480.0f;
    float entryY = 400.0f;
    float deathDistance = 0.0f;         // maximum distance from entry before death

    Scene(const std::string &name, const std::string &backgroundImagePath,
          float left_border, float right_border)
        : name(name), backgroundImagePath(backgroundImagePath),
          left_border(left_border), right_border(right_border) {}

    void addDialogue(const Dialogue &dialogue);
    void setParallaxEnabled();
    void addBackgroundLayer(const std::string &layerPath);
    void unsetParallaxEnabled();
    void setPointClickEnabled();
    void releaseBackgroundTextures();
    void addInteractable(const Interactable &interactable);
    void setBackground(const std::string &path);          // swap the background image
    Interactable *find(const std::string &name);          // nullptr if not found
    void drawScene(Character &player, vita2d_texture *deadTexture, vita2d_texture *idleTexture,
                   vita2d_texture *walkTexture, float cameraX, float groundY,
                   vita2d_pgf *font, vita2d_texture *crossButton);
};

struct Fade {
    float t = 1.0f, duration = 1.0f;     // progress 0..1 and length in frames
    float from = 255, to = 255;          // alpha at start and end
    float alpha = 255;

    // seconds is game time at 60 fps
    void start(float fromAlpha, float toAlpha, float seconds) {
        from = fromAlpha; to = toAlpha;
        duration = seconds * 60.0f;
        t = 0; alpha = from;
    }
    void update() {                        // call once per frame
        if (t >= 1.0f) return;
        t += 1.0f / duration;
        if (t > 1.0f) t = 1.0f;
        float e = t * t * (3.0f - 2.0f * t);   // smoothstep: eases in and out
        alpha = from + (to - from) * e;
    }
    bool done() const { return t >= 1.0f; }
    void draw() const {                    // call LAST, after everything else
        if (alpha <= 0.5f) return;
        vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(0, 0, 0, (int)alpha));
    }
};

#endif