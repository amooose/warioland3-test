// Port of src/engine/action_help/special_power_ups.asm
#include "ported.h"

// rst JumpTable, with the table at `table` (right after the rst). Leaves
// every register as JumpTable does and returns the target (= hl).
static uint16_t jumptable(GB *gb, uint16_t table)
{
	gb_push16(gb, table); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	return REG_HL(gb);
}

PORT_JUMP(ExecuteSpecialPowerUpActionHelp)
{
	gb->cpu.a = alu_sub(gb, rd8(gb, wActionHelpPowerUp), POWER_UP_OWL);
	JUMP_TO(jumptable(gb, ADDR_ExecuteSpecialPowerUpActionHelp + 6)); // table
}
