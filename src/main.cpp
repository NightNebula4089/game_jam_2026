#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <vita2d.h>
#include <sstream>
#include <vector>
#include <psp2/appmgr.h>
#include <player.h>
#include <dialogue.h>
#include <scene.h>
#include <menu.h>
#include <pause.h>
#include <audio.h>
#include <cmath>
#include <world.h>
#include <cstdio>

static const int SCREEN_W = 960;
static const int SCREEN_H = 544;

// ---- EDIT THESE to match your sprite sheet ----
static const int   FRAME_W       = 128;    // width of one frame in px
static const int   FRAME_H       = 128;    // height of one frame in px
static const int   IDLE_ROW      = 0;     // which row of the sheet
static const int   IDLE_FRAMES   = 11; // number of frames in the idle animation
static const int   DEAD_FRAMES   = 4; // number of frames in the dead animation
static const int   WALK_ROW      = 0;
static const int   WALK_FRAMES   = 8;
static const int   ANIM_SPEED    = 6;     // game frames per animation frame (60 fps)
static const bool  ART_FACES_RIGHT = true; // does the sheet draw him facing right?
static const float SPRITE_SCALE  = 1.0f;  // scale the sprite up
static const float PLAYER_SCALE = 2.5f; // scale the player up
static const float WALK_SPEED    = 3.0f;  // px per frame
static const float FADE_SECONDS  = 1.5f;
static const int INPUT_DEBOUNCE_FRAMES = 8;
static const float HEALTH_DECREASE_RATE = 0.005f; // Health decrease rate per frame
static const float MOVEMENT_HEALTH_PENALTY = 0.01f; // Health penalty for moving
static const int DEATH_TIME_LIMIT = 600;

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
	SoundManager sounds;
	const bool soundReady = sounds.init();
	vita2d_texture *idleTexture = vita2d_load_PNG_file(playerSprites[0].c_str());
	vita2d_texture *walkTexture = vita2d_load_PNG_file(playerSprites[4].c_str());
	vita2d_texture *deadTexture = vita2d_load_PNG_file(playerSprites[5].c_str());
	vita2d_texture *cross_btn = vita2d_load_PNG_file("app0:assets/ui/buttons/cross_btn.png");

	vita2d_font *font = vita2d_load_font_file("app0:/assets/fonts/confession_font.ttf");
	vita2d_pgf *defaultFont = vita2d_load_default_pgf();

	MainMenu mainMenu({"Start", "Exit"});
	mainMenu.loadTextures();
	PauseMenu pauseMenu;
	Fade sceneFade;
	sceneFade.start(255.0f, 0.0f, FADE_SECONDS);
	bool sceneTransitionPending = false;
	GameStateEnum pendingState = Playing;

	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	SceCtrlData pad{};

	const float speed = 3.0f;

	Character player = Character(960/2, GROUND_Y, 1, "app0:/assets/sprites/player/Idle_2.png"); // Create a player character at (100, GROUND_Y)
	player.state = Idle;
	// Level 1 initialisation -------------------------------------------------------------------------
	Scene level1Scene("Basement","app0:/assets/sprites/backgrounds/basement/bgfinal.png",75.0f,865.0f);
	Scene brickScene("Brick Scene", "app0:assets/sprites/backgrounds/basement/bricks.png",0.0f,0.0f);
	Scene mattressScene("Mattress Scene", "app0:assets/sprites/backgrounds/basement/mattress.png",0.0f,0.0f);
	mattressScene.setPointClickEnabled();
	mattressScene.entryX = SCREEN_W / 2.0f;
	mattressScene.entryY = GROUND_Y;
	brickScene.setPointClickEnabled();
	brickScene.entryX = SCREEN_W / 2.0f;
	brickScene.entryY = GROUND_Y;
	// Add interactables to the scene
	Interactable bed(
    "Bed",
    130.0f,
    430.0f,
    250.0f,
    50.0f,
    Dialogue({
        "This matteress, its slumped in the middle .....","were there other people here?", "Wait ... there's something on the side"},
        130.0f,
        true
    ),
	20.0f,
    "",
	false,
	&mattressScene,
	{},
	"",
	nullptr,
	false
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
	"",
	false,
	nullptr,
	{},
	"",
	nullptr,
	false
	);
	Interactable brickWall(
		"Bricks",
		75.0f,
		430.0f,
		30.0f,
		200.0f,
		Dialogue({
			"A brick! I can use this to break the door open!"
		},
		480.0f,
		true
		),
		80.0f,
		"",
		false,
		&brickScene,
		{},
		"",
		nullptr,
		false
	);
	level1Scene.addInteractable(door);
	level1Scene.addInteractable(bed);
	level1Scene.addInteractable(brickWall);

	// Add dialogues to the scene
	level1Scene.addDialogue(Dialogue({"Urgh what am I doing here?","I need to find a way out ........", "Aghhh, my stomach fuck I am bleeding"}, 960/2.0f, false));

	// --------------------------------------------------------------------------------- Level 1 initialisation


	// interactables for matteress 

	Interactable note(
		"Note",
		462,
		150,
		100,
		100,
		Dialogue({
			"A note... it's written in blood, huh smart.",
			"It says: 'XEN UXFMZ OX'","Some kind of cipher.", "But where's the key?"},
			462.0f,
			true
		),
		60.0f,
		"app0:assets/sprites/backgrounds/basement/paper.png",
		true
	);

	Interactable shiv(
		"Shiv",
		462,
		150,
		100,
		100,
		Dialogue({
			"A shiv!", "The previous victim must have died before he could have used it to escape.","I might be able to use it but where?"
		},462.0f,true),
		60.0f,
		"",
		true
	);
	shiv.enabled = false;
	note.onInteract = [&]() {
		note.enabled = false;
		if (auto *s = mattressScene.find("Shiv")) s->enabled = true;
	};

	Interactable blood(
		"Blood",
		840,
		280,
		100,
		100,
		Dialogue({
			"Blood stains on the mattress...","Are they mine?"},
			840.0f,
			true
		),
		60.0f,
		"",
		false
	);

	mattressScene.addInteractable(note);
	mattressScene.addInteractable(shiv);
	mattressScene.addInteractable(blood);


	Scene forestScene("Forest", "",0.0f,0.0f);
	forestScene.deathDistance = 300.0f;
	Scene blankScene("Blank", "app0:assets/sprites/backgrounds/Black.png",0.0f,0.0f);
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

	Scene *currentScene = &level1Scene;
	Scene *previousScene = nullptr;
	Scene *pendingScene = nullptr;

	Interactable tallyTen(
		"Tally 10", 360, 100, 240, 180,
		Dialogue({"tally marks .... must have been etched into the bricks by the last person here , 10 it says"}, 0.0f, true), 0.0f, "");
	Interactable tallySeven(
		"Tally 7", 350, 362, 240, 300,
		Dialogue({"Tally marks here as well, it says 7"}, 0.0f, true), 0.0f, "");

	Interactable hole(
		"Hole", 75.0f, 430.0f, 100.0f, 200.0f,
		Dialogue({"Cold air... and trees. I'm going through."}, 0.0f, true),
		80.0f, "", false, &forestScene);
	hole.enabled = false;
	level1Scene.addInteractable(hole);

	Interactable looseBrick(
		"Loose Brick", 700, 330, 100, 100,
		Dialogue({"I wedge the shiv behind the brick and pull.",
		          "It shifts. Dust, then cold air."}, 0.0f, true),
		0.0f, "");
	looseBrick.requiredItem = "Shiv";
	looseBrick.closedText = {
		"This brick seems to have been dug in more than the others,",
		"maybe I can dig into it with something."
	};
	looseBrick.scene = &level1Scene;
	looseBrick.onInteract = [&]() {
		if (soundReady) sounds.playVentAirflow();
		level1Scene.setBackground("app0:assets/sprites/backgrounds/basement/bgfinalhole.png");
		if (auto *wall = level1Scene.find("Bricks")) wall->enabled = false;
		if (auto *roomHole = level1Scene.find("Hole")) roomHole->enabled = true;
	};

	brickScene.addInteractable(tallyTen);
	brickScene.addInteractable(tallySeven);
	brickScene.addInteractable(looseBrick);

	GameState gameState(MainMenuState);
	if (auto *roomHole = level1Scene.find("Hole")) {
		roomHole->onInteract = [&]() {
			gameState.gameWon = true;
		};
	}
	float previousPlayerX = 0.0f;
	float previousPlayerY = 0.0f;
	float pendingPlayerX = 0.0f;
	float pendingPlayerY = 0.0f;
	float cam_x = 0.0f;

	unsigned int held = 0;
	unsigned int pressed = 0;
	unsigned int released = 0;
	unsigned int dead_timer = 0;
	int inputDebounceTimer = 0;
	bool touchWasDown = false;
	bool inventoryOpen = false;
	int selectedInventoryItem = 0;
	Dialogue inventoryDialogue(std::vector<std::string>(), 0.0f, true);

	while(true){
		unsigned int prev = held;
		sceCtrlPeekBufferPositive(0, &pad, 1);

		if (gameState.state == Playing && player.state == Dead) {
			if(currentScene->name == "Forest" && player.x >= 100.0f){
				player.health = -1.0f; // Set health to -1 to indicate death
			}
			pressed = 0;                  // no interacting, scene changes or inventory
			pad.buttons = 0;              // no d-pad
			pad.lx = pad.ly = 128;        // stick centred
			sounds.updateDead(true);      // play death sound
		} else {
			sounds.updateDead(false);
		}

		held = pad.buttons;
		const unsigned int rawPressed = held & ~prev;
		if (inputDebounceTimer > 0) {
			--inputDebounceTimer;
			pressed = 0;
		} else {
			pressed = rawPressed;
			if (pressed != 0) inputDebounceTimer = INPUT_DEBOUNCE_FRAMES;
		}
		released = (~held) & prev;

		SceTouchData touchData{};
		sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touchData, 1);
		const bool touchDown = touchData.reportNum > 0;
		const bool touchPressed = touchDown && !touchWasDown;
		const bool touchReleased = !touchDown && touchWasDown;
		const float touchX = touchDown ? touchData.report[0].x * (SCREEN_W / 1920.0f) : 0.0f;
		const float touchY = touchDown ? touchData.report[0].y * (SCREEN_H / 1088.0f) : 0.0f;	
		sceneFade.update();

		if (sceneTransitionPending && sceneFade.done()) {
			if (pendingScene) {
				currentScene = pendingScene;
				player.x = pendingPlayerX;
				player.y = pendingPlayerY;
				pendingScene = nullptr;
			}
			gameState.state = pendingState;
			sceneTransitionPending = false;
			sceneFade.start(255.0f, 0.0f, FADE_SECONDS);
		}

		if (pad.buttons & SCE_CTRL_RTRIGGER) break; // change it later

		if (gameState.state == Playing && (pressed & SCE_CTRL_START)) {
			gameState.state = Paused;
		}

		if(player.state == Dead){
			dead_timer++;
			if(dead_timer >= DEATH_TIME_LIMIT) {
				gameState.state = GameOver;
				dead_timer = 0;
			}
		}

		if (gameState.state == MainMenuState) {
			if (soundReady) sounds.updateMenu(true);
			SceCtrlData menuPad = pad;
			menuPad.buttons = pressed;
			int selected = mainMenu.updateState(menuPad);
			if (selected == 0 && !sceneTransitionPending && sceneFade.done()) {
				pendingState = Playing;
				sceneTransitionPending = true;
				sceneFade.start(0.0f, 255.0f, FADE_SECONDS);
			} else if (selected == 1 && !sceneTransitionPending && sceneFade.done()) {
				pendingState = Exit;
				sceneTransitionPending = true;
				sceneFade.start(0.0f, 255.0f, FADE_SECONDS);
			}

			vita2d_start_drawing();
			vita2d_clear_screen();
			mainMenu.draw(font);
			sceneFade.draw();
			vita2d_end_drawing();
			vita2d_swap_buffers();
			touchWasDown = touchDown;
			continue;
		}

		if (gameState.state == Exit) break;
		if (soundReady) sounds.updateMenu(false);

		if (gameState.state == Paused) {
			SceCtrlData pausePad = pad;
			pausePad.buttons = pressed;
			const int selected = pauseMenu.updateState(pausePad, pressed);
			if (selected == 0) {
				gameState.state = Playing;
			} else if (selected == 1) {
				gameState.state = MainMenuState;
			} else if (selected == 3) {
				gameState.state = Exit;
			}

			vita2d_start_drawing();
			vita2d_clear_screen();
			currentScene->drawScene(player, deadTexture, idleTexture, walkTexture,
				cam_x, GROUND_Y, defaultFont, cross_btn);
			pauseMenu.draw(defaultFont);
			sceneFade.draw();
			vita2d_end_drawing();
			vita2d_swap_buffers();
			touchWasDown = touchDown;
			continue;
		}

		if(gameState.state == Playing) {
			if (soundReady) sounds.updateGameplay(player.state == Walking);
			// Handle playing logic here

			if ((pressed & SCE_CTRL_CIRCLE) && previousScene && !sceneTransitionPending && !inventoryOpen) {
				pendingScene = previousScene;
				pendingPlayerX = previousPlayerX;
				pendingPlayerY = previousPlayerY;
				previousScene = nullptr;
				sceneTransitionPending = true;
				sceneFade.start(0.0f, 255.0f, FADE_SECONDS);
			}

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
				if (currentScene->pointClickEnabled) {
					dir = 0.0f;

					bool busy = false;                       // ignore new taps while a dialogue is up
					for (auto &ia : currentScene->interactables) busy = busy || ia.dialogue.active;

					bool touchConsumed = false;              // one tap can only trigger one interactable
					for (auto it = currentScene->interactables.begin(); it != currentScene->interactables.end();) {
						auto &ia = *it;
						if (!ia.enabled) {
							ia.active = false;
							++it;
							continue;
						}

						const bool hit = touchPressed && !busy && !touchConsumed &&
							touchX >= ia.x && touchX <= ia.x + ia.width &&
							touchY >= ia.y && touchY <= ia.y + ia.height;

						if (hit) {
							touchConsumed = true;
							const bool hasItem = ia.requiredItem.empty() ||
												player.inventory.getItemQuantity(ia.requiredItem) > 0;
							ia.runLocked = !hasItem;

							if (!hasItem) {
								ia.dialogue.text = ia.closedText;
							} else if (ia.pickable) {
								player.inventory.addItem(ia.name, 1, ia.openText);
								ia.pickable = false;
								ia.removeAfterDialogue = true;
								ia.dialogue.text = ia.openText;
								ia.dialogue.text.push_back("Picked up " + ia.name + ".  (SELECT: inventory)");
							} else {
								ia.dialogue.text = ia.openText;
							}

							if (!ia.dialogue.text.empty()) {
								ia.dialogue.index = 0;
								ia.dialogue.visibleChars = 0;
								ia.dialogue.timer = 0;
								ia.dialogue.active = true;
								ia.dialogue.update(ia.dialogue.text);
							}
						}

						ia.active = hit || ia.dialogue.active;

						if (ia.dialogue.active && !hit && (pressed & SCE_CTRL_CROSS)) {
							if (ia.dialogue.index >= ia.dialogue.text.size() - 1) {
								ia.dialogue.active = false;
								if (ia.removeAfterDialogue) {
									if (ia.onInteract) ia.onInteract();
									it = currentScene->interactables.erase(it);
									continue;
								}
								if (!ia.runLocked) {                          // only the "real" run does things
									if (ia.onInteract) ia.onInteract();
									if (ia.scene && !sceneTransitionPending) {
										const bool returningToPreviousScene = ia.scene == previousScene;
										pendingScene = ia.scene;
										pendingPlayerX = returningToPreviousScene ? previousPlayerX : ia.scene->entryX;
										pendingPlayerY = returningToPreviousScene ? previousPlayerY : ia.scene->entryY;
										if (returningToPreviousScene) previousScene = nullptr;
										sceneTransitionPending = true;
										sceneFade.start(0.0f, 255.0f, FADE_SECONDS);
									}
								}
							} else {
								++ia.dialogue.index;
								ia.dialogue.visibleChars = 0;
								ia.dialogue.timer = 0;
							}
						}
						++it;
					}
				} else {

		// READ CONTROLLER INPUT ---------------------------------------------------------------

		// D-pad

		if(pad.buttons & SCE_CTRL_LEFT) dir -= speed;
		if(pad.buttons & SCE_CTRL_RIGHT) dir += speed;

		bool interactableDialogueActive = false;
		for (auto interactableIt = currentScene->interactables.begin();
			 interactableIt != currentScene->interactables.end();) {
			auto &interactable = *interactableIt;
			if (!interactable.enabled) {
				interactable.active = false;
				++interactableIt;
				continue;
			}
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
					interactableIt = currentScene->interactables.erase(interactableIt);
				continue;
			}

			if (interactable.dialogue.text.empty()) {
				interactable.dialogue.active = false;
				++interactableIt;
				continue;
			}

			if (pressed & SCE_CTRL_CROSS) {
				if (interactable.dialogue.active) {
					if (interactable.dialogue.index >= interactable.dialogue.text.size() - 1) {
						interactable.dialogue.active = false;
						if (!interactable.runLocked) {
							if (interactable.onInteract) interactable.onInteract();
							if (interactable.scene && !sceneTransitionPending) {
								previousScene = currentScene;
								previousPlayerX = player.x;
								previousPlayerY = player.y;
								pendingScene = interactable.scene;
								pendingPlayerX = interactable.scene->entryX;
								pendingPlayerY = interactable.scene->entryY;
								sceneTransitionPending = true;
								sceneFade.start(0.0f, 255.0f, FADE_SECONDS);
							}
						}
					} else {
						++interactable.dialogue.index;
						interactable.dialogue.visibleChars = 0;
						interactable.dialogue.timer = 0;
					}
				} else if (inRange && interactable.dialogue.interactable) {
					const bool hasItem = interactable.requiredItem.empty() ||
						player.inventory.getItemQuantity(interactable.requiredItem) > 0;
					interactable.runLocked = !hasItem;
					interactable.dialogue.text = hasItem
						? interactable.openText
						: interactable.closedText;
					if (!interactable.dialogue.text.empty()) {
						interactable.dialogue.index = 0;
						interactable.dialogue.update(interactable.dialogue.text);
						interactable.dialogue.active = true;
					}
				}
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
		for (auto &dialogue : currentScene->dialogues) {
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
			}

		// ----------------------------------------------------------------- READ CONTROLLER INPUT

		// UPDATE GAME STATE -----------------------------------------------------------------

		if(player.state != Dead){
			if(dir < 0) {
				player.facingRight = false;
				player.state = Walking;
			} else if(dir > 0) {
				player.facingRight = true;
				player.state = Walking;
				
			} else {
				player.state = Idle;
			}
		}

		if (player.state == Walking) {
			player.x += dir;
			player.y += 0; // No vertical movement
			player.health -= MOVEMENT_HEALTH_PENALTY; // Decrease health when moving
			if(!currentScene->parallaxEnabled) {
				if(player.x < currentScene->left_border) player.x = currentScene->left_border;
				if(player.x > currentScene->right_border) player.x = currentScene->right_border;
			}
		}
		if (currentScene->deathDistance > 0.0f &&
			std::fabs(player.x - currentScene->entryX) >= currentScene->deathDistance) {
			player.state = Dead;
		}
		cam_x = player.x - SCREEN_W / 2.0f;

        // if (player.y < 0) player.y = 0;
        // if (player.y > SCREEN_H - FRAME_H * SPRITE_SCALE) player.y = SCREEN_H - FRAME_H * SPRITE_SCALE;

		// ----------------------------------------------------------------- UPDATE GAME STATE

		// DRAWING -----------------------------------------------------------------

		vita2d_start_drawing();
		vita2d_clear_screen();

		currentScene->drawScene(player, deadTexture, idleTexture, walkTexture, cam_x, GROUND_Y, defaultFont, cross_btn);
		if (inventoryOpen) {
			drawInventoryPanel(player.inventory, defaultFont, selectedInventoryItem);
			if (inventoryDialogue.active) {
				vita2d_draw_rectangle(40, 390, 880, 120, RGBA8(20, 18, 24, 235));
				inventoryDialogue.animateDialogue(inventoryDialogue, defaultFont, nullptr,
					RGBA8(255, 255, 255, 255), 55, 410, inventoryDialogue.index);
			}
		}
		sceneFade.draw();

		vita2d_end_drawing();
		vita2d_swap_buffers();
		touchWasDown = touchDown;
		} else if(gameState.state == Paused) {
			// Handle paused logic here
		} else if(gameState.state == GameOver) {
			if(pressed & SCE_CTRL_CROSS) {
				gameState.state = MainMenuState;
				player.health = max_health; // Reset health
				player.state = Idle; // Reset state
				player.x = SCREEN_W / 2; // Reset position
				player.y = GROUND_Y; // Reset position
				previousScene = nullptr;
				pendingScene = nullptr;
				sceneTransitionPending = false;
				sceneFade.start(255.0f, 0.0f, FADE_SECONDS);
			}
			vita2d_start_drawing();
			vita2d_clear_screen();
			vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, RGBA8(0, 0, 0, 200));
			std::string gameOverText = (gameState.gameWon) ? "You thought you could escape?" : "Game Over";
			vita2d_pgf_draw_text(defaultFont, SCREEN_W / 2 - 100, SCREEN_H / 2, RGBA8(255, 0, 0, 255), 1.5f, gameOverText.c_str());
			vita2d_end_drawing();
			vita2d_swap_buffers();
		}

		player.health -= HEALTH_DECREASE_RATE;
		if(player.health <= 0) {
			player.state = Dead;
		}

		// ----------------------------------------------------------------- DRAWING
	}

	vita2d_fini();
	vita2d_free_texture(idleTexture);
	vita2d_free_texture(walkTexture);
	vita2d_free_texture(cross_btn);
	mainMenu.releaseTextures();
	currentScene->releaseBackgroundTextures();
	level1Scene.releaseBackgroundTextures();
	brickScene.releaseBackgroundTextures();
	mattressScene.releaseBackgroundTextures();
	forestScene.releaseBackgroundTextures();
	blankScene.releaseBackgroundTextures();
	vita2d_free_pgf(defaultFont);
	if (soundReady) sounds.shutdown();
	sceKernelExitProcess(0);
	return 0;

}
