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

// DoughnutFunc_LeftRight and DoughnutFunc_Up.Update: destroys the doughnut
// or sends it at Wario depending on its state, else moves it with movement.
static void doughnut_update(GB *gb, uint16_t movement)
{
	gb->cpu.a = rd8(gb, wCurObjState);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_DoughnutFunc_Destroy);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x19);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_DoughnutFunc);
		return;
	}
	SET_BC(gb, movement);
	JUMP(ApplyObjYMovement);
}

PORT_JUMP(DoughnutFunc_LeftRight)
{
	gb->cpu.l = OBJ_FLAGS;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	doughnut_update(gb, ADDR_Data_60510);
}

PORT_JUMP(DoughnutFunc_Up)
{
	uint16_t update = ADDR_DoughnutFunc_Up + 0x11; // .Update
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	hl = (uint16_t)((hl & 0xff00) | (uint8_t)((hl & 0xff) - 1)); // dec l
	gb->cpu.a = 0x8f;
	wr8(gb, hl, 0x8f);
	hl = (uint16_t)((hl & 0xff00) | OBJ_FLAGS);
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	doughnut_update(gb, ADDR_Data_60540);
}
