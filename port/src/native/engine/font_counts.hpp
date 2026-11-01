// The font and screen loaders (src/engine/load_font.asm) and the number
// printers (print_counts.asm) the menus share, as plain functions; their
// callers map the bank (Routines::in_bank, as the original's farcall).
#pragma once

#include "../native.hpp"

namespace wl3::engine {

void load_font_tiles(GB *gb); // LoadFontTiles: to v0Tiles0 (or VRAM1's)
void load_font_pals(GB *gb); // LoadFontPals: to wTempPals1 and 2
void load_language_selection_text(GB *gb); // LoadLanguageSelectionText
void load_gb_incompatible_screen_gfx(GB *gb); // LoadGBIncompatibleScreenGfx
void load_gb_incompatible_screen_map(GB *gb); // LoadGBIncompatibleScreenMap: with rBGP

// Three BCD digits, each two tiles high (tile $a0 + 2 * digit over the
// next), from dest: wNumCoins (PrintNumberCoins) or wNumCollectedTreasures
// (PrintNumCollectedTreasures)
void print_number_coins(GB *gb, uint16_t dest);
void print_num_collected_treasures(GB *gb, uint16_t dest);

} // namespace wl3::engine
