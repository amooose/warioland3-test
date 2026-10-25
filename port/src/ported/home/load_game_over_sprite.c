// Port of src/home/load_game_over_sprite.asm
#include "ported.h"

PORT(LoadGameOverSprite)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = alu_add(gb, rd8(gb, hl++), OAM_Y_OFS);
	wr8(gb, wCurSpriteYCoord, gb->cpu.a);
	gb->cpu.a = alu_add(gb, rd8(gb, hl++), OAM_X_OFS);
	wr8(gb, wCurSpriteXCoord, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wCurSpriteFrame, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wCurSpriteAttributes, gb->cpu.a);
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = BANK_OAM_aab5c;
	bankswitch(gb, gb->cpu.a);
	SET_HL(gb, ADDR_OAM_aab5c);
	CALL(LoadSprite);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}
