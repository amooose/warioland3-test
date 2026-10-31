// Port of src/engine/overworld/cutscene/common.asm
#include "ported.h"

// InitCutscene and UpdateCutscene are overworld states (HandleOverworld,
// which clobbers every register); the C++ cutscenes (src/native/cutscenes)
// leave b to l as they like.
PORT_EX(InitCutscene, 0, R_B | R_C | R_D | R_E | R_H | R_L)
{
	FARCALL(_InitCutscene);
}

PORT_EX(UpdateCutscene, 0, R_B | R_C | R_D | R_E | R_H | R_L)
{
	// .SkipCutsceneIfAble
	gb->cpu.a = alu_and(gb, rd8(gb, wOWBarsState), rd8(gb, wOWBarsState));
	if (!(gb->cpu.f & FLAG_Z)) {
		// was a cutscene triggered from the Top Bar
		gb->cpu.a = rd8(gb, w2d013);
		alu_cp(gb, gb->cpu.a, 0x01);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.a = rd8(gb, wJoypadPressed);
			if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
				// b btn pressed: pop hl skips the rest of UpdateCutscene
				SET_HL(gb, wSubState);
				wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
				// stop_sfx
				wr8(gb, hSFXID + 0, 0xff);
				gb->cpu.a = 0x00;
				wr8(gb, hSFXID + 1, gb->cpu.a);
				return;
			}
		}
	}
	FARCALL(DoCutsceneFunc);
	CALL(ClearUnusedVirtualOAM);
}

PORT_JUMP(Func_80db1)
{
	JUMP_TO(ADDR_InitOverworld + 0x87); // InitOverworld.after_cutscene
}
