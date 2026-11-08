// Loading a level's block and object maps (src/home/load_level.asm), and
// reloading only the objects' bits of the block map.
//
// Plain C++ over memory. The callers are C (StartRoom) and read no
// registers after, so the hooks clobber them all. A level without maps
// ($ff) jumps to InvalidLevelData with the asm's push af on the stack.
#include "../native.hpp"
#include "../routines.hpp"

namespace wl3::home {

namespace {

#define ADDR_RLO_OnlyHighBit (ADDR_ReloadLevelObjects + 0x38) // .DecompressLevelBlockMap_OnlyHighBit
#define ADDR_RLO_Decompress (ADDR_ReloadLevelObjects + 0x71)  // .Decompress

class LevelLoader {
public:
	explicit LevelLoader(GB *gb) : gb_(gb), mem_(gb) {}

	void load(); // LoadLevelBlockMapAndObjects
	void reload_objects(); // ReloadLevelObjects
	void reload_objects_bits(); // .DecompressLevelBlockMap_OnlyHighBit
	void or_objects_bits(uint16_t src); // .Decompress

private:
	void sram_switch(uint8_t bank)
	{
		mem_[wSRAMBank] = bank;
		mem_[rRAMB + 0x100] = bank;
	}
	void rom_switch(uint8_t bank)
	{
		mem_[wROMBank] = bank;
		mem_[rROMB0 + 0x100] = bank;
	}
	bool maps(uint8_t &rom, uint16_t &header);
	uint16_t read_block_map_header(uint16_t header);

	GB *gb_;
	Memory mem_;
};

// header = the level's maps header, with its bank mapped (rom: the bank
// before); false after the jump to InvalidLevelData for a level without
bool LevelLoader::maps(uint8_t &rom, uint16_t &header)
{
	uint8_t offset = static_cast<uint8_t>(mem_[wLevel] * 2);
	bool d = mem_[wLevel] & 0x80; // add a's carry, rl d
	uint16_t table = static_cast<uint16_t>(ADDR_LevelBlockAndObjectMaps + (d ? 0x100 : 0));
	uint16_t at = static_cast<uint16_t>(table + offset);
	rom = mem_[wROMBank];
	rom_switch(BANK_LevelBlockAndObjectMaps);
	header = Word(gb_, at);
	if (header >> 8 != 0xff)
		return true;
	// push af saved rl d's z and add hl, de's h and c
	unsigned de = (d ? 0x100u : 0u) | offset;
	uint8_t f = static_cast<uint8_t>((d ? 0 : FLAG_Z)
		| ((ADDR_LevelBlockAndObjectMaps & 0xfff) + (de & 0xfff) > 0xfff ? FLAG_H : 0)
		| (ADDR_LevelBlockAndObjectMaps + de > 0xffff ? FLAG_C : 0));
	gb_push16(gb_, static_cast<uint16_t>(rom << 8 | f));
	gb_->cpu.pc = ADDR_InvalidLevelData;
	return false;
}

// the block map's pointer (big-endian) and bank; returns where the object
// map's header starts (the block map's bank byte is also its bank)
uint16_t LevelLoader::read_block_map_header(uint16_t header)
{
	mem_[wCompressedLevelBlockMapPtr + 1] = mem_[header + 0];
	mem_[wCompressedLevelBlockMapPtr + 0] = mem_[header + 1];
	mem_[wTempBank] = mem_[header + 2];
	return static_cast<uint16_t>(header + 2);
}

void LevelLoader::load()
{
	uint8_t rom;
	uint16_t header;
	if (!maps(rom, header))
		return;
	header = read_block_map_header(header);
	Routines(gb_).call(ADDR_DecompressLevelBlockMap);
	mem_[wTempBank] = mem_[header + 0];
	mem_[wCompressedLevelBlockMapPtr + 1] = mem_[header + 1];
	mem_[wCompressedLevelBlockMapPtr + 0] = mem_[header + 2];
	rom_switch(rom);
	if (!(mem_[wceef] & 0x3c))
		Routines(gb_).call(ADDR_DecompressLevelObjectMap);
	gb_->cpu.pc = gb_pop16(gb_); // ret
}

void LevelLoader::reload_objects()
{
	uint8_t rom;
	uint16_t header;
	if (!maps(rom, header))
		return;
	read_block_map_header(header);
	rom_switch(rom);
	reload_objects_bits();
	gb_->cpu.pc = gb_pop16(gb_); // ret
}

// almost identical to DecompressLevelBlockMap, but only ors in the
// blocks' bit 7 (their objects)
void LevelLoader::reload_objects_bits()
{
	if (mem_[wceef] & 0x3c)
		return;
	uint8_t sram = mem_[wSRAMBank];
	sram_switch(BANK_sLevelBlockObjectMap); // BANK("SRAM1")
	uint8_t rom = mem_[wROMBank];
	rom_switch(mem_[wTempBank]);
	or_objects_bits(static_cast<uint16_t>(mem_[wCompressedLevelBlockMapPtr + 0] << 8
		| mem_[wCompressedLevelBlockMapPtr + 1]));
	rom_switch(rom);
	sram_switch(sram);
}

void LevelLoader::or_objects_bits(uint16_t src)
{
	uint16_t map = sLevelBlockObjectMap;
	auto put = [&](uint8_t bit) {
		mem_[map] = static_cast<uint8_t>(mem_[map] | bit);
		map++;
		if (static_cast<uint8_t>(map) != LEVEL_WIDTH)
			return;
		map = static_cast<uint16_t>((map & 0xff00) + 0x100); // the next row
		if (map >> 8 == 0xc0) { // past SRAM: the next bank's
			map = 0xa000; // STARTOF(SRAM)
			sram_switch(static_cast<uint8_t>(mem_[wSRAMBank] + 1));
		}
	};
	while (uint8_t cmd = mem_[src]) {
		if (!(cmd & 0x80)) {
			src++;
			mem_[wRepeatByte] = mem_[src++] & 0x80;
			while (cmd--)
				put(mem_[wRepeatByte]);
		} else {
			int n = (cmd & 0x7f) ? (cmd & 0x7f) : 256;
			src++;
			while (n--)
				put(mem_[src++] & 0x80);
		}
	}
}

} // namespace

} // namespace wl3::home

using wl3::home::LevelLoader;

PORT_EX(InvalidLevelData, HOOK_MANUAL_RET, ALL_REGISTERS) { gb->cpu.pc = ADDR_Init; }
PORT_EX(LoadLevelBlockMapAndObjects, HOOK_MANUAL_RET, ALL_REGISTERS) { LevelLoader(gb).load(); }
PORT_EX(ReloadLevelObjects, HOOK_MANUAL_RET, ALL_REGISTERS) { LevelLoader(gb).reload_objects(); }
PORT_LOCAL_EX(ReloadLevelObjects, DecompressLevelBlockMap_OnlyHighBit, 0, ALL_REGISTERS)
{
	LevelLoader(gb).reload_objects_bits();
}
// de = the compressed data
PORT_LOCAL_EX(ReloadLevelObjects, Decompress, 0, ALL_REGISTERS) { LevelLoader(gb).or_objects_bits(REG_DE(gb)); }
