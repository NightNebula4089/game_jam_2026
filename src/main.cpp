#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <vita2d.h>
#include <sstream>
#include <vector>
#include <psp2/appmgr.h>
#include <cstdio>

static const int SCREEN_WIDTH  = 960;
static const int SCREEN_HEIGHT = 544;

int main(int argc, char *argv[]) {

	vita2d_init();
	vita2d_set_clear_color(RGBA8(0, 0, 0, 255));

	vita2d_pgf *font = vita2d_load_default_pgf();

	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	SceCtrlData pad{};

	const float boxSize = 50.0f;
	const float speed = 5.0f;
	float x = (SCREEN_WIDTH - boxSize) / 2.0f;
	float y = (SCREEN_HEIGHT - boxSize) / 2.0f;

	while(true){
		sceCtrlPeekBufferPositive(0, &pad, 1);
		if(pad.buttons & SCE_CTRL_START) break;


		// D-pad
		if(pad.buttons & SCE_CTRL_UP) y -= speed;
		if(pad.buttons & SCE_CTRL_DOWN) y += speed;
		if(pad.buttons & SCE_CTRL_LEFT) x -= speed;
		if(pad.buttons & SCE_CTRL_RIGHT) x += speed;

		// Analog stick
		float lx = pad.lx - 128; // Center the value around 0
		float ly = pad.ly - 128; // Center the value around 0
		if(lx > 30 || lx < -30) x += (lx / 128.0f) * speed;
		if(ly > 30 || ly < -30) y += (ly / 128.0f) * speed;

		if(x < 0) x = 0;
		if(y < 0) y = 0;
		if(x > SCREEN_WIDTH - boxSize) x = SCREEN_WIDTH - boxSize;
		if(y > SCREEN_HEIGHT - boxSize) y = SCREEN_HEIGHT - boxSize;

		vita2d_start_drawing();
		vita2d_clear_screen();

		vita2d_draw_rectangle(x, y, boxSize, boxSize, RGBA8(255, 0, 0, 255));
		vita2d_pgf_draw_text(font,x+boxSize/2.0f, y+boxSize/2.0f,RGBA8(255, 255, 255, 255), 1.2f, "Hello, World!");

		vita2d_end_drawing();
		vita2d_swap_buffers();
	}

	vita2d_fini();
	vita2d_free_pgf(font);
	sceKernelExitProcess(0);
	return 0;

}
