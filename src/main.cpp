#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <vita2d.h>
#include <sstream>
#include <vector>
#include <psp2/appmgr.h>
#include <player.h>
#include <dialogue.h>
#include <scene.h>
#include <menu.h>
#include <cmath>
#include <world.h>
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
static const float PLAYER_SCALE = 2.5f; // scale the player up
static const float WALK_SPEED    = 3.0f;  // px per frame

static const int   BG_W      = 928;
static const int   BG_H      = 793;
static const float BG_SCALE  = 1.0f;
static const float BG_CROP_Y = BG_H - SCREEN_H / BG_SCALE;   // 249: show the bottom of the layers
static const float GROUND_Y  = (730 - BG_CROP_Y) * BG_SCALE; // 481: feet line (tree bases end ~729)

static const std::string playerSprites[] = {
	"app0:/assets/sprites/player/Idle_2.png",
	"app0:/assets/sprites/player/Run.png",
	"app0:/assets/sprites/player/Jump.png",
	"app0:/assets/sprites/player/Hurt.png",
	"app0:/assets/sprites/player/Walk.png",
	"app0:/assets/sprites/player/Dead.png"
};

static void drawInventoryPanel(const Inventory &inventory, vita2d_pgf *font, int selected) {
	vita2d_draw_rectangle(120, 80, 720, 384, RGBA8(20, 18, 24, 240));
	vita2d_pgf_draw_text(font, 160, 125, RGBA8(255, 255, 255, 255), 1.2f, "Inventory");

	for (size_t i = 0; i < inventory.items.size(); ++i) {
		const bool highlighted = static_cast<int>(i) == selected;
		const unsigned int color = highlighted
			? RGBA8(255, 220, 80, 255)
			: RGBA8(255, 255, 255, 255);
		char label[128];
		snprintf(label, sizeof(label), "%u. %s x%d", static_cast<unsigned>(i + 1),
			inventory.items[i].name.c_str(), inventory.items[i].quantity);
		vita2d_pgf_draw_text(font, 170, 180 + static_cast<int>(i) * 40,
			color, 1.0f, label);
	}
}

int main(int argc, char *argv[]) {

	vita2d_init();
	vita2d_set_clear_color(RGBA8(0, 0, 0, 255));
	vita2d_texture *idleTexture = vita2d_load_PNG_file(playerSprites[0].c_str());
	vita2d_texture *walkTexture = vita2d_load_PNG_file(playerSprites[4].c_str());
	vita2d_texture *cross_btn = vita2d_load_PNG_file("app0:assets/ui/buttons/cross_btn.png");

	vita2d_font *font = vita2d_load_font_file("app0:/assets/fonts/confession_font.ttf");
	vita2d_pgf *defaultFont = vita2d_load_default_pgf();
	MainMenu mainMenu({"Start", "Exit"});
	mainMenu.loadTextures();

	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	SceCtrlData pad{};

	const float speed = 3.0f;

	Character player = Character(960/2, GROUND_Y, 1, "app0:/assets/sprites/player/Idle_2.png"); // Create a player character at (100, GROUND_Y)
	player.state = Idle;
	// Level 1 initialisation -------------------------------------------------------------------------
	Scene level1Scene("Basement","app0:/assets/sprites/backgrounds/basement/bgrough1.png",75.0f,865.0f);
	// Add interactables to the scene
	Interactable bed(
    "Bed",
    130.0f,
    430.0f,
    250.0f,
    70.0f,
    Dialogue({
        "This matteress, its slumped in the middle .....","were there other people here?", "Wait ... there's something on the side"},
        130.0f,
        true
    ),
    60.0f,
    ""
	);
	Interactable door(
	"Door",
	820.0f,
	430.0f,
	50.0f,
	200.0f,
	Dialogue({
		"The door is locked, I need to find a key to open it."
	},
		820.0f,
		true
	),
	60.0f,
	""
	);
	Interactable key(
	"Bones",
	460.0f,
	GROUND_Y,
	50.0f,
	50.0f,
	Dialogue({
		"A pile of bones .... what the fuck?", "I really need to get out of here",
		"There were other people in here before me."
	},
		460.0f,
		true
	),
	60.0f,
	"app0:assets/sprites/Interactables/Bones.png",
	true
	);
	level1Scene.addInteractable(door);
	level1Scene.addInteractable(key);
	level1Scene.addInteractable(bed);

	// Add dialogues to the scene
	level1Scene.addDialogue(Dialogue({"Urgh what am I doing here?","I need to find a way out ........", "Grhh, my stomach fuck I am bleeding"}, 700.0f, false));

	// --------------------------------------------------------------------------------- Level 1 initialisation

	Scene forestScene("Forest", "",0.0f,0.0f);
	Scene blankScene("Blank", "",0.0f,0.0f);
	Scene interactSceneStub("2D Scene Stub", "",0.0f,0.0f);
	forestScene.setParallaxEnabled();
	forestScene.addBackgroundLayer("app0:assets/sprites/backgrounds/forest/bg_00_sky_bands.png");
	forestScene.addBackgroundLayer("app0:assets/sprites/backgrounds/forest/bg_02_fog.png");
	forestScene.addBackgroundLayer("app0:assets/sprites/backgrounds/forest/bg_03_trees.png");
	forestScene.addBackgroundLayer("app0:assets/sprites/backgrounds/forest/bg_03b_lights.png");
	forestScene.addBackgroundLayer("app0:assets/sprites/backgrounds/forest/bg_04_trees.png");
	forestScene.addBackgroundLayer("app0:assets/sprites/backgrounds/forest/bg_05_trees.png");
	forestScene.addBackgroundLayer("app0:assets/sprites/backgrounds/forest/bg_05b_lights.png");
	forestScene.addBackgroundLayer("app0:assets/sprites/backgrounds/forest/bg_06_trunks.png");
	forestScene.addBackgroundLayer("app0:assets/sprites/backgrounds/forest/bg_07_canopy.png");
	forestScene.addBackgroundLayer("app0:assets/sprites/backgrounds/forest/bg_08_grass.png");
	forestScene.addBackgroundLayer("app0:assets/sprites/backgrounds/forest/bg_09_ground.png");

	GameState gameState(player,level1Scene,MainMenuState);
	float cam_x = 0.0f;

	unsigned int held = 0;
	unsigned int pressed = 0;
	unsigned int released = 0;
	bool inventoryOpen = false;
	int selectedInventoryItem = 0;
	Dialogue inventoryDialogue(std::vector<std::string>(), 0.0f, true);

	while(true){
		unsigned int prev = held;
		sceCtrlPeekBufferPositive(0, &pad, 1);
		held = pad.buttons;
		pressed = (held) & (~prev);
		released = (~held) & prev;

		if (pad.buttons & SCE_CTRL_RTRIGGER) break; // change it later

		if (gameState.state == MainMenuState) {
			int selected = mainMenu.updateState(pad);
			if (selected == 0) {
				gameState.state = Playing;
			} else if (selected == 1) {
				gameState.state = Exit;
			}

			vita2d_start_drawing();
			vita2d_clear_screen();
			mainMenu.draw(font);
			vita2d_end_drawing();
			vita2d_swap_buffers();
			continue;
		}

		if (gameState.state == Exit) break;

		if(gameState.state == Playing) {
			// Handle playing logic here
			if (pressed & SCE_CTRL_SELECT) {
				inventoryOpen = !inventoryOpen;
				inventoryDialogue.active = false;
			}

			float dir = 0.0f;
			if (inventoryOpen) {
				bool openedInventoryDialogue = false;
				if (!player.inventory.items.empty()) {
					const int itemCount = static_cast<int>(player.inventory.items.size());
					if (pressed & SCE_CTRL_UP) {
						selectedInventoryItem = (selectedInventoryItem + itemCount - 1) % itemCount;
					}
					if (pressed & SCE_CTRL_DOWN) {
						selectedInventoryItem = (selectedInventoryItem + 1) % itemCount;
					}
					if ((pressed & SCE_CTRL_CROSS) && !inventoryDialogue.active) {
						const InventoryItem &item = player.inventory.items[selectedInventoryItem];
						if (!item.dialogue.empty()) {
							inventoryDialogue.text = item.dialogue;
							inventoryDialogue.index = 0;
							inventoryDialogue.visibleChars = 0;
							inventoryDialogue.timer = 0;
							inventoryDialogue.active = true;
							openedInventoryDialogue = true;
						}
					}
				} else {
					selectedInventoryItem = 0;
				}

				if (!openedInventoryDialogue && inventoryDialogue.active && (pressed & SCE_CTRL_CROSS)) {
					if (inventoryDialogue.index >= inventoryDialogue.text.size() - 1) {
						inventoryDialogue.active = false;
					} else {
						++inventoryDialogue.index;
						inventoryDialogue.visibleChars = 0;
						inventoryDialogue.timer = 0;
					}
				}
			} else {

		// READ CONTROLLER INPUT ---------------------------------------------------------------

		// D-pad
		if(pad.buttons & SCE_CTRL_LEFT) dir -= speed;
		if(pad.buttons & SCE_CTRL_RIGHT) dir += speed;

		bool interactableDialogueActive = false;
		for (auto interactableIt = gameState.currentScene.interactables.begin();
			 interactableIt != gameState.currentScene.interactables.end();) {
			auto &interactable = *interactableIt;
			const bool inRange =
				player.x >= interactable.x - interactable.range &&
				player.x <= interactable.x + interactable.width + interactable.range &&
				player.y >= interactable.y - interactable.range &&
				player.y <= interactable.y + interactable.height + interactable.range;

			interactable.active = inRange;
			if (inRange && interactable.pickable && (pressed & SCE_CTRL_CROSS)) {
				player.inventory.addItem(
					interactable.name,
					1,
					interactable.dialogue.text
				);
				interactableIt = gameState.currentScene.interactables.erase(interactableIt);
				continue;
			}

			if (interactable.dialogue.text.empty()) {
				interactable.dialogue.active = false;
				++interactableIt;
				continue;
			} else if (interactable.dialogue.active && (pressed & SCE_CTRL_CROSS)) {
				if(interactable.dialogue.index >= interactable.dialogue.text.size() - 1){
					interactable.dialogue.active = false;
				} else {
					interactable.dialogue.index++;
					interactable.dialogue.visibleChars = 0;
					interactable.dialogue.timer = 0;
				}
			} else if (inRange && (pressed & SCE_CTRL_CROSS) && interactable.dialogue.interactable) {
				interactable.dialogue.active = true;
				interactable.dialogue.update(interactable.dialogue.text);
			} 

			if (!inRange) {
				interactable.dialogue.active = false;
			}
			interactableDialogueActive =
				interactableDialogueActive || interactable.dialogue.active;
			++interactableIt;
		}

		// Analog stick
		float lx = pad.lx - 128.0f; // Center the value around 0
		if(lx > 30 || lx < -30) dir += (lx / 128.0f) * speed;

		bool sceneDialogueActive = false;
		for (auto &dialogue : gameState.currentScene.dialogues) {
			if (!dialogue.interactable && !dialogue.triggered && player.x >= dialogue.x) {
				dialogue.active = true;
				dialogue.triggered = true;
				dialogue.update(dialogue.text);
			}

			if (dialogue.active && (pressed & SCE_CTRL_CROSS)) {
				if(dialogue.index >= dialogue.text.size() - 1){
					dialogue.active = false;
				} else {
					dialogue.index++;
					dialogue.visibleChars = 0;
					dialogue.timer = 0;
				}
			}
			sceneDialogueActive = sceneDialogueActive || dialogue.active;
		}

		if (sceneDialogueActive || interactableDialogueActive) {
			dir = 0;
		}
			}

		// ----------------------------------------------------------------- READ CONTROLLER INPUT

		// UPDATE GAME STATE -----------------------------------------------------------------

		if(dir < 0) {
			player.facingRight = false;
			player.state = Walking;
		} else if(dir > 0) {
			player.facingRight = true;
			player.state = Walking;
			
		} else {
			player.state = Idle;
		}

		if (player.state == Walking) {
			player.x += dir;
			if(!gameState.currentScene.parallaxEnabled) {
				if(player.x < gameState.currentScene.left_border) player.x = gameState.currentScene.left_border;
				if(player.x > gameState.currentScene.right_border) player.x = gameState.currentScene.right_border;
			}
		}
		cam_x = player.x - SCREEN_W / 2.0f;

        // if (player.y < 0) player.y = 0;
        // if (player.y > SCREEN_H - FRAME_H * SPRITE_SCALE) player.y = SCREEN_H - FRAME_H * SPRITE_SCALE;

		// ----------------------------------------------------------------- UPDATE GAME STATE

		// DRAWING -----------------------------------------------------------------

		vita2d_start_drawing();
		vita2d_clear_screen();

		gameState.currentScene.drawScene(player, idleTexture, walkTexture, cam_x, GROUND_Y, defaultFont, cross_btn);
		if (inventoryOpen) {
			drawInventoryPanel(player.inventory, defaultFont, selectedInventoryItem);
			if (inventoryDialogue.active) {
				vita2d_draw_rectangle(40, 390, 880, 120, RGBA8(20, 18, 24, 235));
				inventoryDialogue.animateDialogue(inventoryDialogue, defaultFont, nullptr,
					RGBA8(255, 255, 255, 255), 55, 410, inventoryDialogue.index);
			}
		}

		vita2d_end_drawing();
		vita2d_swap_buffers();
		} else if(gameState.state == Paused) {
			// Handle paused logic here
		} else if(gameState.state == GameOver) {
			// Handle game over logic here
		}

		// ----------------------------------------------------------------- DRAWING
	}

	vita2d_fini();
	vita2d_free_texture(idleTexture);
	vita2d_free_texture(walkTexture);
	vita2d_free_texture(cross_btn);
	mainMenu.releaseTextures();
	gameState.currentScene.releaseBackgroundTextures();
	vita2d_free_pgf(defaultFont);
	sceKernelExitProcess(0);
	return 0;

}
