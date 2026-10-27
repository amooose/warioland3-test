// Port of src/engine/level/objects/doughnuteer.asm
#include "ported.h"

PORT(DoughnutFunc_Destroy)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}

// Sets the doughnut to fly towards Wario.
PORT(DoughnutFunc)
{
	gb->cpu.a = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	SET_BC(gb, (gb->cpu.f & FLAG_C) ? ADDR_DoughnutFunc + 0x21 : ADDR_DoughnutFunc + 0x26); // .Update_Left/Right
	gb->cpu.a = gb->cpu.b;
	wr8(gb, wCurObjUpdateFunction + 1, gb->cpu.b);
	wr8(gb, wCurObjUpdateFunction + 0, gb->cpu.c);
	SET_HL(gb, wCurObjUpdateFunction);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
}

// DoughnutFunc_LeftRight and DoughnutFunc_Up.Update: destroys the doughnut
// or sends it at Wario depending on its state, else moves it with movement.
static void doughnut_update(GB *gb, uint16_t movement)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_DoughnutFunc_Destroy);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x19);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_DoughnutFunc);
		return;
	}
	SET_BC(gb, movement);
	JUMP(ApplyObjYMovement);
}

PORT_JUMP(DoughnutFunc_LeftRight)
{
	gb->cpu.l = OBJ_FLAGS;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	doughnut_update(gb, ADDR_Data_60510);
}

PORT_JUMP(DoughnutFunc_Up)
{
	uint16_t update = ADDR_DoughnutFunc_Up + 0x11; // .Update
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	hl = (uint16_t)((hl & 0xff00) | (uint8_t)((hl & 0xff) - 1)); // dec l
	gb->cpu.a = 0x8f;
	wr8(gb, hl, 0x8f);
	hl = (uint16_t)((hl & 0xff00) | OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	doughnut_update(gb, ADDR_Data_60540);
}

#define DE(off) ((uint16_t)(ADDR_DoughnuteerFunc + (off)))
#define DE_INIT DE(0x0a) // .Init
#define DE_UPDATE DE(0x1e) // .Update

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

static void set_state(GB *gb, uint8_t state)
{
	gb->cpu.a = state;
	wr8(gb, wCurObjState, state);
}

// ld hl, wCurObjState / ld a, state / ld [hld], a
static uint16_t state_hld(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	SET_HL(gb, hl);
	return hl;
}

// ld a, [hld] / rlca: the substate's direction into carry
static bool hdir_hld(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	return gb->cpu.f & FLAG_C;
}

// ld a, [hld] / rlca / frameset left (nc) or right (c) / call SetObjectFramesetPtr
static void dir_frameset(GB *gb, uint16_t left, uint16_t right)
{
	set_frameset(gb, hdir_hld(gb) ? right : left);
}

// ld l, OBJ_INTERACTION_TYPE / ld a, [hl] / and HEAVY_OBJ / or type / ld [hld], a
static void set_interaction(GB *gb, uint8_t type)
{
	uint16_t hl = lo_hl(gb, OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), type);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// wCurObjScreenXPos + $2a compared with wWarioScreenXPos + $2a: carry if
// the object is left of Wario
static bool left_of_wario(GB *gb)
{
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	return gb->cpu.f & FLAG_C;
}

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

// wGroundShakeCounter >= $10: sets state and returns true
static bool ground_shake(GB *gb, uint8_t state)
{
	gb->cpu.a = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (gb->cpu.f & FLAG_C)
		return false;
	set_state(gb, state);
	return true;
}

// dec [hl] at hl: true if it reached 0
static bool dec_hl(GB *gb, uint16_t hl)
{
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	return !d;
}

// [wCurObjPos] + dy, dx into hYPos / hXPos (sub/sbc or add/adc)
static void offset_pos(GB *gb, int dy, int dx)
{
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), (uint8_t)-dy);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0x00);
	wr8(gb, hYPosHi, gb->cpu.a);
	if (dx >= 0) {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), (uint8_t)dx);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0x00);
	} else {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), (uint8_t)-dx);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0x00);
	}
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

// Walks on the ground: falls off ledges (state $11), stops at walls
// (blocked). rets: where the asm's calls of Func_3543, ProcessBlockOnLeft,
// Func_3513 (left), ProcessBlockOnRight and Func_3513 (right) return.
static void walk(GB *gb, const uint16_t rets[5], bool slow, void (*blocked)(GB *gb, bool right))
{
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3543, rets[0]);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		set_state(gb, 0x11);
		RET();
		return;
	}
	hl = wCurObjYPos;
	gb->cpu.a = rd8(gb, hYPosLo);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	bool right = gb->cpu.f & FLAG_C;
	if (!right) {
		gb->cpu.a = alu_sub(gb, alu_and(gb, rd8(gb, hl), 0x0f), 0x06);
		if (gb->cpu.f & FLAG_C) {
			gb_call_ret(gb, ADDR_ProcessBlockOnLeft, rets[1]);
			gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
			if (!gb->cpu.a) {
				blocked(gb, false);
				return;
			}
		}
		offset_pos(gb, -4, -6);
		gb_call_ret(gb, ADDR_Func_3513, rets[2]);
	} else {
		gb->cpu.a = alu_add(gb, alu_and(gb, rd8(gb, hl), 0x0f), 0x05);
		alu_cp(gb, gb->cpu.a, 0x10);
		if (!(gb->cpu.f & FLAG_C)) {
			gb_call_ret(gb, ADDR_ProcessBlockOnRight, rets[3]);
			gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
			if (!gb->cpu.a) {
				blocked(gb, true);
				return;
			}
		}
		offset_pos(gb, -4, 5);
		gb_call_ret(gb, ADDR_Func_3513, rets[4]);
	}
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		if (right) {
			if (slow)
				JUMP(MoveObjectRight_Slow);
			else
				JUMP(MoveObjectRight);
		} else {
			if (slow)
				JUMP(MoveObjectLeft_Slow);
			else
				JUMP(MoveObjectLeft);
		}
		return;
	}
	blocked(gb, right);
}

PORT(DoughnuteerFunc)
{
	store_update(gb, wCurObjUpdateFunction + 1, DE_INIT);
}

// Turns to face Wario and walks.
static void func_41cdb(GB *gb)
{
	uint16_t hl = wCurObjInteractionType;
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_01);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	hl = lo_hl(gb, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x05);
	wr8(gb, hl--, 0xfa);
	gb->cpu.a = 0xff;
	wr8(gb, hl, 0xff);
	SET_HL(gb, hl);
	hl = lo_hl(gb, OBJ_STATE);
	gb->cpu.a = 0x30;
	wr8(gb, hl--, 0x30);
	SET_HL(gb, hl);
	bool left = left_of_wario(gb);
	set_bits(gb, hl, left ? OBJSUBFLAG_HDIR : 0, left ? 0 : OBJSUBFLAG_HDIR);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	set_frameset(gb, left ? ADDR_Frameset_6856b : ADDR_Frameset_68562);
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Init)
{
	uint16_t hl = wCurObjFlags;
	set_bits(gb, hl, 0, OBJFLAG_INVISIBLE);
	store_update(gb, (uint16_t)((hl & 0xff00) | (OBJ_UPDATE_FUNCTION + 1)), DE_UPDATE);
	hl = lo_hl(gb, OBJ_SUBSTATE);
	set_bits(gb, hl, 0, OBJSUBFLAG_UNINITIALISED);
	func_41cdb(gb);
	RET();
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Update)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	JUMP_TO(jumptable(gb, DE(0x22)));
}

PORT_LOCAL(DoughnuteerFunc, Func_41cdb) { func_41cdb(gb); }

// .asm_41bcf etc.: turns around
static void turn(GB *gb, uint16_t hl)
{
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	hl = lo_hl(gb, OBJ_SUBSTATE);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl));
	if (gb->cpu.f & FLAG_C)
		set_bits(gb, hl, 0, OBJSUBFLAG_HDIR);
	else
		set_bits(gb, hl, OBJSUBFLAG_HDIR, 0);
	RET();
}

// Throws a doughnut.
PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41b58)
{
	if (ground_shake(gb, 0x1a)) {
		RET();
		return;
	}
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x1a);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_41b8e
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		hl = lo_hl(gb, OBJ_FLAGS);
		if (alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F))
			play_sfx(gb, SFX_067);
		gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
		SET_BC(gb, (gb->cpu.f & FLAG_C) ? ADDR_ObjParams_DoughnutRight : ADDR_ObjParams_DoughnutLeft);
		JUMP(CreateObjectAtRelativePos);
		return;
	}
	if (!dec_hl(gb, hl)) {
		JUMP(Func_3308);
		return;
	}
	hl = lo_hl(gb, OBJ_STATE);
	gb->cpu.a = 0x2b;
	wr8(gb, hl--, 0x2b);
	SET_HL(gb, hl);
	dir_frameset(gb, ADDR_Frameset_685fe, ADDR_Frameset_68605);
	ld_hli(gb, 0x1a);
	RET();
}

// state duration 8: turns around, else waits then (state) with frameset
static void wait_turn(GB *gb, uint8_t state, uint16_t left, uint16_t right, int duration)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (gb->cpu.f & FLAG_Z) {
		turn(gb, hl);
		return;
	}
	if (!dec_hl(gb, hl)) {
		JUMP(Func_3308);
		return;
	}
	hl = lo_hl(gb, OBJ_STATE);
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	SET_HL(gb, hl);
	if (duration < 0) {
		SET_DE(gb, hdir_hld(gb) ? right : left);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	dir_frameset(gb, left, right);
	ld_hli(gb, (uint8_t)duration);
	RET();
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41bae)
{
	wait_turn(gb, 0x5b, ADDR_Frameset_68574, ADDR_Frameset_6857d, -1);
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41c8b)
{
	wait_turn(gb, 0x5c, ADDR_Frameset_685c4, ADDR_Frameset_685d7, 0x37);
}

static void blocked_2c(GB *gb, bool right)
{
	set_state(gb, 0x2c);
	set_frameset(gb, right ? ADDR_Frameset_68605 : ADDR_Frameset_685fe);
	ld_hli(gb, 0x1a);
	RET();
}

static void blocked_13(GB *gb, bool right)
{
	(void)right;
	set_state(gb, 0x13);
	RET();
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41bdc)
{
	if (ground_shake(gb, 0x1c)) {
		RET();
		return;
	}
	static const uint16_t rets[5] = {
		ADDR_DoughnuteerFunc + 0x17f, ADDR_DoughnuteerFunc + 0x1a1, ADDR_DoughnuteerFunc + 0x1bf,
		ADDR_DoughnuteerFunc + 0x1de, ADDR_DoughnuteerFunc + 0x1fc,
	};
	walk(gb, rets, false, blocked_2c);
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41cc1)
{
	if (ground_shake(gb, 0x1c)) {
		RET();
		return;
	}
	if (!dec_hl(gb, wCurObjStateDuration)) {
		JUMP(Func_3308);
		return;
	}
	set_state(gb, 0x01);
	RET();
}

PORT_LOCAL(DoughnuteerFunc, Func_41d1a)
{
	state_hld(gb, 0x41);
	bool c = hdir_hld(gb);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	set_frameset(gb, c ? ADDR_Frameset_6856b : ADDR_Frameset_68562);
}

PORT_LOCAL(DoughnuteerFunc, Func_41d36)
{
	uint16_t hl = wCurObjInteractionType;
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_01);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	hl = lo_hl(gb, OBJ_STATE);
	gb->cpu.a = 0x31;
	wr8(gb, hl--, 0x31);
	SET_HL(gb, hl);
	dir_frameset(gb, ADDR_Frameset_68562, ADDR_Frameset_6856b);
}

// Walks towards Wario; gets ready to throw when he's close at about its
// height.
PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41d56)
{
	if (ground_shake(gb, 0x1a)) {
		RET();
		return;
	}
	gb->cpu.c = 0x2a;
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenYPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), 0x2a);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	bool close = true;
	alu_cp(gb, gb->cpu.a, 0x20);
	if (!(gb->cpu.f & FLAG_C)) {
		alu_cp(gb, gb->cpu.a, 0xe0);
		close = !(gb->cpu.f & FLAG_C);
	}
	if (close) {
		// .asm_41d77
		uint16_t hl = wCurObjSubState;
		SET_HL(gb, hl);
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		alu_cp(gb, gb->cpu.a, 0xc0);
		uint16_t frameset = 0;
		bool aim = false;
		if (!(gb->cpu.f & FLAG_C)) {
			// .asm_41d99
			gb->cpu.a = alu_rlca(gb, rd8(gb, hl++));
			SET_HL(gb, hl);
			aim = true;
			frameset = (gb->cpu.f & FLAG_C) ? ADDR_Frameset_68597 : 0;
		} else {
			alu_cp(gb, gb->cpu.a, 0x40);
			if (gb->cpu.f & FLAG_C) {
				gb->cpu.a = alu_rlca(gb, rd8(gb, hl++));
				SET_HL(gb, hl);
				aim = true;
				frameset = (gb->cpu.f & FLAG_C) ? 0 : ADDR_Frameset_68586;
			}
		}
		if (aim) {
			if (!frameset) {
				// .asm_41d95
				gb->cpu.a = 0x13;
				wr8(gb, hl, 0x13);
				RET();
				return;
			}
			// .asm_41da0
			SET_DE(gb, frameset);
			gb->cpu.a = 0x5a;
			wr8(gb, hl, 0x5a);
			CALL(SetObjectFramesetPtr);
			ld_hli(gb, 0x33);
			RET();
			return;
		}
	}
	static const uint16_t rets[5] = {
		ADDR_DoughnuteerFunc + 0x340, ADDR_DoughnuteerFunc + 0x362, ADDR_DoughnuteerFunc + 0x380,
		ADDR_DoughnuteerFunc + 0x396, ADDR_DoughnuteerFunc + 0x3b4,
	};
	walk(gb, rets, true, blocked_13);
}

// .asm_41e93: bumped
static void bumped(GB *gb, uint8_t state)
{
	state_hld(gb, state);
	dir_frameset(gb, ADDR_Frameset_685a8, ADDR_Frameset_685ad);
	gb->cpu.a = 0x0c;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl--, 0x0c);
	SET_HL(gb, hl);
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjVar2, 0x02);
}

// Bumped; throws a doughnut up unless the frame is in [6, $24).
static void bumped_throw(GB *gb, uint16_t params, uint8_t state)
{
	gb->cpu.a = rd8(gb, wCurObjFrame);
	bool create = true;
	alu_cp(gb, gb->cpu.a, 0x24);
	if (gb->cpu.f & FLAG_C) {
		alu_cp(gb, gb->cpu.a, 0x06);
		create = gb->cpu.f & FLAG_C;
	}
	if (create) {
		SET_BC(gb, params);
		CALL(CreateObjectAtRelativePos);
	}
	bumped(gb, state);
}

PORT_LOCAL(DoughnuteerFunc, Func_41e3a) { bumped_throw(gb, ADDR_ObjParams_DoughnutUpLeft, 0x32); }
PORT_LOCAL(DoughnuteerFunc, Func_41e7c) { bumped_throw(gb, ADDR_ObjParams_DoughnutUpRight, 0x33); }

// Bumped back, then gets ready to throw again.
static void bump_back(GB *gb, bool right)
{
	uint16_t hl = wCurObjVar2;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		if (right)
			JUMP(ObjState_BumpRight);
		else
			JUMP(ObjState_BumpLeft);
		return;
	}
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	if (!dec_hl(gb, REG_HL(gb))) {
		RET();
		return;
	}
	hl = lo_hl(gb, OBJ_SUBSTATE);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl++));
	SET_HL(gb, hl);
	bool c = gb->cpu.f & FLAG_C;
	if (c != right) {
		// $5b
		gb->cpu.a = 0x5b;
		wr8(gb, hl, 0x5b);
		set_frameset(gb, c ? ADDR_Frameset_6857d : ADDR_Frameset_68574);
	} else {
		gb->cpu.a = 0x2b;
		wr8(gb, hl, 0x2b);
		set_frameset(gb, c ? ADDR_Frameset_68605 : ADDR_Frameset_685fe);
		ld_hli(gb, 0x1a);
	}
	RET();
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41e53) { bump_back(gb, false); }
PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41eae) { bump_back(gb, true); }

// Sliding after an attack, while var2 isn't 0.
static void slide(GB *gb, bool right)
{
	uint16_t hl = wCurObjVar2;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		if (right)
			JUMP(Func_32bd);
		else
			JUMP(Func_32ae);
		return;
	}
	set_state(gb, 0x0f);
	RET();
}

static void start_slide(GB *gb, uint8_t state)
{
	state_hld(gb, state);
	dir_frameset(gb, ADDR_Frameset_685b2, ADDR_Frameset_685bb);
	set_interaction(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41ed7)
{
	start_slide(gb, 0x46);
	slide(gb, false);
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41ef4) { slide(gb, false); }

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41f02)
{
	start_slide(gb, 0x47);
	slide(gb, true);
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41f1f) { slide(gb, true); }

static void func_41f3b(GB *gb, uint8_t state)
{
	state_hld(gb, state);
	dir_frameset(gb, ADDR_Frameset_685b2, ADDR_Frameset_685bb);
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	ld_hli(gb, 0x02);
	ld_hli(gb, 0x00);
	gb->cpu.f = FLAG_Z; // xor a
	set_interaction(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL(DoughnuteerFunc, Func_41f2d) { func_41f3b(gb, 0x44); }
PORT_LOCAL(DoughnuteerFunc, Func_41f35) { func_41f3b(gb, 0x45); }

// Knocked over.
static void func_41f6c(GB *gb)
{
	if (ground_shake(gb, 0x1c)) {
		RET();
		return;
	}
	if (!dec_hl(gb, wCurObjStateDuration)) {
		JUMP(Func_32ea);
		return;
	}
	set_state(gb, 0x10);
	RET();
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41f5d)
{
	set_state(gb, 0x3f);
	gb->cpu.a = 0x64;
	wr8(gb, wCurObjStateDuration, 0x64);
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	set_bits(gb, hl, 0, OBJFLAG_GRABBED);
	func_41f6c(gb);
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41f6c) { func_41f6c(gb); }

// Gets up.
static void func_41f94(GB *gb)
{
	if (!dec_hl(gb, wCurObjStateDuration)) {
		JUMP(Func_32f9);
		return;
	}
	set_state(gb, 0x00);
	RET();
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41f86)
{
	state_hld(gb, 0x40);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	gb->cpu.a = 0x07;
	wr8(gb, wCurObjStateDuration, 0x07);
	func_41f94(gb);
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41f94) { func_41f94(gb); }

PORT_LOCAL(DoughnuteerFunc, Func_41fa1)
{
	state_hld(gb, 0x43);
	dir_frameset(gb, ADDR_Frameset_685f0, ADDR_Frameset_685f7);
	ld_hli(gb, 0x1a);
}

// Looks around.
PORT_LOCAL_JUMP(DoughnuteerFunc, Func_41fbd)
{
	if (ground_shake(gb, 0x1a)) {
		RET();
		return;
	}
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (gb->cpu.f & FLAG_Z) {
		turn(gb, hl);
		return;
	}
	if (!dec_hl(gb, hl)) {
		JUMP(Func_3308);
		return;
	}
	set_state(gb, 0x01);
	RET();
}

// .asm_41ffb: stunned
static void stunned(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjAction;
	gb->cpu.a = 0x8f;
	wr8(gb, hl--, 0x8f);
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	SET_HL(gb, hl);
	dir_frameset(gb, ADDR_Frameset_685b2, ADDR_Frameset_685bb);
	ld_hli(gb, 0x04);
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	ld_hli(gb, 0x02);
	ld_hli(gb, 0x00);
	gb->cpu.f = FLAG_Z; // xor a
	set_interaction(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL(DoughnuteerFunc, Func_41fe9) { stunned(gb, 0x38); }
PORT_LOCAL(DoughnuteerFunc, Func_41ff3) { stunned(gb, 0x39); }

// .asm_42046: knocked away
static void knock(GB *gb, uint8_t state)
{
	uint16_t hl = state_hld(gb, state);
	gb->cpu.b = 0x02;
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
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_685bb : ADDR_Frameset_685b2);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_42020) { knock(gb, 0x59); }
PORT_LOCAL_JUMP(DoughnuteerFunc, Func_4202a) { knock(gb, 0x58); }
PORT_LOCAL_JUMP(DoughnuteerFunc, Func_42034) { knock(gb, 0x34); }
PORT_LOCAL_JUMP(DoughnuteerFunc, Func_4203e) { knock(gb, 0x35); }

// Thrown up.
static void thrown(GB *gb, uint8_t state, uint8_t var2)
{
	state_hld(gb, state);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl--, 0x41);
	wr8(gb, hl--, var2);
	gb->cpu.a = 0x01;
	wr8(gb, hl--, 0x01);
	SET_HL(gb, hl);
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_42064)
{
	thrown(gb, 0x36, 0xc0);
	JUMP(Func_3335);
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_42077)
{
	thrown(gb, 0x37, 0xe0);
	JUMP(Func_3344);
}

// Grabbed: .Func_4208a ($49) and .Func_420b6 ($48) turn with Wario.
static void grab(GB *gb, bool right)
{
	for (;;) {
		state_hld(gb, right ? 0x49 : 0x48);
		dir_frameset(gb, ADDR_Frameset_685b2, ADDR_Frameset_685bb);
		set_bits(gb, lo_hl(gb, OBJ_FLAGS), OBJFLAG_GRABBED, 0);
		gb->cpu.a = rd8(gb, wDirection);
		bool dir = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
		if (right ? dir : !dir) {
			if (right)
				JUMP(Func_3353);
			else
				JUMP(Func_3362);
			return;
		}
		uint16_t hl = wCurObjSubState;
		SET_HL(gb, hl);
		gb->cpu.a = alu_xor(gb, rd8(gb, hl), 0x80);
		wr8(gb, hl, gb->cpu.a);
		right = !right;
	}
}

static void hold(GB *gb, bool right)
{
	gb->cpu.a = rd8(gb, wDirection);
	bool dir = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	if (right ? dir : !dir) {
		if (right)
			JUMP(Func_3353);
		else
			JUMP(Func_3362);
		return;
	}
	uint16_t hl = wCurObjSubState;
	SET_HL(gb, hl);
	gb->cpu.a = alu_xor(gb, rd8(gb, hl), 0x80);
	wr8(gb, hl, gb->cpu.a);
	grab(gb, !right);
}

PORT_LOCAL_JUMP(DoughnuteerFunc, Func_4208a) { grab(gb, true); }
PORT_LOCAL_JUMP(DoughnuteerFunc, Func_420a6) { hold(gb, true); }
PORT_LOCAL_JUMP(DoughnuteerFunc, Func_420b6) { grab(gb, false); }
PORT_LOCAL_JUMP(DoughnuteerFunc, Func_420d2) { hold(gb, false); }

PORT_LOCAL(DoughnuteerFunc, Func_420e3)
{
	uint16_t hl = wCurObjAction;
	gb->cpu.a = 0x9f;
	wr8(gb, hl--, 0x9f);
	gb->cpu.a = 0x42;
	wr8(gb, hl--, 0x42);
	SET_HL(gb, hl);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	set_interaction(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL_JUMP(DoughnutFunc_Up, Update) { doughnut_update(gb, ADDR_Data_60540); }

static void doughnut_fly(GB *gb)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	SET_BC(gb, ADDR_Data_60040);
	JUMP(ApplyObjYMovement);
}

PORT_LOCAL_JUMP(DoughnutFunc, Update_Left)
{
	CALL(MoveObjectLeft);
	doughnut_fly(gb);
}

PORT_LOCAL_JUMP(DoughnutFunc, Update_Right)
{
	CALL(MoveObjectRight);
	doughnut_fly(gb);
}

PORT_JUMP(DoughnutFunc_Left)
{
	CALL(MoveObjectLeft_Fast);
	JUMP_TO(ADDR_DoughnutFunc_LeftRight);
}

PORT_JUMP(DoughnutFunc_Right)
{
	CALL(MoveObjectRight_Fast);
	JUMP_TO(ADDR_DoughnutFunc_LeftRight); // fallthrough
}
