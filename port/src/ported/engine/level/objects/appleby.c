// Port of src/engine/level/objects/appleby.asm
#include "ported.h"

#define APPLE_UPDATE1_LEFT (ADDR_AppleFunc + 0x12) // .Update1_Left
#define APPLE_UPDATE1_RIGHT (ADDR_AppleFunc + 0x34) // .Update1_Right

PORT_JUMP(AppleFunc_Left)
{
	gb->cpu.a = (uint8_t)(APPLE_UPDATE1_LEFT >> 8);
	gb->cpu.b = (uint8_t)APPLE_UPDATE1_LEFT;
	JUMP_TO(ADDR_AppleFunc);
}

PORT_JUMP(AppleFunc_Right)
{
	gb->cpu.a = (uint8_t)(APPLE_UPDATE1_RIGHT >> 8);
	gb->cpu.b = (uint8_t)APPLE_UPDATE1_RIGHT;
	JUMP_TO(ADDR_AppleFunc); // fallthrough
}

// ab = update function
PORT(AppleFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.b);
	hl = (uint16_t)((hl & 0xff00) | OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	play_sfx(gb, SFX_067);
}

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))
#define ABY(off) ((uint16_t)(ADDR_ApplebyFunc + (off)))
#define APPLEBY_UPDATE ABY(0x49) // .Update
#define APPLE(off) ((uint16_t)(ADDR_AppleFunc + (off)))
#define APPLE_UPDATE2_LEFT APPLE(0x56) // .Update2_Left
#define APPLE_UPDATE2_RIGHT APPLE(0x6d) // .Update2_Right
#define APPLE_FUNC_47BD3 APPLE(0xba) // .Func_47bd3
#define APPLE_FUNC_47BD8 APPLE(0xbf) // .Func_47bd8

static uint16_t lo_hl(GB *gb, uint8_t l)
{
	gb->cpu.l = l;
	return REG_HL(gb);
}

static void set_bits(GB *gb, uint16_t hl, uint8_t set, uint8_t res)
{
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~res) | set));
}

static void store_update(GB *gb, uint16_t hl, uint16_t update)
{
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

static void set_frameset(GB *gb, uint16_t frameset)
{
	SET_DE(gb, frameset);
	CALL(SetObjectFramesetPtr);
}

// ld [hli], a with the hl SetObjectFramesetPtr left
static void ld_hli(GB *gb, uint8_t v)
{
	gb->cpu.a = v;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, v);
	SET_HL(gb, hl);
}

static void no_actions(GB *gb)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
}

// dec [hl] at OBJ_STATE_DURATION: true if it reached 0
static bool dec_duration(GB *gb)
{
	uint16_t hl = lo_hl(gb, OBJ_STATE_DURATION);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	return !d;
}

static void set_state(GB *gb, uint8_t state)
{
	gb->cpu.a = state;
	wr8(gb, wCurObjState, state);
}

PORT(ApplebyFunc)
{
	uint16_t hl = wCurObjFlags;
	set_bits(gb, hl, OBJFLAG_NO_COLLISION, OBJFLAG_INVISIBLE);
	store_update(gb, LO(hl, OBJ_UPDATE_FUNCTION + 1), APPLEBY_UPDATE);
	set_frameset(gb, ADDR_Frameset_689e9);
	ld_hli(gb, 5);
	hl = lo_hl(gb, OBJ_SUBSTATE);
	set_bits(gb, hl, 0, OBJSUBFLAG_UNINITIALISED);
	hl = lo_hl(gb, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 7);
	wr8(gb, hl--, (uint8_t)-8);
	gb->cpu.a = (uint8_t)-1;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// Waits on screen, then pops up.
static void func_479e0(GB *gb)
{
	no_actions(gb);
	uint16_t hl = lo_hl(gb, OBJ_FLAGS);
	if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F))
		return;
	if (!dec_duration(gb))
		return;
	set_frameset(gb, ADDR_Frameset_68a00);
	ld_hli(gb, 29);
	set_state(gb, 0x5a);
	set_bits(gb, lo_hl(gb, OBJ_FLAGS), 0, OBJFLAG_NO_COLLISION);
}

PORT_LOCAL(ApplebyFunc, Func_479e0) { func_479e0(gb); }

// .asm_47ac4: hides back
static void hide(GB *gb)
{
	gb->cpu.a = 0x2d;
	wr8(gb, REG_HL(gb), 0x2d);
	set_frameset(gb, ADDR_Frameset_68a09);
	ld_hli(gb, 29);
	set_bits(gb, lo_hl(gb, OBJ_FLAGS), OBJFLAG_NO_COLLISION, 0);
}

// a = state, hl = wCurObjState: hit by Wario
static void func_47ab0(GB *gb)
{
	uint8_t a = gb->cpu.a;
	uint16_t func;
	alu_cp(gb, a, 0x0a);
	if (gb->cpu.f & FLAG_Z) {
		JUMP(VanishObject);
		return;
	}
	alu_cp(gb, a, 0x04);
	if (gb->cpu.f & FLAG_Z) {
		func = ADDR_Func_3326;
	} else {
		alu_cp(gb, a, 0x05);
		if (gb->cpu.f & FLAG_Z) {
			func = ADDR_Func_3317;
		} else {
			alu_cp(gb, a, 0x28);
			if (gb->cpu.f & FLAG_Z) {
				func = ADDR_Func_33f8;
			} else {
				alu_cp(gb, a, 0x29);
				if (gb->cpu.f & FLAG_Z) {
					func = ADDR_Func_3407;
				} else {
					hide(gb);
					RET();
					return;
				}
			}
		}
	}
	// .asm_47af6
	gb->cpu.a = (uint8_t)(func >> 8);
	gb->cpu.c = (uint8_t)func;
	gb->cpu.b = 0x02;
	uint16_t hl = lo_hl(gb, OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.c);
	SET_HL(gb, hl);
	set_frameset(gb, ADDR_Frameset_68a63);
	hl = lo_hl(gb, OBJ_SUBSTATE);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), OBJSUBFLAGS_MASK);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	wr8(gb, hl, gb->cpu.b);
	SET_HL(gb, hl);
	no_actions(gb);
	RET();
}

PORT_LOCAL_JUMP(ApplebyFunc, Func_47ab0) { func_47ab0(gb); }

PORT_LOCAL_JUMP(ApplebyFunc, Update)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	uint8_t a = gb->cpu.a;
	if (!alu_and(gb, a, a)) {
		func_479e0(gb);
		RET();
		return;
	}
	alu_cp(gb, a, 0x2a);
	if (gb->cpu.f & FLAG_C) {
		func_47ab0(gb);
		return;
	}
	alu_cp(gb, a, 0x5a);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_47a1e
		no_actions(gb);
		if (dec_duration(gb)) {
			set_frameset(gb, ADDR_Frameset_68a12);
			set_state(gb, 91);
		}
		RET();
		return;
	}
	alu_cp(gb, a, 0x5b);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_47a6b
		hl = lo_hl(gb, OBJ_STATE_DURATION);
		gb->cpu.a = rd8(gb, hl);
		if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
			RET();
			return;
		}
		// .asm_47a73: throws an apple when Wario is in range
		hl = lo_hl(gb, OBJ_SUBSTATE);
		gb->cpu.c = 0x2a;
		gb->cpu.b = alu_add(gb, rd8(gb, wCurObjScreenYPos), 0x2a);
		gb->cpu.a = alu_add(gb, rd8(gb, wWarioScreenYPos), 0x2a);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		alu_cp(gb, gb->cpu.a, 0x04);
		if (gb->cpu.f & FLAG_C) {
			RET();
			return;
		}
		alu_cp(gb, gb->cpu.a, 0x46);
		if (!(gb->cpu.f & FLAG_C)) {
			RET();
			return;
		}
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		alu_cp(gb, gb->cpu.a, 0x40);
		uint16_t frameset;
		if (gb->cpu.f & FLAG_C) {
			set_bits(gb, hl, 0, OBJSUBFLAG_HDIR);
			frameset = ADDR_Frameset_68a1f;
		} else {
			alu_cp(gb, gb->cpu.a, 0xc0);
			if (gb->cpu.f & FLAG_C) {
				RET();
				return;
			}
			set_bits(gb, hl, OBJSUBFLAG_HDIR, 0);
			frameset = ADDR_Frameset_68a3a;
		}
		set_frameset(gb, frameset);
		ld_hli(gb, 122);
		set_state(gb, 0x5c);
		RET();
		return;
	}
	alu_cp(gb, a, 0x5c);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_47a4a
		hl = lo_hl(gb, OBJ_STATE_DURATION);
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, 0x46);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
			SET_BC(gb, (gb->cpu.f & FLAG_C) ? ADDR_ObjParams_AppleRight : ADDR_ObjParams_AppleLeft);
			CALL(CreateObjectAtRelativePos);
			hl = wCurObjStateDuration;
			SET_HL(gb, hl);
		}
		// .asm_47a65
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (!d) {
			lo_hl(gb, OBJ_STATE);
			hide(gb);
		}
		RET();
		return;
	}
	alu_cp(gb, a, 0x2d);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_47a33
		no_actions(gb);
		if (dec_duration(gb)) {
			set_frameset(gb, ADDR_Frameset_689e9);
			ld_hli(gb, 60);
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wCurObjState, 0);
		}
	}
	RET();
}

static void update1(GB *gb, uint16_t update2, bool left)
{
	store_update(gb, wCurObjUpdateFunction + 1, update2);
	uint8_t wario = rd8(gb, wWarioScreenXPos), obj = rd8(gb, wCurObjScreenXPos);
	gb->cpu.b = alu_add(gb, left ? wario : obj, 0x2a);
	gb->cpu.a = alu_add(gb, left ? obj : wario, 0x2a);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C)
		return;
	alu_cp(gb, gb->cpu.a, 0x10);
	if (gb->cpu.f & FLAG_C)
		return;
	uint16_t hl = lo_hl(gb, OBJ_VAR_1);
	gb->cpu.a = 0x01;
	wr8(gb, hl++, 0x01);
	gb->cpu.a = 0x02;
	wr8(gb, hl, 0x02);
	SET_HL(gb, hl);
}

PORT_LOCAL(AppleFunc, Update1_Left) { update1(gb, APPLE_UPDATE2_LEFT, true); }
PORT_LOCAL(AppleFunc, Update1_Right) { update1(gb, APPLE_UPDATE2_RIGHT, false); }

// .asm_47b9c: falls, then rolls off once landed
static void apple_fall(GB *gb)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (gb->cpu.f & FLAG_Z) {
		// .destroy
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjFlags, 0);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x19);
	if (!(gb->cpu.f & FLAG_Z)) {
		SET_BC(gb, ADDR_Data_60670);
		JUMP(ApplyObjYMovement);
		return;
	}
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	SET_BC(gb, (gb->cpu.f & FLAG_C) ? APPLE_FUNC_47BD3 : APPLE_FUNC_47BD8);
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.c);
	SET_HL(gb, hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
	RET();
}

// var1 = 0: moves every other frame
PORT_LOCAL_JUMP(AppleFunc, Update2_Left)
{
	uint16_t hl = wCurObjVar1;
	gb->cpu.a = rd8(gb, hl++);
	SET_HL(gb, hl);
	bool move = true;
	if (gb->cpu.f & FLAG_Z) { // no and a: the flags from the caller
		gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
		move = !(gb->cpu.f & FLAG_C);
	}
	if (move) {
		gb->cpu.b = rd8(gb, hl);
		hl = lo_hl(gb, OBJ_X_POS);
		gb->cpu.a = alu_sub(gb, rd8(gb, hl), gb->cpu.b);
		wr8(gb, hl++, gb->cpu.a);
		SET_HL(gb, hl);
		if (gb->cpu.f & FLAG_C)
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	}
	apple_fall(gb);
}

PORT_LOCAL_JUMP(AppleFunc, Update2_Right)
{
	uint16_t hl = wCurObjVar1;
	gb->cpu.a = rd8(gb, hl++);
	SET_HL(gb, hl);
	bool move = true;
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
		move = gb->cpu.f & FLAG_C;
	}
	if (move) {
		gb->cpu.b = rd8(gb, hl);
		hl = lo_hl(gb, OBJ_X_POS);
		gb->cpu.a = alu_add(gb, rd8(gb, hl), gb->cpu.b);
		wr8(gb, hl++, gb->cpu.a);
		SET_HL(gb, hl);
		if (gb->cpu.f & FLAG_C)
			wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	}
	apple_fall(gb);
}

static void roll(GB *gb)
{
	no_actions(gb);
	SET_BC(gb, ADDR_Data_60040);
	JUMP(ApplyObjYMovement);
}

PORT_LOCAL_JUMP(AppleFunc, Func_47bd3)
{
	CALL(MoveObjectLeft);
	roll(gb);
}

PORT_LOCAL_JUMP(AppleFunc, Func_47bd8)
{
	CALL(MoveObjectRight);
	roll(gb);
}
