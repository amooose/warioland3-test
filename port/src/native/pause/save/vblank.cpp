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
	// every register around them. Their calls return to cpu_.pc + n, the
	// block's start wherever it runs from.
	void transfer_oam(uint16_t ret)
	{
		cpu_.a = static_cast<uint8_t>(wVirtualOAM >> 8);
		gb_call_ret(gb_, hTransferVirtualOAM, ret);
		RET();
	}

	void port_VBlank_Saving__Func() { transfer_oam(static_cast<uint16_t>(cpu_.pc + 0x05)); }

	// VBlank_SaveScreen.Func: the queued tile (wcee4 bit 7; its big-endian
	// address in wBGMapTileVRAM0Queue): tile $02, palette 5
	void port_VBlank_SaveScreen__Func()
	{
		uint16_t base = cpu_.pc;
		if (mem_[wcee4] & 0x80) {
			uint16_t at = static_cast<uint16_t>(mem_[wBGMapTileVRAM0Queue] << 8 | mem_[wBGMapTileVRAM0Queue + 1]);
			mem_[rVBK] = 1; // BANK("VRAM1")
			mem_[at] = 0x05; // pal
			mem_[rVBK] = 0;
			mem_[at] = 0x02;
			mem_[wcee4] &= 0x7f;
		}
		transfer_oam(static_cast<uint16_t>(base + 0x22));
	}
};

} // namespace

} // namespace wl3::pause

using wl3::pause::PauseSaveVblank;

PORT_LOCAL_EX(VBlank_Saving, Func, HOOK_MANUAL_RET, ALL_REGISTERS) { PauseSaveVblank(gb).port_VBlank_Saving__Func(); }
PORT_LOCAL_EX(VBlank_SaveScreen, Func, HOOK_MANUAL_RET, ALL_REGISTERS) { PauseSaveVblank(gb).port_VBlank_SaveScreen__Func(); }
