// Port of src/engine/level/objects/muddee.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

// Inits the turtle, and destroys it if a TURTLE1 object is active.
PORT(Turtle2Func)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, (uint8_t)(ADDR_TurtleCommonFunc >> 8));
	wr8(gb, hl--, (uint8_t)ADDR_TurtleCommonFunc);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0x0a);
	wr8(gb, hl--, 0xf5); // OBJ_COLLBOX_LEFT
	wr8(gb, hl, 0x04); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_69d5c);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 0x10;
	wr8(gb, wCurObjVar2, 0x10);
	CALL(MoveObjectUpByVar2);

	gb->cpu.a = (uint8_t)wObj1;
	gb->cpu.b = (uint8_t)wObj1;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	for (;;) {
		// .asm_55e14
		gb->cpu.l = OBJ_ID;
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.l);
		gb->cpu.l = gb->cpu.a;
		gb->cpu.a = rd8(gb, REG_HL(gb)); // OBJ_ID
		alu_cp(gb, gb->cpu.a, TURTLE1);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = alu_sub(gb, gb->cpu.l, OBJ_ID - OBJ_FLAGS);
			gb->cpu.l = gb->cpu.a;
			gb->cpu.a = alu_rra(gb, rd8(gb, REG_HL(gb))); // OBJ_FLAGS
			if (gb->cpu.f & FLAG_C) {
				// .asm_55e2e
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z; // xor a
				wr8(gb, wCurObjFlags, 0);
				return;
			}
		}
		// .asm_55e25
		gb->cpu.a = gb->cpu.b;
		alu_cp(gb, gb->cpu.a, (uint8_t)wObj8);
		if (gb->cpu.f & FLAG_Z)
			return;
		gb->cpu.a = alu_add(gb, gb->cpu.a, OBJ_STRUCT_LENGTH);
		gb->cpu.b = gb->cpu.a;
	}
}

PORT(Func_55e33)
{
	uint16_t hl = wCurObjFrameDuration;
	gb->cpu.a = rd8(gb, hl++);
	SET_HL(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x04);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	play_sfx(gb, SFX_084);
}
