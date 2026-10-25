// Port of src/home/move_wario.asm
#include "ported.h"

// Adds (or subtracts) b to a 16-bit big-endian position: the low byte with
// add/sub, then the carry into the high byte with adc/sbc 0.
static void add_to_pos(GB *gb, uint16_t pos, bool subtract)
{
	uint8_t lo = rd8(gb, pos + 1);
	lo = subtract ? alu_sub(gb, lo, gb->cpu.b) : alu_add(gb, lo, gb->cpu.b);
	wr8(gb, pos + 1, lo);
	uint8_t hi = rd8(gb, pos);
	if (subtract) {
		// sbc 0
		unsigned borrow = (gb->cpu.f & FLAG_C) ? 1 : 0;
		gb->cpu.f = (uint8_t)(((uint8_t)(hi - borrow) ? 0 : FLAG_Z) | FLAG_N |
			((hi & 0xf) < borrow ? FLAG_H : 0) | (hi < borrow ? FLAG_C : 0));
		hi = (uint8_t)(hi - borrow);
	} else {
		hi = alu_adc(gb, hi, 0);
	}
	wr8(gb, pos, hi);
	gb->cpu.a = hi;
}

static void move_camera(GB *gb, uint16_t delta, bool subtract)
{
	uint8_t v = rd8(gb, delta);
	v = subtract ? alu_sub(gb, v, gb->cpu.b) : alu_add(gb, v, gb->cpu.b);
	wr8(gb, delta, v);
}

PORT(MoveWarioRight)
{
	move_camera(gb, wCameraXDelta, false);
	add_to_pos(gb, wWarioXPos, false);
}

PORT(MoveWarioLeft)
{
	move_camera(gb, wCameraXDelta, true);
	add_to_pos(gb, wWarioXPos, true);
}

// b = y offset
PORT(MoveWarioDown)
{
	move_camera(gb, wCameraYDelta, false);
	add_to_pos(gb, wWarioYPos, false);
}

PORT(MoveWarioDown_WithoutCameraOffset)
{
	add_to_pos(gb, wWarioYPos, false);
}

// b = y offset
PORT(MoveWarioUp)
{
	move_camera(gb, wCameraYDelta, true);
	add_to_pos(gb, wWarioYPos, true);
}

PORT(MoveWarioUp_WithoutCameraOffset)
{
	add_to_pos(gb, wWarioYPos, true);
}
