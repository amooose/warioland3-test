// Drawing what scrolls into view (src/engine/level/scroll.asm): the row or
// column of blocks at the edge the camera moved past goes into the BG map
// queues (addresses, tiles and attributes), and the objects there spawn.
// And the room ID at Wario's position.
//
// These keep the original's registers, flags and pushes: the queue steps
// push the block map pointer as the asm does (the queue code elsewhere
// reads the stack), and the callers are C.
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"

namespace wl3::level {

namespace {

constexpr uint8_t GFX_RAM = 0x03; // BANK("GFX RAM")

// what a queue step copies
enum class Step { FULL, FIRST, LAST_FULL, LAST_HALF };

class EdgeQueue {
public:
	explicit EdgeQueue(GB *gb) : gb_(gb), cpu_(gb->cpu), mem_(gb), routines_(gb) {}

	void room_id(); // GetNextLevelRoomID
	void update(); // Func_8ed9
	void queue_column_at_block(); // Func_a0e2
	// Func_a2aa, Func_a79e, Func_aca6, Func_b182
	void queue_column(uint16_t queue, uint16_t ptr_var, uint16_t base, int offset, bool wram);
	// QueueBlockTilesAlongRow_OddColumn, ... (row_block_queue.asm)
	void queue_block_row(uint16_t queue, uint16_t ptr_var, uint16_t base, int offset, bool wram, bool quirk);

private:
	void sram_switch(uint8_t bank)
	{
		mem_[wSRAMBank] = bank;
		mem_[rRAMB + 0x100] = bank;
	}
	void inc_l(int n)
	{
		while (n--) {
			cpu_.f = flags::inc(cpu_.f, cpu_.l);
			cpu_.l++;
		}
	}
	void step(uint16_t base, uint16_t ptr_var, int incs, Step kind, bool row);
	void queue_row(uint16_t queue, uint16_t ptr_var, uint16_t base, bool bottom, bool wram);
	void start_queue(uint16_t queue, uint16_t ptr_var);
	void cam_pos(uint16_t cam, uint16_t add);
	bool clamp_neg();
	void clamp_max(uint8_t limit, uint16_t max);
	void store_pos(uint16_t var);
	void column_y();
	void left_x();
	void queue_edge_row(); // .Func_9085

	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
};

// hl = Wario's position: wLevelRoomID = his x high byte + 10 * y high byte
void EdgeQueue::room_id()
{
	uint16_t at = REG_HL(gb_);
	uint8_t y_hi = mem_[at];
	uint8_t x_hi = mem_[at + 2];
	SET_HL(gb_, at + 2);
	uint8_t id = x_hi;
	if (!y_hi) {
		cpu_.f = flags::and_(0);
	} else {
		cpu_.b = y_hi;
		cpu_.c = 10;
		do {
			cpu_.f = flags::add(id, 10);
			id = static_cast<uint8_t>(id + 10);
			cpu_.f = flags::dec(cpu_.f, cpu_.b);
		} while (--cpu_.b);
	}
	cpu_.a = id;
	mem_[wLevelRoomID] = id;
}

// One block of a column (or row) at hl of the level's block map: copies
// bytes of its 4-byte entry in base (skipping incs) to the queue at
// [ptr_var]; a column then moves hl down a row (into the next SRAM bank
// at the end of one), a row right.
void EdgeQueue::step(uint16_t base, uint16_t ptr_var, int incs, Step kind, bool row)
{
	uint16_t map = REG_HL(gb_);
	uint8_t block = mem_[map];
	// ld d, 0 / ld e, a / sla e / sla e / rl d: block * 4, but only the
	// second sla's carry reaches d (bit 7 is lost)
	uint16_t offset = static_cast<uint16_t>((block * 4) & 0x1ff);
	gb_push16(gb_, row ? static_cast<uint16_t>(map + 1) : map);
	unsigned sum = base + offset;
	cpu_.f = static_cast<uint8_t>(flags::z(static_cast<uint8_t>(offset >> 8)) | ((base & 0xfff) + (offset & 0xfff) > 0xfff ? FLAG_H : 0)
		| (sum > 0xffff ? FLAG_C : 0));
	SET_HL(gb_, sum);
	inc_l(incs);
	uint16_t entry = REG_HL(gb_);
	cpu_.d = mem_[ptr_var + 0];
	cpu_.e = mem_[ptr_var + 1];
	if (kind == Step::FULL) {
		cpu_.f = flags::add(cpu_.e, 2);
		mem_[ptr_var + 1] = static_cast<uint8_t>(cpu_.e + 2);
	} else if (kind == Step::FIRST) {
		cpu_.f = flags::inc(cpu_.f, cpu_.e);
		mem_[ptr_var + 1] = static_cast<uint8_t>(cpu_.e + 1);
	}
	cpu_.a = cpu_.e;
	if (kind == Step::FIRST || kind == Step::LAST_HALF) {
		cpu_.a = mem_[entry];
		mem_[REG_DE(gb_)] = cpu_.a;
	} else {
		cpu_.a = mem_[entry++];
		mem_[REG_DE(gb_)] = cpu_.a;
		if (!row) { // the tile below: inc l
			cpu_.f = flags::inc(cpu_.f, static_cast<uint8_t>(entry));
			entry = static_cast<uint16_t>((entry & 0xff00) | static_cast<uint8_t>(entry + 1));
		}
		cpu_.f = flags::inc(cpu_.f, cpu_.e);
		cpu_.e++;
		cpu_.a = mem_[entry];
		mem_[REG_DE(gb_)] = cpu_.a;
	}
	SET_HL(gb_, gb_pop16(gb_));
	if (row || kind == Step::LAST_FULL || kind == Step::LAST_HALF)
		return;
	cpu_.f = flags::inc(cpu_.f, cpu_.h);
	cpu_.h++;
	cpu_.a = cpu_.h;
	cpu_.f = flags::cp(cpu_.h, 0xc0); // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
	if (cpu_.f & FLAG_Z) {
		cpu_.h = sLevelBlockObjectMap >> 8;
		uint8_t bank = mem_[wSRAMBank];
		cpu_.f = flags::inc(cpu_.f, bank);
		cpu_.a = static_cast<uint8_t>(bank + 1);
		sram_switch(cpu_.a);
	}
}

// the queue pointer at ptr_var = the queue + wBGMapTileQueueSize
void EdgeQueue::start_queue(uint16_t queue, uint16_t ptr_var)
{
	cpu_.b = mem_[wBGMapTileQueueSize];
	mem_[ptr_var + 0] = static_cast<uint8_t>(queue >> 8);
	cpu_.f = flags::add(static_cast<uint8_t>(queue), cpu_.b);
	cpu_.a = static_cast<uint8_t>(queue + cpu_.b);
	mem_[ptr_var + 1] = cpu_.a;
}

// the 12 blocks' tiles (or attributes, in GFX RAM) of a column of the level
// map at hl: their left (offset 0) or right (1) tiles, from halfway down
// the first block if wBlockYPos is
void EdgeQueue::queue_column(uint16_t queue, uint16_t ptr_var, uint16_t base, int offset, bool wram)
{
	start_queue(queue, ptr_var);
	cpu_.a = mem_[wBlockYPos + 1] & 0x08;
	cpu_.f = flags::and_(cpu_.a);
	bool half = cpu_.a;
	if (wram) {
		cpu_.a = mem_[rWBK];
		gb_push16(gb_, REG_AF(gb_)); // push af
		cpu_.a = GFX_RAM;
		mem_[rWBK] = GFX_RAM;
	}
	if (!half) {
		for (int i = 0; i < 11; i++)
			step(base, ptr_var, offset, Step::FULL, false);
		step(base, ptr_var, offset, Step::LAST_HALF, false);
	} else {
		step(base, ptr_var, offset + 2, Step::FIRST, false);
		for (int i = 0; i < 10; i++)
			step(base, ptr_var, offset, Step::FULL, false);
		step(base, ptr_var, offset, Step::LAST_FULL, false);
	}
	if (wram) {
		uint16_t af = gb_pop16(gb_);
		cpu_.a = static_cast<uint8_t>(af >> 8);
		cpu_.f = static_cast<uint8_t>(af & 0xf0);
		mem_[rWBK] = cpu_.a;
	}
}

// 16 blocks' tiles (or attributes, in GFX RAM) down the level map from hl,
// for the room's first drawing: their left (offset 0) or right (1) tiles,
// from halfway into the first block if wBlockYPos is.
// quirk (QueueBlockAttributesAlongRow_OddColumn): the half first block's
// tile is read from the queue's address instead of the attributes, and a
// whole first block gets an extra half step at the end
void EdgeQueue::queue_block_row(uint16_t queue, uint16_t ptr_var, uint16_t base, int offset, bool wram, bool quirk)
{
	if (wram) {
		cpu_.a = mem_[rWBK];
		gb_push16(gb_, REG_AF(gb_)); // push af
		cpu_.a = BANK_wRoomBlockAttributes;
		mem_[rWBK] = BANK_wRoomBlockAttributes;
	}
	start_queue(queue, ptr_var);
	cpu_.a = mem_[wBlockYPos + 1] & 0x08;
	cpu_.f = flags::and_(cpu_.a);
	bool half = cpu_.a;
	if (half)
		step(quirk ? wBGMapTileVRAM1Queue : base, ptr_var, offset + 2, Step::FIRST, false);
	cpu_.b = 0x10;
	do {
		step(base, ptr_var, offset, Step::FULL, false);
		cpu_.f = flags::dec(cpu_.f, cpu_.b);
	} while (--cpu_.b);
	if (quirk && !half)
		step(base, ptr_var, offset, Step::LAST_HALF, false);
	if (wram) {
		uint16_t af = gb_pop16(gb_);
		cpu_.a = static_cast<uint8_t>(af >> 8);
		cpu_.f = static_cast<uint8_t>(af & 0xf0);
		mem_[rWBK] = cpu_.a;
	}
}

// a column of 23 BG map addresses at the block's, and the tiles and
// attributes to draw there
void EdgeQueue::queue_column_at_block()
{
	routines_.call(ADDR_GetBlockBGPtr);
	cpu_.b = mem_[wBGMapAddressQueueSize];
	SET_DE(gb_, wBGMapAddressQueue);
	cpu_.f = flags::add(cpu_.e, cpu_.b);
	cpu_.e = static_cast<uint8_t>(cpu_.e + cpu_.b);
	uint16_t bg = REG_HL(gb_);
	for (int i = 0; i < 23; i++) {
		mem_[REG_DE(gb_)] = static_cast<uint8_t>(bg >> 8);
		cpu_.e++;
		mem_[REG_DE(gb_)] = static_cast<uint8_t>(bg);
		cpu_.e++;
		gb_push16(gb_, REG_DE(gb_));
		unsigned sum = bg + TILEMAP_WIDTH;
		cpu_.f = static_cast<uint8_t>((cpu_.f & FLAG_Z) | ((bg & 0xfff) + TILEMAP_WIDTH > 0xfff ? FLAG_H : 0)
			| (sum > 0xffff ? FLAG_C : 0));
		gb_pop16(gb_);
		cpu_.a = static_cast<uint8_t>((sum >> 8) & 0xfb); // wraps within the BG map
		cpu_.f = flags::and_(cpu_.a);
		bg = static_cast<uint16_t>(cpu_.a << 8 | (sum & 0xff));
	}
	SET_HL(gb_, bg);
	cpu_.f = flags::add(mem_[wBGMapAddressQueueSize], 2 * 23);
	mem_[wBGMapAddressQueueSize] = static_cast<uint8_t>(mem_[wBGMapAddressQueueSize] + 2 * 23);

	SET_HL(gb_, wBlockPos);
	routines_.call(ADDR_GetBlockPtr);
	cpu_.a = mem_[wBlockPtrBank];
	sram_switch(cpu_.a);
	cpu_.a = mem_[wBlockXPos + 1] & 0x08;
	cpu_.f = flags::and_(cpu_.a);
	bool odd = cpu_.a;
	uint16_t map = REG_HL(gb_);
	gb_push16(gb_, map);
	if (odd)
		queue_column(wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, 1, false); // Func_a2aa
	else
		queue_column(wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, 0, false); // Func_aca6
	SET_HL(gb_, gb_pop16(gb_));
	cpu_.a = mem_[wBlockPtrBank];
	sram_switch(cpu_.a);
	if (odd)
		queue_column(wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, 1, true); // Func_a79e
	else
		queue_column(wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, 0, true); // Func_b182
	uint8_t size = mem_[wBGMapTileQueueSize];
	cpu_.f = flags::add(size, 23);
	cpu_.a = static_cast<uint8_t>(size + 23);
	mem_[wBGMapTileQueueSize] = cpu_.a;
}

// the 13 blocks' tiles (or attributes, in GFX RAM) of a row of the level
// map at hl: their top or bottom tiles, from halfway into the first block
// if wBlockXPos is
void EdgeQueue::queue_row(uint16_t queue, uint16_t ptr_var, uint16_t base, bool bottom, bool wram)
{
	start_queue(queue, ptr_var);
	cpu_.a = mem_[wBlockXPos + 1] & 0x08;
	cpu_.f = flags::and_(cpu_.a);
	bool half = cpu_.a;
	if (wram) {
		cpu_.a = mem_[rWBK];
		gb_push16(gb_, REG_AF(gb_)); // push af
		cpu_.a = GFX_RAM;
		mem_[rWBK] = GFX_RAM;
	}
	int offset = bottom ? 2 : 0;
	if (!half) {
		for (int i = 0; i < 12; i++)
			step(base, ptr_var, offset, Step::FULL, true);
		step(base, ptr_var, offset, Step::LAST_HALF, true);
	} else {
		step(base, ptr_var, offset + 1, Step::FIRST, true);
		if (bottom) {
			for (int i = 0; i < 11; i++)
				step(base, ptr_var, offset, Step::FULL, true);
			step(base, ptr_var, offset, Step::LAST_FULL, true);
		} else {
			for (int i = 0; i < 12; i++)
				step(base, ptr_var, offset, Step::FULL, true);
		}
	}
	if (wram) {
		uint16_t af = gb_pop16(gb_);
		cpu_.a = static_cast<uint8_t>(af >> 8);
		cpu_.f = static_cast<uint8_t>(af & 0xf0);
		mem_[rWBK] = cpu_.a;
	}
}

// ld hl, cam + 1 / ld a, [hld] / and $f8 / ld e, a / ld h, [hl] / ld l, e /
// ld de, add / add hl, de: the camera's position at a whole tile, moved
void EdgeQueue::cam_pos(uint16_t cam, uint16_t add)
{
	cpu_.a = mem_[cam + 1] & 0xf8;
	cpu_.f = flags::and_(cpu_.a);
	cpu_.e = cpu_.a;
	uint16_t pos = static_cast<uint16_t>(mem_[cam] << 8 | cpu_.e);
	SET_DE(gb_, add);
	unsigned sum = pos + add;
	cpu_.f = static_cast<uint8_t>((cpu_.f & FLAG_Z) | ((pos & 0xfff) + (add & 0xfff) > 0xfff ? FLAG_H : 0)
		| (sum > 0xffff ? FLAG_C : 0));
	SET_HL(gb_, sum);
}

// bit 7, h / jr z / ld hl, 0: true if it was negative
bool EdgeQueue::clamp_neg()
{
	cpu_.f = flags::bit(cpu_.f, cpu_.h, 7);
	if (!(cpu_.h & 0x80))
		return false;
	SET_HL(gb_, 0);
	return true;
}

// ld a, h / cp limit / jr c / ld hl, max / ld a, h
void EdgeQueue::clamp_max(uint8_t limit, uint16_t max)
{
	cpu_.a = cpu_.h;
	cpu_.f = flags::cp(cpu_.h, limit);
	if (!(cpu_.f & FLAG_C)) {
		SET_HL(gb_, max);
		cpu_.a = cpu_.h;
	}
}

void EdgeQueue::store_pos(uint16_t var)
{
	mem_[var + 0] = cpu_.h;
	cpu_.a = cpu_.l;
	mem_[var + 1] = cpu_.l;
}

// a column's y at the right or left edge: the camera's - $10, at least 0,
// and $240 where + $b8 would reach $300
void EdgeQueue::column_y()
{
	cam_pos(wCameraSCY, static_cast<uint16_t>(-0x10));
	if (!clamp_neg()) {
		uint16_t pos = REG_HL(gb_);
		SET_DE(gb_, 0xb8);
		gb_push16(gb_, pos);
		unsigned sum = pos + 0xb8;
		cpu_.a = static_cast<uint8_t>(sum >> 8);
		cpu_.f = flags::cp(cpu_.a, 0x03);
		SET_HL(gb_, gb_pop16(gb_));
		if (!(cpu_.f & FLAG_C))
			SET_HL(gb_, 0x240);
	}
	store_pos(wBlockYPos);
}

// the camera's x - $10, at least 0
void EdgeQueue::left_x()
{
	cam_pos(wCameraSCX, static_cast<uint16_t>(-0x10));
	clamp_neg();
	store_pos(wBlockXPos);
}

// a row of 25 BG map addresses at the block's, and the tiles and attributes
// to draw there
void EdgeQueue::queue_edge_row()
{
	routines_.call(ADDR_GetBlockBGPtr);
	cpu_.b = mem_[wBGMapAddressQueueSize];
	SET_DE(gb_, wBGMapAddressQueue);
	cpu_.f = flags::add(cpu_.e, cpu_.b);
	cpu_.e = static_cast<uint8_t>(cpu_.e + cpu_.b);
	for (int i = 0; i < 25; i++) {
		cpu_.a = cpu_.h;
		mem_[REG_DE(gb_)] = cpu_.a;
		cpu_.f = flags::inc(cpu_.f, cpu_.e);
		cpu_.e++;
		cpu_.a = cpu_.l;
		mem_[REG_DE(gb_)] = cpu_.a;
		cpu_.c = cpu_.l & 0xe0;
		cpu_.f = flags::inc(cpu_.f, cpu_.e);
		cpu_.e++;
		cpu_.f = flags::inc(cpu_.f, cpu_.l);
		cpu_.l++;
		uint8_t column = cpu_.l & 0x1f; // wraps within the BG map row
		cpu_.f = flags::add(column, cpu_.c);
		cpu_.a = static_cast<uint8_t>(column + cpu_.c);
		cpu_.l = cpu_.a;
	}
	uint8_t size = mem_[wBGMapAddressQueueSize];
	cpu_.f = flags::add(size, 2 * 25);
	mem_[wBGMapAddressQueueSize] = static_cast<uint8_t>(size + 2 * 25);

	SET_HL(gb_, wBlockPos);
	routines_.call(ADDR_GetBlockPtr);
	cpu_.a = mem_[wBlockPtrBank];
	sram_switch(cpu_.a);
	cpu_.a = mem_[wBlockYPos + 1] & 0x08;
	cpu_.f = flags::and_(cpu_.a);
	bool bottom = cpu_.a;
	gb_push16(gb_, REG_HL(gb_));
	queue_row(wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, bottom, false);
	SET_HL(gb_, gb_pop16(gb_));
	cpu_.a = mem_[wBlockPtrBank];
	sram_switch(cpu_.a);
	queue_row(wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, bottom, true);
	uint8_t tiles = mem_[wBGMapTileQueueSize];
	cpu_.f = flags::add(tiles, 25);
	cpu_.a = static_cast<uint8_t>(tiles + 25);
	mem_[wBGMapTileQueueSize] = cpu_.a;
}

// draws the rows and columns that scrolled into view (wLevelEdgeUpdateFlags)
// and spawns the objects there
void EdgeQueue::update()
{
	uint8_t edges = mem_[wLevelEdgeUpdateFlags];
	cpu_.a = edges;
	cpu_.f = flags::and_(edges);
	if (!edges)
		return;
	cpu_.b = edges;
	cpu_.a = edges & (LEVEL_EDGE_UPDATE_RIGHT | LEVEL_EDGE_UPDATE_LEFT);
	cpu_.f = flags::and_(cpu_.a);
	uint8_t check = edges;
	if (cpu_.a) {
		bool right = edges & LEVEL_EDGE_UPDATE_RIGHT;
		cpu_.f = flags::bit(cpu_.f, edges, LEVEL_EDGE_UPDATE_RIGHT_F);
		if (!right)
			cpu_.f = flags::bit(cpu_.f, edges, LEVEL_EDGE_UPDATE_LEFT_F);
		if (right || (edges & LEVEL_EDGE_UPDATE_LEFT)) {
			if (right) { // .Func_8fec: the column right of the screen
				column_y();
				cam_pos(wCameraSCX, 0xb0);
				clamp_max(0x0a, 0x9f8);
				store_pos(wBlockXPos);
			} else { // .Func_9039: the column left of it
				column_y();
				left_x();
			}
			queue_column_at_block();
			SET_HL(gb_, wLevelEdgeUpdateFlags);
			mem_[wLevelEdgeUpdateFlags] &= ~(right ? LEVEL_EDGE_UPDATE_RIGHT : LEVEL_EDGE_UPDATE_LEFT);
			routines_.farcall(BANK_SpawnRowEdgeObjects, ADDR_SpawnRowEdgeObjects);
			cpu_.a = mem_[wLevelEdgeUpdateFlags];
			check = cpu_.a;
		}
	}
	// .check_vertical_direction
	bool up = check & LEVEL_EDGE_UPDATE_UP;
	cpu_.f = flags::bit(cpu_.f, check, LEVEL_EDGE_UPDATE_UP_F);
	if (!up)
		cpu_.f = flags::bit(cpu_.f, check, LEVEL_EDGE_UPDATE_DOWN_F);
	if (!up && !(check & LEVEL_EDGE_UPDATE_DOWN))
		return;
	if (up) { // .Func_8fb3: the row above the screen
		cam_pos(wCameraSCY, static_cast<uint16_t>(-0x10));
		clamp_neg();
		store_pos(wBlockYPos);
	} else { // .Func_8f79: the row below it
		cam_pos(wCameraSCY, 0xa0);
		clamp_max(0x03, 0x2f8);
		store_pos(wBlockYPos);
	}
	left_x();
	queue_edge_row();
	SET_HL(gb_, wLevelEdgeUpdateFlags);
	mem_[wLevelEdgeUpdateFlags] &= ~(up ? LEVEL_EDGE_UPDATE_UP : LEVEL_EDGE_UPDATE_DOWN);
	routines_.farcall(BANK_SpawnColEdgeObjects, ADDR_SpawnColEdgeObjects);
}

} // namespace

} // namespace wl3::level

using wl3::level::EdgeQueue;

PORT(GetNextLevelRoomID) { EdgeQueue(gb).room_id(); }
PORT(Func_8ed9) { EdgeQueue(gb).update(); }
PORT(Func_a0e2) { EdgeQueue(gb).queue_column_at_block(); }
PORT(Func_a2aa) { EdgeQueue(gb).queue_column(wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, 1, false); }
PORT(Func_a79e) { EdgeQueue(gb).queue_column(wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, 1, true); }
PORT(Func_aca6) { EdgeQueue(gb).queue_column(wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, 0, false); }
PORT(Func_b182) { EdgeQueue(gb).queue_column(wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, 0, true); }
PORT(QueueBlockTilesAlongRow_OddColumn) { EdgeQueue(gb).queue_block_row(wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, 1, false, false); }
PORT(QueueBlockAttributesAlongRow_OddColumn) { EdgeQueue(gb).queue_block_row(wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, 1, true, true); }
PORT(QueueBlockTilesAlongRow_EvenColumn) { EdgeQueue(gb).queue_block_row(wBGMapTileVRAM0Queue, wcce7, wRoomBlockTiles, 0, false, false); }
PORT(QueueBlockAttributesAlongRow_EvenColumn) { EdgeQueue(gb).queue_block_row(wBGMapTileVRAM1Queue, wc0b3, wRoomBlockAttributes, 0, true, false); }
