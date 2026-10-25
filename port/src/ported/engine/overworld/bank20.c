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
