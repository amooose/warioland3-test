// Port of src/engine/level/objects/doughnuteer.asm
#include "ported.h"

PORT(DoughnutFunc_Destroy)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}

// Sets the doughnut to fly towards Wario.
PORT(DoughnutFunc)
{
	gb->cpu.a = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	SET_BC(gb, (gb->cpu.f & FLAG_C) ? ADDR_DoughnutFunc + 0x21 : ADDR_DoughnutFunc + 0x26); // .Update_Left/Right
	gb->cpu.a = gb->cpu.b;
	wr8(gb, wCurObjUpdateFunction + 1, gb->cpu.b);
	wr8(gb, wCurObjUpdateFunction + 0, gb->cpu.c);
	SET_HL(gb, wCurObjUpdateFunction);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjVar3, 0);
}
