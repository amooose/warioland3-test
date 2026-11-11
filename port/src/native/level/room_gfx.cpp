// A room's graphics (src/engine/level: load_room_gfx.asm,
// update_animated_tiles.asm, get_bg_ptr.asm): loading its block functions,
// tilemap, tiles and palettes, its animated tiles and palette cycle, and
// the BG map address of a block or of Wario.
//
// The room load (LoadRoomGfx and the animated tiles' and palettes' setup)
// is called only by LoadRoom, which reads no registers after, so those
// hooks clobber them all. The others leave the registers and flags the
// original does.
#include "frame.hpp"
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"
#include "room_gfx.hpp"

namespace wl3::level {

namespace {

constexpr uint8_t LEVEL_DATA_1 = 0x38; // BANK("Level Data 1")
constexpr uint8_t ROOM_MAIN_TILES_1 = 0x51; // BANK("Room Main Tiles 1")
constexpr uint8_t ROOM_SPECIAL_TILES_1 = 0x4e; // BANK("Room Special Tiles 1")
constexpr uint8_t LEVEL_PALETTES = 0x33; // BANK("Level Palettes")
constexpr uint8_t VRAM1 = BANK_v1Tiles2;

class RoomGfx {
public:
	explicit RoomGfx(GB *gb) : gb_(gb), mem_(gb), routines_(gb) {}

	void load(); // LoadRoomGfx
	void init_animated_tiles(); // InitRoomAnimatedTiles
	void init_animated_pals(); // InitRoomAnimatedPals
	void update_animated_tiles(); // UpdateRoomAnimatedTiles
	void block_bg_ptr(); // GetBlockBGPtr
	void wario_bg_ptr(); // GetWarioBGPtr

private:
	uint16_t table_entry(uint16_t table, uint8_t index);
	void bg_ptr(uint16_t y, uint16_t x);
	void wait_stat(bool hblank);

	GB *gb_;
	Memory mem_;
	Routines routines_;
};

// table + 2 * index (ld d, 0 / add a / ld e, a / rl d / add hl, de)
uint16_t RoomGfx::table_entry(uint16_t table, uint8_t index)
{
	return static_cast<uint16_t>(table + index * 2);
}

// loads the room's block function table, tilemap, main and special tiles
// and its palettes; a room without data ($ff) resets the game
void RoomGfx::load()
{
	// the room tables and data, all in bank $30 (LoadRoomGfx's and these routines')
	Memory rom(gb_, BANK_LoadRoomGfx);
	uint16_t data = rom.word(table_entry(ADDR_RoomBlockAndGfxData, mem_.room()));
	if (data >> 8 == 0xff) { // debug_assert_not $ff
		gb_tail_jump(gb_, ADDR_Init);
		return;
	}
	uint8_t function_table = rom[data + 0];
	mem_.room_block_function_table() = function_table;
	mem_.room_tile_map() = rom[data + 1];
	mem_.room_main_tiles() = rom[data + 2];
	mem_.room_special_tiles() = rom[data + 3];
	mem_.room_palettes() = rom[data + 4];

	// the function table's bank: one per 6 tables
	mem_.room_block_function_table_bank() = static_cast<uint8_t>(LEVEL_DATA_1 + function_table / 6);
	routines_.call(ADDR_Func_298d);
	routines_.call(ADDR_LoadBlockFunctionTable);
	mem_.unk_c0d0() = mem_.room_block_function_table_bank();
	routines_.call(ADDR_LoadRoomTileMap);
	mem_.room_main_tiles_bank() = static_cast<uint8_t>((mem_.room_main_tiles() >> 3) + ROOM_MAIN_TILES_1);
	routines_.call(ADDR_LoadRoomMainTiles);
	mem_.room_special_tiles_bank() = static_cast<uint8_t>((mem_.room_special_tiles() >> 3) + ROOM_SPECIAL_TILES_1);
	routines_.call(ADDR_LoadRoomSpecialTiles);
	mem_.palette_bank() = LEVEL_PALETTES;
	routines_.call(ADDR_LoadRoomPalettes);
}

void RoomGfx::init_animated_tiles()
{
	Memory rom(gb_, BANK_LoadRoomGfx); // the groups, in bank $30
	uint16_t entry = table_entry(ADDR_AnimatedTilesGroups, mem_.animated_tiles_group());
	mem_.animated_tiles_frame_duration() = rom[entry];
	mem_.animated_tiles_gfx() = rom[entry + 1];
	mem_.animated_tiles_frame() = 0;
	mem_.animated_tiles_frame_count() = 0;
}

void RoomGfx::init_animated_pals()
{
	Memory rom(gb_, BANK_LoadRoomGfx); // the cycles, in bank $30
	uint16_t entry = table_entry(ADDR_RoomPalCycles, mem_.room_pal_cycle());
	uint8_t lo = rom[entry], hi = rom[entry + 1];
	mem_[wRoomPalCyclePtr + 1] = lo;
	mem_[wRoomPalCyclePtr + 0] = hi;
	uint16_t cycle = static_cast<uint16_t>((rom[entry] | hi << 8) + ROOM_PALCYCLE_LENGTH);
	uint8_t duration = rom[cycle];
	mem_.room_pal_cycle_duration() = duration;
	mem_.room_pal_cycle_counter() = duration >> 1;
	mem_.room_pal_cycle_index() = 0;
}

// wait_not_hblank / wait_hblank, letting emulated time pass as the asm's
// polling does
void RoomGfx::wait_stat(bool hblank)
{
	while (static_cast<bool>(mem_[rSTAT] & STAT_MODE) == hblank) {
		if (!gb_->no_tick)
			gb_tick(gb_, 32);
	}
}

// copies the next frame of the room's animated tiles to v1Tiles2 tile $70,
// a row of 8 bytes per H-Blank, when the frame's duration has passed
void RoomGfx::update_animated_tiles()
{
	uint8_t ly = mem_[rLY];
	if (ly >= LY_VBLANK - 20) { // too close to V-Blank
		gb_->cpu.a = ly;
		gb_->cpu.f = flags::cp(ly, LY_VBLANK - 20);
		return;
	}
	uint8_t duration = mem_.animated_tiles_frame_duration();
	if (!duration) {
		gb_->cpu.a = 0;
		gb_->cpu.f = flags::and_(0);
		return;
	}
	uint8_t count = static_cast<uint8_t>(mem_.animated_tiles_frame_count() + 1);
	mem_.animated_tiles_frame_count() = count;
	gb_->cpu.b = duration;
	if (count < duration) {
		gb_->cpu.a = count;
		gb_->cpu.f = flags::cp(count, duration);
		return;
	}
	mem_.animated_tiles_frame_count() = 0;
	uint16_t src = static_cast<uint16_t>(
		static_cast<uint8_t>(mem_.animated_tiles_gfx() * 2 + (ADDR_AnimatedTilesGfx >> 8)) << 8
		| (ADDR_AnimatedTilesGfx & 0xff));
	uint8_t frame = static_cast<uint8_t>(mem_.animated_tiles_frame() + 1);
	if (frame == 4)
		frame = 0;
	mem_.animated_tiles_frame() = frame;
	// swap a / add a (x3): the frame's $80 bytes in, with the last add's
	// carry as d
	uint8_t swapped = static_cast<uint8_t>(frame << 4 | frame >> 4);
	uint16_t offset = static_cast<uint16_t>(static_cast<uint8_t>(swapped << 3) | ((swapped << 2) & 0x80 ? 0x100 : 0));
	src = static_cast<uint16_t>(src + offset);

	uint16_t dest = v1Tiles2 + TILE_SIZE * 0x70;
	mem_[rVBK] = VRAM1;
	for (int row = 0; row < TILE_SIZE; row++) {
		wait_stat(false);
		wait_stat(true);
		for (int i = 0; i < 8; i++) {
			mem_[dest] = RomPtr(gb_, BANK_AnimatedTilesGfx, src++)[0];
			dest = static_cast<uint16_t>((dest & 0xff00) | static_cast<uint8_t>(dest + 1)); // inc e
		}
	}
	mem_[rVBK] = 0; // VRAM0
	SET_DE(gb_, dest);
	SET_HL(gb_, src);
	gb_->cpu.b = 0;
	gb_->cpu.a = 0;
	gb_->cpu.f = FLAG_Z;
}

// hl and wBGPtr = the BG map address of pixel (x, y): de = the row's
// offset, a = l, and add hl, de's flags (z from and TILEMAP_WIDTH - 1)
void RoomGfx::bg_ptr(uint16_t y, uint16_t x)
{
	uint16_t row = static_cast<uint16_t>((y >> 3) << 5);
	uint16_t de = static_cast<uint16_t>((row & 0x00ff) | (row & ((TILEMAP_WIDTH * TILEMAP_HEIGHT - 1) & 0xff00)));
	uint8_t column = (x >> 3) & (TILEMAP_WIDTH - 1);
	uint16_t hl = static_cast<uint16_t>((v0BGMap0 & 0xff00) | column);
	unsigned sum = hl + de;
	gb_->cpu.f = static_cast<uint8_t>(flags::z(column) | ((hl & 0xfff) + (de & 0xfff) > 0xfff ? FLAG_H : 0)
		| (sum > 0xffff ? FLAG_C : 0));
	hl = static_cast<uint16_t>(sum);
	SET_DE(gb_, de);
	SET_HL(gb_, hl);
	mem_.bg_pointer() = hl;
	gb_->cpu.a = static_cast<uint8_t>(hl);
}

void RoomGfx::block_bg_ptr()
{
	uint16_t y = mem_.block_y();
	uint16_t x = mem_.block_x();
	bg_ptr(y, x);
}

// Wario's: 24 pixels above his position
void RoomGfx::wario_bg_ptr()
{
	uint16_t y = static_cast<uint16_t>(mem_.wario_y() - 0x18);
	uint16_t x = mem_.wario_x();
	bg_ptr(y, x);
}

} // namespace

uint16_t block_bg_ptr(GB *gb)
{
	Memory mem(gb);
	uint16_t y = mem.block_y();
	uint16_t x = mem.block_x();
	uint16_t row = static_cast<uint16_t>((y >> 3) << 5);
	uint16_t offset = static_cast<uint16_t>((row & 0x00ff) | (row & ((TILEMAP_WIDTH * TILEMAP_HEIGHT - 1) & 0xff00)));
	auto ptr = static_cast<uint16_t>((v0BGMap0 & 0xff00) + ((x >> 3) & (TILEMAP_WIDTH - 1)) + offset);
	mem.bg_pointer() = ptr;
	return ptr;
}

uint16_t wario_bg_ptr(GB *gb)
{
	Memory mem(gb);
	uint16_t y = static_cast<uint16_t>(mem.wario_y() - 0x18);
	uint16_t x = mem.wario_x();
	uint16_t row = static_cast<uint16_t>((y >> 3) << 5);
	uint16_t offset = static_cast<uint16_t>((row & 0x00ff) | (row & ((TILEMAP_WIDTH * TILEMAP_HEIGHT - 1) & 0xff00)));
	auto ptr = static_cast<uint16_t>((v0BGMap0 & 0xff00) + ((x >> 3) & (TILEMAP_WIDTH - 1)) + offset);
	mem.bg_pointer() = ptr;
	return ptr;
}

} // namespace wl3::level

using wl3::level::RoomGfx;

// (or a jump to Init for a room without data)
PORT_EX(LoadRoomGfx, 0, ALL_REGISTERS) { RoomGfx(gb).load(); }

// de = 2 * the group, hl at its gfx byte; a = 0, f = Z
PORT_EX(InitRoomAnimatedTiles, 0, ALL_REGISTERS) { RoomGfx(gb).init_animated_tiles(); }

// de = ROOM_PALCYCLE_LENGTH, hl = the cycle's duration; a = 0, f = Z
PORT_EX(InitRoomAnimatedPals, 0, ALL_REGISTERS) { RoomGfx(gb).init_animated_pals(); }

PORT(UpdateRoomAnimatedTiles) { wl3::level::update_room_animated_tiles(gb); }
PORT(GetBlockBGPtr) { RoomGfx(gb).block_bg_ptr(); }
PORT(GetWarioBGPtr) { RoomGfx(gb).wario_bg_ptr(); }

// unreferenced: a = the nybble for column l / 2 of the row at h
PORT(Func_8eac)
{
	bool odd = gb->cpu.l & 1;
	gb->cpu.l = static_cast<uint8_t>((gb->cpu.l >> 1) + 0xb0);
	uint8_t v = wl3::Memory(gb)[REG_HL(gb)];
	if (!odd)
		v = static_cast<uint8_t>(v << 4 | v >> 4);
	gb->cpu.a = v & 0x0f;
	gb->cpu.f = wl3::flags::and_(gb->cpu.a);
}

namespace wl3::level {
void update_room_animated_tiles(GB *gb)
{
	trace_entry(gb, ADDR_UpdateRoomAnimatedTiles); // it reads LY
	RoomGfx(gb).update_animated_tiles();
}
} // namespace wl3::level
