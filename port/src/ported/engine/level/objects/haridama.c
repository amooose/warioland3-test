// Port of src/engine/level/objects/haridama.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define HD(off) (ADDR_HaridamaFunc + (off))
#define HD_INIT HD(0xa) // .Init
#define HD_UPDATE HD(0xed) // .Update
#define HD_FUNC_490E8 HD(0x206)
#define HD_FUNC_49199 HD(0x2b7)
#define HD_FUNC_491B6 HD(0x2d4)

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

// Checks for a wall ahead: Func_3513 at the position 8 up and dx to the
// side, returns a & $0f (z: free).
static void wall_ahead(GB *gb, int dx, uint16_t ret)
{
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x08);
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
	gb_call_ret(gb, ADDR_Func_3513, ret);
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
}

PORT_LOCAL(HaridamaFunc, Func_49199) { wall_ahead(gb, -9, HD(0x2d1)); }
PORT_LOCAL(HaridamaFunc, Func_491b6) { wall_ahead(gb, 8, HD(0x2ee)); }

// call .Func_49199 / .Func_491b6 (as the asm does, through the stack);
// returns whether the way is free
static bool way_free(GB *gb, bool right, uint16_t ret)
{
	gb_call_ret(gb, right ? HD_FUNC_491B6 : HD_FUNC_49199, ret);
	return gb->cpu.f & FLAG_Z;
}

PORT(HaridamaFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(HD_INIT >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)HD_INIT;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT_LOCAL_JUMP(HaridamaFunc, Init)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	hl = set_update(gb, hl, HD_UPDATE);
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x07);
	wr8(gb, hl--, 0xf8);
	wr8(gb, hl, 0xfd);
	hl = LO(hl, OBJ_SUBSTATE);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C) {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
		SET_DE(gb, ADDR_Frameset_69290);
	} else {
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR));
		SET_DE(gb, ADDR_Frameset_6927f);
	}
	JUMP(SetObjectFramesetPtr);
}

// .asm_48f8e (de = frameset): starts rolling.
static void roll(GB *gb)
{
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, 0x1b);
	gb->cpu.a = 0x5a;
	wr8(gb, wCurObjState, 0x5a);
	SET_HL(gb, set_interaction(gb, hl, OBJ_INTERACTION_06));
	RET();
}

// .asm_48fa2 (de = frameset): puffs up its spikes.
static void puff(GB *gb)
{
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, 0x12);
	SET_HL(gb, hl);
	gb->cpu.a = 0x5b;
	wr8(gb, wCurObjState, 0x5b);
	RET();
}

// .Func_48f28: walks slowly, puffing up when Wario's close in front.
static void walk(GB *gb)
{
	gb->cpu.c = 0x2a;
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	bool right = gb->cpu.f & FLAG_C;
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenYPos), gb->cpu.c);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.c);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	alu_cp(gb, gb->cpu.a, 0x08);
	bool close = gb->cpu.f & FLAG_C;
	if (!close) {
		alu_cp(gb, gb->cpu.a, 0xe8);
		close = !(gb->cpu.f & FLAG_C);
	}
	if (close) {
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), gb->cpu.c);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), gb->cpu.c);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		alu_cp(gb, gb->cpu.a, right ? 0xc0 : 0x40);
		if (right ? !(gb->cpu.f & FLAG_C) : (gb->cpu.f & FLAG_C)) {
			SET_DE(gb, right ? ADDR_Frameset_692ce : ADDR_Frameset_692c7);
			puff(gb);
			return;
		}
	}
	if (way_free(gb, right, right ? HD(0xa6) : HD(0x76))) {
		JUMP_TO(right ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
		return;
	}
	SET_DE(gb, right ? ADDR_Frameset_692aa : ADDR_Frameset_692a1);
	roll(gb);
}

// .asm_49054: back to walking.
static void stop(GB *gb)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_69290 : ADDR_Frameset_6927f);
	JUMP(SetObjectFramesetPtr);
}

// .asm_48ff6
static void roll_by_dir(GB *gb)
{
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_692aa : ADDR_Frameset_692a1);
	roll(gb);
}

// .asm_49006 / .asm_49013 (hl = wCurObjState): knocked from the given side.
static void knocked(GB *gb, uint16_t hl, bool from_right)
{
	gb->cpu.a = 0x32;
	wr8(gb, hl--, 0x32);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	bool c = gb->cpu.f & FLAG_C;
	if (from_right ? !c : c) {
		roll_by_dir(gb);
		return;
	}
	SET_DE(gb, from_right ? ADDR_Frameset_692b8 : ADDR_Frameset_692b3);
	JUMP(SetObjectFramesetPtr);
}

// .Func_49094: floats up while in water.
static void float_up(GB *gb)
{
	for (int i = 0; i < 2; i++) {
		uint16_t hl = wCurObjYPos;
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x11);
		wr8(gb, hYPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
		wr8(gb, hYPosHi, gb->cpu.a);
		if (!i) {
			gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x08);
			wr8(gb, hXPosLo, gb->cpu.a);
			gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
		} else {
			gb->cpu.a = alu_add(gb, rd8(gb, hl++), 0x07);
			wr8(gb, hXPosLo, gb->cpu.a);
			gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
		}
		wr8(gb, hXPosHi, gb->cpu.a);
		SET_HL(gb, hl);
		gb_call_ret(gb, ADDR_Func_358b, i ? HD(0x1ef) : HD(0x1cc));
		if (alu_and(gb, gb->cpu.a, gb->cpu.a))
			break;
		if (!i) {
			gb->cpu.a = rd8(gb, wIsInWaterOrSand);
			if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
				break;
		} else {
			JUMP(MoveObjectUp_Slow);
			return;
		}
	}
	// .asm_490d5
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(HD_FUNC_490E8 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)HD_FUNC_490E8;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = 0x64;
	wr8(gb, wCurObjStateDuration, 0x64);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjState, 0);
	RET();
}

PORT_LOCAL_JUMP(HaridamaFunc, Update)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	uint8_t s = gb->cpu.a;
	if (!alu_and(gb, s, s)) {
		walk(gb);
		return;
	}
	alu_cp(gb, s, 0x5a);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_4903e
		hl = LO(hl, OBJ_STATE_DURATION);
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, 0x0d);
		if (gb->cpu.f & FLAG_Z) {
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
			RET();
			return;
		}
		stop(gb);
		return;
	}
	alu_cp(gb, s, 0x5b);
	if (gb->cpu.f & FLAG_Z) {
		// .Func_48fae
		hl = LO(hl, OBJ_STATE_DURATION);
		SET_HL(gb, hl);
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			RET();
			return;
		}
		hl = set_interaction(gb, hl, OBJ_INTERACTION_HARIDAMA);
		hl = LO(hl, OBJ_STATE);
		gb->cpu.a = 0x32;
		wr8(gb, hl--, 0x32);
		gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
		SET_HL(gb, hl);
		SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_692c2 : ADDR_Frameset_692bd);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	alu_cp(gb, s, 0x02);
	if (gb->cpu.f & FLAG_Z) {
		knocked(gb, hl, true);
		return;
	}
	alu_cp(gb, s, 0x03);
	if (gb->cpu.f & FLAG_Z) {
		knocked(gb, hl, false);
		return;
	}
	gb->cpu.a = alu_and(gb, s, 0xfe);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_49069
		hl = LO(hl, OBJ_UNK_1D);
		SET_HL(gb, hl);
		if (!alu_bit(gb, rd8(gb, hl), 7)) {
			hl = LO(hl, OBJ_SUBSTATE);
			gb->cpu.a = alu_rlca(gb, rd8(gb, hl++));
			knocked(gb, hl, gb->cpu.f & FLAG_C);
			return;
		}
		// .asm_49077
		hl = set_interaction(gb, hl, OBJ_INTERACTION_06);
		hl = LO(hl, OBJ_STATE);
		gb->cpu.a = 0x38;
		wr8(gb, hl--, 0x38);
		gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
		SET_HL(gb, hl);
		SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_692de : ADDR_Frameset_692d5);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x32);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_49020
		gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
		bool right = gb->cpu.f & FLAG_C;
		if (way_free(gb, right, right ? HD(0x153) : HD(0x147))) {
			JUMP_TO(right ? ADDR_MoveObjectRight_Fast : ADDR_MoveObjectLeft_Fast);
			return;
		}
		SET_DE(gb, right ? ADDR_Frameset_692aa : ADDR_Frameset_692a1);
		roll(gb);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x38);
	if (gb->cpu.f & FLAG_Z) {
		float_up(gb);
		return;
	}
	roll_by_dir(gb);
}

// Floats on the surface, drifting back and forth.
PORT_LOCAL_JUMP(HaridamaFunc, Func_490e8)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	uint8_t s = gb->cpu.a;
	if (!alu_and(gb, s, s)) {
		// .asm_49118
		hl = LO(hl, OBJ_STATE_DURATION);
		SET_HL(gb, hl);
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (!d) {
			// .asm_4913f
			hl = set_update(gb, hl, HD_UPDATE);
			hl = LO(hl, OBJ_SUBSTATE);
			SET_HL(gb, hl);
			alu_rlca(gb, rd8(gb, hl));
			bool right = gb->cpu.f & FLAG_C;
			gb->cpu.a = rd8(gb, wCurObjFrame);
			alu_cp(gb, gb->cpu.a, 0x1a);
			bool c = gb->cpu.f & FLAG_C;
			if (right ? !c : c) {
				stop(gb);
				return;
			}
			if (right) {
				wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR));
				SET_DE(gb, ADDR_Frameset_692a1);
			} else {
				wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
				SET_DE(gb, ADDR_Frameset_692aa);
			}
			roll(gb);
			return;
		}
		gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
		if (gb->cpu.a) {
			RET();
			return;
		}
		gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
		bool right = gb->cpu.f & FLAG_C;
		if (way_free(gb, right, right ? HD(0x252) : HD(0x24a))) {
			JUMP_TO(right ? ADDR_MoveObjectRight : ADDR_MoveObjectLeft);
			return;
		}
		// .asm_49137
		hl = wCurObjSubState;
		SET_HL(gb, hl);
		gb->cpu.a = alu_xor(gb, rd8(gb, hl), 0x80);
		wr8(gb, hl, gb->cpu.a);
		RET();
		return;
	}
	alu_cp(gb, s, 0x32);
	bool left = gb->cpu.f & FLAG_Z;
	if (!left)
		alu_cp(gb, s, 0x33);
	if (left || (gb->cpu.f & FLAG_Z)) {
		// .asm_4916a / .Func_49184
		hl = wCurObjVar1;
		SET_HL(gb, hl);
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (!d) {
			// .asm_4917f
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wCurObjState, 0);
			RET();
			return;
		}
		if (way_free(gb, !left, left ? HD(0x291) : HD(0x2ab))) {
			JUMP_TO(left ? ADDR_MoveObjectLeft : ADDR_MoveObjectRight);
			return;
		}
		hl = wCurObjState;
		gb->cpu.a = left ? 0x33 : 0x32;
		wr8(gb, hl--, gb->cpu.a);
		SET_HL(gb, hl);
		if (left)
			wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
		else
			wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR));
		RET();
		return;
	}
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C) {
		gb->cpu.a = 0x32;
		wr8(gb, hl--, 0x32);
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_HDIR));
	} else {
		// .asm_4910d
		gb->cpu.a = 0x33;
		wr8(gb, hl--, 0x33);
		wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJSUBFLAG_HDIR));
	}
	SET_HL(gb, hl);
	// .asm_49112
	gb->cpu.a = 0x3c;
	wr8(gb, wCurObjVar1, 0x3c);
	RET();
}
