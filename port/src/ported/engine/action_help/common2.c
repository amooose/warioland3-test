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
