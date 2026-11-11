// Home's overworld helpers (src/home/overworld.asm): pointer tables, an
// OW object's movement, its tile in wTilemap, the level names' and the
// HDMA's parameters, the BG map and palette configs.
//
// Plain functions (ow.hpp) for C++ modules; the hooks wrap them for the
// asm's callers, leaving the registers and flags the asm leaves.
#include "ow.hpp"

#include "../flags.hpp"
#include "memory.hpp"

namespace wl3::home {

uint16_t ow_coord_in_tilemap(GB *gb, uint16_t pos)
{
	Memory mem(gb);
	uint8_t y = mem[pos] & 0xf8;
	uint8_t rows = static_cast<uint8_t>(y << 2 | y >> 6); // rlca / rlca: y / 8 * 32
	uint8_t x = mem[pos + 1] & 0xf8;
	x = static_cast<uint8_t>(x << 1 | x >> 7); // rlca
	x = static_cast<uint8_t>(x << 4 | x >> 4); // swap: x / 8
	uint8_t lo = static_cast<uint8_t>(x + (rows & 0xf0));
	mem[wOWTilemapPtr + 0] = lo;
	uint8_t hi = static_cast<uint8_t>((rows & 0x0f) + (wTilemap >> 8));
	mem[wOWTilemapPtr + 1] = hi;
	return static_cast<uint16_t>(hi << 8 | lo);
}

uint16_t load_level_name(GB *gb, uint8_t side, uint8_t index)
{
	Memory mem(gb);
	mem.hdma_bank() = static_cast<uint8_t>(BANK_LevelNamesJPGfx + mem.language());
	mem.hdma() = 0;
	uint8_t side_hi = static_cast<uint8_t>(side << 4 | side >> 4); // swap: *$10
	uint16_t src = static_cast<uint16_t>(ADDR_LevelNamesJPGfx + (side_hi << 8)); // aka LevelNamesENGfx
	src = static_cast<uint16_t>(src + (static_cast<uint8_t>(index * 2) << 8));
	mem.unk_2d0b5_source_hi() = static_cast<uint8_t>(src >> 8);
	mem.unk_2d0b5_source_lo() = static_cast<uint8_t>(src);
	mem.unk_2d0b5_dest_hi() = 0x15;
	mem.unk_2d0b5_dest_lo() = 0;
	mem.unk_2d0b5_mode() = 32 - 1; // 32 dma_tiles
	return src;
}

void start_hdma(GB *gb)
{
	Memory mem(gb);
	mem[rVBK] = BANK_v1Tiles0; // BANK("VRAM1")
	for (int i = 0; i < 5; i++)
		mem[static_cast<uint16_t>(rVDMA_SRC_HIGH + i)] = mem[static_cast<uint16_t>(w2d0b5 + i)];
	mem.hdma_bank() = 0;
}

void load_far_bg_map(GB *gb, uint8_t bank, uint16_t src)
{
	Memory mem(gb);
	uint8_t rom = mem.rom_bank();
	bankswitch(gb, bank, ADDR_LoadFarBGMap);
	copy(gb, RomPtr(gb, bank, src), v0BGMap0, 18 * TILEMAP_WIDTH);
	bankswitch(gb, rom, ADDR_LoadFarBGMap);
}

PalConfigEnd apply_pal_config(GB *gb, uint16_t config)
{
	Memory mem(gb);
	uint8_t reg = mem[config];
	mem[config] = 0;
	mem[static_cast<uint16_t>(0xff00 | reg)] = mem[config + 1];
	uint8_t data = static_cast<uint8_t>(reg + 1);
	int blocks = mem[config + 2] ? mem[config + 2] : 256;
	uint16_t src = static_cast<uint16_t>(mem[config + 3] << 8 | mem[config + 4]);
	uint8_t last = 0;
	for (int i = 0; i < blocks * 8; i++)
		mem[static_cast<uint16_t>(0xff00 | data)] = last = mem[src++];
	return {src, data, last};
}

OWMovement apply_ow_movement(GB *gb, uint16_t obj, uint16_t table, bool mirrored, int code_bank)
{
	Memory mem = Memory::in(gb, code_bank);
	// the step counter at obj | 7 indexes the table's (y, x) offsets
	Byte counter = mem[static_cast<uint16_t>(obj | 7)];
	uint8_t step = counter;
	counter = static_cast<uint8_t>(step + 1);
	uint16_t entry = static_cast<uint16_t>(table + step * 2);
	uint8_t dy = mem[entry], dx = mem[entry + 1];
	mem[obj] = static_cast<uint8_t>(mem[obj] + dy);
	uint16_t x = static_cast<uint16_t>((obj & 0xff00) | static_cast<uint8_t>(obj + 1)); // inc c
	mem[x] = static_cast<uint8_t>(mirrored ? mem[x] - dx : mem[x] + dx);
	OWMovement end = {x, static_cast<uint16_t>(entry + 2), static_cast<uint8_t>(dy), static_cast<uint8_t>(dx), 0, false};
	end.next = mem[end.entry];
	if (end.next == 0x80) { // the end: reset the counter
		end.x = static_cast<uint16_t>(x | 7);
		mem[end.x] = 0;
		end.finished = true;
	}
	return end;
}

} // namespace wl3::home

using namespace wl3::home;
namespace flags = wl3::flags;
using wl3::Memory;
using wl3::Word;
using wl3::Byte;

// a = the treasure: a = TRUE and carry if collected, else z. Leaves hl at
// its byte, d = its bit and e = 1 or $10 (bit 2's nybble)
PORT(IsTreasureCollected)
{
	CPU &cpu = gb->cpu;
	uint8_t treasure = cpu.a;
	uint16_t at = static_cast<uint16_t>(wTreasuresCollected + (treasure >> 3));
	cpu.e = (treasure & 4) ? 0x10 : 1;
	cpu.d = static_cast<uint8_t>(cpu.e << (treasure & 3));
	SET_HL(gb, at);
	bool collected = is_treasure_collected(gb, treasure);
	cpu.a = collected ? TRUE : 0;
	cpu.f = collected ? FLAG_C : (FLAG_Z | FLAG_H); // scf, or and's
}

// hl += a * 2: de = a * 2, a = its carry, f as add hl, de's (z from adc)
static void add_offset(GB *gb, uint8_t index)
{
	CPU &cpu = gb->cpu;
	uint16_t offset = static_cast<uint16_t>(index * 2);
	uint16_t hl = REG_HL(gb);
	unsigned sum = hl + offset;
	SET_DE(gb, offset);
	cpu.a = static_cast<uint8_t>(offset >> 8);
	cpu.f = static_cast<uint8_t>(flags::z(cpu.a) | ((hl & 0xfff) + offset > 0xfff ? FLAG_H : 0)
		| (sum > 0xffff ? FLAG_C : 0));
	SET_HL(gb, sum);
}

PORT(AddOffsetInPointerTable) { add_offset(gb, gb->cpu.a); }

// ld a, [hli] / ld h, [hl] / ld l, a (or to de)
PORT(GetPointerFromTableHL)
{
	add_offset(gb, gb->cpu.a);
	uint16_t ptr = Word(gb, REG_HL(gb));
	gb->cpu.a = static_cast<uint8_t>(ptr);
	SET_HL(gb, ptr);
}

PORT(GetPointerFromTableDE)
{
	add_offset(gb, gb->cpu.a);
	uint16_t entry = REG_HL(gb);
	uint16_t ptr = Word(gb, entry);
	gb->cpu.a = static_cast<uint8_t>(ptr);
	SET_DE(gb, ptr);
	SET_HL(gb, entry + 1);
}

PORT(GetCthWordFromAthTable)
{
	add_offset(gb, gb->cpu.a);
	SET_HL(gb, static_cast<uint16_t>(Word(gb, REG_HL(gb))));
	add_offset(gb, gb->cpu.c);
}

PORT(GetByteFromPointerInHL)
{
	uint16_t ptr = Word(gb, REG_HL(gb));
	SET_HL(gb, ptr);
	gb->cpu.a = Memory(gb)[ptr];
}

// hl = y, x: hl = their tile; b, c = the row's offset, a = h, f as the
// add of h
PORT(GetOWCoordInTilemap)
{
	uint16_t pos = REG_HL(gb);
	uint8_t y = Memory(gb)[pos] & 0xf8;
	uint16_t tile = ow_coord_in_tilemap(gb, pos);
	uint8_t rows = static_cast<uint8_t>(y << 2 | y >> 6);
	CPU &cpu = gb->cpu;
	cpu.b = rows & 0x0f;
	cpu.c = rows & 0xf0;
	SET_HL(gb, tile);
	cpu.a = static_cast<uint8_t>(tile >> 8);
	cpu.f = flags::add(cpu.b, static_cast<uint8_t>(wTilemap >> 8));
}

// b = the map side, d = the level's index in it
PORT(LoadLevelName)
{
	CPU &cpu = gb->cpu;
	uint16_t src = load_level_name(gb, cpu.b, cpu.d);
	cpu.b = static_cast<uint8_t>(cpu.b << 4 | cpu.b >> 4);
	cpu.c = 0;
	cpu.d = static_cast<uint8_t>(cpu.d * 2);
	cpu.e = 0;
	SET_HL(gb, src);
	cpu.a = 32 - 1;
	cpu.f = FLAG_Z; // xor a
}

// a = the value for [bc]; if [hl] is $80, also clears [bc | 7]
PORT(Func_3c29)
{
	CPU &cpu = gb->cpu;
	Memory mem(gb);
	mem[REG_BC(gb)] = cpu.a;
	cpu.a = mem[REG_HL(gb)];
	cpu.f = flags::cp(cpu.a, 0x80);
	if (cpu.a != 0x80)
		return;
	cpu.c |= 0x07;
	cpu.a = 0;
	cpu.f = FLAG_Z; // xor a
	mem[REG_BC(gb)] = 0;
}

// hl = struct, de = table of (y, x) offsets: adds the y offset of the
// counter's entry (the counter at [hl | 7]) to [hl]; bc = the x, a = it,
// de = the offsets, hl = the next entry, f as inc c's
PORT(Func_3c35)
{
	CPU &cpu = gb->cpu;
	Memory mem(gb);
	uint16_t obj = REG_HL(gb);
	Byte counter = mem[static_cast<uint16_t>(obj | 7)];
	uint8_t step = counter;
	counter = static_cast<uint8_t>(step + 1);
	uint16_t entry = static_cast<uint16_t>(REG_DE(gb) + step * 2);
	cpu.d = mem[entry];
	cpu.e = mem[entry + 1];
	SET_HL(gb, entry + 2);
	uint8_t y = mem[obj];
	mem[obj] = static_cast<uint8_t>(y + cpu.d);
	cpu.b = static_cast<uint8_t>(obj >> 8);
	cpu.f = flags::inc(flags::add(y, cpu.d), static_cast<uint8_t>(obj));
	cpu.c = static_cast<uint8_t>(obj + 1);
	cpu.a = mem[REG_BC(gb)];
}

PORT(Func_3c03)
{
	start_hdma(gb);
	CPU &cpu = gb->cpu;
	SET_HL(gb, w2d0b5 + 5);
	cpu.c = static_cast<uint8_t>(rVDMA_SRC_HIGH + 4);
	cpu.a = 0;
	cpu.f = FLAG_Z; // xor a
}

// hl = tilemap, b = its bank: leaves CopyHLToDE_BC's hl, de and bc, and
// pop af's a and f (the bank it came from, the entry flags)
PORT(LoadFarBGMap)
{
	uint8_t rom = Memory(gb).rom_bank();
	uint16_t src = REG_HL(gb);
	load_far_bg_map(gb, gb->cpu.b, src);
	SET_HL(gb, src + 18 * TILEMAP_WIDTH);
	SET_DE(gb, v0BGMap0 + 18 * TILEMAP_WIDTH);
	SET_BC(gb, 0);
	gb->cpu.a = rom;
}

// hl = pal config: a = the last byte, b = 0, c = the data register, f as
// the last dec b's (carry clear from the xor a)
PORT(ApplyPalConfig)
{
	PalConfigEnd end = apply_pal_config(gb, REG_HL(gb));
	CPU &cpu = gb->cpu;
	SET_HL(gb, end.src);
	cpu.b = 0;
	cpu.c = end.data_reg;
	cpu.a = end.last;
	cpu.f = FLAG_Z | FLAG_N;
}

// hl = OW obj, de = relative positions: z when the movement has finished
// (Func_3c35, then Func_3c29's registers)
static void ow_movement(GB *gb, bool mirrored)
{
	OWMovement end = apply_ow_movement(gb, REG_HL(gb), REG_DE(gb), mirrored);
	CPU &cpu = gb->cpu;
	SET_BC(gb, end.x);
	cpu.d = end.dy;
	cpu.e = end.dx;
	SET_HL(gb, end.entry);
	cpu.a = end.finished ? 0 : end.next;
	cpu.f = end.finished ? static_cast<uint8_t>(FLAG_Z) : flags::cp(end.next, 0x80);
}

PORT(ApplyOWMovement_Mirrored) { ow_movement(gb, true); }
PORT(ApplyOWMovement) { ow_movement(gb, false); }

// clears w2d0c0's $10 bytes, leaving WriteAToHL_BCTimes's registers
PORT(Func_3c76)
{
	CPU &cpu = gb->cpu;
	cpu.a = 0;
	cpu.f = FLAG_Z; // xor a
	SET_HL(gb, w2d0c0);
	SET_BC(gb, 0x10);
	gb_call(gb, ADDR_WriteAToHL_BCTimes);
}
