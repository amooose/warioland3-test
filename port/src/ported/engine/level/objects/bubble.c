// Port of src/engine/level/objects/bubble.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define BUBBLE_HOLE_UPDATE (ADDR_BubbleHoleFunc + 0x13) // .Update
#define BUBBLE_UPDATE (ADDR_BubbleFunc + 0x1f) // .Update
#define BUBBLE_FUNC_47C88 (ADDR_BubbleFunc + 0x52) // .Func_47c88
#define BUBBLE_BURST (ADDR_BubbleFunc + 0x43) // .Burst
#define BB(off) ((uint16_t)(ADDR_BubbleFunc + (off)))

static void spawn(GB *gb);

// Spawns a bubble, unless there are already enough of them (1 while Wario is
// Bubble Wario, else 2).
PORT_JUMP(BubbleHoleFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(BUBBLE_HOLE_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)BUBBLE_HOLE_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	SET_HL(gb, hl);
	spawn(gb);
}

// .asm_47c08
static void spawn(GB *gb)
{
	CALL(CountActiveTransientObjects);
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_BUBBLE);
	gb->cpu.a = gb->cpu.e;
	alu_cp(gb, gb->cpu.a, (gb->cpu.f & FLAG_Z) ? 1 : 2);
	if (!(gb->cpu.f & FLAG_C)) {
		// .wait
		gb->cpu.a = 1;
		wr8(gb, wCurObjStateDuration, 1);
		RET();
		return;
	}
	// .create_bubble
	gb->cpu.a = 208;
	wr8(gb, wCurObjStateDuration, 208);
	play_sfx(gb, SFX_069);
	SET_BC(gb, ADDR_ObjParams_Bubble);
	JUMP(CreateObjectFromCurObjPos);
}

PORT_LOCAL_JUMP(BubbleHoleFunc, Update)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	gb->cpu.a = 2;
	wr8(gb, wCurObjFrameDuration, 2);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	spawn(gb);
}

PORT_JUMP(BubbleFunc)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP_TO(BUBBLE_FUNC_47C88);
		return;
	}
	SET_DE(gb, ADDR_Frameset_690f4);
	CALL(SetObjectFramesetPtr);
	hl = LO(REG_HL(gb), OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(BUBBLE_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)BUBBLE_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	RET();
}

static void destroy(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}

PORT_LOCAL(BubbleFunc, Update)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		destroy(gb);
		return;
	}
	gb_call_ret(gb, BUBBLE_FUNC_47C88, BB(0x28));
}

PORT_LOCAL(BubbleFunc, Burst)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	destroy(gb);
}

static void set_burst(GB *gb)
{
	SET_DE(gb, ADDR_Frameset_6910c);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 16;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, 16);
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(BUBBLE_BURST >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)BUBBLE_BURST;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	play_sfx(gb, SFX_072);
}

// [hl] + delta (sub/sbc, or add/adc for positive delta) into lo/hi
static uint16_t offset_pos(GB *gb, uint16_t hl, int delta, uint16_t lo, uint16_t hi)
{
	if (delta < 0) {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), (uint8_t)-delta);
		wr8(gb, lo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
		wr8(gb, hi, gb->cpu.a);
	} else {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), (uint8_t)delta);
		wr8(gb, lo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
		wr8(gb, hi, gb->cpu.a);
	}
	SET_HL(gb, hl);
	return hl;
}

// wCurObjYPos - dy into hYPos, wCurObjXPos as is into hXPos
static void pos_above(GB *gb, int dy)
{
	uint16_t hl = offset_pos(gb, wCurObjYPos, dy, hYPosLo, hYPosHi);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hXPosLo, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

// .asm_47d2b / .asm_47d88: turns around
static void turn(GB *gb, bool left)
{
	uint16_t hl = wCurObjSubState;
	uint8_t v = rd8(gb, hl);
	wr8(gb, hl, left ? (uint8_t)(v | OBJSUBFLAG_HDIR) : (uint8_t)(v & ~OBJSUBFLAG_HDIR));
	hl = LO(hl, OBJ_VAR_1);
	gb->cpu.a = 0x30;
	wr8(gb, hl++, 0x30);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
}

// Drifts sideways, turning around at walls.
static void drift(GB *gb, bool left)
{
	offset_pos(gb, offset_pos(gb, wCurObjYPos, -0x18, hYPosLo, hYPosHi), left ? -0x0c : 0x0b, hXPosLo,
		hXPosHi);
	gb_call_ret(gb, ADDR_Func_3513, BB(left ? 0xdd : 0x13a));
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	if (!gb->cpu.a) {
		offset_pos(gb, wCurObjYPos, -0x07, hYPosLo, hYPosHi);
		gb_call_ret(gb, ADDR_Func_3513, BB(left ? 0xf1 : 0x14e));
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	}
	if (gb->cpu.a) {
		turn(gb, left);
		return;
	}
	// .asm_47d38 / .asm_47d95
	uint16_t hl = wCurObjVar2;
	SET_HL(gb, hl);
	gb->cpu.b = 0x40;
	gb->cpu.a = alu_add(gb, rd8(gb, hl), left ? 0x20 : 0x50);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		gb->cpu.a = rd8(gb, REG_BC(gb));
	} else {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	}
	gb->cpu.b = gb->cpu.a;
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	hl = LO(hl, OBJ_X_POS);
	gb->cpu.a = alu_add(gb, rd8(gb, hl), gb->cpu.b);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	if (left) {
		if (gb->cpu.f & FLAG_C)
			return;
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	} else {
		if (!(gb->cpu.f & FLAG_C))
			return;
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	}
}

// Rises while in water (sideways every other frame on screen), and bursts
// at the surface or on hitting a ceiling.
PORT_LOCAL(BubbleFunc, Func_47c88)
{
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	if (alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F)) {
		gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 1);
		if (gb->cpu.a) {
			// .asm_47ce0
			hl = wCurObjVar1;
			uint8_t v = alu_dec(gb, rd8(gb, hl));
			wr8(gb, hl, v);
			hl = LO(hl, OBJ_SUBSTATE);
			SET_HL(gb, hl);
			if (!v) {
				gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
				wr8(gb, hl, gb->cpu.a);
				hl = LO(hl, OBJ_VAR_1);
				gb->cpu.a = 0x30;
				wr8(gb, hl++, 0x30);
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z; // xor a
				wr8(gb, hl, 0);
				SET_HL(gb, hl);
				return;
			}
			gb->cpu.a = alu_rla(gb, rd8(gb, hl));
			drift(gb, !(gb->cpu.f & FLAG_C));
			return;
		}
	}
	// .asm_47c96
	hl = LO(hl, OBJ_Y_POS);
	gb->cpu.a = alu_sub(gb, rd8(gb, hl), 1);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	if (gb->cpu.f & FLAG_C)
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	pos_above(gb, -0x18);
	gb_call_ret(gb, ADDR_Func_358b, BB(0x7f));
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		set_burst(gb);
		return;
	}
	gb->cpu.a = rd8(gb, wIsInWaterOrSand);
	wr8(gb, hIsInWaterOrSandBackup, gb->cpu.a);
	pos_above(gb, -0x0c);
	gb_call_ret(gb, ADDR_Func_358b, BB(0x9e));
	gb->cpu.a = rd8(gb, wIsInWaterOrSand);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return; // still underwater
	gb->cpu.a = rd8(gb, hIsInWaterOrSandBackup);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return; // still underwater
	set_burst(gb);
}
