// Port of src/engine/level/objects/beam_bot.asm
#include "ported.h"

PORT_JUMP(BeamFunc)
{
	SET_HL(gb, wCurObjState);
	gb->cpu.a = rd8(gb, wCurObjState);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjState, 0);
		play_sfx(gb, SFX_07D);
	}
	JUMP(ApplyObjXMovement);
}
