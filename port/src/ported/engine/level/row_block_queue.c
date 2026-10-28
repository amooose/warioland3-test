// Port of src/engine/level/row_block_queue.asm
#include "wario_states.h"

enum { STEP_FULL, STEP_FIRST, STEP_LAST_FULL, STEP_LAST_HALF };

// One block of the column at hl (in the level's block map): copies bytes
// of its 4-byte entry in base (skipping incs bytes) to the queue at
// [ptr_var], and (for the non-last steps) moves hl down a row, into the
// next SRAM bank at the end of one.
static void queue_column_step(GB *gb, uint16_t base, uint16_t ptr_var, int incs, int kind)
{
	uint16_t map = REG_HL(gb);
	gb->cpu.a = rd8(gb, map);
	gb->cpu.d = 0x00;
	gb->cpu.e = gb->cpu.a;
	gb->cpu.e = alu_sla(gb, gb->cpu.e);
	gb->cpu.e = alu_sla(gb, gb->cpu.e);
	gb->cpu.d = alu_rl(gb, gb->cpu.d);
	gb_push16(gb, map);
	uint16_t hl = alu_add16(gb, base, REG_DE(gb));
	for (int i = 0; i < incs; i++)
		hl = (uint16_t)((hl & 0xff00) | alu_inc(gb, (uint8_t)hl)); // inc l
	gb->cpu.a = rd8(gb, ptr_var + 0);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.a = rd8(gb, ptr_var + 1);
	gb->cpu.e = gb->cpu.a;
	if (kind == STEP_FULL) {
		gb->cpu.a = alu_add(gb, gb->cpu.a, 2);
		wr8(gb, ptr_var + 1, gb->cpu.a);
	} else if (kind == STEP_FIRST) {
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
		wr8(gb, ptr_var + 1, gb->cpu.a);
	}
	if (kind == STEP_FIRST || kind == STEP_LAST_HALF) {
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, REG_DE(gb), gb->cpu.a);
	} else {
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		hl = (uint16_t)((hl & 0xff00) | alu_inc(gb, (uint8_t)hl)); // inc l
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, REG_DE(gb), gb->cpu.a);
	}
	SET_HL(gb, gb_pop16(gb));
	if (kind == STEP_FULL || kind == STEP_FIRST) {
		gb->cpu.h = alu_inc(gb, gb->cpu.h);
		gb->cpu.a = gb->cpu.h;
		alu_cp(gb, gb->cpu.a, 0xc0); // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
		if (ZF) {
			gb->cpu.h = (uint8_t)(sLevelBlockObjectMap >> 8);
			gb->cpu.a = alu_inc(gb, rd8(gb, wSRAMBank));
			sramswitch(gb, gb->cpu.a);
		}
	}
}

// Queues 16 blocks' tiles (or attributes, in WRAM bank "GFX RAM") along
// the level map at hl: the left (o = 0) or right (o = 1) tiles of each
// block, starting halfway into the first block if wBlockYPos is.
// quirk (QueueBlockAttributesAlongRow_OddColumn): a half step at the end of
// a whole first block, and the half first block read from the queue address
static void queue_row(GB *gb, uint16_t queue, uint16_t ptr_var, uint16_t base, int o, bool wram, bool quirk)
{
	if (wram) {
		gb->cpu.a = rd8(gb, rWBK);
		push_af(gb);
		st(gb, rWBK, BANK_wRoomBlockAttributes); // BANK("GFX RAM")
	}
	gb->cpu.a = rd8(gb, wBGMapTileQueueSize);
	gb->cpu.b = gb->cpu.a;
	st(gb, ptr_var + 0, (uint8_t)(queue >> 8));
	gb->cpu.a = alu_add(gb, (uint8_t)queue, gb->cpu.b);
	wr8(gb, ptr_var + 1, gb->cpu.a); // queue + wBGMapTileQueueSize
	bool half = ld_mask(gb, wBlockYPos + 1, 0x08);
	if (half)
		queue_column_step(gb, quirk ? wBGMapTileVRAM1Queue : base, ptr_var, o + 2, STEP_FIRST);
	gb->cpu.b = 0x10;
	do {
		queue_column_step(gb, base, ptr_var, o, STEP_FULL);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	if (quirk && !half)
		queue_column_step(gb, base, ptr_var, o, STEP_LAST_HALF);
	if (wram) {
		pop_af(gb);
		wr8(gb, rWBK, gb->cpu.a);
	}
}

PORT(QueueBlockTilesAlongRow_OddColumn) { queue_row(gb, wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, 1, false, false); }
PORT(QueueBlockAttributesAlongRow_OddColumn) { queue_row(gb, wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, 1, true, true); }
PORT(QueueBlockTilesAlongRow_EvenColumn) { queue_row(gb, wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, 0, false, false); }
PORT(QueueBlockAttributesAlongRow_EvenColumn) { queue_row(gb, wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, 0, true, false); }
