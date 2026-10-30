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

// --- queueing a row of blocks' tiles/attributes ---

// One block of the row at hl (in the level's block map, hl moves right):
// like queue_column_step, without the move to the next row.
static void queue_row_step(GB *gb, uint16_t base, uint16_t ptr_var, int incs, int kind)
{
	uint16_t map = REG_HL(gb);
	gb->cpu.a = rd8(gb, map++);
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
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, REG_DE(gb), gb->cpu.a);
	}
	SET_HL(gb, gb_pop16(gb));
}

// Queues the 13 blocks' tiles (or attributes, in WRAM bank 3) of a row of
// the level map at hl: the top or bottom tiles of each block, starting
// halfway into the first block if wBlockXPos is.
static void queue_row(GB *gb, uint16_t queue, uint16_t ptr_var, uint16_t base, bool bottom, bool wram)
{
	gb->cpu.a = rd8(gb, wBGMapTileQueueSize);
	gb->cpu.b = gb->cpu.a;
	st(gb, ptr_var + 0, (uint8_t)(queue >> 8));
	gb->cpu.a = alu_add(gb, (uint8_t)queue, gb->cpu.b);
	wr8(gb, ptr_var + 1, gb->cpu.a); // queue + wBGMapTileQueueSize
	bool half = ld_mask(gb, wBlockXPos + 1, 0x08);
	if (wram) {
		gb->cpu.a = rd8(gb, rWBK);
		push_af(gb);
		st(gb, rWBK, 0x03);
	}
	int o = bottom ? 2 : 0;
	if (!half) {
		for (int i = 0; i < 12; i++)
			queue_row_step(gb, base, ptr_var, o, STEP_FULL);
		queue_row_step(gb, base, ptr_var, o, STEP_LAST_HALF);
	} else {
		queue_row_step(gb, base, ptr_var, o + 1, STEP_FIRST);
		if (bottom) {
			for (int i = 0; i < 11; i++)
				queue_row_step(gb, base, ptr_var, o, STEP_FULL);
			queue_row_step(gb, base, ptr_var, o, STEP_LAST_FULL);
		} else {
			for (int i = 0; i < 12; i++)
				queue_row_step(gb, base, ptr_var, o, STEP_FULL);
		}
	}
	if (wram) {
		pop_af(gb);
		wr8(gb, rWBK, gb->cpu.a);
	}
}

// ld hl, cam + 1 / ld a, [hld] / and $f8 / ld e, a / ld h, [hl] / ld l, e /
// ld de, add / add hl, de
static void cam_pos(GB *gb, uint16_t cam, uint16_t add)
{
	gb->cpu.a = alu_and(gb, rd8(gb, cam + 1), 0xf8);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.h = rd8(gb, cam);
	gb->cpu.l = gb->cpu.e;
	SET_DE(gb, add);
	SET_HL(gb, alu_add16(gb, REG_HL(gb), add));
}

// bit 7, h / jr z / ld hl, 0; true if it was negative
static bool clamp_neg(GB *gb)
{
	if (!alu_bit(gb, gb->cpu.h, 7))
		return false;
	SET_HL(gb, 0);
	return true;
}

// ld a, h / cp lim / jr c / ld hl, max / ld a, h
static void clamp_max(GB *gb, uint8_t lim, uint16_t max)
{
	gb->cpu.a = gb->cpu.h;
	alu_cp(gb, gb->cpu.a, lim);
	if (!(gb->cpu.f & FLAG_C)) {
		SET_HL(gb, max);
		gb->cpu.a = gb->cpu.h;
	}
}

// ld a, h / ld [var + 0], a / ld a, l / ld [var + 1], a
static void store_pos(GB *gb, uint16_t var)
{
	gb->cpu.a = gb->cpu.h;
	wr8(gb, var + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.l;
	wr8(gb, var + 1, gb->cpu.a);
}

// The y of a column at the right or left edge: the camera's - $10,
// clamped to 0, and to $240 when + $b8 reaches $300
static void column_y(GB *gb)
{
	cam_pos(gb, wCameraSCY, (uint16_t)-0x10);
	if (!clamp_neg(gb)) {
		uint16_t hl = REG_HL(gb);
		SET_DE(gb, 0xb8);
		gb_push16(gb, hl);
		SET_HL(gb, alu_add16(gb, hl, 0xb8));
		gb->cpu.a = gb->cpu.h;
		alu_cp(gb, gb->cpu.a, 0x03);
		SET_HL(gb, gb_pop16(gb));
		if (!(gb->cpu.f & FLAG_C))
			SET_HL(gb, 0x240);
	}
	store_pos(gb, wBlockYPos);
}

// the camera's x - $10, clamped to 0
static void left_x(GB *gb)
{
	cam_pos(gb, wCameraSCX, (uint16_t)-0x10);
	clamp_neg(gb);
	store_pos(gb, wBlockXPos);
}

// Func_8ed9.Func_8f79: the row below the screen
static void edge_down_pos(GB *gb)
{
	cam_pos(gb, wCameraSCY, 0xa0);
	clamp_max(gb, 0x03, 0x2f8);
	store_pos(gb, wBlockYPos);
	left_x(gb);
}

// Func_8ed9.Func_8fb3: the row above the screen
static void edge_up_pos(GB *gb)
{
	cam_pos(gb, wCameraSCY, (uint16_t)-0x10);
	clamp_neg(gb);
	store_pos(gb, wBlockYPos);
	left_x(gb);
}

// Func_8ed9.Func_8fec: the column right of the screen
static void edge_right_pos(GB *gb)
{
	column_y(gb);
	cam_pos(gb, wCameraSCX, 0xb0);
	clamp_max(gb, 0x0a, 0x9f8);
	store_pos(gb, wBlockXPos);
}

// Func_8ed9.Func_9039: the column left of the screen
static void edge_left_pos(GB *gb)
{
	column_y(gb);
	left_x(gb);
}

// Func_8ed9.Func_9085: queues a row of 25 BG map addresses and the block
// tiles and attributes to draw there
static void queue_edge_row(GB *gb)
{
	gb_call(gb, ADDR_GetBlockBGPtr); // same bank: whatever is mapped
	gb->cpu.a = rd8(gb, wBGMapAddressQueueSize);
	gb->cpu.b = gb->cpu.a;
	SET_DE(gb, wBGMapAddressQueue);
	gb->cpu.a = alu_add(gb, gb->cpu.e, gb->cpu.b);
	gb->cpu.e = gb->cpu.a;
	for (int i = 0; i < 25; i++) {
		gb->cpu.a = gb->cpu.h;
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = gb->cpu.l;
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0xe0);
		gb->cpu.c = gb->cpu.a;
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.l = alu_inc(gb, gb->cpu.l);
		gb->cpu.a = alu_and(gb, gb->cpu.l, 0x1f);
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
		gb->cpu.l = gb->cpu.a;
	}
	gb->cpu.a = alu_add(gb, rd8(gb, wBGMapAddressQueueSize), 2 * 25);
	wr8(gb, wBGMapAddressQueueSize, gb->cpu.a);

	SET_HL(gb, wBlockPos);
	CALL(GetBlockPtr);
	sramswitch(gb, gb->cpu.a = rd8(gb, wBlockPtrBank));
	bool bottom = ld_mask(gb, wBlockYPos + 1, 0x08);
	gb_push16(gb, REG_HL(gb));
	queue_row(gb, wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, bottom, false);
	SET_HL(gb, gb_pop16(gb));
	sramswitch(gb, gb->cpu.a = rd8(gb, wBlockPtrBank));
	queue_row(gb, wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, bottom, true);
	gb->cpu.a = alu_add(gb, rd8(gb, wBGMapTileQueueSize), 25);
	wr8(gb, wBGMapTileQueueSize, gb->cpu.a);
}

// Draws the rows and columns that scrolled into view (wLevelEdgeUpdateFlags)
// and spawns the objects there
PORT(Func_8ed9)
{
	gb->cpu.a = rd8(gb, wLevelEdgeUpdateFlags);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.a, LEVEL_EDGE_UPDATE_RIGHT | LEVEL_EDGE_UPDATE_LEFT);
	uint8_t flags = gb->cpu.b;
	if (gb->cpu.a) {
		bool right = alu_bit(gb, gb->cpu.b, LEVEL_EDGE_UPDATE_RIGHT_F);
		if (right || alu_bit(gb, gb->cpu.b, LEVEL_EDGE_UPDATE_LEFT_F)) {
			if (right)
				edge_right_pos(gb);
			else
				edge_left_pos(gb);
			gb_call(gb, ADDR_Func_a0e2); // same bank: whatever is mapped
			SET_HL(gb, wLevelEdgeUpdateFlags);
			wr8(gb, wLevelEdgeUpdateFlags, rd8(gb, wLevelEdgeUpdateFlags) &
				(uint8_t)~(right ? LEVEL_EDGE_UPDATE_RIGHT : LEVEL_EDGE_UPDATE_LEFT));
			FARCALL(SpawnRowEdgeObjects);
			gb->cpu.a = rd8(gb, wLevelEdgeUpdateFlags);
			flags = gb->cpu.a;
		}
	}
	// .check_vertical_direction
	bool up = alu_bit(gb, flags, LEVEL_EDGE_UPDATE_UP_F);
	if (!up && !alu_bit(gb, flags, LEVEL_EDGE_UPDATE_DOWN_F))
		return;
	if (up)
		edge_up_pos(gb);
	else
		edge_down_pos(gb);
	queue_edge_row(gb);
	SET_HL(gb, wLevelEdgeUpdateFlags);
	wr8(gb, wLevelEdgeUpdateFlags, rd8(gb, wLevelEdgeUpdateFlags) &
		(uint8_t)~(up ? LEVEL_EDGE_UPDATE_UP : LEVEL_EDGE_UPDATE_DOWN));
	FARCALL(SpawnColEdgeObjects);
}
