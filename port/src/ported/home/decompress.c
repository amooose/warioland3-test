// Port of src/home/decompress.asm
#include "ported.h"

// decompresses data from hl and writes to bc
// the game stores some data in compressed form, such as tile gfx and bg maps
// this routine decompresses it by doing the following logic:
// - if bit 7 is not set, repeat the following byte that amount of times
// - if bit 7 is set, copy the next amount of bytes literally
// - a 0 byte ends the data
PORT(Decompress)
{
	uint16_t hl = REG_HL(gb), bc = REG_BC(gb);
	for (;;) {
		uint8_t cmd = rd8(gb, hl++);
		if (!cmd)
			break;
		if (cmd & 0x80) {
			// copies literal x bytes; a count of 0 means 256
			int n = (cmd & 0x7f) ? (cmd & 0x7f) : 256;
			for (int i = 0; i < n; i++)
				wr8(gb, bc++, rd8(gb, hl++));
		} else {
			// copies byte x times
			gb->cpu.e = rd8(gb, hl++);
			for (int i = 0; i < cmd; i++)
				wr8(gb, bc++, gb->cpu.e);
		}
		gb->cpu.d = 0;
	}
	SET_HL(gb, hl);
	SET_BC(gb, bc);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z | FLAG_H; // and a
}
