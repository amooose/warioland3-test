// The level map's objects and changed blocks (src/engine/level/
// block_queue.asm): despawning an object back into the object map,
// changing an object's block in it (and spawning there), and queueing the
// tiles of the next block that changed.
//
// Objects and C call these, and the asm's push af / pop af restore a and f
// at the end, so they keep the original's registers, flags and pushes.
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"

namespace wl3::level {

namespace {

constexpr uint8_t SRAM1 = BANK_sLevelBlockObjectMap;
constexpr uint8_t GFX_RAM = 0x03; // BANK("GFX RAM")

class BlockQueue {
public:
	explicit BlockQueue(GB *gb) : gb_(gb), cpu_(gb->cpu), mem_(gb), routines_(gb) {}

	void despawn(); // DespawnObject
	void set_block(); // Func_ba42
	void clear_block(); // Func_bb2d
	void queue_changed_block(); // Func_bb85

private:
	void sram_switch(uint8_t bank)
	{
		mem_[wSRAMBank] = bank;
		mem_[rRAMB + 0x100] = bank;
	}
	void push_af() { gb_push16(gb_, REG_AF(gb_)); }
	void pop_af()
	{
		uint16_t af = gb_pop16(gb_);
		cpu_.a = static_cast<uint8_t>(af >> 8);
		cpu_.f = static_cast<uint8_t>(af & 0xf0);
	}
	void push_banks();
	void pop_banks();
	uint16_t obj_map_page(uint16_t at);
	void obj_block();
	void put_e(uint8_t v);
	uint8_t next_tile_l();
	void copy_block_entry(uint16_t entry, uint16_t queue);

	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
};

// push_wram BANK("Level Objects WRAM") / ld a, [wSRAMBank] / push af /
// ld a, $01 / sramswitch
void BlockQueue::push_banks()
{
	cpu_.a = mem_[rWBK];
	push_af();
	cpu_.a = BANK_wObjects;
	mem_[rWBK] = BANK_wObjects;
	cpu_.a = mem_[wSRAMBank];
	push_af();
	cpu_.a = SRAM1;
	sram_switch(SRAM1);
}

// pop af / sramswitch / pop_wram
void BlockQueue::pop_banks()
{
	pop_af();
	sram_switch(cpu_.a);
	pop_af();
	mem_[rWBK] = cpu_.a;
}

// c = 1 / ld a, [hld] / the object map's SRAM bank (c, from 1) and page
// (a) of the high byte at `at`; returns hl
uint16_t BlockQueue::obj_map_page(uint16_t at)
{
	cpu_.c = 1;
	cpu_.a = mem_[at--];
	cpu_.f = flags::cp(cpu_.a, 0xc0); // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
	for (int i = 0; i < 2 && (cpu_.f & FLAG_C) == 0; i++) {
		cpu_.f = flags::inc(cpu_.f, cpu_.c);
		cpu_.c++;
		cpu_.f = flags::sub(cpu_.a, 0x20);
		cpu_.a = static_cast<uint8_t>(cpu_.a - 0x20);
		if (i == 0)
			cpu_.f = flags::cp(cpu_.a, 0xc0);
	}
	return at;
}

// hl = the object map pointer's high byte: hl = its block in the level,
// with the block's SRAM bank mapped and b = Func_d8c's output
void BlockQueue::obj_block()
{
	uint16_t at = obj_map_page(REG_HL(gb_));
	cpu_.l = mem_[at];
	cpu_.h = cpu_.a;
	cpu_.a = cpu_.c;
	mem_[wBlockPtrBank] = cpu_.c;
	routines_.call(ADDR_Func_d8c);
	cpu_.a = mem_[wccec];
	sram_switch(cpu_.a);
}

// hl = the object's OBJ_UNK_02: marks its spawn point in the object map,
// so it can spawn again, and clears OBJ_FLAGS
void BlockQueue::despawn()
{
	cpu_.a = mem_[rWBK];
	push_af();
	mem_[rWBK] = BANK_wObjects;
	cpu_.a = mem_[wSRAMBank];
	push_af();
	cpu_.a = SRAM1;
	sram_switch(SRAM1);

	uint16_t at = obj_map_page(REG_HL(gb_));
	cpu_.d = cpu_.a;
	cpu_.e = mem_[at--]; // OBJ_UNK_01
	mem_[at] = 0; // OBJ_FLAGS
	SET_HL(gb_, at);
	cpu_.a = cpu_.c;
	sram_switch(cpu_.c);
	cpu_.a = mem_[REG_DE(gb_)] | 0x80;
	cpu_.f = flags::or_(cpu_.a);
	mem_[REG_DE(gb_)] = cpu_.a;
	pop_banks();
}

// hl = the object's OBJ_UNK_02, e = a nybble: puts it in the object's block
// in the level map, marks its spawn point and spawns it there
void BlockQueue::set_block()
{
	push_banks();
	gb_push16(gb_, REG_DE(gb_));
	gb_push16(gb_, REG_HL(gb_));
	obj_block();
	uint16_t block = REG_HL(gb_);
	cpu_.f = flags::bit(cpu_.f, cpu_.b, 0);
	if (cpu_.b & 1) {
		cpu_.e = static_cast<uint8_t>(cpu_.e << 4 | cpu_.e >> 4); // swap e
		cpu_.a = mem_[block] & 0x0f;
	} else {
		cpu_.a = mem_[block] & 0xf0;
	}
	cpu_.a |= cpu_.e;
	cpu_.f = flags::or_(cpu_.a);
	mem_[block] = cpu_.a;
	SET_HL(gb_, gb_pop16(gb_));

	push_banks();
	uint16_t at = obj_map_page(REG_HL(gb_));
	cpu_.d = cpu_.a;
	cpu_.a = mem_[at--];
	cpu_.e = cpu_.a;
	SET_HL(gb_, at);
	cpu_.a = cpu_.c;
	sram_switch(cpu_.c);
	cpu_.a = mem_[REG_DE(gb_)] | 0x80;
	cpu_.f = flags::or_(cpu_.a);
	mem_[REG_DE(gb_)] = cpu_.a;
	pop_banks();

	cpu_.a = cpu_.c;
	mem_[wBlockPtrBank] = cpu_.c;
	cpu_.h = cpu_.d;
	cpu_.l = cpu_.e;
	routines_.call(ADDR_Func_d3e);
	SET_DE(gb_, gb_pop16(gb_));
	cpu_.c = cpu_.e;
	cpu_.d = cpu_.h;
	cpu_.e = cpu_.l;
	routines_.farcall(BANK_SpawnObject, ADDR_SpawnObject);
	pop_banks();
}

// hl = the object's OBJ_UNK_02: clears its nybble in its block
void BlockQueue::clear_block()
{
	push_banks();
	obj_block();
	uint16_t block = REG_HL(gb_);
	cpu_.f = flags::bit(cpu_.f, cpu_.b, 0);
	cpu_.a = mem_[block] & ((cpu_.b & 1) ? 0x0f : 0xf0);
	cpu_.f = flags::and_(cpu_.a);
	mem_[block] = cpu_.a;
	pop_banks();
}

// [de] = v / inc e
void BlockQueue::put_e(uint8_t v)
{
	cpu_.a = v;
	mem_[REG_DE(gb_)] = v;
	cpu_.f = flags::inc(cpu_.f, cpu_.e);
	cpu_.e++;
}

// the next tile right of l, in the same BG map row
uint8_t BlockQueue::next_tile_l()
{
	cpu_.c = cpu_.l & 0xe0;
	uint8_t column = static_cast<uint8_t>(cpu_.l + 1) & 0x1f;
	cpu_.f = flags::add(column, cpu_.c);
	return static_cast<uint8_t>(column + cpu_.c);
}

// (queue + wBGMapTileQueueSize) = the 4 bytes at entry
void BlockQueue::copy_block_entry(uint16_t entry, uint16_t queue)
{
	uint8_t size = mem_[wBGMapTileQueueSize];
	cpu_.d = static_cast<uint8_t>(queue >> 8);
	cpu_.f = flags::add(static_cast<uint8_t>(queue), size);
	cpu_.e = static_cast<uint8_t>(queue + size);
	for (int i = 0; i < 4; i++) {
		cpu_.a = mem_[entry];
		mem_[REG_DE(gb_)] = cpu_.a;
		if (i < 3) {
			entry++;
			cpu_.f = flags::inc(cpu_.f, cpu_.e);
			cpu_.e++;
		}
	}
	SET_HL(gb_, entry);
}

// queues the tiles, attributes and BG map addresses of the next changed
// block in wc18e
void BlockQueue::queue_changed_block()
{
	cpu_.a = mem_[wIsDMATransferPending];
	cpu_.f = flags::and_(cpu_.a);
	if (cpu_.a)
		return; // a DMA's pending

	cpu_.b = SRAM1;
	cpu_.e = mem_[wc19f];
	cpu_.d = 0;
	unsigned sum = wc18e + cpu_.e;
	cpu_.f = static_cast<uint8_t>((cpu_.f & FLAG_Z) | ((wc18e & 0xfff) + cpu_.e > 0xfff ? FLAG_H : 0)
		| (sum > 0xffff ? FLAG_C : 0));
	uint16_t entry = static_cast<uint16_t>(sum);
	cpu_.a = mem_[entry++];
	SET_HL(gb_, entry);
	cpu_.f = flags::and_(cpu_.a);
	if (!cpu_.a)
		return; // nothing to do

	cpu_.l = mem_[entry];
	cpu_.f = flags::cp(cpu_.a, 0xc0);
	for (int i = 0; i < 2 && (cpu_.f & FLAG_C) == 0; i++) {
		cpu_.f = flags::inc(cpu_.f, cpu_.b);
		cpu_.b++; // SRAM2, SRAM3
		cpu_.f = flags::sub(cpu_.a, 0x20);
		cpu_.a = static_cast<uint8_t>(cpu_.a - 0x20);
		if (i == 0)
			cpu_.f = flags::cp(cpu_.a, 0xc0);
	}
	cpu_.h = cpu_.a;
	cpu_.a = cpu_.b;
	mem_[wBlockPtrBank] = cpu_.b;
	sram_switch(cpu_.b);
	gb_push16(gb_, REG_HL(gb_));
	routines_.call(ADDR_GetBlockPtrBGMapAddress);
	cpu_.b = mem_[wBGMapAddressQueueSize];
	SET_DE(gb_, wBGMapAddressQueue);
	cpu_.f = flags::add(cpu_.e, cpu_.b);
	cpu_.e = static_cast<uint8_t>(cpu_.e + cpu_.b);

	// each block is 2x2 tiles: top-left, top-right, bottom-left, bottom-right
	put_e(cpu_.h);
	put_e(cpu_.l);
	put_e(cpu_.h);
	put_e(next_tile_l());
	SET_BC(gb_, TILEMAP_WIDTH);
	uint16_t bg = REG_HL(gb_);
	unsigned below = bg + TILEMAP_WIDTH;
	cpu_.f = static_cast<uint8_t>((cpu_.f & FLAG_Z) | ((bg & 0xfff) + TILEMAP_WIDTH > 0xfff ? FLAG_H : 0)
		| (below > 0xffff ? FLAG_C : 0));
	SET_HL(gb_, below);
	cpu_.a = cpu_.h & 0xfb; // wraps within the BG map
	cpu_.f = flags::and_(cpu_.a);
	cpu_.h = cpu_.a;
	put_e(cpu_.a);
	put_e(cpu_.l);
	put_e(cpu_.h);
	cpu_.a = next_tile_l();
	mem_[REG_DE(gb_)] = cpu_.a;

	uint8_t addresses = mem_[wBGMapAddressQueueSize];
	cpu_.f = flags::add(addresses, 2 * 4);
	mem_[wBGMapAddressQueueSize] = static_cast<uint8_t>(addresses + 2 * 4);
	uint16_t map = gb_pop16(gb_);

	// sla e / sla e / rl d: block * 4, losing bit 7
	uint16_t offset = static_cast<uint16_t>((mem_[map] * 4) & 0x1ff);
	cpu_.e = static_cast<uint8_t>(offset);
	cpu_.d = static_cast<uint8_t>(offset >> 8);
	cpu_.f = (offset >> 8) ? 0 : FLAG_Z; // rl d
	cpu_.a = mem_[rWBK];
	push_af(); // push_wram
	cpu_.a = GFX_RAM;
	mem_[rWBK] = GFX_RAM;
	gb_push16(gb_, offset);
	copy_block_entry(static_cast<uint16_t>(wRoomBlockTiles + offset), wBGMapTileVRAM0Queue);
	SET_DE(gb_, gb_pop16(gb_));
	copy_block_entry(static_cast<uint16_t>(wRoomBlockAttributes + offset), wBGMapTileVRAM1Queue);

	uint8_t tiles = mem_[wBGMapTileQueueSize];
	cpu_.f = flags::add(tiles, 4);
	mem_[wBGMapTileQueueSize] = static_cast<uint8_t>(tiles + 4);

	cpu_.e = mem_[wc19f];
	cpu_.d = 0;
	uint16_t done = static_cast<uint16_t>(wc18e + cpu_.e);
	mem_[done] = 0;
	mem_[done + 1] = 0;
	SET_HL(gb_, done + 1);
	cpu_.f = flags::add(cpu_.e, 2);
	cpu_.a = static_cast<uint8_t>((cpu_.e + 2) & 0x0f);
	cpu_.f = flags::and_(cpu_.a);
	mem_[wc19f] = cpu_.a;
	pop_af(); // pop_wram
	mem_[rWBK] = cpu_.a;
}

} // namespace

} // namespace wl3::level

using wl3::level::BlockQueue;

// a and f are push_wram's (as it was called), the rest as the original's
PORT(DespawnObject) { BlockQueue(gb).despawn(); }
PORT(Func_ba42) { BlockQueue(gb).set_block(); }
PORT(Func_bb2d) { BlockQueue(gb).clear_block(); }
PORT(Func_bb85) { BlockQueue(gb).queue_changed_block(); }
