// Port of src/engine/golf/building/building.asm
#include "ported.h"

// loads attribute map to wGolfCourseTileMap
// and palettes to w1db00 depending on wGolfCourse
PORT_JUMP(Func_1c87db)
{
	gb->cpu.a = BANK_GolfBuildingCourse1Tilemap; // BANK("Golf Gfx 2")
	wr8(gb, wTempBank, gb->cpu.a);
	gb->cpu.a = rd8(gb, wGolfCourse);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.b = 0;
	gb->cpu.c = gb->cpu.a;
	uint16_t hl = alu_add16(gb, ADDR_Func_1c87db + 0x2d, REG_BC(gb)); // .data
	gb->cpu.e = gb->cpu.a = rd8(gb, hl++);
	gb->cpu.d = gb->cpu.a = rd8(gb, hl++);
	gb_push16(gb, hl); // push hl
	SET_HL(gb, REG_DE(gb));
	SET_DE(gb, wGolfCourseTileMap);
	SET_BC(gb, 7 * TILEMAP_WIDTH);
	CALL(FarCopyHLToDE_BC2);
	hl = gb_pop16(gb); // pop hl
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.h = rd8(gb, hl);
	gb->cpu.l = gb->cpu.a;
	SET_DE(gb, w1db00);
	gb->cpu.b = 8 * 8;
	JUMP(CopyHLToDE);
}

// gets the golf course scroll X value and writes it as the target SCX
PORT(GetGolfCourseTargetSCX)
{
	gb->cpu.a = rd8(gb, wGolfCourse);
	gb->cpu.b = 0;
	gb->cpu.c = gb->cpu.a;
	uint16_t hl = alu_add16(gb, ADDR_GetGolfCourseTargetSCX + 0x0f, REG_BC(gb)); // .scroll_x
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wGolfMenuTargetSCX, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT(Func_1c88dc)
{
	gb->cpu.a = rd8(gb, wGolfMenuTargetSCX);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, wGolfMenuScrollingDir);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	bool left = gb->cpu.a != 0;
	gb->cpu.a = rd8(gb, wSCX);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0x02;
		wr8(gb, w1dc00, 0x02);
		wr8(gb, wGolfVBlankMode, 0x02);
		return;
	}
	if (left)
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 4);
	else
		gb->cpu.a = alu_add(gb, gb->cpu.a, 4);
	wr8(gb, wSCX, gb->cpu.a);
}

PORT(Func_1c8907)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w1dc00, 0);
	gb->cpu.a = 0x03;
	wr8(gb, wGolfVBlankMode, 0x03);
}

PORT(Func_1c8911)
{
	gb->cpu.a = rd8(gb, wGolfCounter);
	alu_cp(gb, gb->cpu.a, 0x32);
	if (gb->cpu.f & FLAG_C) {
		wr8(gb, wGolfCounter, alu_inc(gb, rd8(gb, wGolfCounter)));
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x03);
		if (gb->cpu.f & FLAG_Z)
			play_sfx(gb, SFX_12B);
		gb->cpu.a = rd8(gb, wGolfNumCoins + 1);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 1);
		gb->cpu.a = alu_daa(gb, gb->cpu.a);
		wr8(gb, wGolfNumCoins + 1, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, wGolfNumCoins), 0);
		wr8(gb, wGolfNumCoins, gb->cpu.a);
		SET_HL(gb, wGolfNumCoins);
		return;
	}
	play_sfx(gb, SFX_SELECTION);
	// .SetGolfHole
	gb->cpu.a = rd8(gb, wGolfMenuOption);
	gb->cpu.b = 0;
	gb->cpu.c = gb->cpu.a;
	uint16_t hl = alu_add16(gb, ADDR_Func_1c8911 + 0x42, REG_BC(gb)); // .first_golf_holes
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wPredeterminedGolfHole, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = 0x04;
	wr8(gb, wSubState, 0x04);
}
