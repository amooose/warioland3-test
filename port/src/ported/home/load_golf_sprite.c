// Port of src/home/load_golf_sprite.asm
#include "ported.h"

// hl = source, wdc11 = destination, w1dc13 = length - 1
PORT(Func_2c30)
{
	gb->cpu.a = gb->cpu.h;
	wr8(gb, rVDMA_SRC_HIGH, gb->cpu.a);
	gb->cpu.a = gb->cpu.l;
	wr8(gb, rVDMA_SRC_LOW, gb->cpu.a);
	gb->cpu.a = rd8(gb, wdc11 + 0);
	wr8(gb, rVDMA_DEST_HIGH, gb->cpu.a);
	gb->cpu.a = rd8(gb, wdc11 + 1);
	wr8(gb, rVDMA_DEST_LOW, gb->cpu.a);
	gb->cpu.a = rd8(gb, w1dc13);
	wr8(gb, rVDMA_LEN, gb->cpu.a);
}

// copies 4 BG palettes from hl to rBGPD
PORT(Func_2c46)
{
	SET_DE(gb, rBGPI);
	gb->cpu.c = 4;
	gb->cpu.a = 0x80;
	wr8(gb, REG_DE(gb), gb->cpu.a);
	gb->cpu.e++; // inc e
	uint16_t hl = REG_HL(gb);
	uint16_t de = REG_DE(gb);
	do {
		for (int i = 0; i < 0x10; i++) {
			gb->cpu.a = rd8(gb, hl++);
			wr8(gb, de, gb->cpu.a);
		}
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
	} while (gb->cpu.c);
	SET_HL(gb, hl);
}

// hl = golf object duration, de = frameset
// outputs in c whether animation finished
PORT(UpdateGolfObjectAnimation)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.c = 0x00;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl), 1);
	wr8(gb, hl++, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_C)) {
		SET_HL(gb, hl);
		return;
	}
	gb->cpu.a = alu_add(gb, rd8(gb, hl++), gb->cpu.e);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_adc(gb, gb->cpu.d, 0x00);
	gb->cpu.b = gb->cpu.a;
	uint16_t bc = REG_BC(gb);
	gb->cpu.a = rd8(gb, bc);
	alu_cp(gb, gb->cpu.a, 0xff);
	if (gb->cpu.f & FLAG_Z) {
		// .finished_animation
		uint16_t de = REG_DE(gb);
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, wGolfAnimLastFrame, gb->cpu.a);
		gb->cpu.a = rd8(gb, de);
		wr8(gb, hl--, gb->cpu.a);
		de++;
		gb->cpu.a = 0x2;
		wr8(gb, hl--, gb->cpu.a);
		gb->cpu.a = rd8(gb, de);
		wr8(gb, hl, gb->cpu.a);
		SET_DE(gb, de);
		SET_HL(gb, hl);
		gb->cpu.c = 1; // TRUE
		return;
	}
	bc++;
	wr8(gb, hl--, gb->cpu.a); // frame
	gb->cpu.a = alu_add(gb, rd8(gb, hl), 0x2);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = rd8(gb, bc);
	wr8(gb, hl, gb->cpu.a); // duration
	SET_HL(gb, hl);
	gb->cpu.b = (uint8_t)(bc >> 8);
	gb->cpu.c = 0; // FALSE
}

// hl = golf object sprite, wGolfOAMPtr = oam pointer
PORT(LoadGolfSprite)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wCurSpriteYCoord, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wCurSpriteXCoord, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wCurSpriteFrame, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wCurSpriteAttributes, gb->cpu.a);
	gb->cpu.a = rd8(gb, wGolfOAMPtr + 0);
	gb->cpu.h = gb->cpu.a;
	gb->cpu.a = rd8(gb, wGolfOAMPtr + 1);
	gb->cpu.l = gb->cpu.a;
	CALL(LoadSprite);
}
