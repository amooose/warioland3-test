// Port of src/engine/level/objects/small_leaf.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define SMALL_LEAF_UPDATE1 (ADDR_SmallLeafFunc + 0x31) // .Update1

PORT(SmallLeafFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x06);
	wr8(gb, hl--, 0xf9); // OBJ_COLLBOX_LEFT
	wr8(gb, hl, 0xf3); // OBJ_COLLBOX_BOTTOM

	// .Init
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(SMALL_LEAF_UPDATE1 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)SMALL_LEAF_UPDATE1;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68a9f);
	CALL(SetObjectFramesetPtr);
}

#define SMALL_LEAF_UPDATE2 (ADDR_SmallLeafFunc + 0x60) // .Update2
#define SMALL_LEAF_UPDATE3 (ADDR_SmallLeafFunc + 0x90) // .Update3
#define SMALL_LEAF_UPDATE4 (ADDR_SmallLeafFunc + 0x25) // .Update4

// ld l, OBJ_UPDATE_FUNCTION + 1 / ld a, HIGH(f) / ld [hld], a /
// ld a, LOW(f) / ld [hld], a
static void set_update(GB *gb, uint16_t f)
{
	uint16_t hl = LO(REG_HL(gb), OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(f >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)f;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// .Init
static void small_leaf_init(GB *gb)
{
	set_update(gb, SMALL_LEAF_UPDATE1);
	SET_DE(gb, ADDR_Frameset_68a9f);
	CALL(SetObjectFramesetPtr);
}

PORT_LOCAL(SmallLeafFunc, Init)
{
	small_leaf_init(gb);
}

// ld a, no_actions_for 1 / ld [wCurObjAction], a / ld hl,
// wCurObjStateDuration / dec [hl]: true when it reached 0
static bool no_actions_dec_duration(GB *gb)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	SET_HL(gb, wCurObjStateDuration);
	uint8_t v = alu_dec(gb, rd8(gb, wCurObjStateDuration));
	wr8(gb, wCurObjStateDuration, v);
	return v == 0;
}

PORT_LOCAL(SmallLeafFunc, Update4)
{
	if (!no_actions_dec_duration(gb))
		return;
	small_leaf_init(gb);
}

// ld hl, wCurObjState / ld a, [hl] / cp $0a / jr z, .asm_43747 /
// and $fe / cp $04 / jr z, .asm_43747 / xor a / ld [hl], a: false when it
// took the jp Func_3173
static bool check_state(GB *gb)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x0a);
	if (gb->cpu.f & FLAG_Z)
		return false;
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0xfe);
	alu_cp(gb, gb->cpu.a, 0x04);
	if (gb->cpu.f & FLAG_Z)
		return false;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, hl, 0);
	return true;
}

PORT_LOCAL_JUMP(SmallLeafFunc, Update1)
{
	CALL(SetOneWayPlatformAction);
	if (!check_state(gb)) {
		JUMP(Func_3173); // .asm_43747
		return;
	}
	uint16_t hl = LO(REG_HL(gb), OBJ_FLAGS);
	SET_HL(gb, hl);
	if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
		RET();
		return;
	}
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_STEPPED));
	SET_DE(gb, ADDR_Frameset_68aa8);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 0x28;
	wr8(gb, hl++, 0x28);
	SET_HL(gb, hl);
	set_update(gb, SMALL_LEAF_UPDATE2);
	RET();
}

PORT_LOCAL_JUMP(SmallLeafFunc, Update2)
{
	if (!check_state(gb)) {
		JUMP(Func_3173); // .asm_43747
		return;
	}
	uint16_t hl = LO(REG_HL(gb), OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x0f);
	if (gb->cpu.f & FLAG_C) {
		gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
		wr8(gb, wCurObjAction, gb->cpu.a);
	}
	// .asm_43766
	uint8_t v = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, v);
	if (v) {
		RET();
		return;
	}
	gb->cpu.a = 0x9b;
	wr8(gb, hl++, 0x9b);
	hl = LO(hl, OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_STEPPED) | OBJFLAG_INVISIBLE));
	SET_HL(gb, hl);
	set_update(gb, SMALL_LEAF_UPDATE3);
	RET();
}

PORT_LOCAL(SmallLeafFunc, Update3)
{
	if (!no_actions_dec_duration(gb))
		return;
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68abb);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 0x28;
	wr8(gb, hl++, 0x28);
	SET_HL(gb, hl);
	set_update(gb, SMALL_LEAF_UPDATE4);
}
