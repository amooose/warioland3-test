// Port of src/home/load_credits_sprite.asm
#include "ported.h"

PORT(LoadCreditsMusicBoxSprite)
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
	gb->cpu.a = rd8(gb, wCreditsMusicBoxOAMBank);
	wr8(gb, wTempBank, gb->cpu.a);
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = rd8(gb, wTempBank);
	bankswitch(gb, gb->cpu.a);
	gb->cpu.h = rd8(gb, wCreditsMusicBoxOAMPtr + 0);
	gb->cpu.a = rd8(gb, wCreditsMusicBoxOAMPtr + 1);
	gb->cpu.l = gb->cpu.a;
	CALL(LoadSprite);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}
