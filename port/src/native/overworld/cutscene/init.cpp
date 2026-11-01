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
		wr8(wEventWithCutscene, 0);
	}

	// returns nz if event in wCurEvent has a cutscene
	// if TRUE, outputs event in wEventWithCutscene
	// otherwise outputs 0
	void port_CheckIfEventHasCutscene()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wEventWithCutscene, 0);
		cpu_.a = rd8(wCurEvent);
		cpu_.b = cpu_.a;
		SET_HL(gb_, ADDR_CutsceneInitFunctions);
		CALL(GetPointerFromTableHL);
		cpu_.a = cpu_.h;
		alu_.cp(cpu_.a, (uint8_t)(ADDR_NoInitCutscene >> 8));
		if (cpu_.f & FLAG_Z) {
			cpu_.a = cpu_.l;
			alu_.cp(cpu_.a, (uint8_t)ADDR_NoInitCutscene);
			if (cpu_.f & FLAG_Z)
				return; // no cutscene
		}
		cpu_.a = cpu_.b;
		wr8(wEventWithCutscene, cpu_.a);
	}

	// Only farcalled, by InitCutscene; InitEventCutscene (C++) leaves c, d and
	// e as it likes.
	void port__InitCutscene()
	{
		CALL(DisableLCD);
		CALL(ClearVirtualOAM);
		// stop_music2
		cpu_.a = 0xff;
		wr8(hMusicIDBackup + 0, 0xff);
		wr8(hMusicID + 0, 0xff);
		cpu_.a = 0x00;
		wr8(hMusicIDBackup + 1, 0x00);
		wr8(hMusicID + 1, 0x00);

		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		SET_HL(gb_, wLCDCFlagsToFlip);
		SET_BC(gb_, 0x30);
		CALL(WriteAToHL_BCTimes);

		gb_call(gb_, ADDR_VBlank_Cutscene);
		gb_call(gb_, ADDR_InitEventCutscene);

		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rSCY, 0);
		wr8(wSCY, 0);
		wr8(rSCX, 0);
		wr8(wSCX, 0);
		wr8(w2d801, 0);
		wr8(w2d013, 0);
		wr8(w2d014, 0);
		wr8(wPalConfig1TotalSteps, 0);
		wr8(wPalConfig2TotalSteps, 0);

		cpu_.a = rd8(wLCDCFlagsToFlip);
		cpu_.b = cpu_.a;
		cpu_.a = alu_.and_(cpu_.a, cpu_.a);
		cpu_.a = LCDC_BG_ON | LCDC_OBJ_ON | LCDC_OBJ_16 | LCDC_WIN_9C00 | LCDC_ON;
		if (!(cpu_.f & FLAG_Z))
			cpu_.a = alu_.xor_(cpu_.a, cpu_.b);
		wr8(rLCDC, cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wLCDCFlagsToFlip, 0);

		SET_HL(gb_, wSubState);
		wr8(wSubState, alu_.inc(rd8(wSubState)));
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldCutsceneInit;

PORT_EX(NoInitCutscene, 0, 0) { OverworldCutsceneInit(gb).port_NoInitCutscene(); }
PORT_EX(CheckIfEventHasCutscene, 0, 0) { OverworldCutsceneInit(gb).port_CheckIfEventHasCutscene(); }
PORT_EX(_InitCutscene, 0, R_B | R_C | R_D | R_E | R_H | R_L) { OverworldCutsceneInit(gb).port__InitCutscene(); }
