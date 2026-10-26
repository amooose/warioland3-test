// Port of src/engine/level/tick_time.asm
#include "ported.h"

// Adds 1 to a BCD time digit pair; returns whether it reached $60 (ret c).
static bool tick(GB *gb, uint16_t addr)
{
	gb->cpu.a = alu_daa(gb, alu_add(gb, rd8(gb, addr), 1));
	wr8(gb, addr, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x60);
	return !(gb->cpu.f & FLAG_C);
}

// in Time Attack, adds a frame to wLevelTime (minutes, seconds, frames in
// BCD), capping at 59:59
PORT(TickLevelTime)
{
	gb->cpu.a = rd8(gb, wGameModeFlags);
	if (!alu_bit(gb, gb->cpu.a, MODE_TIME_ATTACK_F))
		return;
	gb->cpu.a = rd8(gb, wLevelEndScreen);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	for (int i = 2; i > 0; i--) {
		if (!tick(gb, wLevelTime + i))
			return;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wLevelTime + i, 0);
	}
	if (!tick(gb, wLevelTime + 0))
		return;
	gb->cpu.a = 0x59;
	wr8(gb, wLevelTime + 0, 0x59);
	wr8(gb, wLevelTime + 1, 0x59);
}
