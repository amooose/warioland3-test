// Port of src/engine/level/objects/brrr_bear.asm
#include "ported.h"

#define SNOWFLAKE_UPDATE (ADDR_SnowflakeFunc + 0x1c) // .Update

PORT_JUMP(SnowflakeFunc_Left)
{
	CALL(MoveObjectLeft_Fast);
	JUMP_TO(ADDR_SnowflakeFunc);
}

PORT_JUMP(SnowflakeFunc_Right)
{
	CALL(MoveObjectRight_Fast);
	JUMP_TO(ADDR_SnowflakeFunc); // fallthrough
}

// Once its state changes, the snowflake shatters.
PORT(SnowflakeFunc)
{
	SET_HL(gb, wCurObjFlags);
	wr8(gb, wCurObjFlags, (uint8_t)(rd8(gb, wCurObjFlags) | OBJFLAG_NO_COLLISION));
	gb->cpu.a = rd8(gb, wCurObjState);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	SET_DE(gb, ADDR_Frameset_6916e);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 0x0c;
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, 0x0c); // OBJ_STATE_DURATION
	hl = (uint16_t)((hl & 0xff00) | (OBJ_UPDATE_FUNCTION + 1));
	gb->cpu.a = (uint8_t)(SNOWFLAKE_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)SNOWFLAKE_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}
