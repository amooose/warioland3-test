// Port of src/engine/level/objects/sun_moon.asm
#include "ported.h"

#define SUN_FLAME_UPDATE (ADDR_SunFlameFunc + 0x20) // .Update
#define MOON_STAR_UPDATE (ADDR_MoonStarFunc + 0x20) // .Update

// hYPos = [wCurObjYPos] - y (16-bit), hl = wCurObjYPos + 2
static void set_hypos_sub(GB *gb, uint8_t y)
{
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), y);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	wr8(gb, hYPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

// Moves the sun/moon horizontally by var2 unless a wall is in the way.
static void sun_moon_move_x(GB *gb, bool left, uint16_t routine)
{
	set_hypos_sub(gb, 6);
	uint16_t hl = REG_HL(gb);
	if (left) {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 12);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	} else {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), 12);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
	}
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3513, (uint16_t)(routine + 0x1a));
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	set_hypos_sub(gb, 20);
	gb_call_ret(gb, ADDR_Func_3513, (uint16_t)(routine + 0x2c));
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	if (left)
		JUMP(MoveObjectLeftByVar2);
	else
		JUMP(MoveObjectRightByVar2);
}

PORT_JUMP(SunMoon_MoveRight) { sun_moon_move_x(gb, false, ADDR_SunMoon_MoveRight); }
PORT_JUMP(SunMoon_MoveLeft) { sun_moon_move_x(gb, true, ADDR_SunMoon_MoveLeft); }

PORT_JUMP(SunMoon_MoveDown)
{
	uint16_t hl = wCurObjYPos;
	wr8(gb, hYPosLo, rd8(gb, hl++));
	wr8(gb, hYPosHi, rd8(gb, hl++));
	wr8(gb, hXPosLo, rd8(gb, hl++));
	wr8(gb, hXPosHi, rd8(gb, hl));
	SET_HL(gb, hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wc1ca, 0);
	gb_call_ret(gb, ADDR_Func_352b, ADDR_SunMoon_MoveDown + 0x16);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = rd8(gb, wc1ca);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			RET();
			return;
		}
	}
	JUMP(MoveObjectDownByVar2);
}

PORT_JUMP(SunMoon_MoveUp)
{
	set_hypos_sub(gb, 24);
	uint16_t hl = REG_HL(gb);
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_358b, ADDR_SunMoon_MoveUp + 0x16);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	JUMP(MoveObjectUpByVar2);
}

// Moves with bc's movement; once its state changes, it bursts.
static void flame_star_func(GB *gb, uint16_t update, uint16_t frameset)
{
	CALL(ApplyObjXMovement);
	SET_HL(gb, wCurObjFlags);
	wr8(gb, wCurObjFlags, (uint8_t)(rd8(gb, wCurObjFlags) | OBJFLAG_NO_COLLISION));
	gb->cpu.a = rd8(gb, wCurObjState);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, frameset);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 14;
	hl = REG_HL(gb);
	wr8(gb, hl++, 14); // OBJ_STATE_DURATION
	SET_HL(gb, hl);
}

PORT_JUMP(SunFlameFunc_Left)
{
	SET_BC(gb, ADDR_Data_606e0);
	JUMP_TO(ADDR_SunFlameFunc);
}

PORT_JUMP(SunFlameFunc_Right)
{
	SET_BC(gb, ADDR_Data_606f0);
	JUMP_TO(ADDR_SunFlameFunc); // fallthrough
}

PORT(SunFlameFunc) { flame_star_func(gb, SUN_FLAME_UPDATE, ADDR_Frameset_68fc9); }

PORT_JUMP(MoonStarFunc_Left)
{
	SET_BC(gb, ADDR_Data_606e0);
	JUMP_TO(ADDR_MoonStarFunc);
}

PORT_JUMP(MoonStarFunc_Right)
{
	SET_BC(gb, ADDR_Data_606f0);
	JUMP_TO(ADDR_MoonStarFunc); // fallthrough
}

PORT(MoonStarFunc) { flame_star_func(gb, MOON_STAR_UPDATE, ADDR_Frameset_69024); }
