// Port of src/engine/clear/treasure_count.asm
#include "ported.h"

PORT(UpdateNumCollectedTreasures)
{
	// push_wram BANK(wTreasuresCollected)
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = BANK_wTreasuresCollected;
	wr8(gb, rWBK, gb->cpu.a);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wNumCollectedTreasures + 1, 0);
	wr8(gb, wNumCollectedTreasures + 0, 0);
	gb->cpu.a = NUM_TREASURES - 1;
	do {
		push_af(gb);
		CALL(IsTreasureCollected);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (gb->cpu.a) {
			gb->cpu.a = alu_add(gb, rd8(gb, wNumCollectedTreasures + 1), 1);
			gb->cpu.a = alu_daa(gb, gb->cpu.a);
			wr8(gb, wNumCollectedTreasures + 1, gb->cpu.a);
			gb->cpu.a = alu_adc(gb, rd8(gb, wNumCollectedTreasures + 0), 0);
			gb->cpu.a = alu_daa(gb, gb->cpu.a);
			wr8(gb, wNumCollectedTreasures + 0, gb->cpu.a);
		}
		pop_af(gb);
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
	} while (gb->cpu.a);

	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}
