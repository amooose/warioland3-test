// Port of src/engine/overworld/cutscene/prologue/init.asm
#include "ported.h"

// decompress_tilemap / decompress_tiles0: decompresses src (in bank) to dest
static void decompress(GB *gb, uint8_t bank, uint16_t src, uint16_t dest)
{
	gb->cpu.a = bank;
	wr8(gb, wTempBank, bank);
	SET_HL(gb, src);
	SET_BC(gb, dest);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
}

// decompress_attrmap / decompress_tiles1: the same, into VRAM bank 1
static void decompress_vram1(GB *gb, uint8_t bank, uint16_t src, uint16_t dest)
{
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, 1);
	decompress(gb, bank, src, dest);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

// xor a / ld hl, start / ld bc, size / call WriteAToHL_BCTimes
static void clear_bytes(GB *gb, uint16_t start, uint16_t size)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, start);
	SET_BC(gb, size);
	CALL(WriteAToHL_BCTimes);
}

PORT(_InitPrologue)
{
	// clear RAM
	gb_call(gb, ADDR_ClearGeneralSceneRAM);
	gb_call(gb, ADDR_ClearSceneObjsRAM);

	// set V-Blank handler
	gb_call(gb, ADDR_VBlank_PrologueEpilogue);

	// load BG maps for the prologue
	decompress_vram1(gb, BANK_PrologueAttrmap, ADDR_PrologueAttrmap, v1BGMap1);
	decompress(gb, BANK_PrologueTilemap, ADDR_PrologueTilemap, v0BGMap1);

	// load gfx
	SET_HL(gb, ADDR_PrologueBackgroundGfx);
	gb->cpu.b = BANK_PrologueBackgroundGfx;
	CALL(LoadFarTiles);

	decompress_vram1(gb, BANK_FontGfx, ADDR_FontGfx, v1Tiles0);
	decompress_vram1(gb, BANK_PrologueGfx, ADDR_PrologueGfx, v1Tiles0);

	// load BG maps for the Music Box scene
	decompress_vram1(gb, BANK_MusicBoxAttrmap, ADDR_MusicBoxAttrmap, v1BGMap0);
	decompress(gb, BANK_MusicBoxTilemap, ADDR_MusicBoxTilemap, v0BGMap0);

	// load gfx
	SET_HL(gb, ADDR_MusicBoxGfx);
	SET_DE(gb, v0Tiles1);
	gb->cpu.b = BANK_MusicBoxGfx;
	gb->cpu.c = (0x30 * TILE_SIZE) >> 8; // HIGH($30 tiles)
	CALL(CopyFarBytes_Long);

	// load the prologue text to buffer
	gb->cpu.a = alu_and(gb, rd8(gb, wLanguage), rd8(gb, wLanguage));
	if (!(gb->cpu.f & FLAG_Z))
		decompress(gb, BANK_TextEN_HiddenFigureAreYouAware, ADDR_TextEN_HiddenFigureAreYouAware, wTextBuffer);
	else
		decompress(gb, BANK_TextJP_HiddenFigureAreYouAware, ADDR_TextJP_HiddenFigureAreYouAware, wTextBuffer);
	gb_call(gb, ADDR_FillClearedTextBuffer);

	// clear temp pals
	clear_bytes(gb, wTempPals1, 8 * PAL_SIZE);
	clear_bytes(gb, wTempPals2, 8 * PAL_SIZE);

	// init scene Wario object
	gb->cpu.a = 96;
	wr8(gb, wSceneWarioYCoord + 0, 96); // y
	wr8(gb, wSceneWarioYCoord + 1, 96); // x
	SET_HL(gb, wSceneWarioYCoord + 2);
	gb->cpu.a = SCENEWARIO_GROUP_NONE;
	wr8(gb, wSceneWarioStateGroup, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a (SCENEWARIO_NONE)
	gb_call(gb, ADDR_SetSceneWarioState);

	gb->cpu.a = LCDC_BG_MAP;
	wr8(gb, wLCDCFlagsToFlip, gb->cpu.a);
}
