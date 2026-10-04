#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <vita2d.h>
#include <sstream>
#include <vector>
#include <psp2/appmgr.h>
#include <cstdio>
#include <player.h>
#include <scene.h>
#include <cmath>
#include <vision.h>

void Scene::addDialogue(const Dialogue &dialogue) {
    dialogues.push_back(dialogue);
}

void Scene::addInteractable(const Interactable &interactable) {
    interactables.push_back(interactable);
}

void Scene::addBackgroundLayer(const std::string &layerPath) {
    backgroundLayers.push_back(layerPath);
}

void Scene::setParallaxEnabled()   { parallaxEnabled = true; }
void Scene::unsetParallaxEnabled() { parallaxEnabled = false; }
void Scene::setPointClickEnabled() { pointClickEnabled = true; }

void Scene::setBackground(const std::string &path) {
    vita2d_wait_rendering_done();            // the GPU may still be using the old texture
    if (backgroundTexture) {
        vita2d_free_texture(backgroundTexture);
        backgroundTexture = nullptr;         // drawScene reloads it from the new path
    }
    backgroundImagePath = path;
}

Interactable *Scene::find(const std::string &n) {
    for (auto &ia : interactables) {
        if (ia.name == n) return &ia;
    }
    return nullptr;
}

void Scene::releaseBackgroundTextures() {
    if (backgroundTexture) {
        vita2d_free_texture(backgroundTexture);
        backgroundTexture = nullptr;
    }
    for (vita2d_texture *texture : backgroundTextures) {
        if (texture) vita2d_free_texture(texture);
    }
    backgroundTextures.clear();

    for (auto &interactable : interactables) {
        if (interactable.texture) {
            vita2d_free_texture(interactable.texture);
            interactable.texture = nullptr;
        }
    }
}

// "Press [X] to interact with <target>", bottom right
static void drawInteractPrompt(vita2d_pgf *font, vita2d_texture *icon, const char *target) {
    const int SCREEN_W = 960;
    const int SCREEN_H = 544;
    const unsigned int white = RGBA8(255, 255, 255, 255);
    const float scale    = 1.0f;
    const float iconSize = 32.0f;
    const float gap      = 8.0f;

    const char *before = "Press";
    char after[128];
    snprintf(after, sizeof(after), "to interact with %s", target);

    float wBefore = vita2d_pgf_text_width(font, scale, before);
    float wAfter  = vita2d_pgf_text_width(font, scale, after);
    float textH   = vita2d_pgf_text_height(font, scale, before);

    float total    = wBefore + gap + iconSize + gap + wAfter;
    float x        = SCREEN_W - total - 20.0f;
    float baseline = SCREEN_H - 50.0f;
    float iconY    = baseline - textH / 2.0f - iconSize / 2.0f;

    vita2d_pgf_draw_text(font, x, baseline, white, scale, before);
    x += wBefore + gap;

    if (icon) {
        float sx = iconSize / vita2d_texture_get_width(icon);
        float sy = iconSize / vita2d_texture_get_height(icon);
        vita2d_draw_texture_scale(icon, x, iconY, sx, sy);
    }
    x += iconSize + gap;

    vita2d_pgf_draw_text(font, x, baseline, white, scale, after);
}

// Small X icon in the corner of a dialogue box ("press to continue")
static void drawAdvanceIcon(vita2d_texture *icon) {
    if (!icon) return;
    const float size = 40.0f;
    float sx = size / vita2d_texture_get_width(icon);
    float sy = size / vita2d_texture_get_height(icon);
    vita2d_draw_texture_scale(icon, 920 - size - 10, 510 - size - 10, sx, sy);
}

void Scene::drawScene(Character &player, vita2d_texture *deadTexture, vita2d_texture *idleTexture,
                      vita2d_texture *walkTexture, float cameraX, float groundY,
                      vita2d_pgf *font, vita2d_texture *crossButton) {
    static const int   FRAME_W = 128;
    static const int   FRAME_H = 128;
    static const int   IDLE_FRAMES = 11;
    static const int   WALK_FRAMES = 8;
    static const int   DEAD_FRAMES = 4;
    static const int   ANIM_SPEED = 6;
    static const int   DEAD_ANIM_SPEED = 12;     // slower, so the collapse is readable
    static const int   IDLE_LIMIT = 300;
    static const float PLAYER_SCALE = 2.5f;
    static const int   SCREEN_W = 960;
    static const int   SCREEN_H = 544;
    static const int   BG_W = 928;
    static const int   BG_H = 793;
    static const float BG_SCALE = 1.0f;
    static const float BG_CROP_Y = 249.0f;
    static const bool  DEBUG_BOXES = false;      // true = green boxes on touch zones
    static const float layerScroll[] = {
        0.0f, 0.3f, 0.5f, 0.5f, 0.7f, 0.8f,
        0.8f, 0.85f, 0.9f, 1.0f, 1.0f
    };

    // main.cpp sets `active` every frame (in range, or touched)
    Interactable *activeInteractable = nullptr;
    for (auto &ia : interactables) {
        if (ia.enabled && ia.active) {
            activeInteractable = &ia;
            break;
        }
    }

    // ---------------- background ----------------
    if (parallaxEnabled) {
        if (backgroundTextures.size() != backgroundLayers.size()) {
            backgroundTextures.clear();
            for (const std::string &path : backgroundLayers) {
                backgroundTextures.push_back(vita2d_load_PNG_file(path.c_str()));
            }
        }

        for (size_t i = 0; i < backgroundTextures.size(); ++i) {
            vita2d_texture *texture = backgroundTextures[i];
            if (!texture) continue;

            const float speed = layerScroll[i < 11 ? i : 10];
            float offset = std::fmod(cameraX * speed, BG_W * BG_SCALE);
            if (offset < 0.0f) offset += BG_W * BG_SCALE;

            for (float x = (SCREEN_W - BG_W) / 2.0f - offset - BG_W * BG_SCALE;
                 x < SCREEN_W;
                 x += BG_W * BG_SCALE) {
                vita2d_draw_texture_part_scale(texture, x, -BG_CROP_Y, 0, 0,
                                               BG_W, BG_H, BG_SCALE, BG_SCALE);
            }
        }
    } else {
        if (!backgroundTexture && !backgroundImagePath.empty()) {
            backgroundTexture = vita2d_load_PNG_file(backgroundImagePath.c_str());
        }
        if (backgroundTexture) {
            vita2d_draw_texture(backgroundTexture, 0.0f, 0.0f);
        }
    }

    // ---------------- player (walking scenes only) ----------------
    if (!pointClickEnabled) {
        const bool walking = player.state == Walking;
        const bool dead    = player.state == Dead;

        if (dead) {
            playIdleAnimation = false;
            if (!wasDead) {
                animFrame = 0;
                animTimer = 0;
            } else if (++animTimer >= DEAD_ANIM_SPEED) {
                animTimer = 0;
                if (animFrame < DEAD_FRAMES - 1) ++animFrame;    // hold the last frame
            }
        } else if (walking) {
            idleTimer = 0;
            playIdleAnimation = false;
            if (!wasWalking) {
                animFrame = 0;
                animTimer = 0;
            } else if (++animTimer >= ANIM_SPEED) {
                animTimer = 0;
                animFrame = (animFrame + 1) % WALK_FRAMES;
            }
        } else if (wasWalking) {
            idleTimer = 0;
            animFrame = 0;
            animTimer = 0;
        } else if (!playIdleAnimation) {
            animFrame = 0;
            animTimer = 0;
            if (++idleTimer >= IDLE_LIMIT) {
                idleTimer = 0;
                playIdleAnimation = true;
            }
        } else if (++animTimer >= ANIM_SPEED) {
            animTimer = 0;
            ++animFrame;
            if (animFrame >= IDLE_FRAMES) {
                animFrame = 0;
                playIdleAnimation = false;
            }
        }

        vita2d_texture *playerTexture = dead ? deadTexture : (walking ? walkTexture : idleTexture);
        if (playerTexture) {
            const int frame = (walking || playIdleAnimation || dead) ? animFrame : 0;
            const float width = FRAME_W * PLAYER_SCALE;
            const float height = FRAME_H * PLAYER_SCALE;
            float screenX = parallaxEnabled ? player.x - cameraX : player.x;
            const float left = screenX - width / 2.0f;
            const float top = groundY - height;
            const bool flip = !player.facingRight;

            vita2d_draw_texture_part_scale(
                playerTexture,
                flip ? left + width : left,
                top,
                FRAME_W * frame, 0, FRAME_W, FRAME_H,
                PLAYER_SCALE * (flip ? -1.0f : 1.0f),
                PLAYER_SCALE
            );
        }

        wasWalking = walking;
        wasDead = dead;
    }

    // ---------------- interactable sprites ----------------
    for (auto &interactable : interactables) {
        if (!interactable.enabled) continue;

        if (DEBUG_BOXES) {
            vita2d_draw_rectangle(interactable.x, interactable.y,
                                  interactable.width, interactable.height,
                                  RGBA8(0, 255, 0, 80));
        }

        if (interactable.spritePath.empty()) continue;
        if (!interactable.texture) {
            interactable.texture = vita2d_load_PNG_file(interactable.spritePath.c_str());
        }
        if (interactable.texture) {
            float screenX = parallaxEnabled ? interactable.x - cameraX : interactable.x;
            float screenY = interactable.y;
            if (pointClickEnabled) {
                screenX += (interactable.width - vita2d_texture_get_width(interactable.texture)) / 2.0f;
                screenY += (interactable.height - vita2d_texture_get_height(interactable.texture)) / 2.0f;
            }
            vita2d_draw_texture(interactable.texture, screenX, screenY);
        }
    }

    // ---------------- vision (darkness closes in as health drops) ----------------
    if (!pointClickEnabled) {
        float px = parallaxEnabled ? player.x - cameraX : player.x;   // screen position, not world
        drawVision(px, groundY - 160.0f, player.health / max_health);
    } else {
        drawVision(SCREEN_W / 2.0f, SCREEN_H / 2.0f, player.health / max_health);
    }

    // ---------------- UI (drawn after the darkness, so it stays readable) ----------------
    if (activeInteractable && !pointClickEnabled && !activeInteractable->dialogue.active) {
        drawInteractPrompt(font, crossButton, activeInteractable->name.c_str());
    }

    if (activeInteractable &&
        !activeInteractable->dialogue.text.empty() &&
        activeInteractable->dialogue.active) {
        vita2d_draw_rectangle(40, 390, 880, 120, RGBA8(20, 18, 24, 235));
        activeInteractable->dialogue.animateDialogue(
            activeInteractable->dialogue, font, nullptr,
            RGBA8(255, 255, 255, 255), 55, 410,
            activeInteractable->dialogue.index);
        drawAdvanceIcon(crossButton);
    }

    for (auto &dialogue : dialogues) {
        if (!dialogue.active) continue;

        vita2d_draw_rectangle(40, 390, 880, 120, RGBA8(20, 18, 24, 235));
        dialogue.animateDialogue(dialogue, font, nullptr,
                                 RGBA8(255, 255, 255, 255), 55, 410, dialogue.index);
        drawAdvanceIcon(crossButton);
    }
}