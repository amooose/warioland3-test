// Port of src/engine/overworld/cutscene/init.asm
#include "ported.h"

// for cutscenes that don't need to run an init function
PORT(NoInitCutscene)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wEventWithCutscene, 0);
}

// returns nz if event in wCurEvent has a cutscene
// if TRUE, outputs event in wEventWithCutscene
// otherwise outputs 0
PORT(CheckIfEventHasCutscene)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wEventWithCutscene, 0);
	gb->cpu.a = rd8(gb, wCurEvent);
	gb->cpu.b = gb->cpu.a;
	SET_HL(gb, ADDR_CutsceneInitFunctions);
	CALL(GetPointerFromTableHL);
	gb->cpu.a = gb->cpu.h;
	alu_cp(gb, gb->cpu.a, (uint8_t)(ADDR_NoInitCutscene >> 8));
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = gb->cpu.l;
		alu_cp(gb, gb->cpu.a, (uint8_t)ADDR_NoInitCutscene);
		if (gb->cpu.f & FLAG_Z)
			return; // no cutscene
	}
	gb->cpu.a = gb->cpu.b;
	wr8(gb, wEventWithCutscene, gb->cpu.a);
}

// Only farcalled, by InitCutscene; InitEventCutscene (C++) leaves c, d and
// e as it likes.
PORT_EX(_InitCutscene, 0, R_B | R_C | R_D | R_E | R_H | R_L)
{
	CALL(DisableLCD);
	CALL(ClearVirtualOAM);
	// stop_music2
	gb->cpu.a = 0xff;
	wr8(gb, hMusicIDBackup + 0, 0xff);
	wr8(gb, hMusicID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hMusicIDBackup + 1, 0x00);
	wr8(gb, hMusicID + 1, 0x00);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, wLCDCFlagsToFlip);
	SET_BC(gb, 0x30);
	CALL(WriteAToHL_BCTimes);

	gb_call(gb, ADDR_VBlank_Cutscene);
	gb_call(gb, ADDR_InitEventCutscene);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCY, 0);
	wr8(gb, wSCY, 0);
	wr8(gb, rSCX, 0);
	wr8(gb, wSCX, 0);
	wr8(gb, w2d801, 0);
	wr8(gb, w2d013, 0);
	wr8(gb, w2d014, 0);
	wr8(gb, wPalConfig1TotalSteps, 0);
	wr8(gb, wPalConfig2TotalSteps, 0);

	gb->cpu.a = rd8(gb, wLCDCFlagsToFlip);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = LCDC_BG_ON | LCDC_OBJ_ON | LCDC_OBJ_16 | LCDC_WIN_9C00 | LCDC_ON;
	if (!(gb->cpu.f & FLAG_Z))
		gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.b);
	wr8(gb, rLCDC, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wLCDCFlagsToFlip, 0);

	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}
