// Port of src/engine/level/objects/electric_lamp.asm
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

PORT(ElectricLampFunc)
{
	uint16_t hl = show_no_collision(gb);
	hl = set_update(gb, hl, ADDR_ElectricLampFunc + 0x1a); // .Discharged
	SET_HL(gb, set_initialised(gb, hl));
	SET_DE(gb, ADDR_Frameset_68cc4);
	CALL(SetObjectFramesetPtr);
}

#define ELECTRIC_LAMP_SPARK_GO_LEFT (ADDR_ElectricLampSparkFunc + 0x5a) // .GoLeft
#define ELECTRIC_LAMP_SPARK_GO_RIGHT (ADDR_ElectricLampSparkFunc + 0x66) // .GoRight

// Falls until it reaches the ground, then runs towards Wario.
PORT(ElectricLampSparkFunc)
{
	SET_HL(gb, wCurObjFlags);
	wr8(gb, wCurObjFlags, (uint8_t)(rd8(gb, wCurObjFlags) | OBJFLAG_NO_COLLISION));
	SET_BC(gb, ADDR_Data_60040);
	CALL(ApplyObjYMovement);
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_352b, ADDR_ElectricLampSparkFunc + 0x1d);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;

	// reached ground
	hl = wCurObjYPos;
	wr8(gb, hl++, rd8(gb, hYPosLo));
	gb->cpu.a = rd8(gb, hYPosHi);
	wr8(gb, hl, gb->cpu.a);
	hl = LO(hl, OBJ_COLLBOX_TOP);
	wr8(gb, hl++, (uint8_t)-6);
	gb->cpu.a = 5;
	wr8(gb, hl, 5);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68ca0);
	CALL(SetObjectFramesetPtr);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ELECTRIC_LAMP_SPARK_GO_RIGHT : ELECTRIC_LAMP_SPARK_GO_LEFT);
	// .update_function
	hl = LO(REG_HL(gb), OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, hl--, gb->cpu.d);
	wr8(gb, hl, gb->cpu.e);
	SET_HL(gb, hl);
	play_sfx(gb, SFX_06C);
}
