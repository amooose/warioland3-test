// overworld/cutscene/init (src/engine/overworld/cutscene/init.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldCutsceneInit : public AsmModule {
public:
	using AsmModule::AsmModule;

	// for cutscenes that don't need to run an init function
	void port_NoInitCutscene()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		mem_.event_with_cutscene() = 0;
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldCutsceneInit;

PORT_EX(NoInitCutscene, 0, 0) { OverworldCutsceneInit(gb).port_NoInitCutscene(); }
