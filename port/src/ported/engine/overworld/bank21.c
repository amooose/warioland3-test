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

// Fades palette config 1 (background) or 2 (objects), slowly.
static void fade_pal_config_slow(GB *gb, uint16_t reg_addr, uint8_t reg, uint8_t config, uint16_t total_steps)
{
	gb->cpu.a = reg;
	wr8(gb, reg_addr, reg);
	if (config) {
		gb->cpu.a = config;
	} else {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
	}
	wr8(gb, wPalConfigToFade, config);
	gb->cpu.a = 1; // slow fade
	wr8(gb, wPalFadeRepeat, 1);
	SET_HL(gb, total_steps);
	gb_call(gb, ADDR_FadePalConfig);
}

PORT(Func_851bc)
{
	fade_pal_config_slow(gb, wPalConfig1Register, (uint8_t)rBGPI, 0, wPalConfig1TotalSteps);
}

PORT(Func_851d1)
{
	fade_pal_config_slow(gb, wPalConfig2Register, (uint8_t)rOBPI, 1, wPalConfig2TotalSteps);
}

// Copies .Func, the VBlank handler run from WRAM, to wVBlankFunc
PORT(VBlank_OWPalTransition)
{
	SET_HL(gb, ADDR_VBlank_OWPalTransition + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = (uint8_t)(ADDR_OWPals - (ADDR_VBlank_OWPalTransition + 0x0c)); // .end - .Func
	CALL(CopyHLToDE);
}

// hl = table + wCurMapSide * size
static void get_map_side_pal_group(GB *gb, uint16_t table, uint16_t size)
{
	uint16_t hl = table;
	SET_DE(gb, size);
	gb->cpu.a = alu_inc(gb, rd8(gb, wCurMapSide));
	for (;;) {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z)
			break;
		hl = alu_add16(gb, hl, size);
	}
	SET_HL(gb, hl);
}

PORT(GetMapSidePalGroup1) { get_map_side_pal_group(gb, ADDR_Pals_84000, 40 * PAL_SIZE); }
PORT(GetMapSidePalGroup2) { get_map_side_pal_group(gb, ADDR_Pals_84900, 8 * PAL_SIZE); }

// xor a / ld hl, start / ld bc, size / call WriteAToHL_BCTimes
static void clear_bytes(GB *gb, uint16_t start, uint16_t size)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, start);
	SET_BC(gb, size);
	CALL(WriteAToHL_BCTimes);
}

PORT(ClearTempPals_Bank21)
{
	clear_bytes(gb, wTempPals1, 8 * PAL_SIZE);
	clear_bytes(gb, wTempPals2, 8 * PAL_SIZE);
}

PORT(Func_84fd5)
{
	clear_bytes(gb, wTempBGPals, 8 * PAL_SIZE);
	clear_bytes(gb, wTempOBPals, 8 * PAL_SIZE);
}

// ld hl, src / ld de, dest / ld b, n / call CopyHLToDE
static void copy_bytes(GB *gb, uint16_t src, uint16_t dest, uint8_t n)
{
	SET_HL(gb, src);
	SET_DE(gb, dest);
	gb->cpu.b = n;
	CALL(CopyHLToDE);
}

PORT(SkipDayNightTransition)
{
	copy_bytes(gb, wTempPals1, wTempBGPals, 8 * PAL_SIZE);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wDayNightTransistionSteps, 0);
	wr8(gb, wOWPalTransitionCounter, 0);
	gb->cpu.b = 0x06;
	gb->cpu.a = rd8(gb, w2d011);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		gb->cpu.b = 0x04;
	gb->cpu.a = gb->cpu.b;
	wr8(gb, wOWPalTransitionState, gb->cpu.a);
}

// wPalConfig*: Register, Number, SourceHi, SourceLo, Index
static void set_pal_config(GB *gb, int config, uint16_t src, uint8_t index, uint8_t number)
{
	uint16_t source_hi = config == 1 ? wPalConfig1SourceHi : wPalConfig2SourceHi;
	uint16_t source_lo = config == 1 ? wPalConfig1SourceLo : wPalConfig2SourceLo;
	uint16_t index_addr = config == 1 ? wPalConfig1Index : wPalConfig2Index;
	uint16_t number_addr = config == 1 ? wPalConfig1Number : wPalConfig2Number;
	gb->cpu.a = (uint8_t)(src >> 8);
	wr8(gb, source_hi, gb->cpu.a);
	gb->cpu.a = (uint8_t)src;
	wr8(gb, source_lo, gb->cpu.a);
	gb->cpu.a = index;
	wr8(gb, index_addr, gb->cpu.a);
	gb->cpu.a = number;
	wr8(gb, number_addr, gb->cpu.a);
}

PORT(LoadOWStarIndicatorPals)
{
	copy_bytes(gb, ADDR_Pals_848e0, wTempPals2 + 4 * PAL_SIZE, 2 * PAL_SIZE);
	set_pal_config(gb, 1, wTempPals2 + 4 * PAL_SIZE, OBPI_AUTOINC | 4 * PAL_SIZE, 2);
	gb->cpu.a = (uint8_t)rOBPI;
	wr8(gb, wPalConfig1Register, gb->cpu.a);
}

// de = pointer: adds $20 to it, and returns de past it
PORT(Func_85a91)
{
	uint16_t de = REG_DE(gb);
	gb->cpu.a = rd8(gb, de);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.a = rd8(gb, (uint16_t)(de + 1));
	gb->cpu.h = gb->cpu.a;
	SET_BC(gb, 0x20);
	uint16_t hl = alu_add16(gb, REG_HL(gb), 0x20);
	SET_HL(gb, hl);
	gb->cpu.a = (uint8_t)hl;
	wr8(gb, de++, gb->cpu.a);
	gb->cpu.a = (uint8_t)(hl >> 8);
	wr8(gb, de++, gb->cpu.a);
	SET_DE(gb, de);
}

// hl = list of: address, bytes for it until $70, bytes for the address
// $300 below until $70; ends with 0
PORT(Func_854ee)
{
	uint16_t hl = REG_HL(gb);
	for (;;) {
		gb->cpu.a = rd8(gb, hl++);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z)
			break;
		gb->cpu.e = gb->cpu.a;
		gb->cpu.c = gb->cpu.a;
		gb->cpu.a = rd8(gb, hl++);
		gb->cpu.d = gb->cpu.a;
		gb->cpu.b = gb->cpu.a;
		uint16_t de = REG_DE(gb);
		for (;;) {
			gb->cpu.a = rd8(gb, hl++);
			alu_cp(gb, gb->cpu.a, 0x70);
			if (gb->cpu.f & FLAG_Z)
				break;
			wr8(gb, de++, gb->cpu.a);
		}
		SET_DE(gb, de);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
		uint16_t bc = REG_BC(gb);
		for (;;) {
			gb->cpu.a = rd8(gb, hl++);
			alu_cp(gb, gb->cpu.a, 0x70);
			if (gb->cpu.f & FLAG_Z)
				break;
			wr8(gb, bc++, gb->cpu.a);
		}
		SET_BC(gb, bc);
	}
	SET_HL(gb, hl);
}

// ld a, [wPalConfig*TotalSteps] / cp 1
static bool pal_fade_started(GB *gb, uint16_t total_steps)
{
	gb->cpu.a = rd8(gb, total_steps);
	alu_cp(gb, gb->cpu.a, 0x01);
	return !(gb->cpu.f & FLAG_C);
}

// ld hl, [wPalConfig*TotalSteps] / inc [hl]
static void inc_total_steps(GB *gb, uint16_t total_steps)
{
	SET_HL(gb, total_steps);
	wr8(gb, total_steps, alu_inc(gb, rd8(gb, total_steps)));
}

// sets up both pal configs for all of wTempBGPals and wTempOBPals
static void set_both_pal_configs(GB *gb)
{
	set_pal_config(gb, 1, wTempBGPals, BGPI_AUTOINC | 0, 8);
	set_pal_config(gb, 2, wTempOBPals, OBPI_AUTOINC | 0, 8);
}

// the fade's step: returns false on the ret c (not finished)
static bool fade_step(GB *gb, bool obj, bool bg)
{
	if (obj)
		gb_call(gb, ADDR_Func_851d1);
	if (bg)
		gb_call(gb, ADDR_Func_851bc);
	return !(gb->cpu.f & FLAG_C);
}

// xor a / ld [total_steps], a / ld [counter], a / ld hl, next / inc [hl]
static void fade_done(GB *gb, uint16_t total_steps, uint16_t counter, uint16_t next)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, total_steps, 0);
	wr8(gb, counter, 0);
	SET_HL(gb, next);
	wr8(gb, next, alu_inc(gb, rd8(gb, next)));
}

PORT(Func_84e43)
{
	if (!pal_fade_started(gb, wPalConfig1TotalSteps)) {
		inc_total_steps(gb, wPalConfig1TotalSteps);
		set_both_pal_configs(gb);
		return;
	}
	if (!fade_step(gb, true, true))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPalConfig1TotalSteps, 0);
	wr8(gb, wPalConfig2TotalSteps, 0);
}

PORT(Func_8518c)
{
	if (!pal_fade_started(gb, wPalConfig1TotalSteps)) {
		set_pal_config(gb, 1, wTempBGPals, OBPI_AUTOINC | 0, 8);
		inc_total_steps(gb, wPalConfig1TotalSteps);
		return;
	}
	if (!fade_step(gb, false, true))
		return;
	fade_done(gb, wPalConfig1TotalSteps, wOWFuncCounter, w2d062);
}

// copies the pals to the temp pals, and fills the source with white
static void copy_and_whiten(GB *gb, uint16_t src, uint16_t dest, uint8_t size)
{
	copy_bytes(gb, src, dest, size);
	gb->cpu.a = 0xff;
	SET_HL(gb, src);
	SET_BC(gb, size);
	CALL(WriteAToHL_BCTimes);
}

PORT(Func_850b9)
{
	if (!pal_fade_started(gb, wPalConfig2TotalSteps)) {
		copy_and_whiten(gb, wTempPals2, wTempOBPals, 8 * PAL_SIZE);
		inc_total_steps(gb, wPalConfig2TotalSteps);
		set_pal_config(gb, 2, wTempOBPals, OBPI_AUTOINC | 0, 8);
		return;
	}
	if (!fade_step(gb, true, false))
		return;
	fade_done(gb, wPalConfig2TotalSteps, w2d014, w2d013);
}

PORT(Func_850ff)
{
	if (!pal_fade_started(gb, wPalConfig1TotalSteps)) {
		copy_and_whiten(gb, wTempPals1 + PAL_SIZE, wTempBGPals + PAL_SIZE, 7 * PAL_SIZE);
		inc_total_steps(gb, wPalConfig1TotalSteps);
		set_pal_config(gb, 1, wTempBGPals + PAL_SIZE, BGPI_AUTOINC | PAL_SIZE, 7);
		return;
	}
	if (!fade_step(gb, false, true))
		return;
	fade_done(gb, wPalConfig1TotalSteps, w2d014, w2d013);
}

PORT(Func_85145)
{
	if (!pal_fade_started(gb, wPalConfig1TotalSteps)) {
		inc_total_steps(gb, wPalConfig1TotalSteps);
		set_both_pal_configs(gb);
		return;
	}
	if (!fade_step(gb, true, true))
		return;
	fade_done(gb, wPalConfig1TotalSteps, wOWFuncCounter, w2d062);
}

PORT(Func_84fff)
{
	if (!pal_fade_started(gb, wPalConfig1TotalSteps)) {
		inc_total_steps(gb, wPalConfig1TotalSteps);
		set_both_pal_configs(gb);
		return;
	}
	if (!fade_step(gb, true, true))
		return;
	fade_done(gb, wPalConfig1TotalSteps, w2d014, w2d013);
}

PORT(Func_85046)
{
	if (!pal_fade_started(gb, wPalConfig1TotalSteps)) {
		copy_and_whiten(gb, wTempPals1 + PAL_SIZE, wTempBGPals + PAL_SIZE, 7 * PAL_SIZE);
		copy_and_whiten(gb, wTempPals2, wTempOBPals, 8 * PAL_SIZE);
		inc_total_steps(gb, wPalConfig1TotalSteps);
		set_pal_config(gb, 1, wTempBGPals + PAL_SIZE, BGPI_AUTOINC | PAL_SIZE, 7);
		set_pal_config(gb, 2, wTempOBPals, OBPI_AUTOINC | 0, 8);
		return;
	}
	if (!fade_step(gb, true, true))
		return;
	fade_done(gb, wPalConfig1TotalSteps, w2d014, w2d013);
}

// a = palette set
// output: hl = its pals in OWPals
PORT(GetOWPals)
{
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.c = gb->cpu.a;
	// .CheckCrayons
	gb->cpu.a = alu_and(gb, rd8(gb, wCrayonFlags), ALL_CRAYONS);
	alu_cp(gb, gb->cpu.a, ALL_CRAYONS);
	gb->cpu.a = rd8(gb, wCurMapSide);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = 0x4; // north
		} else {
			alu_cp(gb, gb->cpu.a, WEST);
			if (gb->cpu.f & FLAG_Z)
				gb->cpu.a = 0x5;
		}
	}
	gb->cpu.a = alu_swap(gb, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	gb->cpu.a = alu_swap(gb, rd8(gb, w2d011));
	gb->cpu.a = rrca(gb, gb->cpu.a);
	gb->cpu.a = alu_or(gb, gb->cpu.a, gb->cpu.e);
	gb->cpu.a = alu_or(gb, gb->cpu.a, gb->cpu.c);
	gb->cpu.e = gb->cpu.a;
	SET_HL(gb, alu_add16(gb, ADDR_OWPals, REG_DE(gb)));
}

PORT(LoadGreyOWPals)
{
	gb->cpu.a = rd8(gb, wCurMapSide);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	uint16_t hl = alu_add16(gb, ADDR_Data_84e39, REG_DE(gb));
	gb->cpu.a = EAST;
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_Z) {
		// .GetEastMapNoIcePalette
		gb_push16(gb, hl); // push hl
		gb->cpu.a = BLUE_BOOK;
		CALL(IsTreasureCollected);
		bool next = gb->cpu.f & FLAG_Z;
		if (!next) {
			gb->cpu.a = MAGIC_WAND;
			CALL(IsTreasureCollected);
			next = gb->cpu.f & FLAG_Z;
		}
		hl = gb_pop16(gb); // pop hl
		if (next)
			hl += 2; // .next_pal
	}
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.h = rd8(gb, hl);
	gb->cpu.l = gb->cpu.a;
	SET_DE(gb, wTempPals1);
	gb->cpu.b = 8 * PAL_SIZE;
	CALL(CopyHLToDE);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, wTempPals1 + 7 * PAL_SIZE + 1 * 2);
	SET_BC(gb, 0x6);
	CALL(WriteAToHL_BCTimes);
}

// sets up the colour fade structs in wFadePals from their base and target
PORT(InitColourFadeStructs)
{
	gb->cpu.a = 8 * PAL_COLORS * 3;
	wr8(gb, w2d807, gb->cpu.a);
	uint16_t hl = wFadePals;
	for (;;) {
		gb->cpu.a = rd8(gb, hl++);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = rd8(gb, hl++);
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, hl++, 0);
			wr8(gb, hl++, 0);
			wr8(gb, hl++, 0);
			wr8(gb, hl, gb->cpu.e);
		} else {
			gb->cpu.d = 0x01;
			if (gb->cpu.f & FLAG_C) {
				gb->cpu.d = gb->cpu.a;
				gb->cpu.a = gb->cpu.b;
				gb->cpu.b = gb->cpu.d;
				gb->cpu.d = 0x02;
			}
			gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
			gb->cpu.b = gb->cpu.a;
			gb->cpu.a = gb->cpu.d;
			wr8(gb, hl++, gb->cpu.a);
			gb->cpu.a = gb->cpu.b;
			wr8(gb, hl++, gb->cpu.a);
			wr8(gb, hl++, gb->cpu.a);
			wr8(gb, hl, gb->cpu.e);
		}
		SET_BC(gb, (COLOURFADESTRUCT_LENGTH + COLOURFADESTRUCT_BASE) - COLOURFADESTRUCT_CURRENT);
		hl = alu_add16(gb, hl, REG_BC(gb));
		gb->cpu.a = alu_dec(gb, rd8(gb, w2d807));
		wr8(gb, w2d807, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z)
			break;
	}
	SET_HL(gb, hl);
}

// de = palettes, hl = colour fade struct field: unpacks each colour into
// red, green and blue of three consecutive structs
PORT(CopyPalToColourFadeStructs)
{
	gb->cpu.a = 8 * PAL_COLORS;
	wr8(gb, w2d807, gb->cpu.a);
	uint16_t hl = REG_HL(gb), de = REG_DE(gb);
	do {
		gb->cpu.a = alu_and(gb, rd8(gb, de), 0x1f);
		wr8(gb, hl, gb->cpu.a);
		SET_BC(gb, COLOURFADESTRUCT_LENGTH);
		hl = alu_add16(gb, hl, COLOURFADESTRUCT_LENGTH);
		gb->cpu.a = rd8(gb, de);
		gb->cpu.c = gb->cpu.a;
		de++;
		gb->cpu.a = rd8(gb, de);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = gb->cpu.c;
		gb->cpu.b = alu_srl(gb, gb->cpu.b);
		gb->cpu.a = alu_rra(gb, gb->cpu.a);
		gb->cpu.b = alu_srl(gb, gb->cpu.b);
		gb->cpu.a = alu_rra(gb, gb->cpu.a);
		gb->cpu.a = alu_rra(gb, gb->cpu.a);
		gb->cpu.a = alu_rra(gb, gb->cpu.a);
		gb->cpu.a = alu_rra(gb, gb->cpu.a);
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x1f);
		wr8(gb, hl, gb->cpu.a);
		gb->cpu.a = gb->cpu.b;
		SET_BC(gb, COLOURFADESTRUCT_LENGTH);
		hl = alu_add16(gb, hl, COLOURFADESTRUCT_LENGTH);
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x1f);
		wr8(gb, hl, gb->cpu.a);
		de++;
		SET_BC(gb, COLOURFADESTRUCT_LENGTH);
		hl = alu_add16(gb, hl, COLOURFADESTRUCT_LENGTH);
		gb->cpu.a = alu_dec(gb, rd8(gb, w2d807));
		wr8(gb, w2d807, gb->cpu.a);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	} while (!(gb->cpu.f & FLAG_Z));
	SET_HL(gb, hl);
	SET_DE(gb, de);
}

// hl = palette config: applies its palettes and clears its register
PORT(ApplyPalConfig_Bank21)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.c = rd8(gb, hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, (uint16_t)(0xff00 | gb->cpu.c), gb->cpu.a);
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.l = rd8(gb, hl);
	gb->cpu.h = gb->cpu.a;
	hl = REG_HL(gb);
	do {
		for (int i = 0; i < 8; i++) {
			gb->cpu.a = rd8(gb, hl++);
			wr8(gb, (uint16_t)(0xff00 | gb->cpu.c), gb->cpu.a);
		}
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	SET_HL(gb, hl);
}

PORT(FadeBGPalConfig)
{
	gb->cpu.a = (uint8_t)rBGPI;
	wr8(gb, wPalConfig1Register, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPalConfigToFade, 0);
	gb->cpu.b = 4;
	gb->cpu.a = rd8(gb, wCurEvent);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		gb->cpu.b = 1;
	gb->cpu.a = gb->cpu.b;
	wr8(gb, wPalFadeRepeat, gb->cpu.a);
	SET_HL(gb, wPalConfig1TotalSteps);
	gb_call(gb, ADDR_FadePalConfig);
}

// output: c = TRUE (and carry) while the level name's palette is being set up
PORT(FadeInLevelName)
{
	if (!pal_fade_started(gb, wPalConfig1TotalSteps)) {
		gb->cpu.a = 0x02;
		wr8(gb, w2d0e0, gb->cpu.a);
		copy_bytes(gb, wTempPals1 + 7 * PAL_SIZE, wTempBGPals + 7 * PAL_SIZE, 1 * PAL_SIZE);
		gb->cpu.a = rd8(gb, w2d0e4);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z)) {
			copy_bytes(gb, ADDR_Pals_852dd + 2, wTempPals1 + 7 * PAL_SIZE + 2, 0x6);
		} else {
			uint16_t src = ADDR_Pals_84000 + 7 * PAL_SIZE;
			gb->cpu.a = rd8(gb, w2d011);
			gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
			if (!(gb->cpu.f & FLAG_Z))
				src = ADDR_Pals_840c0 + 7 * PAL_SIZE;
			copy_bytes(gb, src, wTempPals1 + 7 * PAL_SIZE, 1 * PAL_SIZE);
		}
		set_pal_config(gb, 1, wTempBGPals + 7 * PAL_SIZE, OBPI_AUTOINC | 7 * PAL_SIZE, 1);
		inc_total_steps(gb, wPalConfig1TotalSteps);
		gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | FLAG_C); // scf
	} else {
		gb_call(gb, ADDR_FadeBGPalConfig);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.a = 0;
			wr8(gb, wPalConfig1TotalSteps, 0);
			gb->cpu.a = alu_inc(gb, gb->cpu.a);
			wr8(gb, w2d0e0, gb->cpu.a);
		}
	}
	gb->cpu.c = alu_rl(gb, 0x00);
}

PORT(Func_852e5)
{
	gb->cpu.a = alu_xor(gb, rd8(gb, w2d0e0), 0x03);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = rd8(gb, wBottomBarAction);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	if (!pal_fade_started(gb, wPalConfig1TotalSteps)) {
		copy_bytes(gb, wTempPals1 + 7 * PAL_SIZE + 2, wTempBGPals + 7 * PAL_SIZE + 2, 0x6);
		clear_bytes(gb, wTempPals1 + 7 * PAL_SIZE + 2, 0x6);
		inc_total_steps(gb, wPalConfig1TotalSteps);
		set_pal_config(gb, 1, wTempBGPals + 7 * PAL_SIZE, OBPI_AUTOINC | 7 * PAL_SIZE, 1);
		return;
	}
	gb_call(gb, ADDR_FadeBGPalConfig);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPalConfig1TotalSteps, 0);
	wr8(gb, w2d0e0, 0);
}

PORT_JUMP(ApplyPalConfig1)
{
	SET_HL(gb, wPalConfig1);
	JUMP_TO(ADDR_ApplyPalConfig_Bank21); // fallthrough
}

PORT_JUMP(ApplyPalConfig2)
{
	SET_HL(gb, wPalConfig2);
	JUMP_TO(ADDR_ApplyPalConfig_Bank21);
}
