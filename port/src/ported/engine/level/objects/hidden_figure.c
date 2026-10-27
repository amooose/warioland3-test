// Port of src/engine/level/objects/hidden_figure.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define HIDDEN_FIGURE_FUNC_4CF55 (ADDR_HiddenFigureFaceFunc + 0x10e) // HiddenFigureFaceFunc.Func_4cf55

PORT(Func_4ce39)
{
	gb->cpu.a = 0x3c;
	wr8(gb, wCurObjStateDuration, 0x3c);
	gb->cpu.l = OBJ_UPDATE_FUNCTION + 1;
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = (uint8_t)(HIDDEN_FIGURE_FUNC_4CF55 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)HIDDEN_FIGURE_FUNC_4CF55;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// Reads the next value of the sequence at bc + [index] (the sequence ends with
// $80, after which its last value repeats) and advances the index.
static void next_offset(GB *gb, uint16_t index)
{
	SET_HL(gb, index);
	gb->cpu.a = alu_add(gb, rd8(gb, index), gb->cpu.c);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		gb->cpu.a = rd8(gb, REG_BC(gb));
	} else {
		wr8(gb, index, alu_inc(gb, rd8(gb, index)));
	}
}

// [addr] += next offset
static void add_offset(GB *gb, uint16_t index, uint16_t addr)
{
	next_offset(gb, index);
	SET_HL(gb, addr);
	gb->cpu.c = rd8(gb, addr);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	wr8(gb, addr, gb->cpu.a);
}

// The 16-bit position at hl's object's pos += next offset (sign-extended)
static void add_pos_offset(GB *gb, uint16_t index, uint8_t pos)
{
	next_offset(gb, index);
	uint16_t hl = LO(index, pos);
	alu_cp(gb, gb->cpu.a, 0x80);
	gb->cpu.c = rd8(gb, hl);
	bool negative = !(gb->cpu.f & FLAG_C);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	bool carry = gb->cpu.f & FLAG_C;
	if (!negative) {
		if (!carry)
			return;
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	} else {
		if (carry)
			return;
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	}
}

PORT(Func_4cf70) { add_offset(gb, wCurObjVar3, wHiddenFigureSCYShake); }
PORT(Func_4cf87) { add_offset(gb, wCurObjVar2, wc08b); }
PORT(Func_4cf9e) { add_pos_offset(gb, wCurObjVar3, OBJ_Y_POS); }
PORT(Func_4cfbf) { add_pos_offset(gb, wCurObjVar2, OBJ_X_POS); }
