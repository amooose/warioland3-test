// Port of src/engine/level/objects/wall_crack.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define WALL_CRACK_UPDATE (ADDR_WallCrackFunc + 0x16) // .Update

PORT_JUMP(WallCrackClosedFunc)
{
	SET_DE(gb, ADDR_Frameset_6a052);
	JUMP_TO(ADDR_WallCrackFunc);
}

PORT_JUMP(WallCrackOpenFunc)
{
	SET_DE(gb, ADDR_Frameset_6a05f);
	JUMP_TO(ADDR_WallCrackFunc); // fallthrough
}

// de = frameset
PORT(WallCrackFunc)
{
	CALL(SetObjectFramesetPtr);
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, (uint8_t)(WALL_CRACK_UPDATE >> 8));
	wr8(gb, hl--, (uint8_t)WALL_CRACK_UPDATE);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	SET_HL(gb, hl);
	// .Update
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
}
