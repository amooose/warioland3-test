// clear/vblank (src/engine/clear/vblank.asm): the clear screens' V-Blank
// functions.
#include "../asm_module.hpp"

namespace wl3::clear {

namespace {

class ClearVblank : public AsmModule {
public:
	using AsmModule::AsmModule;

	// The clear screens' V-Blank functions, as copied to wVBlankFunc,
	// plain over memory: the V-Blank handler pushes and pops every register
	// around them. They call hTransferVirtualOAM (a = wVirtualOAM's page)
	// at the end.

	// a tile's bit plane rotated down a row: from src (its last row's byte)
	// up, each byte the one 2 above (dec l and dec e x2 stay in the page)
	void rotate_tile_plane(uint16_t src)
	{
		uint8_t last = mem_[src];
		uint16_t at = src;
		uint8_t from = static_cast<uint8_t>(src - 2);
		for (int i = 0; i < TILE_SIZE - 1; i++) {
			mem_[at] = mem_[static_cast<uint16_t>((src & 0xff00) | from)];
			at = static_cast<uint16_t>(at - 1); // ld [hld], a
			at = static_cast<uint16_t>((at & 0xff00) | static_cast<uint8_t>(at - 1)); // dec l
			from = static_cast<uint8_t>(from - 2);
		}
		mem_[at] = last;
	}

	void transfer_oam()
	{
		cpu_.a = static_cast<uint8_t>(wVirtualOAM >> 8);
		transfer_virtual_oam(gb_);
	}

	// VBlank_ClearScreen.Func: the scroll, and tile $71's rows rotated
	void port_VBlank_ClearScreen__Func()
	{
		mem_[rSCY] = mem_.scy();
		mem_[rSCX] = mem_.scx();
		mem_[rVBK] = 0;
		rotate_tile_plane(v0Tiles2 + 0x71 * TILE_SIZE + (TILE_SIZE - 2));
		rotate_tile_plane(v0Tiles2 + 0x71 * TILE_SIZE + (TILE_SIZE - 1));
		transfer_oam();
	}

	// the queue's next (big-endian address, byte) entry at at
	uint16_t write_queued(uint16_t at)
	{
		uint16_t dest = static_cast<uint16_t>(mem_[at] << 8 | mem_[at + 1]);
		mem_[dest] = mem_[at + 2];
		return static_cast<uint16_t>(at + 3);
	}

	// VBlank_MusicalCoinsScreen.Func: the scroll, and the queued writes
	// (wcee4 bit 7): an attribute (with its low bits set) or two tiles
	void port_VBlank_MusicalCoinsScreen__Func()
	{
		mem_[rSCY] = mem_.scy();
		mem_[rSCX] = mem_.scx();
		uint8_t queued = mem_.unk_cee4();
		if (queued & 0x80) {
			if (queued & 0x7f) { // .asm_d47d9
				mem_[rVBK] = 1; // BANK("VRAM1")
				write_queued(wBGMapTileVRAM0Queue);
			} else {
				write_queued(write_queued(wBGMapTileVRAM0Queue));
			}
			mem_.unk_cee4() &= 0x7f;
		}
		transfer_oam();
	}
};

} // namespace

} // namespace wl3::clear

using wl3::clear::ClearVblank;

PORT_LOCAL_EX(VBlank_ClearScreen, Func, 0, ALL_REGISTERS) { ClearVblank(gb).port_VBlank_ClearScreen__Func(); }
PORT_LOCAL_EX(VBlank_MusicalCoinsScreen, Func, 0, ALL_REGISTERS) { ClearVblank(gb).port_VBlank_MusicalCoinsScreen__Func(); }
