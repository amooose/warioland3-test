// home/clear_save (src/home/clear_save.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeClearSave : public AsmModule {
public:
	using AsmModule::AsmModule;

	// clears some SRAM stuff
	void port_ClearSaveData()
	{
		static const uint16_t areas[] = {sCheckVals, sBackupCheckVals, s0a000, s0a800, s0a400, s0ac00};
		cpu_.a = 0;
		cpu_.f = FLAG_Z;
		for (int i = 0; i < 6; i++) {
			SET_HL(gb_, areas[i]);
			cpu_.b = 8;
			CALL(WriteAToHL_BTimes);
		}
		cpu_.a = 0;
		wr8(hUnused_fffd, 0);
		JUMP(Init);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeClearSave;

PORT_EX(ClearSaveData, HOOK_MANUAL_RET, 0) { HomeClearSave(gb).port_ClearSaveData(); }
