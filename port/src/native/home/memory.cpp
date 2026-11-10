// Home's memory routines; see memory.hpp.
#include "memory.hpp"

namespace wl3::home {

void bankswitch(GB *gb, uint8_t bank, uint16_t home_routine)
{
	uint16_t pc = gb->cpu.pc;
	gb->cpu.pc = home_routine;
	Memory mem(gb);
	mem.rom_bank() = bank;
	mem[rROMB0 + 0x100] = bank;
	gb->cpu.pc = pc;
}

void idle(GB *gb, int cycles)
{
	if (!gb->no_tick)
		gb_tick(gb, cycles);
}

void fill(GB *gb, uint16_t dest, uint8_t value, uint32_t count)
{
	Memory mem(gb);
	while (count--)
		mem[dest++] = value;
}

uint8_t copy(GB *gb, uint16_t src, uint16_t dest, uint32_t count)
{
	Memory mem(gb);
	uint8_t last = 0;
	while (count--)
		mem[dest++] = last = mem[src++];
	return last;
}

void far_copy(GB *gb, uint16_t src, uint16_t dest, uint32_t count)
{
	Memory mem(gb);
	uint8_t bank = mem.rom_bank();
	bankswitch(gb, mem.temp_bank(), ADDR_FarCopyHLToDE_BC2);
	copy(gb, src, dest, count);
	bankswitch(gb, bank, ADDR_FarCopyHLToDE_BC2);
}

DecompressEnd decompress(GB *gb, uint16_t src, uint16_t dest)
{
	Memory mem(gb);
	DecompressEnd end = {0, 0, false, -1};
	while (uint8_t cmd = mem[src++]) {
		if (cmd & 0x80) {
			int n = (cmd & 0x7f) ? (cmd & 0x7f) : 256;
			while (n--)
				mem[dest++] = mem[src++];
		} else {
			uint8_t value = mem[src++];
			while (cmd--)
				mem[dest++] = value;
			end.last_fill = value;
		}
		end.any = true;
	}
	end.src = src;
	end.dest = dest;
	return end;
}

DecompressEnd far_decompress(GB *gb, uint16_t src, uint16_t dest)
{
	Memory mem(gb);
	uint8_t bank = mem.rom_bank();
	bankswitch(gb, mem.temp_bank(), ADDR_FarDecompress);
	DecompressEnd end = decompress(gb, src, dest);
	bankswitch(gb, bank, ADDR_FarDecompress);
	return end;
}

void clear_virtual_oam(GB *gb)
{
	fill(gb, wVirtualOAM, 0, OAM_SIZE);
	Memory mem(gb);
	mem.virtual_oam_byte_size() = 0;
}

// A sprite at a time. As in the asm, a remainder under 4 would clear 256
// sprites; it never happens, sprites being added 4 bytes at a time.
int clear_unused_virtual_oam(GB *gb)
{
	Memory mem(gb);
	uint8_t used = mem.virtual_oam_byte_size();
	int sprites = 0;
	if (used < OAM_SIZE) {
		sprites = (OAM_SIZE - used) >> 2;
		if (!sprites)
			sprites = 256;
		fill(gb, static_cast<uint16_t>((wVirtualOAM & 0xff00) | used), 0, static_cast<uint32_t>(sprites * 4));
	}
	mem.virtual_oam_byte_size() = 0;
	return sprites;
}

void fill_bg_map0(GB *gb, uint8_t value)
{
	Memory mem(gb);
	mem[rVBK] = BANK_v1BGMap0;
	fill(gb, v1BGMap0, 0, v1BGMap1 - v1BGMap0);
	mem[rVBK] = 0;
	fill(gb, v0BGMap0, value, v0BGMap1 - v0BGMap0);
}

void clear_bg_map1(GB *gb)
{
	fill(gb, v0BGMap1, 0xff, v0End - v0BGMap1);
}

void clear_vram(GB *gb)
{
	Memory mem(gb);
	if (!mem.cgb())
		return;
	mem[rVBK] = BANK_v1Tiles0;
	fill(gb, 0x8000, 0, 0x2000);
	mem[rVBK] = 0;
	fill(gb, 0x8000, 0, 0x2000);
}

void clear_wram(GB *gb)
{
	Memory mem(gb);
	for (uint8_t bank = 1; bank < 8; bank++) {
		mem[rWBK] = bank;
		fill(gb, 0xd000, 0, 0x1000);
	}
	mem[rWBK] = 0;
}

// The hardware only lets the LCD off during V-Blank.
bool disable_lcd(GB *gb)
{
	Memory mem(gb);
	if (!(mem[rLCDC] & LCDC_ON))
		return false;
	uint8_t ie = mem[rIE];
	mem.temp_ie() = ie;
	mem[rIE] = static_cast<uint8_t>(ie & ~IE_VBLANK);
	while (mem[rLY] != LY_VBLANK + 1)
		idle(gb, 32);
	mem[rLCDC] = static_cast<uint8_t>(mem[rLCDC] & ~LCDC_ON);
	mem[rIF] = 0;
	mem[rIE] = mem.temp_ie();
	return true;
}

} // namespace wl3::home

namespace {

using namespace wl3::home;

// The flags the asm's final dec b leaves: z and n, carry kept.
uint8_t dec_b_flags(GB *gb)
{
	return static_cast<uint8_t>((gb->cpu.f & FLAG_C) | FLAG_Z | FLAG_N);
}

// a b count (0 is 256) or a bc count (0 is 65536)
uint32_t b_count(GB *gb)
{
	return gb->cpu.b ? gb->cpu.b : 0x100;
}

uint32_t bc_count(GB *gb)
{
	uint16_t bc = REG_BC(gb);
	return bc ? bc : 0x10000;
}

// hl and de after copying n bytes
void copied(GB *gb, uint16_t hl, uint16_t de, uint32_t n)
{
	SET_HL(gb, static_cast<uint16_t>(hl + n));
	SET_DE(gb, static_cast<uint16_t>(de + n));
}

void decompressed(GB *gb, const DecompressEnd &end)
{
	SET_HL(gb, end.src);
	SET_BC(gb, end.dest);
	if (end.any)
		gb->cpu.d = 0;
	if (end.last_fill >= 0)
		gb->cpu.e = static_cast<uint8_t>(end.last_fill);
}

} // namespace

PORT_EX(WriteAToHL_BTimes, 0, 0)
{
	uint16_t hl = REG_HL(gb);
	uint32_t n = b_count(gb);
	fill(gb, hl, gb->cpu.a, n);
	SET_HL(gb, static_cast<uint16_t>(hl + n));
	gb->cpu.b = 0;
	gb->cpu.f = dec_b_flags(gb);
}

PORT_EX(WriteAToHL_BCTimes, 0, 0)
{
	uint16_t hl = REG_HL(gb);
	uint32_t n = bc_count(gb);
	fill(gb, hl, gb->cpu.a, n);
	SET_HL(gb, static_cast<uint16_t>(hl + n));
	SET_BC(gb, 0);
	gb->cpu.f = dec_b_flags(gb); // push af / pop af keep the entry carry
}

PORT_EX(CopyHLToDE_BC, 0, 0)
{
	uint16_t hl = REG_HL(gb), de = REG_DE(gb);
	uint32_t n = bc_count(gb);
	gb->cpu.a = copy(gb, hl, de, n);
	copied(gb, hl, de, n);
	SET_BC(gb, 0);
	gb->cpu.f = FLAG_Z | FLAG_N; // and a clears carry
}

PORT_EX(CopyHLToDE, 0, 0)
{
	uint16_t hl = REG_HL(gb), de = REG_DE(gb);
	uint32_t n = b_count(gb);
	gb->cpu.a = copy(gb, hl, de, n);
	copied(gb, hl, de, n);
	gb->cpu.b = 0;
	gb->cpu.f = dec_b_flags(gb);
}

// FarCopyHLToDE_BC and FarCopyHLToDE_BC2 differ only in their loops; both
// pop the entry's af (a = the bank they came from, the entry's flags)
// before switching back.
static void far_copy_regs(GB *gb)
{
	uint8_t bank = wl3::Memory(gb).rom_bank();
	uint16_t hl = REG_HL(gb), de = REG_DE(gb);
	uint32_t n = bc_count(gb);
	far_copy(gb, hl, de, n);
	copied(gb, hl, de, n);
	SET_BC(gb, 0);
	gb->cpu.a = bank;
}

PORT_EX(FarCopyHLToDE_BC, 0, 0)
{
	far_copy_regs(gb);
}

PORT_EX(FarCopyHLToDE_BC2, 0, 0)
{
	far_copy_regs(gb);
}

PORT_EX(Decompress, 0, 0)
{
	decompressed(gb, decompress(gb, REG_HL(gb), REG_BC(gb)));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z | FLAG_H; // and a
}

PORT_EX(FarDecompress, 0, 0)
{
	uint8_t bank = wl3::Memory(gb).rom_bank();
	decompressed(gb, far_decompress(gb, REG_HL(gb), REG_BC(gb)));
	gb->cpu.a = bank; // pop af: the entry's flags too
}

PORT_EX(ClearVirtualOAM, 0, 0)
{
	clear_virtual_oam(gb);
	SET_HL(gb, wVirtualOAM + OAM_SIZE);
	gb->cpu.b = 0;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

PORT_EX(ClearUnusedVirtualOAM, 0, 0)
{
	uint8_t used = wl3::Memory(gb).virtual_oam_byte_size();
	int sprites = clear_unused_virtual_oam(gb);
	SET_HL(gb, static_cast<uint16_t>(((wVirtualOAM & 0xff00) | used) + sprites * 4));
	if (sprites)
		gb->cpu.b = 0;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

// FillBGMap0 with a, and the two that jump to it with their value
static void fill_bg_map0_regs(GB *gb)
{
	uint8_t value = gb->cpu.a;
	fill_bg_map0(gb, value);
	gb->cpu.d = value;
	gb->cpu.a = value;
	SET_HL(gb, v0BGMap1);
	SET_BC(gb, 0);
	gb->cpu.f = FLAG_Z | FLAG_N; // the second fill's dec b, after xor a
}

PORT_EX(FillBGMap0, 0, 0)
{
	fill_bg_map0_regs(gb);
}

PORT_EX(FillBGMap0_With7f, 0, 0)
{
	gb->cpu.a = 0x7f;
	fill_bg_map0_regs(gb);
}

PORT_EX(ClearBGMap0, 0, 0)
{
	gb->cpu.a = 0xff;
	fill_bg_map0_regs(gb);
}

PORT_EX(ClearBGMap1, 0, 0)
{
	clear_bg_map1(gb);
	gb->cpu.a = 0xff;
	SET_HL(gb, v0End);
	SET_BC(gb, 0);
	gb->cpu.f = dec_b_flags(gb);
}

PORT_EX(ClearVRAM, 0, 0)
{
	clear_vram(gb);
	gb->cpu.a = 0;
	if (!wl3::Memory(gb).cgb()) {
		gb->cpu.f = FLAG_Z | FLAG_H; // and a
		return;
	}
	SET_HL(gb, 0xa000);
	SET_BC(gb, 0);
	gb->cpu.f = FLAG_Z | FLAG_N; // the fill's dec b, after xor a
}

PORT_EX(ClearWRAM, 0, 0)
{
	clear_wram(gb);
	gb->cpu.e = 8;
	SET_HL(gb, 0xe000);
	SET_BC(gb, 0);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

PORT_EX(DisableLCD, 0, 0)
{
	uint8_t lcdc = wl3::Memory(gb)[rLCDC];
	if (!disable_lcd(gb)) {
		gb->cpu.a = lcdc;
		gb->cpu.f = static_cast<uint8_t>((gb->cpu.f & FLAG_C) | FLAG_Z | FLAG_H); // bit
		return;
	}
	gb->cpu.a = wl3::Memory(gb).temp_ie();
	gb->cpu.f = FLAG_Z; // xor a
}
