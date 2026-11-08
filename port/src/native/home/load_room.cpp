// Loading a room (src/home/load_room.asm): its header (spawn, limits,
// camera, objects, animated tiles and palettes), its graphics' tables,
// tiles and palettes, and its palette cycle each frame.
//
// Plain C++ over memory. The callers are C (StartRoom, LoadRoomGfx, the
// game loop) and read no registers after, so the hooks clobber them all.
// The debug asserts jump to Init with the asm's push af still on the
// stack.
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
		mem_[wTempBank] = bank;
		uint8_t rom = mem_[wROMBank];
		switch_bank(bank);
		copy(gb_, src, dest, count);
		switch_bank(rom);
	}
	void switch_bank(uint8_t bank) { bankswitch(gb_, bank, gb_->cpu.pc); }
	// debug_assert_not $ff: Init, with the asm's push af (a = the ROM
	// bank it came from) on the stack
	void reset(uint8_t rom, uint8_t f)
	{
		gb_push16(gb_, static_cast<uint16_t>(rom << 8 | f));
		gb_->cpu.pc = ADDR_Init;
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
	uint16_t src = pointer(ADDR_RoomBlockTilesPointers, mem_[wRoomBlockFunctionTable]);
	far_copy(wRoomBlockFunctionTableBank, src, wRoomBlockTiles, 4 * NUM_ROOM_BLOCK_TYPES);
}

void RoomLoader::load_tile_map()
{
	uint16_t src = pointer(ADDR_RoomBlockAttributePointers, mem_[wRoomTileMap]);
	uint8_t bank = mem_[wc0d0];
	mem_[wTempBank] = bank;
	farcall(gb_, bank, ADDR_Decompress, [&] { decompress(gb_, src, wRoomBlockAttributes); });
}

void RoomLoader::load_main_tiles()
{
	uint16_t src = pointer(ADDR_LevelMainTilesPointers, mem_[wRoomMainTiles]);
	far_copy(wRoomMainTilesBank, src, v0Tiles2, 0x800);
}

void RoomLoader::load_special_tiles()
{
	mem_[rVBK] = 1;
	uint16_t src = pointer(ADDR_SpecialTilesPointers, mem_[wRoomSpecialTiles]);
	far_copy(wRoomSpecialTilesBank, src, v0Tiles2, 0x800);
	mem_[rVBK] = 0;
}

void RoomLoader::load_palettes()
{
	uint16_t src = pointer(ADDR_RoomPals, mem_[wRoomPalettes]);
	uint8_t bank = mem_[wPaletteBank];
	mem_[wTempBank] = bank;
	farcall(gb_, bank, ADDR_LoadPalsToTempPals1, [&] { load_pals_to_temp_pals1(gb_, src); });
}

// the room's palette cycle: when its duration has passed, the next of its
// palettes into the BG palettes, 12 bytes per H-Blank (only with at least
// 8 scanlines to V-Blank)
void RoomLoader::update_animated_pals()
{
	if (mem_[rLY] >= LY_VBLANK - 8)
		return;
	uint8_t duration = mem_[wRoomPalCycleDuration];
	if (!duration)
		return;
	uint8_t count = static_cast<uint8_t>(mem_[wRoomPalCycleCounter] + 1);
	mem_[wRoomPalCycleCounter] = count;
	if (count < duration)
		return;
	mem_[wRoomPalCycleCounter] = 0;
	uint16_t cycle = static_cast<uint16_t>(mem_[wRoomPalCyclePtr + 0] << 8 | mem_[wRoomPalCyclePtr + 1]);
	uint8_t index = static_cast<uint8_t>(mem_[wRoomPalCycleIndex] + 1);
	if (index >= ROOM_PALCYCLE_LENGTH)
		index = 0;
	mem_[wRoomPalCycleIndex] = index;

	uint8_t rom = mem_[wROMBank];
	switch_bank(BANK_RoomPalCycles);
	uint8_t pal = mem_[static_cast<uint16_t>(cycle + index)];
	uint16_t pals = Word(gb_, static_cast<uint16_t>(ADDR_RoomPals + pal * 2));
	switch_bank(rom);

	switch_bank(LEVEL_PALETTES);
	mem_[rBGPI] = BGPI_AUTOINC | 0;
	for (int i = 0; i < 5; i++) {
		wait_next_hblank();
		for (int j = 0; j < 12; j++)
			mem_[rBGPD] = mem_[pals++];
	}
	wait_next_hblank();
	for (int j = 0; j < 4; j++)
		mem_[rBGPD] = mem_[pals++];
	switch_bank(rom);
}

// the room's block functions: from its table in group 1 or 2's bank
void RoomLoader::load_block_function_table()
{
	uint8_t table = mem_[wRoomBlockFunctionTable];
	uint8_t offset = static_cast<uint8_t>(table * 2);
	uint16_t at = static_cast<uint16_t>(ADDR_BlockFunctionTablePointers + offset);
	uint8_t rom = mem_[wROMBank];
	switch_bank(BANK_BlockFunctionTablePointers);
	uint16_t src = Word(gb_, at);
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
	copy(gb_, src, wBlockFunctionTable, 0x100);
	if (group_2)
		switch_bank(BANK_BlockFunctionTablePointers);
	switch_bank(rom);
	gb_->cpu.pc = gb_pop16(gb_); // ret
}

// a level's header: its rooms' headers in bank $30 or $31
void RoomLoader::load()
{
	mem_[wPrevWarioScreenYPos] = 0x70;
	mem_[wCamRightSpacing] = 0x58;
	mem_[wCamLeftSpacing] = 0x58; // $58 - ($58 - $58)

	uint8_t rom = mem_[wROMBank];
	switch_bank(BANK_LevelHeaders);
	uint8_t level = mem_[wLevel];
	uint16_t rooms = Word(gb_, static_cast<uint16_t>(ADDR_LevelHeaders + level * 2));
	if (rooms >> 8 == 0xff) {
		reset(rom, 0); // the flags of add $58
		return;
	}
	switch_bank(rom);

	level = mem_[wLevel];
	rom = mem_[wROMBank];
	switch_bank(level < THE_GRASSLANDS_NIGHT_1 ? 0x30 : 0x31);
	uint16_t header = Word(gb_, static_cast<uint16_t>(rooms + static_cast<uint8_t>(mem_[wLevelRoomID] * 2)));
	if (header >> 8 == 0xff) {
		reset(rom, flags::cp(level, THE_GRASSLANDS_NIGHT_1));
		return;
	}
	mem_[wWarioSpawnPos] = mem_[header + 0];
	uint8_t y_limits = mem_[header + 1];
	mem_[wRoomUpperYLimit] = y_limits >> 4;
	mem_[wRoomLowerYLimit] = y_limits & 0x0f;
	uint8_t x_limits = mem_[header + 2];
	mem_[wRoomLeftXLimit] = x_limits >> 4;
	mem_[wRoomRightXLimit] = x_limits & 0x0f;
	mem_[wCameraConfigFlags] = mem_[header + 3];

	mem_[wObjectGroup] = mem_[header + 4];
	uint8_t wbk = mem_[rWBK]; // push_wram
	mem_[rWBK] = LEVEL_OBJECTS_WRAM;
	routines_.farcall(BANK_LoadObjectGroupData, ADDR_LoadObjectGroupData);
	mem_[rWBK] = wbk; // pop_wram

	mem_[wAnimatedTilesGroup] = mem_[header + 5];
	routines_.farcall(BANK_InitRoomAnimatedTiles, ADDR_InitRoomAnimatedTiles);
	mem_[wRoomPalCycle] = mem_[header + 6];
	routines_.farcall(BANK_InitRoomAnimatedPals, ADDR_InitRoomAnimatedPals);
	mem_[wRoom] = mem_[header + 7];
	routines_.farcall(BANK_LoadRoomGfx, ADDR_LoadRoomGfx);
	switch_bank(rom);

	// the camera's limits: $ff where a limit of 0 wraps to 0
	auto limit = [](uint8_t blocks, int shift, uint8_t base) {
		return static_cast<uint8_t>((blocks << shift) + base);
	};
	uint8_t lower = limit(mem_[wRoomLowerYLimit], 4, 0xa0);
	mem_[wc0c4] = lower ? lower : 0xff;
	mem_[wc0c5] = limit(mem_[wRoomUpperYLimit], 4, 0xa0);
	uint8_t right = limit(mem_[wRoomRightXLimit], 3, 0xb0);
	mem_[wc0c7] = right ? right : 0xff;
	mem_[wc0c6] = limit(mem_[wRoomLeftXLimit], 3, 0xb0);
	gb_->cpu.pc = gb_pop16(gb_); // ret
}

} // namespace

} // namespace wl3::home

using wl3::home::RoomLoader;

PORT_EX(Func_298d, 0, ALL_REGISTERS) { RoomLoader(gb).load_block_tiles(); }
PORT_EX(LoadRoomTileMap, 0, ALL_REGISTERS) { RoomLoader(gb).load_tile_map(); }
PORT_EX(LoadRoomMainTiles, 0, ALL_REGISTERS) { RoomLoader(gb).load_main_tiles(); }
PORT_EX(LoadRoomSpecialTiles, 0, ALL_REGISTERS) { RoomLoader(gb).load_special_tiles(); }
PORT_EX(LoadRoomPalettes, 0, ALL_REGISTERS) { RoomLoader(gb).load_palettes(); }
PORT_EX(UpdateRoomAnimatedPals, 0, ALL_REGISTERS) { RoomLoader(gb).update_animated_pals(); }
PORT_EX(LoadBlockFunctionTable, HOOK_MANUAL_RET, ALL_REGISTERS) { RoomLoader(gb).load_block_function_table(); }
PORT_EX(LoadRoom, HOOK_MANUAL_RET, ALL_REGISTERS) { RoomLoader(gb).load(); }
