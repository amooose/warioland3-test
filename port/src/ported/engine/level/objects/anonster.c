// Port of src/engine/level/objects/anonster.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define SILK_PLATFORMS_CREATE_PLATFORM1 (ADDR_SilkPlatformsFunc + 0x26) // .CreatePlatform1
#define ANONSTER_SILK_DESTROY (ADDR_AnonsterSilkFunc + 0x14f) // AnonsterSilkFunc.destroy

static void create_platform(GB *gb);

// Spawns the three silk platforms one after the other.
PORT_JUMP(SilkPlatformsFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(SILK_PLATFORMS_CREATE_PLATFORM1 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)SILK_PLATFORMS_CREATE_PLATFORM1;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	wr8(gb, wCurObjStateDuration, 0x28);

	create_platform(gb);
}

// .asm_50746: bc = the platform's params, de = the next update function
static void create_platform(GB *gb)
{
	uint16_t hl;
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	gb->cpu.a = 2;
	wr8(gb, wCurObjFrameDuration, 2);
	hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	gb->cpu.a = 0x32;
	wr8(gb, hl, 0x32);
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	JUMP(CreateObjectAtRelativePos);
}

PORT_JUMP(SilkPlatformFunc)
{
	CALL(SetOneWayPlatformAction);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	wr8(gb, hl, alu_dec(gb, gb->cpu.a));
	JUMP(MoveObjectDown_Slow);
}

PORT_JUMP(AnonsteWaveLFunc_Left)
{
	CALL(MoveObjectLeft_Fast);
	JUMP_TO(ADDR_AnonsteWaveFunc);
}

PORT_JUMP(AnonsterWaveFunc_Right)
{
	CALL(MoveObjectRight_Fast);
	JUMP_TO(ADDR_AnonsteWaveFunc); // fallthrough
}

PORT_JUMP(AnonsteWaveFunc)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(ANONSTER_SILK_DESTROY);
		return;
	}
	RET();
}

#define SILK(off) ((uint16_t)(ADDR_AnonsterSilkFunc + (off)))
#define SILK_FLOAT_LEFT SILK(0x00) // .FloatLeft
#define SILK_FLOAT_RIGHT SILK(0x3f) // .FloatRight
#define SILK_FUNC_508BE SILK(0x145) // .Func_508be
#define SILK_FUNC_508CD SILK(0x154) // .Func_508cd

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

// ld hl, wCurObjState / ld a, state / ld [hld], a
static uint16_t state_hld(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	SET_HL(gb, hl);
	return hl;
}

// dec [hl] at hl: true if it reached 0
static bool dec_hl(GB *gb, uint16_t hl)
{
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	return !d;
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

// ld a, [hli] / add or sub delta / ldh [lo], a / ld a, [hli] / adc or sbc 0 / ldh [hi], a
static uint16_t offset16(GB *gb, uint16_t hl, int delta, uint16_t lo, uint16_t hi)
{
	if (delta < 0) {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), (uint8_t)-delta);
		wr8(gb, lo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	} else {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), (uint8_t)delta);
		wr8(gb, lo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
	}
	wr8(gb, hi, gb->cpu.a);
	SET_HL(gb, hl);
	return hl;
}


static void silk_create(GB *gb, uint16_t params, uint16_t next)
{
	SET_BC(gb, params);
	SET_DE(gb, next);
	create_platform(gb);
}

PORT_LOCAL_JUMP(SilkPlatformsFunc, CreatePlatform3)
{
	silk_create(gb, ADDR_ObjParams_SilkPlatform3, ADDR_SilkPlatformsFunc + 0x47); // .Wait
}

PORT_LOCAL_JUMP(SilkPlatformsFunc, CreatePlatform2)
{
	silk_create(gb, ADDR_ObjParams_SilkPlatform2, ADDR_SilkPlatformsFunc + 0x16); // .CreatePlatform3
}

PORT_LOCAL_JUMP(SilkPlatformsFunc, CreatePlatform1)
{
	silk_create(gb, ADDR_ObjParams_SilkPlatform1, ADDR_SilkPlatformsFunc + 0x1e); // .CreatePlatform2
}

PORT_LOCAL(SilkPlatformsFunc, Wait)
{
	no_actions(gb);
	gb->cpu.a = 2;
	wr8(gb, wCurObjFrameDuration, 2);
}

static void destroy(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}

// Falls off: disappears after a while.
static void func_50898(GB *gb)
{
	no_actions(gb);
	store_update(gb, wCurObjUpdateFunction + 1, SILK_FUNC_508BE);
	set_frameset(gb, ADDR_Frameset_6955b);
	ld_hli(gb, 0x1d);
	set_bits(gb, lo_hl(gb, OBJ_FLAGS), OBJFLAG_NO_COLLISION, OBJFLAG_GRABBED);
	play_sfx(gb, SFX_0A1);
}

PORT_LOCAL(AnonsterSilkFunc, Func_50898) { func_50898(gb); }

PORT_LOCAL(AnonsterSilkFunc, Func_508be)
{
	no_actions(gb);
	if (!dec_hl(gb, wCurObjStateDuration))
		return;
	destroy(gb);
}

PORT_LOCAL(AnonsterSilkFunc, destroy) { destroy(gb); }

// .asm_50855: hit or not, then sinks a pixel every 4 frames until it lands
static void silk_state(GB *gb)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		alu_cp(gb, gb->cpu.a, 0x0b);
		if (gb->cpu.f & FLAG_C)
			func_50898(gb);
		else
			destroy(gb);
		return;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
	if (gb->cpu.a)
		return;
	CALL(MoveObjectDown);
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_352b, SILK(0x103));
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	set_frameset(gb, ADDR_Frameset_694f5);
	ld_hli(gb, 0x64);
	store_update(gb, lo_hl(gb, OBJ_UPDATE_FUNCTION + 1), SILK_FUNC_508CD);
	hl = lo_hl(gb, OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_00);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// .asm_507f7: drifts with the wind, turning at walls and at random
static void drift(GB *gb)
{
	gb->cpu.a = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, gb->cpu.a, 0xb0);
	if (!(gb->cpu.f & FLAG_C)) {
		destroy(gb);
		return;
	}
	offset16(gb, wCurObjYPos, -4, hYPosLo, hYPosHi);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (gb->cpu.f & FLAG_C) {
		SET_BC(gb, ADDR_Data_60800);
		CALL(ApplyObjXMovement_Loop);
		offset16(gb, lo_hl(gb, OBJ_X_POS), -5, hXPosLo, hXPosHi);
	} else {
		SET_BC(gb, ADDR_Data_60830);
		CALL(ApplyObjXMovement_Loop);
		offset16(gb, lo_hl(gb, OBJ_X_POS), 4, hXPosLo, hXPosHi);
	}
	gb_call_ret(gb, ADDR_Func_3513, SILK(0xc6));
	bool turn;
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		turn = true;
	} else {
		gb->cpu.a = rd8(gb, wCurObjVar2);
		alu_cp(gb, gb->cpu.a, 0x2e);
		if (!(gb->cpu.f & FLAG_Z)) {
			silk_state(gb);
			return;
		}
		// .asm_50849
		gb->cpu.a = alu_rra(gb, rd8(gb, rDIV));
		turn = !(gb->cpu.f & FLAG_C);
	}
	if (turn) {
		// .asm_5084e
		uint16_t hl = wCurObjSubState;
		SET_HL(gb, hl);
		gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
		wr8(gb, hl, gb->cpu.a);
	}
	silk_state(gb);
}

// Floats sideways while the state duration lasts, turning at walls.
static void float_side(GB *gb, bool right)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		drift(gb);
		return;
	}
	if (right)
		CALL(MoveObjectRight_Fast);
	else
		CALL(MoveObjectLeft_Fast);
	hl = offset16(gb, wCurObjYPos, -4, hYPosLo, hYPosHi);
	offset16(gb, hl, right ? 4 : -5, hXPosLo, hXPosHi);
	gb_call_ret(gb, ADDR_Func_3513, SILK(right ? 0x63 : 0x24));
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		store_update(gb, wCurObjUpdateFunction + 1, right ? SILK_FLOAT_LEFT : SILK_FLOAT_RIGHT);
		hl = lo_hl(gb, OBJ_SUBSTATE);
		set_bits(gb, hl, right ? 0 : OBJSUBFLAG_HDIR, right ? OBJSUBFLAG_HDIR : 0);
		return;
	}
	// .asm_507ae / .asm_507ed
	if (dec_hl(gb, wCurObjStateDuration)) {
		// .asm_50849
		gb->cpu.a = alu_rra(gb, rd8(gb, rDIV));
		if (!(gb->cpu.f & FLAG_C)) {
			hl = wCurObjSubState;
			SET_HL(gb, hl);
			gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
			wr8(gb, hl, gb->cpu.a);
		}
	}
	silk_state(gb);
}

PORT(AnonsterSilkFunc) { float_side(gb, false); } // .FloatLeft
PORT_LOCAL(AnonsterSilkFunc, FloatRight) { float_side(gb, true); }

// Landed: hit states.
PORT_LOCAL_JUMP(AnonsterSilkFunc, Func_508cd)
{
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	set_bits(gb, hl, 0, OBJFLAG_INVISIBLE);
	gb->cpu.a = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, gb->cpu.a, 0xb0);
	if (!(gb->cpu.f & FLAG_C)) {
		alu_cp(gb, gb->cpu.a, 0xc0);
		if (gb->cpu.f & FLAG_C) {
			func_50898(gb);
			RET();
			return;
		}
	}
	gb->cpu.a = rd8(gb, wCurObjState);
	JUMP_TO(jumptable(gb, SILK(0x168)));
}

static void func_509a3(GB *gb, uint8_t state)
{
	state_hld(gb, state);
	uint16_t hl = lo_hl(gb, OBJ_VAR_2);
	gb->cpu.a = 0x02;
	wr8(gb, hl++, 0x02);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	SET_HL(gb, hl);
}

PORT_LOCAL(AnonsterSilkFunc, Func_50995) { func_509a3(gb, 0x44); }
PORT_LOCAL(AnonsterSilkFunc, Func_5099d) { func_509a3(gb, 0x45); }

PORT_LOCAL(AnonsterSilkFunc, Func_509ab)
{
	gb->cpu.a = 0x3f;
	wr8(gb, wCurObjState, 0x3f);
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	set_bits(gb, hl, 0, OBJFLAG_GRABBED);
	gb->cpu.a = 0x78;
	wr8(gb, wCurObjStateDuration, 0x78);
}

// Lies there, flickering at the end, then disappears.
PORT_LOCAL_JUMP(AnonsterSilkFunc, Func_509bb)
{
	uint16_t hl = wCurObjStateDuration;
	if (dec_hl(gb, hl)) {
		destroy(gb);
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x3c);
	if (gb->cpu.f & FLAG_C) {
		hl = lo_hl(gb, OBJ_FLAGS);
		gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x07);
		alu_cp(gb, gb->cpu.a, 0x04);
		if (gb->cpu.f & FLAG_C)
			set_bits(gb, hl, OBJFLAG_INVISIBLE, 0);
		else
			set_bits(gb, hl, 0, OBJFLAG_INVISIBLE);
	}
	JUMP(Func_32ea);
}

// .asm_50a13: knocked away, b = speed
static void knock(GB *gb, uint8_t state)
{
	state_hld(gb, state);
	gb->cpu.b = 0x02;
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	no_actions(gb);
}

// knocked away unless it's high on the screen
static void knock_low(GB *gb, uint8_t state)
{
	gb->cpu.a = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, gb->cpu.a, 0x50);
	if (gb->cpu.f & FLAG_C) {
		JUMP(VanishObject2);
		return;
	}
	knock(gb, state);
	RET();
}

PORT_LOCAL_JUMP(AnonsterSilkFunc, Func_509dd) { knock_low(gb, 0x59); }
PORT_LOCAL_JUMP(AnonsterSilkFunc, Func_509ef) { knock_low(gb, 0x58); }
PORT_LOCAL(AnonsterSilkFunc, Func_50a01) { knock(gb, 0x34); }
PORT_LOCAL(AnonsterSilkFunc, Func_50a0b) { knock(gb, 0x35); }

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

PORT_LOCAL_JUMP(AnonsterSilkFunc, Func_50a1e)
{
	thrown(gb, 0x36, 0xc0);
	JUMP(Func_3335);
}

PORT_LOCAL_JUMP(AnonsterSilkFunc, Func_50a31)
{
	thrown(gb, 0x37, 0xe0);
	JUMP(Func_3344);
}

// Grabbed: .Func_50a44 ($49) and .Func_50a5e ($48) turn with Wario.
static void grab(GB *gb, bool right)
{
	for (;;) {
		state_hld(gb, right ? 0x49 : 0x48);
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
		gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
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
	gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
	wr8(gb, hl, gb->cpu.a);
	grab(gb, !right);
}

PORT_LOCAL_JUMP(AnonsterSilkFunc, Func_50a44) { grab(gb, true); }
PORT_LOCAL_JUMP(AnonsterSilkFunc, Func_50a4e) { hold(gb, true); }
PORT_LOCAL_JUMP(AnonsterSilkFunc, Func_50a5e) { grab(gb, false); }
PORT_LOCAL_JUMP(AnonsterSilkFunc, Func_50a68) { hold(gb, false); }

PORT_LOCAL(AnonsterSilkFunc, Func_50a79)
{
	uint16_t hl = wCurObjAction;
	gb->cpu.a = 31 | (1 << 7); // no_actions_for 31
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = 0x42;
	wr8(gb, hl--, 0x42); // OBJ_STATE
	SET_HL(gb, hl);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
}

PORT_JUMP(UpdateAnonsterPal)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wCurObjSubState), 0x0f);
	alu_cp(gb, gb->cpu.a, 0x03);
	if (gb->cpu.f & FLAG_Z) {
		SET_HL(gb, ADDR_UpdateAnonsterPal + 0x1e); // .Pals_50ab4
	} else {
		alu_cp(gb, gb->cpu.a, 0x02);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		SET_HL(gb, ADDR_UpdateAnonsterPal + 0x26); // .Pals_50abc
	}
	SET_DE(gb, wTempPals2 + 7 * PAL_SIZE);
	gb->cpu.c = 7;
	gb->cpu.b = 1;
	JUMP(CopyAndApplyOBPals);
}

#define AF(off) ((uint16_t)(ADDR_AnonsterFunc + (off)))
#define AF_50024 AF(0x24)
#define AF_5005E AF(0x5e)
#define AF_5007F AF(0x7f)
#define AF_5016F AF(0x16f)
#define AF_501C4 AF(0x1c4)
#define AF_50200 AF(0x200)
#define AF_5023D AF(0x23d)
#define AF_50282 AF(0x282)
#define AF_50293 AF(0x293)
#define AF_502D1 AF(0x2d1)
#define AF_5034C AF(0x34c)
#define AF_50397 AF(0x397)
#define AF_5041D AF(0x41d)
#define AF_505BE AF(0x5be)
#define AF_505F4 AF(0x5f4)
#define AF_50610 AF(0x610)
#define AF_50635 AF(0x635)
#define AF_50650 AF(0x650)
#define AF_5066C AF(0x66c)
#define AF_50689 AF(0x689)
#define AF_50705 AF(0x705)

// a = [wCurObjSubState] rlca'd: frameset left (nc) or right (c)
static void sub_frameset(GB *gb, uint16_t left, uint16_t right)
{
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	set_frameset(gb, (gb->cpu.f & FLAG_C) ? right : left);
}

// ld l, OBJ_UPDATE_FUNCTION + 1 / ld a, HIGH(x) / ld [hld], a / ...
static void set_update(GB *gb, uint16_t update)
{
	store_update(gb, lo_hl(gb, OBJ_UPDATE_FUNCTION + 1), update);
}

// carry if the object is left of Wario (screen x + $2a compared)
static bool left_of_wario(GB *gb)
{
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	return gb->cpu.f & FLAG_C;
}

// [wCurObjPos] into hYPos / hXPos
static void copy_pos(GB *gb)
{
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

static void store_y(GB *gb)
{
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = rd8(gb, hYPosLo);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
}

// .asm_50031: hides in the ceiling
static void anonster_50031(GB *gb)
{
	sub_frameset(gb, ADDR_Frameset_693c4, ADDR_Frameset_693d5);
	ld_hli(gb, 0x50);
	set_update(gb, AF_5005E);
	uint16_t hl = lo_hl(gb, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x08);
	wr8(gb, hl--, 0xf7);
	wr8(gb, hl--, 0xfc);
	gb->cpu.a = 0xe3;
	wr8(gb, hl--, 0xe3);
	SET_HL(gb, hl);
	set_bits(gb, hl, 0, 1 << 7);
}

static void func_50024(GB *gb)
{
	if (!dec_hl(gb, wCurObjStateDuration))
		return;
	gb->cpu.a = BOSS_REGULAR;
	wr8(gb, wBossState, BOSS_REGULAR);
	CALL(UpdateLevelMusic);
	anonster_50031(gb);
}

PORT(AnonsterFunc)
{
	gb->cpu.a = TRUE;
	wr8(gb, wIsBossBattle, TRUE);
	gb->cpu.a = 27;
	wr8(gb, wCurObjVar2, 27);
	CALL(MoveObjectUpByVar2);
	uint16_t hl = lo_hl(gb, OBJ_SUBSTATE);
	set_bits(gb, hl, 0, OBJSUBFLAG_UNINITIALISED);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), 0xf0), 0x04);
	wr8(gb, hl, gb->cpu.a);
	gb->cpu.a = 0x28;
	wr8(gb, wCurObjStateDuration, 0x28);
	set_update(gb, AF_50024);
	func_50024(gb);
}

PORT_LOCAL(AnonsterFunc, Func_50024) { func_50024(gb); }

PORT_LOCAL(AnonsterFunc, Func_5005e)
{
	no_actions(gb);
	if (!dec_hl(gb, wCurObjStateDuration))
		return;
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	set_bits(gb, hl, 0, OBJFLAG_INVISIBLE);
	set_update(gb, AF_5007F);
	hl = lo_hl(gb, OBJ_STATE);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	hl = REG_HL(gb);
	wr8(gb, hl--, 0);
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	hl = REG_HL(gb);
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
}

// .asm_5018e
static void anonster_5018e(GB *gb)
{
	sub_frameset(gb, ADDR_Frameset_693f9, ADDR_Frameset_69424);
	ld_hli(gb, 0x20);
}

// .asm_5017f
static void anonster_5017f(GB *gb)
{
	SET_BC(gb, ADDR_Data_60790);
	JUMP(ApplyObjYMovement_Loop);
}

// Walks along the ceiling, turning at walls, until it's time to drop silk.
PORT_LOCAL_JUMP(AnonsterFunc, Func_5007f)
{
	no_actions(gb);
	uint16_t hl = wCurObjState;
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0xfe);
	alu_cp(gb, gb->cpu.a, 0x28);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
		SET_HL(gb, hl);
		set_frameset(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_69441 : ADDR_Frameset_6943c);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, REG_HL(gb), 0);
		set_update(gb, AF_50282);
		RET();
		return;
	}
	// .asm_500a8
	hl = lo_hl(gb, OBJ_FLAGS);
	set_bits(gb, hl, 0, OBJFLAG_NO_COLLISION);
	SET_BC(gb, ADDR_Data_60790);
	CALL(ApplyObjYMovement_Loop);
	hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			RET();
			return;
		}
		gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
		SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_693d5 : ADDR_Frameset_693c4);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	// .asm_500cd
	hl = lo_hl(gb, OBJ_VAR_3);
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x4f);
	if (gb->cpu.f & FLAG_Z) {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, 0x02);
		if (!(gb->cpu.f & FLAG_C)) {
			FARCALL(_CountActiveTransientObjects);
			gb->cpu.a = gb->cpu.e;
			if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
				hl = REG_HL(gb);
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z; // xor a
				wr8(gb, hl++, 0);
				wr8(gb, hl, 0);
				SET_HL(gb, hl);
				set_update(gb, AF_5016F);
				hl = lo_hl(gb, OBJ_SUBSTATE);
				if (left_of_wario(gb)) {
					set_bits(gb, hl, OBJSUBFLAG_HDIR, 0);
					SET_DE(gb, ADDR_Frameset_69411);
				} else {
					set_bits(gb, hl, 0, OBJSUBFLAG_HDIR);
					SET_DE(gb, ADDR_Frameset_693e6);
				}
				JUMP(SetObjectFramesetPtr);
				return;
			}
		}
	}
	// .asm_50118
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	gb->cpu.a = alu_dec(gb, rd8(gb, REG_HL(gb)));
	if (gb->cpu.f & FLAG_Z)
		play_sfx(gb, SFX_0B0);
	hl = offset16(gb, wCurObjYPos, -4, hYPosLo, hYPosHi);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	bool right = gb->cpu.f & FLAG_C;
	offset16(gb, hl, right ? 0x10 : -0x11, hXPosLo, hXPosHi);
	gb_call_ret(gb, ADDR_Func_3513, AF(right ? 0x15b : 0x145));
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		if (right)
			JUMP(MoveObjectRight);
		else
			JUMP(MoveObjectLeft);
		return;
	}
	// .asm_50162
	set_frameset(gb, right ? ADDR_Frameset_69512 : ADDR_Frameset_69446);
	ld_hli(gb, 0x0a);
	hl = lo_hl(gb, OBJ_SUBSTATE);
	gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
	wr8(gb, hl, gb->cpu.a);
	RET();
}

// .asm_50266: back to walking
static void anonster_50266(GB *gb)
{
	set_update(gb, AF_5016F);
	sub_frameset(gb, ADDR_Frameset_69402, ADDR_Frameset_6942d);
	anonster_5017f(gb);
}

// Drops silk at var3 $1f, goes back to walking at $4f.
PORT_LOCAL_JUMP(AnonsterFunc, Func_5016f)
{
	no_actions(gb);
	gb->cpu.a = rd8(gb, wCurObjVar3);
	alu_cp(gb, gb->cpu.a, 0x1f);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_50185
		store_update(gb, wCurObjUpdateFunction + 1, AF_501C4);
		anonster_5018e(gb);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x4f);
	if (!(gb->cpu.f & FLAG_Z)) {
		anonster_5017f(gb);
		return;
	}
	// .asm_501a3
	store_update(gb, wCurObjUpdateFunction + 1, AF_5007F);
	uint16_t hl = lo_hl(gb, OBJ_SUBSTATE);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	bool c = gb->cpu.f & FLAG_C;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	set_frameset(gb, c ? ADDR_Frameset_693d5 : ADDR_Frameset_693c4);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, REG_HL(gb), 0);
	RET();
}

// Drops a silk strand at duration $18; next: the next update function, or
// back to walking if the substate's low nybble is last.
static void drop_silk(GB *gb, uint16_t left, uint16_t right, uint8_t last, uint16_t next)
{
	no_actions(gb);
	uint16_t hl = wCurObjStateDuration;
	if (dec_hl(gb, hl)) {
		if (last) {
			gb->cpu.a = alu_and(gb, rd8(gb, wCurObjSubState), 0x0f);
			alu_cp(gb, gb->cpu.a, last);
			if (!(gb->cpu.f & FLAG_Z)) {
				set_update(gb, next);
				anonster_5018e(gb);
				RET();
				return;
			}
		}
		anonster_50266(gb);
		return;
	}
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	play_sfx(gb, SFX_07E);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	SET_BC(gb, (gb->cpu.f & FLAG_C) ? right : left);
	JUMP(CreateObjectAtRelativePos);
}

PORT_LOCAL_JUMP(AnonsterFunc, Func_501c4)
{
	drop_silk(gb, ADDR_ObjParams_AnonsterSilk1Left, ADDR_ObjParams_AnonsterSilk1Right, 0x04, AF_50200);
}

PORT_LOCAL_JUMP(AnonsterFunc, Func_50200)
{
	drop_silk(gb, ADDR_ObjParams_AnonsterSilk2Left, ADDR_ObjParams_AnonsterSilk2Right, 0x03, AF_5023D);
}

PORT_LOCAL_JUMP(AnonsterFunc, Func_5023d)
{
	drop_silk(gb, ADDR_ObjParams_AnonsterSilk3Left, ADDR_ObjParams_AnonsterSilk3Right, 0, 0);
}

// Climbs to the top of the screen, then turns towards Wario.
static void func_50293(GB *gb)
{
	no_actions(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d) {
			RET();
			return;
		}
		gb->cpu.l = alu_inc(gb, gb->cpu.l);
		gb->cpu.a = 0x03;
		wr8(gb, REG_HL(gb), 0x03);
		set_update(gb, AF_502D1);
		RET();
		return;
	}
	// .asm_502ae
	gb->cpu.a = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (!(gb->cpu.f & FLAG_C)) {
		JUMP(MoveObjectUp_Slow);
		return;
	}
	gb->cpu.a = 0x3c;
	wr8(gb, wCurObjStateDuration, 0x3c);
	hl = lo_hl(gb, OBJ_SUBSTATE);
	if (left_of_wario(gb))
		set_bits(gb, hl, OBJSUBFLAG_HDIR, 0);
	else
		set_bits(gb, hl, 0, OBJSUBFLAG_HDIR);
	RET();
}

PORT_LOCAL_JUMP(AnonsterFunc, Func_50282)
{
	store_update(gb, wCurObjUpdateFunction + 1, AF_50293);
	play_sfx(gb, SFX_0B1);
	func_50293(gb);
}

PORT_LOCAL_JUMP(AnonsterFunc, Func_50293) { func_50293(gb); }

// .asm_50388: the collision box while on the ground
static void ground_collbox(GB *gb)
{
	uint16_t hl = lo_hl(gb, OBJ_COLLBOX_TOP);
	wr8(gb, hl++, 0xf0);
	wr8(gb, hl++, 0x00);
	wr8(gb, hl++, 0xf0);
	gb->cpu.a = 0x0f;
	wr8(gb, hl++, 0x0f);
	SET_HL(gb, hl);
}

// Runs along the ceiling to above Wario (var1 turns at most), then drops.
PORT_LOCAL_JUMP(AnonsterFunc, Func_502d1)
{
	gb->cpu.b = alu_and(gb, rd8(gb, wWarioScreenXPos), 0xfe);
	gb->cpu.a = alu_and(gb, rd8(gb, wCurObjScreenXPos), 0xfe);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	bool drop = gb->cpu.f & FLAG_Z;
	if (!drop) {
		uint16_t hl = offset16(gb, wCurObjYPos, -4, hYPosLo, hYPosHi);
		gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
		bool right = gb->cpu.f & FLAG_C;
		offset16(gb, hl, right ? 0x10 : -0x11, hXPosLo, hXPosHi);
		gb_call_ret(gb, ADDR_Func_3513, AF(right ? 0x312 : 0x2ff));
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			if (right)
				JUMP(MoveObjectRight_Fast);
			else
				JUMP(MoveObjectLeft_Fast);
			return;
		}
		// .asm_50316
		hl = wCurObjSubState;
		gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
		wr8(gb, hl, gb->cpu.a);
		SET_HL(gb, hl);
		if (!dec_hl(gb, lo_hl(gb, OBJ_VAR_1))) {
			RET();
			return;
		}
	}
	// .asm_50321
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	set_bits(gb, hl, 0, OBJFLAG_NO_COLLISION);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
	set_update(gb, AF_5034C);
	hl = lo_hl(gb, OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_48 | HEAVY_OBJ);
	wr8(gb, hl++, gb->cpu.a);
	wr8(gb, hl++, 0xe8);
	wr8(gb, hl++, 0xf8);
	wr8(gb, hl++, 0xf4);
	gb->cpu.a = 0x0b;
	wr8(gb, hl++, 0x0b);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_6944d);
	JUMP(SetObjectFramesetPtr);
}

// Drops to the ground.
PORT_LOCAL_JUMP(AnonsterFunc, Func_5034c)
{
	gb->cpu.a = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, gb->cpu.a, 0x50);
	bool landed = false;
	if (!(gb->cpu.f & FLAG_C)) {
		copy_pos(gb);
		gb_call_ret(gb, ADDR_Func_352b, AF(0x365));
		landed = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	}
	if (!landed) {
		SET_BC(gb, ADDR_Data_607f0);
		JUMP(ApplyObjYMovement);
		return;
	}
	store_y(gb);
	set_update(gb, AF_50397);
	set_frameset(gb, ADDR_Frameset_69450);
	ld_hli(gb, 0x20);
	ground_collbox(gb);
	RET();
}

// Lands (shaking the ground), then turns to Wario.
PORT_LOCAL_JUMP(AnonsterFunc, Func_50397)
{
	uint16_t hl = wCurObjStateDuration;
	dec_hl(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_503d3
		play_sfx(gb, SFX_061);
		gb->cpu.b = 0x18;
		JUMP(DoGroundShake);
		return;
	}
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	set_update(gb, AF_5041D);
	hl = lo_hl(gb, OBJ_STATE);
	gb->cpu.a = 0x5a;
	wr8(gb, hl--, 0x5a);
	SET_HL(gb, hl);
	if (left_of_wario(gb)) {
		set_bits(gb, hl, 1 << 7, 0);
		SET_DE(gb, ADDR_Frameset_694c9);
	} else {
		set_bits(gb, hl, 0, 1 << 7);
		SET_DE(gb, ADDR_Frameset_694d0);
	}
	CALL(SetObjectFramesetPtr);
	ld_hli(gb, 0x07);
	gb->cpu.a = 0x02;
	wr8(gb, REG_HL(gb), 0x02);
	RET();
}

// .asm_5043b: stands, facing its direction
static void anonster_5043b(GB *gb)
{
	uint16_t hl = lo_hl(gb, OBJ_COLLBOX_RIGHT);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	uint8_t left;
	if (gb->cpu.f & FLAG_C) {
		SET_DE(gb, ADDR_Frameset_69496);
		wr8(gb, hl--, 0x07);
		left = 0xef;
	} else {
		SET_DE(gb, ADDR_Frameset_69485);
		wr8(gb, hl--, 0x10);
		left = 0xf8;
	}
	wr8(gb, hl--, left);
	wr8(gb, hl--, 0x00);
	wr8(gb, hl--, 0xf0);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_48);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, REG_HL(gb), 0);
	wr8(gb, wCurObjState, 0);
	RET();
}

// .asm_504b1: hit by Ball o' String Wario
static void anonster_504b1(GB *gb)
{
	sub_frameset(gb, ADDR_Frameset_69554, ADDR_Frameset_6954d);
	ld_hli(gb, 0x07);
	set_update(gb, AF_50650);
	ground_collbox(gb);
	RET();
}

// .asm_504fe: rushes, turning at walls
static void anonster_504fe(GB *gb)
{
	uint16_t hl = lo_hl(gb, OBJ_SUBSTATE);
	if (!alu_bit(gb, rd8(gb, hl), OBJSUBFLAG_HDIR_F)) {
		hl = offset16(gb, wCurObjYPos, -4, hYPosLo, hYPosHi);
		offset16(gb, hl, -0x14, hXPosLo, hXPosHi);
		gb_call_ret(gb, ADDR_Func_3513, AF(0x51e));
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			JUMP(MoveObjectLeft);
			return;
		}
		SET_DE(gb, ADDR_Frameset_69573);
	} else {
		hl = wCurObjYPos;
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, hYPosLo, gb->cpu.a);
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, hYPosHi, gb->cpu.a);
		offset16(gb, hl, 0x17, hXPosLo, hXPosHi);
		gb_call_ret(gb, ADDR_Func_3543, AF(0x53d));
		if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			JUMP(MoveObjectRight);
			return;
		}
		SET_DE(gb, ADDR_Frameset_69568);
	}
	// .asm_50544
	CALL(SetObjectFramesetPtr);
	ld_hli(gb, 0x0e);
	RET();
}

// state duration: counts down to standing, turning around at 7
static bool count_down(GB *gb)
{
	uint16_t hl = lo_hl(gb, OBJ_STATE_DURATION);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return false;
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d) {
		anonster_5043b(gb);
		return true;
	}
	gb->cpu.a = d;
	alu_cp(gb, d, 0x07);
	if (gb->cpu.f & FLAG_Z) {
		hl = lo_hl(gb, OBJ_SUBSTATE);
		gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
		wr8(gb, hl, gb->cpu.a);
	}
	RET();
	return true;
}

// .asm_5054b (hl = wCurObjState): hit from behind
static void anonster_5054b(GB *gb)
{
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	uint16_t hl = REG_HL(gb);
	bool c = gb->cpu.f & FLAG_C;
	bool hdir = alu_bit(gb, rd8(gb, hl), 7);
	if (hdir ? c : !c) {
		anonster_5043b(gb);
		return;
	}
	// .asm_50564
	hl = lo_hl(gb, OBJ_INTERACTION_TYPE);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_0B);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	play_sfx(gb, SFX_VANISH);
	CALL(DecrementRemainingBossHits);
	hl = REG_HL(gb);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
	SET_HL(gb, hl);
	set_frameset(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_694b8 : ADDR_Frameset_694a7);
	gb->cpu.a = alu_dec(gb, alu_and(gb, rd8(gb, wCurObjSubState), 0x0f));
	hl = REG_HL(gb);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_505a0: defeated
		gb->cpu.a = 0xac;
		wr8(gb, hl, 0xac);
		hl = lo_hl(gb, OBJ_SUBSTATE);
		gb->cpu.a = alu_and(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
		wr8(gb, hl, gb->cpu.a);
		set_update(gb, AF_50689);
		// stop_music2
		gb->cpu.a = 0xff;
		wr8(gb, hMusicIDBackup + 0, 0xff);
		wr8(gb, hMusicID + 0, 0xff);
		gb->cpu.a = 0x00;
		wr8(gb, hMusicIDBackup + 1, 0x00);
		wr8(gb, hMusicID + 1, 0x00);
		RET();
		return;
	}
	gb->cpu.a = 0x56;
	wr8(gb, hl, 0x56);
	hl = lo_hl(gb, OBJ_FLAGS);
	set_bits(gb, hl, 0, OBJFLAG_NO_COLLISION);
	set_update(gb, AF_505BE);
	JUMP_TO(ADDR_UpdateAnonsterPal);
}

PORT_LOCAL_JUMP(AnonsterFunc, Func_5041d)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	uint8_t a = gb->cpu.a;
	if (!alu_and(gb, a, a)) {
		// .asm_5046b
		if (count_down(gb))
			return;
		// .asm_5047f
		gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
		if (gb->cpu.f & FLAG_C) {
			RET();
			return;
		}
		gb->cpu.b = rd8(gb, wWarioScreenXPos);
		gb->cpu.a = alu_sub(gb, rd8(gb, wCurObjScreenXPos), gb->cpu.b);
		bool c = gb->cpu.f & FLAG_C;
		hl = lo_hl(gb, OBJ_SUBSTATE);
		bool hdir = alu_bit(gb, rd8(gb, hl), OBJSUBFLAG_HDIR_F);
		if (hdir ? !c : c) {
			anonster_504fe(gb);
			return;
		}
		set_frameset(gb, hdir ? ADDR_Frameset_69474 : ADDR_Frameset_69463);
		ld_hli(gb, 0x5d);
		gb->cpu.a = 0x5b;
		wr8(gb, wCurObjState, 0x5b);
		RET();
		return;
	}
	alu_cp(gb, a, 0x5b);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_503e0: makes waves
		hl = lo_hl(gb, OBJ_STATE_DURATION);
		if (dec_hl(gb, hl)) {
			// .asm_50403
			gb->cpu.l = alu_inc(gb, gb->cpu.l);
			if (dec_hl(gb, REG_HL(gb))) {
				anonster_504b1(gb);
				return;
			}
			hl = lo_hl(gb, OBJ_STATE);
			gb->cpu.a = 0x5c;
			wr8(gb, hl--, 0x5c);
			gb->cpu.a = alu_rlca(gb, rd8(gb, hl--));
			SET_HL(gb, hl);
			SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_69496 : ADDR_Frameset_69485);
			JUMP(SetObjectFramesetPtr);
			return;
		}
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, 0x2b);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		play_sfx(gb, SFX_07E);
		gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
		SET_BC(gb, (gb->cpu.f & FLAG_C) ? ADDR_ObjParams_AnonsterWaveRight : ADDR_ObjParams_AnonsterWaveLeft);
		JUMP(CreateObjectAtRelativePos);
		return;
	}
	alu_cp(gb, a, 0x5c);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_504aa
		gb->cpu.a = rd8(gb, wTransformation);
		alu_cp(gb, gb->cpu.a, TRANSFORMATION_BALL_O_STRING_WARIO);
		if (gb->cpu.f & FLAG_Z) {
			anonster_504b1(gb);
			return;
		}
		// .asm_504d0
		if (count_down(gb))
			return;
		// .asm_504e5
		hl = lo_hl(gb, OBJ_FRAME_DURATION);
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, 0x03);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x03);
			wr8(gb, hl, gb->cpu.a);
		}
		gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x0f);
		if (gb->cpu.f & FLAG_Z)
			play_sfx(gb, SFX_0AF);
		anonster_504fe(gb);
		return;
	}
	alu_cp(gb, a, 0x5a);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_50437
		if (!dec_hl(gb, lo_hl(gb, OBJ_STATE_DURATION))) {
			RET();
			return;
		}
		anonster_5043b(gb);
		return;
	}
	alu_cp(gb, a, 0x0a);
	if (gb->cpu.f & FLAG_Z) {
		anonster_5054b(gb);
		return;
	}
	anonster_5043b(gb);
}

PORT_LOCAL_JUMP(AnonsterFunc, Func_505be)
{
	no_actions(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x4e);
	if (gb->cpu.f & FLAG_Z)
		play_sfx(gb, SFX_0B1);
	if (!dec_hl(gb, hl)) {
		RET();
		return;
	}
	sub_frameset(gb, ADDR_Frameset_6953c, ADDR_Frameset_69543);
	ld_hli(gb, 0x0e);
	set_update(gb, AF_505F4);
	ground_collbox(gb);
	RET();
}

PORT_LOCAL(AnonsterFunc, Func_505f4)
{
	no_actions(gb);
	if (!dec_hl(gb, wCurObjStateDuration))
		return;
	set_frameset(gb, ADDR_Frameset_6951e);
	ld_hli(gb, 0x33);
	set_update(gb, AF_50610);
}

PORT_LOCAL(AnonsterFunc, Func_50610)
{
	no_actions(gb);
	if (!dec_hl(gb, wCurObjStateDuration))
		return;
	set_frameset(gb, ADDR_Frameset_69519);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
	set_update(gb, AF_50635);
	play_sfx(gb, SFX_0B2);
}

// Climbs back up, then hides in the ceiling again.
PORT_LOCAL_JUMP(AnonsterFunc, Func_50635)
{
	no_actions(gb);
	gb->cpu.a = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (!(gb->cpu.f & FLAG_C)) {
		SET_BC(gb, ADDR_Data_607e0);
		JUMP(ApplyObjYMovement);
		return;
	}
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_BALL_O_STRING_WARIO);
	if (!(gb->cpu.f & FLAG_Z))
		anonster_50031(gb);
	RET();
}

PORT_LOCAL(AnonsterFunc, Func_50650)
{
	no_actions(gb);
	if (!dec_hl(gb, wCurObjStateDuration))
		return;
	set_frameset(gb, ADDR_Frameset_694e1);
	ld_hli(gb, 0x2d);
	set_update(gb, AF_5066C);
}

PORT_LOCAL(AnonsterFunc, Func_5066c)
{
	no_actions(gb);
	if (!dec_hl(gb, wCurObjStateDuration))
		return;
	set_frameset(gb, ADDR_Frameset_6944d);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
	set_update(gb, AF_50635);
}

// Falls off the screen.
static void func_50705(GB *gb)
{
	no_actions(gb);
	SET_BC(gb, ADDR_Data_60180);
	CALL(ApplyObjYMovement);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (!(gb->cpu.f & FLAG_C))
		JUMP(MoveObjectRight);
	else
		JUMP(MoveObjectLeft);
}

// Defeated.
PORT_LOCAL_JUMP(AnonsterFunc, Func_50689)
{
	no_actions(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// .asm_506f5
		set_update(gb, AF_50705);
		play_sfx(gb, SFX_0A3);
		func_50705(gb);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0xa4);
	if (gb->cpu.f & FLAG_Z)
		play_sfx(gb, SFX_0B1);
	if (!dec_hl(gb, hl)) {
		RET();
		return;
	}
	SET_HL(gb, wCurObjUnk02);
	gb->cpu.e = 0x05;
	FARCALL(Func_ba42);
	gb->cpu.a = BOSS_DEFEATED;
	wr8(gb, wBossState, BOSS_DEFEATED);
	CALL(UpdateLevelMusic);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wIsBossBattle, 0);
	sub_frameset(gb, ADDR_Frameset_694d7, ADDR_Frameset_694dc);
	hl = lo_hl(gb, OBJ_STATE);
	gb->cpu.a = 0x48;
	wr8(gb, hl--, 0x48);
	SET_HL(gb, hl);
	if (left_of_wario(gb))
		set_bits(gb, hl, OBJSUBFLAG_HDIR, 0);
	else
		set_bits(gb, hl, 0, OBJSUBFLAG_HDIR);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, REG_HL(gb), 0);
	hl = lo_hl(gb, OBJ_FLAGS);
	set_bits(gb, hl, OBJFLAG_PRIORITY, 0);
	RET();
}

PORT_LOCAL_JUMP(AnonsterFunc, Func_50705) { func_50705(gb); }
