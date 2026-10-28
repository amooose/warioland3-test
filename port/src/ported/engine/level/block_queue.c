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

// push_wram BANK("Level Objects WRAM") / ld a, [wSRAMBank] / push af /
// ld a, $01 / sramswitch
static void push_banks(GB *gb)
{
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = BANK_wObjects;
	wr8(gb, rWBK, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSRAMBank);
	push_af(gb);
	gb->cpu.a = 0x01;
	sramswitch(gb, 0x01);
}

// pop af / sramswitch / pop_wram
static void pop_banks(GB *gb)
{
	pop_af(gb);
	sramswitch(gb, gb->cpu.a);
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

// c = 1 / ld a, [hld] / the SRAM bank (c) and page (a) of the object map
// pointer's high byte
static uint16_t obj_map_page(GB *gb, uint16_t hl)
{
	gb->cpu.c = 0x01;
	gb->cpu.a = rd8(gb, hl--);
	alu_cp(gb, gb->cpu.a, 0xc0); // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.c = alu_inc(gb, gb->cpu.c);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x20);
		alu_cp(gb, gb->cpu.a, 0xc0);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.c = alu_inc(gb, gb->cpu.c);
			gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x20);
		}
	}
	return hl;
}

// hl = the object map pointer's high byte: hl = its block in the level,
// with the block's SRAM bank mapped and b = Func_d8c's output
static void obj_block(GB *gb)
{
	uint16_t hl = obj_map_page(gb, REG_HL(gb));
	gb->cpu.l = rd8(gb, hl);
	gb->cpu.h = gb->cpu.a;
	gb->cpu.a = gb->cpu.c;
	wr8(gb, wBlockPtrBank, gb->cpu.a);
	CALL(Func_d8c);
	gb->cpu.a = rd8(gb, wccec);
	sramswitch(gb, gb->cpu.a);
}

// input:
// - e = ?
PORT(Func_ba42)
{
	push_banks(gb);
	gb_push16(gb, REG_DE(gb));
	gb_push16(gb, REG_HL(gb));
	obj_block(gb);
	uint16_t hl = REG_HL(gb);
	if (alu_bit(gb, gb->cpu.b, 0)) {
		gb->cpu.e = alu_swap(gb, gb->cpu.e);
		gb->cpu.a = alu_and(gb, rd8(gb, hl), 0x0f);
	} else {
		gb->cpu.a = alu_and(gb, rd8(gb, hl), 0xf0);
	}
	gb->cpu.a = alu_or(gb, gb->cpu.a, gb->cpu.e);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, gb_pop16(gb));

	push_banks(gb);
	hl = obj_map_page(gb, REG_HL(gb));
	gb->cpu.d = gb->cpu.a;
	gb->cpu.a = rd8(gb, hl--);
	gb->cpu.e = gb->cpu.a;
	SET_HL(gb, hl);
	gb->cpu.a = gb->cpu.c;
	sramswitch(gb, gb->cpu.a);
	gb->cpu.a = alu_or(gb, rd8(gb, REG_DE(gb)), 0x80);
	wr8(gb, REG_DE(gb), gb->cpu.a);
	pop_banks(gb);

	gb->cpu.a = gb->cpu.c;
	wr8(gb, wBlockPtrBank, gb->cpu.a);
	gb->cpu.h = gb->cpu.d;
	gb->cpu.l = gb->cpu.e;
	CALL(Func_d3e);
	SET_DE(gb, gb_pop16(gb));

	gb->cpu.c = gb->cpu.e;
	gb->cpu.d = gb->cpu.h;
	gb->cpu.e = gb->cpu.l;
	FARCALL(SpawnObject);
	pop_banks(gb);
}

PORT(Func_bb2d)
{
	push_banks(gb);
	obj_block(gb);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), alu_bit(gb, gb->cpu.b, 0) ? 0x0f : 0xf0);
	wr8(gb, hl, gb->cpu.a);
	pop_banks(gb);
}

// (dest + [size_var]) <- the 4 bytes at hl
static void copy_block_entry(GB *gb, uint16_t hl, uint16_t queue)
{
	gb->cpu.e = rd8(gb, wBGMapTileQueueSize);
	gb->cpu.d = (uint8_t)(queue >> 8);
	gb->cpu.a = alu_add(gb, (uint8_t)queue, gb->cpu.e);
	gb->cpu.e = gb->cpu.a;
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, i < 3 ? hl++ : hl);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		if (i < 3)
			gb->cpu.e = alu_inc(gb, gb->cpu.e);
	}
	SET_HL(gb, hl);
}

// [de] = a / inc e
static void put_e(GB *gb, uint8_t v)
{
	gb->cpu.a = v;
	wr8(gb, REG_DE(gb), v);
	gb->cpu.e = alu_inc(gb, gb->cpu.e);
}

// the next tile right of l, in the same BG map row
static uint8_t next_tile_l(GB *gb)
{
	gb->cpu.a = alu_and(gb, gb->cpu.l, 0xe0);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_inc(gb, gb->cpu.l);
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x1f);
	return alu_add(gb, gb->cpu.a, gb->cpu.c);
}

// Queues the tiles, attributes and BG map addresses of the next changed
// block in wc18e.
PORT(Func_bb85)
{
	gb->cpu.a = rd8(gb, wIsDMATransferPending);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return; // exit if DMA pending

	gb->cpu.b = BANK_sLevelBlockObjectMap; // BANK("SRAM1")
	gb->cpu.a = rd8(gb, wc19f);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	uint16_t hl = alu_add16(gb, wc18e, REG_DE(gb));
	gb->cpu.a = rd8(gb, hl++);
	SET_HL(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return; // nothing to do

	gb->cpu.l = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0xc0);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.b = alu_inc(gb, gb->cpu.b); // BANK("SRAM2")
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x20);
		alu_cp(gb, gb->cpu.a, 0xc0);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.b = alu_inc(gb, gb->cpu.b); // BANK("SRAM3")
			gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x20);
		}
	}
	gb->cpu.h = gb->cpu.a;
	gb->cpu.a = gb->cpu.b;
	wr8(gb, wBlockPtrBank, gb->cpu.a);
	sramswitch(gb, gb->cpu.a);
	gb_push16(gb, REG_HL(gb));
	CALL(GetBlockPtrBGMapAddress);
	gb->cpu.a = rd8(gb, wBGMapAddressQueueSize);
	gb->cpu.b = gb->cpu.a;
	SET_DE(gb, wBGMapAddressQueue);
	gb->cpu.a = alu_add(gb, gb->cpu.e, gb->cpu.b);
	gb->cpu.e = gb->cpu.a;

	// each block is 2x2 tiles: top-left, top-right, bottom-left, bottom-right
	put_e(gb, gb->cpu.h);
	put_e(gb, gb->cpu.l);
	put_e(gb, gb->cpu.h);
	put_e(gb, next_tile_l(gb));
	SET_BC(gb, TILEMAP_WIDTH);
	hl = alu_add16(gb, REG_HL(gb), REG_BC(gb));
	SET_HL(gb, hl);
	gb->cpu.a = alu_and(gb, gb->cpu.h, 0xfb);
	gb->cpu.h = gb->cpu.a;
	put_e(gb, gb->cpu.a);
	put_e(gb, gb->cpu.l);
	put_e(gb, gb->cpu.h);
	gb->cpu.a = next_tile_l(gb);
	wr8(gb, REG_DE(gb), gb->cpu.a);

	gb->cpu.a = alu_add(gb, rd8(gb, wBGMapAddressQueueSize), 2 * 4);
	wr8(gb, wBGMapAddressQueueSize, gb->cpu.a);
	hl = gb_pop16(gb);

	gb->cpu.a = rd8(gb, hl);
	gb->cpu.d = 0x00;
	gb->cpu.e = gb->cpu.a;
	gb->cpu.e = alu_sla(gb, gb->cpu.e);
	gb->cpu.e = alu_sla(gb, gb->cpu.e);
	gb->cpu.d = alu_rl(gb, gb->cpu.d); // *4
	// push_wram $03
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 0x03;
	wr8(gb, rWBK, 0x03);
	uint16_t de = REG_DE(gb);
	gb_push16(gb, de);
	copy_block_entry(gb, alu_add16(gb, wRoomBlockTiles, de), wBGMapTileVRAM0Queue);
	de = gb_pop16(gb);
	SET_DE(gb, de);
	copy_block_entry(gb, alu_add16(gb, wRoomBlockAttributes, de), wBGMapTileVRAM1Queue);

	gb->cpu.a = alu_add(gb, rd8(gb, wBGMapTileQueueSize), 4);
	wr8(gb, wBGMapTileQueueSize, gb->cpu.a);

	gb->cpu.a = rd8(gb, wc19f);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	hl = alu_add16(gb, wc18e, REG_DE(gb));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
	gb->cpu.a = alu_add(gb, gb->cpu.e, 2);
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	wr8(gb, wc19f, gb->cpu.a);
	pop_af(gb); // pop_wram
	wr8(gb, rWBK, gb->cpu.a);
}
