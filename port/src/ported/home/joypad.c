// Port of src/home/joypad.asm
#include "ported.h"

// Read the joypad register and translate it to something more workable for
// use in-game: one byte with the d-pad in the high nybble and the buttons in
// the low nybble.
PORT(UpdateJoypad)
{
	wr8(gb, rJOYP, JOYP_GET_CTRL_PAD);
	uint8_t dpad = (uint8_t)(~rd8(gb, rJOYP) & 0x0f);
	gb->cpu.b = (uint8_t)(dpad << 4);

	wr8(gb, rJOYP, JOYP_GET_BUTTONS);
	uint8_t buttons = (uint8_t)(~rd8(gb, rJOYP) & 0x0f);
	gb->cpu.c = gb->cpu.b | buttons;

	// only bits newly set this frame count as pressed
	uint8_t pressed = (rd8(gb, wJoypadDown) ^ gb->cpu.c) & gb->cpu.c;
	wr8(gb, wJoypadPressed, pressed);
	wr8(gb, wJoypadDown, gb->cpu.c);

	wr8(gb, rJOYP, JOYP_GET_NONE);
	gb->cpu.a = JOYP_GET_NONE;
	// flags from the final and c
	gb->cpu.f = (uint8_t)(FLAG_H | (pressed ? 0 : FLAG_Z));
}
