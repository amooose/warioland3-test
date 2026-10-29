// Port of src/engine/level/objects/unused_flower.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define FLOWER_PROJECTILE_DESTROY (ADDR_UnusedFlowerProjectileCommonFunc + 0x85) // .Destroy

PORT_JUMP(UnusedFlowerProjectileRightFunc)
{
	CALL(MoveObjectRight);
	JUMP_TO(ADDR_UnusedFlowerProjectileCommonFunc);
}

PORT_JUMP(UnusedFlowerProjectileLeftFunc)
{
	CALL(MoveObjectLeft);
	JUMP_TO(ADDR_UnusedFlowerProjectileCommonFunc); // fallthrough
}

// hPos = the object's position, with y - y_off
static void set_hpos(GB *gb, uint16_t hl, uint8_t y_off)
{
	if (y_off) {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), y_off);
		wr8(gb, hYPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
		wr8(gb, hYPosHi, gb->cpu.a);
	} else {
		wr8(gb, hYPosLo, rd8(gb, hl++));
		wr8(gb, hYPosHi, rd8(gb, hl++));
	}
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT(UnusedFlowerProjectileCommonFunc)
{
	const uint16_t routine = ADDR_UnusedFlowerProjectileCommonFunc;
	gb->cpu.a = rd8(gb, wCurObjState);
	bool thrown = !alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!thrown) {
		alu_cp(gb, gb->cpu.a, OBJSTATE_SPECIAL_1);
		if (gb->cpu.f & FLAG_Z) {
			// .Roll
			set_hpos(gb, LO(REG_HL(gb), OBJ_Y_POS), 4);
			SET_HL(gb, REG_HL(gb) + 1); // ld a, [hli]
			gb_call_ret(gb, ADDR_Func_3513, routine + 0x36);
			if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
				set_hpos(gb, wCurObjYPos, 0);
				gb_call_ret(gb, ADDR_Func_3543, routine + 0x4b);
				if (alu_and(gb, gb->cpu.a, gb->cpu.a))
					return;
				gb->cpu.a = OBJSTATE_SPECIAL_2;
				wr8(gb, wCurObjState, OBJSTATE_SPECIAL_2);
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z; // xor a
				wr8(gb, wCurObjVar3, 0);
				return;
			}
			goto set_destroy;
		}
		alu_cp(gb, gb->cpu.a, OBJSTATE_SPECIAL_2);
		if (!(gb->cpu.f & FLAG_Z))
			goto set_destroy;
	}
	// .Jump / .Thrown
	SET_BC(gb, thrown ? ADDR_Data_60320 : ADDR_Data_60320 + 8);
	CALL(ApplyObjYMovement);
	set_hpos(gb, wCurObjYPos, 0);
	gb_call_ret(gb, ADDR_Func_352b, routine + 0x74);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	wr8(gb, wCurObjYPos + 0, rd8(gb, hYPosLo));
	wr8(gb, wCurObjYPos + 1, rd8(gb, hYPosHi));
	SET_HL(gb, wCurObjYPos + 1);
	gb->cpu.a = OBJSTATE_SPECIAL_1;
	wr8(gb, wCurObjState, OBJSTATE_SPECIAL_1);
	return;

set_destroy:;
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(FLOWER_PROJECTILE_DESTROY >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)FLOWER_PROJECTILE_DESTROY;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68389);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 8;
	hl = REG_HL(gb);
	wr8(gb, hl++, 8); // OBJ_STATE_DURATION
	SET_HL(gb, hl);
}

#define FLOWER(off) (ADDR_UnusedFlowerFunc + (off))
#define FLOWER_GROW FLOWER(0x21)
#define FLOWER_IDLE FLOWER(0x4b)
#define FLOWER_PREPARING_ATTACK FLOWER(0x7a)
#define FLOWER_ATTACK FLOWER(0xaf)

// ld hl, wCurObjUpdateFunction + 1 / ld a, HIGH(f) / ld [hld], a /
// ld a, LOW(f) / ld [hld], a
static void flower_set_update(GB *gb, uint16_t f)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(f >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)f;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// ld de, frameset / call SetObjectFramesetPtr / ld a, dur / ld [hli], a
static void flower_frameset(GB *gb, uint16_t frameset, uint8_t dur)
{
	SET_DE(gb, frameset);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = dur;
	wr8(gb, hl++, dur);
	SET_HL(gb, hl);
}

static void flower_no_actions(GB *gb)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
}

PORT(UnusedFlowerFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	hl = (uint16_t)((hl & 0xff00) | OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = (uint16_t)((hl & 0xff00) | (OBJ_UPDATE_FUNCTION + 1));
	gb->cpu.a = (uint8_t)(FLOWER_GROW >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)FLOWER_GROW;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	flower_frameset(gb, ADDR_Frameset_6837a, 30);
	gb->cpu.a = rd8(gb, wGlobalCounter);
	wr8(gb, REG_HL(gb), gb->cpu.a); // OBJ_VAR_1
}

PORT_LOCAL(UnusedFlowerFunc, Grow)
{
	flower_no_actions(gb);
	gb->cpu.a = 2;
	wr8(gb, wCurObjFrameDuration, 2);
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
	alu_cp(gb, gb->cpu.a, OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	hl = (uint16_t)((hl & 0xff00) | OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	flower_set_update(gb, FLOWER_IDLE);
	flower_frameset(gb, ADDR_Frameset_6837a, 49);
}

// waits for the state duration, then until the global counter is back at
// var1; returns whether it's time
static bool flower_wait(GB *gb)
{
	flower_no_actions(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint8_t d = alu_dec(gb, gb->cpu.a);
		wr8(gb, hl, d);
		if (d)
			return false;
	}
	gb->cpu.a = 2;
	wr8(gb, wCurObjFrameDuration, 2);
	gb->cpu.e = rd8(gb, wGlobalCounter);
	gb->cpu.a = rd8(gb, wCurObjVar1);
	alu_cp(gb, gb->cpu.a, gb->cpu.e);
	return gb->cpu.f & FLAG_Z;
}

PORT_LOCAL(UnusedFlowerFunc, Idle)
{
	if (!flower_wait(gb))
		return;
	flower_set_update(gb, FLOWER_PREPARING_ATTACK);
	flower_frameset(gb, ADDR_Frameset_683ac, 68);
}

PORT_LOCAL_JUMP(UnusedFlowerFunc, PreparingAttack)
{
	flower_no_actions(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	flower_set_update(gb, FLOWER_ATTACK);
	flower_frameset(gb, ADDR_Frameset_683b1, 35);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	SET_BC(gb, (gb->cpu.f & FLAG_C) ? ADDR_ObjParams_UnusedFlowerProjectileRight
		: ADDR_ObjParams_UnusedFlowerProjectileLeft);
	JUMP(CreateObjectAtRelativePos);
}

PORT_LOCAL(UnusedFlowerFunc, Attack)
{
	if (!flower_wait(gb))
		return;
	flower_set_update(gb, ADDR_UnusedFlowerFunc); // .Init
}

PORT_LOCAL(UnusedFlowerProjectileCommonFunc, Destroy)
{
	flower_no_actions(gb);
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
