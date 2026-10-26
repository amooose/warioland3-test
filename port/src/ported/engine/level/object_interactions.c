// Port of src/engine/level/object_interactions.asm
#include "ported.h"

// b = OBJACTION_* constant
// Sets the action of the object at wObjPtr, and records which side Wario
// interacted with it from.
PORT(SetObjAction)
{
	uint16_t obj = (uint16_t)(rd8(gb, wObjPtr + 0) << 8 | rd8(gb, wObjPtr + 1));
	gb->cpu.d = 0;
	gb->cpu.e = OBJ_ACTION;
	uint16_t hl = alu_add16(gb, obj, OBJ_ACTION);
	wr8(gb, hl, gb->cpu.b);
	gb->cpu.h = (uint8_t)(hl >> 8);
	gb->cpu.l = alu_inc(gb, (uint8_t)hl); // inc l
	hl = REG_HL(gb);

	gb->cpu.b = rd8(gb, wInteractionSide);
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), 0x0f), gb->cpu.b); // OBJ_UNK_1D
	wr8(gb, hl, gb->cpu.a);
}

PORT(Func_20a63)
{
	gb->cpu.b = OBJACTION_07;
	CALL(SetObjAction);
}

PORT(Func_20a69)
{
	gb->cpu.b = OBJACTION_0D;
	CALL(SetObjAction);
}

PORT(Func_21cc9)
{
	gb->cpu.b = OBJACTION_13;
	CALL(SetObjAction);
}
