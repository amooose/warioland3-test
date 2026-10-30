// Port of src/engine/level/init.asm
#include "ported.h"

#define IL(off) ((uint16_t)(ADDR_InitLevel + (off)))

PORT(InitLevel)
{
	gb_call_ret(gb, ADDR_DisableLCD, IL(0x03));
	gb_call_ret(gb, ADDR_FillBGMap0_With7f, IL(0x06));
	gb_call_ret(gb, ADDR_ClearVirtualOAM, IL(0x09));
	gb_call_ret(gb, ADDR_ClearParticles, IL(0x0c));

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCY, 0);
	wr8(gb, wSCY, 0);
	wr8(gb, rSCX, 0);
	wr8(gb, wSCX, 0);

	FARCALL_RET(LoadLevelCommonGfxAndTreasures, IL(0x26));

	gb_call_ret(gb, ADDR_StartRoom_FromLevelStart, IL(0x29));
	gb_call_ret(gb, ADDR_UpdateLevelMusic, IL(0x2c));

	gb->cpu.a = TRUE;
	wr8(gb, wRoomAnimatedTilesEnabled, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wceef, 0);

	gb->cpu.a = rd8(gb, wSRAMBank);
	push_af(gb);
	gb->cpu.a = BANK_s0a000; // BANK("SRAM0")
	sramswitch(gb, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	for (int i = 0; i < 4; i++)
		wr8(gb, (uint16_t)(s0a000 + i), 0);
	for (int i = 0; i < 4; i++)
		wr8(gb, (uint16_t)(s0a800 + i), 0);
	pop_af(gb);
	sramswitch(gb, gb->cpu.a);

	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
	gb->cpu.a = rd8(gb, wLevel);
	alu_cp(gb, gb->cpu.a, THE_TEMPLE);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = LCDC_DEFAULT;
		wr8(gb, rLCDC, gb->cpu.a);
		return;
	}

	// .final_battle
	FARCALL_RET(LoadHiddenFigureGfx, IL(0x76));
	gb->cpu.a = 0x88;
	wr8(gb, wc08d, gb->cpu.a);
	wr8(gb, rWY, gb->cpu.a);
	gb->cpu.a = 0 + WX_OFS;
	wr8(gb, rWX, gb->cpu.a);
	gb->cpu.a = LCDC_ON | LCDC_WIN_9C00 | LCDC_WIN_ON | LCDC_OBJ_16 | LCDC_OBJ_ON | LCDC_BG_ON;
	wr8(gb, rLCDC, gb->cpu.a);
}
