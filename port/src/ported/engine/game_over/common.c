// Port of src/engine/game_over/common.asm
#include "ported.h"

// Its state routines leave bc, de and hl as they end, and MainStateTable
// returns to the main loop, where none is live (see src/native/title).
PORT_EX(GameOverStateTable, 0, R_B | R_C | R_D | R_E | R_H | R_L)
{
	// push_wram BANK("GFX RAM")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 0x03;
	wr8(gb, rWBK, gb->cpu.a);
	FARCALL(_GameOverStateTable);
	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}
