// Port of src/home/vblank.asm
#include "ported.h"

// wait_not_hblank / wait_hblank: poll rSTAT until the mode is (or isn't)
// HBlank, letting emulated time pass.
static void wait_not_hblank(GB *gb)
{
	while (!(gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
		idle(gb, 32);
}

static void wait_hblank(GB *gb)
{
	while ((gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
		idle(gb, 32);
}

// Waits for the start of the next HBlank.
PORT(WaitNextHBlank)
{
	wait_not_hblank(gb);
	wait_hblank(gb);
}
