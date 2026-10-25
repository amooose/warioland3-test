// Port of src/home/floor_transition.asm
#include "ported.h"

// returns in c:
// 6 if   $0 <= ypos <  $80
// 5 if  $80 <= ypos < $100
// 4 if $100 <= ypos < $180
// 3 if $180 <= ypos < $200
// 2 if $200 <= ypos < $280
// 1 if $280 <= ypos < $300
PORT(GetFloorForYPos)
{
	uint8_t hi = alu_dec(gb, rd8(gb, wWarioYPos + 0));
	if (!hi)
		gb->cpu.c = 3;
	else if (!alu_dec(gb, hi))
		gb->cpu.c = 1;
	else
		gb->cpu.c = 5;

	gb->cpu.a = rd8(gb, wWarioYPos + 1);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_C)
		gb->cpu.c = alu_inc(gb, gb->cpu.c); // < $80
}
