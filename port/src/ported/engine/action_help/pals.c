// Action Help's register-level parts (see src/native/action_help): the
// palette loaders, which copy one palette per HBlank, waiting on rSTAT as
// the original does; the V-Blank function copied to wVBlankFunc; and the
// unreferenced Func_1e4b6d.
#include "ported.h"

// hl = y, x, frame and attributes; y is relative to the background scroll
static uint16_t set_cur_sprite(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.b = rd8(gb, wSCY);
	wr8(gb, wCurSpriteYCoord, alu_add(gb, alu_sub(gb, rd8(gb, hl++), gb->cpu.b), OAM_Y_OFS));
	wr8(gb, wCurSpriteXCoord, alu_add(gb, rd8(gb, hl++), OAM_X_OFS));
	wr8(gb, wCurSpriteFrame, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wCurSpriteAttributes, gb->cpu.a);
	return hl;
}

// Copies b palettes from hl to the palette data register c, one palette
// per HBlank (wait_not_hblank / wait_hblank).
static void copy_pals_hblank(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	do {
		while (!(gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
			idle(gb, 32);
		while ((gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
			idle(gb, 32);
		for (int i = 0; i < PAL_SIZE; i++) {
			gb->cpu.a = rd8(gb, hl++);
			wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), gb->cpu.a);
		}
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	SET_HL(gb, hl);
}

// push hl / ld de, dst / ld b, n / call CopyHLToDE_Short / pop hl
static void copy_pals_keep_hl(GB *gb, uint16_t dst, uint8_t n)
{
	uint16_t hl = REG_HL(gb);
	gb_push16(gb, hl);
	SET_DE(gb, dst);
	gb->cpu.b = n;
	CALL(CopyHLToDE_Short);
	SET_HL(gb, gb_pop16(gb));
}

static void write_a_to_hl(GB *gb, uint16_t hl, uint8_t n)
{
	SET_HL(gb, hl);
	gb->cpu.b = n;
	CALL(WriteAToHL_BTimes);
}

// Returns whether the power-up being demonstrated is a main one; if so,
// leaves the carry of comparing it with Wario's power-up level.
static bool cp_power_up_level(GB *gb)
{
	gb->cpu.a = rd8(gb, wActionHelpPowerUp);
	alu_cp(gb, gb->cpu.a, NUM_MAIN_POWER_UPS);
	if (!(gb->cpu.f & FLAG_C))
		return false;
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), POWER_UP_MASK);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	return true;
}

PORT(LoadActionHelpOBPals)
{
	SET_HL(gb, ADDR_Pals_1e4f2f);
	if (!cp_power_up_level(gb))
		return; // special Power-Up
	if (gb->cpu.f & FLAG_C) {
		// doesn't have Power-Up, so black out some OB pals
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		write_a_to_hl(gb, wTempPals2, 2 * PAL_SIZE);
		write_a_to_hl(gb, wTempPals2 + 3 * PAL_SIZE, 5 * PAL_SIZE);
		SET_HL(gb, wTempPals2);
	} else {
		copy_pals_keep_hl(gb, wTempPals2, 8 * PAL_SIZE);
	}
	gb->cpu.a = OBPI_AUTOINC | 0;
	wr8(gb, rOBPI, gb->cpu.a);
	gb->cpu.b = 8;
	gb->cpu.c = (uint8_t)rOBPD;
	copy_pals_hblank(gb);
}

PORT(Func_1e4a47)
{
	SET_HL(gb, ADDR_Pals_1e4f2f + 4 * PAL_SIZE);
	if (!cp_power_up_level(gb))
		return;
	if (gb->cpu.f & FLAG_C) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		write_a_to_hl(gb, wTempPals2 + 4 * PAL_SIZE, PAL_SIZE);
		write_a_to_hl(gb, wTempPals2 + 7 * PAL_SIZE, PAL_SIZE);
		SET_HL(gb, wTempPals2 + 4 * PAL_SIZE);
	} else {
		uint16_t hl = REG_HL(gb);
		gb_push16(gb, hl);
		SET_DE(gb, wTempPals2 + 4 * PAL_SIZE);
		gb->cpu.b = PAL_SIZE;
		CALL(CopyHLToDE_Short);
		SET_HL(gb, ADDR_Pals_1e4f2f + 7 * PAL_SIZE);
		SET_DE(gb, wTempPals2 + 7 * PAL_SIZE);
		gb->cpu.b = PAL_SIZE;
		CALL(CopyHLToDE_Short);
		SET_HL(gb, gb_pop16(gb));
	}
	gb->cpu.a = OBPI_AUTOINC | (4 * PAL_SIZE);
	wr8(gb, rOBPI, gb->cpu.a);
	gb->cpu.c = (uint8_t)rOBPD;
	gb->cpu.b = 4;
	copy_pals_hblank(gb);
}

// hl = 8 BG palettes: copies them to wTempPals1 and to the BG palette RAM
PORT(LoadActionHelpBGPals)
{
	copy_pals_keep_hl(gb, wTempPals1, 8 * PAL_SIZE);
	gb->cpu.a = BGPI_AUTOINC | 0;
	wr8(gb, rBGPI, gb->cpu.a);
	gb->cpu.b = 8;
	gb->cpu.c = (uint8_t)rBGPD;
	copy_pals_hblank(gb);
}

PORT(LoadActionHelpPalsWithWater)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wActionHelpBlackedOut, 0);
	SET_HL(gb, ADDR_Pals_1e4e6f);
	gb_call(gb, ADDR_LoadActionHelpBGPals);
	gb_call(gb, ADDR_LoadActionHelpOBPals);
}

PORT(LoadActionHelpPalsWithoutWater)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wActionHelpBlackedOut, 0);
	SET_HL(gb, ADDR_Pals_1e4eaf);
	gb_call(gb, ADDR_LoadActionHelpBGPals);
	gb_call(gb, ADDR_LoadActionHelpOBPals);
}

PORT(Func_1e48bc)
{
	gb->cpu.a = TRUE;
	wr8(gb, wActionHelpBlackedOut, TRUE);
	SET_HL(gb, ADDR_Pals_1e4eef);
	gb_call(gb, ADDR_LoadActionHelpBGPals);
}

PORT(Func_1e4b6d)
{
	set_cur_sprite(gb);
	SET_HL(gb, ADDR_OAM_1e6bab);
	CALL(LoadSprite);
}

// VBlank_1fb6b7.Func, as copied to wVBlankFunc (see ram_code.c). Its call
// returns to base + n, the block's start wherever it runs from.
PORT_LOCAL_JUMP(VBlank_1fb6b7, Func)
{
	uint16_t base = gb->cpu.pc;
	gb->cpu.a = rd8(gb, wIsDMATransferPending);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		uint16_t hl = wPendingDMASourceBank;
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, rROMB0 + 0x100, gb->cpu.a);
		gb->cpu.c = (uint8_t)rVDMA_SRC_HIGH;
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, (uint16_t)(0xff00 | gb->cpu.c), gb->cpu.a);
		gb->cpu.c = alu_inc(gb, gb->cpu.c);
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, (uint16_t)(0xff00 | gb->cpu.c), gb->cpu.a);
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, rVBK, gb->cpu.a);
		gb->cpu.c = alu_inc(gb, gb->cpu.c);
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, (uint16_t)(0xff00 | gb->cpu.c), gb->cpu.a);
		gb->cpu.c = alu_inc(gb, gb->cpu.c);
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, (uint16_t)(0xff00 | gb->cpu.c), gb->cpu.a);
		gb->cpu.c = alu_inc(gb, gb->cpu.c);
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, (uint16_t)(0xff00 | gb->cpu.c), gb->cpu.a);
		SET_HL(gb, hl);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wIsDMATransferPending, gb->cpu.a);
	}
	// .not_pending
	gb->cpu.a = rd8(gb, wSCY);
	wr8(gb, rSCY, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	gb_call_ret(gb, hTransferVirtualOAM, (uint16_t)(base + 0x33));
	RET();
}
