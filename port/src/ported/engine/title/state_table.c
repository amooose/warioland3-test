// Port of src/engine/title/state_table.asm
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

PORT(TitleStateTable)
{
	// push_wram BANK("GFX RAM")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 0x03;
	wr8(gb, rWBK, gb->cpu.a);
	// call .jump: ld a, [wSubState] / jumptable
	gb_push16(gb, ADDR_TitleStateTable + 0x0a);
	gb->cpu.a = rd8(gb, wSubState);
	uint16_t target = jumptable(gb, ADDR_TitleStateTable + 0x12);
	gb_pop16(gb);
	gb_call_ret(gb, target, ADDR_TitleStateTable + 0x0a);
	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}
