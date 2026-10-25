// Port of src/engine/overworld/bank2b_2.asm
#include "ported.h"

// xor a / ld [w2d891], a / inc [w2d890]
static void advance_w2d890(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d891, 0);
	SET_HL(gb, w2d891 - 1);
	wr8(gb, w2d891 - 1, alu_inc(gb, rd8(gb, w2d891 - 1))); // w2d890
}

// c = number of frames to wait
PORT(Func_ae1c9)
{
	gb->cpu.a = rd8(gb, w2d891);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (gb->cpu.f & FLAG_C)
		return;
	advance_w2d890(gb);
}

PORT(Func_ae1ce)
{
	advance_w2d890(gb);
}
