// Home's palette fades as plain functions (src/home/fade.asm), for C++
// modules to call directly; their hooks (fades.cpp) wrap them for
// register-level callers, leaving the registers the asm leaves.
//
// A fade runs a step per frame on wTempBGPals and wTempOBPals (RGB555),
// which a V-Blank function applies. Its first frame installs that function
// (backing up wVBlankFunc), its second one switches it to the one applying
// the palettes from WRAM; once the fade has taken its frames, wVBlankFunc
// is restored and wSubState advances.
#pragma once

#include "../native.hpp"

namespace wl3::home {

// One frame of a fade; the wrappers need the rest.
struct FadeFrame {
	uint8_t counter; // wPalFadeCounter at the start
	bool stepped;    // the palettes moved (not just the first frame's set-up)
	uint8_t e;       // the palette offset the loop ended at (low byte)
	uint8_t c, a;    // the last colour's packing: green's top bits, its high byte
	uint8_t limit;   // the frames the fade takes
	bool done;       // it ended this frame
	uint8_t sub_state; // wSubState before the fade's end advanced it
};

// FadeBGToWhite and the speed setters before it (FADE_SPEED_*; 0xff: as set)
FadeFrame fade_bg_to_white(GB *gb, uint8_t speed = 0xff);
// DarkenBGToPal (and its setters): towards wTempPals1
FadeFrame darken_bg_to_pal(GB *gb, uint8_t speed = 0xff);
// SlowFadeInScreen: towards wTempPals1 a step at a time, in 33 frames
FadeFrame slow_fade_in_screen(GB *gb);

void vblank_88d(GB *gb); // VBlank_88d: back up wVBlankFunc, apply the palettes stored...
void vblank_8bf(GB *gb); // VBlank_8bf: ... then the ones from WRAM

} // namespace wl3::home
