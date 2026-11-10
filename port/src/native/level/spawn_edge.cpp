// Spawning the objects of the row or column that scrolled into view
// (src/engine/level/spawn_edge.asm). Plain C++ over memory: the edge
// drawing (scroll.cpp) farcalls them and reads no registers after, so the
// hooks clobber them all.
#include "frame.hpp"
#include "../native.hpp"
#include "../routines.hpp"
#include "object.hpp"
#include "../home/level_map.hpp"
#include "../home/object_pos.hpp"

namespace wl3::level {

namespace {

class EdgeSpawner {
public:
	explicit EdgeSpawner(GB *gb) : gb_(gb), mem_(gb), routines_(gb) {}

	void spawn_row(); // SpawnRowEdgeObjects
	void spawn_column(); // SpawnColEdgeObjects

private:
	void sram_switch(uint8_t bank)
	{
		mem_[wSRAMBank] = bank;
		mem_[rRAMB + 0x100] = bank;
	}
	int start();
	void spawn_block_object(uint16_t block);

	GB *gb_;
	Memory mem_;
	Routines routines_;
};

// the level hasn't ended: the block at wBlockPos, its SRAM bank mapped
// (-1 if it has)
int EdgeSpawner::start()
{
	if (mem_[wLevelEndScreen])
		return -1; // the level ended
	home::BlockPtr block = home::block_ptr(gb_, wBlockPos);
	sram_switch(mem_[wBlockPtrBank]);
	return block.ptr;
}

// the object of the block (bit 7), if its object map entry has one
void EdgeSpawner::spawn_block_object(uint16_t block)
{
	if (!(mem_[block] & 0x80))
		return;
	uint8_t h = static_cast<uint8_t>(block >> 8), l = static_cast<uint8_t>(block);
	home::ObjMapBlock map = home::obj_map_col(gb_, h, l); // Func_d8c
	uint8_t v = mem_[static_cast<uint16_t>(map.row << 8 | map.col)];
	if (map.parity & 1)
		v = static_cast<uint8_t>(v << 4 | v >> 4);
	uint8_t entry = v & 0x0f;
	if (!entry)
		return;
	home::level_block_to_hpos(gb_, h, l); // Func_d3e
	uint8_t wbk = mem_[rWBK]; // push_wram BANK("Level Objects WRAM")
	mem_[rWBK] = BANK_wObjects;
	routines_.in_bank(BANK_SpawnObject, ADDR_SpawnObject, [&] { spawn_object(gb_, entry, block); });
	mem_[rWBK] = wbk; // pop_wram
}

// down 12 blocks, into the next SRAM bank at the end of one
void EdgeSpawner::spawn_row()
{
	int start_block = start();
	if (start_block < 0)
		return;
	uint16_t block = static_cast<uint16_t>(start_block);
	for (int i = 0; i < 12; i++) {
		spawn_block_object(block);
		block = static_cast<uint16_t>(block + 0x100); // inc h
		if (block >> 8 == 0xc0) {
			block = static_cast<uint16_t>((sLevelBlockObjectMap & 0xff00) | (block & 0xff));
			uint8_t bank = static_cast<uint8_t>(mem_[wBlockPtrBank] + 1);
			mem_[wBlockPtrBank] = bank;
			sram_switch(bank);
		}
	}
}

// across 13 blocks (inc l: within the row)
void EdgeSpawner::spawn_column()
{
	int start_block = start();
	if (start_block < 0)
		return;
	uint16_t block = static_cast<uint16_t>(start_block);
	for (int i = 0; i < 13; i++) {
		spawn_block_object(block);
		block = static_cast<uint16_t>((block & 0xff00) | static_cast<uint8_t>(block + 1));
	}
}

} // namespace

} // namespace wl3::level

using wl3::level::EdgeSpawner;

PORT_EX(SpawnRowEdgeObjects, 0, ALL_REGISTERS) { wl3::level::spawn_row_edge_objects(gb); }
PORT_EX(SpawnColEdgeObjects, 0, ALL_REGISTERS) { wl3::level::spawn_col_edge_objects(gb); }

namespace wl3::level {
void spawn_row_edge_objects(GB *gb) { EdgeSpawner(gb).spawn_row(); }
void spawn_col_edge_objects(GB *gb) { EdgeSpawner(gb).spawn_column(); }
} // namespace wl3::level
