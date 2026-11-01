// The font and screen loaders and the number printers; see font_counts.hpp.
#include "font_counts.hpp"

#include "../home/memory.hpp"
#include "../home/pals.hpp"

namespace wl3::engine {

void load_font_tiles(GB *gb)
{
	home::decompress(gb, ADDR_FontGfx, v0Tiles0);
}

void load_font_pals(GB *gb)
{
	home::load_pals_to_temp_pals1(gb, ADDR_Pals_1f3610);
	home::load_pals_to_temp_pals2(gb, ADDR_Pals_1f3610);
}

void load_language_selection_text(GB *gb)
{
	Memory mem(gb);
	mem[rVBK] = BANK_v1BGMap0; // BANK("VRAM1")
	home::decompress(gb, ADDR_LanguageSelection2TextMap, v1BGMap0);
	mem[rVBK] = 0;
	home::decompress(gb, ADDR_LanguageSelection1TextMap, v0BGMap0);
}

void load_gb_incompatible_screen_gfx(GB *gb)
{
	home::decompress(gb, ADDR_GBIncompatibleGfx, v0Tiles0);
}

void load_gb_incompatible_screen_map(GB *gb)
{
	home::decompress(gb, ADDR_GBIncompatibleTilemap, v0BGMap0);
	Memory(gb).byte(rBGP) = 0xe4; // %11100100
}

namespace {

void print_bcd3(GB *gb, uint16_t dest, uint16_t number)
{
	Memory mem(gb);
	uint8_t digits[3] = {static_cast<uint8_t>(mem[number] & 0x0f),
		static_cast<uint8_t>(mem[static_cast<uint16_t>(number + 1)] >> 4),
		static_cast<uint8_t>(mem[static_cast<uint16_t>(number + 1)] & 0x0f)};
	for (uint8_t digit : digits) {
		uint8_t tile = static_cast<uint8_t>(0xa0 + digit * 2);
		mem[dest] = tile;
		mem[static_cast<uint16_t>(dest + TILEMAP_WIDTH)] = static_cast<uint8_t>(tile + 1);
		dest++;
	}
}

} // namespace

void print_number_coins(GB *gb, uint16_t dest)
{
	print_bcd3(gb, dest, wNumCoins);
}

void print_num_collected_treasures(GB *gb, uint16_t dest)
{
	print_bcd3(gb, dest, wNumCollectedTreasures);
}

} // namespace wl3::engine

// Only C++ modules call these now (through in_bank).
PORT_COVERED(LoadFontTiles);
PORT_COVERED(LoadFontPals);
PORT_COVERED(LoadLanguageSelectionText);
PORT_COVERED(LoadGBIncompatibleScreenGfx);
PORT_COVERED(LoadGBIncompatibleScreenMap);
PORT_COVERED(PrintNumberCoins);
PORT_COVERED(PrintNumCollectedTreasures);
