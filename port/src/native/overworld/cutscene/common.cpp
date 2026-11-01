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

	void port_UpdateCutscene()
	{
		// .SkipCutsceneIfAble
		cpu_.a = alu_.and_(rd8(wOWBarsState), rd8(wOWBarsState));
		if (!(cpu_.f & FLAG_Z)) {
			// was a cutscene triggered from the Top Bar
			cpu_.a = rd8(w2d013);
			alu_.cp(cpu_.a, 0x01);
			if (!(cpu_.f & FLAG_C)) {
				cpu_.a = rd8(wJoypadPressed);
				if (alu_.bit(cpu_.a, B_PAD_B)) {
					// b btn pressed: pop hl skips the rest of UpdateCutscene
					SET_HL(gb_, wSubState);
					wr8(wSubState, alu_.inc(rd8(wSubState)));
					// stop_sfx
					wr8(hSFXID + 0, 0xff);
					cpu_.a = 0x00;
					wr8(hSFXID + 1, cpu_.a);
					return;
				}
			}
		}
		FARCALL(DoCutsceneFunc);
		CALL(ClearUnusedVirtualOAM);
	}

	void port_Func_80db1()
	{
		JUMP_TO(ADDR_InitOverworld + 0x87); // InitOverworld.after_cutscene
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldCutsceneCommon;

PORT_EX(InitCutscene, 0, R_B | R_C | R_D | R_E | R_H | R_L) { OverworldCutsceneCommon(gb).port_InitCutscene(); }
PORT_EX(UpdateCutscene, 0, R_B | R_C | R_D | R_E | R_H | R_L) { OverworldCutsceneCommon(gb).port_UpdateCutscene(); }
PORT_EX(Func_80db1, HOOK_MANUAL_RET, 0) { OverworldCutsceneCommon(gb).port_Func_80db1(); }
