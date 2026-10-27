// Port of src/engine/pause/bank7d.asm
#include "ported.h"

PORT(LoadSaveScreenPals)
{
	SET_HL(gb, ADDR_Pals_1f628c);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_1f628c);
	CALL(LoadPalsToTempPals2);
}

PORT(LoadPauseMenuPals)
{
	SET_HL(gb, ADDR_Pals_1f4182);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_1f41c2);
	CALL(LoadPalsToTempPals2);
}

PORT(LoadPauseMenuGfx)
{
	SET_HL(gb, ADDR_PauseMenuGfx);
	SET_BC(gb, v0Tiles0);
	CALL(Decompress);

	gb->cpu.a = rd8(gb, wTransformation);
	uint16_t gfx;
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_OWL_WARIO);
	if (gb->cpu.f & FLAG_Z) {
		gfx = ADDR_ActionHelpOwlGfx;
	} else {
		alu_cp(gb, gb->cpu.a, TRANSFORMATION_RAIL);
		if (gb->cpu.f & FLAG_Z) {
			gfx = ADDR_ActionHelpRailGfx;
		} else {
			alu_cp(gb, gb->cpu.a, TRANSFORMATION_VAMPIRE_WARIO);
			if (!(gb->cpu.f & FLAG_Z))
				return;
			gfx = ADDR_ActionHelpVampireGfx;
		}
	}
	SET_HL(gb, gfx);
	SET_DE(gb, v0Tiles0);
	SET_BC(gb, 0x20 * TILE_SIZE);
	CALL(CopyHLToDE_BC);
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

// Draws the 2 BCD digits of a as 2-tile-high numbers at hl, moving right.
// bc = -TILEMAP_WIDTH, de = TILEMAP_WIDTH
static uint16_t draw_digit(GB *gb, uint16_t hl, uint8_t digit)
{
	gb->cpu.a = alu_add(gb, digit, digit);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0xa0);
	wr8(gb, hl, gb->cpu.a);
	hl = alu_add16(gb, hl, REG_DE(gb));
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, hl++, gb->cpu.a);
	return alu_add16(gb, hl, REG_BC(gb));
}

// draws mm:ss (BCD bytes at time) at hl
static uint16_t draw_time(GB *gb, uint16_t hl, uint16_t time)
{
	SET_BC(gb, (uint16_t)-TILEMAP_WIDTH);
	SET_DE(gb, TILEMAP_WIDTH);
	gb->cpu.a = alu_swap(gb, rd8(gb, time + 0));
	hl = draw_digit(gb, hl, alu_and(gb, gb->cpu.a, 0x0f));
	hl = draw_digit(gb, hl, alu_and(gb, rd8(gb, time + 0), 0x0f));
	hl = (uint16_t)((hl & 0xff00) | alu_inc(gb, (uint8_t)hl)); // inc l
	gb->cpu.a = alu_swap(gb, rd8(gb, time + 1));
	hl = draw_digit(gb, hl, alu_and(gb, gb->cpu.a, 0x0f));
	hl = draw_digit(gb, hl, alu_and(gb, rd8(gb, time + 1), 0x0f));
	return hl;
}

PORT(Func_1f403f)
{
	gb->cpu.a = rd8(gb, wGameModeFlags);
	if (!alu_bit(gb, gb->cpu.a, MODE_TIME_ATTACK_F)) {
		draw_maps(gb, ADDR_PauseMenuAttrmap, ADDR_PauseMenuTilemap, v1BGMap1);
		return;
	}
	draw_maps(gb, ADDR_TimeAttackPauseMenuAttrmap, ADDR_TimeAttackPauseMenuTilemap, v0BGMap1);

	gb->cpu.a = alu_srl(gb, rd8(gb, wLevel));
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, wLevelTimeAttackScores, REG_DE(gb));
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, w3d511 + 0, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, w3d511 + 1, gb->cpu.a);
	hl = draw_time(gb, v1BGMap1 + 2 * TILEMAP_WIDTH + 4, w3d511);
	hl = draw_time(gb, v1BGMap1 + 2 * TILEMAP_WIDTH + 11, wLevelTime);
	SET_HL(gb, hl);
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
