// Port of src/engine/pause/pause_menu_2.asm
#include "ported.h"

// hl = y, x, frame and attributes of a pause menu sprite
PORT(LoadPauseMenuSprite)
{
	uint16_t hl = REG_HL(gb);
	wr8(gb, wCurSpriteYCoord, alu_add(gb, rd8(gb, hl++), OAM_Y_OFS));
	wr8(gb, wCurSpriteXCoord, alu_add(gb, rd8(gb, hl++), OAM_X_OFS));
	wr8(gb, wCurSpriteFrame, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wCurSpriteAttributes, gb->cpu.a);
	SET_HL(gb, ADDR_OAM_1f156c);
	CALL(LoadSprite);
}
