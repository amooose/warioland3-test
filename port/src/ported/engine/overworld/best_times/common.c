// Port of src/engine/overworld/best_times/common.asm
#include "ported.h"

PORT(Func_80de0)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	gb->cpu.a = rd8(gb, wGameModeFlags);
	if (!alu_bit(gb, gb->cpu.a, MODE_TIME_ATTACK_F))
		return;
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_START))
		return;
	gb->cpu.a = SST_OVERWORLD_BEST_TIMES;
	wr8(gb, wSubState, gb->cpu.a);
}

PORT(InitBestTimeList)
{
	FARCALL(_InitBestTimeList);
}

PORT(BestTimeList)
{
	FARCALL(_BestTimeList);
}
