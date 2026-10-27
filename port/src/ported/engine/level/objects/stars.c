// Port of src/engine/level/objects/stars.asm
#include "ported.h"

PORT(StarsFunc)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	SET_HL(gb, wCurObjStateDuration);
	uint8_t d = alu_dec(gb, rd8(gb, wCurObjStateDuration));
	wr8(gb, wCurObjStateDuration, d);
	if (d)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}

PORT_JUMP(StarsFunc_SetFlags)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(ADDR_StarsFunc >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)ADDR_StarsFunc;
	wr8(gb, hl--, gb->cpu.a);
	hl = (uint16_t)((hl & 0xff00) | OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~(1 << OBJFLAG_GRABBED_F)) | OBJFLAG_NO_COLLISION));
	JUMP_TO(ADDR_StarsFunc); // fallthrough
}

PORT(Func_4c860)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	gb->cpu.l = OBJ_UNK_02;
	FARCALL(DespawnObject);
}
