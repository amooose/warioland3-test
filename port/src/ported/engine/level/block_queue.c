// Port of src/engine/level/block_queue.asm
#include "ported.h"

// hl = obj unk02
// Removes an object, marking its spawn point in the level's object map
// (in SRAM) so it can spawn again.
PORT(DespawnObject)
{
	// push_wram BANK("Level Objects WRAM")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	wr8(gb, rWBK, BANK_wObjects);
	gb->cpu.a = rd8(gb, wSRAMBank);
	push_af(gb);
	sramswitch(gb, BANK_sLevelBlockObjectMap); // SRAM1

	// the object map spans several SRAM banks of $20 pages each
	uint16_t hl = REG_HL(gb);
	gb->cpu.c = 1;
	uint8_t page = rd8(gb, hl--); // OBJ_UNK_02
	alu_cp(gb, page, 0xc0); // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.c = alu_inc(gb, gb->cpu.c);
		page = alu_sub(gb, page, 0x20);
		alu_cp(gb, page, 0xc0);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.c = alu_inc(gb, gb->cpu.c);
			page = alu_sub(gb, page, 0x20);
		}
	}
	gb->cpu.d = page;
	gb->cpu.e = rd8(gb, hl--); // OBJ_UNK_01
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
	sramswitch(gb, gb->cpu.c);
	uint16_t de = REG_DE(gb);
	wr8(gb, de, alu_or(gb, rd8(gb, de), 0x80));

	pop_af(gb);
	sramswitch(gb, gb->cpu.a);
	pop_af(gb); // pop_wram
	wr8(gb, rWBK, gb->cpu.a);
}
