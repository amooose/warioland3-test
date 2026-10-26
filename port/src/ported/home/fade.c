// Port of src/home/fade.asm
#include "ported.h"

// Raises one colour component (5 bits) by the fade step, capping it at $1f:
// +1 for FADE_SPEED_SLOW, +2 for FADE_SPEED_NORMAL, +4 otherwise.
static uint8_t lighten(uint8_t v, uint8_t speed)
{
	int step = speed == FADE_SPEED_SLOW ? 1 : speed == FADE_SPEED_NORMAL ? 2 : 4;
	v = (uint8_t)(v + step);
	return v >= 0x1f ? 0x1f : v;
}

// One step of fading the BG palettes to white, per frame. The first two
// frames set up the VBlank function that applies wTempBGPals; when the fade
// is done, the previous VBlank function is restored and wSubState advances.
PORT(FadeBGToWhite)
{
	uint8_t counter = rd8(gb, wPalFadeCounter);
	gb->cpu.a = counter;
	alu_cp(gb, counter, 2);
	if (counter < 2) {
		gb->cpu.a = alu_dec(gb, counter);
		if (counter != 1) {
			gb->cpu.a = 1;
			wr8(gb, wUnused_c186, 1);
			CALL(VBlank_88d);
			SET_HL(gb, wPalFadeCounter);
			wr8(gb, wPalFadeCounter, alu_inc(gb, rd8(gb, wPalFadeCounter)));
			return;
		}
		CALL(VBlank_8bf);
	}

	// .lighten: each colour is RGB555, little-endian, from wTempBGPals; it's
	// unpacked into wCurRed/Green/Blue, lightened and packed back.
	uint8_t speed = rd8(gb, wFadeSpeed);
	gb->cpu.d = (uint8_t)(wTempBGPals >> 8);
	uint8_t e = (uint8_t)wTempBGPals; // inc e / dec e never carry into d
	for (int i = 0; i < 8 * PAL_SIZE; i++) { // b = 8 palettes: also covers wTempOBPals
		uint16_t de = (uint16_t)(gb->cpu.d << 8);
		uint8_t lo = rd8(gb, de | e);
		uint8_t hi = rd8(gb, de | (uint8_t)(e + 1));
		uint8_t red = lo & 0x1f;
		uint8_t green = (uint8_t)(((lo & 0xe0) >> 5) | ((hi & 0x03) << 3));
		uint8_t blue = (hi & 0x7c) >> 2;

		uint8_t b = lighten(blue, speed);
		uint8_t g = lighten(green, speed);
		uint8_t r = lighten(red, speed);
		wr8(gb, wCurRed, r);
		wr8(gb, wCurGreen, g);
		wr8(gb, wCurBlue, b);

		// convert back to RGB555
		lo = (uint8_t)(r | (g & 0x07) << 5);
		wr8(gb, de | e, lo);
		gb->cpu.c = (g & 0x18) >> 3;
		hi = (uint8_t)(b << 2 | gb->cpu.c);
		wr8(gb, de | (uint8_t)(e + 1), hi);
		e = (uint8_t)(e + 2);
	}
	gb->cpu.e = e;
	gb->cpu.a = (uint8_t)((rd8(gb, wCurBlue) << 2) | gb->cpu.c);

	// the fade takes 33, 18 or 10 frames depending on the speed
	SET_HL(gb, wPalFadeCounter);
	counter = (uint8_t)(rd8(gb, wPalFadeCounter) + 1);
	wr8(gb, wPalFadeCounter, counter);
	gb->cpu.b = speed == FADE_SPEED_SLOW ? 33 : speed == FADE_SPEED_NORMAL ? 18 : 10;
	gb->cpu.a = counter;
	alu_cp(gb, counter, gb->cpu.b);
	if (gb->cpu.f & FLAG_C)
		return;

	// done: restore the VBlank function
	gb->cpu.ime = false; // di
	SET_HL(gb, wBackupVBlankFunc);
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 3;
	CALL(CopyHLToDE);
	gb->cpu.ime = true; // ei
	gb->cpu.ime_delay = 0;

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wFadeSpeed, 0);
	wr8(gb, wPalFadeCounter, 0);
	wr8(gb, wUnused_c186, 0);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}

PORT_JUMP(FadeBGToWhite_Normal)
{
	gb->cpu.a = FADE_SPEED_NORMAL;
	wr8(gb, wFadeSpeed, gb->cpu.a);
	JUMP(FadeBGToWhite); // jr
}

PORT_JUMP(SlowFadeBGToWhite)
{
	gb->cpu.a = FADE_SPEED_FAST;
	wr8(gb, wFadeSpeed, gb->cpu.a);
	JUMP(FadeBGToWhite); // jr
}

PORT_JUMP(FastFadeToWhite)
{
	gb->cpu.a = FADE_SPEED_SLOW;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wFadeSpeed, gb->cpu.a);
	JUMP(FadeBGToWhite); // fallthrough
}

// The .Func of VBlank_88d and VBlank_8bf: jp to the VBlank function proper.
#define VBLANK_88D_FUNC (ADDR_VBlank_88d + 25)
#define VBLANK_8BF_FUNC (ADDR_VBlank_8bf + 14)

// backs up wVBlankFunc and sets the one that applies the BG and OB pals
// from wTempBGPals / wTempOBPals
PORT(VBlank_88d)
{
	gb->cpu.ime = false; // di
	gb->cpu.ime_delay = 0;
	SET_HL(gb, wVBlankFunc);
	SET_DE(gb, wBackupVBlankFunc);
	gb->cpu.b = 3;
	CALL(CopyHLToDE);
	SET_HL(gb, VBLANK_88D_FUNC);
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 3;
	CALL(CopyHLToDE);
	gb->cpu.ime = true; // ei
}

PORT(VBlank_8bf)
{
	gb->cpu.ime = false; // di
	gb->cpu.ime_delay = 0;
	SET_HL(gb, VBLANK_8BF_FUNC);
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 3;
	CALL(CopyHLToDE);
	gb->cpu.ime = true; // ei
}
