// Port of src/engine/overworld/init.asm: Func_803e6 and Func_80bd9, which the
// map's code (bank20.c, bank2d.c) calls with registers (the rest is C++,
// src/native/overworld)
#include "ported.h"

PORT(Func_80bd9)
{
	uint16_t hl = wAttrmap + 14 * TILEMAP_WIDTH;
	wr8(gb, hl, rd8(gb, hl) & ~(1 << B_BG_PRIO));

	// reset BG priority inside a
	// 3x4 rectangle, at coord (0, 15)
	SET_DE(gb, 15 * TILEMAP_WIDTH);
	gb->cpu.c = 4;
	for (;;) {
		hl = alu_add16(gb, wAttrmap, REG_DE(gb));
		gb->cpu.b = 3;
		do {
			wr8(gb, hl, rd8(gb, hl) & ~(1 << B_BG_PRIO));
			hl++;
			gb->cpu.b = alu_dec(gb, gb->cpu.b);
		} while (!(gb->cpu.f & FLAG_Z));
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		if (gb->cpu.f & FLAG_Z)
			break;
		hl = alu_add16(gb, TILEMAP_WIDTH, REG_DE(gb));
		SET_DE(gb, hl);
	}
	SET_HL(gb, hl);
}

PORT_JUMP(Func_803e6)
{
	gb->cpu.a = rd8(gb, wTransitionParam);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wOWBarsState);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	alu_cp(gb, gb->cpu.a, OW_EXITS);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	JUMP(SaveGameOrEnterPerfectScreen);
}
