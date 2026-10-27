// Port of src/engine/level/objects/beam_bot.asm
#include "ported.h"

PORT_JUMP(BeamFunc)
{
	SET_HL(gb, wCurObjState);
	gb->cpu.a = rd8(gb, wCurObjState);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjState, 0);
		play_sfx(gb, SFX_07D);
	}
	JUMP(ApplyObjXMovement);
}

static void beam_update(GB *gb, uint16_t frameset, uint16_t movement);

// Sets the beam's update function to the .Update that follows, makes it
// non-collidable and plays the beam sfx, then runs .Update: changes frameset
// when wCurObjStateDuration runs out and moves the beam with movement.
static void beam_func(GB *gb, uint16_t update, uint16_t frameset, uint16_t movement)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	hl = (uint16_t)((hl & 0xff00) | OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	play_sfx(gb, SFX_07C);
	beam_update(gb, frameset, movement);
}

// .Update
static void beam_update(GB *gb, uint16_t frameset, uint16_t movement)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint8_t d = alu_dec(gb, gb->cpu.a);
		wr8(gb, hl, d);
		if (!d) {
			SET_DE(gb, frameset);
			CALL(SetObjectFramesetPtr);
		}
	}
	SET_BC(gb, movement);
	JUMP_TO(ADDR_BeamFunc);
}

PORT_JUMP(BeamFunc_Left)
{
	beam_func(gb, ADDR_BeamFunc_Left + 0x15, ADDR_Frameset_68ef3, ADDR_Data_60610);
}

PORT_JUMP(BeamFunc_Right)
{
	beam_func(gb, ADDR_BeamFunc_Right + 0x15, ADDR_Frameset_68ef8, ADDR_Data_60630);
}

PORT_LOCAL_JUMP(BeamFunc_Left, Update) { beam_update(gb, ADDR_Frameset_68ef3, ADDR_Data_60610); }
PORT_LOCAL_JUMP(BeamFunc_Right, Update) { beam_update(gb, ADDR_Frameset_68ef8, ADDR_Data_60630); }

#define BOT(off) ((uint16_t)(ADDR_BeamBotFunc + (off)))
#define BOT_INIT BOT(0x0a) // .Init
#define BOT_UPDATE BOT(0x1e) // .Update

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

// ld a, [hld] / rlca / ld_obj_frameset left, right / call SetObjectFramesetPtr
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

// ld l, OBJ_COLLBOX_RIGHT / ld a, $05 / ld [hld], a / ld a, $fa / ld [hld], a
static void collbox(GB *gb)
{
	uint16_t hl = lo_hl(gb, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x05);
	gb->cpu.a = 0xfa;
	wr8(gb, hl--, 0xfa);
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

// dec [hl] at hl: true if it reached 0
static bool dec_hl(GB *gb, uint16_t hl)
{
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	return !d;
}

PORT(BeamBotFunc)
{
	store_update(gb, wCurObjUpdateFunction + 1, BOT_INIT);
}

// Turns to face Wario and walks.
static void func_46691(GB *gb)
{
	uint16_t hl = wCurObjInteractionType;
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_01);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	collbox(gb);
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
	SET_DE(gb, left ? ADDR_Frameset_68e80 : ADDR_Frameset_68e77);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(BeamBotFunc, Init)
{
	uint16_t hl = wCurObjFlags;
	set_bits(gb, hl, 0, OBJFLAG_INVISIBLE);
	store_update(gb, (uint16_t)((hl & 0xff00) | (OBJ_UPDATE_FUNCTION + 1)), BOT_UPDATE);
	hl = lo_hl(gb, OBJ_SUBSTATE);
	set_bits(gb, hl, 0, OBJSUBFLAG_UNINITIALISED);
	func_46691(gb);
}

PORT_LOCAL_JUMP(BeamBotFunc, Update)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	JUMP_TO(jumptable(gb, BOT(0x22)));
}

PORT_LOCAL_JUMP(BeamBotFunc, Func_46691) { func_46691(gb); }

PORT_LOCAL(BeamBotFunc, Func_466cb)
{
	state_hld(gb, 0x41);
	bool c = hdir_hld(gb);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	set_frameset(gb, c ? ADDR_Frameset_68e80 : ADDR_Frameset_68e77);
}

// Shoots a beam.
PORT_LOCAL_JUMP(BeamBotFunc, Func_466e7)
{
	if (ground_shake(gb)) {
		RET();
		return;
	}
	uint16_t hl = wCurObjStateDuration;
	if (dec_hl(gb, hl)) {
		// .asm_46714
		lo_hl(gb, OBJ_STATE);
		hl = REG_HL(gb);
		gb->cpu.a = 0x2e;
		wr8(gb, hl--, 0x2e);
		SET_HL(gb, hl);
		dir_frameset(gb, ADDR_Frameset_68ea3, ADDR_Frameset_68eaa);
		ld_hli(gb, 50);
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x19);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	SET_BC(gb, (gb->cpu.f & FLAG_C) ? ADDR_ObjParams_BeamRight : ADDR_ObjParams_BeamLeft);
	JUMP(CreateObjectAtRelativePos);
}

PORT_LOCAL(BeamBotFunc, Func_4672c)
{
	if (ground_shake(gb))
		return;
	if (!dec_hl(gb, wCurObjStateDuration))
		return;
	set_state(gb, 0x13);
}

PORT_LOCAL(BeamBotFunc, Func_46748)
{
	uint16_t hl = wCurObjInteractionType;
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), OBJ_INTERACTION_01);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	hl = lo_hl(gb, OBJ_STATE);
	gb->cpu.a = 0x31;
	wr8(gb, hl--, 0x31);
	SET_HL(gb, hl);
	dir_frameset(gb, ADDR_Frameset_68e77, ADDR_Frameset_68e80);
	ld_hli(gb, 100);
}

// .asm_4685f: walking into a wall
static void blocked(GB *gb)
{
	gb->cpu.a = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (!(gb->cpu.f & FLAG_C)) {
		uint16_t hl = wCurObjFrameDuration;
		SET_HL(gb, hl);
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	}
	if (dec_hl(gb, wCurObjStateDuration))
		set_state(gb, 0x13);
	RET();
}

// Walks towards Wario; aims when he's close at about its height.
PORT_LOCAL_JUMP(BeamBotFunc, Func_46769)
{
	if (ground_shake(gb)) {
		RET();
		return;
	}
	bool look = true;
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_SPLIT);
	if (gb->cpu.f & FLAG_Z) {
		look = false;
	} else {
		uint16_t hl = wCurObjFlags;
		SET_HL(gb, hl);
		look = alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F);
	}
	if (look) {
		gb->cpu.c = 0x2a;
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenYPos), 0x2a);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), 0x2a);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		alu_cp(gb, gb->cpu.a, 0x1c);
		if (!(gb->cpu.f & FLAG_C)) {
			alu_cp(gb, gb->cpu.a, 0xe4);
			look = !(gb->cpu.f & FLAG_C);
		}
	}
	if (look) {
		// .asm_4679c
		uint16_t hl = wCurObjSubState;
		SET_HL(gb, hl);
		gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
		alu_cp(gb, gb->cpu.a, 0xb8);
		uint16_t frameset = 0;
		bool aim = false;
		if (!(gb->cpu.f & FLAG_C)) {
			// .asm_467be
			gb->cpu.a = alu_rlca(gb, rd8(gb, hl++));
			SET_HL(gb, hl);
			aim = true;
			frameset = (gb->cpu.f & FLAG_C) ? ADDR_Frameset_68e96 : 0;
		} else {
			alu_cp(gb, gb->cpu.a, 0x48);
			if (gb->cpu.f & FLAG_C) {
				gb->cpu.a = alu_rlca(gb, rd8(gb, hl++));
				SET_HL(gb, hl);
				aim = true;
				frameset = (gb->cpu.f & FLAG_C) ? 0 : ADDR_Frameset_68e89;
			}
		}
		if (aim) {
			if (!frameset) {
				// .asm_467ba
				gb->cpu.a = 0x13;
				wr8(gb, hl, 0x13);
				RET();
				return;
			}
			// .asm_467c5
			SET_DE(gb, frameset);
			gb->cpu.a = 0x2d;
			wr8(gb, hl, 0x2d);
			CALL(SetObjectFramesetPtr);
			ld_hli(gb, 41);
			RET();
			return;
		}
	}
	// .asm_467cf
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3543, BOT(0x22c));
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		set_state(gb, 0x11);
		RET();
		return;
	}
	// .asm_467ea
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
			gb_call_ret(gb, ADDR_ProcessBlockOnLeft, BOT(0x24e));
			gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
			if (!gb->cpu.a) {
				blocked(gb);
				return;
			}
		}
	} else {
		gb->cpu.a = alu_add(gb, alu_and(gb, rd8(gb, hl), 0x0f), 0x05);
		alu_cp(gb, gb->cpu.a, 0x10);
		if (!(gb->cpu.f & FLAG_C)) {
			gb_call_ret(gb, ADDR_ProcessBlockOnRight, BOT(0x282));
			gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
			if (!gb->cpu.a) {
				blocked(gb);
				return;
			}
		}
	}
	// .asm_46807 / .asm_4683b
	hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x04);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0x00);
	wr8(gb, hYPosHi, gb->cpu.a);
	if (right) {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), 0x05);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0x00);
	} else {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x06);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0x00);
	}
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3513, BOT(right ? 0x2a0 : 0x26c));
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		if (right)
			JUMP(MoveObjectRight_Slow);
		else
			JUMP(MoveObjectLeft_Slow);
		return;
	}
	set_state(gb, 0x13);
	RET();
}

// Bumped.
static void bumped(GB *gb, uint8_t state)
{
	state_hld(gb, state);
	dir_frameset(gb, ADDR_Frameset_68eb1, ADDR_Frameset_68eb4);
	gb->cpu.a = 12;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl--, 12);
	SET_HL(gb, hl);
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjVar2, 0x02);
	collbox(gb);
	set_interaction(gb, OBJ_INTERACTION_01);
}

PORT_LOCAL(BeamBotFunc, Func_46874) { bumped(gb, 0x32); }
PORT_LOCAL(BeamBotFunc, Func_4687c) { bumped(gb, 0x33); }

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
	dir_frameset(gb, ADDR_Frameset_68eb7, ADDR_Frameset_68ec0);
	collbox(gb);
	set_interaction(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL_JUMP(BeamBotFunc, Func_468aa)
{
	start_slide(gb, 0x46);
	slide(gb, false);
}

PORT_LOCAL_JUMP(BeamBotFunc, Func_468cf) { slide(gb, false); }

PORT_LOCAL_JUMP(BeamBotFunc, Func_468dd)
{
	start_slide(gb, 0x47);
	slide(gb, true);
}

PORT_LOCAL_JUMP(BeamBotFunc, Func_46902) { slide(gb, true); }

static void func_4691e(GB *gb, uint8_t state)
{
	state_hld(gb, state);
	dir_frameset(gb, ADDR_Frameset_68eb7, ADDR_Frameset_68ec0);
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	ld_hli(gb, 0x02);
	ld_hli(gb, 0x00);
	gb->cpu.f = FLAG_Z; // xor a
	collbox(gb);
	set_interaction(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL(BeamBotFunc, Func_46910) { func_4691e(gb, 0x44); }
PORT_LOCAL(BeamBotFunc, Func_46918) { func_4691e(gb, 0x45); }

// Knocked over.
static void func_46954(GB *gb)
{
	gb->cpu.a = rd8(gb, wGroundShakeCounter);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (!(gb->cpu.f & FLAG_C)) {
		set_state(gb, 0x1c);
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

PORT_LOCAL_JUMP(BeamBotFunc, Func_46945)
{
	set_state(gb, 0x3f);
	gb->cpu.a = 0x64;
	wr8(gb, wCurObjStateDuration, 0x64);
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	set_bits(gb, hl, 0, OBJFLAG_GRABBED);
	func_46954(gb);
}

PORT_LOCAL_JUMP(BeamBotFunc, Func_46954) { func_46954(gb); }

// Gets up.
static void func_4697c(GB *gb)
{
	if (!dec_hl(gb, wCurObjStateDuration)) {
		JUMP(Func_32f9);
		return;
	}
	set_state(gb, 0x00);
	RET();
}

PORT_LOCAL_JUMP(BeamBotFunc, Func_4696e)
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
	func_4697c(gb);
}

PORT_LOCAL_JUMP(BeamBotFunc, Func_4697c) { func_4697c(gb); }

PORT_LOCAL(BeamBotFunc, Func_46989)
{
	state_hld(gb, 0x43);
	dir_frameset(gb, ADDR_Frameset_68ed3, ADDR_Frameset_68eda);
	ld_hli(gb, 17);
	collbox(gb);
	set_interaction(gb, OBJ_INTERACTION_01);
}

// Looks around.
PORT_LOCAL_JUMP(BeamBotFunc, Func_469b2)
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
	if (!dec_hl(gb, hl)) {
		JUMP(Func_3308);
		return;
	}
	set_state(gb, 0x01);
	RET();
}

// .asm_469f4: stunned
static void stunned(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjAction;
	gb->cpu.a = 0x8f;
	wr8(gb, hl--, 0x8f);
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	SET_HL(gb, hl);
	dir_frameset(gb, ADDR_Frameset_68eb7, ADDR_Frameset_68ec0);
	ld_hli(gb, 4);
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	ld_hli(gb, 0x02);
	ld_hli(gb, 0x00);
	gb->cpu.f = FLAG_Z; // xor a
	collbox(gb);
	set_interaction(gb, OBJ_INTERACTION_00);
}

PORT_LOCAL(BeamBotFunc, Func_469e2) { stunned(gb, 0x38); }
PORT_LOCAL(BeamBotFunc, Func_469ec) { stunned(gb, 0x39); }

// .asm_46a44: knocked away, b = speed
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
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_68ece : ADDR_Frameset_68ec9);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(BeamBotFunc, Func_46a1e) { knock(gb, 0x59); }
PORT_LOCAL_JUMP(BeamBotFunc, Func_46a28) { knock(gb, 0x58); }
PORT_LOCAL_JUMP(BeamBotFunc, Func_46a32) { knock(gb, 0x34); }
PORT_LOCAL_JUMP(BeamBotFunc, Func_46a3c) { knock(gb, 0x35); }

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

PORT_LOCAL_JUMP(BeamBotFunc, Func_46a62)
{
	thrown(gb, 0x36, 0xc0);
	JUMP(Func_3335);
}

PORT_LOCAL_JUMP(BeamBotFunc, Func_46a75)
{
	thrown(gb, 0x37, 0xe0);
	JUMP(Func_3344);
}

// Grabbed: .Func_46a88 ($49) and .Func_46ab9 ($48) turn with Wario.
static void grab(GB *gb, bool right)
{
	for (;;) {
		state_hld(gb, right ? 0x49 : 0x48);
		dir_frameset(gb, ADDR_Frameset_68eb7, ADDR_Frameset_68ec0);
		collbox(gb);
		set_bits(gb, lo_hl(gb, OBJ_FLAGS), OBJFLAG_GRABBED, 0);
		// .Func_46aa9 / .Func_46ada
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

PORT_LOCAL_JUMP(BeamBotFunc, Func_46a88) { grab(gb, true); }
PORT_LOCAL_JUMP(BeamBotFunc, Func_46aa9) { hold(gb, true); }
PORT_LOCAL_JUMP(BeamBotFunc, Func_46ab9) { grab(gb, false); }
PORT_LOCAL_JUMP(BeamBotFunc, Func_46ada) { hold(gb, false); }

PORT_LOCAL(BeamBotFunc, Func_46aeb)
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
