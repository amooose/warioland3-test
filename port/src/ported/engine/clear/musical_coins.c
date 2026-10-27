// Port of src/engine/clear/musical_coins.asm
#include "ported.h"

PORT(Func_d45e1)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTimer + 0, 0);
	gb->cpu.a = 2;
	wr8(gb, wTimer + 1, 2);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, SST_CLEAR_EXIT);
}
