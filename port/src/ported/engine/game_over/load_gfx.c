// Port of src/engine/game_over/load_gfx.asm
#include "ported.h"

PORT(LoadHiddenFigureGfx)
{
	// .LoadPals
	SET_HL(gb, ADDR_Pals_db000);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_db040);
	SET_DE(gb, wTempPals2 + 3 * 8);
	gb->cpu.b = 5 * 8;
	CALL(CopyHLToDE_Short);

	// .LoadTiles
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_HiddenFigure2Gfx);
	SET_DE(gb, v1Tiles0);
	SET_BC(gb, 0x180 * 16);
	CALL(CopyHLToDE_BC);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_HiddenFigure1Gfx);
	SET_DE(gb, v0Tiles0);
	SET_BC(gb, 0x180 * 16);
	CALL(CopyHLToDE_BC);

	// .LoadMap
	gb->cpu.a = BANK_v1Tiles0;
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_HiddenFigure2Map);
	SET_BC(gb, v1BGMap0);
	CALL(Decompress);
	SET_HL(gb, ADDR_HiddenFigure4Map);
	SET_BC(gb, v1BGMap1);
	CALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_HiddenFigure1Map);
	SET_BC(gb, v0BGMap0);
	CALL(Decompress);
	SET_HL(gb, ADDR_HiddenFigure3Map);
	SET_BC(gb, v0BGMap1);
	CALL(Decompress);
}

PORT(LoadGameOverPals)
{
	SET_HL(gb, ADDR_Pals_db509);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_db549);
	CALL(LoadPalsToTempPals2);
	SET_HL(gb, ADDR_Pals_86ad5);
	SET_DE(gb, wTempPals2);
	gb->cpu.b = 1 * 8;
	FARCALL_BANK(BANK_Pals_86ad5, CopyHLToDE_Short);
}

PORT(LoadGameOverGfx)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_GameOverGfx);
	SET_BC(gb, v0Tiles0);
	CALL(Decompress);
	SET_HL(gb, ADDR_WarioSleepGfx);
	SET_DE(gb, v0Tiles0);
	SET_BC(gb, 0x80 * 16);
	FARCALL_BANK(BANK_WarioSleepGfx, CopyHLToDE_BC);
}

PORT(Func_db4cf)
{
	gb->cpu.a = BANK_v1Tiles0;
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_GameOverAttrmap);
	SET_BC(gb, v0BGMap0);
	CALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_GameOverTilemap);
	SET_BC(gb, v0BGMap0);
	CALL(Decompress);
}

PORT(Func_db4e9)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.b = rd8(gb, wSCY);
	gb->cpu.a = alu_sub(gb, alu_add(gb, rd8(gb, hl++), OAM_Y_OFS), gb->cpu.b);
	wr8(gb, wCurSpriteYCoord, gb->cpu.a);
	wr8(gb, wCurSpriteXCoord, gb->cpu.a = alu_add(gb, rd8(gb, hl++), OAM_X_OFS));
	wr8(gb, wCurSpriteFrame, gb->cpu.a = rd8(gb, hl++));
	wr8(gb, wCurSpriteAttributes, gb->cpu.a = rd8(gb, hl));
	SET_HL(gb, ADDR_OAM_dbbd2);
	CALL(LoadSprite);
}
