// Port of src/engine/overworld/bank20.asm
#include "ported.h"

PORT(Func_82a8d)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWFuncCounter, 0);
	SET_HL(gb, w2d062);
	wr8(gb, w2d062, alu_inc(gb, rd8(gb, w2d062)));
}

// hl = scene params
// output:
// - z set if scene finished
PORT(GetOWSceneParams_GotPtr)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wOWSceneAction, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (!(gb->cpu.f & FLAG_Z)) { // else finished
		wr8(gb, wOWSceneMapSide, rd8(gb, hl++));
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, wOWSceneActionParam, gb->cpu.a);
		wr8(gb, wOWSceneParamsPtr + 0, (uint8_t)hl);
		gb->cpu.a = (uint8_t)(hl >> 8);
		wr8(gb, wOWSceneParamsPtr + 1, gb->cpu.a);
	}
	SET_HL(gb, hl);
}
