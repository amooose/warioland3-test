// Port of src/home/clear_transformation.asm
#include "ported.h"

// ClearTransformationValues.RestoreBlindPalettes, which waits for H-Blank and
// stays in asm. Local labels aren't exported: it follows the 46 bytes of
// ClearTransformationValues.
#define ADDR_RestoreBlindPalettes (ADDR_ClearTransformationValues + 0x2e)

PORT(PlayRecoverySFX)
{
	play_sfx(gb, SFX_RECOVERY);
}

PORT(ClearTransformationValues)
{
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_BLIND);
	if (gb->cpu.f & FLAG_Z)
		gb_call(gb, ADDR_RestoreBlindPalettes);

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
