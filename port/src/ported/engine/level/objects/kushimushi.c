// Port of src/engine/level/objects/kushimushi.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define KUSHIMUSHI_FUNC_42ACF (ADDR_KushimushiVerticalFunc + 0x10b) // KushimushiVerticalFunc.Func_42acf

PORT(KushimushiHorizontalFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(KUSHIMUSHI_FUNC_42ACF >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)KUSHIMUSHI_FUNC_42ACF;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68611);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x60);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, hl++, 0);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | OBJSUBFLAG_HDIR));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 4);
	wr8(gb, hl--, (uint8_t)-5); // OBJ_COLLBOX_LEFT
	gb->cpu.a = (uint8_t)-4;
	wr8(gb, hl, gb->cpu.a); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
}
