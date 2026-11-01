// overworld/cutscene/common (src/engine/overworld/cutscene/common.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldCutsceneCommon : public AsmModule {
public:
	using AsmModule::AsmModule;

	// InitCutscene and UpdateCutscene are overworld states (HandleOverworld,
	// which clobbers every register); the C++ cutscenes (src/native/cutscenes)
	// leave b to l as they like.
	void port_InitCutscene()
	{
		FARCALL(_InitCutscene);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldCutsceneCommon;

PORT_EX(InitCutscene, 0, R_B | R_C | R_D | R_E | R_H | R_L) { OverworldCutsceneCommon(gb).port_InitCutscene(); }
