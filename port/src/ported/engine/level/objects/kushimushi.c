// Port of src/engine/level/objects/kushimushi.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define KUSHIMUSHI_FUNC_42ACF (ADDR_KushimushiVerticalFunc + 0x10b) // KushimushiVerticalFunc.Func_42acf

PORT(KushimushiHorizontalFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(KUSHIMUSHI_FUNC_42ACF >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)KUSHIMUSHI_FUNC_42ACF;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68611);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x60);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, hl++, 0);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | OBJSUBFLAG_HDIR));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 4);
	wr8(gb, hl--, (uint8_t)-5); // OBJ_COLLBOX_LEFT
	gb->cpu.a = (uint8_t)-4;
	wr8(gb, hl, gb->cpu.a); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
}

#define KUSHIMUSHI_UPDATE (ADDR_KushimushiVerticalFunc + 0x29) // .Update

PORT(KushimushiVerticalFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(KUSHIMUSHI_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)KUSHIMUSHI_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_6861b);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x60);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, hl++, 0);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 3);
	wr8(gb, hl--, (uint8_t)-4); // OBJ_COLLBOX_LEFT
	gb->cpu.a = (uint8_t)-3;
	wr8(gb, hl, gb->cpu.a); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
}

// .asm_42a4b: knocked over; hands the object to the given update function
// (a:c) with OBJ_STATE_DURATION b.
static void knocked(GB *gb, uint16_t hl, uint16_t func)
{
	gb->cpu.a = (uint8_t)(func >> 8);
	gb->cpu.c = (uint8_t)func;
	gb->cpu.b = 0x02;
	hl = LO(hl, OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.c);
	hl = LO(hl, OBJ_SUBSTATE);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), 0xf0);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	SET_DE(gb, ADDR_Frameset_68698);
	JUMP(SetObjectFramesetPtr);
}

// .asm_42ac3 / .asm_42b71: ld [hli], a / ld [hl], b / sets the interaction
// type to c / jp SetObjectFramesetPtr
static void next_phase(GB *gb, uint16_t hl, uint16_t frameset, uint8_t b, uint8_t c, uint8_t a)
{
	SET_DE(gb, frameset);
	gb->cpu.b = b;
	gb->cpu.c = c;
	gb->cpu.a = a;
	wr8(gb, hl++, a); // OBJ_STATE_DURATION
	wr8(gb, hl, b); // OBJ_VAR_1
	hl = LO(hl, OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), c);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	JUMP(SetObjectFramesetPtr);
}

// The shared update of both Kushimushi: reacts to being hit, else moves
// back and forth with a pause and a sting at each end.
static void kushimushi_update(GB *gb, bool horizontal)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint8_t s = gb->cpu.a;
		alu_cp(gb, s, horizontal ? 0x0b : 0x0e);
		bool z = gb->cpu.f & FLAG_Z;
		if (!z) {
			alu_cp(gb, s, horizontal ? 0x0c : 0x0d);
			z = gb->cpu.f & FLAG_Z;
		}
		if (z) {
			// .asm_42a16 / .asm_42afe
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, hl, 0);
			hl = LO(hl, OBJ_VAR_2);
			gb->cpu.a = 0x3c;
			wr8(gb, hl--, 0x3c);
			SET_HL(gb, hl);
			gb->cpu.a = rd8(gb, hl);
			bool nz = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
			if (horizontal)
				SET_DE(gb, nz ? ADDR_Frameset_68620 : ADDR_Frameset_68625);
			else
				SET_DE(gb, nz ? ADDR_Frameset_6862a : ADDR_Frameset_6862f);
			JUMP(SetObjectFramesetPtr);
			return;
		}
		alu_cp(gb, s, 0x0a);
		if (gb->cpu.f & FLAG_Z) {
			JUMP(VanishObject2);
			return;
		}
		uint16_t func;
		alu_cp(gb, s, 0x28);
		if (gb->cpu.f & FLAG_Z) {
			func = ADDR_Func_33f8;
		} else if (alu_cp(gb, s, 0x29), gb->cpu.f & FLAG_Z) {
			func = ADDR_Func_3407;
		} else if (alu_cp(gb, s, 0x04), gb->cpu.f & FLAG_Z) {
			func = ADDR_Func_3326;
		} else if (alu_cp(gb, s, 0x05), gb->cpu.f & FLAG_Z) {
			func = ADDR_Func_3317;
		} else {
			gb->cpu.a = alu_rra(gb, s);
			func = (gb->cpu.f & FLAG_C) ? ADDR_Func_33f8 : ADDR_Func_3407;
		}
		knocked(gb, hl, func);
		return;
	}
	// .asm_42a68 / .asm_42b15
	hl = LO(hl, OBJ_VAR_2);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		if (!horizontal) {
			gb->cpu.a = 2;
			wr8(gb, wCurObjFrameDuration, 2);
		}
		RET();
		return;
	}
	gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	hl = LO(hl, OBJ_VAR_1);
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	uint8_t phase = gb->cpu.a;
	if (alu_and(gb, phase, phase)) {
		alu_cp(gb, phase, 0x01);
		if (!(gb->cpu.f & FLAG_Z)) {
			alu_cp(gb, phase, 0x02);
			if (!(gb->cpu.f & FLAG_Z)) {
				alu_cp(gb, phase, 0x03);
				if (!(gb->cpu.f & FLAG_Z)) {
					RET();
					return;
				}
			}
		}
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl)); // OBJ_STATE_DURATION
	wr8(gb, hl, d);
	if (phase == 0 || phase == 2) {
		// move
		if (d) {
			if (horizontal)
				JUMP_TO(phase == 0 ? ADDR_MoveObjectRight : ADDR_MoveObjectLeft);
			else
				JUMP_TO(phase == 0 ? ADDR_MoveObjectDown : ADDR_MoveObjectUp);
			return;
		}
		uint16_t fs;
		if (horizontal)
			fs = phase == 0 ? ADDR_Frameset_68655 : ADDR_Frameset_6864a;
		else
			fs = phase == 0 ? ADDR_Frameset_6863f : ADDR_Frameset_68634;
		next_phase(gb, hl, fs, phase + 1, OBJ_INTERACTION_01, 22);
		return;
	}
	// turn around
	if (d) {
		RET();
		return;
	}
	uint8_t b = phase == 1 ? 2 : 0;
	if (!horizontal) {
		next_phase(gb, hl, phase == 1 ? ADDR_Frameset_68616 : ADDR_Frameset_6861b, b,
			phase == 1 ? OBJ_INTERACTION_TOP_STING : OBJ_INTERACTION_BOTTOM_STING, 96);
		return;
	}
	SET_DE(gb, phase == 1 ? ADDR_Frameset_6860c : ADDR_Frameset_68611);
	gb->cpu.b = b;
	gb->cpu.c = OBJ_INTERACTION_FRONT_STING;
	// .flip_direction
	hl = LO(hl, OBJ_SUBSTATE);
	gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
	wr8(gb, hl, gb->cpu.a);
	hl = LO(hl, OBJ_STATE_DURATION);
	next_phase(gb, hl, REG_DE(gb), b, OBJ_INTERACTION_FRONT_STING, 96);
}

PORT_LOCAL_JUMP(KushimushiVerticalFunc, Update) { kushimushi_update(gb, false); }
PORT_LOCAL_JUMP(KushimushiVerticalFunc, Func_42acf) { kushimushi_update(gb, true); }
