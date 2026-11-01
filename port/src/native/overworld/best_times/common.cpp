// overworld/best_times/common (src/engine/overworld/best_times/common.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldBestTimesCommon : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_Func_80de0()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		cpu_.a = rd8(wGameModeFlags);
		if (!alu_.bit(cpu_.a, MODE_TIME_ATTACK_F))
			return;
		cpu_.a = rd8(wJoypadPressed);
		if (!alu_.bit(cpu_.a, B_PAD_START))
			return;
		cpu_.a = SST_OVERWORLD_BEST_TIMES;
		wr8(wSubState, cpu_.a);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldBestTimesCommon;

PORT_EX(Func_80de0, 0, 0) { OverworldBestTimesCommon(gb).port_Func_80de0(); }
