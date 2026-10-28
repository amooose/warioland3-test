// Port of src/home/return_level.asm
#include "ported.h"

PORT(ReturnToPendingLevelState)
{
	SET_HL(gb, wState);
	wr8(gb, wState, ST_LEVEL);
	gb->cpu.a = rd8(gb, wPendingSubState);
	wr8(gb, wSubState, gb->cpu.a);
}

PORT(ReturnToMap)
{
	gb->cpu.a = TRANSITION_RETURN_TO_MAP;
	wr8(gb, wTransitionParam, TRANSITION_RETURN_TO_MAP);
	FARCALL(StartOverworldState);
}

PORT_JUMP(ReturnToMapFromLevel)
{
	SET_HL(gb, wDayNight);
	wr8(gb, wDayNight, (uint8_t)(rd8(gb, wDayNight) | 0x80));
	JUMP(ReturnToMap); // fallthrough
}
