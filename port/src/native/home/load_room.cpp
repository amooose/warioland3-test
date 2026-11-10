// Loading a room (src/home/load_room.asm): its header (spawn, limits,
// camera, objects, animated tiles and palettes), its graphics' tables,
// tiles and palettes, and its palette cycle each frame.
//
// Plain C++ over memory. The callers are C (StartRoom, LoadRoomGfx, the
// game loop) and read no registers after, so the hooks clobber them all.
// The debug asserts jump to Init with the asm's push af still on the
// stack.
#include "../level/frame.hpp"
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"
#include "memory.hpp"
#include "pals.hpp"

namespace wl3::home {

namespace {

constexpr uint8_t BANK_BLOCK_FUNCTION_TABLES_2 = 0x50; // BANK("Block Function Tables 2")
constexpr uint8_t LEVEL_PALETTES = 0x33; // BANK("Level Palettes")
constexpr uint8_t LEVEL_OBJECTS_WRAM = 0x01; // BANK("Level Objects WRAM")

class RoomLoader {
public:
	explicit RoomLoader(GB *gb) : gb_(gb), mem_(gb), routines_(gb) {}

	void load(); // LoadRoom
	void load_block_function_table(); // LoadBlockFunctionTable
	void load_block_tiles(); // Func_298d
	void load_tile_map(); // LoadRoomTileMap
	void load_main_tiles(); // LoadRoomMainTiles
	void load_special_tiles(); // LoadRoomSpecialTiles
	void load_palettes(); // LoadRoomPalettes
	void update_animated_pals(); // UpdateRoomAnimatedPals

private:
	// table's dw at index (add a: the index's low 7 bits)
	uint16_t pointer(uint16_t table, uint8_t index)
	{
		return Word(gb_, static_cast<uint16_t>(table + static_cast<uint8_t>(index * 2)));
	}
	// copies count bytes from src to dest with the ROM bank in bank_var
	// mapped (by way of wTempBank)
	void far_copy(uint16_t bank_var, uint16_t src, uint16_t dest, uint32_t count)
	{
		uint8_t bank = mem_[bank_var];
		mem_.temp_bank() = bank;
		uint8_t rom = mem_.rom_bank();
		switch_bank(bank);
		copy(gb_, RomPtr(gb_, bank, src), dest, count);
		switch_bank(rom);
	}
	void switch_bank(uint8_t bank) { bankswitch(gb_, bank, gb_->cpu.pc); }
	// debug_assert_not $ff: Init, with the asm's push af (a = the ROM
	// bank it came from) on the stack
	void reset(uint8_t rom, uint8_t f)
	{
		gb_push16(gb_, static_cast<uint16_t>(rom << 8 | f));
		gb_tail_jump(gb_, ADDR_Init);
	}
	void wait_next_hblank()
	{
		while (!(mem_[rSTAT] & STAT_MODE))
			idle(gb_, 32);
		while (mem_[rSTAT] & STAT_MODE)
			idle(gb_, 32);
	}

	GB *gb_;
	Memory mem_;
	Routines routines_;
};

void RoomLoader::load_block_tiles()
{
	uint16_t src = pointer(ADDR_RoomBlockTilesPointers, mem_.room_block_function_table());
	far_copy(wRoomBlockFunctionTableBank, src, wRoomBlockTiles, 4 * NUM_ROOM_BLOCK_TYPES);
}

void RoomLoader::load_tile_map()
{
	uint16_t src = pointer(ADDR_RoomBlockAttributePointers, mem_.room_tile_map());
	uint8_t bank = mem_.unk_c0d0();
	mem_.temp_bank() = bank;
	farcall(gb_, bank, ADDR_Decompress, [&] { decompress(gb_, RomPtr(gb_, bank, src), wRoomBlockAttributes); });
}

void RoomLoader::load_main_tiles()
{
	uint16_t src = pointer(ADDR_LevelMainTilesPointers, mem_.room_main_tiles());
	far_copy(wRoomMainTilesBank, src, v0Tiles2, 0x800);
}

void RoomLoader::load_special_tiles()
{
	mem_[rVBK] = 1;
	uint16_t src = pointer(ADDR_SpecialTilesPointers, mem_.room_special_tiles());
	far_copy(wRoomSpecialTilesBank, src, v0Tiles2, 0x800);
	mem_[rVBK] = 0;
}

void RoomLoader::load_palettes()
{
	uint16_t src = pointer(ADDR_RoomPals, mem_.room_palettes());
	uint8_t bank = mem_.palette_bank();
	mem_.temp_bank() = bank;
	farcall(gb_, bank, ADDR_LoadPalsToTempPals1, [&] { load_pals_to_temp_pals1(gb_, RomPtr(gb_, bank, src)); });
}

// the room's palette cycle: when its duration has passed, the next of its
// palettes into the BG palettes, 12 bytes per H-Blank (only with at least
// 8 scanlines to V-Blank)
void RoomLoader::update_animated_pals()
{
	if (mem_[rLY] >= LY_VBLANK - 8)
		return;
	uint8_t duration = mem_.room_pal_cycle_duration();
	if (!duration)
		return;
	uint8_t count = static_cast<uint8_t>(mem_.room_pal_cycle_counter() + 1);
	mem_.room_pal_cycle_counter() = count;
	if (count < duration)
		return;
	mem_.room_pal_cycle_counter() = 0;
	uint16_t cycle = static_cast<uint16_t>(mem_[wRoomPalCyclePtr + 0] << 8 | mem_[wRoomPalCyclePtr + 1]);
	uint8_t index = static_cast<uint8_t>(mem_.room_pal_cycle_index() + 1);
	if (index >= ROOM_PALCYCLE_LENGTH)
		index = 0;
	mem_.room_pal_cycle_index() = index;

	uint8_t rom = mem_.rom_bank();
	switch_bank(BANK_RoomPalCycles);
	uint8_t pal = RomPtr(gb_, BANK_RoomPalCycles, static_cast<uint16_t>(cycle + index))[0];
	RomPtr pals(gb_, LEVEL_PALETTES, RomPtr(gb_, BANK_RoomPalCycles, static_cast<uint16_t>(ADDR_RoomPals + pal * 2)).word());
	switch_bank(rom);

	switch_bank(LEVEL_PALETTES);
	mem_[rBGPI] = BGPI_AUTOINC | 0;
	for (int i = 0; i < 5; i++) {
		wait_next_hblank();
		for (int j = 0; j < 12; j++, ++pals)
			mem_[rBGPD] = *pals;
	}
	wait_next_hblank();
	for (int j = 0; j < 4; j++, ++pals)
		mem_[rBGPD] = *pals;
	switch_bank(rom);
}

// the room's block functions: from its table in group 1 or 2's bank
void RoomLoader::load_block_function_table()
{
	uint8_t table = mem_.room_block_function_table();
	uint8_t offset = static_cast<uint8_t>(table * 2);
	uint16_t at = static_cast<uint16_t>(ADDR_BlockFunctionTablePointers + offset);
	uint8_t rom = mem_.rom_bank();
	switch_bank(BANK_BlockFunctionTablePointers);
	uint16_t src = RomPtr(gb_, BANK_BlockFunctionTablePointers, at).word();
	if (src >> 8 == 0xff) { // .reset; push af saved add a / add hl, de's flags
		uint8_t f = static_cast<uint8_t>((offset ? 0 : FLAG_Z)
			| ((ADDR_BlockFunctionTablePointers & 0xfff) + offset > 0xfff ? FLAG_H : 0)
			| (ADDR_BlockFunctionTablePointers + offset > 0xffff ? FLAG_C : 0));
		reset(rom, f);
		return;
	}
	bool group_2 = table >= NUM_BLOCK_FUNCTION_TABLES_GROUP_1 - 1;
	if (group_2)
		switch_bank(BANK_BLOCK_FUNCTION_TABLES_2);
	copy(gb_, RomPtr(gb_, group_2 ? BANK_BLOCK_FUNCTION_TABLES_2 : BANK_BlockFunctionTablePointers, src),
		wBlockFunctionTable, 0x100);
	if (group_2)
		switch_bank(BANK_BlockFunctionTablePointers);
	switch_bank(rom);
}

// a level's header: its rooms' headers in bank $30 or $31
void RoomLoader::load()
{
	mem_.prev_wario_screen_y_pos() = 0x70;
	mem_.cam_right_spacing() = 0x58;
	mem_.cam_left_spacing() = 0x58; // $58 - ($58 - $58)

	uint8_t rom = mem_.rom_bank();
	switch_bank(BANK_LevelHeaders);
	uint8_t level = mem_.level();
	uint16_t rooms = RomPtr(gb_, BANK_LevelHeaders, static_cast<uint16_t>(ADDR_LevelHeaders + level * 2)).word();
	if (rooms >> 8 == 0xff) {
		reset(rom, 0); // the flags of add $58
		return;
	}
	switch_bank(rom);

	level = mem_.level();
	rom = mem_.rom_bank();
	uint8_t headers = level < THE_GRASSLANDS_NIGHT_1 ? 0x30 : 0x31;
	switch_bank(headers);
	uint16_t header_at = RomPtr(gb_, headers, static_cast<uint16_t>(rooms + static_cast<uint8_t>(mem_.level_room_id() * 2))).word();
	RomPtr header(gb_, headers, header_at);
	if (header_at >> 8 == 0xff) {
		reset(rom, flags::cp(level, THE_GRASSLANDS_NIGHT_1));
		return;
	}
	mem_.wario_spawn_pos() = header[0];
	uint8_t y_limits = header[1];
	mem_.room_upper_y_limit() = y_limits >> 4;
	mem_.room_lower_y_limit() = y_limits & 0x0f;
	uint8_t x_limits = header[2];
	mem_.room_left_x_limit() = x_limits >> 4;
	mem_.room_right_x_limit() = x_limits & 0x0f;
	mem_.camera_config_flags() = header[3];

	mem_.object_group() = header[4];
	uint8_t wbk = mem_[rWBK]; // push_wram
	mem_[rWBK] = LEVEL_OBJECTS_WRAM;
	routines_.farcall(BANK_LoadObjectGroupData, ADDR_LoadObjectGroupData);
	mem_[rWBK] = wbk; // pop_wram

	mem_.animated_tiles_group() = header[5];
	routines_.farcall(BANK_InitRoomAnimatedTiles, ADDR_InitRoomAnimatedTiles);
	mem_.room_pal_cycle() = header[6];
	routines_.farcall(BANK_InitRoomAnimatedPals, ADDR_InitRoomAnimatedPals);
	mem_.room() = header[7];
	routines_.farcall(BANK_LoadRoomGfx, ADDR_LoadRoomGfx);
	switch_bank(rom);

	// the camera's limits: $ff where a limit of 0 wraps to 0
	auto limit = [](uint8_t blocks, int shift, uint8_t base) {
		return static_cast<uint8_t>((blocks << shift) + base);
	};
	uint8_t lower = limit(mem_.room_lower_y_limit(), 4, 0xa0);
	mem_.unk_c0c4() = lower ? lower : 0xff;
	mem_.unk_c0c5() = limit(mem_.room_upper_y_limit(), 4, 0xa0);
	uint8_t right = limit(mem_.room_right_x_limit(), 3, 0xb0);
	mem_.unk_c0c7() = right ? right : 0xff;
	mem_.unk_c0c6() = limit(mem_.room_left_x_limit(), 3, 0xb0);
}

} // namespace

} // namespace wl3::home

using wl3::home::RoomLoader;

PORT_EX(Func_298d, 0, ALL_REGISTERS) { RoomLoader(gb).load_block_tiles(); }
PORT_EX(LoadRoomTileMap, 0, ALL_REGISTERS) { RoomLoader(gb).load_tile_map(); }
PORT_EX(LoadRoomMainTiles, 0, ALL_REGISTERS) { RoomLoader(gb).load_main_tiles(); }
PORT_EX(LoadRoomSpecialTiles, 0, ALL_REGISTERS) { RoomLoader(gb).load_special_tiles(); }
PORT_EX(LoadRoomPalettes, 0, ALL_REGISTERS) { RoomLoader(gb).load_palettes(); }
PORT_EX(UpdateRoomAnimatedPals, 0, ALL_REGISTERS) { wl3::home::update_room_animated_pals(gb); }
PORT_EX(LoadBlockFunctionTable, 0, ALL_REGISTERS) { RoomLoader(gb).load_block_function_table(); }
PORT_EX(LoadRoom, 0, ALL_REGISTERS) { RoomLoader(gb).load(); }

namespace wl3::home {
void update_room_animated_pals(GB *gb) { RoomLoader(gb).update_animated_pals(); }
} // namespace wl3::home
