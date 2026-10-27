// Port of src/engine/level/objects/unused_flower.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define FLOWER_PROJECTILE_DESTROY (ADDR_UnusedFlowerProjectileCommonFunc + 0x85) // .Destroy

PORT_JUMP(UnusedFlowerProjectileRightFunc)
{
	CALL(MoveObjectRight);
	JUMP_TO(ADDR_UnusedFlowerProjectileCommonFunc);
}

PORT_JUMP(UnusedFlowerProjectileLeftFunc)
{
	CALL(MoveObjectLeft);
	JUMP_TO(ADDR_UnusedFlowerProjectileCommonFunc); // fallthrough
}

// hPos = the object's position, with y - y_off
static void set_hpos(GB *gb, uint16_t hl, uint8_t y_off)
{
	if (y_off) {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), y_off);
		wr8(gb, hYPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
		wr8(gb, hYPosHi, gb->cpu.a);
	} else {
		wr8(gb, hYPosLo, rd8(gb, hl++));
		wr8(gb, hYPosHi, rd8(gb, hl++));
	}
	wr8(gb, hXPosLo, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT(UnusedFlowerProjectileCommonFunc)
{
	const uint16_t routine = ADDR_UnusedFlowerProjectileCommonFunc;
	gb->cpu.a = rd8(gb, wCurObjState);
	bool thrown = !alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!thrown) {
		alu_cp(gb, gb->cpu.a, OBJSTATE_SPECIAL_1);
		if (gb->cpu.f & FLAG_Z) {
			// .Roll
			set_hpos(gb, LO(REG_HL(gb), OBJ_Y_POS), 4);
			SET_HL(gb, REG_HL(gb) + 1); // ld a, [hli]
			gb_call_ret(gb, ADDR_Func_3513, routine + 0x36);
			if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
				set_hpos(gb, wCurObjYPos, 0);
				gb_call_ret(gb, ADDR_Func_3543, routine + 0x4b);
				if (alu_and(gb, gb->cpu.a, gb->cpu.a))
					return;
				gb->cpu.a = OBJSTATE_SPECIAL_2;
				wr8(gb, wCurObjState, OBJSTATE_SPECIAL_2);
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z; // xor a
				wr8(gb, wCurObjVar3, 0);
				return;
			}
			goto set_destroy;
		}
		alu_cp(gb, gb->cpu.a, OBJSTATE_SPECIAL_2);
		if (!(gb->cpu.f & FLAG_Z))
			goto set_destroy;
	}
	// .Jump / .Thrown
	SET_BC(gb, thrown ? ADDR_Data_60320 : ADDR_Data_60320 + 8);
	CALL(ApplyObjYMovement);
	set_hpos(gb, wCurObjYPos, 0);
	gb_call_ret(gb, ADDR_Func_352b, routine + 0x74);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	wr8(gb, wCurObjYPos + 0, rd8(gb, hYPosLo));
	wr8(gb, wCurObjYPos + 1, rd8(gb, hYPosHi));
	SET_HL(gb, wCurObjYPos + 1);
	gb->cpu.a = OBJSTATE_SPECIAL_1;
	wr8(gb, wCurObjState, OBJSTATE_SPECIAL_1);
	return;

set_destroy:;
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(FLOWER_PROJECTILE_DESTROY >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)FLOWER_PROJECTILE_DESTROY;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68389);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 8;
	hl = REG_HL(gb);
	wr8(gb, hl++, 8); // OBJ_STATE_DURATION
	SET_HL(gb, hl);
}
