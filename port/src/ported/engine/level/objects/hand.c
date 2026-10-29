// Port of src/engine/level/objects/hand.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))
#define INC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_inc(gb, (uint8_t)h_)); }) // inc l (sets flags)

#define HAND_UPDATE (ADDR_HandFunc + 0x24) // .Update
#define HAND_JUMPTABLE (ADDR_HandFunc + 0x3f + 4) // after .asm_4778e's rst JumpTable

PORT(HandFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(HAND_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)HAND_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_BOTTOM);
	wr8(gb, hl++, (uint8_t)-12);
	wr8(gb, hl++, (uint8_t)-6);
	wr8(gb, hl, 5);
	hl = LO(hl, OBJ_STATE_DURATION);
	wr8(gb, hl++, 50);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0); // var1
	SET_HL(gb, hl);
}

// .ApplyOffsetToWario: puts Wario at the hand's position, b pixels up.
static void apply_offset_to_wario(GB *gb)
{
	uint16_t hl = LO(REG_HL(gb), OBJ_Y_POS);
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), gb->cpu.b);
	wr8(gb, wWarioYPos + 1, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	wr8(gb, wWarioYPos + 0, gb->cpu.a);
	wr8(gb, wWarioXPos + 1, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wWarioXPos + 0, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT_LOCAL(HandFunc, ApplyOffsetToWario) { apply_offset_to_wario(gb); }

PORT_LOCAL_JUMP(HandFunc, Update)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		alu_cp(gb, gb->cpu.a, 0x2e);
		if (!(gb->cpu.f & FLAG_Z)) {
			alu_cp(gb, gb->cpu.a, 0x2f);
			if (gb->cpu.f & FLAG_Z) {
				gb->cpu.a = 0x2e;
				wr8(gb, hl, 0x2e);
				gb->cpu.b = 11;
				apply_offset_to_wario(gb);
			} else {
				// .asm_4778c
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z; // xor a
				wr8(gb, hl, 0);
			}
		}
	}
	// .asm_4778e
	gb->cpu.a = rd8(gb, wCurObjVar1);
	gb_push16(gb, HAND_JUMPTABLE); // rst JumpTable
	hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	JUMP_TO(REG_HL(gb));
}

// ld a, no_actions_for 1 / ld [wCurObjAction], a / ld hl, wCurObjStateDuration /
// dec [hl]; returns whether it ran out
static bool count_down(GB *gb)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	return !d;
}

// call SetObjectFramesetPtr / ld a, duration / ld [hli], a / inc [hl] (next
// step in OBJ_VAR_1)
static uint16_t next_step(GB *gb, uint16_t frameset, uint8_t duration)
{
	SET_DE(gb, frameset);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = duration;
	wr8(gb, hl++, duration);
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	SET_HL(gb, hl);
	return hl;
}

// Waits hidden in the sand, then pops out.
PORT_LOCAL(HandFunc, Func_477b2)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, wIsInSand);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// .in_sand
		gb->cpu.a = 50;
		wr8(gb, hl, 50);
		return;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	hl = wCurObjFlags;
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	next_step(gb, ADDR_Frameset_690ab, 41);
	play_sfx(gb, SFX_081);
}

PORT_LOCAL(HandFunc, Func_477de)
{
	if (!count_down(gb))
		return;
	next_step(gb, ADDR_Frameset_690b4, 1);
}

PORT_LOCAL_JUMP(HandFunc, Func_477f3)
{
	if (!count_down(gb)) {
		RET();
		return;
	}
	uint16_t hl = INC_L(next_step(gb, ADDR_Frameset_690b7, 1));
	gb->cpu.a = 4;
	wr8(gb, hl, 4);
	SET_HL(gb, hl);
	JUMP(MoveObjectUpByVar2);
}

// Rising out of the sand, a step at a time.
static void rise(GB *gb, uint16_t frameset)
{
	if (!count_down(gb)) {
		RET();
		return;
	}
	next_step(gb, frameset, 1);
	JUMP(MoveObjectUpByVar2);
}

PORT_LOCAL_JUMP(HandFunc, Func_4780e) { rise(gb, ADDR_Frameset_690ba); }
PORT_LOCAL_JUMP(HandFunc, Func_47825) { rise(gb, ADDR_Frameset_690bd); }
PORT_LOCAL_JUMP(HandFunc, Func_4783c) { rise(gb, ADDR_Frameset_690c0); }
PORT_LOCAL_JUMP(HandFunc, Func_47853) { rise(gb, ADDR_Frameset_690c3); }
PORT_LOCAL_JUMP(HandFunc, Func_4786a) { rise(gb, ADDR_Frameset_690c6); }

PORT_LOCAL(HandFunc, Func_47881)
{
	if (!count_down(gb))
		return;
	next_step(gb, ADDR_Frameset_690c9, 56);
}

// ld a, [wCurObjState] / cp $2e / ret nz / ld b, b / jp .ApplyOffsetToWario
static void drag_wario(GB *gb, uint8_t b)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	alu_cp(gb, gb->cpu.a, 0x2e);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.b = b;
	apply_offset_to_wario(gb);
}

// Holds still, carrying Wario if it caught him, then sinks back.
PORT_LOCAL(HandFunc, Func_47896)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 49);
	if (gb->cpu.f & FLAG_C) {
		alu_cp(gb, gb->cpu.a, 26);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.a = alu_sub(gb, rd8(gb, wCurObjScreenYPos), 4);
			gb->cpu.b = gb->cpu.a;
			gb->cpu.a = rd8(gb, wWarioScreenYPos);
			alu_cp(gb, gb->cpu.a, gb->cpu.b);
			if (gb->cpu.f & FLAG_C) {
				wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
				return;
			}
		} else {
			// .asm_478b0
			gb->cpu.a = rd8(gb, wCurObjState);
			alu_cp(gb, gb->cpu.a, 0x2e);
			if (gb->cpu.f & FLAG_Z) {
				gb->cpu.b = 7;
				apply_offset_to_wario(gb);
				hl = LO(REG_HL(gb), OBJ_STATE_DURATION);
				SET_HL(gb, hl);
			}
		}
	}
	// .asm_478be
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	hl = INC_L(next_step(gb, ADDR_Frameset_690dc, 2));
	gb->cpu.a = 6;
	wr8(gb, hl, 6);
	SET_HL(gb, hl);
	CALL(MoveObjectDownByVar2);
	drag_wario(gb, 9);
}

// Sinking back into the sand, a step at a time.
static void sink(GB *gb, uint16_t frameset, uint8_t b)
{
	if (!count_down(gb))
		return;
	next_step(gb, frameset, 2);
	CALL(MoveObjectDownByVar2);
	drag_wario(gb, b);
}

PORT_LOCAL(HandFunc, Func_478e1) { sink(gb, ADDR_Frameset_690df, 8); }
PORT_LOCAL(HandFunc, Func_47903) { sink(gb, ADDR_Frameset_690e2, 6); }
PORT_LOCAL(HandFunc, Func_47925) { sink(gb, ADDR_Frameset_690e5, 8); }

PORT_LOCAL(HandFunc, Func_47947)
{
	if (!count_down(gb))
		return;
	next_step(gb, ADDR_Frameset_690e8, 2);
	drag_wario(gb, 0);
}

PORT_LOCAL(HandFunc, Func_47966)
{
	if (!count_down(gb))
		return;
	next_step(gb, ADDR_Frameset_690eb, 41);
	gb->cpu.a = rd8(gb, wCurObjState);
	alu_cp(gb, gb->cpu.a, 0x2e);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wAutoMoveState, 0);
	wr8(gb, wCurObjState, 0);
}

PORT_LOCAL(HandFunc, Invisible)
{
	if (!count_down(gb))
		return;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, 50);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	hl = wCurObjFlags;
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_INVISIBLE));
}
