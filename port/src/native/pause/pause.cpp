// The pause menu; see pause.hpp.
#include "pause.hpp"

namespace wl3::pause {

namespace {

constexpr uint8_t VRAM1 = BANK_v1Tiles0;

} // namespace

void PauseMenu::run_state()
{
	uint8_t state = wram_.sub_state();
	switch (state) {
	case 0: routines_.call(ADDR_SlowFadeBGToWhite); return;
	case 1: init(); return;
	case 2: routines_.call(ADDR_DarkenBGToPal_Fast); return;
	case 3: update(); return;
	case 4: routines_.call(ADDR_SlowFadeBGToWhite); return;
	case 5: exit(); return;
	case 6: routines_.call(ADDR_DarkenBGToPal_Fast); return;
	case 7: routines_.call(ADDR_ReturnToPendingLevelState); return;
	}
	// the three ways to save: from the pause menu (the level in progress),
	// and the game's (SST_PAUSE_18 then a transition, SST_PAUSE_28 then a
	// reset)
	static constexpr uint8_t STARTS[3] = {SST_PAUSE_MENU_SAVE, SST_PAUSE_18, SST_PAUSE_28};
	static constexpr uint8_t SIZES[3] = {16, 16, 19};
	for (int i = 0; i < 3; i++) {
		if (state < STARTS[i] || state >= STARTS[i] + SIZES[i])
			continue;
		SaveScreen save(gb_);
		bool level = i == 0;
		switch (state - STARTS[i]) {
		case 0: case 4: routines_.call(ADDR_SlowFadeBGToWhite); return;
		case 1: save.init(!level); return;
		case 2: case 6: routines_.call(ADDR_DarkenBGToPal_Fast); return;
		case 3: save.fill_bar(); return;
		case 5:
			if (level)
				save.save_level();
			else
				save.save();
			return;
		case 7: save.handle_complete_box(); return;
		case 8:
			if (level)
				routines_.call(ADDR_Init); // ResetAfterPauseMenuSave
			else
				routines_.call(ADDR_SlowFadeBGToWhite);
			return;
		case 9:
			if (i == 1)
				save.transition_after_save();
			else if (i == 2)
				routines_.call(ADDR_Init); // ResetAfterSave
			else
				routines_.call(ADDR_DebugReset);
			return;
		default: routines_.call(ADDR_DebugReset); return;
		}
	}
	unrealistic();
}

void PauseMenu::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

void PauseMenu::draw(MenuObject obj)
{
	obj.load_sprite(ADDR_OAM_1f156c, false);
}

// ---------------------------------------------------------------------------
// pause_menu_1.asm

void PauseMenu::init()
{
	routines_.disable_lcd();
	routines_.save_backup_vram();
	wram_.temp_animated_tiles_frame_duration() = wram_.animated_tiles_frame_duration();
	wram_.temp_animated_tiles_group() = wram_.animated_tiles_gfx();
	init_menu();
}

void PauseMenu::return_from_action_help()
{
	routines_.disable_lcd();
	wram_.state() = ST_PAUSE_MENU;
	wram_.sub_state() = SST_PAUSE_INIT_MENU;
	init_menu();
}

void PauseMenu::init_menu()
{
	mem_[hSFXID + 0] = 0xff; // stop_sfx
	mem_[hSFXID + 1] = 0x00;
	play_music(mem_, MUSIC_PAUSE_MENU);
	wram_.pause_menu_selection() = PAUSEMENU_RETURN;
	wram_.room_animated_tiles_enabled() = 0;
	routines_.call(ADDR_ClearBGMap0);
	routines_.clear_virtual_oam();
	routines_.in_bank(BANK_LoadPauseMenuPals, ADDR_LoadPauseMenuPals, [&] {
		routines_.load_pals_to_temp_pals1(ADDR_Pals_1f4182);
		routines_.load_pals_to_temp_pals2(ADDR_Pals_1f41c2);
	});
	routines_.in_bank(BANK_LoadPauseMenuGfx, ADDR_LoadPauseMenuGfx, [&] { load_gfx(); });
	routines_.in_bank(BANK_Func_1f403f, ADDR_Func_1f403f, [&] { draw_maps(); });
	print_music_coins();
	SET_HL(gb_, v0BGMap1 + 15 * TILEMAP_WIDTH + 14);
	routines_.farcall(BANK_PrintNumberCoins, ADDR_PrintNumberCoins);
	routines_.call(ADDR_VBlank_PauseMenu);
	mem_[rSCY] = 0;
	mem_[rSCX] = 0;
	wram_.scy() = 0;
	wram_.scx() = 0;

	show(cursor(), 0x38, 0x20, 0, ADDR_Frameset_1f1cb8);
	show(object(wMenuObj3), 0x8c, 0x20, 0, ADDR_Frameset_1f1cd4);
	// the level's keys, and its treasures (found or not)
	static constexpr struct { int key, treasure; uint16_t key_obj, treasure_obj; uint8_t x, attributes;
		uint16_t key_frameset, got, not_got; } COLOURS[4] = {
		{GREY_KEY_F, GREY_TREASURE_F, wMenuObj4, wMenuObj8, 0x38, 0x03,
			ADDR_Frameset_1f1c4a, ADDR_Frameset_1f1cf0, ADDR_Frameset_1f1ced},
		{RED_KEY_F, RED_TREASURE_F, wMenuObj5, wMenuObj9, 0x48, 0x03,
			ADDR_Frameset_1f1c4d, ADDR_Frameset_1f1cf6, ADDR_Frameset_1f1cf3},
		{GREEN_KEY_F, GREEN_TREASURE_F, wMenuObj6, wMenuObj10, 0x58, 0x04,
			ADDR_Frameset_1f1c4a, ADDR_Frameset_1f1cf0, ADDR_Frameset_1f1ced},
		{BLUE_KEY_F, BLUE_TREASURE_F, wMenuObj7, wMenuObj11, 0x68, 0x04,
			ADDR_Frameset_1f1c4d, ADDR_Frameset_1f1cf6, ADDR_Frameset_1f1cf3},
	};
	for (const auto &c : COLOURS)
		if (wram_.key_and_treasure_flags().bit(c.key))
			show(object(c.key_obj), 0x54, c.x, c.attributes, c.key_frameset);
	for (const auto &c : COLOURS)
		show(object(c.treasure_obj), 0x64, c.x, c.attributes,
			wram_.key_and_treasure_flags().bit(c.treasure) ? c.got : c.not_got);

	mem_[rLCDC] = LCDC_ON | LCDC_BG_9C00 | LCDC_OBJ_16 | LCDC_OBJ_ON | LCDC_BG_ON;
	++wram_.sub_state();
}

void PauseMenu::show(MenuObject obj, uint8_t y, uint8_t x, uint8_t attributes, uint16_t frameset)
{
	obj.init(y, x, frameset);
	obj.attributes() = attributes;
	obj.animate();
	draw(obj);
}

void PauseMenu::update()
{
	handle_input();
	MenuObject box = object(wMenuObj3);
	box.animate();
	draw(box);
	static constexpr struct { int flag; uint16_t obj; } KEYS[4] = {
		{GREY_KEY_F, wMenuObj4}, {RED_KEY_F, wMenuObj5}, {GREEN_KEY_F, wMenuObj6}, {BLUE_KEY_F, wMenuObj7},
	};
	for (const auto &k : KEYS)
		if (wram_.key_and_treasure_flags().bit(k.flag))
			draw(object(k.obj));
	draw(object(wMenuObj8));
	draw(object(wMenuObj9));
	draw(object(wMenuObj10));
	draw(object(wMenuObj11));
	cursor().animate();
	draw(cursor());
	routines_.clear_unused_virtual_oam();

	// once the chosen option's animation is over
	Byte selection = wram_.pause_menu_selection();
	if (!selection.bit(PAUSEMENUF_B_PAD_SELECT) || !wram_.obj_anim_was_reset())
		return;
	switch (selection & 0x0f) {
	case PAUSEMENU_TO_MAP:
		routines_.call(ADDR_ReturnToMapFromLevel);
		break;
	case PAUSEMENU_SAVE:
		wram_.reset_disabled() = TRUE;
		wram_.sub_state() = SST_PAUSE_MENU_SAVE;
		break;
	case PAUSEMENU_ACTION_HELP:
		routines_.call(ADDR_OpenActionHelp);
		break;
	default: // back to the level
		++wram_.sub_state();
		break;
	}
}

void PauseMenu::exit()
{
	routines_.disable_lcd();
	routines_.clear_virtual_oam();
	routines_.call(ADDR_DrawLevelObjectsAfterLevelReturn);
	routines_.call(ADDR_LoadBackupVRAM);
	wram_.unused_is_paused() = 0;
	wram_.animated_tiles_frame_duration() = wram_.temp_animated_tiles_frame_duration();
	wram_.animated_tiles_gfx() = wram_.temp_animated_tiles_group();
	wram_.animated_tiles_frame_count() = 0;
	wram_.animated_tiles_frame() = 0;
	wram_.room_animated_tiles_enabled() = TRUE;
	routines_.call(ADDR_UpdateLevelMusic);
	mem_[rLCDC] = LCDC_DEFAULT;
	++wram_.sub_state();
}

// ---------------------------------------------------------------------------
// pause_menu_2.asm

void PauseMenu::print_music_coins()
{
	uint8_t tile = static_cast<uint8_t>(wram_.num_musical_coins() * 2 + 0xa0);
	mem_[v0BGMap1 + 15 * TILEMAP_WIDTH + 7] = tile;
	mem_[v0BGMap1 + 16 * TILEMAP_WIDTH + 7] = static_cast<uint8_t>(tile + 1);
}

uint16_t PauseMenu::transformation_frameset(uint16_t owl, uint16_t rail, uint16_t vampire, uint16_t other) const
{
	switch (wram_.transformation()) {
	case TRANSFORMATION_OWL_WARIO: return owl;
	case TRANSFORMATION_RAIL: return rail;
	case TRANSFORMATION_VAMPIRE_WARIO: return vampire;
	default: return other;
	}
}

void PauseMenu::highlight(uint8_t selection, uint8_t y, uint8_t x, uint16_t frameset)
{
	wram_.pause_menu_selection() = selection;
	cursor().init(y, x, frameset);
}

void PauseMenu::highlight_save()
{
	highlight(PAUSEMENU_SAVE, 0x40, 0x40, ADDR_Frameset_1f1c9c);
	play_sfx(mem_, SFX_0E2);
}

void PauseMenu::highlight_action_help()
{
	highlight(PAUSEMENU_ACTION_HELP, 0x38, 0x60, transformation_frameset(ADDR_Frameset_1f1cf9,
		ADDR_Frameset_1f1d1b, ADDR_Frameset_1f1d41, ADDR_Frameset_1f1c50));
	play_sfx(mem_, SFX_0E2);
}

void PauseMenu::highlight_to_map()
{
	highlight(PAUSEMENU_TO_MAP, 0x40, 0x80, ADDR_Frameset_1f1c74);
	play_sfx(mem_, SFX_0E2);
}

void PauseMenu::highlight_return()
{
	highlight(PAUSEMENU_RETURN, 0x38, 0x20, ADDR_Frameset_1f1cb8);
	play_sfx(mem_, SFX_0E2);
}

void PauseMenu::choose(uint16_t frameset, uint16_t sfx)
{
	cursor().set_frameset(frameset);
	wram_.pause_menu_selection().set_bit(PAUSEMENUF_B_PAD_SELECT);
	play_sfx(mem_, sfx);
}

// Left and right move between the options (save is skipped in time
// attack); A picks one, B goes back to the level.
void PauseMenu::handle_input()
{
	Byte selection = wram_.pause_menu_selection();
	if (selection.bit(PAUSEMENUF_B_PAD_SELECT))
		return;
	Byte pressed = wram_.joypad_pressed();
	if (pressed.bit(B_PAD_B)) {
		highlight(PAUSEMENU_RETURN, 0x38, 0x20, ADDR_Frameset_1f1cbb);
		wram_.pause_menu_selection().set_bit(PAUSEMENUF_B_PAD_SELECT);
		play_sfx(mem_, SFX_0E7);
		return;
	}
	switch (selection) {
	case PAUSEMENU_SAVE:
		if (pressed.bit(B_PAD_LEFT))
			highlight_return();
		else if (pressed.bit(B_PAD_RIGHT))
			highlight_action_help();
		else if (pressed.bit(B_PAD_A))
			choose(ADDR_Frameset_1f1c9f, SFX_SELECTION);
		return;
	case PAUSEMENU_ACTION_HELP:
		if (pressed.bit(B_PAD_LEFT)) {
			if (time_attack())
				highlight_return();
			else
				highlight_save();
		} else if (pressed.bit(B_PAD_RIGHT)) {
			highlight_to_map();
		} else if (pressed.bit(B_PAD_A)) {
			choose(transformation_frameset(ADDR_Frameset_1f1d0a, ADDR_Frameset_1f1d20,
				ADDR_Frameset_1f1d4c, ADDR_Frameset_1f1c69), SFX_SELECTION);
		}
		return;
	case PAUSEMENU_TO_MAP:
		if (pressed.bit(B_PAD_LEFT))
			highlight_action_help();
		else if (pressed.bit(B_PAD_A))
			choose(ADDR_Frameset_1f1c77, SFX_SELECTION);
		return;
	default: // return
		if (pressed.bit(B_PAD_RIGHT)) {
			if (time_attack())
				highlight_action_help();
			else
				highlight_save();
		} else if (pressed.bit(B_PAD_A)) {
			highlight(PAUSEMENU_RETURN, 0x38, 0x20, ADDR_Frameset_1f1cbb);
			wram_.pause_menu_selection().set_bit(PAUSEMENUF_B_PAD_SELECT);
			play_sfx(mem_, SFX_0E7);
		}
		return;
	}
}

// ---------------------------------------------------------------------------
// bank7d.asm

void PauseMenu::load_gfx()
{
	routines_.decompress(ADDR_PauseMenuGfx, v0Tiles0);
	uint16_t gfx = transformation_frameset(ADDR_ActionHelpOwlGfx, ADDR_ActionHelpRailGfx,
		ADDR_ActionHelpVampireGfx, 0);
	if (gfx)
		routines_.copy_bc(gfx, v0Tiles0, 0x20 * TILE_SIZE);
}

// the menu, or in time attack, with the best and current times
void PauseMenu::draw_maps()
{
	bool ta = time_attack();
	uint16_t dest = ta ? v0BGMap1 : v1BGMap1;
	mem_[rVBK] = VRAM1;
	routines_.decompress(ta ? ADDR_TimeAttackPauseMenuAttrmap : ADDR_PauseMenuAttrmap, dest);
	mem_[rVBK] = 0;
	routines_.decompress(ta ? ADDR_TimeAttackPauseMenuTilemap : ADDR_PauseMenuTilemap, dest);
	if (!ta)
		return;
	uint16_t best = static_cast<uint16_t>(wLevelTimeAttackScores + (wram_.level() >> 3) * 2);
	mem_[w3d511 + 0] = mem_[best];
	mem_[w3d511 + 1] = mem_[static_cast<uint16_t>(best + 1)];
	draw_time(v1BGMap1 + 2 * TILEMAP_WIDTH + 4, w3d511);
	draw_time(v1BGMap1 + 2 * TILEMAP_WIDTH + 11, wLevelTime);
}

// four BCD digits, each two tiles high, a gap after the second
void PauseMenu::draw_time(uint16_t coord, uint16_t time)
{
	static constexpr uint8_t COLUMNS[4] = {0, 1, 3, 4};
	for (int i = 0; i < 4; i++) {
		uint8_t byte = mem_[static_cast<uint16_t>(time + i / 2)];
		uint8_t digit = (i % 2 ? byte : byte >> 4) & 0x0f;
		uint8_t tile = static_cast<uint8_t>(digit * 2 + 0xa0);
		uint16_t addr = static_cast<uint16_t>((coord & 0xff00) | static_cast<uint8_t>(coord + COLUMNS[i]));
		mem_[addr] = tile;
		mem_[static_cast<uint16_t>(addr + TILEMAP_WIDTH)] = static_cast<uint8_t>(tile + 1);
	}
}

} // namespace wl3::pause

using wl3::pause::PauseMenu;

// Called from PauseMenuStateTable (see src/ported/engine/pause/common.c).
PORT_EX(_PauseMenuStateTable, 0, ALL_REGISTERS)
{
	PauseMenu(gb).run_state();
}

// Called from ReturnToPauseMenuFromActionHelp, whose caller (Action Help's
// state table) uses no register after it.
PORT_EX(_ReturnToPauseMenuFromActionHelp, 0, ALL_REGISTERS)
{
	PauseMenu(gb).return_from_action_help();
}

// hl = the object; farcalled from the musical coins screen too, which uses
// no register after it.
PORT_EX(LoadPauseMenuSprite, 0, ALL_REGISTERS)
{
	PauseMenu(gb).draw(wl3::MenuObject(gb, REG_HL(gb)));
}

// The routines implemented inside this module (they have no hooks).
PORT_COVERED(InitPauseMenu);
PORT_COVERED(InitPauseMenu_SkipBackupVRAM);
PORT_COVERED(UpdatePauseMenu);
PORT_COVERED(ExitPauseMenu);
PORT_COVERED(PrintNumberMusicCoins);
PORT_COVERED(HandlePauseMenuInput);
PORT_COVERED(LoadPauseMenuPals);
PORT_COVERED(LoadPauseMenuGfx);
PORT_COVERED(Func_1f403f);
