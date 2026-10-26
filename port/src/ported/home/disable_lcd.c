// Port of src/home/disable_lcd.asm
#include "ported.h"

// Turns the screen off, which the hardware only allows during VBlank.
PORT(DisableLCD)
{
	gb->cpu.a = rd8(gb, rLCDC);
	if (!alu_bit(gb, gb->cpu.a, B_LCDC_ENABLE))
		return; // already disabled

	gb->cpu.a = rd8(gb, rIE);
	wr8(gb, wTempIE, gb->cpu.a);
	// disable VBlank interrupt
	gb->cpu.a &= (uint8_t)~IE_VBLANK;
	wr8(gb, rIE, gb->cpu.a);

	// wait until VBlank would normally happen
	for (;;) {
		gb->cpu.a = rd8(gb, rLY);
		alu_cp(gb, gb->cpu.a, LY_VBLANK + 1);
		if (gb->cpu.f & FLAG_Z)
			break;
		idle(gb, 32);
	}

	// turn LCD off
	gb->cpu.a = alu_and(gb, rd8(gb, rLCDC), 0xff ^ LCDC_ON);
	wr8(gb, rLCDC, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rIF, 0);
	gb->cpu.a = rd8(gb, wTempIE);
	wr8(gb, rIE, gb->cpu.a);
}
