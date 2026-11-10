// The epilogue's state table (src/engine/overworld/epilogue/state_table.asm):
// a fade to white every fourth frame, then the title's Func_4628.
#include "../../routines.hpp"

// MainStateTable calls it; the main loop sets every register it uses after.
PORT_EX(EpilogueStateTable, 0, ALL_REGISTERS)
{
	wl3::Memory mem(gb);
	switch (mem.sub_state()) {
	case 0: // .FadeToWhite
		if (!(mem.global_counter() & 0b11))
			wl3::home::fade_bg_to_white(gb, FADE_SPEED_SLOW); // FastFadeToWhite
		return;
	case 1: // .Func_473c
		wl3::Routines(gb).farcall(BANK_Func_4628, ADDR_Func_4628);
		return;
	default: // past the table: the asm jumps through whatever follows
		if (gb->test.active)
			gb->test.unrealistic = true;
		return;
	}
}

