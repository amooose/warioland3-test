// Port of src/engine/level/wario_states_3.asm
#include "ported.h"

PORT(DecrementTransformationDuration)
{
	// 16-bit big-endian counter
	uint8_t lo = alu_sub(gb, rd8(gb, wTransformationDuration + 1), 1);
	wr8(gb, wTransformationDuration + 1, lo);
	gb->cpu.a = alu_sbc(gb, rd8(gb, wTransformationDuration), 0);
	wr8(gb, wTransformationDuration, gb->cpu.a);
	SET_HL(gb, wTransformationDuration);
}

PORT(Func_28c15)
{
	gb->cpu.a = FALLING_JUMP_VEL_INDEX;
	wr8(gb, wJumpVelIndex, gb->cpu.a);
	gb->cpu.a = JUMP_VEL_HIGH_JUMP;
	wr8(gb, wJumpVelTable, gb->cpu.a);
	gb->cpu.a = WST_BALL_O_STRING_AIRBORNE;
	wr8(gb, wWarioState, gb->cpu.a);
}
