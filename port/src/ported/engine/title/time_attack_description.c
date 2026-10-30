// Port of src/engine/title/time_attack_description.asm
#include "ported.h"

PORT(InitTimeAttackDescription)
{
	CALL(DisableLCD);
	CALL(FillBGMap0_With7f);
	CALL(ClearVirtualOAM);
	gb_call(gb, ADDR_LoadTimeAttackDescriptionPals);
	CALL(ApplyTempPals1ToBGPals);
	gb_call(gb, ADDR_LoadTimeAttackDescriptionTiles);
	gb_call(gb, ADDR_LoadTimeAttackText);
	CALL(VBlank_TimeAttackDescription);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSCY, 0);
	wr8(gb, rSCY, 0);
	wr8(gb, wSCX, 0);
	wr8(gb, rSCX, 0);
	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTimer, 0);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}

PORT_JUMP(TimeAttackDescription)
{
	gb_call(gb, ADDR_AdjustJapaneseTimeAttackDescriptionWindow);
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		RET();
		return;
	}
	CALL(DisableLCD);
	CALL(FillWhiteBGPal);
	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	JUMP_TO(ADDR_StartOverworldState); // same bank
}
