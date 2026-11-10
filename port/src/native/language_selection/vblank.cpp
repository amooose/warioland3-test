// language_selection/vblank (src/engine/language_selection/vblank.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"
#include "../home/memory.hpp"

namespace wl3::language_selection {

namespace {

class LanguageSelectionVblank : public AsmModule {
public:
	using AsmModule::AsmModule;

	// (the rest is C++, src/native/screens)

	// VBlank_LanguageSelection.Func, as copied to wVBlankFunc, plain over
	// memory (the V-Blank handler pushes and pops every register around
	// it): the queued palette change (wcee4 bit 7) puts the chosen
	// language's row in palette 7 and the other in 6, then the scroll and
	// hTransferVirtualOAM (a = wVirtualOAM's page)
	void port_VBlank_LanguageSelection__Func()
	{
		uint8_t queued = mem_.unk_cee4();
		if (queued & 0x80) {
			bool japanese = static_cast<uint8_t>((queued & 0x7f) - 1) != 0;
			mem_[rVBK] = 1; // BANK("VRAM1")
			home::fill(gb_, v0BGMap0 + 7 * TILEMAP_WIDTH + 5, japanese ? 0x07 : 0x06, 13);
			home::fill(gb_, v0BGMap0 + 9 * TILEMAP_WIDTH + 5, japanese ? 0x06 : 0x07, 13);
			mem_.unk_cee4() &= 0x7f;
		}
		mem_[rSCY] = mem_.scy();
		mem_[rSCX] = mem_.scx();
		cpu_.a = static_cast<uint8_t>(wVirtualOAM >> 8);
		transfer_virtual_oam(gb_);
	}
};

} // namespace

} // namespace wl3::language_selection

using wl3::language_selection::LanguageSelectionVblank;

PORT_LOCAL_EX(VBlank_LanguageSelection, Func, 0, ALL_REGISTERS) { LanguageSelectionVblank(gb).port_VBlank_LanguageSelection__Func(); }
