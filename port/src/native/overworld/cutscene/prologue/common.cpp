// overworld/cutscene/prologue/common (src/engine/overworld/cutscene/prologue/common.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../../../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldCutscenePrologueCommon : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_InitPrologue()
	{
		FARCALL(_InitPrologue);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldCutscenePrologueCommon;

PORT_EX(InitPrologue, 0, 0) { OverworldCutscenePrologueCommon(gb).port_InitPrologue(); }
