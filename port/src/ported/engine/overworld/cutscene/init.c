// Port of src/engine/overworld/cutscene/init.asm
#include "ported.h"

// rst JumpTable, with the table at `table` (right after the rst): leaves every
// register as JumpTable does and returns the target (= hl).
static uint16_t jumptable(GB *gb, uint16_t table)
{
	gb_push16(gb, table); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	return REG_HL(gb);
}

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

PORT(_InitCutscene)
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

// runs the init function from CutsceneInitFunctions
// for the cutscene in [wCurEvent]
PORT_JUMP(InitEventCutscene)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wLCDCFlagsToFlip, 0);
	CALL(ClearVirtualOAM);
	gb->cpu.a = alu_dec(gb, rd8(gb, wCurEvent));
	if (!(gb->cpu.f & FLAG_Z)) {
		// != EVENT_PROLOGUE: decompress_tiles1 Cutscenes6Gfx, v1Tiles0
		gb->cpu.a = 1; // BANK("VRAM1")
		wr8(gb, rVBK, 1);
		wr8(gb, wTempBank, BANK_Cutscenes6Gfx);
		SET_HL(gb, ADDR_Cutscenes6Gfx);
		SET_BC(gb, v1Tiles0);
		gb->cpu.a = rd8(gb, wTempBank);
		wr8(gb, hCallFuncBank, gb->cpu.a);
		HCALL(Decompress);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, rVBK, 0);
	}
	gb->cpu.a = rd8(gb, wCurEvent);
	wr8(gb, wEventWithCutscene, gb->cpu.a);
	JUMP_TO(jumptable(gb, ADDR_CutsceneInitFunctions));
}
