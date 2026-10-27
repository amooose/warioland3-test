// Port of src/engine/level/objects/chests.asm
#include "ported.h"

// de = frameset to set once wCurObjStateDuration runs out
PORT_JUMP(TreasureFunc)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, gb->cpu.a);
	SET_HL(gb, wCurObjStateDuration);
	gb->cpu.a = rd8(gb, wCurObjStateDuration);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	uint8_t duration = alu_dec(gb, gb->cpu.a);
	wr8(gb, wCurObjStateDuration, duration);
	if (duration) {
		RET();
		return;
	}
	JUMP(SetObjectFramesetPtr);
}

PORT(DeinitChest)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}
