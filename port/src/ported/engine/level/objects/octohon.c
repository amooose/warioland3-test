// Port of src/engine/level/objects/octohon.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))
#define DEC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_dec(gb, (uint8_t)h_)); }) // dec l (sets flags)

#define SMALL(off) (ADDR_SmallOctohonFunc + (off))
#define SMALL_FUNC_4A4D4 SMALL(0x1f)
#define SMALL_FUNC_4A50F SMALL(0x5a)
#define BIG(off) (ADDR_BigOctohonFunc + (off))
#define BIG_FUNC_4A6B9 BIG(0x1f)
#define BIG_FUNC_4A729 BIG(0x8f)

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

// The entry of both: visible, no collision, update f, collision box.
static void octohon_init(GB *gb, uint16_t f, uint8_t right, uint8_t left, uint8_t bottom)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	hl = set_update(gb, hl, f);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, right);
	wr8(gb, hl--, left);
	gb->cpu.a = bottom;
	wr8(gb, hl, bottom);
	SET_HL(gb, hl);
}

PORT(SmallOctohonFunc) { octohon_init(gb, SMALL_FUNC_4A4D4, 0x00, 0x00, 0x00); }
PORT(BigOctohonFunc) { octohon_init(gb, BIG_FUNC_4A6B9, 0x08, 0xf7, 0x08); }

// The bobbing: moves the octohon horizontally along Data_4a5a9 (the
// .asm_4a57d / .asm_4a875 code), towards the left if left.
static void bob(GB *gb, bool left_flag_from_var1)
{
	uint16_t de = ADDR_Data_4a5a9;
	SET_DE(gb, de);
	uint16_t hl = wCurObjVar2;
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.b = gb->cpu.d;
	gb->cpu.c = gb->cpu.e;
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	gb->cpu.c = gb->cpu.a;
	if (gb->cpu.f & FLAG_C)
		gb->cpu.b = alu_inc(gb, gb->cpu.b);
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
		gb->cpu.b = gb->cpu.d;
		gb->cpu.c = gb->cpu.e;
		gb->cpu.a = rd8(gb, REG_BC(gb));
	}
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	gb->cpu.b = gb->cpu.a;
	bool add;
	if (left_flag_from_var1) {
		hl = DEC_L(hl);
		gb->cpu.a = rd8(gb, hl);
		add = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	} else {
		gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
		add = gb->cpu.f & FLAG_C;
	}
	hl = LO(hl, OBJ_X_POS);
	gb->cpu.a = rd8(gb, hl);
	if (add) {
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.b);
		wr8(gb, hl++, gb->cpu.a);
		SET_HL(gb, hl);
		if (gb->cpu.f & FLAG_C)
			wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	} else {
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		wr8(gb, hl++, gb->cpu.a);
		SET_HL(gb, hl);
		if (gb->cpu.f & FLAG_C)
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	}
}

// .asm_4a4f7: hl = wCurObjStateDuration, b = facing, de = frameset
static void small_turn(GB *gb)
{
	uint16_t hl = wCurObjStateDuration;
	wr8(gb, hl++, 0xac);
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
	gb->cpu.a = alu_rra(gb, rd8(gb, rDIV));
	if (gb->cpu.f & FLAG_C) {
		hl = LO(hl, OBJ_FLAGS);
		SET_HL(gb, hl);
		gb->cpu.a = alu_xor(gb, rd8(gb, hl), 0x80);
		wr8(gb, hl, gb->cpu.a);
	}
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(SmallOctohonFunc, Func_4a4d4)
{
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C) {
		SET_DE(gb, ADDR_Frameset_69b38);
		gb->cpu.b = 0x01;
	} else {
		SET_DE(gb, ADDR_Frameset_69b2f);
		gb->cpu.b = 0x00;
	}
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(SMALL_FUNC_4A50F >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)SMALL_FUNC_4A50F;
	wr8(gb, hl--, gb->cpu.a);
	small_turn(gb);
}

// Swims back and forth.
PORT_LOCAL_JUMP(SmallOctohonFunc, Func_4a50f)
{
	gb->cpu.a = 0x81;
	wr8(gb, wCurObjAction, 0x81);
	uint16_t hl = wCurObjVar1;
	gb->cpu.a = rd8(gb, hl);
	uint8_t v = gb->cpu.a;
	bool right = false;
	if (alu_and(gb, v, v)) {
		alu_cp(gb, v, 0x01);
		if (gb->cpu.f & FLAG_Z) {
			right = true;
		} else {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, hl, 0);
		}
	}
	// .asm_4a521 / .asm_4a53f
	hl = DEC_L(hl);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (!d) {
			SET_DE(gb, right ? ADDR_Frameset_69b4c : ADDR_Frameset_69b41);
			CALL(SetObjectFramesetPtr);
		}
	} else {
		hl = LO(hl, OBJ_VAR_2);
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, 0xee);
		if (gb->cpu.f & FLAG_Z) {
			SET_DE(gb, right ? ADDR_Frameset_69b2f : ADDR_Frameset_69b38);
			gb->cpu.b = right ? 0x00 : 0x01;
			small_turn(gb);
			return;
		}
	}
	// .asm_4a55d
	gb->cpu.a = rd8(gb, wCurObjVar2);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		hl = wCurObjYPos;
		wr8(gb, hYPosLo, rd8(gb, hl++));
		wr8(gb, hYPosHi, rd8(gb, hl++));
		wr8(gb, hXPosLo, rd8(gb, hl++));
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, hXPosHi, gb->cpu.a);
		SET_HL(gb, hl);
		gb_call_ret(gb, ADDR_Func_3513, SMALL(0xc0));
		if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			hl = wCurObjVar2;
			SET_HL(gb, hl);
			wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
			RET();
			return;
		}
	}
	bob(gb, true);
	RET();
}

// .asm_4a6df: hl = wCurObjStateDuration, de = frameset
static void big_reset(GB *gb)
{
	uint16_t hl = wCurObjStateDuration;
	wr8(gb, hl++, 0xac);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(BigOctohonFunc, Func_4a6b9)
{
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	uint16_t hl = wCurObjSubState;
	if (gb->cpu.f & FLAG_C) {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
		SET_DE(gb, ADDR_Frameset_69b60);
	} else {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR));
		SET_DE(gb, ADDR_Frameset_69b57);
	}
	hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(BIG_FUNC_4A729 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)BIG_FUNC_4A729;
	wr8(gb, hl--, gb->cpu.a);
	big_reset(gb);
}

// .asm_4a6eb / .asm_4a6f0 (hl from the caller): squirts ink.
static void squirt(GB *gb, bool right)
{
	SET_DE(gb, right ? ADDR_Frameset_69b74 : ADDR_Frameset_69b69);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, REG_HL(gb), 0);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, 0x43);
	wr8(gb, hl++, 0x04);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	hl = LO(hl, OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_06);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	play_sfx(gb, SFX_08D);
	RET();
}

// .asm_4a860 (hl in the object): turns around.
static void big_turn(GB *gb, uint16_t hl)
{
	hl = LO(hl, OBJ_SUBSTATE);
	SET_HL(gb, hl);
	gb->cpu.a = alu_xor(gb, rd8(gb, hl), 0x80);
	wr8(gb, hl, gb->cpu.a);
	gb->cpu.a = alu_rlca(gb, gb->cpu.a);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_69b60 : ADDR_Frameset_69b57);
	big_reset(gb);
}

// Func_3513 at the position 6 pixels ahead; returns a.
static uint8_t check_ahead(GB *gb, uint16_t ret_left, uint16_t ret_right, bool *right)
{
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	*right = gb->cpu.f & FLAG_C;
	if (*right) {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), 0x06);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
	} else {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x06);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	}
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3513, *right ? ret_right : ret_left);
	return alu_and(gb, gb->cpu.a, gb->cpu.a);
}

// .asm_4a823: bobs along unless blocked.
static void big_bob(GB *gb)
{
	bool right;
	if (check_ahead(gb, BIG(0x1a5), BIG(0x1b7), &right)) {
		// .asm_4a854
		uint16_t hl = wCurObjVar2;
		SET_HL(gb, hl);
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
		RET();
		return;
	}
	bob(gb, false);
	RET();
}

PORT_LOCAL_JUMP(BigOctohonFunc, Func_4a729)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	uint8_t s = gb->cpu.a;
	if (alu_and(gb, s, s)) {
		alu_cp(gb, s, 0x0b);
		if (gb->cpu.f & FLAG_Z) {
			squirt(gb, false);
			return;
		}
		alu_cp(gb, s, 0x0c);
		if (gb->cpu.f & FLAG_Z) {
			squirt(gb, true);
			return;
		}
		alu_cp(gb, s, 0x0a);
		if (gb->cpu.f & FLAG_Z) {
			JUMP(Func_3173);
			return;
		}
		// .asm_4a711
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl--, 0);
		gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
		SET_HL(gb, hl);
		SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_69b88 : ADDR_Frameset_69b7f);
		CALL(SetObjectFramesetPtr);
		hl = REG_HL(gb);
		wr8(gb, hl++, 0x3c);
		gb->cpu.a = 0x03;
		wr8(gb, hl, 0x03);
		SET_HL(gb, hl);
		RET();
		return;
	}
	// .asm_4a73f
	hl = LO(hl, OBJ_VAR_1);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	uint8_t v = gb->cpu.a;
	if (!alu_and(gb, v, v)) {
		// .asm_4a7c2
		hl = DEC_L(hl);
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			// .asm_4a859
			hl = LO(hl, OBJ_VAR_2);
			SET_HL(gb, hl);
			gb->cpu.a = rd8(gb, hl);
			alu_cp(gb, gb->cpu.a, 0xee);
			if (gb->cpu.f & FLAG_Z)
				big_turn(gb, hl);
			else
				big_bob(gb);
			return;
		}
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (!d) {
			// .asm_4a812
			gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
			SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_69b74 : ADDR_Frameset_69b69);
			CALL(SetObjectFramesetPtr);
			big_bob(gb);
			return;
		}
		gb->cpu.c = 0x2a;
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenYPos), gb->cpu.c);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.c);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		alu_cp(gb, gb->cpu.a, 0x14);
		bool close = gb->cpu.f & FLAG_C;
		if (!close) {
			alu_cp(gb, gb->cpu.a, 0xec);
			close = !(gb->cpu.f & FLAG_C);
		}
		if (close) {
			// .asm_4a7df
			gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), gb->cpu.c);
			gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), gb->cpu.c);
			gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
			hl = wCurObjSubState;
			SET_HL(gb, hl);
			bool right = alu_bit(gb, rd8(gb, hl), OBJSUBFLAG_HDIR_F);
			alu_cp(gb, gb->cpu.a, right ? 0xc0 : 0x40);
			bool c = gb->cpu.f & FLAG_C;
			if (right ? !c : c) {
				SET_DE(gb, right ? ADDR_Frameset_69bce : ADDR_Frameset_69bc3);
				// .asm_4a800
				CALL(SetObjectFramesetPtr);
				hl = REG_HL(gb);
				wr8(gb, hl++, 0x24);
				gb->cpu.a = 0x01;
				wr8(gb, hl, 0x01);
				hl = LO(hl, OBJ_INTERACTION_TYPE);
				gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_49);
				wr8(gb, hl--, gb->cpu.a);
				SET_HL(gb, hl);
				RET();
				return;
			}
		}
		big_bob(gb);
		return;
	}
	alu_cp(gb, v, 0x01);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_4a77f
		hl = DEC_L(hl);
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
			RET();
			return;
		}
		gb->cpu.a = 0x02;
		wr8(gb, wCurObjFrameDuration, 0x02);
	} else {
		bool v4 = false;
		alu_cp(gb, v, 0x02);
		bool v2 = gb->cpu.f & FLAG_Z;
		if (!v2) {
			alu_cp(gb, v, 0x03);
			if (gb->cpu.f & FLAG_Z) {
				// .asm_4a756
				hl = DEC_L(hl);
				SET_HL(gb, hl);
				uint8_t d = alu_dec(gb, rd8(gb, hl));
				wr8(gb, hl, d);
				if (!d) {
					gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
					SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_69b60 : ADDR_Frameset_69b57);
					big_reset(gb);
					return;
				}
				goto charge;
			}
			alu_cp(gb, v, 0x04);
			if (!(gb->cpu.f & FLAG_Z)) {
				RET();
				return;
			}
			v4 = true;
		}
		if (v4) {
			// .asm_4a76c
			SET_BC(gb, ADDR_Data_60d50);
			CALL(ApplyObjXMovement);
			hl = LO(REG_HL(gb), OBJ_VAR_1);
		}
		// .asm_4a774
		gb->cpu.a = 0x81;
		wr8(gb, wCurObjAction, 0x81);
		hl = DEC_L(hl);
		SET_HL(gb, hl);
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			RET();
			return;
		}
		big_turn(gb, hl);
		return;
	}
charge:
	// .asm_4a78b: charges until it hits a wall.
	{
		bool right;
		if (!check_ahead(gb, BIG(0x10d), BIG(0x121), &right)) {
			JUMP_TO(right ? ADDR_MoveObjectRight_Fast : ADDR_MoveObjectLeft_Fast);
			return;
		}
		squirt(gb, right);
	}
}
