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

// The first two frames of a fade set up the VBlank function that applies
// wTempBGPals: returns whether to go on with this frame's step.
static bool fade_begin(GB *gb)
{
	uint8_t counter = rd8(gb, wPalFadeCounter);
	gb->cpu.a = counter;
	alu_cp(gb, counter, 2);
	if (counter >= 2)
		return true;
	gb->cpu.a = alu_dec(gb, counter);
	if (counter == 1) {
		CALL(VBlank_8bf);
		return true;
	}
	gb->cpu.a = 1;
	wr8(gb, wUnused_c186, 1);
	CALL(VBlank_88d);
	SET_HL(gb, wPalFadeCounter);
	wr8(gb, wPalFadeCounter, alu_inc(gb, rd8(gb, wPalFadeCounter))); // set up to begin fade
	return false;
}

// Counts the frame; once the fade has taken 33 frames (or 18 / 10 for the
// faster speeds, if by_speed), the previous VBlank function is restored and
// wSubState advances.
static void fade_end(GB *gb, bool by_speed)
{
	SET_HL(gb, wPalFadeCounter);
	uint8_t counter = (uint8_t)(rd8(gb, wPalFadeCounter) + 1);
	wr8(gb, wPalFadeCounter, counter);
	uint8_t speed = rd8(gb, wFadeSpeed);
	gb->cpu.b = 33;
	if (by_speed)
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

// One step of fading the BG palettes to white, per frame.
PORT(FadeBGToWhite)
{
	if (!fade_begin(gb))
		return;
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

	fade_end(gb, true);
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

// RGB555, little-endian, at lo / hi, into red, green and blue at rgb.
static void unpack_rgb(GB *gb, uint16_t lo, uint16_t hi, uint16_t rgb)
{
	wr8(gb, rgb + 0, rd8(gb, lo) & 0x1f);
	wr8(gb, rgb + 1, (uint8_t)(((rd8(gb, lo) & 0xe0) >> 5) | ((rd8(gb, hi) & 0x03) << 3)));
	wr8(gb, rgb + 2, (uint8_t)((rd8(gb, hi) & 0x7c) >> 2));
}

// Packs wCurRGB back into lo / hi. Components aren't masked, so one that
// wrapped below 0 (see darken) spills into the others, as in the asm.
static void pack_cur_rgb(GB *gb, uint16_t lo, uint16_t hi)
{
	uint8_t r = rd8(gb, wCurRed), g = rd8(gb, wCurGreen), b = rd8(gb, wCurBlue);
	wr8(gb, lo, (uint8_t)(r | (g & 0x07) << 5));
	gb->cpu.c = (g & 0x18) >> 3;
	gb->cpu.a = (uint8_t)((b << 2 | b >> 6) | gb->cpu.c); // rlca / rlca / or c
	wr8(gb, hi, gb->cpu.a);
}

// Steps every colour of wTempBGPals (and wTempOBPals) towards wTempPals1,
// one component at a time (blue, green, red) with step(cur, target).
static void fade_to_pals1(GB *gb, uint8_t (*step)(GB *gb, uint8_t cur, uint8_t target))
{
	uint8_t e = (uint8_t)wTempPals1; // inc e / dec e never carry into d
	for (int i = 0; i < 8 * PAL_SIZE; i++) { // b = 8 palettes
		uint16_t target = (uint16_t)((wTempPals1 & 0xff00) | e);
		uint16_t cur = (uint16_t)((wTempBGPals & 0xff00) | e);
		uint16_t target_hi = (uint16_t)((target & 0xff00) | (uint8_t)(e + 1));
		uint16_t cur_hi = (uint16_t)((cur & 0xff00) | (uint8_t)(e + 1));
		unpack_rgb(gb, target, target_hi, wTargetRGB);
		unpack_rgb(gb, cur, cur_hi, wCurRGB);
		for (int c = 2; c >= 0; c--) {
			gb->cpu.c = rd8(gb, wTargetRGB + c);
			wr8(gb, wCurRGB + c, step(gb, rd8(gb, wCurRGB + c), gb->cpu.c));
		}
		pack_cur_rgb(gb, cur, cur_hi);
		e = (uint8_t)(e + 2);
	}
	gb->cpu.b = 0;
	gb->cpu.d = (uint8_t)(wTempPals1 >> 8);
	gb->cpu.e = e;
	SET_HL(gb, wTargetRGB);
}

// Lowers a component by 1, 2 or 4 by speed, stopping at 0, then raises it
// to the target if it went below. A component that starts at 0 wraps to
// $ff and down, and is kept (it's not below the target).
static uint8_t darken(GB *gb, uint8_t v, uint8_t target)
{
	uint8_t speed = rd8(gb, wFadeSpeed);
	int steps = speed == FADE_SPEED_SLOW ? 1 : speed == FADE_SPEED_NORMAL ? 2 : 4;
	for (int i = 0; i < steps; i++) {
		v = (uint8_t)(v - 1);
		if (v == 0)
			break;
	}
	return v < target ? target : v;
}

// fades BG palettes to wTempPals1
// gradually darkens up to wTempPals1
PORT(DarkenBGToPal)
{
	if (!fade_begin(gb))
		return;
	fade_to_pals1(gb, darken);
	fade_end(gb, true);
}

PORT_JUMP(DarkenBGToPal_Normal)
{
	gb->cpu.a = FADE_SPEED_NORMAL;
	wr8(gb, wFadeSpeed, gb->cpu.a);
	JUMP(DarkenBGToPal); // jr
}

PORT_JUMP(DarkenBGToPal_Fast)
{
	gb->cpu.a = FADE_SPEED_FAST;
	wr8(gb, wFadeSpeed, gb->cpu.a);
	JUMP(DarkenBGToPal); // jr
}

PORT_JUMP(SlowFadeFromWhite)
{
	gb->cpu.a = FADE_SPEED_SLOW;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wFadeSpeed, gb->cpu.a);
	JUMP(DarkenBGToPal); // fallthrough
}

// Moves a component 1 towards the target.
static uint8_t move_towards(GB *gb, uint8_t v, uint8_t target)
{
	(void)gb;
	return target == v ? v : target > v ? (uint8_t)(v + 1) : (uint8_t)(v - 1);
}

// fades BG palettes to wTempPals1, in 33 frames
PORT(SlowFadeInScreen)
{
	if (!fade_begin(gb))
		return;
	fade_to_pals1(gb, move_towards);
	fade_end(gb, false);
}

// The end of Func_54e / Func_7cf: like fade_end(gb, false), but wFadeSpeed
// is left alone.
static void fade_end_unref(GB *gb)
{
	SET_HL(gb, wPalFadeCounter);
	uint8_t counter = alu_inc(gb, rd8(gb, wPalFadeCounter));
	wr8(gb, wPalFadeCounter, counter);
	gb->cpu.a = counter;
	alu_cp(gb, counter, 0x21);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.ime = false; // di
	SET_HL(gb, wBackupVBlankFunc);
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 3;
	CALL(CopyHLToDE);
	gb->cpu.ime = true; // ei
	gb->cpu.ime_delay = 0;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPalFadeCounter, 0);
	wr8(gb, wUnused_c186, 0);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}

// unreferenced: darkens wTempBGPals (and wTempOBPals) by 1 per component
// and frame, down to 0, in 33 frames
PORT(Func_54e)
{
	if (!fade_begin(gb))
		return;
	uint8_t e = (uint8_t)wTempBGPals; // inc e / dec e never carry into d
	for (int i = 0; i < 8 * PAL_SIZE; i++) {
		uint16_t lo = (uint16_t)((wTempBGPals & 0xff00) | e);
		uint16_t hi = (uint16_t)((wTempBGPals & 0xff00) | (uint8_t)(e + 1));
		unpack_rgb(gb, lo, hi, wCurRGB);
		for (int c = 2; c >= 0; c--) {
			uint8_t v = rd8(gb, wCurRGB + c);
			gb->cpu.a = alu_and(gb, v, v);
			if (v)
				wr8(gb, wCurRGB + c, alu_dec(gb, v));
		}
		pack_cur_rgb(gb, lo, hi);
		e = (uint8_t)(e + 2);
	}
	gb->cpu.b = 0;
	gb->cpu.d = (uint8_t)(wTempBGPals >> 8);
	gb->cpu.e = e;
	fade_end_unref(gb);
}

// unreferenced: raises each component of wTempBGPals (and wTempOBPals) by 1
// per frame until it equals wTempPals1's, in 33 frames
PORT(Func_7cf)
{
	if (!fade_begin(gb))
		return;
	uint8_t e = (uint8_t)wTempPals1; // inc e / dec e never carry into d
	for (int i = 0; i < 0x40; i++) {
		uint16_t target = (uint16_t)(0xc000 | e);
		uint16_t target_hi = (uint16_t)(0xc000 | (uint8_t)(e + 1));
		uint16_t cur = (uint16_t)(0xc100 | e);
		uint16_t cur_hi = (uint16_t)(0xc100 | (uint8_t)(e + 1));
		unpack_rgb(gb, target, target_hi, wTargetRGB);
		unpack_rgb(gb, cur, cur_hi, wCurRGB);
		for (int c = 2; c >= 0; c--) {
			gb->cpu.c = rd8(gb, wTargetRGB + c);
			uint8_t v = rd8(gb, wCurRGB + c);
			gb->cpu.a = v;
			alu_cp(gb, v, gb->cpu.c);
			if (!(gb->cpu.f & FLAG_Z))
				wr8(gb, wCurRGB + c, alu_inc(gb, v));
		}
		pack_cur_rgb(gb, cur, cur_hi);
		e = (uint8_t)(e + 2);
	}
	gb->cpu.b = 0;
	gb->cpu.d = 0xc0;
	gb->cpu.e = e;
	fade_end_unref(gb);
}
