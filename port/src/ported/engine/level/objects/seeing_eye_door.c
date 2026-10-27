// Port of src/engine/level/objects/seeing_eye_door.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define SEEING_EYE_DOOR_CHECK_DISTANCE (ADDR_SeeingEyeDoorFunc + 0x2b) // .CheckWarioDistance

PORT(SeeingEyeDoorFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, (uint8_t)(SEEING_EYE_DOOR_CHECK_DISTANCE >> 8));
	wr8(gb, hl--, (uint8_t)SEEING_EYE_DOOR_CHECK_DISTANCE);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	SET_HL(gb, hl);
	gb->cpu.a = 24;
	wr8(gb, wCurObjVar2, 24);
	CALL(MoveObjectDownByVar2);
	SET_DE(gb, ADDR_Frameset_68c47);
	CALL(SetObjectFramesetPtr);
	hl = LO(REG_HL(gb), OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x02);
	wr8(gb, hl--, 0xfd); // OBJ_COLLBOX_LEFT
	gb->cpu.a = 0x17;
	wr8(gb, hl, 0x17); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
}
