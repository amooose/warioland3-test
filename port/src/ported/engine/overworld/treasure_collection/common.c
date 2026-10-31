// Port of src/engine/overworld/treasure_collection/common.asm: SetObjPositionToLevelCoords (the wrappers are C++,
// src/native/overworld/treasure_collection.cpp)
#include "ported.h"

// de = scene obj
// c = level index in wCurMapSide
PORT(SetObjPositionToLevelCoords)
{
	gb->cpu.a = gb->cpu.c;
	gb_push16(gb, REG_DE(gb));
	gb_call(gb, ADDR_GetMapLevelCoords);
	SET_DE(gb, gb_pop16(gb));
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, REG_DE(gb), gb->cpu.a);
	gb->cpu.e = alu_inc(gb, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, REG_DE(gb), gb->cpu.a);
	SET_HL(gb, hl);
}

