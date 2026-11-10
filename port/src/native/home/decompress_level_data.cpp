// The level's block and object maps (src/home/decompress_level_data.asm):
// decompressing them into SRAM, and a block's place in them and in the BG
// map.
//
// Plain C++ over memory (level_map.hpp). The decompressions are called
// only by LoadLevelBlockMapAndObjects, which reads no registers after, so
// their hooks clobber them all; GetBlockPtr and GetBlockPtrBGMapAddress
// leave the asm's registers for their register-level callers.
#include "level_map.hpp"

#include "../flags.hpp"

namespace wl3::home {

namespace {

// a coordinate's block: its low 12 bits / $10 ($XWYZ -> $WY), as swap /
// and $0f / add
uint8_t block_coord(uint8_t hi, uint8_t lo)
{
	return static_cast<uint8_t>((hi & 0x0f) << 4 | lo >> 4);
}

void sram_switch(Memory mem, uint8_t bank)
{
	mem.sram_bank() = bank;
	mem[rRAMB + 0x100] = bank;
}

void rom_switch(GB *gb, uint8_t bank, uint16_t routine)
{
	Memory mem(gb);
	uint16_t pc = gb->cpu.pc;
	gb->cpu.pc = routine; // home code
	mem.rom_bank() = bank;
	mem[rROMB0 + 0x100] = bank;
	gb->cpu.pc = pc;
}

// the RLE data at wCompressedLevelBlockMapPtr (in the ROM bank wTempBank)
// into the block map, or the object map (objmap): each command repeats
// its next byte (under $80) or copies the bytes after it (from $80: the
// count in the low 7 bits, 0 being 256), up to a 0 command. The maps are
// row by row, each row a page and the pages of SRAM across its banks;
// the object map's rows start at $b0, 2 blocks per byte.
void decompress_level_map(GB *gb, bool objmap, uint16_t routine)
{
	Memory mem(gb);
	uint8_t sram = mem.sram_bank();
	sram_switch(mem, BANK_sLevelBlockObjectMap); // BANK("SRAM1")
	uint8_t rom = mem.rom_bank();
	rom_switch(gb, mem.temp_bank(), routine);

	RomPtr src(gb, mem.temp_bank(), static_cast<uint16_t>(mem[wCompressedLevelBlockMapPtr + 0] << 8
		| mem[wCompressedLevelBlockMapPtr + 1]));
	auto next = [&] {
		uint8_t v = *src;
		++src;
		return v;
	};
	uint8_t row_end = objmap ? (LEVEL_WIDTH >> 1) + 0xb0 - 1 : LEVEL_WIDTH;
	uint8_t row_start = objmap ? 0xb0 : 0x00;
	uint16_t dest = static_cast<uint16_t>(sLevelBlockObjectMap + row_start);
	// the block map steps de first and then compares e; the object map
	// compares first and steps only within the row
	auto put = [&](uint8_t v) {
		mem[dest] = v;
		if (!objmap)
			dest++;
		if (static_cast<uint8_t>(dest) == row_end) {
			dest = static_cast<uint16_t>(((dest & 0xff00) + 0x100) | row_start); // the next row
			if (dest >> 8 == 0xc0) { // past SRAM: the next bank's
				dest = static_cast<uint16_t>((0xa000) | row_start);
				sram_switch(mem, static_cast<uint8_t>(mem.sram_bank() + 1));
			}
		} else if (objmap) {
			dest++;
		}
	};
	while (uint8_t cmd = next()) {
		if (cmd & 0x80) {
			int n = (cmd & 0x7f) ? (cmd & 0x7f) : 256;
			while (n--)
				put(next());
		} else {
			mem.repeat_byte() = next();
			while (cmd--)
				put(mem.repeat_byte());
		}
	}
	rom_switch(gb, rom, routine);
	sram_switch(mem, sram);
}

} // namespace

BlockPtr block_ptr(GB *gb, uint16_t pos, uint8_t *x)
{
	Memory mem(gb);
	// the block map spans several SRAM banks of $20 pages each
	uint8_t y_hi = mem[pos], y_lo = mem[pos + 1];
	uint8_t page = static_cast<uint8_t>(block_coord(y_hi, y_lo) + (sLevelBlockObjectMap >> 8));
	uint8_t bank = BANK_sLevelBlockObjectMap;
	for (int i = 0; i < 2 && page >= 0xc0; i++) { // HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
		bank++;
		page = static_cast<uint8_t>(page - 0x20);
	}
	mem.block_pointer().hi() = page;
	mem.block_ptr_bank() = bank;
	// (x is read after the writes above, as the asm does)
	uint8_t x_hi = mem[pos + 2], x_lo = mem[pos + 3];
	if (x) {
		x[0] = x_hi;
		x[1] = x_lo;
	}
	uint8_t column = block_coord(x_hi, x_lo);
	mem.block_pointer().lo() = column;
	return {static_cast<uint16_t>(page << 8 | column), bank};
}

// each row of blocks is 2 rows of the BG map ($40 bytes), each column 2
// tiles, 16 columns wide
uint16_t block_bg_map_address(GB *gb, uint16_t ptr)
{
	uint16_t row = static_cast<uint16_t>((static_cast<uint8_t>((ptr >> 8) - 0xa0) << 6) & 0x3ff);
	uint16_t bg = static_cast<uint16_t>(((v0BGMap0 >> 8) << 8 | ((ptr & 0x0f) * 2)) + row);
	Memory mem(gb);
	mem.bg_pointer() = bg;
	return bg;
}

} // namespace wl3::home

using namespace wl3::home;

// hl = the block pointer; a = h, b = the bank, d = x's high byte and f
// as the x block's add
PORT(GetBlockPtr)
{
	uint8_t x[2];
	BlockPtr block = block_ptr(gb, REG_HL(gb), x);
	uint8_t x_hi = x[0], x_lo = x[1];
	CPU &cpu = gb->cpu;
	SET_HL(gb, block.ptr);
	cpu.a = static_cast<uint8_t>(block.ptr >> 8);
	cpu.b = block.bank;
	cpu.d = x_hi;
	cpu.f = wl3::flags::add(static_cast<uint8_t>((x_hi & 0x0f) << 4), x_lo >> 4);
}

PORT_EX(DecompressLevelBlockMap, 0, ALL_REGISTERS)
{
	if (!(wl3::Memory(gb).unk_ceef() & 0x3c))
		decompress_level_map(gb, false, ADDR_DecompressLevelBlockMap);
}

PORT_EX(DecompressLevelObjectMap, 0, ALL_REGISTERS)
{
	decompress_level_map(gb, true, ADDR_DecompressLevelObjectMap);
}

// hl = the BG map address, a = l, de = the row's offset, f as add hl, de's
// (z from add a)
PORT(GetBlockPtrBGMapAddress)
{
	uint16_t ptr = REG_HL(gb);
	uint16_t bg = block_bg_map_address(gb, ptr);
	uint16_t row = static_cast<uint16_t>((static_cast<uint8_t>((ptr >> 8) - 0xa0) << 6) & 0x3ff);
	uint8_t column = static_cast<uint8_t>((ptr & 0x0f) * 2);
	CPU &cpu = gb->cpu;
	uint16_t base = static_cast<uint16_t>((v0BGMap0 & 0xff00) | column);
	cpu.f = static_cast<uint8_t>(wl3::flags::z(column) | ((base & 0xfff) + (row & 0xfff) > 0xfff ? FLAG_H : 0)
		| (base + row > 0xffff ? FLAG_C : 0));
	SET_DE(gb, row);
	SET_HL(gb, bg);
	cpu.a = static_cast<uint8_t>(bg);
}
