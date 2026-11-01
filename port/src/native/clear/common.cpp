// clear/common (src/engine/clear/common.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::clear {

namespace {

class ClearCommon : public AsmModule {
public:
	using AsmModule::AsmModule;

	// MainStateTable calls it and returns to the main loop, which reloads
	// every register (see src/native/clear).
	void port_ClearScreenStateTable()
	{
		// push_wram BANK("GFX RAM")
		cpu_.a = rd8(rWBK);
		push_af();
		cpu_.a = 0x03;
		wr8(rWBK, cpu_.a);
		FARCALL(_ClearScreenStateTable);
		// pop_wram
		pop_af();
		wr8(rWBK, cpu_.a);
	}
};

} // namespace

} // namespace wl3::clear

using wl3::clear::ClearCommon;

PORT_EX(ClearScreenStateTable, 0, R_B | R_C | R_D | R_E | R_H | R_L) { ClearCommon(gb).port_ClearScreenStateTable(); }
