// Port of src/engine/level/objects/kobatto.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define KOBATTO_UPDATE (ADDR_KobattoFunc + 0x30) // .Update

PORT(KobattoFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(KOBATTO_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)KOBATTO_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_69041);
	CALL(SetObjectFramesetPtr);

	hl = LO(REG_HL(gb), OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));

	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 6);
	wr8(gb, hl--, (uint8_t)-7); // OBJ_COLLBOX_LEFT
	wr8(gb, hl, (uint8_t)-4); // OBJ_COLLBOX_BOTTOM

	hl = LO(hl, OBJ_VAR_1);
	wr8(gb, hl++, 0x01); // var1
	wr8(gb, hl++, (uint8_t)ADDR_Data_60ec9); // var2
	gb->cpu.a = (uint8_t)(ADDR_Data_60ec9 >> 8);
	wr8(gb, hl++, gb->cpu.a); // var3
	SET_HL(gb, hl);
}

PORT_LOCAL(KobattoFunc, Update)
{
	SET_DE(gb, ADDR_Data_60ec9);
	CALL(ApplyObjSetMovement);
}
