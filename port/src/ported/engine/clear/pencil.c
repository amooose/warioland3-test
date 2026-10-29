// Port of src/engine/clear/pencil.asm
#include "ported.h"

PORT(LoadPencilSprite)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.b = rd8(gb, wSCY);
	gb->cpu.a = alu_sub(gb, alu_add(gb, rd8(gb, hl++), OAM_Y_OFS), gb->cpu.b);
	wr8(gb, wCurSprite, gb->cpu.a);
	wr8(gb, wCurSpriteXCoord, gb->cpu.a = alu_add(gb, rd8(gb, hl++), OAM_X_OFS));
	wr8(gb, wCurSpriteFrame, gb->cpu.a = rd8(gb, hl++));
	wr8(gb, wCurSpriteAttributes, gb->cpu.a = rd8(gb, hl));
	SET_HL(gb, ADDR_OAM_d65e1);
	CALL(LoadSprite);
}

// returns TRUE in a if done movement
PORT(DoPencilMovement)
{
	gb->cpu.a = rd8(gb, wAllMusicalCoinsLevel);
	static const uint8_t thresholds[] = { NUM_LEVELS - 5, NUM_LEVELS - 10, NUM_LEVELS - 15, NUM_LEVELS - 20 };
	static const uint8_t subs[] = { 20, 15, 10, 5 };
	static const uint8_t ys[] = { 0x68, 0x60, 0x58, 0x50 };
	int row = -1;
	for (int i = 0; i < 4; i++) {
		alu_cp(gb, gb->cpu.a, thresholds[i]);
		if (!(gb->cpu.f & FLAG_C)) {
			row = i;
			break;
		}
	}
	if (row < 0) {
		gb->cpu.b = gb->cpu.a;
		gb->cpu.d = 0x48;
	} else {
		gb->cpu.a = alu_sub(gb, gb->cpu.a, subs[row]);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.d = ys[row];
	}
	// .got_y
	SET_HL(gb, wMenuObj1YCoord);
	gb->cpu.a = rd8(gb, wMenuObj1YCoord);
	alu_cp(gb, gb->cpu.a, gb->cpu.d);
	if (!(gb->cpu.f & FLAG_Z)) {
		// move down
		wr8(gb, wMenuObj1YCoord, alu_inc(gb, gb->cpu.a));
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		return;
	}
	// .check_column
	static const uint8_t xs[] = { 0x48, 0x50, 0x58, 0x60 };
	gb->cpu.e = 0x40;
	for (int i = 0; i < 4; i++) {
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.e = xs[i];
			break;
		}
	}
	// .got_x
	SET_HL(gb, wMenuObj1XCoord);
	gb->cpu.a = rd8(gb, wMenuObj1XCoord);
	alu_cp(gb, gb->cpu.a, gb->cpu.e);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = TRUE;
		return;
	}
	// move left
	wr8(gb, wMenuObj1XCoord, alu_dec(gb, gb->cpu.a));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

PORT(DoPencilScribbling)
{
	gb->cpu.a = rd8(gb, wTimer);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, ADDR_PencilScribbleXOffsets, REG_DE(gb));
	gb->cpu.a = alu_add(gb, rd8(gb, wMenuObj1YCoord), rd8(gb, hl));
	wr8(gb, wMenuObj1YCoord, gb->cpu.a);
	hl = alu_add16(gb, ADDR_PencilScribbleYOffsets, REG_DE(gb));
	gb->cpu.a = alu_add(gb, rd8(gb, wMenuObj1XCoord), rd8(gb, hl));
	wr8(gb, wMenuObj1XCoord, gb->cpu.a);
	SET_HL(gb, hl);
}
