// Port of src/engine/perfect/common.asm
#include "ported.h"

// MainStateTable calls it and returns to the main loop, which reloads
// every register (see src/native/screens).
PORT_EX(PerfectStateTable, 0, R_B | R_C | R_D | R_E | R_H | R_L)
{
	// push_wram BANK("GFX RAM")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 0x03;
	wr8(gb, rWBK, gb->cpu.a);
	FARCALL(_PerfectStateTable);
	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}
