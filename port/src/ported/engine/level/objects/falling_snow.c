// Port of src/engine/level/objects/falling_snow.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define SPAWNER_FUNC_49544 (ADDR_FallingSnowSpawnerFunc + 0x17) // .Func_49544
#define SPAWNER_FUNC_49566 (ADDR_FallingSnowSpawnerFunc + 0x39) // .Func_49566
#define SPAWNER_FUNC_4959D (ADDR_FallingSnowSpawnerFunc + 0x70) // .Func_4959d
#define SNOW_FUNC_495EF (ADDR_FallingSnowFunc + 0x2d) // .Func_495ef
#define SNOW_FUNC_49610 (ADDR_FallingSnowFunc + 0x4e) // .Func_49610
#define SNOW_FUNC_49675 (ADDR_FallingSnowFunc + 0xb3) // .Func_49675
#define SNOW_FUNC_496AE (ADDR_FallingSnowFunc + 0xec) // .Func_496ae

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

// ld a, no_actions_for 1 / ld [wCurObjAction], a
static inline void no_actions_for_1(GB *gb)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, gb->cpu.a);
}

// ld hl, wCurObjStateDuration / dec [hl]; returns whether it hit 0
static bool dec_duration(GB *gb)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	return d == 0;
}

// call SetObjectFramesetPtr / ld a, v / ld [hli], a
static inline uint16_t set_frameset_duration(GB *gb, uint16_t frameset, uint8_t v)
{
	SET_DE(gb, frameset);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = v;
	wr8(gb, hl++, v);
	SET_HL(gb, hl);
	return hl;
}

// ld l, OBJ_FLAGS / bit OBJFLAG_ON_SCREEN_F, [hl] / ret z / play_sfx sfx
static void sfx_if_on_screen(GB *gb, uint16_t hl, uint16_t sfx)
{
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	if (!alu_bit(gb, rd8(gb, hl), OBJFLAG_ON_SCREEN_F))
		return;
	play_sfx(gb, sfx);
}

PORT(FallingSnowSpawnerFunc)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(SPAWNER_FUNC_49544 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)SPAWNER_FUNC_49544;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_FLAGS);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	SET_HL(gb, hl);
	gb->cpu.a = 0x1e;
	wr8(gb, wCurObjStateDuration, 0x1e);
}

// Waits, then drops a snowflake.
PORT_LOCAL_JUMP(FallingSnowSpawnerFunc, Func_49544)
{
	no_actions_for_1(gb);
	gb->cpu.a = 2;
	wr8(gb, wCurObjFrameDuration, 2);
	if (!dec_duration(gb)) {
		RET();
		return;
	}
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, 0x56);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	SET_HL(gb, set_update(gb, hl, SPAWNER_FUNC_49566));
	SET_BC(gb, ADDR_ObjParams_FallingSnow);
	JUMP(CreateObjectFromCurObjPos);
}

// Shows a puff of snow, sometimes a bit to the left.
PORT_LOCAL_JUMP(FallingSnowSpawnerFunc, Func_49566)
{
	no_actions_for_1(gb);
	gb->cpu.a = 2;
	wr8(gb, wCurObjFrameDuration, 2);
	if (!dec_duration(gb)) {
		RET();
		return;
	}
	gb->cpu.a = 0x0a;
	wr8(gb, wCurObjVar2, 0x0a);
	CALL(MoveObjectUpByVar2);
	set_update(gb, REG_HL(gb), SPAWNER_FUNC_4959D);
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	SET_HL(gb, hl);
	hl = set_frameset_duration(gb, ADDR_Frameset_695c0, 0x1d);
	gb->cpu.a = alu_rra(gb, rd8(gb, rDIV));
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	gb->cpu.a = 0x01;
	wr8(gb, hl, 0x01);
	JUMP(MoveObjectLeftByVar2);
}

PORT_LOCAL(FallingSnowSpawnerFunc, Func_4959d)
{
	no_actions_for_1(gb);
	if (!dec_duration(gb))
		return;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, 0x3c);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		CALL(MoveObjectRightByVar2);
	// .asm_495b1
	CALL(MoveObjectDownByVar2);
	hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_INVISIBLE));
	SET_HL(gb, set_update(gb, hl, SPAWNER_FUNC_49544));
}

PORT(FallingSnowFunc)
{
	no_actions_for_1(gb);
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	hl = set_frameset_duration(gb, ADDR_Frameset_6958f, 0x20);
	hl = set_update(gb, hl, SNOW_FUNC_495EF);
	sfx_if_on_screen(gb, hl, SFX_080);
}

PORT_LOCAL(FallingSnowFunc, Func_495ef)
{
	no_actions_for_1(gb);
	if (!dec_duration(gb))
		return;
	SET_DE(gb, ADDR_Frameset_695a6);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = set_update(gb, REG_HL(gb), SNOW_FUNC_49610);
	hl = LO(hl, OBJ_Y_POS);
	gb->cpu.a = alu_add(gb, rd8(gb, hl), 0x08);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	if (gb->cpu.f & FLAG_C)
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
}

// Copies the object's position to hYPos / hXPos and checks the ground under
// it; returns whether it's landed (a != 0).
static bool landed(GB *gb, uint16_t ret)
{
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_352b, ret);
	bool r = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	SET_BC(gb, ADDR_Data_603e0);
	if (r) {
		hl = wCurObjYPos;
		wr8(gb, hl++, rd8(gb, hYPosLo));
		gb->cpu.a = rd8(gb, hYPosHi);
		wr8(gb, hl, gb->cpu.a);
		SET_HL(gb, hl);
	}
	return r;
}

// Falls until it lands (or melts when hit).
PORT_LOCAL_JUMP(FallingSnowFunc, Func_49610)
{
	uint16_t hl;
	gb->cpu.a = rd8(gb, wCurObjState);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// .asm_49625
		if (!landed(gb, ADDR_FallingSnowFunc + 0x75)) {
			JUMP(ApplyObjYMovement);
			return;
		}
		hl = set_frameset_duration(gb, ADDR_Frameset_695af, 0x2d);
	} else {
		alu_cp(gb, gb->cpu.a, 0x18);
		if (gb->cpu.f & FLAG_Z) {
			// .asm_49666
			hl = wCurObjUpdateFunction + 1;
			gb->cpu.a = (uint8_t)(SNOW_FUNC_49675 >> 8);
			wr8(gb, hl--, gb->cpu.a);
			gb->cpu.a = (uint8_t)SNOW_FUNC_49675;
			wr8(gb, hl--, gb->cpu.a);
			SET_HL(gb, hl);
			SET_DE(gb, ADDR_Frameset_695d6);
			JUMP(SetObjectFramesetPtr);
			return;
		}
		hl = set_frameset_duration(gb, ADDR_Frameset_695cb, 0x11);
	}
	// .asm_49650
	hl = set_update(gb, hl, SNOW_FUNC_496AE);
	sfx_if_on_screen(gb, hl, SFX_0A1);
	RET();
}

PORT_LOCAL_JUMP(FallingSnowFunc, Func_49675)
{
	no_actions_for_1(gb);
	if (!landed(gb, ADDR_FallingSnowFunc + 0xca)) {
		JUMP(ApplyObjYMovement);
		return;
	}
	uint16_t hl = set_frameset_duration(gb, ADDR_Frameset_695d9, 0x07);
	SET_HL(gb, set_update(gb, hl, SNOW_FUNC_496AE));
	RET();
}

PORT_LOCAL(FallingSnowFunc, Func_496ae)
{
	no_actions_for_1(gb);
	if (!dec_duration(gb))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}
