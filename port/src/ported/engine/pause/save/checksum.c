// Port of src/engine/pause/save/checksum.asm
#include "ported.h"

// de += [hl++]
static void add_byte(GB *gb, uint16_t *hl)
{
	gb->cpu.a = alu_add(gb, rd8(gb, (*hl)++), gb->cpu.e);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.a = alu_adc(gb, gb->cpu.d, 0);
	gb->cpu.d = gb->cpu.a;
}

// sums b bytes starting at hl
// result is added in de
PORT(CalculateChecksum)
{
	uint16_t hl = REG_HL(gb);
	do {
		add_byte(gb, &hl);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	SET_HL(gb, hl);
}

// same as CalculateChecksum but for bc bytes; buggy, after the first byte
// it jumps into CalculateChecksum
// The jr runs whatever bank is mapped, so it's a JUMP_TO, not a JUMP.
PORT_JUMP(CalculateChecksumLong)
{
	uint16_t hl = REG_HL(gb);
	add_byte(gb, &hl);
	SET_HL(gb, hl);
	SET_BC(gb, REG_BC(gb) - 1);
	gb->cpu.a = alu_or(gb, gb->cpu.b, gb->cpu.c);
	if (!(gb->cpu.f & FLAG_Z)) {
		JUMP_TO(ADDR_CalculateChecksum);
		return;
	}
	RET();
}

// increments 4-byte counter in wSaveCounter
PORT(IncrementSaveCounter)
{
	gb->cpu.a = alu_add(gb, rd8(gb, wSaveCounter + 3), 1);
	wr8(gb, wSaveCounter + 3, gb->cpu.a);
	for (int i = 2; i >= 0; i--) {
		gb->cpu.a = alu_adc(gb, rd8(gb, (uint16_t)(wSaveCounter + i)), 0);
		wr8(gb, (uint16_t)(wSaveCounter + i), gb->cpu.a);
	}
}

// SIZEOF("Level Objects WRAM"), SIZEOF("Progress WRAM") + SIZEOF("Level WRAM")
#define SIZEOF_LEVEL_OBJECTS_WRAM 0x14a
#define SIZEOF_PROGRESS_LEVEL_WRAM 0xca

// push_wram bank
static void push_wram(GB *gb, uint8_t bank)
{
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = bank;
	wr8(gb, rWBK, bank);
}

static void pop_wram(GB *gb)
{
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

PORT(Func_1f0b5b)
{
	SET_DE(gb, 0);
	gb->cpu.b = 0xca;
	CALL(CalculateChecksum);
	push_wram(gb, 0x02);
	gb->cpu.b = 0x11;
	CALL(CalculateChecksum);
	pop_wram(gb);
	push_wram(gb, BANK_wObjects);
	SET_BC(gb, SIZEOF_LEVEL_OBJECTS_WRAM);
	CALL(CalculateChecksumLong);
	pop_wram(gb);
}

PORT(CalculateWRAMDataChecksum)
{
	SET_DE(gb, 0);
	SET_HL(gb, 0xca00); // STARTOF("Progress WRAM")
	gb->cpu.b = SIZEOF_PROGRESS_LEVEL_WRAM;
	CALL(CalculateChecksum);
	push_wram(gb, BANK_wTreasuresCollected);
	SET_HL(gb, wTreasuresCollected);
	gb->cpu.b = (uint8_t)((wOWLevel - wTreasuresCollected) + 2);
	CALL(CalculateChecksum);
	pop_wram(gb);
	push_wram(gb, BANK_wObjects);
	SET_HL(gb, wObjects);
	SET_BC(gb, SIZEOF_LEVEL_OBJECTS_WRAM);
	CALL(CalculateChecksumLong);
	pop_wram(gb);
}

// de += sum of the bc bytes at hl (dec c / jr nz / dec b / jr nz)
static void sum_bc(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	do {
		do {
			add_byte(gb, &hl);
			gb->cpu.c = alu_dec(gb, gb->cpu.c);
		} while (gb->cpu.c);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	SET_HL(gb, hl);
}

// checksum of the whole of sram bank 1 (or 3), then half of sram bank 2
static void backup_sram_checksum(GB *gb, uint8_t bank, uint16_t start, uint16_t sram2)
{
	gb->cpu.a = rd8(gb, wSRAMBank);
	push_af(gb);
	gb->cpu.a = bank;
	sramswitch(gb, bank);
	SET_HL(gb, start);
	SET_DE(gb, 0);
	SET_BC(gb, 0x2000); // SIZEOF(SRAM)
	sum_bc(gb);
	pop_af(gb);
	sramswitch(gb, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSRAMBank);
	push_af(gb);

	gb->cpu.a = BANK_s2a000;
	sramswitch(gb, BANK_s2a000);
	SET_HL(gb, sram2);
	SET_BC(gb, 0x1000);
	sum_bc(gb);
	pop_af(gb);
	sramswitch(gb, gb->cpu.a);
}

// calculates checksum for SRAM1 and first half of SRAM2
// outputs result in de
PORT(CalculateBackupSRAMChecksum1)
{
	backup_sram_checksum(gb, BANK_sLevelBlockObjectMap, sLevelBlockObjectMap, s2a000);
}

// calculates checksum for SRAM3 and second half of SRAM2
// outputs result in de
PORT(CalculateBackupSRAMChecksum2)
{
	backup_sram_checksum(gb, BANK_s3a000, s3a000, s2b000);
}
