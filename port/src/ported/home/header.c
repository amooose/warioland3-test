// Port of src/home/header.asm
#include "ported.h"

// rst $28: jumps to entry a of the table that follows the rst
PORT_JUMP(JumpTable)
{
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	uint16_t hl = gb_pop16(gb);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	hl = alu_add16(gb, hl, REG_DE(gb));
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	JUMP_TO(REG_HL(gb));
}
