// Spawning the objects of the row or column that scrolled into view
// (src/engine/level/spawn_edge.asm). Called by farcall from the edge
// drawing (scroll.cpp), whose callers are C: they keep the original's
// registers, flags and pushes.
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"

namespace wl3::level {

namespace {

class EdgeSpawner {
public:
	explicit EdgeSpawner(GB *gb) : gb_(gb), cpu_(gb->cpu), mem_(gb), routines_(gb) {}

	void spawn_row(); // SpawnRowEdgeObjects
	void spawn_column(); // SpawnColEdgeObjects

private:
	void sram_switch(uint8_t bank)
	{
		mem_[wSRAMBank] = bank;
		mem_[rRAMB + 0x100] = bank;
	}
	bool start();
	void spawn_block_object();

	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
};

// the level hasn't ended: hl = the block at wBlockPos, its SRAM bank mapped
bool EdgeSpawner::start()
{
	cpu_.a = mem_[wLevelEndScreen];
	cpu_.f = flags::and_(cpu_.a);
	if (cpu_.a)
		return false; // the level ended
	SET_HL(gb_, wBlockPos);
	routines_.call(ADDR_GetBlockPtr);
	cpu_.a = mem_[wBlockPtrBank];
	sram_switch(cpu_.a);
	return true;
}

// the object of the block at hl (bit 7), if it has one that isn't out
void EdgeSpawner::spawn_block_object()
{
	uint16_t block = REG_HL(gb_);
	cpu_.f = flags::bit(cpu_.f, mem_[block], 7);
	if (!(mem_[block] & 0x80))
		return;
	gb_push16(gb_, block);
	routines_.call(ADDR_Func_d8c);
	uint8_t v = mem_[REG_HL(gb_)];
	cpu_.f = flags::bit(cpu_.f, cpu_.b, 0);
	if (cpu_.b & 1)
		v = static_cast<uint8_t>(v << 4 | v >> 4);
	cpu_.a = v & 0x0f;
	cpu_.f = flags::and_(cpu_.a);
	cpu_.c = cpu_.a;
	SET_HL(gb_, gb_pop16(gb_));
	if (!cpu_.c)
		return;
	gb_push16(gb_, REG_DE(gb_));
	gb_push16(gb_, REG_BC(gb_));
	routines_.call(ADDR_Func_d3e);
	cpu_.d = cpu_.h;
	cpu_.e = cpu_.l;
	gb_push16(gb_, REG_HL(gb_));
	cpu_.a = mem_[rWBK]; // push_wram BANK("Level Objects WRAM")
	gb_push16(gb_, REG_AF(gb_));
	cpu_.a = BANK_wObjects;
	mem_[rWBK] = BANK_wObjects;
	routines_.farcall(BANK_SpawnObject, ADDR_SpawnObject);
	uint16_t af = gb_pop16(gb_); // pop_wram
	cpu_.a = static_cast<uint8_t>(af >> 8);
	cpu_.f = static_cast<uint8_t>(af & 0xf0);
	mem_[rWBK] = cpu_.a;
	SET_HL(gb_, gb_pop16(gb_));
	SET_BC(gb_, gb_pop16(gb_));
	SET_DE(gb_, gb_pop16(gb_));
}

// down 12 blocks, into the next SRAM bank at the end of one
void EdgeSpawner::spawn_row()
{
	if (!start())
		return;
	cpu_.e = 0x0c;
	do {
		spawn_block_object();
		cpu_.f = flags::inc(cpu_.f, cpu_.h);
		cpu_.h++;
		cpu_.a = cpu_.h;
		cpu_.f = flags::cp(cpu_.h, 0xc0);
		if (cpu_.f & FLAG_Z) {
			cpu_.h = sLevelBlockObjectMap >> 8;
			uint8_t bank = mem_[wBlockPtrBank];
			cpu_.f = flags::inc(cpu_.f, bank);
			cpu_.a = static_cast<uint8_t>(bank + 1);
			mem_[wBlockPtrBank] = cpu_.a;
			sram_switch(cpu_.a);
		}
		cpu_.f = flags::dec(cpu_.f, cpu_.e);
	} while (--cpu_.e);
}

// across 13 blocks (b's bit 0: which half of the object map byte)
void EdgeSpawner::spawn_column()
{
	if (!start())
		return;
	cpu_.e = 0x0d;
	do {
		spawn_block_object();
		cpu_.a = cpu_.b ^ 0x01;
		cpu_.f = flags::xor_(cpu_.a);
		cpu_.b = cpu_.a;
		cpu_.f = flags::inc(cpu_.f, cpu_.l);
		cpu_.l++;
		cpu_.f = flags::dec(cpu_.f, cpu_.e);
	} while (--cpu_.e);
}

} // namespace

} // namespace wl3::level

using wl3::level::EdgeSpawner;

PORT(SpawnRowEdgeObjects) { EdgeSpawner(gb).spawn_row(); }
PORT(SpawnColEdgeObjects) { EdgeSpawner(gb).spawn_column(); }
