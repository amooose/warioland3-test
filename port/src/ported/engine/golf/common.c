// Port of src/engine/golf/common.asm
#include "ported.h"

PORT(GolfStateTable)
{
	// push_wram BANK("Golf RAM")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 0x01;
	wr8(gb, rWBK, gb->cpu.a);
	FARCALL(_GolfStateTable);
	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}
