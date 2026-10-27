// Port of src/engine/level/objects/chests.asm
#include "ported.h"

// de = frameset to set once wCurObjStateDuration runs out
PORT_JUMP(TreasureFunc)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, gb->cpu.a);
	SET_HL(gb, wCurObjStateDuration);
	gb->cpu.a = rd8(gb, wCurObjStateDuration);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	uint8_t duration = alu_dec(gb, gb->cpu.a);
	wr8(gb, wCurObjStateDuration, duration);
	if (duration) {
		RET();
		return;
	}
	JUMP(SetObjectFramesetPtr);
}

PORT(DeinitChest)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}

// The object's flags and substate as the init functions set them: visible,
// no collision, update function = update, initialised.
static uint16_t init_obj(GB *gb, uint16_t update)
{
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	hl = (uint16_t)((hl & 0xff00) | (OBJ_UPDATE_FUNCTION + 1));
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	hl = (uint16_t)((hl & 0xff00) | OBJ_SUBSTATE);
	SET_HL(gb, hl);
	return hl;
}

PORT(ChestFunc)
{
	uint16_t hl = init_obj(gb, ADDR_ChestFunc + 0x24); // .Update
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	SET_DE(gb, ADDR_Frameset_68000);
	CALL(SetObjectFramesetPtr);
	gb->cpu.l = OBJ_COLLBOX_RIGHT;
	hl = REG_HL(gb);
	wr8(gb, hl--, 13);
	gb->cpu.a = (uint8_t)-14;
	wr8(gb, hl--, gb->cpu.a); // OBJ_COLLBOX_LEFT
	SET_HL(gb, hl);
}
