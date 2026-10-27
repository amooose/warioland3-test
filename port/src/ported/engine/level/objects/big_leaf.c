// Port of src/engine/level/objects/big_leaf.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

// Stays hidden until wCurObjStateDuration runs out, then shows up as a
// BigLeafFunc.
PORT(BigLeaf1Func)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_INVISIBLE | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	gb->cpu.a = 18;
	wr8(gb, hl, 18);
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, (uint8_t)(ADDR_BigLeafFunc >> 8));
	gb->cpu.a = (uint8_t)ADDR_BigLeafFunc;
	wr8(gb, hl--, gb->cpu.a);
	hl = wCurObjFlags;
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	SET_DE(gb, ADDR_Frameset_68732);
	CALL(SetObjectFramesetPtr);
}

PORT(BigLeafSpawnerFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)((ADDR_BigLeafSpawnerFunc + 0x17) >> 8); // .SpawnSet1
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)(ADDR_BigLeafSpawnerFunc + 0x17);
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	gb->cpu.a = 20;
	wr8(gb, wCurObjStateDuration, 20);
	hl = LO(hl, OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	SET_HL(gb, hl);
}
