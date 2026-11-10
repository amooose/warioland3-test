// pause/save/vblank (src/engine/pause/save/vblank.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../../asm_module.hpp"

namespace wl3::pause {

namespace {

class PauseSaveVblank : public AsmModule {
public:
	using AsmModule::AsmModule;

	// The save screens' V-Blank functions, as copied to wVBlankFunc (see
	// ram_code.c), plain over memory: the V-Blank handler pushes and pops
	// every register around them.
	void transfer_oam()
	{
		cpu_.a = static_cast<uint8_t>(wVirtualOAM >> 8);
		transfer_virtual_oam(gb_);
	}

	void port_VBlank_Saving__Func() { transfer_oam(); }

	// VBlank_SaveScreen.Func: the queued tile (wcee4 bit 7; its big-endian
	// address in wBGMapTileVRAM0Queue): tile $02, palette 5
	void port_VBlank_SaveScreen__Func()
	{
		if (mem_[wcee4] & 0x80) {
			uint16_t at = static_cast<uint16_t>(mem_[wBGMapTileVRAM0Queue] << 8 | mem_[wBGMapTileVRAM0Queue + 1]);
			mem_[rVBK] = 1; // BANK("VRAM1")
			mem_[at] = 0x05; // pal
			mem_[rVBK] = 0;
			mem_[at] = 0x02;
			mem_[wcee4] &= 0x7f;
		}
		transfer_oam();
	}
};

} // namespace

} // namespace wl3::pause

using wl3::pause::PauseSaveVblank;

PORT_LOCAL_EX(VBlank_Saving, Func, 0, ALL_REGISTERS) { PauseSaveVblank(gb).port_VBlank_Saving__Func(); }
PORT_LOCAL_EX(VBlank_SaveScreen, Func, 0, ALL_REGISTERS) { PauseSaveVblank(gb).port_VBlank_SaveScreen__Func(); }
