// Port of src/engine/level/objects/bird.asm
#include "ported.h"

#define BIRD(off) ((uint16_t)(ADDR_BirdFunc + (off)))
#define BIRD_INIT BIRD(0x0a) // .Init
#define BIRD_UPDATE BIRD(0xea) // .Update
#define BIRD_FUNC_43353 BIRD(0xdb) // .Func_43353
#define BIRD_FUNC_4340B BIRD(0x193) // .Func_4340b
#define BIRD_FUNC_43427 BIRD(0x1af) // .Func_43427

static uint16_t lo_hl(GB *gb, uint8_t l)
{
	gb->cpu.l = l;
	return REG_HL(gb);
}

// ld l, OBJ_UPDATE_FUNCTION + 1 / ld a, HIGH(x) / ld [hld], a / ld a, LOW(x) / ld [hld], a
static void set_update(GB *gb, uint16_t update)
{
	uint16_t hl = lo_hl(gb, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
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

PORT(BirdFunc)
{
	SET_HL(gb, wCurObjUpdateFunction + 1);
	set_update(gb, BIRD_INIT);
}

// Faces Wario.
PORT_LOCAL(BirdFunc, Init)
{
	SET_HL(gb, wCurObjFlags);
	set_bits(gb, wCurObjFlags, 0, OBJFLAG_INVISIBLE);
	set_update(gb, BIRD_UPDATE);
	uint16_t hl = lo_hl(gb, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x06);
	wr8(gb, hl--, 0xf9);
	gb->cpu.a = 0xfa;
	wr8(gb, hl, 0xfa);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjStateDuration, 0);
	SET_HL(gb, hl);
	hl = lo_hl(gb, OBJ_SUBSTATE);
	set_bits(gb, hl, 0, OBJSUBFLAG_UNINITIALISED);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C) {
		set_bits(gb, hl, OBJSUBFLAG_HDIR, 0);
		set_frameset(gb, ADDR_Frameset_687e4);
	} else {
		set_bits(gb, hl, 0, OBJSUBFLAG_HDIR);
		set_frameset(gb, ADDR_Frameset_687d3);
	}
}

// Swoops at Wario once he's close enough at about its height.
static void func_432c2(GB *gb)
{
	uint16_t hl = lo_hl(gb, OBJ_FLAGS);
	if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F))
		return;
	hl = lo_hl(gb, OBJ_STATE_DURATION);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d)
			return;
		hl = lo_hl(gb, OBJ_SUBSTATE);
		gb->cpu.a = alu_rlca(gb, rd8(gb, hl));
		if (!(gb->cpu.f & FLAG_C)) {
			set_bits(gb, hl, OBJSUBFLAG_HDIR, 0);
			set_frameset(gb, ADDR_Frameset_687e4);
		} else {
			set_bits(gb, hl, 0, OBJSUBFLAG_HDIR);
			set_frameset(gb, ADDR_Frameset_687d3);
		}
	}
	// .asm_432e7
	gb->cpu.c = 0x2a;
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenYPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), 0x2a);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (!(gb->cpu.f & FLAG_C)) {
		alu_cp(gb, gb->cpu.a, 0xe8);
		if (gb->cpu.f & FLAG_C)
			return;
	}
	hl = lo_hl(gb, OBJ_SUBSTATE);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	gb->cpu.b = gb->cpu.a;
	bool behind;
	if (!(gb->cpu.f & FLAG_C)) {
		behind = alu_bit(gb, rd8(gb, hl), OBJSUBFLAG_HDIR_F);
		if (!behind) {
			gb->cpu.a = gb->cpu.b;
			alu_cp(gb, gb->cpu.a, 0x70);
			if (!(gb->cpu.f & FLAG_C))
				return;
			set_frameset(gb, ADDR_Frameset_687c9);
		}
	} else {
		behind = !alu_bit(gb, rd8(gb, hl), OBJSUBFLAG_HDIR_F);
		if (!behind) {
			gb->cpu.a = gb->cpu.b;
			alu_cp(gb, gb->cpu.a, 0x90);
			if (gb->cpu.f & FLAG_C)
				return;
			set_frameset(gb, ADDR_Frameset_687ce);
		}
	}
	if (behind) {
		// .asm_43338
		set_frameset(gb, ADDR_Frameset_687ff);
		gb->cpu.a = 0x0c;
		hl = REG_HL(gb);
		wr8(gb, hl++, 0x0c);
		SET_HL(gb, hl);
		return;
	}
	// .asm_43327
	gb->cpu.a = 0x23;
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x23);
	SET_HL(gb, hl);
	gb->cpu.a = 0x2f;
	wr8(gb, wCurObjState, 0x2f);
	play_sfx(gb, SFX_06F);
}

PORT_LOCAL(BirdFunc, Func_432c2) { func_432c2(gb); }

static void fly(GB *gb);

static void func_43342(GB *gb)
{
	uint16_t hl = lo_hl(gb, OBJ_STATE_DURATION);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		fly(gb); // .asm_43358
		return;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d)
		play_sfx(gb, SFX_070);
	RET();
}

PORT_LOCAL_JUMP(BirdFunc, Func_43342) { func_43342(gb); }

// .asm_43358: flies on
static void fly(GB *gb)
{
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	if (gb->cpu.f & FLAG_C)
		JUMP(MoveObjectRight_Fast);
	else
		JUMP(MoveObjectLeft_Fast);
}

PORT_LOCAL_JUMP(BirdFunc, Func_43353)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, gb->cpu.a);
	fly(gb);
}

// .asm_433e8: a:c = the knocked-back update function, b = its speed
static void knocked(GB *gb)
{
	uint16_t hl = lo_hl(gb, OBJ_FLAGS);
	set_bits(gb, hl, OBJFLAG_NO_COLLISION, 0);
	hl = lo_hl(gb, OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.c);
	SET_HL(gb, hl);
	hl = lo_hl(gb, OBJ_VAR_2);
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	SET_HL(gb, hl);
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, gb->cpu.a);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl));
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_687fa : ADDR_Frameset_687f5);
	JUMP(SetObjectFramesetPtr);
}

static void knock(GB *gb, uint16_t func)
{
	gb->cpu.a = (uint8_t)(func >> 8);
	gb->cpu.c = (uint8_t)func;
	gb->cpu.b = 0x02;
	knocked(gb);
}

PORT_LOCAL_JUMP(BirdFunc, Update)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		func_432c2(gb);
		RET();
		return;
	}
	uint8_t a = gb->cpu.a;
	alu_cp(gb, a, 0x2f);
	if (gb->cpu.f & FLAG_Z) {
		func_43342(gb);
		return;
	}
	uint16_t params = 0;
	alu_cp(gb, a, 0x0b);
	if (gb->cpu.f & FLAG_Z) {
		params = ADDR_ObjParams_BirdStars1;
	} else {
		alu_cp(gb, a, 0x0c);
		if (gb->cpu.f & FLAG_Z)
			params = ADDR_ObjParams_BirdStars2;
	}
	if (params) {
		SET_BC(gb, params);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
		hl = lo_hl(gb, OBJ_FLAGS);
		if (alu_bit(gb, rd8(gb, hl), OBJFLAG_NO_COLLISION_F)) {
			hl = lo_hl(gb, OBJ_SUBSTATE);
			if (alu_bit(gb, rd8(gb, hl), OBJSUBFLAG_HDIR_F))
				knock(gb, ADDR_Func_33f8);
			else
				knock(gb, ADDR_Func_3407);
			return;
		}
		// .asm_433a7
		set_bits(gb, hl, OBJFLAG_NO_COLLISION, 0);
		gb->cpu.l = OBJ_UPDATE_FUNCTION + 1;
		gb->cpu.a = rd8(gb, wCurObjId);
		alu_cp(gb, gb->cpu.a, BLUE_BIRD);
		if (gb->cpu.f & FLAG_Z) {
			set_update(gb, BIRD_FUNC_4340B);
			gb->cpu.a = 0x14;
			wr8(gb, wCurObjStateDuration, 0x14);
			RET();
			return;
		}
		set_update(gb, BIRD_FUNC_43353);
		JUMP(CreateObjectAtRelativePos);
		return;
	}
	alu_cp(gb, a, 0x0a);
	if (gb->cpu.f & FLAG_Z) {
		JUMP(Func_3173);
		return;
	}
	alu_cp(gb, a, 0x28);
	if (gb->cpu.f & FLAG_Z) {
		knock(gb, ADDR_Func_33f8);
		return;
	}
	alu_cp(gb, a, 0x29);
	if (gb->cpu.f & FLAG_Z) {
		knock(gb, ADDR_Func_3407);
		return;
	}
	alu_cp(gb, a, 0x04);
	if (gb->cpu.f & FLAG_Z) {
		knock(gb, ADDR_Func_3326);
		return;
	}
	alu_cp(gb, a, 0x05);
	if (gb->cpu.f & FLAG_Z) {
		knock(gb, ADDR_Func_3317);
		return;
	}
	gb->cpu.a = alu_rra(gb, a);
	knock(gb, (gb->cpu.f & FLAG_C) ? ADDR_Func_33f8 : ADDR_Func_3407);
}

// The blue bird waits, then turns into the flying-off frameset.
PORT_LOCAL(BirdFunc, Func_4340b)
{
	uint16_t hl = wCurObjAction;
	gb->cpu.a = 0x81;
	wr8(gb, hl--, 0x81);
	SET_HL(gb, hl);
	hl = lo_hl(gb, OBJ_STATE_DURATION);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	set_frameset(gb, ADDR_Frameset_687ff);
	gb->cpu.a = 0x0c;
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x0c);
	SET_HL(gb, hl);
	set_update(gb, BIRD_FUNC_43427);
}

PORT_LOCAL_JUMP(BirdFunc, Func_43427)
{
	uint16_t hl = wCurObjAction;
	gb->cpu.a = 0x81;
	wr8(gb, hl--, 0x81);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		func_43342(gb);
		return;
	}
	hl = lo_hl(gb, OBJ_STATE_DURATION);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	hl = lo_hl(gb, OBJ_STATE);
	gb->cpu.a = 0x01;
	wr8(gb, hl--, 0x01);
	SET_HL(gb, hl);
	gb->cpu.a = alu_rlca(gb, rd8(gb, hl));
	if (gb->cpu.f & FLAG_C) {
		set_bits(gb, hl, 0, OBJSUBFLAG_HDIR);
		set_frameset(gb, ADDR_Frameset_687c9);
	} else {
		set_bits(gb, hl, OBJSUBFLAG_HDIR, 0);
		set_frameset(gb, ADDR_Frameset_687ce);
	}
	RET();
}
