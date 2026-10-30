// Port of src/home/double_speed.asm
#include "ported.h"

PORT(EnableDoubleSpeed)
{
	gb->cpu.a = rd8(gb, rSPD);
	alu_bit(gb, gb->cpu.a, 7);
	if (!(gb->cpu.f & FLAG_Z))
		return; // already in double speed

	gb->cpu.a = SPD_PREPARE;
	wr8(gb, rSPD, gb->cpu.a);
	gb->cpu.a = rd8(gb, rIE);
	push_af(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a: disable all interrupts
	wr8(gb, rIE, 0);
	gb->cpu.a = JOYP_GET_NONE;
	wr8(gb, rJOYP, gb->cpu.a);
	// stop: with the switch armed, it changes the CPU speed
	if (gb->cpu.speed_switch_armed) {
		gb->cpu.double_speed = !gb->cpu.double_speed;
		gb->cpu.speed_switch_armed = false;
	}
	gb->div_counter = 0;

	do { // .wait
		gb->cpu.a = rd8(gb, rSPD);
		alu_bit(gb, gb->cpu.a, 7);
	} while (gb->cpu.f & FLAG_Z);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rJOYP, 0);
	wr8(gb, rIF, 0);
	pop_af(gb);
	wr8(gb, rIE, gb->cpu.a);
}
