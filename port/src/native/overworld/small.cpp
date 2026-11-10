// Small map routines (day_night.asm, crayon_flags.asm, init.asm,
// best_times/common.asm) the overworld modules call; see overworld.hpp.
#include "overworld.hpp"

#include "../cutscenes/cutscenes.hpp"
#include "../home/memory.hpp"
#include "../routines.hpp"
#include "../home/system.hpp"

namespace wl3::overworld {

void switch_day_night(GB *gb)
{
	Memory mem(gb);
	mem.day_night() = static_cast<uint8_t>(mem.day_night() ^ (1 << 4));
}

// the East side stays by day without the Sun Medallion
void do_day_night_spell(GB *gb)
{
	Memory mem(gb);
	if (mem.cur_map_side() == EAST && !mem.got_sun_medallion()) {
		mem.day_night() = static_cast<uint8_t>(mem.day_night() & ~(1 << 4));
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
	mem.crayon_flags() = flags & 0x7f;
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
	if (!mem.transition_param() || mem.ow_bars_state() || mem.map_side_level_id() >= OW_EXITS)
		return;
	home::save_game_or_enter_perfect_screen(gb);
}

// Func_80de0: start in time attack opens the best times
void check_best_times(GB *gb)
{
	Memory mem(gb);
	if ((mem.game_mode_flags() & (1 << MODE_TIME_ATTACK_F)) && (mem.joypad_pressed() & (1 << B_PAD_START)))
		mem.sub_state() = SST_OVERWORLD_BEST_TIMES;
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
		home::copy(gb, ROM(gb, NumbersGfx) + (digit * 0x20), dest, 0x20);
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

// CheckIfEventHasCutscene (cutscene/init.asm): wCurEvent to
// wEventWithCutscene if its CutsceneInitFunctions entry isn't
// NoInitCutscene, else 0
void check_if_event_has_cutscene(GB *gb)
{
	Memory mem(gb);
	uint8_t event = mem.cur_event();
	mem.event_with_cutscene() = 0;
	uint16_t init = ROM(mem.gb(), CutsceneInitFunctions).word(event * 2);
	if (init != ADDR_NoInitCutscene)
		mem.event_with_cutscene() = event;
}

// _InitCutscene (cutscene/init.asm): the screen off and cleared, the
// cutscene's V-Blank function and init, then the LCD back on (with the
// flags the cutscene asked flipped)
void init_cutscene(GB *gb)
{
	Memory mem(gb);
	home::disable_lcd(gb);
	home::clear_virtual_oam(gb);
	stop_music2(mem);
	home::fill(gb, wLCDCFlagsToFlip, 0, 0x30);
	home::copy(gb, ROM(gb, VBlank_Cutscene) + 0x0c, wVBlankFunc, 0x51); // VBlank_Cutscene
	cutscenes::Cutscenes(gb).init(); // InitEventCutscene
	mem[rSCY] = 0;
	mem.scy() = 0;
	mem[rSCX] = 0;
	mem.scx() = 0;
	mem.unk_2d801() = 0;
	mem.unk_2d013() = 0;
	mem.unk_2d014() = 0;
	mem.pal_config1_total_steps() = 0;
	mem.pal_config2_total_steps() = 0;
	mem[rLCDC] = static_cast<uint8_t>((LCDC_BG_ON | LCDC_OBJ_ON | LCDC_OBJ_16 | LCDC_WIN_9C00 | LCDC_ON) ^ mem.lcdc_flags_to_flip());
	mem.lcdc_flags_to_flip() = 0;
	++mem.sub_state();
}

// SetObjPositionToLevelCoords (treasure_collection/common.asm): a scene
// object's y and x at a level of the current map side (inc e between them)
void set_obj_position_to_level_coords(GB *gb, uint16_t obj, uint8_t level)
{
	Memory mem(gb);
	uint16_t coords = map_level_coords(gb, level);
	mem[obj] = mem[coords];
	mem[static_cast<uint16_t>((obj & 0xff00) | static_cast<uint8_t>(obj + 1))] = mem[static_cast<uint16_t>(coords + 1)];
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
PORT_COVERED(CheckIfEventHasCutscene);
PORT_COVERED(InitCutscene);
PORT_COVERED(_InitCutscene);
PORT_COVERED(SetObjPositionToLevelCoords);
PORT_COVERED(GetMapLevelCoords);
