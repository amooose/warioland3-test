// Port of src/engine/overworld/day_night.asm
#include "ported.h"

// flips bit 4 of wDayNight
PORT(SwitchDayNight)
{
	SET_HL(gb, wDayNight);
	gb->cpu.a = alu_xor(gb, 1 << 4, rd8(gb, wDayNight));
	wr8(gb, wDayNight, gb->cpu.a);
}
