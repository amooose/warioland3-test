// Port of src/home/clear_transformation.asm
#include "ported.h"

PORT(PlayRecoverySFX)
{
	play_sfx(gb, SFX_RECOVERY);
}

// Copies b palettes from hl to the palette data port c, one palette per
// H-Blank (wait_not_hblank / wait_hblank).
static void copy_pals_hblank(GB *gb, uint16_t hl)
{
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

// .RestoreBlindPalettes: restores the palettes after recovering from the
// Blind transformation
static void restore_blind_palettes(GB *gb)
{
	gb->cpu.a = BGPI_AUTOINC | 0;
	wr8(gb, rBGPI, gb->cpu.a);
	gb->cpu.b = 8;
	gb->cpu.c = (uint8_t)rBGPD;
	copy_pals_hblank(gb, wTempPals1);

	gb->cpu.a = OBPI_AUTOINC | (3 * PAL_SIZE);
	wr8(gb, rOBPI, gb->cpu.a);
	gb->cpu.b = 4;
	gb->cpu.c = (uint8_t)rOBPD;
	copy_pals_hblank(gb, (uint16_t)(wTempPals2 + 3 * PAL_SIZE));
}

PORT_LOCAL(ClearTransformationValues, RestoreBlindPalettes)
{
	restore_blind_palettes(gb);
}

PORT(ClearTransformationValues)
{
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_BLIND);
	if (gb->cpu.f & FLAG_Z)
		restore_blind_palettes(gb);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, wInvincibleCounter, 0);
	wr8(gb, wTransformation, 0);
	wr8(gb, wTransformationProgress, 0);
	wr8(gb, wStingTouchState, 0);
	wr8(gb, wTouchState, 0);
	wr8(gb, wca94, 0);
	wr8(gb, wIsIntangible, 0);
	wr8(gb, wTransformationDuration + 0, 0);
	wr8(gb, wTransformationDuration + 1, 0);
	wr8(gb, wAutoMoveState, 0);
	wr8(gb, wInvisibleFrame, 0);
	wr8(gb, wSwimmingDirectionInput, 0);
}

// unreferenced
PORT(Func_10fc)
{
	gb->cpu.a = gb->cpu.c;
	gb->cpu.b = 0;
	uint16_t hl = 0;
	for (;;) {
		alu_cp(gb, gb->cpu.a, 100);
		if (gb->cpu.f & FLAG_C)
			break;
		hl = alu_add16(gb, hl, 0x9c);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 100);
	}
	gb->cpu.b = 0;
	for (;;) {
		alu_cp(gb, gb->cpu.a, 10);
		if (gb->cpu.f & FLAG_C)
			break;
		hl = alu_add16(gb, hl, 0x6);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 10);
	}
	SET_DE(gb, 0x6);
	SET_HL(gb, alu_add16(gb, hl, REG_BC(gb)));
}

// unreferenced
PORT(Func_111e)
{
	gb_push16(gb, REG_HL(gb));
	gb->cpu.a = gb->cpu.h;
	uint16_t hl = 0;
	for (;;) {
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!gb->cpu.a)
			break;
		hl = alu_add16(gb, hl, (uint16_t)-0x9c);
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
	}
	SET_BC(gb, hl);
	uint16_t entry_hl = gb_pop16(gb);
	gb_push16(gb, entry_hl);
	gb->cpu.a = (uint8_t)entry_hl;
	hl = 0;
	for (;;) {
		alu_cp(gb, gb->cpu.a, 16);
		if (gb->cpu.f & FLAG_C)
			break;
		hl = alu_add16(gb, hl, (uint16_t)-0x6);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 16);
	}
	hl = alu_add16(gb, hl, REG_BC(gb));
	SET_DE(gb, gb_pop16(gb));
	hl = alu_add16(gb, hl, REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.c = gb->cpu.l;
}
