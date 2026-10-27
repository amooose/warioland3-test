// Port of src/engine/level/objects/brrr_bear.asm
#include "ported.h"

#define SNOWFLAKE_UPDATE (ADDR_SnowflakeFunc + 0x1c) // .Update

PORT_JUMP(SnowflakeFunc_Left)
{
	CALL(MoveObjectLeft_Fast);
	JUMP_TO(ADDR_SnowflakeFunc);
}

PORT_JUMP(SnowflakeFunc_Right)
{
	CALL(MoveObjectRight_Fast);
	JUMP_TO(ADDR_SnowflakeFunc); // fallthrough
}

// Once its state changes, the snowflake shatters.
PORT(SnowflakeFunc)
{
	SET_HL(gb, wCurObjFlags);
	wr8(gb, wCurObjFlags, (uint8_t)(rd8(gb, wCurObjFlags) | OBJFLAG_NO_COLLISION));
	gb->cpu.a = rd8(gb, wCurObjState);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	SET_DE(gb, ADDR_Frameset_6916e);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 0x0c;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, 0x0c); // OBJ_STATE_DURATION
	hl = (uint16_t)((hl & 0xff00) | (OBJ_UPDATE_FUNCTION + 1));
	gb->cpu.a = (uint8_t)(SNOWFLAKE_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)SNOWFLAKE_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

#define BB(off) ((uint16_t)(ADDR_BrrrBearFunc + (off)))
#define BRRR_INIT BB(0x0a) // .Init
#define BRRR_UPDATE BB(0x29) // .Update

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

// A strong ground shake knocks it over: state 8 or 9 at random.
static bool ground_shake(GB *gb)
{
	gb->cpu.a = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (gb->cpu.f & FLAG_C)
		return false;
	gb->cpu.a = alu_rra(gb, rd8(gb, rDIV));
	gb->cpu.a = alu_rla(gb, 0x04);
	wr8(gb, wCurObjState, gb->cpu.a);
	return true;
}

PORT(BrrrBearFunc)
{
	store_update(gb, wCurObjUpdateFunction + 1, BRRR_INIT);
}

// Turns to face Wario and walks.
static void func_4811f(GB *gb)
{
	uint16_t hl = wCurObjInteractionType;
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_01);
	wr8(gb, hl--, gb->cpu.a);
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
	set_frameset(gb, left ? ADDR_Frameset_69126 : ADDR_Frameset_69115);
}

PORT_LOCAL_JUMP(BrrrBearFunc, Init)
{
	uint16_t hl = wCurObjFlags;
	set_bits(gb, hl, 0, OBJFLAG_INVISIBLE);
	store_update(gb, (uint16_t)((hl & 0xff00) | (OBJ_UPDATE_FUNCTION + 1)), BRRR_UPDATE);
	hl = lo_hl(gb, OBJ_SUBSTATE);
	set_bits(gb, hl, 0, OBJSUBFLAG_UNINITIALISED);
	hl = lo_hl(gb, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x08);
	wr8(gb, hl--, 0xf7);
	gb->cpu.a = 0xf9;
	wr8(gb, hl, 0xf9);
	SET_HL(gb, hl);
	func_4811f(gb);
	RET();
}

PORT_LOCAL_JUMP(BrrrBearFunc, Update)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	JUMP_TO(jumptable(gb, BB(0x2d)));
}

PORT_LOCAL(BrrrBearFunc, Func_4811f) { func_4811f(gb); }

// .asm_48407: stunned (state in a)
static void stunned(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjAction;
	gb->cpu.a = 0x8f;
	wr8(gb, hl--, 0x8f);
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	SET_HL(gb, hl);
	set_frameset(gb, ADDR_Frameset_6915b);
	ld_hli(gb, 0x04);
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	ld_hli(gb, 0x02);
	ld_hli(gb, 0x00);
	gb->cpu.f = FLAG_Z; // xor a
	set_interaction(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL(BrrrBearFunc, Func_483f5) { stunned(gb, 0x38); }
PORT_LOCAL(BrrrBearFunc, Func_483ff) { stunned(gb, 0x39); }

// Vanishes unless Wario can be bumped by it.
PORT_LOCAL_JUMP(BrrrBearFunc, Func_480e7)
{
	gb->cpu.a = rd8(gb, wCurObjAction);
	bool vanish = !alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!vanish) {
		gb->cpu.a = rd8(gb, wTransformation);
		vanish = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	}
	if (!vanish) {
		gb->cpu.a = rd8(gb, wEffectivePowerUp);
		alu_cp(gb, gb->cpu.a, 0x06);
		vanish = !(gb->cpu.f & FLAG_C);
	}
	if (!vanish) {
		gb->cpu.a = alu_and(gb, rd8(gb, wCurObjInteractionType), 0x7f);
		vanish = !gb->cpu.a;
	}
	if (vanish) {
		JUMP(VanishObject2);
		return;
	}
	play_sfx(gb, SFX_014);
	stunned(gb, left_of_wario(gb) ? 0x39 : 0x38);
	RET();
}

PORT_LOCAL(BrrrBearFunc, Func_48153)
{
	state_hld(gb, 0x41);
	bool c = hdir_hld(gb);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	set_frameset(gb, c ? ADDR_Frameset_69126 : ADDR_Frameset_69115);
}

// Blows snowflakes.
PORT_LOCAL_JUMP(BrrrBearFunc, Func_4816f)
{
	if (ground_shake(gb)) {
		RET();
		return;
	}
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d) {
		set_state(gb, 0x13);
		RET();
		return;
	}
	gb->cpu.a = d;
	alu_cp(gb, d, 0x22);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	play_sfx(gb, SFX_07E);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	SET_BC(gb, (gb->cpu.f & FLAG_C) ? ADDR_ObjParams_SnowflakeRight : ADDR_ObjParams_SnowflakeLeft);
	JUMP(CreateObjectAtRelativePos);
}

PORT_LOCAL(BrrrBearFunc, Func_481aa)
{
	uint16_t hl = wCurObjInteractionType;
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_01);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	hl = lo_hl(gb, OBJ_STATE);
	gb->cpu.a = 0x31;
	wr8(gb, hl--, 0x31);
	SET_HL(gb, hl);
	set_frameset(gb, hdir_hld(gb) ? ADDR_Frameset_69126 : ADDR_Frameset_69115);
}

static void set_state_13(GB *gb)
{
	set_state(gb, 0x13);
	RET();
}

// Walks towards Wario; starts blowing when he's close at about its height.
PORT_LOCAL_JUMP(BrrrBearFunc, Func_481ca)
{
	if (ground_shake(gb)) {
		RET();
		return;
	}
	gb->cpu.c = 0x2a;
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenYPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), 0x2a);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	bool close = true;
	alu_cp(gb, gb->cpu.a, 0x18);
	if (!(gb->cpu.f & FLAG_C)) {
		alu_cp(gb, gb->cpu.a, 0xe8);
		close = !(gb->cpu.f & FLAG_C);
	}
	if (close) {
		// .asm_481ef
		uint16_t hl = wCurObjSubState;
		SET_HL(gb, hl);
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		alu_cp(gb, gb->cpu.a, 0xb8);
		uint16_t frameset = 0;
		bool blow = false;
		if (!(gb->cpu.f & FLAG_C)) {
			// .asm_48211: Wario on the left
			gb->cpu.a = alu_rlca(gb, rd8(gb, hl++));
			SET_HL(gb, hl);
			blow = true;
			if (!(gb->cpu.f & FLAG_C))
				frameset = 0; // facing right: .asm_4820d
			else
				frameset = ADDR_Frameset_6914c;
		} else {
			alu_cp(gb, gb->cpu.a, 0x48);
			if (gb->cpu.f & FLAG_C) {
				gb->cpu.a = alu_rlca(gb, rd8(gb, hl++));
				SET_HL(gb, hl);
				blow = true;
				frameset = (gb->cpu.f & FLAG_C) ? 0 : ADDR_Frameset_6913d;
			}
		}
		if (blow) {
			if (!frameset) {
				// .asm_4820d
				gb->cpu.a = 0x13;
				wr8(gb, hl, 0x13);
				RET();
				return;
			}
			// .asm_48218
			SET_DE(gb, frameset);
			gb->cpu.a = 0x2d;
			wr8(gb, hl, 0x2d);
			CALL(SetObjectFramesetPtr);
			ld_hli(gb, 0x39);
			RET();
			return;
		}
	}
	// .asm_48222
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3543, BB(0x234));
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		set_state(gb, 0x11);
		RET();
		return;
	}
	// .asm_4823d
	hl = wCurObjYPos;
	gb->cpu.a = rd8(gb, hYPosLo);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = alu_sub(gb, alu_and(gb, rd8(gb, hl), 0x0f), 0x08);
		if (gb->cpu.f & FLAG_C) {
			gb_call_ret(gb, ADDR_ProcessBlockOnLeft, BB(0x256));
			gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
			if (!gb->cpu.a) {
				set_state_13(gb);
				return;
			}
		}
		// .asm_4825a
		hl = wCurObjYPos;
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x04);
		wr8(gb, hYPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0x00);
		wr8(gb, hYPosHi, gb->cpu.a);
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x08);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0x00);
		wr8(gb, hXPosHi, gb->cpu.a);
		SET_HL(gb, hl);
		gb_call_ret(gb, ADDR_Func_3513, BB(0x274));
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			JUMP(MoveObjectLeft_Slow);
			return;
		}
		set_state_13(gb);
		return;
	}
	// .asm_4827e
	gb->cpu.a = alu_add(gb, alu_and(gb, rd8(gb, hl), 0x0f), 0x07);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (!(gb->cpu.f & FLAG_C)) {
		gb_call_ret(gb, ADDR_ProcessBlockOnRight, BB(0x28a));
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
		if (!gb->cpu.a) {
			set_state_13(gb);
			return;
		}
	}
	// .asm_4828e
	hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x04);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0x00);
	wr8(gb, hYPosHi, gb->cpu.a);
	gb->cpu.a = alu_add(gb, rd8(gb, hl++), 0x07);
	wr8(gb, hXPosLo, gb->cpu.a);
	gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0x00);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3513, BB(0x2a8));
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP(MoveObjectRight_Slow);
		return;
	}
	set_state_13(gb);
}

// Bumped.
static void bumped(GB *gb, uint8_t state)
{
	state_hld(gb, state);
	bool c = hdir_hld(gb);
	set_frameset(gb, c ? ADDR_Frameset_6913a : ADDR_Frameset_69137);
	gb->cpu.a = 0x0c;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl--, 0x0c);
	SET_HL(gb, hl);
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjVar2, 0x02);
	set_interaction(gb, OBJ_INTERACTION_01);
}

PORT_LOCAL(BrrrBearFunc, Func_482b2) { bumped(gb, 0x32); }
PORT_LOCAL(BrrrBearFunc, Func_482ba) { bumped(gb, 0x33); }

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
	set_state(gb, state);
	set_frameset(gb, ADDR_Frameset_6915b);
	set_interaction(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL_JUMP(BrrrBearFunc, Func_482e0)
{
	start_slide(gb, 0x46);
	slide(gb, false);
}

PORT_LOCAL_JUMP(BrrrBearFunc, Func_482f3) { slide(gb, false); }

PORT_LOCAL_JUMP(BrrrBearFunc, Func_48301)
{
	start_slide(gb, 0x47);
	slide(gb, true);
}

PORT_LOCAL_JUMP(BrrrBearFunc, Func_48314) { slide(gb, true); }

static void func_48328(GB *gb, uint8_t state)
{
	set_state(gb, state);
	set_frameset(gb, ADDR_Frameset_6915b);
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	ld_hli(gb, 0x02);
	ld_hli(gb, 0x00);
	gb->cpu.f = FLAG_Z; // xor a
	set_interaction(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL(BrrrBearFunc, Func_48322) { func_48328(gb, 0x44); }
PORT_LOCAL(BrrrBearFunc, Func_48326) { func_48328(gb, 0x45); }

// Knocked over.
static void func_48350(GB *gb)
{
	gb->cpu.a = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (!(gb->cpu.f & FLAG_C)) {
		set_state(gb, 0x1c);
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
	set_state(gb, 0x10);
	RET();
}

PORT_LOCAL_JUMP(BrrrBearFunc, Func_48341)
{
	set_state(gb, 0x3f);
	gb->cpu.a = 0x64;
	wr8(gb, wCurObjStateDuration, 0x64);
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	set_bits(gb, hl, 0, OBJFLAG_GRABBED);
	func_48350(gb);
}

PORT_LOCAL_JUMP(BrrrBearFunc, Func_48350) { func_48350(gb); }

// Gets up, facing Wario.
PORT_LOCAL(BrrrBearFunc, Func_4836a)
{
	uint16_t hl = state_hld(gb, 0x40);
	if (left_of_wario(gb)) {
		SET_DE(gb, ADDR_Frameset_6918e);
		set_bits(gb, hl, OBJSUBFLAG_HDIR, 0);
	} else {
		SET_DE(gb, ADDR_Frameset_69183);
		set_bits(gb, hl, 0, OBJSUBFLAG_HDIR);
	}
	CALL(SetObjectFramesetPtr);
	ld_hli(gb, 0x28);
	set_interaction(gb, OBJ_INTERACTION_01);
}

PORT_LOCAL(BrrrBearFunc, Func_48399)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	set_state(gb, 0x01);
}

PORT_LOCAL(BrrrBearFunc, Func_483a4)
{
	state_hld(gb, 0x43);
	bool c = hdir_hld(gb);
	set_frameset(gb, c ? ADDR_Frameset_6917c : ADDR_Frameset_69175);
	ld_hli(gb, 0x16);
	set_interaction(gb, OBJ_INTERACTION_01);
}

// Looks around.
PORT_LOCAL_JUMP(BrrrBearFunc, Func_483c5)
{
	if (ground_shake(gb)) {
		RET();
		return;
	}
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x06);
	if (gb->cpu.f & FLAG_Z) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		hl = lo_hl(gb, OBJ_SUBSTATE);
		gb->cpu.a = alu_rlca(gb, rd8(gb, hl));
		if (gb->cpu.f & FLAG_C)
			set_bits(gb, hl, 0, OBJSUBFLAG_HDIR);
		else
			set_bits(gb, hl, OBJSUBFLAG_HDIR, 0);
		RET();
		return;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		JUMP(Func_3308);
		return;
	}
	set_state(gb, 0x01);
	RET();
}

// .asm_4847a: hl = substate, b = speed
static void knocked(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), 0xf0);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	set_frameset(gb, ADDR_Frameset_69164);
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
}

static void knock(GB *gb, uint8_t state, uint8_t speed)
{
	state_hld(gb, state);
	gb->cpu.b = speed;
	knocked(gb);
}

PORT_LOCAL(BrrrBearFunc, Func_48420) { knock(gb, 0x59, 0x02); }
PORT_LOCAL(BrrrBearFunc, Func_4842a) { knock(gb, 0x58, 0x02); }

// Hit: stunned if Wario is normal and small enough, else knocked away.
static void hit(GB *gb, uint8_t stun_state, uint8_t knock_state)
{
	gb->cpu.a = rd8(gb, wTransformation);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = rd8(gb, wEffectivePowerUp);
		alu_cp(gb, gb->cpu.a, 0x05);
		if (gb->cpu.f & FLAG_C) {
			gb->cpu.a = alu_and(gb, rd8(gb, wCurObjInteractionType), 0x7f);
			if (gb->cpu.a) {
				set_state(gb, stun_state);
				return;
			}
		}
	}
	knock(gb, knock_state, 0x01);
}

PORT_LOCAL(BrrrBearFunc, Func_48434) { hit(gb, 0x08, 0x34); }
PORT_LOCAL(BrrrBearFunc, Func_48458) { hit(gb, 0x09, 0x35); }

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

PORT_LOCAL_JUMP(BrrrBearFunc, Func_4848e)
{
	thrown(gb, 0x36, 0xc0);
	JUMP(Func_3335);
}

PORT_LOCAL_JUMP(BrrrBearFunc, Func_484a1)
{
	thrown(gb, 0x37, 0xe0);
	JUMP(Func_3344);
}

// Grabbed: .Func_484b4 ($49) and .Func_484d3 ($48) turn with Wario.
static void grab(GB *gb, bool right)
{
	for (;;) {
		set_state(gb, right ? 0x49 : 0x48);
		set_frameset(gb, ADDR_Frameset_6915b);
		set_bits(gb, lo_hl(gb, OBJ_FLAGS), OBJFLAG_GRABBED, 0);
		// .Func_484c3 / .Func_484e2
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

PORT_LOCAL_JUMP(BrrrBearFunc, Func_484b4) { grab(gb, true); }
PORT_LOCAL_JUMP(BrrrBearFunc, Func_484c3) { hold(gb, true); }
PORT_LOCAL_JUMP(BrrrBearFunc, Func_484d3) { grab(gb, false); }
PORT_LOCAL_JUMP(BrrrBearFunc, Func_484e2) { hold(gb, false); }

PORT_LOCAL(BrrrBearFunc, Func_484f3)
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

PORT_LOCAL(SnowflakeFunc, Update)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}
