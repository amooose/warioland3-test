// golf/common (src/engine/golf/common.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::golf {

namespace {

class GolfCommon : public AsmModule {
public:
	using AsmModule::AsmModule;

	// Its state routines leave bc, de and hl as they end, and MainStateTable
	// returns to the main loop, where none is live (see src/native/golf).
	void port_GolfStateTable()
	{
		// push_wram BANK("Golf RAM")
		cpu_.a = rd8(rWBK);
		push_af();
		cpu_.a = 0x01;
		wr8(rWBK, cpu_.a);
		FARCALL(_GolfStateTable);
		// pop_wram
		pop_af();
		wr8(rWBK, cpu_.a);
	}
};

} // namespace

} // namespace wl3::golf

using wl3::golf::GolfCommon;

PORT_EX(GolfStateTable, 0, R_B | R_C | R_D | R_E | R_H | R_L) { GolfCommon(gb).port_GolfStateTable(); }
