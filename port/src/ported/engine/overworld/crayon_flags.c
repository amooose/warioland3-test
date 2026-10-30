// Port of src/engine/overworld/crayon_flags.asm
#include "ported.h"

// gets from wTreasuresCollected the crayon treasure flags into wCrayonFlags
PORT(GetCrayonFlags)
{
	uint16_t hl = wTreasuresCollected + (CRAYON_YELLOW_T / 8);
	gb->cpu.a = rd8(gb, hl--);
	gb->cpu.c = rd8(gb, hl);
	gb->cpu.c = alu_sla(gb, gb->cpu.c);
	gb->cpu.a = alu_rla(gb, gb->cpu.a);
	gb->cpu.c = alu_sla(gb, gb->cpu.c);
	gb->cpu.a = alu_rla(gb, gb->cpu.a);
	gb->cpu.a &= (uint8_t)~0x80; // res 7, a
	wr8(gb, wCrayonFlags, gb->cpu.a);
	SET_HL(gb, hl);
}
