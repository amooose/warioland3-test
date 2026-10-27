// Port of src/engine/level/objects/doll_boy.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define HAMMER_PLATFORM_SPAWN (ADDR_HammerPlatformSpawnerFunc + 0x16) // .SpawnHammerPlatform

// Waits 150 frames, then spawns a hammer platform.
PORT(HammerPlatformSpawnerFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(HAMMER_PLATFORM_SPAWN >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)HAMMER_PLATFORM_SPAWN;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	SET_HL(gb, hl);
	wr8(gb, wCurObjStateDuration, 150);
	// .DoNothing
	wr8(gb, wCurObjFrameDuration, 2);
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
}

PORT(UpdateDollBoyRemainingParts)
{
	gb->cpu.a = alu_dec(gb, rd8(gb, wCurObjSubState));
	if (!gb->cpu.a)
		return; // exit if wCurObjSubState == $1
	gb->cpu.a = alu_and(gb, rd8(gb, wCurObjId), 0x0f);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = rd8(gb, wDollBoyRemainingParts);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.b);
	if (!gb->cpu.a)
		return;
	// unset flag in unk7 from wDollBoyRemainingParts
	gb->cpu.a = alu_xor(gb, gb->cpu.b, 0xff);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.e);
	wr8(gb, wDollBoyRemainingParts, gb->cpu.a);
	gb->cpu.a = 0x08;
	wr8(gb, wCurObjVar1, 0x08);
	SET_HL(gb, wCurObjSubState);
	wr8(gb, wCurObjSubState, alu_dec(gb, rd8(gb, wCurObjSubState)));
}
