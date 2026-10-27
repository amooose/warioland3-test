// Port of src/engine/level/objects/muddee_stinger.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define MUDDEE_STINGER_FUNC_4CA2A (ADDR_MuddeeStingerFunc + 0x14) // .Func_4ca2a

PORT(MuddeeStingerFunc)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_INVISIBLE | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << OBJSUBFLAG_UNK_4_F)));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(MUDDEE_STINGER_FUNC_4CA2A >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)MUDDEE_STINGER_FUNC_4CA2A;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// Adds the signed offset [de] to the 16-bit position at [hl] and stores it
// at dst (hl ends at the high byte).
static uint16_t add_offset(GB *gb, uint16_t hl, uint16_t dst)
{
	uint16_t de = REG_DE(gb);
	gb->cpu.a = rd8(gb, de);
	gb->cpu.c = gb->cpu.a;
	alu_cp(gb, gb->cpu.a, 0x80);
	bool neg = !(gb->cpu.f & FLAG_C);
	gb->cpu.a = alu_add(gb, rd8(gb, hl), gb->cpu.c);
	hl++;
	wr8(gb, dst, gb->cpu.a);
	bool carry = (gb->cpu.f & FLAG_C) != 0;
	if (!neg && carry)
		gb->cpu.a = alu_inc(gb, rd8(gb, hl));
	else if (neg && !carry)
		gb->cpu.a = alu_dec(gb, rd8(gb, hl));
	else
		gb->cpu.a = rd8(gb, hl);
	wr8(gb, dst + 1, gb->cpu.a);
	return hl;
}

// Follows the first live Muddee around, at the offset its frame gives.
PORT_LOCAL(MuddeeStingerFunc, Func_4ca2a)
{
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjFrameDuration, 0x02);
	gb->cpu.a = rd8(gb, wCurObjState);
	alu_cp(gb, gb->cpu.a, 0x0a);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0x01;
		wr8(gb, w1d147, 0x01);
	}
	gb->cpu.a = rd8(gb, w1d147);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurObjFlags, 0);
		return;
	}
	gb->cpu.a = (uint8_t)wObj1;
	gb->cpu.b = (uint8_t)wObj1;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	for (;;) {
		// .asm_4ca4c
		gb->cpu.l = OBJ_ID;
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.l);
		gb->cpu.l = gb->cpu.a;
		gb->cpu.a = rd8(gb, REG_HL(gb));
		alu_cp(gb, gb->cpu.a, MUDDEE);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = alu_sub(gb, gb->cpu.l, OBJ_ID - OBJ_FLAGS);
			gb->cpu.l = gb->cpu.a;
			gb->cpu.a = alu_rra(gb, rd8(gb, REG_HL(gb)));
			if (gb->cpu.f & FLAG_C)
				break;
		}
		// .asm_4ca5d
		gb->cpu.a = gb->cpu.b;
		alu_cp(gb, gb->cpu.a, (uint8_t)wObj8);
		if (gb->cpu.f & FLAG_Z)
			return;
		gb->cpu.a = alu_add(gb, gb->cpu.a, OBJ_STRUCT_LENGTH);
		gb->cpu.b = gb->cpu.a;
	}
	// .asm_4ca66
	gb->cpu.a = alu_add(gb, gb->cpu.l, 0x0f);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.c = rd8(gb, REG_HL(gb));
	gb->cpu.a = alu_sub(gb, gb->cpu.l, 0x0c);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.a = rd8(gb, wCurObjId);
	alu_cp(gb, gb->cpu.a, MUDDEE_STINGER2);
	SET_DE(gb, (gb->cpu.f & FLAG_Z) ? ADDR_Data_4cb0d : ADDR_Data_4caed);
	// .asm_4ca7e
	gb->cpu.a = gb->cpu.c;
	alu_cp(gb, gb->cpu.a, 0x10);
	uint16_t hl = REG_HL(gb);
	if (!(gb->cpu.f & FLAG_C)) {
		wr8(gb, wCurObjYPos + 0, rd8(gb, hl++));
		wr8(gb, wCurObjYPos + 1, rd8(gb, hl++));
		wr8(gb, wCurObjXPos + 0, rd8(gb, hl++));
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, wCurObjXPos + 1, gb->cpu.a);
		SET_HL(gb, hl);
		gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
		wr8(gb, wCurObjAction, gb->cpu.a);
		return;
	}
	// .asm_4ca99
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.e);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.a = alu_adc(gb, gb->cpu.d, 0);
	gb->cpu.d = gb->cpu.a;
	hl = add_offset(gb, hl, wCurObjYPos);
	// .asm_4cac6
	hl = LO(hl, alu_inc(gb, (uint8_t)hl)); // inc l
	SET_DE(gb, (uint16_t)(REG_DE(gb) + 1));
	hl = add_offset(gb, hl, wCurObjXPos);
	SET_HL(gb, hl);
}
