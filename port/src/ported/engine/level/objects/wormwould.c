// Port of src/engine/level/objects/wormwould.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define ROLLING_ROCK_HIT (ADDR_RollingRockFunc + 0x2a) // .Hit
#define PALM_TREE_SPAWN_TREES (ADDR_PalmTreeSpawnerFunc + 0x16) // .SpawnTrees

// Sets update (.MoveLeft/.MoveRight) as the update function once
// wCurObjStateDuration runs out, then moves the rock.
static void rolling_rock(GB *gb, uint16_t update, bool left)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d) {
		hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
		gb->cpu.a = (uint8_t)(update >> 8);
		wr8(gb, hl--, gb->cpu.a);
		gb->cpu.a = (uint8_t)update;
		wr8(gb, hl--, gb->cpu.a);
		SET_HL(gb, hl);
	}
	if (left)
		CALL(MoveObjectLeft_Fast);
	else
		CALL(MoveObjectRight_Fast);
	JUMP_TO(ADDR_RollingRockFunc);
}

PORT_JUMP(RollingRockFunc_Left) { rolling_rock(gb, ADDR_RollingRockFunc_Left + 0x13, true); }
PORT_JUMP(RollingRockFunc_Right) { rolling_rock(gb, ADDR_RollingRockFunc_Right + 0x13, false); }

PORT_JUMP(RollingRockFunc)
{
	gb->cpu.a = rd8(gb, wCurObjVar1);
	SET_BC(gb, alu_and(gb, gb->cpu.a, gb->cpu.a) ? ADDR_Data_60900 : ADDR_Data_608b0);
	gb->cpu.a = rd8(gb, wCurObjState);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP(ApplyObjYMovement);
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
	gb->cpu.l = OBJ_UPDATE_FUNCTION + 1;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl--, (uint8_t)(ROLLING_ROCK_HIT >> 8));
	wr8(gb, hl--, (uint8_t)ROLLING_ROCK_HIT);
	SET_HL(gb, hl);
	play_sfx(gb, SFX_016);
	RET();
}

PORT(PalmTreeSpawnerFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(PALM_TREE_SPAWN_TREES >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)PALM_TREE_SPAWN_TREES;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	SET_HL(gb, hl);
	wr8(gb, wCurObjStateDuration, 240);
	// .Wait
	wr8(gb, wCurObjFrameDuration, 2);
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
}

#define PALM_TREE_UPDATE (ADDR_PalmTreeFunc + 0x16) // .Update

PORT_JUMP(PalmTreeFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(PALM_TREE_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)PALM_TREE_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, ADDR_PalmTreePals);
	SET_DE(gb, wTempPals2 + PAL_SIZE * 7);
	gb->cpu.c = 7;
	gb->cpu.b = 1;
	JUMP(CopyAndApplyOBPals);
}

// Rises while var1 counts down, then (with var2 left) keeps still.
PORT_LOCAL(PalmTreeFunc, Update)
{
	CALL(SetOneWayPlatformAction);
	uint16_t hl = wCurObjVar1;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		wr8(gb, hl, alu_dec(gb, gb->cpu.a));
		CALL(MoveObjectUp);
		gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
		wr8(gb, wCurObjAction, gb->cpu.a);
	} else {
		// .open
		gb->cpu.l = alu_inc(gb, gb->cpu.l);
		hl = REG_HL(gb);
		gb->cpu.a = rd8(gb, hl);
		if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			wr8(gb, hl, alu_dec(gb, gb->cpu.a));
			gb->cpu.a = (1 << NO_ACTIONS_F) | 1;
			wr8(gb, wCurObjAction, gb->cpu.a);
			return;
		}
	}
	// .set_frame_duration
	gb->cpu.a = 10;
	wr8(gb, wCurObjFrameDuration, 10);
}
