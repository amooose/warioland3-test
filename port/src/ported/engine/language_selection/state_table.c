// Port of src/engine/language_selection/state_table.asm
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

#define ADDR_LanguageSelectionStateTable_jump (ADDR_LanguageSelectionStateTable + 0x0e)

PORT(LanguageSelectionStateTable)
{
	// push_wram BANK("GFX RAM")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 0x03;
	wr8(gb, rWBK, gb->cpu.a);
	gb_call(gb, ADDR_LanguageSelectionStateTable_jump);
	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

PORT_LOCAL_JUMP(LanguageSelectionStateTable, jump)
{
	gb->cpu.a = rd8(gb, wSubState);
	JUMP_TO(jumptable(gb, ADDR_LanguageSelectionStateTable_jump + 4));
}
