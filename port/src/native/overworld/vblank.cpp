// overworld/vblank (src/engine/overworld/vblank.asm): setting the
// overworld's V-Blank functions, and the functions themselves.
#include "../native.hpp"
#include "../home/ow.hpp"
#include "../home/memory.hpp"
#include "overworld.hpp"

namespace wl3::overworld {

void set_vblank_overworld(GB *gb)
{
	home::copy(gb, ADDR_VBlank_Overworld + 0x0c, wVBlankFunc, ADDR_VBlank_OWScene - (ADDR_VBlank_Overworld + 0x0c));
}

void set_vblank_ow_scene(GB *gb)
{
	home::copy(gb, ADDR_VBlank_OWScene + 0x0c, wVBlankFunc,
		ADDR_UnreferencedOpenTreasureCollection - (ADDR_VBlank_OWScene + 0x0c));
}

namespace {

// The overworld's V-Blank functions, as copied to wVBlankFunc (see
// ram_code.c): plain C++ over memory. Their calls return to base + n, the
// block's start wherever it runs from. They end in hTransferVirtualOAM,
// which takes a (wVirtualOAM's page) and returns into the V-Blank
// handler's pops.
class OverworldVblank {
public:
	explicit OverworldVblank(GB *gb) : gb_(gb), mem_(gb) {}

	void overworld(); // VBlank_Overworld.Func
	void ow_scene(); // VBlank_OWScene.Func

private:
	void switch_bank(uint8_t bank)
	{
		mem_[wROMBank] = bank;
		mem_[rROMB0 + 0x100] = bank;
	}
	// fn with bank mapped (ld a, [wROMBank] / push af / ld a, b /
	// bankswitch ... pop af / bankswitch)
	template <class F>
	void in_bank(uint8_t bank, F fn)
	{
		uint8_t rom = mem_[wROMBank];
		switch_bank(bank);
		fn();
		switch_bank(rom);
	}
	void hdma_from(uint16_t params);
	void hdma_15_rows(uint16_t src, uint16_t dest);
	void transfer_oam()
	{
		gb_->cpu.a = static_cast<uint8_t>(wVirtualOAM >> 8);
		gb_tail_jump(gb_, hTransferVirtualOAM);
	}

	GB *gb_;
	Memory mem_;
};

// the HDMA whose 5 parameters are at params, the first cleared
void OverworldVblank::hdma_from(uint16_t params)
{
	uint8_t first = mem_[params];
	mem_[params] = 0;
	mem_[rVDMA_SRC_HIGH] = first;
	for (int i = 1; i < 5; i++)
		mem_[static_cast<uint16_t>(rVDMA_SRC_HIGH + i)] = mem_[static_cast<uint16_t>(params + i)];
}

// the scroll and window; the sprites' update (Func_1d8c2c); wPalConfig1;
// the HDMAs at wHDMA (from wHDMABank) and w2d0b5 (to VRAM1)
void OverworldVblank::overworld()
{
	mem_[rWBK] = 0x02; // BANK("WRAM2")
	mem_[rSCY] = mem_[wSCY];
	mem_[rSCX] = mem_[wSCX];
	mem_[rWY] = mem_[wWY];
	mem_[rWX] = static_cast<uint8_t>(mem_[wWX] + WX_OFS);
	in_bank(BANK_Func_1d8c2c, [&] { update_ow_tilemap_vdma(gb_); });
	if (mem_[wPalConfig1])
		home::apply_pal_config(gb_, wPalConfig1);
	in_bank(mem_[wHDMABank], [&] {
		if (mem_[wHDMA])
			hdma_from(wHDMA);
		if (mem_[w2d0b5]) {
			mem_[rVBK] = 0x01; // BANK("VRAM1")
			hdma_from(w2d0b5);
		}
	});
	mem_[rVBK] = 0;
	transfer_oam();
}

// a 15-row HDMA from src to dest
void OverworldVblank::hdma_15_rows(uint16_t src, uint16_t dest)
{
	mem_[rVDMA_SRC_HIGH] = static_cast<uint8_t>(src >> 8);
	mem_[rVDMA_SRC_LOW] = 0;
	mem_[rVDMA_DEST_HIGH] = static_cast<uint8_t>((dest >> 8) - 0x80);
	mem_[rVDMA_DEST_LOW] = 0;
	mem_[rVDMA_LEN] = 15 * 2 - 1; // 15 dma_rows
}

// the scroll; a pending tilemap and attribute map update; the HDMA in
// w2d0b5 (from wHDMABank); the sprites' update; the palette configs
void OverworldVblank::ow_scene()
{
	mem_[rWBK] = 0x02;
	mem_[rSCY] = mem_[wSCY];
	mem_[rSCX] = mem_[wSCX];
	if (mem_[wOWPendingTileUpdate]) {
		hdma_15_rows(wTilemap, v0BGMap0);
		mem_[rVBK] = 0x01; // BANK("VRAM1")
		hdma_15_rows(wAttrmap, v1BGMap0);
		mem_[wOWPendingTileUpdate] = 0;
	}
	if (uint8_t bank = mem_[wHDMABank])
		in_bank(bank, [&] { home::start_hdma(gb_); });
	mem_[rVBK] = 0;
	in_bank(BANK_Func_1d8c2c, [&] { update_ow_tilemap_vdma(gb_); });
	in_bank(BANK_ApplyPalConfig1, [&] {
		if (mem_[wPalConfig1Register])
			home::apply_pal_config(gb_, wPalConfig1);
		if (mem_[wPalConfig2])
			home::apply_pal_config(gb_, wPalConfig2);
	});
	transfer_oam();
}

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldVblank;

PORT_LOCAL_EX(VBlank_Overworld, Func, 0, ALL_REGISTERS & ~R_A) { OverworldVblank(gb).overworld(); }
PORT_LOCAL_EX(VBlank_OWScene, Func, 0, ALL_REGISTERS & ~R_A) { OverworldVblank(gb).ow_scene(); }

// Only C++ modules set these now.
PORT_COVERED(VBlank_Overworld);
PORT_COVERED(VBlank_OWScene);
