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

void Scene::setParallaxEnabled() {
    parallaxEnabled = true;
}

void Scene::unsetParallaxEnabled() {
    parallaxEnabled = false;
}

void Scene::setPointClickEnabled() {
    pointClickEnabled = true;
}

void Scene::releaseBackgroundTextures() {
    if (backgroundTexture) {
        vita2d_free_texture(backgroundTexture);
        backgroundTexture = nullptr;
    }

    for (vita2d_texture *texture : backgroundTextures) {
        if (texture) {
            vita2d_free_texture(texture);
        }
    }
    backgroundTextures.clear();

    for (auto &interactable : interactables) {
        if (interactable.texture) {
            vita2d_free_texture(interactable.texture);
            interactable.texture = nullptr;
        }
    }
}

static void drawInteractPrompt(vita2d_pgf *font, vita2d_texture *icon, const char *target) {
    const int SCREEN_W = 960;
    const int SCREEN_H = 544;
    const unsigned int white = RGBA8(255, 255, 255, 255);
    const unsigned int panel = RGBA8(0, 0, 0, 160);   // translucent backing so text is readable
    const float scale    = 1.0f;                      // text size multiplier
    const float iconSize = 32.0f;                     // icon drawn at 32x32
    const float gap      = 8.0f;                      // space either side of the icon
    const float pad      = 12.0f;                     // padding inside the panel

    const char *before = "Press";
    char after[128];
    snprintf(after, sizeof(after), "to interact with %s", target);

    float wBefore = vita2d_pgf_text_width(font, scale, before);
    float wAfter  = vita2d_pgf_text_width(font, scale, after);
    float textH   = vita2d_pgf_text_height(font, scale, before);

    float total    = wBefore + gap + iconSize + gap + wAfter;
    float x        = (SCREEN_W - total - 20.0f);       // x of the text start
    float baseline = SCREEN_H - 50.0f;                // y of the text baseline
    float textMidY = baseline - textH / 2.0f;         // vertical centre of the text
    float iconY    = textMidY - iconSize / 2.0f;      // centre the icon on it

    // "Press"
    vita2d_pgf_draw_text(font, x, baseline, white, scale, before);
    x += wBefore + gap;

    // Icon (scale computed from the texture, so any icon size works)
    if (icon) {
        float sx = iconSize / vita2d_texture_get_width(icon);
        float sy = iconSize / vita2d_texture_get_height(icon);
        vita2d_draw_texture_scale(icon, x, iconY, sx, sy);
    }
    x += iconSize + gap;

    // "to interact with ..."
    vita2d_pgf_draw_text(font, x, baseline, white, scale, after);
}

void Scene::drawScene(Character &player, vita2d_texture *deadTexture, vita2d_texture *idleTexture,
                      vita2d_texture *walkTexture, float cameraX, float groundY,
                      vita2d_pgf *font, vita2d_texture *crossButton) {
    static const int FRAME_W = 128;
    static const int FRAME_H = 128;
    static const int IDLE_FRAMES = 11;
    static const int WALK_FRAMES = 8;
    static const int ANIM_SPEED = 6;
    static const int DEAD_FRAMES = 4;
    static const int IDLE_LIMIT = 300;
    static const float PLAYER_SCALE = 2.5f;
    static const int SCREEN_W = 960;
    static const int SCREEN_H = 544;
    static const int BG_W = 928;
    static const int BG_H = 793;
    static const float BG_SCALE = 1.0f;
    static const float BG_CROP_Y = 249.0f;
    static const float layerScroll[] = {
        0.0f, 0.3f, 0.5f, 0.5f, 0.7f, 0.8f,
        0.8f, 0.85f, 0.9f, 1.0f, 1.0f
    };

    Interactable *activeInteractable = nullptr;
    for (auto &interactable : interactables) {
        if (pointClickEnabled) {
            if (interactable.active) {
                activeInteractable = &interactable;
                break;
            }
        } else {
            const bool inRange =
                player.x >= interactable.x - interactable.range &&
                player.x <= interactable.x + interactable.width + interactable.range &&
                player.y >= interactable.y - interactable.range &&
                player.y <= interactable.y + interactable.height + interactable.range;
            if (inRange) {
                activeInteractable = &interactable;
                break;
            }
        }
    }

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
                vita2d_draw_texture_part_scale(
                    texture,
                    x,
                    -BG_CROP_Y,
                    0,
                    0,
                    BG_W,
                    BG_H,
                    BG_SCALE,
                    BG_SCALE
                );
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

    if (!pointClickEnabled) {
    const bool walking = player.state == Walking;
    const bool dead  = player.state == Dead;

    if (!dead) wasDead = false;

    if (dead) {
        playIdleAnimation = false;
        if (!wasDead) {
            wasDead  = true;
            animFrame = 0;
            animTimer = 0;
        } else if (++animTimer >= ANIM_SPEED) {
            animTimer = 0;
            if (animFrame < DEAD_FRAMES - 1) ++animFrame;
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

    vita2d_texture *playerTexture = player.state == Dead ? deadTexture : walking ? walkTexture : idleTexture;
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
            FRAME_W * frame,
            0,
            FRAME_W,
            FRAME_H,
            PLAYER_SCALE * (flip ? -1.0f : 1.0f),
            PLAYER_SCALE
        );
    }
        wasWalking = walking;
    }

    for(auto &interactable : interactables) {
        if (interactable.spritePath.empty()) continue;
        if (!interactable.texture) {
            interactable.texture = vita2d_load_PNG_file(interactable.spritePath.c_str());
        }
        if (interactable.texture) {
            float screenX = parallaxEnabled ? interactable.x - cameraX : interactable.x;
            vita2d_draw_texture(interactable.texture, screenX, interactable.y);
        } else if (pointClickEnabled) {
            const float screenX = interactable.x;
            vita2d_draw_rectangle(
                screenX,
                interactable.y,
                interactable.width,
                interactable.height,
                RGBA8(180, 40, 40, 255)
            );
        }
    }

    if(!pointClickEnabled) {
        drawVision(player.x,player.y,player.health / max_health);
    } else {
        drawVision(SCREEN_W/2.0f,SCREEN_H/2.0f,player.health / max_health);
    }

    if (activeInteractable && !pointClickEnabled && !activeInteractable->dialogue.active) {
        drawInteractPrompt(font, crossButton, activeInteractable->name.c_str());
    }

    if (activeInteractable &&
        !activeInteractable->dialogue.text.empty() &&
        activeInteractable->dialogue.active) {
        vita2d_draw_rectangle(40, 390, 880, 120, RGBA8(20, 18, 24, 235));
        activeInteractable->dialogue.animateDialogue(
            activeInteractable->dialogue,
            font,
            nullptr,
            RGBA8(255, 255, 255, 255),
            55,
            410,
            activeInteractable->dialogue.index
        );

        if (crossButton) {
            const float buttonSize = 40.0f;
            vita2d_draw_texture_part_scale(
                crossButton,
                920 - buttonSize - 10,
                510 - buttonSize - 10,
                0,
                0,
                480,
                480,
                buttonSize / 480.0f,
                buttonSize / 480.0f
            );
        }
    }

    for (auto &dialogue : dialogues) {
        if (!dialogue.active) continue;

        vita2d_draw_rectangle(40, 390, 880, 120, RGBA8(20, 18, 24, 235));
        dialogue.animateDialogue(
            dialogue,
            font,
            nullptr,
            RGBA8(255, 255, 255, 255),
            55,
            410,
            dialogue.index
        );

        if (crossButton) {
            const float buttonSize = 40.0f;
            vita2d_draw_texture_part_scale(
                crossButton,
                920 - buttonSize - 10,
                510 - buttonSize - 10,
                0,
                0,
                480,
                480,
                buttonSize / 480.0f,
                buttonSize / 480.0f
            );
        }
    }
}