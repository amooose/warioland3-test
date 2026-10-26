// Port of src/home/wario_mechanics2.asm
#include "ported.h"

PORT(OpenActionHelp)
{
	gb->cpu.a = ST_ACTION_HELP;
	wr8(gb, wState, ST_ACTION_HELP);
	wr8(gb, wState + 1, 0); // wSubState
	SET_HL(gb, wState + 1);
}

// Caps wWalkVelIndex: at $0c from $10 on the ground or while jumping up,
// at $04 from $08 while falling.
PORT(Func_1762)
{
	gb->cpu.a = rd8(gb, wJumpVelIndex);
	alu_cp(gb, gb->cpu.a, FALLING_JUMP_VEL_INDEX);
	bool falling = !(gb->cpu.f & FLAG_C);
	gb->cpu.a = rd8(gb, wWalkVelIndex);
	alu_cp(gb, gb->cpu.a, falling ? 0x08 : 0x10);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = falling ? 0x04 : 0x0c;
	wr8(gb, wWalkVelIndex, gb->cpu.a);
}
