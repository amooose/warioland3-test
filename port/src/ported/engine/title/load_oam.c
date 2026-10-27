// Port of src/engine/title/load_oam.asm
#include "ported.h"

// hl = y, x, frame and attributes (y relative to the BG)
PORT(LoadIntroSprite)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.b = rd8(gb, wSCY);
	gb->cpu.a = alu_sub(gb, alu_add(gb, rd8(gb, hl++), OAM_Y_OFS), gb->cpu.b);
	wr8(gb, wCurSpriteYCoord, gb->cpu.a);
	gb->cpu.a = alu_add(gb, rd8(gb, hl++), OAM_X_OFS);
	wr8(gb, wCurSpriteXCoord, gb->cpu.a);
	wr8(gb, wCurSpriteFrame, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wCurSpriteAttributes, gb->cpu.a);
	SET_HL(gb, ADDR_OAM_6d21);
	CALL(LoadSprite);
}

// hl = y, x, frame and attributes (y relative to the BG)
PORT(LoadStartMenuSprite)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.b = rd8(gb, wSCY);
	gb->cpu.a = alu_sub(gb, alu_add(gb, rd8(gb, hl++), OAM_Y_OFS), gb->cpu.b);
	wr8(gb, wCurSpriteYCoord, gb->cpu.a);
	gb->cpu.a = alu_add(gb, rd8(gb, hl++), OAM_X_OFS);
	wr8(gb, wCurSpriteXCoord, gb->cpu.a);
	wr8(gb, wCurSpriteFrame, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wCurSpriteAttributes, gb->cpu.a);
	SET_HL(gb, ADDR_OAM_6b5f);
	CALL(LoadSprite);
}
