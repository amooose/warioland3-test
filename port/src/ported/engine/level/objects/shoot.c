// Port of src/engine/level/objects/shoot.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

// Sets up a goal counter, with update as its update function.
static void goal_counter(GB *gb, uint16_t update)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	hl = LO(hl, OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	SET_HL(gb, hl);
	wr8(gb, wCurObjFrame, 0x3c);
	// move up $58 pixels
	gb->cpu.a = 0x58;
	wr8(gb, wCurObjVar2, 0x58);
	JUMP(MoveObjectUpByVar2);
}

PORT_JUMP(ShootGoalCounterFunc) { goal_counter(gb, ADDR_ShootGoalCounterFunc + 0x20); }
PORT_JUMP(WarioGoalCounterFunc) { goal_counter(gb, ADDR_WarioGoalCounterFunc + 0x20); }
