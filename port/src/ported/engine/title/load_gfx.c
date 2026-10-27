// Port of src/engine/title/load_gfx.asm
#include "ported.h"

static void load_pals(GB *gb, uint16_t pals1, uint16_t pals2)
{
	SET_HL(gb, pals1);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, pals2);
	CALL(LoadPalsToTempPals2);
}

PORT(LoadTitleScreenPals)
{
	gb->cpu.a = rd8(gb, wceef);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a)
		load_pals(gb, ADDR_Pals_4f82, ADDR_Pals_4fc2);
	else
		load_pals(gb, ADDR_Pals_5002, ADDR_Pals_5042);
}

// rVBK = 1, decompress src1 to dst1; rVBK = 0, decompress src0 to dst0
static void decompress_vram(GB *gb, uint16_t src1, uint16_t dst1, uint16_t src0, uint16_t dst0)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, src1);
	SET_BC(gb, dst1);
	CALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, src0);
	SET_BC(gb, dst0);
	CALL(Decompress);
}

PORT(LoadTitleScreenTiles)
{
	decompress_vram(gb, ADDR_PlaneGfx, v1Tiles0, ADDR_TitleScreenGfx, v0Tiles0);
}

PORT(LoadTitleScreenTileMap)
{
	decompress_vram(gb, ADDR_TitleScreenAttrmap, v1BGMap0, ADDR_TitleScreenTilemap, v0BGMap0);
}

PORT(LoadTimeAttackDescriptionPals)
{
	SET_HL(gb, ADDR_Pals_71b0);
	CALL(LoadPalsToTempPals1);
}

PORT(LoadTimeAttackDescriptionTiles)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	FARCALL(LoadFontTiles);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_PrologueBackgroundGfx);
	SET_DE(gb, v0Tiles2);
	SET_BC(gb, 0x80 * TILE_SIZE);
	gb->cpu.a = BANK_PrologueBackgroundGfx;
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(CopyHLToDE_BC);
}

PORT(LoadTimeAttackText)
{
	gb->cpu.a = rd8(gb, wLanguage);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a)
		decompress_vram(gb, ADDR_TimeAttack2JPTextMap, v1BGMap0, ADDR_TimeAttack1JPTextMap, v0BGMap0);
	else
		decompress_vram(gb, ADDR_TimeAttack2ENTextMap, v1BGMap0, ADDR_TimeAttack1ENTextMap, v0BGMap0);
}
