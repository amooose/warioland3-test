// Port of src/engine/overworld/bank21.asm
#include "ported.h"

static uint8_t rlca(GB *gb, uint8_t v)
{
	gb->cpu.f = (v & 0x80) ? FLAG_C : 0;
	return (uint8_t)(v << 1 | v >> 7);
}

static uint8_t rrca(GB *gb, uint8_t v)
{
	gb->cpu.f = (v & 0x01) ? FLAG_C : 0;
	return (uint8_t)(v >> 1 | v << 7);
}

// Unpacks the colour at de into red, green and blue at hl, leaving e one
// up; returns hl at blue.
static uint16_t unpack_rgb(GB *gb, uint16_t hl)
{
	gb->cpu.a = alu_and(gb, rd8(gb, REG_DE(gb)), 0x1f);
	wr8(gb, hl++, gb->cpu.a); // red
	gb->cpu.a = alu_and(gb, rd8(gb, REG_DE(gb)), 0xe0);
	for (int i = 0; i < 5; i++)
		gb->cpu.a = rrca(gb, gb->cpu.a);
	wr8(gb, hl, gb->cpu.a);
	gb->cpu.e = alu_inc(gb, gb->cpu.e);
	gb->cpu.a = alu_and(gb, rd8(gb, REG_DE(gb)), 0x03);
	for (int i = 0; i < 3; i++)
		gb->cpu.a = rlca(gb, gb->cpu.a);
	gb->cpu.a = alu_or(gb, gb->cpu.a, rd8(gb, hl));
	wr8(gb, hl++, gb->cpu.a); // green
	gb->cpu.a = alu_and(gb, rd8(gb, REG_DE(gb)), 0x7c);
	gb->cpu.a = rrca(gb, gb->cpu.a);
	gb->cpu.a = rrca(gb, gb->cpu.a);
	wr8(gb, hl, gb->cpu.a); // blue
	return hl;
}

// Moves the component at hl one step towards the one at target.
static void step_towards(GB *gb, uint16_t hl, uint16_t target)
{
	gb->cpu.a = rd8(gb, target);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (gb->cpu.f & FLAG_Z)
		return;
	if (gb->cpu.f & FLAG_C)
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	else
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
}

// dec l
static uint16_t dec_l(uint16_t hl)
{
	return (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 1));
}

// hl = total steps of the fade
// Steps each colour of the palette config being faded one step towards its
// target, wPalFadeRepeat times; stores the steps done at hl and compares
// them with $20.
PORT(FadePalConfig)
{
	uint16_t steps_ptr = REG_HL(gb);
	gb_push16(gb, steps_ptr);
	gb->cpu.a = rd8(gb, steps_ptr);
	wr8(gb, wCurPalTotalSteps, gb->cpu.a);
	uint16_t hl;
	do {
		gb->cpu.a = rd8(gb, wPalConfigToFade);
		hl = wPalConfig1Number;
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z))
			hl = wPalConfig2Number;
		gb->cpu.a = rd8(gb, hl++); // number of pals
		gb->cpu.a = rlca(gb, gb->cpu.a);
		gb->cpu.a = rlca(gb, gb->cpu.a); // *4
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_dec(gb, rd8(gb, hl++)); // source hi
		gb->cpu.d = gb->cpu.a;
		gb->cpu.e = rd8(gb, hl); // source lo
		hl = wTargetRGB;
		do {
			hl = unpack_rgb(gb, hl) + 1; // wTargetRed, wTargetGreen, wTargetBlue
			gb->cpu.e = alu_dec(gb, gb->cpu.e);

			gb->cpu.d = wTempBGPals >> 8; // aka HIGH(wTempOBPals)
			hl = unpack_rgb(gb, hl); // wCurRed, wCurGreen, wCurBlue

			step_towards(gb, hl, wTargetBlue);
			hl = dec_l(hl);
			step_towards(gb, hl, wTargetBlue - 1); // wTargetGreen
			hl = dec_l(hl);
			step_towards(gb, hl, wTargetRed);

			// pack wCurRGB back into de
			gb->cpu.a = rd8(gb, hl++); // wCurRed
			gb->cpu.c = gb->cpu.a;
			gb->cpu.a = alu_and(gb, rd8(gb, hl), 0x07); // wCurGreen
			for (int i = 0; i < 5; i++)
				gb->cpu.a = rlca(gb, gb->cpu.a);
			gb->cpu.a = alu_or(gb, gb->cpu.a, gb->cpu.c);
			gb->cpu.e = alu_dec(gb, gb->cpu.e);
			wr8(gb, REG_DE(gb), gb->cpu.a);
			gb->cpu.a = alu_and(gb, rd8(gb, hl++), 0x18);
			for (int i = 0; i < 3; i++)
				gb->cpu.a = rrca(gb, gb->cpu.a);
			gb->cpu.c = gb->cpu.a;
			gb->cpu.a = rd8(gb, hl--); // wCurBlue
			gb->cpu.a = rlca(gb, gb->cpu.a);
			gb->cpu.a = rlca(gb, gb->cpu.a);
			gb->cpu.a = alu_or(gb, gb->cpu.a, gb->cpu.c);
			gb->cpu.e = alu_inc(gb, gb->cpu.e);
			wr8(gb, REG_DE(gb), gb->cpu.a);

			gb->cpu.e = alu_inc(gb, gb->cpu.e);
			gb->cpu.d = wTempPals1 >> 8;
			hl = wTargetRGB;
			gb->cpu.b = alu_dec(gb, gb->cpu.b);
		} while (!(gb->cpu.f & FLAG_Z));

		hl = wCurPalTotalSteps;
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
		gb->cpu.a = alu_dec(gb, rd8(gb, wPalFadeRepeat));
		wr8(gb, wPalFadeRepeat, gb->cpu.a);
	} while (!(gb->cpu.f & FLAG_Z));

	gb->cpu.a = rd8(gb, hl);
	steps_ptr = gb_pop16(gb);
	SET_HL(gb, steps_ptr);
	wr8(gb, steps_ptr, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x20);
}
