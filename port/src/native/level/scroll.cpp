// Drawing what scrolls into view (src/engine/level/scroll.asm): the row or
// column of blocks at the edge the camera moved past goes into the BG map
// queues (addresses, tiles and attributes), and the objects there spawn.
// And the room ID at Wario's position.
//
// Plain C++ over memory. The callers are C and read no registers after,
// so the hooks clobber them all, except GetNextLevelRoomID (exact).
// QueueBlockAttributesAlongRow_OddColumn's first half block reads its tile
// from past the tile queue, which can reach the stack in the asm. The port
// doesn't keep that stack, so that byte can differ (see ASM_BUGS.md).
#include "room_gfx.hpp"
#include "frame.hpp"
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"
#include "../home/level_map.hpp"

namespace wl3::level {

namespace {

constexpr uint8_t GFX_RAM = 0x03; // BANK("GFX RAM")

// what a queue step copies
enum class Step { FULL, FIRST, LAST_FULL, LAST_HALF };

class EdgeQueue {
public:
	explicit EdgeQueue(GB *gb) : gb_(gb), mem_(gb), routines_(gb) {}

	void update(); // Func_8ed9
	void queue_column_at_block(); // Func_a0e2
	// Func_a2aa, Func_a79e, Func_aca6, Func_b182
	void queue_column(uint16_t map, uint16_t queue, uint16_t ptr_var, uint16_t base, int offset, bool wram);
	// QueueBlockTilesAlongRow_OddColumn, ... (row_block_queue.asm)
	void queue_block_row(uint16_t map, uint16_t queue, uint16_t ptr_var, uint16_t base, int offset,
		bool wram, bool quirk);

private:
	void sram_switch(uint8_t bank)
	{
		mem_.sram_bank() = bank;
		mem_[rRAMB + 0x100] = bank;
	}
	uint16_t be(uint16_t var) { return static_cast<uint16_t>(mem_[var] << 8 | mem_[var + 1]); }
	void set_be(uint16_t var, uint16_t v)
	{
		mem_[var + 0] = static_cast<uint8_t>(v >> 8);
		mem_[var + 1] = static_cast<uint8_t>(v);
	}
	// the camera's position at a whole tile
	uint16_t tile_pos(uint16_t cam) { return static_cast<uint16_t>(be(cam) & 0xfff8); }
	uint16_t step(uint16_t map, uint16_t base, uint16_t ptr_var, int incs, Step kind, bool row);
	void queue_row(uint16_t map, uint16_t queue, uint16_t ptr_var, uint16_t base, bool bottom, bool wram);
	void start_queue(uint16_t queue, uint16_t ptr_var);
	uint16_t column_y();
	uint16_t left_x();
	void queue_edge_row(); // .Func_9085

	GB *gb_;
	Memory mem_;
	Routines routines_;
};

// One block of a column (or row) at map in the level's block map: copies
// bytes of its 4-byte entry in base (skipping incs) to the queue at
// [ptr_var]. Returns the next block's pointer: a column's moves down a row
// (into the next SRAM bank at the end of one), a row's right.
uint16_t EdgeQueue::step(uint16_t map, uint16_t base, uint16_t ptr_var, int incs, Step kind, bool row)
{
	uint8_t block = mem_[map];
	uint16_t next = row ? static_cast<uint16_t>(map + 1) : map;
	// ld d, 0 / ld e, a / sla e / sla e / rl d: block * 4, but only the
	// second sla's carry reaches d (bit 7 is lost)
	uint16_t entry = static_cast<uint16_t>(base + ((block * 4) & 0x1ff));
	entry = static_cast<uint16_t>((entry & 0xff00) | static_cast<uint8_t>(entry + incs)); // inc l
	uint8_t hi = mem_[ptr_var + 0];
	uint8_t lo = mem_[ptr_var + 1];
	if (kind == Step::FULL)
		mem_[ptr_var + 1] = static_cast<uint8_t>(lo + 2);
	else if (kind == Step::FIRST)
		mem_[ptr_var + 1] = static_cast<uint8_t>(lo + 1);
	auto queue_at = [&](uint8_t e) { return static_cast<uint16_t>(hi << 8 | e); };
	if (kind == Step::FIRST || kind == Step::LAST_HALF) {
		mem_[queue_at(lo)] = mem_[entry];
	} else {
		mem_[queue_at(lo)] = mem_[entry++]; // ld a, [hli]
		if (!row) // the tile below: inc l
			entry = static_cast<uint16_t>((entry & 0xff00) | static_cast<uint8_t>(entry + 1));
		mem_[queue_at(static_cast<uint8_t>(lo + 1))] = mem_[entry]; // inc e
	}
	if (row || kind == Step::LAST_FULL || kind == Step::LAST_HALF)
		return next;
	next = static_cast<uint16_t>(next + 0x100); // inc h
	if (next >> 8 == 0xc0) { // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
		next = static_cast<uint16_t>((sLevelBlockObjectMap & 0xff00) | (next & 0xff));
		sram_switch(static_cast<uint8_t>(mem_.sram_bank() + 1));
	}
	return next;
}

// the queue pointer at ptr_var = the queue + wBGMapTileQueueSize
void EdgeQueue::start_queue(uint16_t queue, uint16_t ptr_var)
{
	mem_[ptr_var + 0] = static_cast<uint8_t>(queue >> 8);
	mem_[ptr_var + 1] = static_cast<uint8_t>(queue + mem_.bg_map_tile_queue_size());
}

// the 12 blocks' tiles (or attributes, in GFX RAM) of a column of the level
// map at map: their left (offset 0) or right (1) tiles, from halfway down
// the first block if wBlockYPos is
void EdgeQueue::queue_column(uint16_t map, uint16_t queue, uint16_t ptr_var, uint16_t base, int offset, bool wram)
{
	start_queue(queue, ptr_var);
	bool half = mem_.block_y().lo() & 0x08;
	uint8_t wbk = 0;
	if (wram) {
		wbk = mem_[rWBK];
		mem_[rWBK] = GFX_RAM;
	}
	if (!half) {
		for (int i = 0; i < 11; i++)
			map = step(map, base, ptr_var, offset, Step::FULL, false);
		step(map, base, ptr_var, offset, Step::LAST_HALF, false);
	} else {
		map = step(map, base, ptr_var, offset + 2, Step::FIRST, false);
		for (int i = 0; i < 10; i++)
			map = step(map, base, ptr_var, offset, Step::FULL, false);
		step(map, base, ptr_var, offset, Step::LAST_FULL, false);
	}
	if (wram)
		mem_[rWBK] = wbk;
}

// 16 blocks' tiles (or attributes, in GFX RAM) down the level map from map,
// for the room's first drawing: their left (offset 0) or right (1) tiles,
// from halfway into the first block if wBlockYPos is.
// quirk (QueueBlockAttributesAlongRow_OddColumn): the half first block's
// tile is read from the queue's address instead of the attributes, and a
// whole first block gets an extra half step at the end
void EdgeQueue::queue_block_row(uint16_t map, uint16_t queue, uint16_t ptr_var, uint16_t base,
	int offset, bool wram, bool quirk)
{
	uint8_t wbk = 0;
	if (wram) {
		wbk = mem_[rWBK];
		mem_[rWBK] = BANK_wRoomBlockAttributes;
	}
	start_queue(queue, ptr_var);
	bool half = mem_.block_y().lo() & 0x08;
	if (half)
		map = step(map, quirk ? wBGMapTileVRAM1Queue : base, ptr_var, offset + 2, Step::FIRST, false);
	for (int i = 0; i < 0x10; i++)
		map = step(map, base, ptr_var, offset, Step::FULL, false);
	if (quirk && !half)
		step(map, base, ptr_var, offset, Step::LAST_HALF, false);
	if (wram) {
		mem_[rWBK] = wbk;
	}
}

// a column of 23 BG map addresses at the block's, and the tiles and
// attributes to draw there
void EdgeQueue::queue_column_at_block()
{
	uint16_t bg = level::block_bg_ptr(gb_);
	uint8_t at = static_cast<uint8_t>(wBGMapAddressQueue + mem_.bg_map_address_queue_size());
	for (int i = 0; i < 23; i++) {
		mem_[static_cast<uint16_t>((wBGMapAddressQueue & 0xff00) | at++)] = static_cast<uint8_t>(bg >> 8);
		mem_[static_cast<uint16_t>((wBGMapAddressQueue & 0xff00) | at++)] = static_cast<uint8_t>(bg);
		bg = static_cast<uint16_t>((bg + TILEMAP_WIDTH) & 0xfbff); // wraps within the BG map
	}
	mem_.bg_map_address_queue_size() = static_cast<uint8_t>(mem_.bg_map_address_queue_size() + 2 * 23);

	uint16_t map = home::block_ptr(gb_, wBlockPos).ptr;
	sram_switch(mem_.block_ptr_bank());
	int offset = (mem_.block_x().lo() & 0x08) ? 1 : 0;
	queue_column(map, wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, offset, false); // Func_a2aa / Func_aca6
	sram_switch(mem_.block_ptr_bank());
	queue_column(map, wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, offset, true); // Func_a79e / Func_b182
	mem_.bg_map_tile_queue_size() = static_cast<uint8_t>(mem_.bg_map_tile_queue_size() + 23);
}

// the 13 blocks' tiles (or attributes, in GFX RAM) of a row of the level
// map at map: their top or bottom tiles, from halfway into the first block
// if wBlockXPos is
void EdgeQueue::queue_row(uint16_t map, uint16_t queue, uint16_t ptr_var, uint16_t base, bool bottom, bool wram)
{
	start_queue(queue, ptr_var);
	bool half = mem_.block_x().lo() & 0x08;
	uint8_t wbk = 0;
	if (wram) {
		wbk = mem_[rWBK];
		mem_[rWBK] = GFX_RAM;
	}
	int offset = bottom ? 2 : 0;
	if (!half) {
		for (int i = 0; i < 12; i++)
			map = step(map, base, ptr_var, offset, Step::FULL, true);
		step(map, base, ptr_var, offset, Step::LAST_HALF, true);
	} else {
		map = step(map, base, ptr_var, offset + 1, Step::FIRST, true);
		for (int i = 0; i < (bottom ? 11 : 12); i++)
			map = step(map, base, ptr_var, offset, Step::FULL, true);
		if (bottom)
			step(map, base, ptr_var, offset, Step::LAST_FULL, true);
	}
	if (wram)
		mem_[rWBK] = wbk;
}

// a column's y at the right or left edge: the camera's - $10, at least 0,
// and $240 where + $b8 would reach $300
uint16_t EdgeQueue::column_y()
{
	uint16_t y = static_cast<uint16_t>(tile_pos(wCameraSCY) - 0x10);
	if (y & 0x8000)
		return 0;
	if (static_cast<uint8_t>((y + 0xb8) >> 8) >= 0x03)
		return 0x240;
	return y;
}

// the camera's x - $10, at least 0
uint16_t EdgeQueue::left_x()
{
	uint16_t x = static_cast<uint16_t>(tile_pos(wCameraSCX) - 0x10);
	return (x & 0x8000) ? 0 : x;
}

// a row of 25 BG map addresses at the block's, and the tiles and attributes
// to draw there
void EdgeQueue::queue_edge_row()
{
	uint16_t bg = level::block_bg_ptr(gb_);
	uint8_t at = static_cast<uint8_t>(wBGMapAddressQueue + mem_.bg_map_address_queue_size());
	for (int i = 0; i < 25; i++) {
		mem_[static_cast<uint16_t>((wBGMapAddressQueue & 0xff00) | at++)] = static_cast<uint8_t>(bg >> 8);
		mem_[static_cast<uint16_t>((wBGMapAddressQueue & 0xff00) | at++)] = static_cast<uint8_t>(bg);
		uint8_t l = static_cast<uint8_t>(bg);
		l = static_cast<uint8_t>((l & 0xe0) + ((l + 1) & 0x1f)); // wraps within the BG map row
		bg = static_cast<uint16_t>((bg & 0xff00) | l);
	}
	mem_.bg_map_address_queue_size() = static_cast<uint8_t>(mem_.bg_map_address_queue_size() + 2 * 25);

	uint16_t map = home::block_ptr(gb_, wBlockPos).ptr;
	sram_switch(mem_.block_ptr_bank());
	bool bottom = mem_.block_y().lo() & 0x08;
	queue_row(map, wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, bottom, false);
	sram_switch(mem_.block_ptr_bank());
	queue_row(map, wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, bottom, true);
	mem_.bg_map_tile_queue_size() = static_cast<uint8_t>(mem_.bg_map_tile_queue_size() + 25);
}

// draws the rows and columns that scrolled into view (wLevelEdgeUpdateFlags)
// and spawns the objects there
void EdgeQueue::update()
{
	uint8_t edges = mem_.level_edge_update_flags();
	if (!edges)
		return;
	if (edges & (LEVEL_EDGE_UPDATE_RIGHT | LEVEL_EDGE_UPDATE_LEFT)) {
		bool right = edges & LEVEL_EDGE_UPDATE_RIGHT;
		set_be(wBlockYPos, column_y());
		if (right) { // .Func_8fec: the column right of the screen
			uint16_t x = static_cast<uint16_t>(tile_pos(wCameraSCX) + 0xb0);
			set_be(wBlockXPos, x >> 8 >= 0x0a ? 0x9f8 : x);
		} else { // .Func_9039: the column left of it
			set_be(wBlockXPos, left_x());
		}
		queue_column_at_block();
		mem_.level_edge_update_flags() &= ~(right ? LEVEL_EDGE_UPDATE_RIGHT : LEVEL_EDGE_UPDATE_LEFT);
		routines_.in_bank(BANK_SpawnRowEdgeObjects, ADDR_SpawnRowEdgeObjects, [&] { spawn_row_edge_objects(gb_); });
		edges = mem_.level_edge_update_flags();
	}
	// .check_vertical_direction
	bool up = edges & LEVEL_EDGE_UPDATE_UP;
	if (!up && !(edges & LEVEL_EDGE_UPDATE_DOWN))
		return;
	if (up) { // .Func_8fb3: the row above the screen
		uint16_t y = static_cast<uint16_t>(tile_pos(wCameraSCY) - 0x10);
		set_be(wBlockYPos, (y & 0x8000) ? 0 : y);
	} else { // .Func_8f79: the row below it
		uint16_t y = static_cast<uint16_t>(tile_pos(wCameraSCY) + 0xa0);
		set_be(wBlockYPos, y >> 8 >= 0x03 ? 0x2f8 : y);
	}
	set_be(wBlockXPos, left_x());
	queue_edge_row();
	mem_.level_edge_update_flags() &= ~(up ? LEVEL_EDGE_UPDATE_UP : LEVEL_EDGE_UPDATE_DOWN);
	routines_.in_bank(BANK_SpawnColEdgeObjects, ADDR_SpawnColEdgeObjects, [&] { spawn_col_edge_objects(gb_); });
}

} // namespace

} // namespace wl3::level

using wl3::level::EdgeQueue;
using wl3::Memory;
namespace flags = wl3::flags;

// GetNextLevelRoomID: hl = Wario's position; wLevelRoomID = his x high
// byte + 10 * y high byte. Exact: a = the ID, hl = his x, and the loop's
// b = 0, c = 10 and f (its last dec b's), or and's f for y 0
PORT(GetNextLevelRoomID)
{
	CPU &cpu = gb->cpu;
	Memory mem(gb);
	uint16_t at = REG_HL(gb);
	uint8_t y_hi = mem[at];
	uint8_t x_hi = mem[at + 2];
	uint8_t id = static_cast<uint8_t>(x_hi + 10 * y_hi);
	SET_HL(gb, at + 2);
	if (!y_hi) {
		cpu.f = flags::and_(0);
	} else {
		cpu.f = flags::dec(flags::add(static_cast<uint8_t>(id - 10), 10), 1);
		cpu.b = 0;
		cpu.c = 10;
	}
	cpu.a = id;
	mem.level_room_id() = id;
}
PORT_EX(Func_8ed9, 0, ALL_REGISTERS) { wl3::level::queue_scrolled_edge(gb); }
PORT_EX(Func_a0e2, 0, ALL_REGISTERS) { EdgeQueue(gb).queue_column_at_block(); }
PORT_EX(Func_a2aa, 0, ALL_REGISTERS)
{
	EdgeQueue(gb).queue_column(REG_HL(gb), wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, 1, false);
}
PORT_EX(Func_a79e, 0, ALL_REGISTERS)
{
	EdgeQueue(gb).queue_column(REG_HL(gb), wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, 1, true);
}
PORT_EX(Func_aca6, 0, ALL_REGISTERS)
{
	EdgeQueue(gb).queue_column(REG_HL(gb), wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, 0, false);
}
PORT_EX(Func_b182, 0, ALL_REGISTERS)
{
	EdgeQueue(gb).queue_column(REG_HL(gb), wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, 0, true);
}
PORT_EX(QueueBlockTilesAlongRow_OddColumn, 0, ALL_REGISTERS)
{
	EdgeQueue(gb).queue_block_row(REG_HL(gb), wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, 1, false, false);
}
PORT_EX(QueueBlockAttributesAlongRow_OddColumn, 0, ALL_REGISTERS)
{
	EdgeQueue(gb).queue_block_row(REG_HL(gb), wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, 1, true, true);
}
PORT_EX(QueueBlockTilesAlongRow_EvenColumn, 0, ALL_REGISTERS)
{
	EdgeQueue(gb).queue_block_row(REG_HL(gb), wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, 0, false, false);
}
PORT_EX(QueueBlockAttributesAlongRow_EvenColumn, 0, ALL_REGISTERS)
{
	EdgeQueue(gb).queue_block_row(REG_HL(gb), wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, 0, true, false);
}

namespace wl3::level {
void queue_scrolled_edge(GB *gb) { EdgeQueue(gb).update(); }
// QueueBlockTilesAlongRow_* / QueueBlockAttributesAlongRow_*: the row of
// blocks from `block`, starting on an odd or even column
void queue_block_tiles_along_row(GB *gb, uint16_t block, bool odd)
{
	EdgeQueue(gb).queue_block_row(block, wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, odd ? 1 : 0, false, false);
}
void queue_block_attributes_along_row(GB *gb, uint16_t block, bool odd)
{
	EdgeQueue(gb).queue_block_row(block, wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, odd ? 1 : 0, true, odd);
}
} // namespace wl3::level
