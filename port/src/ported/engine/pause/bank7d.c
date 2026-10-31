// Port of src/engine/pause/bank7d.asm: the save screen's parts (the pause
// menu's are C++, src/native/pause)
#include "ported.h"

PORT(LoadSaveScreenPals)
{
	SET_HL(gb, ADDR_Pals_1f628c);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_1f628c);
	CALL(LoadPalsToTempPals2);
}

// rVBK = 1, decompress attrmap to bc; rVBK = 0, decompress tilemap to bc
static void draw_maps(GB *gb, uint16_t attrmap, uint16_t tilemap, uint16_t dst)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, attrmap);
	SET_BC(gb, dst);
	CALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, tilemap);
	SET_BC(gb, dst);
	CALL(Decompress);
}

PORT(LoadSaveScreenGfx)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_SaveBoxGfx);
	SET_BC(gb, v0Tiles0);
	gb->cpu.a = BANK_SaveBoxGfx;
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);

	SET_HL(gb, ADDR_WarioSleepGfx);
	SET_DE(gb, v0Tiles0);
	SET_BC(gb, 0x800);
	gb->cpu.a = BANK_WarioSleepGfx;
	wr8(gb, wTempBank, gb->cpu.a);
	CALL(FarCopyHLToDE_BC);
}

PORT(DrawSaveInProgressBox)
{
	draw_maps(gb, ADDR_SaveInProgressBoxAttrmap, ADDR_SaveInProgressBoxTilemap, v0BGMap0);
}

PORT(DrawSaveCompleteBox)
{
	draw_maps(gb, ADDR_SaveCompleteBoxAttrmap, ADDR_SaveCompleteBoxTilemap, v0BGMap0);
}
