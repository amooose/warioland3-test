// Port of src/engine/level/objects/barrel.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

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

// res OBJSUBFLAG_UNINITIALISED_F of hl's object
static inline uint16_t set_initialised(GB *gb, uint16_t hl)
{
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	return hl;
}

// Sets hl's object's collision box (right, left, bottom), a = bottom.
static inline uint16_t set_collbox(GB *gb, uint16_t hl, uint8_t right, uint8_t left, uint8_t bottom)
{
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, right);
	wr8(gb, hl--, left);
	gb->cpu.a = bottom;
	wr8(gb, hl, bottom);
	return hl;
}

// ld hl, wCurObjFlags / res OBJFLAG_INVISIBLE_F, [hl] / set OBJFLAG_NO_COLLISION_F, [hl]
static inline uint16_t show_no_collision(GB *gb)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	return hl;
}

PORT(Barrel1Func)
{
	uint16_t hl = show_no_collision(gb);
	SET_HL(gb, set_update(gb, hl, ADDR_Barrel1Func + 0x28)); // .Update1
	SET_DE(gb, ADDR_Frameset_68a7b);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x0a); // OBJ_STATE_DURATION
	hl = set_initialised(gb, hl);
	SET_HL(gb, set_collbox(gb, hl, 0x06, 0xf9, 0xf7));
}

#define BR1(off) ((uint16_t)(ADDR_Barrel1Func + (off)))
#define BR2(off) ((uint16_t)(ADDR_Barrel2Func + (off)))

static uint16_t lo_hl(GB *gb, uint8_t l)
{
	gb->cpu.l = l;
	return REG_HL(gb);
}

static void set_bits(GB *gb, uint16_t hl, uint8_t set, uint8_t res)
{
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~res) | set));
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

static void set_state(GB *gb, uint8_t state)
{
	gb->cpu.a = state;
	wr8(gb, wCurObjState, state);
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

// Counts down while Wario stands on it, then starts rolling.
PORT_LOCAL(Barrel1Func, Update1)
{
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_STEPPED_F)) {
		gb->cpu.a = 0x0a;
		wr8(gb, wCurObjStateDuration, 0x0a);
		return;
	}
	set_bits(gb, hl, 0, OBJFLAG_STEPPED);
	hl = lo_hl(gb, OBJ_STATE_DURATION);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	set_frameset(gb, ADDR_Frameset_68a7e);
	ld_hli(gb, 0x11);
	SET_HL(gb, set_update(gb, REG_HL(gb), BR1(0x51))); // .Update2
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
}

PORT_LOCAL_JUMP(Barrel1Func, Update2)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		SET_BC(gb, ADDR_Data_60a50);
		JUMP(Func_34fc);
		return;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d)
		play_sfx(gb, SFX_086);
	RET();
}

PORT(Barrel2Func)
{
	uint16_t hl = wCurObjFlags;
	set_bits(gb, hl, 0, OBJFLAG_INVISIBLE);
	SET_HL(gb, set_update(gb, hl, BR2(0x28))); // .Update
	set_frameset(gb, ADDR_Frameset_68a78);
	hl = set_initialised(gb, REG_HL(gb));
	SET_HL(gb, set_collbox(gb, hl, 0x06, 0xf9, 0x00));
	set_state(gb, 0x31);
}

PORT_LOCAL_JUMP(Barrel2Func, Update)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	JUMP_TO(jumptable(gb, BR2(0x2c)));
}

static void func_435c2(GB *gb)
{
	no_actions(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	lo_hl(gb, OBJ_UNK_02);
	FARCALL(DespawnObject);
}

// Breaks.
PORT_LOCAL(Barrel2Func, Func_435a6)
{
	set_state(gb, 0x2f);
	set_frameset(gb, ADDR_Frameset_68a8b);
	ld_hli(gb, 0x0e);
	set_bits(gb, lo_hl(gb, OBJ_FLAGS), OBJFLAG_NO_COLLISION, OBJFLAG_GRABBED);
	play_sfx(gb, SFX_TEMPLE_ROCK);
	func_435c2(gb);
}

PORT_LOCAL(Barrel2Func, Func_435c2) { func_435c2(gb); }
PORT_LOCAL(Barrel2Func, Func_435de) {}

static void bump(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	SET_HL(gb, hl);
	gb->cpu.a = 0x0c;
	wr8(gb, wCurObjStateDuration, 0x0c);
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjVar2, 0x02);
}

PORT_LOCAL(Barrel2Func, Func_435df) { bump(gb, 0x32); }
PORT_LOCAL(Barrel2Func, Func_435e7) { bump(gb, 0x33); }

static void func_435fe(GB *gb, uint8_t state)
{
	set_state(gb, state);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjVar2, 0x02);
}

PORT_LOCAL(Barrel2Func, Func_435f8) { func_435fe(gb, 0x44); }
PORT_LOCAL(Barrel2Func, Func_435fc) { func_435fe(gb, 0x45); }

// Thrown up.
static void thrown(GB *gb, uint8_t state, uint8_t var2)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	SET_HL(gb, hl);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	hl = REG_HL(gb);
	wr8(gb, hl--, 0x41);
	wr8(gb, hl--, var2);
	gb->cpu.a = 0x01;
	wr8(gb, hl--, 0x01);
	SET_HL(gb, hl);
	set_frameset(gb, ADDR_Frameset_68a78);
}

PORT_LOCAL_JUMP(Barrel2Func, Func_4360b)
{
	thrown(gb, 0x36, 0xc0);
	JUMP(Func_3335);
}

PORT_LOCAL_JUMP(Barrel2Func, Func_43624)
{
	thrown(gb, 0x37, 0xe0);
	JUMP(Func_3344);
}

// Grabbed: .Func_4363d (facing $49) and .Func_4365c ($48) turn with Wario.
static void grab(GB *gb, bool right)
{
	for (;;) {
		set_state(gb, right ? 0x49 : 0x48);
		set_frameset(gb, ADDR_Frameset_68a78);
		set_bits(gb, lo_hl(gb, OBJ_FLAGS), OBJFLAG_GRABBED, 0);
		// .Func_4364c / .Func_4366b
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

PORT_LOCAL_JUMP(Barrel2Func, Func_4363d) { grab(gb, true); }
PORT_LOCAL_JUMP(Barrel2Func, Func_4364c) { hold(gb, true); }
PORT_LOCAL_JUMP(Barrel2Func, Func_4365c) { grab(gb, false); }
PORT_LOCAL_JUMP(Barrel2Func, Func_4366b) { hold(gb, false); }

PORT_LOCAL(Barrel2Func, Func_4367c)
{
	uint16_t hl = wCurObjAction;
	gb->cpu.a = 0x9f;
	wr8(gb, hl--, 0x9f);
	gb->cpu.a = 0x42;
	wr8(gb, hl--, 0x42);
	SET_HL(gb, hl);
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, REG_HL(gb), 0);
}

PORT_LOCAL_JUMP(Barrel2Func, Func_43689)
{
	set_state(gb, 0x5e);
	set_frameset(gb, ADDR_Frameset_68a6f);
	JUMP(ObjState_FullThrowLeft);
}

PORT_LOCAL_JUMP(Barrel2Func, Func_43697)
{
	set_state(gb, 0x5f);
	set_frameset(gb, ADDR_Frameset_68a66);
	JUMP(ObjState_FullThrowRight);
}

// .asm_436db: knocked back with the update function func
static void knocked(GB *gb, uint16_t frameset, uint16_t func)
{
	set_frameset(gb, frameset);
	gb->cpu.a = (uint8_t)(func >> 8);
	gb->cpu.c = (uint8_t)func;
	gb->cpu.b = 0x02;
	uint16_t hl = lo_hl(gb, OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.c);
	SET_HL(gb, hl);
	hl = lo_hl(gb, OBJ_VAR_3);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	wr8(gb, hl, gb->cpu.b);
	SET_HL(gb, hl);
	no_actions(gb);
}

PORT_LOCAL(Barrel2Func, Func_436a5) { knocked(gb, ADDR_Frameset_68a6f, ADDR_Func_33f8); }
PORT_LOCAL(Barrel2Func, Func_436b3) { knocked(gb, ADDR_Frameset_68a66, ADDR_Func_3407); }
PORT_LOCAL(Barrel2Func, Func_436c1) { knocked(gb, ADDR_Frameset_68a6f, ADDR_Func_3317); }
PORT_LOCAL(Barrel2Func, Func_436cf) { knocked(gb, ADDR_Frameset_68a66, ADDR_Func_3326); }
