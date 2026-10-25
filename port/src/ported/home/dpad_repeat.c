// Port of src/home/dpad_repeat.asm
#include "ported.h"

// adds a Repeat Delay and Rate to any DPad key
PORT(ProcessDPadRepeat)
{
	// push_wram BANK("GFX RAM")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = BANK_wDPadRepeatCounter;
	wr8(gb, rWBK, gb->cpu.a);

	uint16_t hl = wDPadRepeatCounter;
	SET_HL(gb, hl);
	uint8_t v = 0x10; // reset_counter
	bool set = true;
	if (rd8(gb, hl) && !(rd8(gb, wJoypadPressed) & PAD_CTRL_PAD)) {
		uint8_t down = rd8(gb, wJoypadDown) & PAD_CTRL_PAD;
		if (down) {
			uint8_t cnt = (uint8_t)(rd8(gb, hl) - 1);
			wr8(gb, hl, cnt);
			if (cnt) {
				set = false;
			} else {
				// add down Joypad buttons to the pressed buttons
				gb->cpu.b = down;
				wr8(gb, wJoypadPressed, rd8(gb, wJoypadPressed) | down);
				v = 0x08;
			}
		}
	}
	if (set)
		wr8(gb, hl, v);

	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}
