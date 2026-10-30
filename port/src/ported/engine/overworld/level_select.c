// Port of src/engine/overworld/level_select.asm
#include "ported.h"

// ld a, [hli] / push hl / call IsTreasureCollected / pop hl
static uint16_t check_treasure(GB *gb, uint16_t hl)
{
	gb->cpu.a = rd8(gb, hl++);
	gb_push16(gb, hl);
	CALL(IsTreasureCollected);
	return gb_pop16(gb);
}

static void pop_wram(GB *gb)
{
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

PORT(SelectLevel)
{
	// push_wram 2
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 2;
	wr8(gb, rWBK, 2);

	gb->cpu.a = rd8(gb, wOWLevel);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a || (alu_cp(gb, gb->cpu.a, LEVEL_GOLF_BUILDING), gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = gb->cpu.a ? GOLF_BUILDING : THE_TEMPLE;
		wr8(gb, wLevel, gb->cpu.a);
		pop_wram(gb);
		return;
	}

	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	wr8(gb, wLevel, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, ADDR_LevelTreasureRequirements, REG_DE(gb));

	uint8_t variant = 0;
	for (int v = 3; v >= 1; v--) {
		hl = check_treasure(gb, hl);
		gb->cpu.c = gb->cpu.a;
		hl = check_treasure(gb, hl);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.c);
		if (gb->cpu.a) {
			variant = (uint8_t)v;
			break;
		}
	}
	SET_HL(gb, hl);
	if (variant == 3)
		gb->cpu.a = alu_add(gb, rd8(gb, wLevel), 3);
	else if (variant == 2)
		gb->cpu.a = alu_add(gb, rd8(gb, wLevel), 2);
	else if (variant == 1)
		gb->cpu.a = alu_inc(gb, rd8(gb, wLevel));
	if (variant)
		wr8(gb, wLevel, gb->cpu.a);

	// .day_night
	gb->cpu.a = alu_and(gb, rd8(gb, wDayNight), 1);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_or(gb, rd8(gb, wLevel), gb->cpu.b);
	wr8(gb, wLevel, gb->cpu.a);
	pop_wram(gb);
}
