// Port of src/engine/level/objects/scowler.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define FLOATING_RING_SPAWN (ADDR_FloatingRingSpawnerFunc + 0x16) // .Func_54997

// Waits $40 frames, then spawns a floating ring.
PORT(FloatingRingSpawnerFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(FLOATING_RING_SPAWN >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)FLOATING_RING_SPAWN;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	SET_HL(gb, hl);
	wr8(gb, wCurObjStateDuration, 0x40);
	// .Func_549ab
	wr8(gb, wCurObjFrameDuration, 0x02);
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
}
