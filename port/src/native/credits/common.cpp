// credits/common (src/engine/credits/common.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::credits {

namespace {

class CreditsCommon : public AsmModule {
public:
	using AsmModule::AsmModule;

	// MainStateTable calls it and returns to the main loop, which reloads
	// every register (see src/native/screens).
	void port_CreditsStateTable()
	{
		// push_wram BANK("GFX RAM")
		cpu_.a = rd8(rWBK);
		push_af();
		cpu_.a = 0x03;
		wr8(rWBK, cpu_.a);
		FARCALL(_CreditsStateTable);
		// pop_wram
		pop_af();
		wr8(rWBK, cpu_.a);
	}
};

} // namespace

} // namespace wl3::credits

using wl3::credits::CreditsCommon;

PORT_EX(CreditsStateTable, 0, R_B | R_C | R_D | R_E | R_H | R_L) { CreditsCommon(gb).port_CreditsStateTable(); }
