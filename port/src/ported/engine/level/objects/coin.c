// Port of src/engine/level/objects/coin.asm
#include "ported.h"

#define CF(off) ((uint16_t)(ADDR_CoinFunc + (off)))
#define COIN_UPDATE CF(0x2f) // .Update
#define COIN_TABLE CF(0x7a) // the table after .state_table's rst JumpTable

// rst JumpTable: a = index, table = the address after the rst
static uint16_t jumptable(GB *gb, uint16_t table)
{
	gb_push16(gb, table); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	return REG_HL(gb);
}

static uint16_t lo_hl(GB *gb, uint8_t l)
{
	gb->cpu.l = l;
	return REG_HL(gb);
}

static void set_state(GB *gb, uint8_t state)
{
	gb->cpu.a = state;
	wr8(gb, wCurObjState, state);
}

// The y movement: [hl] is the index into the y velocity table at b:lo (a
// table ending in $80, whose last entry repeats), and the velocity goes in
// hffa0 and is added to the y position.
static void y_movement(GB *gb, uint16_t hl, uint8_t b, uint8_t lo)
{
	SET_HL(gb, hl);
	gb->cpu.b = b;
	gb->cpu.a = alu_add(gb, rd8(gb, hl), lo);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		gb->cpu.a = rd8(gb, REG_BC(gb));
	} else {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	}
	uint8_t vel = gb->cpu.a;
	wr8(gb, hffa0, vel);
	hl = lo_hl(gb, OBJ_Y_POS);
	alu_cp(gb, vel, 0x80);
	bool negative = !(gb->cpu.f & FLAG_C);
	gb->cpu.c = rd8(gb, hl);
	gb->cpu.a = alu_add(gb, vel, gb->cpu.c);
	wr8(gb, hl++, gb->cpu.a);
	bool carry = gb->cpu.f & FLAG_C;
	if (negative && !carry)
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	else if (!negative && carry)
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	SET_HL(gb, hl);
}

// ld a, [hli] / ldh [hYPosLo], a ... ld a, [hl] / ldh [hXPosHi], a
static void copy_pos(GB *gb, uint16_t hl)
{
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

// hl = 16-bit position: [hl] + delta (sub/sbc for negative delta, else
// add/adc) into lo/hi
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

// wCurObjYPos - 8 (or - 4) and wCurObjXPos + dx into hYPos / hXPos
static void offset_yx(GB *gb, int dy, int dx)
{
	uint16_t hl = offset_pos(gb, wCurObjYPos, dy, hYPosLo, hYPosHi);
	offset_pos(gb, hl, dx, hXPosLo, hXPosHi);
}

// ldh a, [hYPosLo] / ld [hli], a / ldh a, [hYPosHi] / ld [hl], a
static uint16_t store_y(GB *gb)
{
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = rd8(gb, hYPosLo);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	return hl;
}

PORT_JUMP(CoinFunc)
{
	gb->cpu.a = alu_and(gb, alu_dec(gb, rd8(gb, wGlobalCounter)), 0x3f);
	if (gb->cpu.f & FLAG_Z) {
		uint16_t hl = wCurObjInteractionType;
		gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_COLOUR_COIN);
		wr8(gb, hl++, gb->cpu.a);
		SET_HL(gb, hl);
		play_sfx(gb, SFX_08E);
		gb->cpu.a = 8;
	} else {
		gb->cpu.a = 0;
	}
	wr8(gb, wCurObjFrame, gb->cpu.a);
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(COIN_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)COIN_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	hl = lo_hl(gb, OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	JUMP_TO(COIN_UPDATE); // fallthrough
}

// Animates the coin, slower and slower, then runs its state.
PORT_LOCAL_JUMP(CoinFunc, Update)
{
	uint16_t hl = wCurObjFrameDuration;
	gb->cpu.a = 2;
	wr8(gb, hl++, 2);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
		if (gb->cpu.f & FLAG_C) {
			RET();
			return;
		}
	}
	// .animate
	hl = wCurObjVar1;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 150);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.b = 4;
	} else {
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
		wr8(gb, hl, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, 100);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.b = 3;
		} else {
			alu_cp(gb, gb->cpu.a, 50);
			gb->cpu.b = (gb->cpu.f & FLAG_C) ? 1 : 2;
		}
	}
	// .got_animation_speed
	hl = lo_hl(gb, OBJ_FRAMESET_PTR);
	uint8_t v = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, v);
	if (!v) {
		gb->cpu.a = gb->cpu.b;
		wr8(gb, hl++, gb->cpu.a);
		SET_HL(gb, hl);
		v = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, v);
		if (!v) {
			gb->cpu.b = 8;
			wr8(gb, hl, 8);
			hl = lo_hl(gb, OBJ_FRAME);
			gb->cpu.a = alu_sub(gb, rd8(gb, hl), 7);
			wr8(gb, hl, gb->cpu.a);
		} else {
			hl = lo_hl(gb, OBJ_FRAME);
			wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
		}
	}
	// .state_table
	gb->cpu.a = rd8(gb, wCurObjState);
	JUMP_TO(jumptable(gb, COIN_TABLE));
}

PORT_LOCAL(CoinFunc, Init)
{
	uint16_t hl = wCurObjAction;
	gb->cpu.a = 15 | (1 << 7);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = 0x1a;
	wr8(gb, hl, 0x1a); // OBJ_STATE
	SET_HL(gb, hl);
	hl = lo_hl(gb, OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
}

// Moves away from Wario.
PORT_LOCAL(CoinFunc, StartMovement)
{
	gb->cpu.c = 0x2a;
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	set_state(gb, (gb->cpu.f & FLAG_C) ? 0x06 : 0x07);
}

// move_coin's .MoveXStart
static void move_start(GB *gb, uint8_t state, uint8_t duration)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	SET_HL(gb, hl);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0); // OBJ_VAR_3
	SET_HL(gb, hl);
	gb->cpu.a = duration;
	wr8(gb, wCurObjStateDuration, duration);
}

// .asm_63502 / .asm_635db: moves sideways, and falls on hitting a wall or
// when the duration runs out
static void move_side(GB *gb, bool left)
{
	if (left)
		CALL(MoveObjectLeftByVar2);
	else
		CALL(MoveObjectRightByVar2);
	offset_yx(gb, -8, left ? -4 : 3);
	gb_call_ret(gb, ADDR_Func_3513, CF(left ? 0x19c : 0x275));
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	if (gb->cpu.a) {
		set_state(gb, left ? 0x0d : 0x0c);
		return;
	}
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	set_state(gb, left ? 0x0c : 0x0d);
}

static void move(GB *gb, uint16_t table, bool left)
{
	y_movement(gb, wCurObjVar3, (uint8_t)(table >> 8), (uint8_t)table);
	move_side(gb, left);
}

PORT_LOCAL(CoinFunc, Move1Start) { move_start(gb, 0x0a, 6); move(gb, ADDR_Data_60300, true); }
PORT_LOCAL(CoinFunc, Move1) { move(gb, ADDR_Data_60300, true); }
PORT_LOCAL(CoinFunc, Move2Start) { move_start(gb, 0x02, 4); move(gb, ADDR_Data_602c0, true); }
PORT_LOCAL(CoinFunc, Move2) { move(gb, ADDR_Data_602c0, true); }
PORT_LOCAL(CoinFunc, Move3Start) { move_start(gb, 0x06, 5); move(gb, ADDR_Data_602e0, true); }
PORT_LOCAL(CoinFunc, Move3) { move(gb, ADDR_Data_602e0, true); }
PORT_LOCAL(CoinFunc, Move4Start) { move_start(gb, 0x0b, 6); move(gb, ADDR_Data_60300, false); }
PORT_LOCAL(CoinFunc, Move4) { move(gb, ADDR_Data_60300, false); }
PORT_LOCAL(CoinFunc, Move5Start) { move_start(gb, 0x03, 4); move(gb, ADDR_Data_602c0, false); }
PORT_LOCAL(CoinFunc, Move5) { move(gb, ADDR_Data_602c0, false); }
PORT_LOCAL(CoinFunc, Move6Start) { move_start(gb, 0x07, 5); move(gb, ADDR_Data_602e0, false); }
PORT_LOCAL(CoinFunc, Move6) { move(gb, ADDR_Data_602e0, false); }

// ld hl, wCurObjState / ld a, state / ld [hld], a / dec l / xor a / ld [hld], a
static void start_state(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	SET_HL(gb, hl);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
}

// .FallLeft / .FallRight: falls sideways until it lands, then bounces (by
// how fast it fell) or slides
static void fall(GB *gb, bool left)
{
	if (left)
		CALL(MoveObjectLeftByVar2);
	else
		CALL(MoveObjectRightByVar2);
	y_movement(gb, lo_hl(gb, OBJ_VAR_3), (uint8_t)(ADDR_FallingYVel_Light >> 8),
		(uint8_t)ADDR_FallingYVel_Light);
	copy_pos(gb, lo_hl(gb, OBJ_Y_POS));
	gb_call_ret(gb, ADDR_Func_352b, CF(left ? 0x2ce : 0x377));
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = rd8(gb, wIsInWaterOrSand);
		if (alu_and(gb, gb->cpu.a, gb->cpu.a))
			wr8(gb, wCurObjFramesetOffset, gb->cpu.a);
		offset_yx(gb, -4, left ? -4 : 3);
		gb_call_ret(gb, ADDR_Func_3513, CF(left ? 0x2f4 : 0x39d));
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
		if (!gb->cpu.a)
			return;
		set_state(gb, left ? 0x0d : 0x0c);
		return;
	}
	// landed
	uint16_t hl = store_y(gb);
	hl = lo_hl(gb, OBJ_VAR_2);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	gb->cpu.a = rd8(gb, hffa0);
	uint8_t state;
	alu_cp(gb, gb->cpu.a, 2);
	if (gb->cpu.f & FLAG_C) {
		state = 0x10;
	} else {
		alu_cp(gb, gb->cpu.a, 3);
		if (gb->cpu.f & FLAG_C) {
			state = 0x00;
		} else {
			alu_cp(gb, gb->cpu.a, 4);
			state = (gb->cpu.f & FLAG_C) ? 0x04 : 0x08;
		}
	}
	set_state(gb, (uint8_t)(state + (left ? 0 : 1)));
}

PORT_LOCAL(CoinFunc, FallLeftStart) { start_state(gb, 0x0e); fall(gb, true); }
PORT_LOCAL(CoinFunc, FallLeft) { fall(gb, true); }
PORT_LOCAL(CoinFunc, FallRightStart) { start_state(gb, 0x0f); fall(gb, false); }
PORT_LOCAL(CoinFunc, FallRight) { fall(gb, false); }

// .State10 / .State11: slides along the ground, slowing down
static void slide(GB *gb, bool left)
{
	uint16_t hl = wCurObjVar2;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		set_state(gb, 0x12);
		return;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x1f);
	if (!gb->cpu.a)
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	offset_yx(gb, -4, left ? -4 : 3);
	uint16_t base = left ? 0x40b : 0x48f;
	gb_call_ret(gb, ADDR_Func_3513, CF(base));
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	if (gb->cpu.a) {
		set_state(gb, left ? 0x11 : 0x10);
		return;
	}
	if (left)
		CALL(MoveObjectLeftByVar2);
	else
		CALL(MoveObjectRightByVar2);
	copy_pos(gb, wCurObjYPos);
	gb_call_ret(gb, ADDR_Func_3543, CF(base + 0x19));
	alu_cp(gb, gb->cpu.a, 0x10);
	uint8_t f = gb->cpu.f;
	store_y(gb);
	gb->cpu.f = f;
	if (f & FLAG_Z)
		return;
	offset_pos(gb, wCurObjXPos, left ? -4 : 3, hXPosLo, hXPosHi);
	gb_call_ret(gb, ADDR_Func_35a3, CF(base + 0x35));
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	offset_pos(gb, wCurObjXPos, left ? 3 : -4, hXPosLo, hXPosHi);
	gb_call_ret(gb, ADDR_Func_35a3, CF(base + 0x47));
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	set_state(gb, left ? 0x0c : 0x0d);
}

PORT_LOCAL(CoinFunc, State10) { slide(gb, true); }
PORT_LOCAL(CoinFunc, State11) { slide(gb, false); }

static void set_wait(GB *gb, uint8_t state)
{
	set_state(gb, state);
	gb->cpu.a = rd8(gb, wCurObjFramesetOffset);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a) ? 50 : 100;
	wr8(gb, wCurObjStateDuration, gb->cpu.a);
}

PORT_LOCAL(CoinFunc, State12) { set_wait(gb, 0x13); }
PORT_LOCAL(CoinFunc, State16) { set_wait(gb, 0x17); }

PORT_LOCAL(CoinFunc, State13)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d) {
		set_state(gb, 0x16);
		return;
	}
	copy_pos(gb, wCurObjYPos);
	gb_call_ret(gb, ADDR_Func_3543, CF(0x513));
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	set_state(gb, 0x14);
}

static void state18(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}

// Flickers, then disappears.
PORT_LOCAL(CoinFunc, State17)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		state18(gb);
		return;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	wr8(gb, hl, gb->cpu.a);
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x03);
	hl = lo_hl(gb, OBJ_FLAGS);
	uint8_t flags = rd8(gb, hl);
	if (gb->cpu.a)
		flags &= (uint8_t)~OBJFLAG_INVISIBLE;
	else
		flags = (uint8_t)((flags & ~OBJFLAG_ON_SCREEN) | OBJFLAG_INVISIBLE);
	gb->cpu.a = flags;
	wr8(gb, hl, flags);
}

PORT_LOCAL(CoinFunc, State18) { state18(gb); }

// Falls straight down.
static void state15(GB *gb)
{
	y_movement(gb, wCurObjVar3, 0x40, 0x40);
	copy_pos(gb, lo_hl(gb, OBJ_Y_POS));
	gb_call_ret(gb, ADDR_Func_352b, CF(0x599));
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		store_y(gb);
		set_state(gb, 0x12);
	}
	gb->cpu.a = rd8(gb, wIsInWaterOrSand);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	wr8(gb, wCurObjFramesetOffset, gb->cpu.a);
}

PORT_LOCAL(CoinFunc, State14) { start_state(gb, 0x15); state15(gb); }
PORT_LOCAL(CoinFunc, State15) { state15(gb); }
