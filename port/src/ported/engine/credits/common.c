// Port of src/engine/credits/common.asm
#include "ported.h"

PORT(CreditsStateTable)
{
	// push_wram BANK("GFX RAM")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 0x03;
	wr8(gb, rWBK, gb->cpu.a);
	FARCALL(_CreditsStateTable);
	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}
