// Port of src/engine/level/objects/small_leaf.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define SMALL_LEAF_UPDATE1 (ADDR_SmallLeafFunc + 0x31) // .Update1

PORT(SmallLeafFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x06);
	wr8(gb, hl--, 0xf9); // OBJ_COLLBOX_LEFT
	wr8(gb, hl, 0xf3); // OBJ_COLLBOX_BOTTOM

	// .Init
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(SMALL_LEAF_UPDATE1 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)SMALL_LEAF_UPDATE1;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68a9f);
	CALL(SetObjectFramesetPtr);
}
