// Port of src/engine/level/objects/big_leaf.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

// Stays hidden until wCurObjStateDuration runs out, then shows up as a
// BigLeafFunc.
PORT(BigLeaf1Func)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_INVISIBLE | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	gb->cpu.a = 18;
	wr8(gb, hl, 18);
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, (uint8_t)(ADDR_BigLeafFunc >> 8));
	gb->cpu.a = (uint8_t)ADDR_BigLeafFunc;
	wr8(gb, hl--, gb->cpu.a);
	hl = wCurObjFlags;
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	SET_DE(gb, ADDR_Frameset_68732);
	CALL(SetObjectFramesetPtr);
}

PORT(BigLeafSpawnerFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)((ADDR_BigLeafSpawnerFunc + 0x17) >> 8); // .SpawnSet1
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)(ADDR_BigLeafSpawnerFunc + 0x17);
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	gb->cpu.a = 20;
	wr8(gb, wCurObjStateDuration, 20);
	hl = LO(hl, OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	SET_HL(gb, hl);
}

#define SPAWN_SET1 (ADDR_BigLeafSpawnerFunc + 0x17) // .SpawnSet1
#define SPAWN_SET2 (ADDR_BigLeafSpawnerFunc + 0x5b) // .SpawnSet2
#define LEAF(off) ((uint16_t)(ADDR_BigLeafFunc + (off)))
#define LEAF_UPDATE LEAF(0x22) // .Update
#define LEAF_FUNC_42D0E LEAF(0xc2) // .Func_42d0e

// ld [hld] the update pointer at hl = OBJ_UPDATE_FUNCTION + 1
static void store_update(GB *gb, uint16_t hl, uint16_t update)
{
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

static void no_actions_frame(GB *gb)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	gb->cpu.a = 2;
	wr8(gb, wCurObjFrameDuration, 2);
}

// Once Wario is past it, waits state duration then spawns 2 leaves.
PORT_LOCAL_JUMP(BigLeafSpawnerFunc, SpawnSet1)
{
	no_actions_frame(gb);
	uint16_t hl = wCurObjXPos;
	gb->cpu.a = alu_add(gb, rd8(gb, hl++), 22);
	gb->cpu.e = gb->cpu.a;
	SET_HL(gb, hl);
	bool carry = gb->cpu.f & FLAG_C;
	gb->cpu.a = rd8(gb, hl);
	if (carry)
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.a = rd8(gb, wWarioXPos + 0);
	alu_cp(gb, gb->cpu.a, gb->cpu.d);
	if (!(gb->cpu.f & FLAG_C)) {
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		gb->cpu.a = rd8(gb, wWarioXPos + 1);
		alu_cp(gb, gb->cpu.a, gb->cpu.e);
		if (!(gb->cpu.f & FLAG_C)) {
			RET();
			return;
		}
	}
	hl = LO(hl, OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		RET();
		return;
	}
	store_update(gb, wCurObjUpdateFunction + 1, SPAWN_SET2);
	gb->cpu.a = 160;
	wr8(gb, wCurObjStateDuration, 160);
	SET_BC(gb, ADDR_ObjParams_BigLeaf1);
	CALL(CreateObjectAtRelativePos);
	SET_BC(gb, ADDR_ObjParams_BigLeaf3);
	JUMP(CreateObjectAtRelativePos);
}

// Waits state duration then spawns 1 leaf.
PORT_LOCAL_JUMP(BigLeafSpawnerFunc, SpawnSet2)
{
	no_actions_frame(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	store_update(gb, LO(hl, OBJ_UPDATE_FUNCTION + 1), SPAWN_SET1);
	gb->cpu.a = 80;
	wr8(gb, wCurObjStateDuration, 80);
	SET_BC(gb, ADDR_ObjParams_BigLeaf2);
	JUMP(CreateObjectAtRelativePos);
}

// Waits state duration, then becomes a platform.
PORT(BigLeafFunc)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	store_update(gb, LO(hl, OBJ_UPDATE_FUNCTION + 1), LEAF_UPDATE);
	SET_DE(gb, ADDR_Frameset_68729);
	CALL(SetObjectFramesetPtr);
}

// A one-way platform that drifts down (faster with Wario on it) and sideways
// once hit, and sinks in water.
PORT_LOCAL_JUMP(BigLeaf2Func, Update)
{
	CALL(SetOneWayPlatformAction);
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		alu_cp(gb, gb->cpu.a, 0x0a);
		if (gb->cpu.f & FLAG_Z) {
			JUMP(VanishObject2);
			return;
		}
		alu_cp(gb, gb->cpu.a, 0x0b);
		if (gb->cpu.f & FLAG_C) {
			// .destroy
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wCurObjFlags, 0);
			RET();
			return;
		}
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		gb->cpu.a = (gb->cpu.f & FLAG_C) ? 0x02 : 0x01;
		wr8(gb, wCurObjStateDuration, gb->cpu.a);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
	}
	// .asm_42c9a
	gb->cpu.a = rd8(gb, wCurObjStateDuration);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (gb->cpu.a)
			CALL(MoveObjectLeft);
		else
			CALL(MoveObjectRight);
	}
	// .asm_42cab
	gb->cpu.l = OBJ_FLAGS;
	hl = REG_HL(gb);
	if (alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_STEPPED));
		gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
		if (!(gb->cpu.f & FLAG_C)) {
			RET();
			return;
		}
		gb->cpu.b = 0x01;
		gb_call_ret(gb, ADDR_MoveWarioDown, LEAF(0x71));
		gb_call_ret(gb, ADDR_HandleDownwardsFloorTransition, LEAF(0x74));
		CALL(MoveObjectDown);
	} else {
		gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
		if (!(gb->cpu.f & FLAG_C)) {
			RET();
			return;
		}
		CALL(MoveObjectDown);
	}
	// .asm_42ccd
	hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_352b, LEAF(0x93));
	gb->cpu.a = rd8(gb, wIsInWaterOrSand);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	store_update(gb, wCurObjUpdateFunction + 1, LEAF_FUNC_42D0E);
	SET_DE(gb, ADDR_Frameset_6873b);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 0x14;
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x14);
	hl = LO(hl, OBJ_SCREEN_Y_POS);
	gb->cpu.a = rd8(gb, hl++);
	SET_HL(gb, hl);
	alu_cp(gb, gb->cpu.a, 0xb8);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0xb8);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	play_sfx(gb, SFX_08F);
	RET();
}

PORT_LOCAL(BigLeaf2Func, Func_42d0e)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}

// Waits state duration, then becomes a platform (.Update).
PORT(BigLeaf2Func)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	store_update(gb, LO(hl, OBJ_UPDATE_FUNCTION + 1), ADDR_BigLeaf2Func + 0x22); // .Update
	SET_DE(gb, ADDR_Frameset_68729);
	CALL(SetObjectFramesetPtr);
}
