// Port of src/engine/overworld/epilogue/state_table.asm
#include "ported.h"

// rst JumpTable, with the table at `table` (right after the rst): leaves every
// register as JumpTable does and returns the target (= hl).
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

PORT_JUMP(EpilogueStateTable)
{
	gb->cpu.a = rd8(gb, wSubState);
	JUMP_TO(jumptable(gb, ADDR_EpilogueStateTable + 4));
}

PORT_LOCAL(EpilogueStateTable, FadeToWhite)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0b11);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	CALL(FastFadeToWhite);
}

PORT_LOCAL(EpilogueStateTable, Func_473c)
{
	FARCALL(Func_4628);
}
