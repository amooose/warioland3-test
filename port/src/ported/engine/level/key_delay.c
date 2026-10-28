// Port of src/engine/level/key_delay.asm
#include "ported.h"

// pauses game for 100 ticks
PORT(CollectKeyDelay)
{
	SET_HL(gb, wTimer);
	gb->cpu.a = alu_inc(gb, rd8(gb, wTimer));
	wr8(gb, wTimer, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 100);
	if (gb->cpu.f & FLAG_C)
		return;
	wr8(gb, wTimer, 0);
	gb->cpu.a = ST_LEVEL;
	wr8(gb, wState, ST_LEVEL);
}
