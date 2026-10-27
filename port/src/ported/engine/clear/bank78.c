// Port of src/engine/clear/bank78.asm
#include "ported.h"

static void load_pals(GB *gb, uint16_t pals1, uint16_t pals2)
{
	SET_HL(gb, pals1);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, pals2);
	CALL(LoadPalsToTempPals2);
}

PORT(LoadClearScreenPals) { load_pals(gb, ADDR_Pals_1e0378, ADDR_Pals_1e03b8); }
PORT(Func_1e000d) { load_pals(gb, ADDR_Pals_1e03f8, ADDR_Pals_1e0438); }

PORT(LoadClearScreenGfx)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_ClearScreen2Gfx);
	SET_BC(gb, v0Tiles0);
	CALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_ClearScreen1Gfx);
	SET_BC(gb, v0Tiles1);
	CALL(Decompress);
	SET_HL(gb, ADDR_WarioClearGfx);
	SET_DE(gb, v0Tiles0);
	SET_BC(gb, 0x80 * TILE_SIZE);
	gb->cpu.a = BANK_WarioClearGfx;
	wr8(gb, wTempBank, gb->cpu.a);
	CALL(FarCopyHLToDE_BC);
}

PORT(Func_1e0045)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_ClearScreenTreasureAttrmap);
	SET_BC(gb, v1BGMap0);
	CALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_ClearScreenTreasureTilemap);
	SET_BC(gb, v0BGMap0);
	CALL(Decompress);
}
