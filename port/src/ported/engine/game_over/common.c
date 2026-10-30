// Port of src/engine/game_over/common.asm
#include "ported.h"

PORT(GameOverStateTable)
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
