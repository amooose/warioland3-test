// Port of src/engine/level/state_table.asm
#include "ported.h"

// ld a, [wSubState] / rst JumpTable, with the table after the rst.
PORT_JUMP(_LevelStateTable)
{
	gb->cpu.a = rd8(gb, wSubState);
	gb_push16(gb, ADDR__LevelStateTable + 4); // rst JumpTable
	uint16_t hl = gb_pop16(gb);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	JUMP_TO(REG_HL(gb));
}
