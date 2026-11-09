// overworld/bank76 (src/engine/overworld/bank76.asm) as plain C++: called
// from the overworld's V-Blank functions (C++), which read no register
// after it. The animations are in overworld/animations.cpp.
#include "../native.hpp"
#include "../routines.hpp"
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

// the five VDMA registers from rVDMA_SRC_HIGH on; the write to rVDMA_LEN
// starts the transfer
void vdma(Memory mem, const uint8_t regs[5])
{
	for (int i = 0; i < 5; i++)
		mem[static_cast<uint16_t>(rVDMA_SRC_HIGH + i)] = regs[i];
}

} // namespace

void update_ow_tilemap_vdma(GB *gb)
{
	Memory mem(gb);
	mem[rVBK] = 1; // BANK("VRAM1")
	const uint16_t src = wTilemap + 0x24 * TILE_SIZE;
	const uint8_t tilemap[5] = {static_cast<uint8_t>(src >> 8), static_cast<uint8_t>(src), 0x17, 0x00, 0x0b};
	vdma(mem, tilemap);
	if (uint8_t hi = mem[w2d044 + 0]) {
		const uint8_t regs[5] = {hi, mem[w2d044 + 1], 0x0e, 0xc0, 0x13};
		vdma(mem, regs);
	}
	mem[rVBK] = 0;
}

} // namespace wl3::overworld

PORT_EX(Func_1d8c2c, 0, ALL_REGISTERS) { wl3::overworld::update_ow_tilemap_vdma(gb); }
