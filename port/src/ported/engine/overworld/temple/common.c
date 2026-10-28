// Port of src/engine/overworld/temple/common.asm
#include "ported.h"

PORT(InitTempleScene)
{
	FARCALL(_InitTempleScene);
}

PORT(UpdateTempleScene)
{
	FARCALL(_UpdateTempleScene);
}

PORT(Func_80e33)
{
	gb->cpu.a = rd8(gb, wJoypadDown);
	if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		gb->cpu.a = SST_OVERWORLD_08;
		wr8(gb, wSubState, gb->cpu.a);
		return;
	}
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}
