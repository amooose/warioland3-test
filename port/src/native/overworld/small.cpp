// Small map routines (day_night.asm, crayon_flags.asm, init.asm,
// best_times/common.asm) the overworld modules call; see overworld.hpp.
#include "overworld.hpp"

#include "../home/memory.hpp"
#include "../home/system.hpp"

namespace wl3::overworld {

void switch_day_night(GB *gb)
{
	Memory mem(gb);
	mem[wDayNight] = static_cast<uint8_t>(mem[wDayNight] ^ (1 << 4));
}

// the East side stays by day without the Sun Medallion
void do_day_night_spell(GB *gb)
{
	Memory mem(gb);
	if (mem[wCurMapSide] == EAST && !mem[wGotSunMedallion]) {
		mem[wDayNight] = static_cast<uint8_t>(mem[wDayNight] & ~(1 << 4));
		return;
	}
	switch_day_night(gb);
}

// the crayons' bits of wTreasuresCollected (they span two bytes)
void get_crayon_flags(GB *gb)
{
	Memory mem(gb);
	uint16_t hi = wTreasuresCollected + CRAYON_YELLOW_T / 8;
	uint8_t flags = static_cast<uint8_t>(mem[hi] << 2 | mem[static_cast<uint16_t>(hi - 1)] >> 6);
	mem[wCrayonFlags] = flags & 0x7f;
}

// Func_80bd9: no BG priority for the tile at (0, 14) and the 3x4 block
// at (0, 15) of wAttrmap
void clear_bottom_left_bg_priority(GB *gb)
{
	Memory mem(gb);
	auto clear = [&](uint16_t at) { mem[at] = static_cast<uint8_t>(mem[at] & ~(1 << B_BG_PRIO)); };
	clear(wAttrmap + 14 * TILEMAP_WIDTH);
	for (int row = 15; row < 19; row++)
		for (int col = 0; col < 3; col++)
			clear(static_cast<uint16_t>(wAttrmap + row * TILEMAP_WIDTH + col));
}

// Func_803e6: back from a level (not an exit), the save (or the perfect
// screen)
void save_after_level(GB *gb)
{
	Memory mem(gb);
	if (!mem[wTransitionParam] || mem[wOWBarsState] || mem[wMapSideLevelID] >= OW_EXITS)
		return;
	home::save_game_or_enter_perfect_screen(gb);
}

// Func_80de0: start in time attack opens the best times
void check_best_times(GB *gb)
{
	Memory mem(gb);
	if ((mem[wGameModeFlags] & (1 << MODE_TIME_ATTACK_F)) && (mem[wJoypadPressed] & (1 << B_PAD_START)))
		mem[wSubState] = SST_OVERWORLD_BEST_TIMES;
}

// DrawCoinCount (bank26.asm): wNumCoins's three BCD digits as tiles, from
// NumbersGfx (two tiles a digit) to v0Tiles1 tile $0a on
void draw_coin_count(GB *gb)
{
	Memory mem(gb);
	uint8_t digits[3] = {static_cast<uint8_t>(mem[wNumCoins + 0] & 0x0f),
		static_cast<uint8_t>(mem[wNumCoins + 1] >> 4), static_cast<uint8_t>(mem[wNumCoins + 1] & 0x0f)};
	uint16_t dest = v0Tiles1 + 0xa * TILE_SIZE;
	for (uint8_t digit : digits) {
		home::copy(gb, static_cast<uint16_t>(ADDR_NumbersGfx + digit * 0x20), dest, 0x20);
		dest += 0x20;
	}
}

// GetLevelThatContainsTreasure (bank26.asm): LevelTreasureIDs has 4 a
// level; searched until found
uint8_t level_with_treasure(GB *gb, uint8_t treasure)
{
	Memory mem(gb);
	uint16_t at = ADDR_LevelTreasureIDs;
	uint8_t index = 0xff;
	do
		index++;
	while (mem[at++] != treasure);
	return index >> 2;
}

} // namespace wl3::overworld

// Only the overworld modules (C++) call these.
PORT_COVERED(SwitchDayNight);
PORT_COVERED(DoDayNightSpell);
PORT_COVERED(GetCrayonFlags);
PORT_COVERED(Func_80bd9);
PORT_COVERED(Func_803e6);
PORT_COVERED(Func_80de0);
PORT_COVERED(DrawCoinCount);
PORT_COVERED(GetLevelThatContainsTreasure);
