// The level map's objects and changed blocks (src/engine/level/
// block_queue.asm): despawning an object back into the object map,
// changing an object's block in it (and spawning there), and queueing the
// tiles of the next block that changed.
//
// Plain C++ over memory; the hooks leave the registers their callers read
// (DespawnObject's, for DespawnAllObjects; the rest have plain callers).
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"
#include "object.hpp"
#include "../home/object_pos.hpp"
#include "../home/level_map.hpp"

#include <initializer_list>

namespace wl3::level {

namespace {

constexpr uint8_t SRAM1 = BANK_sLevelBlockObjectMap;
constexpr uint8_t GFX_RAM = 0x03; // BANK("GFX RAM")

// an object map entry: its SRAM bank and pointer
struct MapLoc {
	uint8_t bank;
	uint16_t ptr;
};

struct Banks;

class BlockQueue {
public:
	explicit BlockQueue(GB *gb) : gb_(gb), cpu_(gb->cpu), mem_(gb), routines_(gb) {}

	MapLoc despawn(uint16_t at); // DespawnObject
	void set_block(uint16_t at, uint8_t nybble); // Func_ba42
	void clear_block(uint16_t at); // Func_bb2d
	bool queue_changed_block(); // Func_bb85: false if a DMA's pending

private:
	void sram_switch(uint8_t bank)
	{
		mem_[wSRAMBank] = bank;
		mem_[rRAMB + 0x100] = bank;
	}
	Banks push_banks();
	void pop_banks(Banks banks);
	MapLoc map_loc(uint16_t at);
	home::ObjMapBlock obj_block(uint16_t at, MapLoc &loc);
	void copy_block_entry(uint16_t entry, uint16_t queue);

	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
};

// the WRAM and SRAM banks to put back after (push_wram / push af)
struct Banks {
	uint8_t wram, sram;
};

// push_wram BANK("Level Objects WRAM") / ld a, [wSRAMBank] / push af /
// ld a, $01 / sramswitch
Banks BlockQueue::push_banks()
{
	Banks banks{mem_[rWBK], mem_[wSRAMBank]};
	mem_[rWBK] = BANK_wObjects;
	sram_switch(SRAM1);
	return banks;
}

// pop af / sramswitch / pop_wram
void BlockQueue::pop_banks(Banks banks)
{
	sram_switch(banks.sram);
	mem_[rWBK] = banks.wram;
}

// the object map entry an object's pointer (its high byte at `at`, the low
// one before) names: the rows past SRAM's end are the next bank's
MapLoc BlockQueue::map_loc(uint16_t at)
{
	uint8_t bank = 1, hi = mem_[at];
	for (int i = 0; i < 2 && hi >= 0xc0; i++) { // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
		bank++;
		hi = static_cast<uint8_t>(hi - 0x20);
	}
	return {bank, static_cast<uint16_t>(hi << 8 | mem_[static_cast<uint16_t>(at - 1)])};
}

// the object's block in the level map (by its object map entry), with the
// block's SRAM bank mapped
home::ObjMapBlock BlockQueue::obj_block(uint16_t at, MapLoc &loc)
{
	loc = map_loc(at);
	mem_[wBlockPtrBank] = loc.bank;
	home::ObjMapBlock block = home::obj_map_col(gb_, static_cast<uint8_t>(loc.ptr >> 8), static_cast<uint8_t>(loc.ptr));
	sram_switch(mem_[wccec]);
	return block;
}

// at = the object's OBJ_UNK_02: marks its spawn point in the object map,
// so it can spawn again, and clears OBJ_FLAGS
MapLoc BlockQueue::despawn(uint16_t at)
{
	Banks banks = push_banks();
	MapLoc loc = map_loc(at);
	mem_[static_cast<uint16_t>(at - 2)] = 0; // OBJ_FLAGS
	sram_switch(loc.bank);
	mem_[loc.ptr] = mem_[loc.ptr] | 0x80;
	pop_banks(banks);
	return loc;
}

// at = the object's OBJ_UNK_02, nybble = an object index: puts it in the
// object's block in the level map, marks its spawn point and spawns it
// there
void BlockQueue::set_block(uint16_t at, uint8_t nybble)
{
	Banks outer = push_banks();
	MapLoc loc;
	home::ObjMapBlock block = obj_block(at, loc);
	uint16_t addr = static_cast<uint16_t>(block.row << 8 | block.col);
	if (block.parity & 1)
		mem_[addr] = static_cast<uint8_t>((mem_[addr] & 0x0f) | static_cast<uint8_t>(nybble << 4 | nybble >> 4));
	else
		mem_[addr] = static_cast<uint8_t>((mem_[addr] & 0xf0) | nybble);

	Banks inner = push_banks();
	loc = map_loc(at);
	sram_switch(loc.bank);
	mem_[loc.ptr] = mem_[loc.ptr] | 0x80;
	pop_banks(inner);

	mem_[wBlockPtrBank] = loc.bank;
	uint8_t row = static_cast<uint8_t>(loc.ptr >> 8), col = static_cast<uint8_t>(loc.ptr);
	home::level_block_to_hpos(gb_, row, col);
	routines_.in_bank(BANK_SpawnObject, ADDR_SpawnObject, [&] { spawn_object(gb_, nybble, loc.ptr); });
	pop_banks(outer);
}

// at = the object's OBJ_UNK_02: clears its nybble in its block
void BlockQueue::clear_block(uint16_t at)
{
	Banks banks = push_banks();
	MapLoc loc;
	home::ObjMapBlock block = obj_block(at, loc);
	uint16_t addr = static_cast<uint16_t>(block.row << 8 | block.col);
	mem_[addr] = mem_[addr] & ((block.parity & 1) ? 0x0f : 0xf0);
	pop_banks(banks);
}

// the next tile right of l, in the same BG map row
static uint8_t next_tile(uint8_t l)
{
	return static_cast<uint8_t>((l & 0xe0) + (static_cast<uint8_t>(l + 1) & 0x1f));
}

// the 4 bytes at entry to the queue (its index wrapping in its page)
void BlockQueue::copy_block_entry(uint16_t entry, uint16_t queue)
{
	uint8_t at = static_cast<uint8_t>(queue + mem_[wBGMapTileQueueSize]);
	for (int i = 0; i < 4; i++)
		mem_[static_cast<uint16_t>((queue & 0xff00) | at++)] = mem_[entry++];
}

// queues the tiles, attributes and BG map addresses of the next changed
// block in wc18e
bool BlockQueue::queue_changed_block()
{
	if (mem_[wIsDMATransferPending])
		return false; // a DMA's pending
	uint16_t entry = static_cast<uint16_t>(wc18e + mem_[wc19f]);
	if (!mem_[entry])
		return true; // nothing to do
	MapLoc loc = map_loc(static_cast<uint16_t>(entry));
	// (map_loc reads the low byte before; the queue holds high, low)
	loc.ptr = static_cast<uint16_t>((loc.ptr & 0xff00) | mem_[static_cast<uint16_t>(entry + 1)]);
	mem_[wBlockPtrBank] = loc.bank;
	sram_switch(loc.bank);
	uint16_t bg = home::block_bg_map_address(gb_, loc.ptr);

	// each block is 2x2 tiles: top-left, top-right, bottom-left, bottom-right
	uint8_t at = static_cast<uint8_t>(wBGMapAddressQueue + mem_[wBGMapAddressQueueSize]);
	uint16_t page = wBGMapAddressQueue & 0xff00;
	uint16_t below = static_cast<uint16_t>((bg + TILEMAP_WIDTH) & 0xfbff); // wraps within the BG map
	for (uint16_t row : {bg, below}) {
		mem_[static_cast<uint16_t>(page | at++)] = static_cast<uint8_t>(row >> 8);
		mem_[static_cast<uint16_t>(page | at++)] = static_cast<uint8_t>(row);
		mem_[static_cast<uint16_t>(page | at++)] = static_cast<uint8_t>(row >> 8);
		mem_[static_cast<uint16_t>(page | at++)] = next_tile(static_cast<uint8_t>(row));
	}
	mem_[wBGMapAddressQueueSize] = static_cast<uint8_t>(mem_[wBGMapAddressQueueSize] + 2 * 4);

	// sla e / sla e / rl d: block * 4, losing bit 7
	uint16_t offset = static_cast<uint16_t>((mem_[loc.ptr] * 4) & 0x1ff);
	uint8_t wram = mem_[rWBK]; // push_wram
	mem_[rWBK] = GFX_RAM;
	copy_block_entry(static_cast<uint16_t>(wRoomBlockTiles + offset), wBGMapTileVRAM0Queue);
	copy_block_entry(static_cast<uint16_t>(wRoomBlockAttributes + offset), wBGMapTileVRAM1Queue);
	mem_[wBGMapTileQueueSize] = static_cast<uint8_t>(mem_[wBGMapTileQueueSize] + 4);

	uint8_t done = mem_[wc19f];
	mem_[static_cast<uint16_t>(wc18e + done)] = 0;
	mem_[static_cast<uint16_t>(wc18e + done + 1)] = 0;
	mem_[wc19f] = static_cast<uint8_t>((done + 2) & 0x0f);
	mem_[rWBK] = wram; // pop_wram
	return true;
}

} // namespace

} // namespace wl3::level

using wl3::level::BlockQueue;

// DespawnObject: hl = the object's OBJ_UNK_02. It leaves what the asm does
// for DespawnAllObjects (which goes on along the objects' page in h): hl =
// the object's OBJ_FLAGS, de the object map entry, c its bank; a and f
// as push_wram found them.
PORT(DespawnObject)
{
	uint8_t a = wl3::Memory(gb)[rWBK], f = gb->cpu.f;
	uint16_t at = REG_HL(gb);
	wl3::level::MapLoc loc = BlockQueue(gb).despawn(at);
	SET_HL(gb, at - 2);
	SET_DE(gb, loc.ptr);
	gb->cpu.c = loc.bank;
	gb->cpu.a = a;
	gb->cpu.f = f;
}
// The objects call these (hl = OBJ_UNK_02, e = the index for Func_ba42)
// and read no register after.
PORT_EX(Func_ba42, 0, ALL_REGISTERS) { BlockQueue(gb).set_block(REG_HL(gb), gb->cpu.e); }
PORT_EX(Func_bb2d, 0, ALL_REGISTERS) { BlockQueue(gb).clear_block(REG_HL(gb)); }
// ProcessMultiBlock's loop calls this, and the b it leaves goes on through
// ProcessBlock to the multi-block functions that jump straight to the
// replace (Func_18f32 and the like), which write it to the level map as
// the new block: SRAM1 when there was nothing to queue, 0 after queueing
// one (the asm's ld bc, TILEMAP_WIDTH), the caller's while a DMA's pending.
PORT_EX(Func_bb85, 0, ALL_REGISTERS & ~R_B)
{
	uint16_t entry = static_cast<uint16_t>(wc18e + wl3::Memory(gb)[wc19f]);
	bool nothing = !wl3::Memory(gb)[entry];
	if (BlockQueue(gb).queue_changed_block())
		gb->cpu.b = nothing ? wl3::level::SRAM1 : 0;
}
