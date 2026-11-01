// overworld/temple/common (src/engine/overworld/temple/common.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldTempleCommon : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_InitTempleScene()
	{
		FARCALL(_InitTempleScene);
	}

	void port_UpdateTempleScene()
	{
		FARCALL(_UpdateTempleScene);
	}

	void port_Func_80e33()
	{
		cpu_.a = rd8(wJoypadDown);
		if (alu_.bit(cpu_.a, B_PAD_B)) {
			cpu_.a = SST_OVERWORLD_08;
			wr8(wSubState, cpu_.a);
			return;
		}
		SET_HL(gb_, wSubState);
		wr8(wSubState, alu_.inc(rd8(wSubState)));
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldTempleCommon;

PORT_EX(InitTempleScene, 0, 0) { OverworldTempleCommon(gb).port_InitTempleScene(); }
PORT_EX(UpdateTempleScene, 0, 0) { OverworldTempleCommon(gb).port_UpdateTempleScene(); }
PORT_EX(Func_80e33, 0, 0) { OverworldTempleCommon(gb).port_Func_80e33(); }
