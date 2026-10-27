// Port of src/engine/level/objects/fire.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define FIRE_UPDATE (ADDR_FireFunc + 0x24) // .Update

PORT(FireFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(FIRE_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)FIRE_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_69c0a);
	CALL(SetObjectFramesetPtr);
	hl = LO(REG_HL(gb), OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x07);
	wr8(gb, hl--, 0xf8); // OBJ_COLLBOX_LEFT
	gb->cpu.a = 0x00;
	wr8(gb, hl, 0x00); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
	// .Update
}

PORT_LOCAL(FireFunc, Update) {}
