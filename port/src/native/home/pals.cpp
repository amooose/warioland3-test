// Home's palette routines; see pals.hpp.
#include "pals.hpp"

#include "memory.hpp"

namespace wl3::home {

namespace {

constexpr uint8_t PALS_SIZE = 8 * PAL_SIZE;

uint8_t apply_pals(GB *gb, uint16_t src, uint16_t index_reg, uint8_t index, uint16_t data_reg)
{
	Memory mem(gb);
	mem[index_reg] = index;
	uint8_t last = 0;
	for (int i = 0; i < PALS_SIZE; i++)
		mem[data_reg] = last = mem[src++];
	return last;
}

uint8_t store_pals(GB *gb, uint16_t dest, uint16_t index_reg, uint16_t data_reg)
{
	Memory mem(gb);
	uint8_t last = 0;
	for (uint8_t i = 0; i < PALS_SIZE; i++) {
		mem[index_reg] = i;
		mem[dest++] = last = mem[data_reg];
	}
	return last;
}

// wait_not_hblank / wait_hblank
void wait_hblank(GB *gb)
{
	Memory mem(gb);
	while (!(mem[rSTAT] & STAT_MODE))
		idle(gb, 32);
	while (mem[rSTAT] & STAT_MODE)
		idle(gb, 32);
}

// a palette from src to rOBPD, in the next H-Blank
RomPtr apply_ob_pal_in_hblank(GB *gb, RomPtr src)
{
	Memory mem(gb);
	wait_hblank(gb);
	for (int i = 0; i < PAL_SIZE; i++, ++src)
		mem[rOBPD] = *src;
	return src;
}

// The level's treasure (wLevelEndScreen's) index, and its palette in
// TreasurePals by the colour wLevelTreasurePals gives it; with WRAM bank
// wLevelTreasurePals mapped.
struct TreasurePal {
	uint8_t index;
	uint16_t pals;
};

TreasurePal treasure_pal(GB *gb)
{
	Memory mem(gb);
	uint8_t index = static_cast<uint8_t>((mem.level_end_screen() & 0x7f) - 1);
	uint8_t colour = mem[static_cast<uint16_t>(wLevelTreasurePals + index)];
	uint8_t offset = static_cast<uint8_t>((colour - OBPAL_TREASURE_YELLOW) * 8);
	return {index, static_cast<uint16_t>(ADDR_TreasurePals + offset)};
}

} // namespace

uint8_t copy_short(GB *gb, uint16_t src, uint16_t dest, int count)
{
	Memory mem(gb);
	if (!count)
		count = 256;
	uint8_t page = dest >> 8, low = static_cast<uint8_t>(dest);
	uint8_t last = 0;
	while (count--)
		mem[static_cast<uint16_t>(page << 8 | low++)] = last = mem[src++];
	return last;
}

uint8_t copy_short(GB *gb, RomPtr src, uint16_t dest, int count)
{
	Memory mem(gb);
	if (!count)
		count = 256;
	uint8_t page = dest >> 8, low = static_cast<uint8_t>(dest);
	uint8_t last = 0;
	while (count--) {
		mem[static_cast<uint16_t>(page << 8 | low++)] = last = *src;
		++src;
	}
	return last;
}

uint8_t apply_bg_pals(GB *gb, uint16_t src)
{
	return apply_pals(gb, src, rBGPI, BGPI_AUTOINC | 0, rBGPD);
}

uint8_t apply_ob_pals(GB *gb, uint16_t src)
{
	return apply_pals(gb, src, rOBPI, OBPI_AUTOINC | 0, rOBPD);
}

uint8_t store_bg_pals(GB *gb, uint16_t dest)
{
	return store_pals(gb, dest, rBGPI, rBGPD);
}

uint8_t store_ob_pals(GB *gb, uint16_t dest)
{
	return store_pals(gb, dest, rOBPI, rOBPD);
}

void load_pals_to_temp_pals1(GB *gb, uint16_t pals)
{
	copy_short(gb, pals, wTempPals1, PALS_SIZE);
}

void load_pals_to_temp_pals2(GB *gb, uint16_t pals)
{
	copy_short(gb, pals, wTempPals2, PALS_SIZE);
}

void load_obj_pals(GB *gb, uint16_t pals)
{
	copy_short(gb, pals, wTempPals2 + 4 * PAL_SIZE, 4 * PAL_SIZE);
}

void load_pals_to_temp_pals1(GB *gb, RomPtr pals)
{
	copy_short(gb, pals, wTempPals1, PALS_SIZE);
}

void load_pals_to_temp_pals2(GB *gb, RomPtr pals)
{
	copy_short(gb, pals, wTempPals2, PALS_SIZE);
}

void load_obj_pals(GB *gb, RomPtr pals)
{
	copy_short(gb, pals, wTempPals2 + 4 * PAL_SIZE, 4 * PAL_SIZE);
}

void set_wario_pal(GB *gb, uint16_t pals)
{
	Memory mem(gb);
	mem[wWarioPalsPtr + 0] = static_cast<uint8_t>(pals >> 8);
	mem[wWarioPalsPtr + 1] = static_cast<uint8_t>(pals);
	uint8_t bank = mem.rom_bank();
	bankswitch(gb, BANK_WarioDefaultPal, ADDR_SetWarioPal); // BANK("Wario Palettes")
	mem.call_func_bank() = BANK_WarioDefaultPal;
	RomPtr src(gb, BANK_WarioDefaultPal, pals);
	hcall(gb, ADDR_CopyHLToDE_Short, [&] { copy_short(gb, src, wTempPals2, 2 * PAL_SIZE); });
	mem[rOBPI] = OBPI_AUTOINC | 0;
	for (int i = 0; i < 2; i++)
		src = apply_ob_pal_in_hblank(gb, src);
	bankswitch(gb, bank, ADDR_SetWarioPal);
}

void load_collected_treasure_pal_clear_screen(GB *gb)
{
	Memory mem(gb);
	uint8_t wram_bank = mem[rWBK];
	mem[rWBK] = BANK_wLevelTreasurePals;
	TreasurePal pal = treasure_pal(gb);
	uint8_t bank = mem.rom_bank();
	bankswitch(gb, BANK_TreasurePals, ADDR_LoadCollectedTreasurePal_ClearScreen);
	// an unneeded farcall, the bank being mapped
	farcall(gb, BANK_TreasurePals, ADDR_CopyHLToDE_Short,
		[&] { copy_short(gb, RomPtr(gb, BANK_TreasurePals, pal.pals), wTempPals2 + 7 * PAL_SIZE, PAL_SIZE); });
	bankswitch(gb, bank, ADDR_LoadCollectedTreasurePal_ClearScreen);
	mem[rWBK] = wram_bank;
}

void load_collected_treasure_pal_level(GB *gb)
{
	Memory mem(gb);
	uint8_t wram_bank = mem[rWBK];
	mem[rWBK] = BANK_wLevelTreasurePals;
	TreasurePal pal = treasure_pal(gb);
	uint8_t bank = mem.rom_bank();
	bankswitch(gb, BANK_TreasurePals, ADDR_LoadCollectedTreasurePal_Level);
	uint8_t offset = static_cast<uint8_t>(pal.index * 8);
	uint16_t dest = static_cast<uint16_t>(((wTempPals2 + 4 * PAL_SIZE) & 0xff00)
		| static_cast<uint8_t>((wTempPals2 + 4 * PAL_SIZE) + offset));
	farcall(gb, BANK_TreasurePals, ADDR_CopyHLToDE_Short, [&] { copy_short(gb, RomPtr(gb, BANK_TreasurePals, pal.pals), dest, PAL_SIZE); });
	mem[rOBPI] = static_cast<uint8_t>(OBPI_AUTOINC | (4 * PAL_SIZE) | offset);
	apply_ob_pal_in_hblank(gb, RomPtr(gb, BANK_TreasurePals, pal.pals));
	bankswitch(gb, bank, ADDR_LoadCollectedTreasurePal_Level);
	mem[rWBK] = wram_bank;
}

// the palette index register at palette `first` (times 8, in a byte),
// auto-incrementing
static void select_pal(GB *gb, bool bg, uint8_t first)
{
	Memory mem(gb);
	mem[bg ? rBGPI : rOBPI] = static_cast<uint8_t>((first * 8) | BGPI_AUTOINC);
}

uint16_t set_pals(GB *gb, bool bg, uint16_t src, uint8_t count, uint8_t first, uint8_t *last, int code_bank)
{
	Memory mem = Memory::in(gb, code_bank);
	select_pal(gb, bg, first);
	uint8_t v = 0;
	for (int i = 0; i < (count ? count : 256); i++) {
		wait_hblank(gb);
		for (int j = 0; j < PAL_SIZE; j++)
			mem[bg ? rBGPD : rOBPD] = v = mem[src++];
	}
	if (last)
		*last = v;
	return src;
}

void black_out_pals(GB *gb, bool bg, uint8_t count, uint8_t first)
{
	Memory mem(gb);
	select_pal(gb, bg, first);
	for (int i = 0; i < (count ? count : 256); i++) {
		wait_hblank(gb);
		for (int j = 0; j < PAL_SIZE; j++)
			mem[bg ? rBGPD : rOBPD] = 0;
	}
}

void copy_and_apply_ob_pals(GB *gb, uint16_t src, uint16_t dest, uint8_t count, uint8_t first, int code_bank)
{
	Memory mem(gb);
	RomPtr from = code_bank < 0 ? RomPtr::mapped(gb, src) : RomPtr(gb, static_cast<uint8_t>(code_bank), src);
	copy_short(gb, from, dest, static_cast<uint8_t>(count * 8));
	mem[rOBPI] = static_cast<uint8_t>((first * 8) | OBPI_AUTOINC);
	for (int i = 0; i < (count ? count : 256); i++)
		from = apply_ob_pal_in_hblank(gb, from);
}

} // namespace wl3::home

namespace {

using namespace wl3::home;

// the flags of the asm's final dec b: z and n, carry kept
uint8_t dec_b_flags(GB *gb)
{
	return static_cast<uint8_t>((gb->cpu.f & FLAG_C) | FLAG_Z | FLAG_N);
}

// after a CopyHLToDE_Short of n bytes from src to dest
void copied_short(GB *gb, uint16_t src, uint16_t dest, int n)
{
	SET_HL(gb, static_cast<uint16_t>(src + n));
	gb->cpu.d = dest >> 8;
	gb->cpu.e = static_cast<uint8_t>(dest + n);
	gb->cpu.b = 0;
}

// after 8 palettes to or from a data register
void pals_done(GB *gb, uint16_t hl, uint8_t data_reg, uint8_t last)
{
	SET_HL(gb, static_cast<uint16_t>(hl + 8 * PAL_SIZE));
	gb->cpu.b = 0;
	gb->cpu.c = data_reg;
	gb->cpu.a = last;
}

void apply_regs(GB *gb, uint16_t src, bool ob)
{
	uint8_t last = ob ? apply_ob_pals(gb, src) : apply_bg_pals(gb, src);
	pals_done(gb, src, static_cast<uint8_t>(ob ? rOBPD : rBGPD), last);
	gb->cpu.f = dec_b_flags(gb);
}

void store_regs(GB *gb, uint16_t dest, bool ob)
{
	uint8_t last = ob ? store_ob_pals(gb, dest) : store_bg_pals(gb, dest);
	pals_done(gb, dest, static_cast<uint8_t>(ob ? rOBPD : rBGPD), last);
	gb->cpu.e = 8 * PAL_SIZE;
	gb->cpu.f = FLAG_Z | FLAG_N; // xor a cleared carry; the final dec b
}

void copy_short_regs(GB *gb, uint16_t dest, int n)
{
	uint16_t src = REG_HL(gb);
	gb->cpu.a = copy_short(gb, src, dest, n);
	copied_short(gb, src, dest, n ? n : 256);
	gb->cpu.f = dec_b_flags(gb);
}

} // namespace

PORT_EX(LoadBGPalettesFromWRAM, 0, 0)
{
	apply_regs(gb, wTempBGPals, false);
}

PORT_EX(LoadOBPalettesFromWRAM, 0, 0)
{
	apply_regs(gb, wTempOBPals, true);
}

PORT_EX(FillWhiteBGPal, 0, 0)
{
	apply_regs(gb, ADDR_PalsWhite, false);
}

PORT_EX(FillWhiteOBPal, 0, 0)
{
	apply_regs(gb, ADDR_PalsWhite, true);
}

PORT_EX(ApplyTempPals1ToBGPals, 0, 0)
{
	apply_regs(gb, wTempPals1, false);
}

PORT_EX(ApplyTempPals2ToOBPals, 0, 0)
{
	apply_regs(gb, wTempPals2, true);
}

PORT_EX(StoreBGPals, 0, 0)
{
	store_regs(gb, wTempBGPals, false);
}

PORT_EX(StoreOBPals, 0, 0)
{
	store_regs(gb, wTempOBPals, true);
}

PORT_EX(LoadPalsToTempPals1, 0, 0)
{
	copy_short_regs(gb, wTempPals1, 8 * PAL_SIZE);
}

PORT_EX(LoadPalsToTempPals2, 0, 0)
{
	copy_short_regs(gb, wTempPals2, 8 * PAL_SIZE);
}

PORT_EX(LoadObjPals, 0, 0)
{
	copy_short_regs(gb, wTempPals2 + 4 * PAL_SIZE, 4 * PAL_SIZE);
}

PORT_EX(CopyHLToDE_Short, 0, 0)
{
	copy_short_regs(gb, REG_DE(gb), gb->cpu.b);
}

// unreferenced: white to wTempBGPals and wTempOBPals
PORT_EX(Func_1ae2, 0, 0)
{
	copy(gb, ADDR_PalsWhite, wTempBGPals, 8 * PAL_SIZE);
	gb->cpu.a = copy(gb, ADDR_PalsWhite, wTempBGPals + 8 * PAL_SIZE, 8 * PAL_SIZE);
	SET_HL(gb, ADDR_PalsWhite + 8 * PAL_SIZE);
	SET_DE(gb, wTempBGPals + 16 * PAL_SIZE);
	gb->cpu.b = 0;
	gb->cpu.f = dec_b_flags(gb);
}

// a and f are popped from the entry's push (the bank it came from)
PORT_EX(SetWarioPal, 0, 0)
{
	uint16_t pals = REG_HL(gb);
	uint8_t bank = wl3::Memory(gb).rom_bank();
	set_wario_pal(gb, pals);
	copied_short(gb, pals, wTempPals2, 2 * PAL_SIZE);
	gb->cpu.c = static_cast<uint8_t>(rOBPD);
	gb->cpu.a = bank;
}

// The registers both leave, a and f popped by pop_wram (the WRAM bank it
// read, the entry's flags).
static void treasure_pal_regs(GB *gb, uint16_t dest, uint8_t c)
{
	wl3::Memory mem(gb);
	uint8_t wram_bank = mem[rWBK];
	mem[rWBK] = BANK_wLevelTreasurePals;
	uint8_t index = static_cast<uint8_t>((mem.level_end_screen() & 0x7f) - 1);
	uint8_t colour = mem[static_cast<uint16_t>(wLevelTreasurePals + index)];
	mem[rWBK] = wram_bank;
	uint16_t pals = static_cast<uint16_t>(ADDR_TreasurePals + static_cast<uint8_t>((colour - OBPAL_TREASURE_YELLOW) * 8));
	if (c == 0xff) // the level: the index's palette, c the data register
		dest = static_cast<uint16_t>((dest & 0xff00) | static_cast<uint8_t>(dest + index * 8));
	copied_short(gb, pals, dest, PAL_SIZE);
	gb->cpu.c = c == 0xff ? static_cast<uint8_t>(rOBPD) : index;
	gb->cpu.a = wram_bank;
}

PORT_EX(LoadCollectedTreasurePal_ClearScreen, 0, 0)
{
	treasure_pal_regs(gb, wTempPals2 + 7 * PAL_SIZE, 0);
	load_collected_treasure_pal_clear_screen(gb);
}

PORT_EX(LoadCollectedTreasurePal_Level, 0, 0)
{
	treasure_pal_regs(gb, wTempPals2 + 4 * PAL_SIZE, 0xff);
	load_collected_treasure_pal_level(gb);
}

// b = palettes, c = the first OB palette, hl = source, de = destination
PORT_EX(CopyAndApplyOBPals, 0, 0)
{
	uint16_t src = REG_HL(gb), dest = REG_DE(gb);
	uint8_t count = gb->cpu.b, first = gb->cpu.c;
	copy_and_apply_ob_pals(gb, src, dest, count, first);
	int bytes = static_cast<uint8_t>(count * 8);
	gb->cpu.e = static_cast<uint8_t>(dest + (bytes ? bytes : 256));
	uint16_t end = static_cast<uint16_t>(src + (count ? count : 256) * PAL_SIZE);
	SET_HL(gb, end);
	gb->cpu.a = wl3::Memory(gb)[static_cast<uint16_t>(end - 1)];
	gb->cpu.b = 0;
	gb->cpu.c = static_cast<uint8_t>(rOBPD);
	gb->cpu.f = FLAG_Z | FLAG_N; // or a cleared carry; the final dec b
}
