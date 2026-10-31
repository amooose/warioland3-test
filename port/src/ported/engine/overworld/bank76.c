// Port of src/engine/overworld/bank76.asm: Func_1d8c2c, which V-Blank code
// calls (the animations are C++, src/native/overworld/animations.cpp)
#include "ported.h"

// Writes the five VDMA registers from rVDMA_SRC_HIGH on, as the asm does
// with ld [$ff00+c], a / inc c; the write to rVDMA_LEN starts the transfer.
static void vdma(GB *gb, const uint8_t regs[5])
{
	gb->cpu.c = (uint8_t)rVDMA_SRC_HIGH;
	for (int i = 0; i < 5; i++) {
		if (i)
			gb->cpu.c = alu_inc(gb, gb->cpu.c);
		gb->cpu.a = regs[i];
		wr8(gb, (uint16_t)(0xff00 | gb->cpu.c), gb->cpu.a);
	}
}

PORT(Func_1d8c2c)
{
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, 1);
	const uint16_t src = wTilemap + 0x24 * TILE_SIZE;
	const uint8_t tilemap[5] = {(uint8_t)(src >> 8), (uint8_t)src, 0x17, 0x00, 0x0b};
	vdma(gb, tilemap);

	gb->cpu.a = rd8(gb, w2d044 + 0);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		const uint8_t regs[5] = {gb->cpu.a, rd8(gb, w2d044 + 1), 0x0e, 0xc0, 0x13};
		vdma(gb, regs);
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}
