// A room's graphics (src/engine/level: load_room_gfx.asm,
// update_animated_tiles.asm, get_bg_ptr.asm): loading its block functions,
// tilemap, tiles and palettes, its animated tiles and palette cycle, and
// the BG map address of a block or of Wario.
//
// The callers are C that may read registers after these, so each entry
// point leaves the registers and flags the original does.
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"

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

// table + 2 * index, as ld d, 0 / add a / ld e, a / rl d / add hl, de
// leaves de
uint16_t RoomGfx::table_entry(uint16_t table, uint8_t index)
{
	uint16_t offset = static_cast<uint16_t>(index * 2);
	SET_DE(gb_, offset);
	return static_cast<uint16_t>(table + offset);
}

// loads the room's block function table, tilemap, main and special tiles
// and its palettes; a room without data ($ff) resets the game
void RoomGfx::load()
{
	uint16_t data = Word(gb_, table_entry(ADDR_RoomBlockAndGfxData, mem_[wRoom]));
	uint8_t high = static_cast<uint8_t>(data >> 8);
	gb_->cpu.a = high;
	gb_->cpu.f = flags::cp(high, 0xff);
	if (high == 0xff) { // debug_assert_not $ff
		SET_HL(gb_, data);
		gb_->cpu.pc = ADDR_Init;
		return;
	}
	uint8_t function_table = mem_[data + 0];
	mem_[wRoomBlockFunctionTable] = function_table;
	mem_[wRoomTileMap] = mem_[data + 1];
	mem_[wRoomMainTiles] = mem_[data + 2];
	mem_[wRoomSpecialTiles] = mem_[data + 3];
	mem_[wRoomPalettes] = mem_[data + 4];
	SET_HL(gb_, data + 4);

	// the function table's bank: one per 6 tables
	uint8_t quotient = function_table / 6;
	uint8_t bank = static_cast<uint8_t>(LEVEL_DATA_1 + quotient);
	mem_[wRoomBlockFunctionTableBank] = bank;
	gb_->cpu.b = quotient;
	gb_->cpu.a = bank;
	gb_->cpu.f = flags::add(LEVEL_DATA_1, quotient);
	routines_.call(ADDR_Func_298d);
	routines_.call(ADDR_LoadBlockFunctionTable);
	gb_->cpu.a = mem_[wRoomBlockFunctionTableBank];
	mem_[wc0d0] = gb_->cpu.a;
	routines_.call(ADDR_LoadRoomTileMap);

	uint8_t main_tiles = mem_[wRoomMainTiles] >> 3;
	gb_->cpu.a = static_cast<uint8_t>(main_tiles + ROOM_MAIN_TILES_1);
	gb_->cpu.f = flags::add(main_tiles, ROOM_MAIN_TILES_1);
	mem_[wRoomMainTilesBank] = gb_->cpu.a;
	routines_.call(ADDR_LoadRoomMainTiles);

	uint8_t special_tiles = mem_[wRoomSpecialTiles] >> 3;
	gb_->cpu.a = static_cast<uint8_t>(special_tiles + ROOM_SPECIAL_TILES_1);
	gb_->cpu.f = flags::add(special_tiles, ROOM_SPECIAL_TILES_1);
	mem_[wRoomSpecialTilesBank] = gb_->cpu.a;
	routines_.call(ADDR_LoadRoomSpecialTiles);

	gb_->cpu.a = LEVEL_PALETTES;
	mem_[wPaletteBank] = LEVEL_PALETTES;
	routines_.call(ADDR_LoadRoomPalettes);
	gb_->cpu.pc = gb_pop16(gb_); // ret
}

void RoomGfx::init_animated_tiles()
{
	uint16_t entry = table_entry(ADDR_AnimatedTilesGroups, mem_[wAnimatedTilesGroup]);
	mem_[wAnimatedTilesFrameDuration] = mem_[entry];
	mem_[wAnimatedTilesGfx] = mem_[entry + 1];
	mem_[wAnimatedTilesFrame] = 0;
	mem_[wAnimatedTilesFrameCount] = 0;
	SET_HL(gb_, entry + 1);
	gb_->cpu.a = 0;
	gb_->cpu.f = FLAG_Z;
}

void RoomGfx::init_animated_pals()
{
	uint16_t entry = table_entry(ADDR_RoomPalCycles, mem_[wRoomPalCycle]);
	uint8_t lo = mem_[entry], hi = mem_[entry + 1];
	mem_[wRoomPalCyclePtr + 1] = lo;
	mem_[wRoomPalCyclePtr + 0] = hi;
	uint16_t cycle = static_cast<uint16_t>((mem_[entry] | hi << 8) + ROOM_PALCYCLE_LENGTH);
	uint8_t duration = mem_[cycle];
	mem_[wRoomPalCycleDuration] = duration;
	mem_[wRoomPalCycleCounter] = duration >> 1;
	mem_[wRoomPalCycleIndex] = 0;
	SET_DE(gb_, ROOM_PALCYCLE_LENGTH);
	SET_HL(gb_, cycle);
	gb_->cpu.a = 0;
	gb_->cpu.f = FLAG_Z;
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
	uint8_t duration = mem_[wAnimatedTilesFrameDuration];
	if (!duration) {
		gb_->cpu.a = 0;
		gb_->cpu.f = flags::and_(0);
		return;
	}
	uint8_t count = static_cast<uint8_t>(mem_[wAnimatedTilesFrameCount] + 1);
	mem_[wAnimatedTilesFrameCount] = count;
	gb_->cpu.b = duration;
	if (count < duration) {
		gb_->cpu.a = count;
		gb_->cpu.f = flags::cp(count, duration);
		return;
	}
	mem_[wAnimatedTilesFrameCount] = 0;
	uint16_t src = static_cast<uint16_t>(
		static_cast<uint8_t>(mem_[wAnimatedTilesGfx] * 2 + (ADDR_AnimatedTilesGfx >> 8)) << 8
		| (ADDR_AnimatedTilesGfx & 0xff));
	uint8_t frame = static_cast<uint8_t>(mem_[wAnimatedTilesFrame] + 1);
	if (frame == 4)
		frame = 0;
	mem_[wAnimatedTilesFrame] = frame;
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
			mem_[dest] = mem_[src++];
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
	mem_[wBGPtr + 0] = static_cast<uint8_t>(hl >> 8);
	mem_[wBGPtr + 1] = static_cast<uint8_t>(hl);
	gb_->cpu.a = static_cast<uint8_t>(hl);
}

void RoomGfx::block_bg_ptr()
{
	uint16_t y = static_cast<uint16_t>(mem_[wBlockYPos + 0] << 8 | mem_[wBlockYPos + 1]);
	uint16_t x = static_cast<uint16_t>(mem_[wBlockXPos + 0] << 8 | mem_[wBlockXPos + 1]);
	bg_ptr(y, x);
}

// Wario's: 24 pixels above his position
void RoomGfx::wario_bg_ptr()
{
	uint16_t y = static_cast<uint16_t>((mem_[wWarioYPos + 0] << 8 | mem_[wWarioYPos + 1]) - 0x18);
	uint16_t x = static_cast<uint16_t>(mem_[wWarioXPos + 0] << 8 | mem_[wWarioXPos + 1]);
	bg_ptr(y, x);
}

} // namespace

} // namespace wl3::level

using wl3::level::RoomGfx;

// LoadRoomPalettes's registers (or a jump to Init for a room without data)
PORT_EX(LoadRoomGfx, HOOK_MANUAL_RET, 0) { RoomGfx(gb).load(); }

// de = 2 * the group, hl at its gfx byte; a = 0, f = Z
PORT(InitRoomAnimatedTiles) { RoomGfx(gb).init_animated_tiles(); }

// de = ROOM_PALCYCLE_LENGTH, hl = the cycle's duration; a = 0, f = Z
PORT(InitRoomAnimatedPals) { RoomGfx(gb).init_animated_pals(); }

PORT(UpdateRoomAnimatedTiles) { RoomGfx(gb).update_animated_tiles(); }
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
