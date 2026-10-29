// Port of src/engine/level/scroll.asm
#include "wario_states.h"

// --- queueing a column of blocks' tiles/attributes ---

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

// Queues the 12 blocks' tiles (or attributes, in WRAM bank 3) of a column
// of the level map at hl: the left (o = 0) or right (o = 1) tiles of each
// block, starting halfway down the first block if wBlockYPos is.
static void queue_column(GB *gb, uint16_t queue, uint16_t ptr_var, uint16_t base, int o, bool wram)
{
	gb->cpu.a = rd8(gb, wBGMapTileQueueSize);
	gb->cpu.b = gb->cpu.a;
	st(gb, ptr_var + 0, (uint8_t)(queue >> 8));
	gb->cpu.a = alu_add(gb, (uint8_t)queue, gb->cpu.b);
	wr8(gb, ptr_var + 1, gb->cpu.a); // queue + wBGMapTileQueueSize
	bool half = ld_mask(gb, wBlockYPos + 1, 0x08);
	if (wram) {
		gb->cpu.a = rd8(gb, rWBK);
		push_af(gb);
		st(gb, rWBK, 0x03);
	}
	if (!half) {
		for (int i = 0; i < 11; i++)
			queue_column_step(gb, base, ptr_var, o, STEP_FULL);
		queue_column_step(gb, base, ptr_var, o, STEP_LAST_HALF);
	} else {
		queue_column_step(gb, base, ptr_var, o + 2, STEP_FIRST);
		for (int i = 0; i < 10; i++)
			queue_column_step(gb, base, ptr_var, o, STEP_FULL);
		queue_column_step(gb, base, ptr_var, o, STEP_LAST_FULL);
	}
	if (wram) {
		pop_af(gb);
		wr8(gb, rWBK, gb->cpu.a);
	}
}

PORT(Func_a2aa) { queue_column(gb, wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, 1, false); }
PORT(Func_a79e) { queue_column(gb, wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, 1, true); }
PORT(Func_aca6) { queue_column(gb, wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, 0, false); }
PORT(Func_b182) { queue_column(gb, wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, 0, true); }

// hl = Wario's position
// wLevelRoomID = XPosHi + 10 * YPosHi
PORT(GetNextLevelRoomID)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++); // hi y
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		hl++;
		gb->cpu.a = rd8(gb, hl); // hi x
	} else {
		gb->cpu.b = gb->cpu.a;
		hl++;
		gb->cpu.a = rd8(gb, hl); // hi x
		gb->cpu.c = 10;
		do {
			gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
			gb->cpu.b = alu_dec(gb, gb->cpu.b);
		} while (gb->cpu.b);
	}
	SET_HL(gb, hl);
	wr8(gb, wLevelRoomID, gb->cpu.a);
}

// Queues a column of 23 BG map addresses and the block tiles and attributes
// to draw there.
PORT(Func_a0e2)
{
	gb_call(gb, ADDR_GetBlockBGPtr); // same bank: whatever is mapped
	gb->cpu.a = rd8(gb, wBGMapAddressQueueSize);
	gb->cpu.b = gb->cpu.a;
	SET_DE(gb, wBGMapAddressQueue);
	gb->cpu.a = alu_add(gb, gb->cpu.e, gb->cpu.b);
	gb->cpu.e = gb->cpu.a;
	uint16_t hl = REG_HL(gb);
	for (int i = 0; i < 23; i++) {
		wr8(gb, REG_DE(gb), (uint8_t)(hl >> 8));
		gb->cpu.e++;
		wr8(gb, REG_DE(gb), (uint8_t)hl);
		gb->cpu.e++;
		gb_push16(gb, REG_DE(gb));
		hl = alu_add16(gb, hl, TILEMAP_WIDTH);
		gb_pop16(gb);
		gb->cpu.a = alu_and(gb, (uint8_t)(hl >> 8), 0xfb);
		hl = (uint16_t)(gb->cpu.a << 8 | (hl & 0xff));
	}
	gb->cpu.a = alu_add(gb, rd8(gb, wBGMapAddressQueueSize), 2 * 23);
	wr8(gb, wBGMapAddressQueueSize, gb->cpu.a);

	SET_HL(gb, wBlockPos);
	CALL(GetBlockPtr);
	sramswitch(gb, gb->cpu.a = rd8(gb, wBlockPtrBank));
	gb->cpu.a = alu_and(gb, rd8(gb, wBlockXPos + 1), 0x08);
	bool odd = !(gb->cpu.f & FLAG_Z);
	gb_push16(gb, REG_HL(gb));
	gb_call(gb, odd ? ADDR_Func_a2aa : ADDR_Func_aca6);
	SET_HL(gb, gb_pop16(gb));
	sramswitch(gb, gb->cpu.a = rd8(gb, wBlockPtrBank));
	gb_call(gb, odd ? ADDR_Func_a79e : ADDR_Func_b182);
	gb->cpu.a = alu_add(gb, rd8(gb, wBGMapTileQueueSize), 23);
	wr8(gb, wBGMapTileQueueSize, gb->cpu.a);
}
