// Port of src/engine/level/objects/prince_froggy.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define PRINCE_FROGGY_UPDATE_STUCK (ADDR_PrinceFroggyFunc + 0x30) // .Update_Stuck
#define PRINCE_FROGGY_UPDATE_SITTING (ADDR_PrinceFroggyFunc + 0xe1) // .Update_Sitting
#define PRINCE_FROGGY_FUNC_441DC (ADDR_PrinceFroggyFunc + 0x118) // .Func_441dc

// Sets the update function at OBJ_UPDATE_FUNCTION of hl's object.
static uint16_t set_update(GB *gb, uint16_t hl, uint16_t update)
{
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	return hl;
}

// Initialised, with its collision box at hl's object.
static uint16_t init_collbox(GB *gb, uint16_t hl)
{
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x03);
	wr8(gb, hl--, 0xec);
	gb->cpu.a = 0x00;
	wr8(gb, hl--, 0x00);
	return hl;
}

PORT_JUMP(GreyPrinceFroggyFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = set_update(gb, hl, PRINCE_FROGGY_UPDATE_SITTING);
	hl = init_collbox(gb, hl);
	SET_HL(gb, LO(hl, OBJ_STATE));
	JUMP_TO(PRINCE_FROGGY_FUNC_441DC);
}

PORT(RedPrinceFroggyFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	SET_HL(gb, set_update(gb, hl, PRINCE_FROGGY_UPDATE_STUCK));
	SET_DE(gb, ADDR_Frameset_68ac6);
	CALL(SetObjectFramesetPtr);
	SET_HL(gb, init_collbox(gb, REG_HL(gb)));
}
