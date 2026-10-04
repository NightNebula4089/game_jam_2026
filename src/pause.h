#ifndef PAUSE_H
#define PAUSE_H

#include <psp2/ctrl.h>
#include <vita2d.h>
#include <string>
#include <vector>

struct PauseMenu {
	std::vector<std::string> options;
	std::vector<std::string> hints;
	int selectedOption;
	int hintIndex;

	PauseMenu()
		: options({"Play", "Menu", "Hint", "Exit"}),
		  hints({
			  "Look closely at objects that seem out of place.",
			  "Some clues can be reviewed from the inventory.",
			  "The loose brick may need something sharp.",
			  "Read every note before choosing a brick.",
			  "The way out was hidden, not locked."
		  }),
		  selectedOption(0),
		  hintIndex(0) {}

	void updateSelection(int direction);
	int updateState(const SceCtrlData &pad, unsigned int pressed);
	void draw(vita2d_pgf *font);
};

#endif
