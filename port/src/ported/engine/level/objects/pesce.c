// Port of src/engine/level/objects/pesce.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define STRONG_WATER_CURRENT_UPDATE (ADDR_StrongWaterCurrentFunc + 0x34) // .Func_51d2d

PORT_JUMP(StrongWaterCurrentFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, (uint8_t)(STRONG_WATER_CURRENT_UPDATE >> 8));
	wr8(gb, hl--, (uint8_t)STRONG_WATER_CURRENT_UPDATE);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x0f);
	wr8(gb, hl--, 0xf0); // OBJ_COLLBOX_LEFT
	wr8(gb, hl, 0xfe); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_69965);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 0x2a;
	wr8(gb, wCurObjVar2, 0x2a);
	CALL(MoveObjectDownByVar2);
	gb->cpu.a = 0x69;
	wr8(gb, wCurObjVar2, 0x69);
	JUMP(MoveObjectLeftByVar2);
}
