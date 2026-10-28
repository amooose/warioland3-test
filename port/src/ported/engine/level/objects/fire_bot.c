// Port of src/engine/level/objects/fire_bot.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))
#define DEC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_dec(gb, (uint8_t)h_)); }) // dec l (sets flags)
#define INC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_inc(gb, (uint8_t)h_)); }) // inc l (sets flags)

#define FB(off) (ADDR_FireBotFunc + (off))
#define FIRE_BOT_INIT FB(0xa) // .Init
#define FIRE_BOT_UPDATE FB(0x1e) // .Update
#define FIRE_BOT_FUNC_46C42 FB(0xdc)
#define FIRE_BOT_FUNC_471D6 FB(0x670)
#define FIRE_BOT_FUNC_47207 FB(0x6a1)

// ld l, OBJ_UPDATE_FUNCTION + 1 / ld a, HIGH(f) / ld [hld], a /
// ld a, LOW(f) / ld [hld], a
static inline uint16_t set_update(GB *gb, uint16_t hl, uint16_t f)
{
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(f >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)f;
	wr8(gb, hl--, gb->cpu.a);
	return hl;
}

// ld l, OBJ_INTERACTION_TYPE / ld a, [hl] / and HEAVY_OBJ / or type / ld [hld], a
static inline uint16_t set_interaction(GB *gb, uint16_t hl, uint8_t type)
{
	hl = LO(hl, OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), type);
	wr8(gb, hl--, gb->cpu.a);
	return hl;
}

// ld l, OBJ_COLLBOX_RIGHT / ld a, right / ld [hld], a / ld a, left / ld [hld], a
static inline uint16_t set_collbox_x(GB *gb, uint16_t hl, uint8_t right, uint8_t left)
{
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, right);
	gb->cpu.a = left;
	wr8(gb, hl--, left);
	return hl;
}

// ld hl, wCurObjState / ld a, state / ld [hld], a / ld a, [hld] / rlca /
// ld_obj_frameset left, right / call SetObjectFramesetPtr
static void set_state_frameset(GB *gb, uint8_t state, uint16_t left, uint16_t right)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? right : left);
	CALL(SetObjectFramesetPtr);
}

// The check at the start of several states: when the ground shakes, the
// object gets knocked over (state 4 or 5, at random). Returns true if so.
static bool ground_shake(GB *gb)
{
	gb->cpu.a = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (gb->cpu.f & FLAG_C)
		return false;
	alu_rra(gb, rd8(gb, rDIV));
	gb->cpu.a = alu_rla(gb, 0x04);
	wr8(gb, wCurObjState, gb->cpu.a);
	return true;
}

// ld hl, wCurObjYPos / copies the position to hYPos / hXPos
static void obj_pos_to_h(GB *gb)
{
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

// The position 4 pixels up and dx pixels to the side into hYPos / hXPos
// (dx < 0: sub / sbc, else add / adc).
static void ahead_pos_to_h(GB *gb, int dx)
{
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 4);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	wr8(gb, hYPosHi, gb->cpu.a);
	if (dx < 0) {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), (uint8_t)-dx);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	} else {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), (uint8_t)dx);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
	}
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT(FireBotFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(FIRE_BOT_INIT >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)FIRE_BOT_INIT;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT_LOCAL_JUMP(FireBotFunc, Init)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = set_update(gb, hl, FIRE_BOT_UPDATE);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	SET_HL(gb, hl);
	JUMP_TO(FIRE_BOT_FUNC_46C42);
}

// rst JumpTable on the object's state
PORT_LOCAL_JUMP(FireBotFunc, Update)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	uint16_t table = FIRE_BOT_UPDATE + 4;
	gb_push16(gb, table); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	JUMP_TO(REG_HL(gb));
}

// Stands still, facing Wario.
PORT_LOCAL(FireBotFunc, Func_46c42)
{
	uint16_t hl = set_interaction(gb, wCurObjInteractionType, OBJ_INTERACTION_01);
	hl = set_collbox_x(gb, hl, 0x05, 0xfa);
	hl = LO(hl, OBJ_STATE);
	gb->cpu.a = 0x30;
	wr8(gb, hl--, 0x30);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	bool right = gb->cpu.f & FLAG_C;
	if (right)
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
	else
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR));
	hl = DEC_L(hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	SET_DE(gb, right ? ADDR_Frameset_68946 : ADDR_Frameset_6893d);
	CALL(SetObjectFramesetPtr);
}

PORT_LOCAL(FireBotFunc, Func_46c7e)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = 0x41;
	wr8(gb, hl--, 0x41);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	bool right = gb->cpu.f & FLAG_C;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	hl = LO(hl, right ? OBJ_COLLBOX_RIGHT : OBJ_COLLBOX_LEFT);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, right ? 0x09 : 0xf6);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = right ? 0x05 : 0xfa;
		wr8(gb, hl, gb->cpu.a);
		SET_DE(gb, right ? ADDR_Frameset_689e2 : ADDR_Frameset_689db);
		CALL(SetObjectFramesetPtr);
		hl = REG_HL(gb);
		gb->cpu.a = 14;
		wr8(gb, hl++, 14);
		SET_HL(gb, hl);
		return;
	}
	SET_DE(gb, right ? ADDR_Frameset_68946 : ADDR_Frameset_6893d);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, REG_HL(gb), 0);
}

PORT_LOCAL_JUMP(FireBotFunc, Func_46cc6)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP(ObjState_StandingFall);
		return;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_68946 : ADDR_Frameset_6893d);
	CALL(SetObjectFramesetPtr);
	RET();
}

// .asm_46d1b onwards: starts the fire attack.
static void start_fire(GB *gb, uint16_t hl)
{
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	if (gb->cpu.f & FLAG_C) {
		hl = set_collbox_x(gb, hl, 0x09, 0xfa);
		SET_DE(gb, ADDR_Frameset_689d2);
	} else {
		hl = set_collbox_x(gb, hl, 0x05, 0xf6);
		SET_DE(gb, ADDR_Frameset_689c9);
	}
	SET_HL(gb, hl);
	// .asm_46d1b
	CALL(SetObjectFramesetPtr);
	hl = INC_L(REG_HL(gb));
	gb->cpu.a = 0x02;
	wr8(gb, hl, 0x02);
	hl = set_interaction(gb, hl, OBJ_INTERACTION_FIRE_BOT);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F))
		return;
	play_sfx(gb, SFX_073);
}

PORT_LOCAL(FireBotFunc, Func_46ce4)
{
	if (ground_shake(gb))
		return;
	// .asm_46cf5
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	hl = LO(hl, OBJ_STATE);
	gb->cpu.a = 0x2e;
	wr8(gb, hl--, 0x2e);
	start_fire(gb, hl);
}

// The walk shared by .Func_46d38 and .Func_46e8d (from .asm_46d49 /
// .asm_46eec): checks for the ground below and a wall ahead. Returns 0
// when it moved (or fell); else 1 (blocked on the left) or 2 (on the right).
static int walk(GB *gb, uint16_t ret_3543, uint16_t ret_left, uint16_t ret_3513_l,
	uint16_t ret_right, uint16_t ret_3513_r, uint16_t move_left, uint16_t move_right)
{
	obj_pos_to_h(gb);
	gb_call_ret(gb, ADDR_Func_3543, ret_3543);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = 0x11;
		wr8(gb, wCurObjState, 0x11);
		RET();
		return 0;
	}
	uint16_t hl = wCurObjYPos;
	wr8(gb, hl++, rd8(gb, hYPosLo));
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = alu_sub(gb, alu_and(gb, rd8(gb, hl), 0x0f), 0x06);
		if (gb->cpu.f & FLAG_C) {
			gb_call_ret(gb, ADDR_ProcessBlockOnLeft, ret_left);
			if (!alu_and(gb, gb->cpu.a, 0x0f)) {
				gb->cpu.a = 0;
				return 1;
			}
		}
		ahead_pos_to_h(gb, -6);
		gb_call_ret(gb, ADDR_Func_3513, ret_3513_l);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			JUMP_TO(move_left);
			return 0;
		}
		return 1;
	}
	gb->cpu.a = alu_add(gb, alu_and(gb, rd8(gb, hl), 0x0f), 0x05);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (!(gb->cpu.f & FLAG_C)) {
		gb_call_ret(gb, ADDR_ProcessBlockOnRight, ret_right);
		if (!alu_and(gb, gb->cpu.a, 0x0f)) {
			gb->cpu.a = 0;
			return 2;
		}
	}
	ahead_pos_to_h(gb, 5);
	gb_call_ret(gb, ADDR_Func_3513, ret_3513_r);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(move_right);
		return 0;
	}
	return 2;
}

PORT_LOCAL_JUMP(FireBotFunc, Func_46d38)
{
	if (ground_shake(gb)) {
		RET();
		return;
	}
	// .asm_46d49
	int side = walk(gb, FB(0x1f5), FB(0x217), FB(0x235), FB(0x255),
		FB(0x273), ADDR_MoveObjectLeft, ADDR_MoveObjectRight);
	if (!side)
		return;
	// .asm_46d9f / .asm_46ddd
	bool right = side == 2;
	gb->cpu.a = rd8(gb, wCurObjVar1);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		SET_DE(gb, right ? ADDR_Frameset_689be : ADDR_Frameset_689b3);
		// .asm_46de6
		CALL(SetObjectFramesetPtr);
		uint16_t hl = REG_HL(gb);
		gb->cpu.a = 36;
		wr8(gb, hl++, 36);
		SET_HL(gb, hl);
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		gb->cpu.a = 0x5a;
		wr8(gb, wCurObjState, 0x5a);
		RET();
		return;
	}
	SET_DE(gb, right ? ADDR_Frameset_68976 : ADDR_Frameset_68969);
	// .asm_46df6
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 61;
	wr8(gb, hl++, 61);
	SET_HL(gb, hl);
	gb->cpu.a = 0x5b;
	wr8(gb, wCurObjState, 0x5b);
	RET();
}

PORT_LOCAL(FireBotFunc, Func_46e02)
{
	if (ground_shake(gb))
		return;
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	gb->cpu.a = 0x13;
	wr8(gb, wCurObjState, 0x13);
}

PORT_LOCAL_JUMP(FireBotFunc, Func_46e1e)
{
	if (ground_shake(gb)) {
		RET();
		return;
	}
	// .asm_46e2f
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x0f);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_46e52
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		hl = LO(hl, OBJ_SUBSTATE);
		gb->cpu.a = alu_rlca(gb, rd8(gb, hl));
		if (gb->cpu.f & FLAG_C) {
			wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR));
			hl = set_collbox_x(gb, hl, 0x05, 0xf6);
		} else {
			wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
			hl = set_collbox_x(gb, hl, 0x09, 0xfa);
		}
		SET_HL(gb, hl);
		RET();
		return;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP(Func_3308);
		return;
	}
	hl = LO(hl, OBJ_STATE);
	gb->cpu.a = 0x2e;
	wr8(gb, hl--, 0x2e);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_689d2 : ADDR_Frameset_689c9);
	CALL(SetObjectFramesetPtr);
	RET();
}

PORT_LOCAL_JUMP(FireBotFunc, Func_46e6f)
{
	uint16_t hl = set_interaction(gb, wCurObjInteractionType, OBJ_INTERACTION_01);
	hl = LO(hl, OBJ_STATE);
	gb->cpu.a = 0x31;
	wr8(gb, hl--, 0x31);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_68946 : ADDR_Frameset_6893d);
	JUMP(SetObjectFramesetPtr);
}

// Walks; when Wario is close in front of it, starts the fire attack.
PORT_LOCAL_JUMP(FireBotFunc, Func_46e8d)
{
	if (ground_shake(gb)) {
		RET();
		return;
	}
	// .asm_46e9e
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_HOT_WARIO);
	bool close = false;
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.c = 0x2a;
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenYPos), gb->cpu.c);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.c);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		alu_cp(gb, gb->cpu.a, 0x18);
		close = gb->cpu.f & FLAG_C;
		if (!close) {
			alu_cp(gb, gb->cpu.a, 0xe8);
			close = !(gb->cpu.f & FLAG_C);
		}
	}
	if (close) {
		// .asm_46eb9
		uint16_t hl = wCurObjSubState;
		SET_HL(gb, hl);
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), gb->cpu.c);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), gb->cpu.c);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		alu_cp(gb, gb->cpu.a, 0xc0);
		int attack = -1; // -1: not close, 0: turn, 1: attack
		uint16_t fs = 0;
		if (!(gb->cpu.f & FLAG_C)) {
			// .asm_46edb
			gb->cpu.a = alu_rlca(gb, rd8(gb, hl++));
			attack = (gb->cpu.f & FLAG_C) ? 1 : 0;
			fs = ADDR_Frameset_6895c;
		} else {
			alu_cp(gb, gb->cpu.a, 0x40);
			if (gb->cpu.f & FLAG_C) {
				gb->cpu.a = alu_rlca(gb, rd8(gb, hl++));
				attack = (gb->cpu.f & FLAG_C) ? 0 : 1;
				fs = ADDR_Frameset_6894f;
			}
		}
		if (attack == 0) {
			// .asm_46ed7
			gb->cpu.a = 0x13;
			wr8(gb, hl, 0x13);
			SET_HL(gb, hl);
			RET();
			return;
		}
		if (attack == 1) {
			SET_DE(gb, fs);
			// .asm_46ee2
			gb->cpu.a = 0x2d;
			wr8(gb, hl, 0x2d);
			SET_HL(gb, hl);
			CALL(SetObjectFramesetPtr);
			hl = REG_HL(gb);
			gb->cpu.a = 23;
			wr8(gb, hl++, 23);
			SET_HL(gb, hl);
			RET();
			return;
		}
	}
	// .asm_46eec
	if (!walk(gb, FB(0x398), FB(0x3ba), FB(0x3d8), FB(0x3ee),
		FB(0x40c), ADDR_MoveObjectLeft_Slow, ADDR_MoveObjectRight_Slow))
		return;
	// .asm_46f42 / .asm_46f76
	gb->cpu.a = 0x13;
	wr8(gb, wCurObjState, 0x13);
	RET();
}

// .Func_46f7c / .Func_46f84
static void func_46f8a(GB *gb, uint8_t state)
{
	set_state_frameset(gb, state, ADDR_Frameset_68983, ADDR_Frameset_68986);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 12;
	wr8(gb, hl--, 12);
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjVar2, 0x02);
	hl = set_collbox_x(gb, hl, 0x05, 0xfa);
	SET_HL(gb, set_interaction(gb, hl, OBJ_INTERACTION_01));
}

PORT_LOCAL(FireBotFunc, Func_46f7c) { func_46f8a(gb, 0x32); }
PORT_LOCAL(FireBotFunc, Func_46f84) { func_46f8a(gb, 0x33); }

// .Func_46fd7 / .Func_4700a
static void func_46fd7(GB *gb, uint16_t func)
{
	uint16_t hl = wCurObjVar2;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(func);
		return;
	}
	gb->cpu.a = 0x0f;
	wr8(gb, wCurObjState, 0x0f);
	RET();
}

// .Func_46fb2 / .Func_46fe5
static void func_46fb2(GB *gb, uint8_t state)
{
	set_state_frameset(gb, state, ADDR_Frameset_68989, ADDR_Frameset_68992);
	uint16_t hl = set_collbox_x(gb, REG_HL(gb), 0x05, 0xfa);
	SET_HL(gb, set_interaction(gb, hl, OBJ_INTERACTION_00));
}

PORT_LOCAL_JUMP(FireBotFunc, Func_46fb2)
{
	func_46fb2(gb, 0x46);
	func_46fd7(gb, ADDR_Func_32ae);
}
PORT_LOCAL_JUMP(FireBotFunc, Func_46fd7) { func_46fd7(gb, ADDR_Func_32ae); }
PORT_LOCAL_JUMP(FireBotFunc, Func_46fe5)
{
	func_46fb2(gb, 0x47);
	func_46fd7(gb, ADDR_Func_32bd);
}
PORT_LOCAL_JUMP(FireBotFunc, Func_4700a) { func_46fd7(gb, ADDR_Func_32bd); }

// .Func_47018 / .Func_47020
static void func_47026(GB *gb, uint8_t state)
{
	set_state_frameset(gb, state, ADDR_Frameset_68989, ADDR_Frameset_68992);
	uint16_t hl = INC_L(INC_L(REG_HL(gb)));
	wr8(gb, hl++, 0x02);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	hl = set_collbox_x(gb, hl, 0x05, 0xfa);
	SET_HL(gb, set_interaction(gb, hl, OBJ_INTERACTION_00));
}

PORT_LOCAL(FireBotFunc, Func_47018) { func_47026(gb, 0x44); }
PORT_LOCAL(FireBotFunc, Func_47020) { func_47026(gb, 0x45); }

// .Func_4705c
static void func_4705c(GB *gb)
{
	gb->cpu.a = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = 0x1c;
		wr8(gb, wCurObjState, 0x1c);
		RET();
		return;
	}
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP(Func_32ea);
		return;
	}
	gb->cpu.a = 0x10;
	wr8(gb, wCurObjState, 0x10);
	RET();
}

PORT_LOCAL_JUMP(FireBotFunc, Func_4704d)
{
	gb->cpu.a = 0x3f;
	wr8(gb, wCurObjState, 0x3f);
	gb->cpu.a = 0x64;
	wr8(gb, wCurObjStateDuration, 0x64);
	SET_HL(gb, wCurObjFlags);
	wr8(gb, wCurObjFlags, (uint8_t)(rd8(gb, wCurObjFlags) & ~OBJFLAG_GRABBED));
	func_4705c(gb);
}
PORT_LOCAL_JUMP(FireBotFunc, Func_4705c) { func_4705c(gb); }

// .Func_47084
static void func_47084(GB *gb)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP(Func_32f9);
		return;
	}
	gb->cpu.a = 0x00;
	wr8(gb, wCurObjState, 0x00);
	RET();
}

PORT_LOCAL_JUMP(FireBotFunc, Func_47076)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = 0x40;
	wr8(gb, hl--, 0x40);
	hl = DEC_L(hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	gb->cpu.a = 0x07;
	wr8(gb, wCurObjStateDuration, 0x07);
	func_47084(gb);
}
PORT_LOCAL_JUMP(FireBotFunc, Func_47084) { func_47084(gb); }

PORT_LOCAL(FireBotFunc, Func_47091)
{
	set_state_frameset(gb, 0x43, ADDR_Frameset_689a5, ADDR_Frameset_689ac);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 26;
	wr8(gb, hl++, 26);
	hl = set_collbox_x(gb, hl, 0x05, 0xfa);
	SET_HL(gb, set_interaction(gb, hl, OBJ_INTERACTION_01));
}

PORT_LOCAL_JUMP(FireBotFunc, Func_470ba)
{
	if (ground_shake(gb)) {
		RET();
		return;
	}
	// .asm_470cb
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x09);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_470dd
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		hl = LO(hl, OBJ_SUBSTATE);
		SET_HL(gb, hl);
		gb->cpu.a = alu_rlca(gb, rd8(gb, hl));
		if (gb->cpu.f & FLAG_C)
			wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR));
		else
			wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
		RET();
		return;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP(Func_3308);
		return;
	}
	gb->cpu.a = 0x01;
	wr8(gb, wCurObjState, 0x01);
	RET();
}

// .Func_470ea / .Func_470f4
static void func_470fc(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjAction;
	gb->cpu.a = 0x8f;
	wr8(gb, hl--, 0x8f);
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_68992 : ADDR_Frameset_68989);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 4);
	hl = INC_L(hl);
	wr8(gb, hl++, 0x02);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	hl = set_collbox_x(gb, hl, 0x05, 0xfa);
	SET_HL(gb, set_interaction(gb, hl, OBJ_INTERACTION_00));
}

PORT_LOCAL(FireBotFunc, Func_470ea) { func_470fc(gb, 0x38); }
PORT_LOCAL(FireBotFunc, Func_470f4) { func_470fc(gb, 0x39); }

// .Func_47126 / .Func_4712d
static void func_47132(GB *gb, uint8_t state)
{
	gb->cpu.a = state;
	wr8(gb, wCurObjState, state);
	uint16_t hl = wCurObjFrameDuration;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, hl++, 0);
	SET_HL(gb, hl);
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjVar2, 0x02);
}

PORT_LOCAL(FireBotFunc, Func_47126) { func_47132(gb, 0x3b); }
PORT_LOCAL(FireBotFunc, Func_4712d) { func_47132(gb, 0x3c); }

// .Func_4713e / .Func_47150
static void func_4713e(GB *gb, uint16_t func, uint16_t frameset)
{
	uint16_t hl = wCurObjFrameDuration;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, hl++, 0);
	hl = LO(hl, OBJ_VAR_2);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(func);
		return;
	}
	SET_DE(gb, frameset);
	// .asm_47160
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 61;
	wr8(gb, hl++, 61);
	SET_HL(gb, hl);
	gb->cpu.a = 0x5b;
	wr8(gb, wCurObjState, 0x5b);
	RET();
}

PORT_LOCAL_JUMP(FireBotFunc, Func_4713e) { func_4713e(gb, ADDR_Func_32bd, ADDR_Frameset_68969); }
PORT_LOCAL_JUMP(FireBotFunc, Func_47150) { func_4713e(gb, ADDR_Func_32ae, ADDR_Frameset_68976); }

// .Func_4716c / .Func_47176 / .Func_47180 / .Func_4718a
static void func_47192(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	gb->cpu.b = 0x02;
	// .asm_47192
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), 0xf0);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = alu_rlca(gb, gb->cpu.c);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_689a0 : ADDR_Frameset_6899b);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(FireBotFunc, Func_4716c) { func_47192(gb, 0x59); }
PORT_LOCAL_JUMP(FireBotFunc, Func_47176) { func_47192(gb, 0x58); }
PORT_LOCAL_JUMP(FireBotFunc, Func_47180) { func_47192(gb, 0x34); }
PORT_LOCAL_JUMP(FireBotFunc, Func_4718a) { func_47192(gb, 0x35); }

// .Func_471b0 / .Func_471c3
static void func_471b0(GB *gb, uint8_t state, uint8_t v)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	hl = DEC_L(hl);
	wr8(gb, hl--, 0x41);
	wr8(gb, hl--, v);
	gb->cpu.a = 0x01;
	wr8(gb, hl--, 0x01);
	SET_HL(gb, hl);
}

PORT_LOCAL_JUMP(FireBotFunc, Func_471b0)
{
	func_471b0(gb, 0x36, 0xc0);
	JUMP(Func_3335);
}
PORT_LOCAL_JUMP(FireBotFunc, Func_471c3)
{
	func_471b0(gb, 0x37, 0xe0);
	JUMP(Func_3344);
}

// .Func_471d6 / .Func_47207 up to .Func_471f7 / .Func_47228
static void grabbed(GB *gb, uint8_t state)
{
	set_state_frameset(gb, state, ADDR_Frameset_68989, ADDR_Frameset_68992);
	uint16_t hl = set_collbox_x(gb, REG_HL(gb), 0x05, 0xfa);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_GRABBED));
}

// .Func_471f7 / .Func_47228: follows Wario's direction while carried.
static void carried(GB *gb, bool left)
{
	gb->cpu.a = rd8(gb, wDirection);
	bool nz = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	if (left ? nz : !nz) {
		if (left)
			JUMP(Func_3353);
		else
			JUMP(Func_3362);
		return;
	}
	uint16_t hl = wCurObjSubState;
	SET_HL(gb, hl);
	gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
	wr8(gb, hl, gb->cpu.a);
	JUMP_TO(left ? FIRE_BOT_FUNC_47207 : FIRE_BOT_FUNC_471D6);
}

PORT_LOCAL_JUMP(FireBotFunc, Func_471d6)
{
	grabbed(gb, 0x49);
	carried(gb, true);
}
PORT_LOCAL_JUMP(FireBotFunc, Func_471f7) { carried(gb, true); }
PORT_LOCAL_JUMP(FireBotFunc, Func_47207)
{
	grabbed(gb, 0x48);
	carried(gb, false);
}
PORT_LOCAL_JUMP(FireBotFunc, Func_47228) { carried(gb, false); }

PORT_LOCAL(FireBotFunc, Func_47239)
{
	uint16_t hl = wCurObjAction;
	wr8(gb, hl--, 0x9f);
	wr8(gb, hl--, 0x42);
	hl = DEC_L(hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, set_interaction(gb, hl, OBJ_INTERACTION_00));
}
