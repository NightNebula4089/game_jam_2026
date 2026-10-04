#include <pause.h>

void PauseMenu::updateSelection(int direction) {
	selectedOption += direction;
	if (selectedOption < 0) {
		selectedOption = static_cast<int>(options.size()) - 1;
	} else if (selectedOption >= static_cast<int>(options.size())) {
		selectedOption = 0;
	}
}

int PauseMenu::updateState(const SceCtrlData &pad, unsigned int pressed) {
	if (pad.buttons & SCE_CTRL_UP) {
		updateSelection(-1);
	} else if (pad.buttons & SCE_CTRL_DOWN) {
		updateSelection(1);
	} else if (pressed & SCE_CTRL_CROSS) {
		if (selectedOption == 2) {
			if (hintIndex < static_cast<int>(hints.size()) - 1) {
				++hintIndex;
			}
			return -1;
		}
		return selectedOption;
	}
	return -1;
}

void PauseMenu::draw(vita2d_pgf *font) {
	vita2d_draw_rectangle(160, 70, 640, 404, RGBA8(15, 15, 22, 245));
	vita2d_pgf_draw_text(font, 220, 115, RGBA8(255, 255, 255, 255), 1.2f, "Paused");

	for (size_t i = 0; i < options.size(); ++i) {
		const unsigned int color = static_cast<int>(i) == selectedOption
			? RGBA8(255, 220, 80, 255)
			: RGBA8(255, 255, 255, 255);
		vita2d_pgf_draw_text(
			font,
			220,
			170 + static_cast<int>(i) * 45,
			color,
			1.0f,
			options[i].c_str()
		);
	}

	vita2d_pgf_draw_text(font, 220, 380, RGBA8(180, 220, 255, 255), 1.0f,
						 hints[hintIndex].c_str());
}
