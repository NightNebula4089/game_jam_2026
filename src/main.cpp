#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <vita2d.h>
#include <sstream>
#include <vector>
#include <psp2/appmgr.h>
#include <player.h>
#include <dialogue.h>
#include <cmath>
#include <cstdio>

static const int SCREEN_W = 960;
static const int SCREEN_H = 544;

// ---- EDIT THESE to match your sprite sheet ----
static const int   FRAME_W       = 128;    // width of one frame in px
static const int   FRAME_H       = 128;    // height of one frame in px
static const int   IDLE_ROW      = 0;     // which row of the sheet
static const int   IDLE_FRAMES   = 11;
static const int   WALK_ROW      = 0;
static const int   WALK_FRAMES   = 8;
static const int   ANIM_SPEED    = 6;     // game frames per animation frame (60 fps)
static const bool  ART_FACES_RIGHT = true; // does the sheet draw him facing right?
static const float SPRITE_SCALE  = 1.0f;  // scale the sprite up
static const float PLAYER_SCALE = 1.8f; // scale the player up
static const float WALK_SPEED    = 3.0f;  // px per frame
static const float IDLE_LIMIT = 300; // frames before idle animation plays

static const int   BG_W      = 928;
static const int   BG_H      = 793;
static const float BG_SCALE  = 1.0f;
static const float BG_CROP_Y = BG_H - SCREEN_H / BG_SCALE;   // 249: show the bottom of the layers
static const float GROUND_Y  = (730 - BG_CROP_Y) * BG_SCALE; // 481: feet line (tree bases end ~729)

struct Layer {
    const char *file;
    float scroll;            // parallax: 0 = fixed, 1 = moves with the world
    bool front;              // true = drawn AFTER the player
    vita2d_texture *tex;
};

static Layer backgroundLayers[] = {
	{"app0:/assets/sprites/backgrounds/forest/bg_00_sky_bands.png", 0.0f, false, nullptr},
	{"app0:/assets/sprites/backgrounds/forest/bg_02_fog.png",        0.3f, false, nullptr},
	{"app0:/assets/sprites/backgrounds/forest/bg_03_trees.png",      0.5f, false, nullptr},
	{"app0:/assets/sprites/backgrounds/forest/bg_03b_lights.png",    0.5f, false, nullptr},
	{"app0:/assets/sprites/backgrounds/forest/bg_04_trees.png",      0.7f, false, nullptr},
	{"app0:/assets/sprites/backgrounds/forest/bg_05_trees.png",      0.8f, false, nullptr},
	{"app0:/assets/sprites/backgrounds/forest/bg_05b_lights.png",    0.8f, false, nullptr},
	{"app0:/assets/sprites/backgrounds/forest/bg_06_trunks.png",     0.85f, false, nullptr},
	{"app0:/assets/sprites/backgrounds/forest/bg_07_canopy.png",     0.9f, false, nullptr},
	{"app0:/assets/sprites/backgrounds/forest/bg_08_grass.png",      1.0f, true, nullptr},
	{"app0:/assets/sprites/backgrounds/forest/bg_09_ground.png",     1.0f, true, nullptr},
};

static const std::string playerSprites[] = {
	"app0:/assets/sprites/player/Idle_2.png",
	"app0:/assets/sprites/player/Run.png",
	"app0:/assets/sprites/player/Jump.png",
	"app0:/assets/sprites/player/Hurt.png",
	"app0:/assets/sprites/player/Walk.png",
	"app0:/assets/sprites/player/Dead.png"
};

static const int BACKGROUND_LAYER_COUNT = sizeof(backgroundLayers) / sizeof(backgroundLayers[0]);

float positiveModulo(float value, float size) {
	float result = std::fmod(value, size);
    return result < 0 ? result + size : result;
}

void drawLayer(vita2d_texture *tex, float offset,float x, float y, int tileX, int tileY, int tileW, int tileH, float scale) {
	float tileWidth = tileW * scale;

	offset = positiveModulo(offset, tileWidth);

	for (float drawX = x - offset - tileWidth; drawX < SCREEN_W; drawX += tileWidth) {
		vita2d_draw_texture_part_scale(tex, drawX, y, tileX, tileY, tileW, tileH, scale, scale);
	}
}


int main(int argc, char *argv[]) {

	vita2d_init();
	vita2d_set_clear_color(RGBA8(0, 0, 0, 255));
	for (int i = 0; i < BACKGROUND_LAYER_COUNT; ++i) {
		backgroundLayers[i].tex = vita2d_load_PNG_file(backgroundLayers[i].file);
	}
	vita2d_texture *idleTexture = vita2d_load_PNG_file(playerSprites[0].c_str());
	vita2d_texture *walkTexture = vita2d_load_PNG_file(playerSprites[4].c_str());
	vita2d_texture *cross_btn = vita2d_load_PNG_file("app0:assets/ui/buttons/cross_btn.png");

	vita2d_pgf *font = vita2d_load_default_pgf();

	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	SceCtrlData pad{};

	const float speed = 3.0f;

	Character player = Character(960/2, GROUND_Y, 1, "app0:/assets/sprites/player/Idle_2.png"); // Create a player character at (100, GROUND_Y)
	player.state = Idle;
	Dialogue forestDialogue("The forest is quieter than it should be.", 700.0f, false);
	bool dialogueTriggered = false;
	int animFrame = 0;
	int animTimer = 0;
	float cam_x = 0.0f;
	int idleTimer = 0;
	bool playIdleAnimation = false;

	while(true){

		// READ CONTROLLER INPUT ---------------------------------------------------------------
		sceCtrlPeekBufferPositive(0, &pad, 1);
		if(pad.buttons & SCE_CTRL_START) break;

		// D-pad
		float dir = 0;
		if(pad.buttons & SCE_CTRL_LEFT) dir -= speed;
		if(pad.buttons & SCE_CTRL_RIGHT) dir += speed;

		if(forestDialogue.active && (pad.buttons & SCE_CTRL_CROSS)) {
			forestDialogue.active = false;
		}

		// Analog stick
		float lx = pad.lx - 128.0f; // Center the value around 0
		if(lx > 30 || lx < -30) dir += (lx / 128.0f) * speed;

		if (!dialogueTriggered && player.x >= forestDialogue.x) {
			forestDialogue.active = true;
			dialogueTriggered = true;
			forestDialogue.update(forestDialogue.text);
		}
		if (forestDialogue.active) {
			dir = 0;
		}

		// ----------------------------------------------------------------- READ CONTROLLER INPUT

		// UPDATE GAME STATE -----------------------------------------------------------------

		bool wasWalking = (player.state == Walking);

		if(dir < 0) {
			player.facingRight = false;
			player.state = Walking;
			idleTimer = 0;
		} else if(dir > 0) {
			player.facingRight = true;
			player.state = Walking;
			idleTimer = 0;
			
		} else {
			player.state = Idle;
		}

		if (player.state == Walking) {
			player.x += dir;
		}
		cam_x = player.x - SCREEN_W / 2.0f;

		if(player.state == Idle && idleTimer >= IDLE_LIMIT) {
			playIdleAnimation = true;
			idleTimer = 0; // Reset the idle timer after triggering the idle animation
			animFrame = 0; // Reset the animation frame for the idle animation
			animTimer = 0; // Reset the animation timer for the idle animation
		} else if(player.state == Walking && wasWalking) {
			idleTimer = 0; // Reset the idle timer if the player starts walking
			playIdleAnimation = false; // Stop the idle animation if the player starts walking
			if(animTimer >= ANIM_SPEED) {
				animFrame = (animFrame + 1) % WALK_FRAMES;
				animTimer = 0;
			} else {
				animTimer++;
			}
		} else if(player.state == Walking && !wasWalking) {
			playIdleAnimation = false; // Stop the idle animation if the player starts walking
			animFrame = 0; // Reset the animation frame when starting to walk
			animTimer = 0; // Reset the animation timer when starting to walk
			idleTimer = 0; // Reset the idle timer if the player starts walking
		} else if(player.state == Idle && wasWalking){
			idleTimer =0;
			animFrame = 0; // Reset the animation frame for the idle animation
			animTimer = 0; // Reset the animation timer for the idle animation
		} else if(player.state == Idle && playIdleAnimation) {
			if(idleTimer >= ANIM_SPEED * IDLE_FRAMES) {
				idleTimer = 0; // Reset the idle timer after completing the idle animation
				playIdleAnimation = false; // Stop the idle animation after completing it
			} else {
				idleTimer++;
				animTimer++; // Increment the animation timer for the idle animation
				if(animTimer >= ANIM_SPEED) {
					animFrame = (animFrame + 1) % IDLE_FRAMES;
					animTimer = 0;
				}
			}
		} else {
			idleTimer++;
		}

        // if (player.y < 0) player.y = 0;
        // if (player.y > SCREEN_H - FRAME_H * SPRITE_SCALE) player.y = SCREEN_H - FRAME_H * SPRITE_SCALE;

		// ----------------------------------------------------------------- UPDATE GAME STATE

		// DRAWING -----------------------------------------------------------------

		vita2d_start_drawing();
		vita2d_clear_screen();

		for (int i = 0; i < BACKGROUND_LAYER_COUNT; ++i) {
			if (backgroundLayers[i].front || !backgroundLayers[i].tex) continue;
			float offsetX = cam_x * backgroundLayers[i].scroll;
			drawLayer(backgroundLayers[i].tex, offsetX, (SCREEN_W - BG_W) / 2.0f, -BG_CROP_Y, 0, 0, BG_W, BG_H, BG_SCALE);
		}

		vita2d_texture *player_img = (player.state == Walking) ? walkTexture : idleTexture;

		if(player_img){
			float w = FRAME_W * PLAYER_SCALE;
			float h = FRAME_H * PLAYER_SCALE;
			float left =  player.x - cam_x;
			float top = GROUND_Y - h;

			bool flip = !player.facingRight;
			vita2d_draw_texture_part_scale(player_img,flip ? left+w : left, top, FRAME_W * animFrame, 0, FRAME_W, FRAME_H, PLAYER_SCALE * (flip ? -1.0f : 1.0f), PLAYER_SCALE);
		} else {
			vita2d_draw_rectangle(dir - 20, GROUND_Y - 80, 40, 80, RGBA8(255, 0, 0, 255)); // fallback
		}

		for (int i = 0; i < BACKGROUND_LAYER_COUNT; ++i) {
			if (!backgroundLayers[i].front || !backgroundLayers[i].tex) continue;
			float offsetX = cam_x * backgroundLayers[i].scroll;
			drawLayer(backgroundLayers[i].tex, offsetX, (SCREEN_W - BG_W) / 2.0f, -BG_CROP_Y, 0, 0, BG_W, BG_H, BG_SCALE);
		}

		if (forestDialogue.active) {
			vita2d_draw_rectangle(40, 390, 880, 120, RGBA8(20, 18, 24, 235));
			forestDialogue.animateDialogue(
				forestDialogue,
				font,
				nullptr,
				RGBA8(255, 255, 255, 255),
				55,
				410
			);
			float btnSize = 40.0f;
			if (cross_btn) {
				vita2d_draw_texture_part_scale(
					cross_btn,
					920 - btnSize - 10,  // x
					510 - btnSize - 10,  // y
					0, 0,
					480, 480,
					btnSize / 480.0f,
					btnSize / 480.0f
				);
			}
		}

		vita2d_end_drawing();
		vita2d_swap_buffers();

		// ----------------------------------------------------------------- DRAWING
	}

	vita2d_fini();
	vita2d_free_texture(idleTexture);
	vita2d_free_texture(walkTexture);
	vita2d_free_texture(cross_btn);
	for (int i = 0; i < BACKGROUND_LAYER_COUNT; ++i) {
		if (backgroundLayers[i].tex) {
			vita2d_free_texture(backgroundLayers[i].tex);
		}
	}
	vita2d_free_pgf(font);
	sceKernelExitProcess(0);
	return 0;

}
