// Port of src/engine/level/objects/omodonmeka.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define OMODONMEKA_FUNC_42535 (ADDR_OmodonmekaWithOmodonFunc + 0x4b) // .Func_42535
#define OMODONMEKA_FUNC_42559 (ADDR_OmodonmekaWithOmodonFunc + 0x6f) // .Func_42559

// Spawns the Omodon riding the Omodonmeka (unless Func_3444 says otherwise).
static void omodonmeka_with_omodon(GB *gb, uint16_t omodon)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	wr8(gb, hl--, (uint8_t)(ADDR_OmodonmekaWithOmodonFunc >> 8));
	wr8(gb, hl--, (uint8_t)ADDR_OmodonmekaWithOmodonFunc);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	wr8(gb, hl--, 0xfd);
	wr8(gb, hl--, 0xf3); // OBJ_COLLBOX_LEFT
	gb->cpu.a = 0xf9;
	wr8(gb, hl--, 0xf9); // OBJ_COLLBOX_BOTTOM
	SET_HL(gb, hl);
	CALL(Func_3444);
	gb->cpu.a = gb->cpu.e;
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		JUMP_TO(OMODONMEKA_FUNC_42559);
		return;
	}
	SET_BC(gb, omodon);
	CALL(CreateObjectAtRelativePos);
	SET_DE(gb, ADDR_Frameset_68f14);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 0x00;
	hl = REG_HL(gb);
	wr8(gb, hl++, 0x00);
	SET_HL(gb, hl);
	wr8(gb, wCurObjVar3, 0x00);
	JUMP_TO(OMODONMEKA_FUNC_42535);
}

PORT_JUMP(OmodonmekaWithOmodon1Func) { omodonmeka_with_omodon(gb, ADDR_ObjParams_Omodon1); }
PORT_JUMP(OmodonmekaWithOmodon2Func) { omodonmeka_with_omodon(gb, ADDR_ObjParams_Omodon2); }

PORT(Func_42725)
{
	uint16_t hl = wCurObjVar3;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.b = (uint8_t)(ADDR_Data_40000 >> 8);
	gb->cpu.a = alu_add(gb, gb->cpu.a, (uint8_t)ADDR_Data_40000);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = alu_sub(gb, gb->cpu.c, 0x3f);
		gb->cpu.c = gb->cpu.a;
		gb->cpu.a = rd8(gb, REG_BC(gb));
		gb->cpu.c = 0x01;
		wr8(gb, hl, 0x01);
		hl = LO(hl, OBJ_STATE_DURATION);
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	} else {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	}
	SET_HL(gb, hl);
	// .apply_y_offset
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	hl = LO(hl, OBJ_Y_POS);
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

PORT(Func_42754)
{
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << OBJFLAG_STEPPED_F)));
	hl = LO(hl, OBJ_STATE_DURATION);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	uint8_t d = alu_dec(gb, gb->cpu.a);
	wr8(gb, hl, d);
	if (d)
		return;
	SET_DE(gb, ADDR_Frameset_68f50);
	CALL(SetObjectFramesetPtr);
}
