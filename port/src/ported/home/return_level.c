// Port of src/home/return_level.asm
#include "ported.h"

PORT(ReturnToPendingLevelState)
{
	SET_HL(gb, wState);
	wr8(gb, wState, ST_LEVEL);
	gb->cpu.a = rd8(gb, wPendingSubState);
	wr8(gb, wSubState, gb->cpu.a);
}
