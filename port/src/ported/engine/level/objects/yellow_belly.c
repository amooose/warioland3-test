// Port of src/engine/level/objects/yellow_belly.asm
#include "ported.h"

PORT(Func_51c77)
{
	SET_HL(gb, wCurObjStateDuration);
	uint8_t d = alu_dec(gb, rd8(gb, wCurObjStateDuration));
	wr8(gb, wCurObjStateDuration, d);
	if (d)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}
