// Port of src/engine/clear/add_sprite.asm
#include "ported.h"

// hl = y, x, frame and attributes (y relative to the BG)
static void load_clear_screen_sprite(GB *gb, uint16_t oam)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.b = rd8(gb, wSCY);
	gb->cpu.a = alu_sub(gb, alu_add(gb, rd8(gb, hl++), OAM_Y_OFS), gb->cpu.b);
	wr8(gb, wCurSprite, gb->cpu.a);
	gb->cpu.a = alu_add(gb, rd8(gb, hl++), OAM_X_OFS);
	wr8(gb, wCurSpriteXCoord, gb->cpu.a);
	wr8(gb, wCurSpriteFrame, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wCurSpriteAttributes, gb->cpu.a);
	SET_HL(gb, oam);
	CALL(LoadSprite);
}

PORT(LoadClearScreenSprite) { load_clear_screen_sprite(gb, ADDR_OAM_d4e26); }
PORT(LoadClearScreenWarioSprite) { load_clear_screen_sprite(gb, ADDR_OAM_d49cc); }
