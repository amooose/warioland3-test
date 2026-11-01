// pause/pause_menu_2 (src/engine/pause/pause_menu_2.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::pause {

namespace {

class PausePauseMenu2 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// which home code calls (the rest is C++, src/native/pause)

	// push_wram BANK("Level Objects WRAM") / farcall / pop_wram
	void farcall_obj_wram(int bank, uint16_t addr)
	{
		cpu_.a = rd8(rWBK);
		push_af();
		cpu_.a = 0x01;
		wr8(rWBK, cpu_.a);
		port_farcall(gb_, bank, addr);
		pop_af();
		wr8(rWBK, cpu_.a);
	}

	void port_DrawLevelObjectsAfterLevelReturn()
	{
		FARCALL(VBlank_Level);
		FARCALL(SetTileQueueProcessingFunctions);
		farcall_obj_wram(BANK_DrawObjects_NoPriority, ADDR_DrawObjects_NoPriority);
		CALL(DrawWario);
		farcall_obj_wram(BANK_DrawObjects_WithPriority, ADDR_DrawObjects_WithPriority);
	}
};

} // namespace

} // namespace wl3::pause

using wl3::pause::PausePauseMenu2;

PORT_EX(DrawLevelObjectsAfterLevelReturn, 0, 0) { PausePauseMenu2(gb).port_DrawLevelObjectsAfterLevelReturn(); }
