// Port of src/engine/action_help/common2.asm
#include "ported.h"

PORT(LoadActionHelpCommonPals)
{
	gb->cpu.a = TRUE;
	wr8(gb, wActionHelpBlackedOut, TRUE);
	SET_HL(gb, ADDR_Pals_1e4eef);
	CALL(LoadPalsToTempPals1);
}

// Loads the OB palettes for the power-up being demonstrated.
PORT(LoadActionHelpPowerUpPals)
{
	uint8_t power_up = alu_and(gb, rd8(gb, wActionHelpPowerUp), POWER_UP_MASK);
	alu_cp(gb, power_up, NUM_MAIN_POWER_UPS);
	bool main_power_up = gb->cpu.f & FLAG_C;

	SET_HL(gb, main_power_up ? ADDR_Pals_1e4f2f : ADDR_Pals_1e4f7f);
	CALL(LoadPalsToTempPals2);
	gb->cpu.a = rd8(gb, wActionHelpPowerUp);
	alu_cp(gb, gb->cpu.a, main_power_up ? GARLIC : POWER_UP_VAMPIRE);
	if (main_power_up ? (gb->cpu.f & FLAG_C) : !(gb->cpu.f & FLAG_Z))
		return;

	// garlic and later power-ups, or the vampire transformation, recolour
	// the first two palettes
	SET_HL(gb, main_power_up ? ADDR_Pals_1e4f6f : ADDR_Pals_1e4fbf);
	SET_DE(gb, wTempPals2);
	gb->cpu.b = 2 * PAL_SIZE;
	CALL(CopyHLToDE_Short);
}

PORT(LoadActionHelpGfx)
{
	wr8(gb, rVBK, BANK_v1Tiles0);
	uint8_t power_up = alu_and(gb, rd8(gb, wActionHelpPowerUp), POWER_UP_MASK);
	alu_cp(gb, power_up, NUM_MAIN_POWER_UPS);
	gb->cpu.a = power_up;
	SET_HL(gb, (gb->cpu.f & FLAG_C) ? ADDR_ActionHelpObjects1Gfx : ADDR_ActionHelpObjects2Gfx);
	SET_BC(gb, v1Tiles0);
	CALL(Decompress);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_ActionHelpHUDGfx);
	SET_BC(gb, v0Tiles0);
	CALL(Decompress);
}

// hl = y, x, frame and attributes; y is relative to the background scroll
static uint16_t set_cur_sprite(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.b = rd8(gb, wSCY);
	wr8(gb, wCurSpriteYCoord, alu_add(gb, alu_sub(gb, rd8(gb, hl++), gb->cpu.b), OAM_Y_OFS));
	wr8(gb, wCurSpriteXCoord, alu_add(gb, rd8(gb, hl++), OAM_X_OFS));
	wr8(gb, wCurSpriteFrame, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wCurSpriteAttributes, gb->cpu.a);
	return hl;
}

PORT(LoadActionHelpWarioSprite)
{
	set_cur_sprite(gb);
	gb->cpu.h = rd8(gb, wActionHelpWarioOAMPtr + 0);
	gb->cpu.l = rd8(gb, wActionHelpWarioOAMPtr + 1);
	FARCALL_BANK(0x05, LoadSprite);
}

// hl = menu object, whose OAM pointer and bank follow its frameset
PORT(LoadActionHelpSprite_Far)
{
	uint16_t hl = set_cur_sprite(gb) + 1;
	uint8_t l = (uint8_t)hl;
	for (int i = 0; i < 4; i++)
		l = alu_inc(gb, l); // inc l
	hl = (uint16_t)((hl & 0xff00) | l);
	gb->cpu.d = rd8(gb, hl++);
	gb->cpu.e = rd8(gb, hl++);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wTempBank, gb->cpu.a);
	SET_HL(gb, REG_DE(gb));
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(LoadSprite);
}

PORT(LoadActionHelpSprite)
{
	set_cur_sprite(gb);
	SET_HL(gb, ADDR_OAM_1e697c);
	CALL(LoadSprite);
}

// Copies b palettes from hl to the palette data register c, one palette
// per HBlank (wait_not_hblank / wait_hblank).
static void copy_pals_hblank(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	do {
		while (!(gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
			idle(gb, 32);
		while ((gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
			idle(gb, 32);
		for (int i = 0; i < PAL_SIZE; i++) {
			gb->cpu.a = rd8(gb, hl++);
			wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), gb->cpu.a);
		}
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	SET_HL(gb, hl);
}

// push hl / ld de, dst / ld b, n / call CopyHLToDE_Short / pop hl
static void copy_pals_keep_hl(GB *gb, uint16_t dst, uint8_t n)
{
	uint16_t hl = REG_HL(gb);
	gb_push16(gb, hl);
	SET_DE(gb, dst);
	gb->cpu.b = n;
	CALL(CopyHLToDE_Short);
	SET_HL(gb, gb_pop16(gb));
}

static void write_a_to_hl(GB *gb, uint16_t hl, uint8_t n)
{
	SET_HL(gb, hl);
	gb->cpu.b = n;
	CALL(WriteAToHL_BTimes);
}

// Returns whether the power-up being demonstrated is a main one; if so,
// leaves the carry of comparing it with Wario's power-up level.
static bool cp_power_up_level(GB *gb)
{
	gb->cpu.a = rd8(gb, wActionHelpPowerUp);
	alu_cp(gb, gb->cpu.a, NUM_MAIN_POWER_UPS);
	if (!(gb->cpu.f & FLAG_C))
		return false;
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), POWER_UP_MASK);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	return true;
}

PORT(LoadActionHelpOBPals)
{
	SET_HL(gb, ADDR_Pals_1e4f2f);
	if (!cp_power_up_level(gb))
		return; // special Power-Up
	if (gb->cpu.f & FLAG_C) {
		// doesn't have Power-Up, so black out some OB pals
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		write_a_to_hl(gb, wTempPals2, 2 * PAL_SIZE);
		write_a_to_hl(gb, wTempPals2 + 3 * PAL_SIZE, 5 * PAL_SIZE);
		SET_HL(gb, wTempPals2);
	} else {
		copy_pals_keep_hl(gb, wTempPals2, 8 * PAL_SIZE);
	}
	gb->cpu.a = OBPI_AUTOINC | 0;
	wr8(gb, rOBPI, gb->cpu.a);
	gb->cpu.b = 8;
	gb->cpu.c = (uint8_t)rOBPD;
	copy_pals_hblank(gb);
}

PORT(Func_1e4a47)
{
	SET_HL(gb, ADDR_Pals_1e4f2f + 4 * PAL_SIZE);
	if (!cp_power_up_level(gb))
		return;
	if (gb->cpu.f & FLAG_C) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		write_a_to_hl(gb, wTempPals2 + 4 * PAL_SIZE, PAL_SIZE);
		write_a_to_hl(gb, wTempPals2 + 7 * PAL_SIZE, PAL_SIZE);
		SET_HL(gb, wTempPals2 + 4 * PAL_SIZE);
	} else {
		uint16_t hl = REG_HL(gb);
		gb_push16(gb, hl);
		SET_DE(gb, wTempPals2 + 4 * PAL_SIZE);
		gb->cpu.b = PAL_SIZE;
		CALL(CopyHLToDE_Short);
		SET_HL(gb, ADDR_Pals_1e4f2f + 7 * PAL_SIZE);
		SET_DE(gb, wTempPals2 + 7 * PAL_SIZE);
		gb->cpu.b = PAL_SIZE;
		CALL(CopyHLToDE_Short);
		SET_HL(gb, gb_pop16(gb));
	}
	gb->cpu.a = OBPI_AUTOINC | (4 * PAL_SIZE);
	wr8(gb, rOBPI, gb->cpu.a);
	gb->cpu.c = (uint8_t)rOBPD;
	gb->cpu.b = 4;
	copy_pals_hblank(gb);
}

// writes a 2x2 block of a at hl (VRAM bank already mapped); de = TILEMAP_WIDTH
// ld [hli], a / ld [hld], a / add hl, de / ld [hli], a / ld [hl], a
static uint16_t fill_2x2(GB *gb, uint16_t hl, uint8_t a)
{
	wr8(gb, hl++, a);
	wr8(gb, hl--, a);
	hl = alu_add16(gb, hl, REG_DE(gb));
	wr8(gb, hl++, a);
	wr8(gb, hl, a);
	return hl;
}

// .Func_1e499c: draws an empty power-up slot at hl
static void draw_empty_slot(GB *gb, uint16_t hl)
{
	SET_DE(gb, TILEMAP_WIDTH);
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	gb->cpu.a = 0x07;
	fill_2x2(gb, hl, 0x07);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	wr8(gb, hl++, 0x64);
	gb->cpu.a = alu_inc(gb, 0x64);
	wr8(gb, hl--, gb->cpu.a);
	hl = alu_add16(gb, hl, REG_DE(gb));
	wr8(gb, hl++, 0x74);
	gb->cpu.a = alu_inc(gb, 0x74);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT(LoadActionHelpBGMaps)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_ActionHelpAttrmap);
	SET_BC(gb, v1BGMap1);
	CALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_ActionHelpTilemap);
	SET_BC(gb, v0BGMap1);
	CALL(Decompress);

	gb->cpu.a = alu_and(gb, rd8(gb, wActionHelpPowerUp), POWER_UP_MASK);
	alu_cp(gb, gb->cpu.a, NUM_MAIN_POWER_UPS);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.b = 0x0a;
	} else {
		gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), POWER_UP_MASK);
		if (gb->cpu.a) {
			gb->cpu.c = gb->cpu.a;
			gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
			wr8(gb, rVBK, gb->cpu.a);
			uint16_t hl = v1BGMap1 + 16 * TILEMAP_WIDTH + 0;
			SET_DE(gb, TILEMAP_WIDTH);
			gb->cpu.b = 0;
			do {
				gb->cpu.a = 0x03;
				fill_2x2(gb, hl, 0x03);
				hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl + 2)); // inc l / inc l
				gb->cpu.b = alu_inc(gb, gb->cpu.b);
				gb->cpu.a = gb->cpu.b;
				alu_cp(gb, gb->cpu.a, gb->cpu.c);
			} while (gb->cpu.f & FLAG_C);
			SET_HL(gb, hl);
		}
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, rVBK, 0);
		gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), POWER_UP_MASK);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_sub(gb, 0x09, gb->cpu.b);
		if (!gb->cpu.a)
			return;
		gb->cpu.b = gb->cpu.a;
	}
	uint16_t hl = v0BGMap1 + 16 * TILEMAP_WIDTH + 18;
	do {
		draw_empty_slot(gb, hl);
		hl -= 2;
		SET_HL(gb, hl);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
}
