// pause/pause_menu_2 (src/engine/pause/pause_menu_2.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../level/frame.hpp"
#include "../asm_module.hpp"

namespace wl3::pause {

namespace {

class PausePauseMenu2 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// which home code calls (the rest is C++, src/native/pause)

	// push_wram BANK("Level Objects WRAM") / farcall / pop_wram
	// the farcall to fn with the objects' WRAM bank (1) mapped
	template <class F>
	void farcall_obj_wram(int bank, uint16_t addr, F fn)
	{
		uint8_t wbk = rd8(rWBK);
		wr8(rWBK, 0x01);
		routines_.in_bank(static_cast<uint8_t>(bank), addr, fn);
		wr8(rWBK, wbk);
	}

	void port_DrawLevelObjectsAfterLevelReturn()
	{
		FARCALL(VBlank_Level);
		routines_.in_bank(BANK_SetTileQueueProcessingFunctions, ADDR_SetTileQueueProcessingFunctions, [&] { level::set_tile_queue_processing_functions(gb_); });
		farcall_obj_wram(BANK_DrawObjects_NoPriority, ADDR_DrawObjects_NoPriority, [&] { level::draw_objects_no_priority(gb_); });
		home::draw_wario(gb_);
		farcall_obj_wram(BANK_DrawObjects_WithPriority, ADDR_DrawObjects_WithPriority, [&] { level::draw_objects_with_priority(gb_); });
	}
};

} // namespace

} // namespace wl3::pause

using wl3::pause::PausePauseMenu2;

// ends on DrawObjects, which leaves b to l as it pleases; ExitPauseMenu and
// ReturnToLevelFromGolf read none after it (live.py)
PORT_EX(DrawLevelObjectsAfterLevelReturn, 0, ALL_REGISTERS) { PausePauseMenu2(gb).port_DrawLevelObjectsAfterLevelReturn(); }
