// The level clear screens; see clear.hpp.
#include "clear.hpp"

namespace wl3::clear {

namespace {

constexpr uint8_t VRAM1 = BANK_v1Tiles0;
constexpr uint8_t PAUSE_OBJECTS_BANK = 0x7c; // the coin box's framesets
constexpr uint16_t VBLANK_CLEAR_FUNC = ADDR_VBlank_ClearScreen + 0x0c; // .Func
constexpr uint8_t VBLANK_CLEAR_FUNC_SIZE = 0x37;
constexpr uint16_t VBLANK_COINS_FUNC = ADDR_VBlank_MusicalCoinsScreen + 0x0c; // .Func
constexpr uint8_t VBLANK_COINS_FUNC_SIZE = 0x3e;
// the treasure's position and frameset by colour, right before Func_1e028a
constexpr uint16_t TREASURE_PARAMS = ADDR_Func_1e028a - 16;

static_assert(wClearScreenWarioVar == wClearScreenWario + 8);
static_assert(wActionHelpWarioVar == wClearScreenWarioVar);
// the music boxes, the fifth first
constexpr uint16_t BOXES[5] = {wMenuObj3, wMenuObj4, wMenuObj5, wMenuObj6, wMenuObj7};

// add n with the carry in, then daa, as the CPU does: the result, and the
// carry out
uint8_t add_daa(uint8_t value, uint8_t n, bool &carry)
{
	unsigned sum = value + n + (carry ? 1u : 0u);
	bool half = ((value & 0x0f) + (n & 0x0f) + (carry ? 1u : 0u)) > 0x0f;
	carry = sum > 0xff;
	uint8_t a = static_cast<uint8_t>(sum);
	uint8_t adjust = 0;
	if (half || (a & 0x0f) > 9)
		adjust |= 0x06;
	if (carry || a > 0x99) {
		adjust |= 0x60;
		carry = true;
	}
	return static_cast<uint8_t>(a + adjust);
}

// the level's row (0 to 4) on the musical coins screen, and its column
uint8_t level_row(uint8_t level, uint8_t &column)
{
	static constexpr uint8_t ROW_STARTS[4] = {NUM_LEVELS - 5, NUM_LEVELS - 10, NUM_LEVELS - 15, NUM_LEVELS - 20};
	for (int i = 0; i < 4; i++)
		if (level >= ROW_STARTS[i]) {
			uint8_t row = static_cast<uint8_t>(4 - i);
			column = static_cast<uint8_t>(level - 5 * row);
			return row;
		}
	column = level;
	return 0;
}

} // namespace

void ClearScreen::run_state()
{
	switch (wram_.sub_state()) {
	case 0: routines_.call(ADDR_FastFadeToWhite); break;
	case 1: init(); break;
	case 2: routines_.call(ADDR_SlowFadeFromWhite); break;
	case 3: update(); break;
	case SST_CLEAR_TIME_ATTACK: routines_.call(ADDR_SlowFadeBGToWhite); break;
	case SST_CLEAR_TIME_ATTACK + 1: init_time_attack(); break;
	case SST_CLEAR_TIME_ATTACK + 2: routines_.call(ADDR_DarkenBGToPal_Fast); break;
	case SST_CLEAR_TIME_ATTACK + 3: update_time_attack(); break;
	case SST_CLEAR_MUSICAL_COINS: routines_.call(ADDR_SlowFadeBGToWhite); break;
	case SST_CLEAR_MUSICAL_COINS + 1: init_musical_coins(); break;
	case SST_CLEAR_MUSICAL_COINS + 2: routines_.call(ADDR_DarkenBGToPal_Fast); break;
	case SST_CLEAR_MUSICAL_COINS + 3: count_coins(); break;
	case SST_CLEAR_MUSICAL_COINS + 4: pencil_moves(); break;
	case SST_CLEAR_MUSICAL_COINS + 5: pencil_scribbles(); break;
	case SST_CLEAR_EXIT: exit(); break;
	case SST_CLEAR_EXIT + 1: case SST_CLEAR_EXIT + 2: case SST_CLEAR_EXIT + 3: case SST_CLEAR_EXIT + 4:
		routines_.call(ADDR_DebugReset);
		break;
	default: unrealistic(); break;
	}
}

void ClearScreen::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

void ClearScreen::draw(MenuObject obj)
{
	obj.load_sprite(ADDR_OAM_d4e26);
}

void ClearScreen::draw_wario()
{
	wario().load_sprite(ADDR_OAM_d49cc);
}

void ClearScreen::update_object(MenuObject obj)
{
	obj.animate();
	draw(obj);
}

void ClearScreen::animate_wario(Byte played, uint16_t next_frameset)
{
	MenuObject w = wario();
	w.animate();
	if (played || !wram_.obj_anim_was_reset())
		return;
	played = TRUE;
	w.set_frameset(next_frameset);
	w.animate();
}

void ClearScreen::stop_sfx()
{
	mem_[hSFXID + 0] = 0xff;
	mem_[hSFXID + 1] = 0x00;
}

void ClearScreen::copy_vblank(uint16_t func, uint8_t size)
{
	routines_.copy(func, wVBlankFunc, size);
}

// ---------------------------------------------------------------------------
// The treasure screen

void ClearScreen::init()
{
	wram_.reset_disabled() = TRUE;
	routines_.disable_lcd();
	routines_.call(ADDR_ClearBGMap0);
	routines_.clear_virtual_oam();
	set_treasure_transition_param();
	count_music_boxes();

	routines_.in_bank(BANK_LoadClearScreenPals, ADDR_LoadClearScreenPals, [&] {
		routines_.load_pals_to_temp_pals1(ADDR_Pals_1e0378);
		routines_.load_pals_to_temp_pals2(ADDR_Pals_1e03b8);
	});
	routines_.call(ADDR_LoadCollectedTreasurePal_ClearScreen);
	routines_.in_bank(BANK_LoadClearScreenGfx, ADDR_LoadClearScreenGfx, [&] { load_gfx(); });
	routines_.farcall(BANK_LoadLevelTreasureData, ADDR_LoadLevelTreasureData);
	routines_.in_bank(BANK_Func_1e0045, ADDR_Func_1e0045, [&] {
		mem_[rVBK] = VRAM1;
		routines_.decompress(ADDR_ClearScreenTreasureAttrmap, v1BGMap0);
		mem_[rVBK] = 0;
		routines_.decompress(ADDR_ClearScreenTreasureTilemap, v0BGMap0);
	});
	SET_HL(gb_, v0BGMap0 + 16 * TILEMAP_WIDTH + 6);
	routines_.farcall(BANK_PrintNumberCoins, ADDR_PrintNumberCoins);

	// the treasures of the level's colours
	static constexpr struct { int flag; uint8_t x; uint8_t tile; } TREASURES[4] = {
		{GREY_TREASURE_F, 3, 0x54},
		{RED_TREASURE_F, 7, 0x54},
		{GREEN_TREASURE_F, 11, 0x5c},
		{BLUE_TREASURE_F, 15, 0x5c},
	};
	Byte flags = wram_.key_and_treasure_flags();
	for (const auto &t : TREASURES)
		if (flags.bit(t.flag)) {
			uint16_t addr = static_cast<uint16_t>(v0BGMap0 + 8 * TILEMAP_WIDTH + t.x);
			mem_[addr] = t.tile;
			mem_[static_cast<uint16_t>(addr + 1)] = static_cast<uint8_t>(t.tile + 1);
		}

	update_num_collected_treasures();
	copy_vblank(VBLANK_CLEAR_FUNC, VBLANK_CLEAR_FUNC_SIZE);
	wram_.scx() = 0;
	mem_[rSCX] = 0;
	wram_.scy() = 0x04;
	mem_[rSCY] = 0x04;
	routines_.in_bank(BANK_Func_1e015b, ADDR_Func_1e015b, [&] { set_up_sprites(); });

	if (wram_.power_up_level() & ACTION_HELP_LEVEL_CLEAR)
		update_object(object(wMenuObj8));
	wario().animate();
	draw_wario();
	if (wram_.level_end_screen() != LVLEND_NO_TREASURE)
		update_object(object(wMenuObj1));
	update_object(object(wMenuObj2));
	// the music boxes: wMenuObj7 for the first, to wMenuObj3 for the fifth
	uint8_t boxes = music_boxes();
	for (int i = 5 - boxes; boxes && i < 5; i++)
		update_object(object(BOXES[i]));

	mem_[rLCDC] = LCDC_DEFAULT;
	++wram_.sub_state();
}

// The treasure found goes to the collected ones; the music boxes are the
// highest of treasure bits 5 to 1.
void ClearScreen::count_music_boxes()
{
	uint8_t wram_bank = mem_[rWBK];
	mem_[rWBK] = BANK_wTreasuresCollected;
	uint8_t param = wram_.transition_param();
	if (param != TRANSITION_RETURN_TO_MAP) {
		gb_->cpu.a = param;
		routines_.call(ADDR_IsTreasureCollected);
		mem_[REG_HL(gb_)] |= gb_->cpu.d;
	}
	uint8_t collected = wram_.treasures_collected();
	uint8_t boxes = 0;
	for (int bit = 5; bit >= 1; bit--)
		if (collected & (1 << bit)) {
			boxes = static_cast<uint8_t>(bit);
			break;
		}
	wram_.num_music_boxes() = boxes;
	mem_[rWBK] = wram_bank;
}

uint8_t ClearScreen::music_boxes() const
{
	uint8_t boxes = wram_.num_music_boxes();
	return boxes > 5 ? 5 : boxes;
}

void ClearScreen::update()
{
	if (wram_.power_up_level() & ACTION_HELP_LEVEL_CLEAR)
		update_object(object(wMenuObj8));
	bool to_map = wram_.transition_param() == TRANSITION_RETURN_TO_MAP;
	animate_wario(wram_.clear_screen_wario_var(), to_map ? ADDR_Frameset_d4d5f : ADDR_Frameset_d4d1b);
	draw_wario();
	draw(object(wMenuObj2));
	if (wram_.level_end_screen() != LVLEND_NO_TREASURE)
		update_object(object(wMenuObj1));
	for (int i = music_boxes() - 1; i >= 0; i--)
		draw(object(BOXES[4 - i]));
	routines_.clear_unused_virtual_oam();
	if (wram_.joypad_pressed().bit(B_PAD_A))
		wram_.sub_state() = SST_CLEAR_MUSICAL_COINS; // OpenMusicalCoinsScreen
}

// Back to the map, or on to the treasure's own transition; a power-up's
// treasure shows its Action Help after.
void ClearScreen::set_treasure_transition_param()
{
	uint8_t screen = wram_.level_end_screen();
	if (screen == LVLEND_NO_TREASURE) {
		wram_.transition_param() = TRANSITION_RETURN_TO_MAP;
		return;
	}
	uint8_t index = static_cast<uint8_t>((wram_.level() >> 3) * 4 + screen - 1);
	uint8_t treasure = mem_[static_cast<uint16_t>(ADDR_LevelTreasureIDs_WithoutTemple + index)];
	wram_.transition_param() = treasure;
	static constexpr uint8_t POWER_UPS[][2] = {
		{LEAD_OVERALLS_T, LEAD_OVERALLS},
		{SWIMMING_FLIPPERS_T, SWIMMING_FLIPPERS},
		{HEAD_SMASH_HELMET_T, HEAD_SMASH_HELMET},
		{GRAB_GLOVE_T, GRAB_GLOVE},
		{GARLIC_T, GARLIC},
		{SUPER_JUMP_SLAM_OVERALLS_T, SUPER_JUMP_SLAM_OVERALLS},
		{HIGH_JUMP_BOOTS_T, HIGH_JUMP_BOOTS},
		{PRINCE_FROGS_GLOVES_T, PRINCE_FROGS_GLOVES},
		{SUPER_GRAB_GLOVES_T, SUPER_GRAB_GLOVES},
	};
	for (const auto &p : POWER_UPS)
		if (treasure == p[0]) {
			wram_.power_up_level() = p[1] | ACTION_HELP_LEVEL_CLEAR;
			return;
		}
}

// wNumCollectedTreasures, in BCD (high byte first)
void ClearScreen::update_num_collected_treasures()
{
	uint8_t wram_bank = mem_[rWBK];
	mem_[rWBK] = BANK_wTreasuresCollected;
	Byte high = mem_[wNumCollectedTreasures + 0];
	Byte low = mem_[wNumCollectedTreasures + 1];
	low = 0;
	high = 0;
	for (uint8_t treasure = NUM_TREASURES - 1; treasure; treasure--) {
		gb_->cpu.a = treasure;
		routines_.call(ADDR_IsTreasureCollected);
		if (!gb_->cpu.a)
			continue;
		bool carry = false;
		low = add_daa(low, 1, carry);
		high = add_daa(high, 0, carry);
	}
	mem_[rWBK] = wram_bank;
}

void ClearScreen::load_gfx()
{
	mem_[rVBK] = VRAM1;
	routines_.decompress(ADDR_ClearScreen2Gfx, v0Tiles0);
	mem_[rVBK] = 0;
	routines_.decompress(ADDR_ClearScreen1Gfx, v0Tiles1);
	wram_.temp_bank() = BANK_WarioClearGfx;
	SET_HL(gb_, ADDR_WarioClearGfx);
	SET_DE(gb_, v0Tiles0);
	SET_BC(gb_, 0x80 * TILE_SIZE);
	routines_.call(ADDR_FarCopyHLToDE_BC);
}

// the power-up, Wario, the treasure and the music boxes
void ClearScreen::set_up_sprites()
{
	if (wram_.power_up_level() & ACTION_HELP_LEVEL_CLEAR)
		object(wMenuObj8).init(0x53, 0x24, ADDR_Frameset_d506b);
	bool to_map = wram_.transition_param() == TRANSITION_RETURN_TO_MAP;
	wario().init(0x76, 0x34, to_map ? ADDR_Frameset_d4d58 : ADDR_Frameset_d4d18);
	wram_.clear_screen_wario_var() = 0;

	static constexpr uint8_t SCREENS[4] = {
		LVLEND_GREY_TREASURE, LVLEND_RED_TREASURE, LVLEND_GREEN_TREASURE, LVLEND_BLUE_TREASURE,
	};
	uint16_t music = MUSIC_LEVEL_CLEAR_NO_TREASURE;
	uint8_t screen = wram_.level_end_screen();
	for (int t = 0; t < 4; t++)
		if (screen == SCREENS[t]) {
			uint16_t params = static_cast<uint16_t>(TREASURE_PARAMS + 4 * t);
			uint16_t frameset = static_cast<uint16_t>(mem_[static_cast<uint16_t>(params + 2)] << 8 |
				mem_[static_cast<uint16_t>(params + 3)]);
			object(wMenuObj1).init(mem_[params], mem_[static_cast<uint16_t>(params + 1)], frameset);
			music = MUSIC_LEVEL_CLEAR_GOT_TREASURE;
			break;
		}
	play_music(mem_, music);
	stop_sfx();
	object(wMenuObj2).init(0x28, 0x30, ADDR_Frameset_d5059);

	uint8_t boxes = music_boxes();
	if (!boxes)
		return;
	if (boxes >= 5)
		object(wMenuObj3).init(0x79, 0x84, ADDR_Frameset_d5068);
	if (boxes >= 4)
		object(wMenuObj4).init(0x8d, 0x7f, ADDR_Frameset_d5065);
	if (boxes >= 3)
		object(wMenuObj5).init(0x8c, 0x69, ADDR_Frameset_d5062);
	if (boxes >= 2)
		object(wMenuObj6).init(0x79, 0x62, ADDR_Frameset_d505f);
	object(wMenuObj7).init(0x67, 0x74, ADDR_Frameset_d505c);
}

// ---------------------------------------------------------------------------
// The time attack result

namespace {

// the keys (wMenuObj4 to wMenuObj7) by flag
constexpr struct { int flag; uint16_t obj; uint8_t y, x; uint16_t frameset; } KEYS[4] = {
	{GREY_KEY_F, wMenuObj4, 0x7c, 0x5c, ADDR_Frameset_d5095},
	{RED_KEY_F, wMenuObj5, 0x7c, 0x74, ADDR_Frameset_d5098},
	{GREEN_KEY_F, wMenuObj6, 0x8e, 0x5c, ADDR_Frameset_d509b},
	{BLUE_KEY_F, wMenuObj7, 0x8e, 0x74, ADDR_Frameset_d509e},
};

} // namespace

void ClearScreen::init_time_attack()
{
	wram_.reset_disabled() = TRUE;
	routines_.disable_lcd();
	routines_.call(ADDR_ClearBGMap0);
	routines_.clear_virtual_oam();
	wram_.transition_param() = TRANSITION_RETURN_TO_MAP;
	routines_.in_bank(BANK_Func_1e000d, ADDR_Func_1e000d, [&] {
		routines_.load_pals_to_temp_pals1(ADDR_Pals_1e03f8);
		routines_.load_pals_to_temp_pals2(ADDR_Pals_1e0438);
	});
	routines_.in_bank(BANK_LoadClearScreenGfx, ADDR_LoadClearScreenGfx, [&] { load_gfx(); });
	routines_.in_bank(BANK_Func_1e005f, ADDR_Func_1e005f, [&] { time_attack_result(); });
	copy_vblank(VBLANK_CLEAR_FUNC, VBLANK_CLEAR_FUNC_SIZE);
	wram_.scx() = 0;
	mem_[rSCX] = 0;
	wram_.scy() = 0x04;
	mem_[rSCY] = 0x04;
	routines_.in_bank(BANK_Func_1e028a, ADDR_Func_1e028a, [&] { set_up_time_attack_sprites(); });

	if (wram_.time_attack_result() == TIME_ATTACK_GOT_HIGH_SCORE)
		update_object(object(wMenuObj8));
	wario().animate();
	draw_wario();
	update_object(object(wMenuObj2));
	update_object(object(wMenuObj3));
	for (const auto &k : KEYS)
		if (wram_.key_and_treasure_flags().bit(k.flag))
			update_object(object(k.obj));
	mem_[rLCDC] = LCDC_DEFAULT;
	++wram_.sub_state();
}

// With all four keys, the level's time is a result, a record if it beats
// the best.
void ClearScreen::time_attack_result()
{
	mem_[rVBK] = VRAM1;
	routines_.decompress(ADDR_ClearScreenTimeAttackAttrmap, v0BGMap0);
	mem_[rVBK] = 0;
	routines_.decompress(ADDR_ClearScreenTimeAttackTilemap, v0BGMap0);

	Byte result = wram_.time_attack_result();
	uint16_t best = static_cast<uint16_t>(wLevelTimeAttackScores + (wram_.level() >> 3) * 2);
	result = TIME_ATTACK_NOT_COMPLETED;
	if ((wram_.key_and_treasure_flags() & KEYS_MASK) == (GREY_KEY | RED_KEY | GREEN_KEY | BLUE_KEY)) {
		result = TIME_ATTACK_NO_HIGH_SCORE;
		uint8_t time_hi = mem_[wLevelTime + 0];
		uint8_t time_lo = mem_[wLevelTime + 1];
		uint8_t best_hi = mem_[best];
		uint8_t best_lo = mem_[static_cast<uint16_t>(best + 1)];
		if (time_hi < best_hi || (time_hi == best_hi && time_lo < best_lo)) {
			mem_[best] = time_hi;
			mem_[static_cast<uint16_t>(best + 1)] = time_lo;
			result = TIME_ATTACK_GOT_HIGH_SCORE;
		}
	}
	mem_[w3d511 + 0] = mem_[best];
	mem_[w3d511 + 1] = mem_[static_cast<uint16_t>(best + 1)];
	print_time(v0BGMap0 + 7 * TILEMAP_WIDTH + 10, w3d511);
	if (result)
		print_time(v0BGMap0 + 10 * TILEMAP_WIDTH + 10, wLevelTime);
}

// four BCD digits, each two tiles high, a gap after the second
void ClearScreen::print_time(uint16_t coord, uint16_t time)
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

void ClearScreen::set_up_time_attack_sprites()
{
	bool record = wram_.time_attack_result() == TIME_ATTACK_GOT_HIGH_SCORE;
	if (record)
		object(wMenuObj8).init(0x60, 0x1c, ADDR_Frameset_d5080);
	wario().init(SCREEN_HEIGHT_PX, 0x2c, record ? ADDR_Frameset_d4d18 : ADDR_Frameset_d4d58);
	wram_.clear_screen_wario_var() = 0;
	play_music(mem_, record ? MUSIC_LEVEL_CLEAR_GOT_TREASURE : MUSIC_LEVEL_CLEAR_NO_TREASURE);
	stop_sfx();
	object(wMenuObj2).init(0x28, 0x30, ADDR_Frameset_d5059);
	object(wMenuObj3).init(0x44, 0x28, ADDR_Frameset_d50a1);
	for (const auto &k : KEYS)
		if (wram_.key_and_treasure_flags().bit(k.flag))
			object(k.obj).init(k.y, k.x, k.frameset);
}

void ClearScreen::update_time_attack()
{
	bool record = wram_.time_attack_result() == TIME_ATTACK_GOT_HIGH_SCORE;
	if (record)
		update_object(object(wMenuObj8));
	animate_wario(wram_.clear_screen_wario_var(), record ? ADDR_Frameset_d4d1b : ADDR_Frameset_d4d5f);
	draw_wario();
	draw(object(wMenuObj2));
	draw(object(wMenuObj3));
	for (const auto &k : KEYS)
		if (wram_.key_and_treasure_flags().bit(k.flag))
			draw(object(k.obj));
	routines_.clear_unused_virtual_oam();
	if (wram_.joypad_pressed().bit(B_PAD_A))
		wram_.sub_state() = SST_CLEAR_MUSICAL_COINS; // OpenMusicalCoinsScreen
}

// ---------------------------------------------------------------------------
// The musical coins

void ClearScreen::init_musical_coins()
{
	routines_.disable_lcd();
	routines_.call(ADDR_ClearBGMap0);
	routines_.clear_virtual_oam();
	routines_.load_pals_to_temp_pals1(ADDR_Pals_d50a4);
	routines_.load_pals_to_temp_pals2(ADDR_Pals_d50e4);
	mem_[rVBK] = 0;
	routines_.decompress(ADDR_GolfMenuGfx, v0Tiles0);
	mem_[rVBK] = VRAM1;
	routines_.decompress(ADDR_GolfMenuAttrmap, v1BGMap0);
	mem_[rVBK] = 0;
	routines_.decompress(ADDR_GolfMenuTilemap, v0BGMap0);
	draw_coin_flags();
	wram_.all_musical_coins_level() = 0xff;
	if (wram_.num_musical_coins() == NUM_LEVEL_MUSICAL_COINS)
		set_musical_coin_flag();
	copy_vblank(VBLANK_COINS_FUNC, VBLANK_COINS_FUNC_SIZE);
	wram_.scx() = 0;
	mem_[rSCX] = 0;
	wram_.scy() = 0;
	mem_[rSCY] = 0;

	MenuObject box = object(wMenuObj3);
	box.init(0x38, 0x40, ADDR_Frameset_1f1cd4);
	box.animate(PAUSE_OBJECTS_BANK);
	SET_HL(gb_, wMenuObj3);
	routines_.farcall(BANK_LoadPauseMenuSprite, ADDR_LoadPauseMenuSprite);

	wram_.animated_tiles_frame_duration() = 8;
	wram_.animated_tiles_gfx() = ANIMATED_TILES_GFX_15;
	wram_.animated_tiles_frame_count() = 0;
	wram_.animated_tiles_frame() = 0;
	wram_.room_animated_tiles_enabled() = TRUE;
	mem_[rLCDC] = LCDC_DEFAULT;
	wram_.musical_coin_visual_counter() = 0;
	wram_.timer() = 60;
	++wram_.sub_state();
}

void ClearScreen::update_coin_box()
{
	object(wMenuObj3).animate(PAUSE_OBJECTS_BANK);
	SET_HL(gb_, wMenuObj3);
	routines_.farcall(BANK_LoadPauseMenuSprite, ADDR_LoadPauseMenuSprite);
	routines_.clear_unused_virtual_oam();
}

// The level's coins count up one by one; with all of them, the pencil
// comes.
void ClearScreen::count_coins()
{
	update_coin_box();
	Byte shown = wram_.musical_coin_visual_counter();
	if (shown != wram_.num_musical_coins()) {
		Byte timer = wram_.timer();
		if (timer) {
			--timer;
			return;
		}
		if (wram_.global_counter() & 0x03)
			return;
		play_sfx(mem_, SFX_REGULAR_COIN);
		// the count's digit, through the BG map tile queue
		uint8_t tile = static_cast<uint8_t>(++shown * 2 + 0xa0);
		uint16_t queue = wBGMapTileVRAM0Queue;
		mem_[queue + 0] = static_cast<uint8_t>((v0BGMap0 + 0x8b) >> 8);
		mem_[queue + 1] = static_cast<uint8_t>(v0BGMap0 + 0x8b);
		mem_[queue + 2] = tile;
		mem_[queue + 3] = 0x98;
		mem_[queue + 4] = 0xab;
		mem_[queue + 5] = static_cast<uint8_t>(tile + 1);
		wram_.unk_cee4() = 0x80;
		return;
	}
	if (wram_.all_musical_coins_level() == 0xff) {
		finish_soon();
		return;
	}
	play_sfx(mem_, SFX_03A);
	MenuObject pencil = object(wMenuObj1);
	pencil.init(0x42, 0x60, ADDR_Frameset_d65f0);
	pencil.animate();
	++wram_.sub_state();
}

void ClearScreen::finish_soon()
{
	mem_[wTimer + 0] = 0;
	mem_[wTimer + 1] = 2;
	wram_.sub_state() = SST_CLEAR_EXIT;
}

void ClearScreen::pencil_moves()
{
	bool there = move_pencil();
	draw_pencil();
	update_coin_box();
	if (!there)
		return;
	wram_.timer() = 0;
	play_sfx(mem_, SFX_03A);
	++wram_.sub_state();
}

// down to the level's row, then left to its column
bool ClearScreen::move_pencil()
{
	uint8_t column;
	uint8_t row = level_row(wram_.all_musical_coins_level(), column);
	static constexpr uint8_t ROW_Y[5] = {0x48, 0x50, 0x58, 0x60, 0x68};
	MenuObject pencil = object(wMenuObj1);
	if (pencil.y() != ROW_Y[row]) {
		++pencil.y();
		return false;
	}
	static constexpr uint8_t COLUMN_X[4] = {0x48, 0x50, 0x58, 0x60};
	uint8_t x = column >= 1 && column <= 4 ? COLUMN_X[column - 1] : 0x40;
	if (pencil.x() == x)
		return true;
	--pencil.x();
	return false;
}

void ClearScreen::pencil_scribbles()
{
	MenuObject pencil = object(wMenuObj1);
	uint8_t step = wram_.timer();
	pencil.y() += mem_[static_cast<uint16_t>(ADDR_PencilScribbleXOffsets + step)];
	pencil.x() += mem_[static_cast<uint16_t>(ADDR_PencilScribbleYOffsets + step)];
	draw_pencil();
	update_coin_box();

	uint8_t t = ++wram_.timer();
	if (t >= 52) {
		play_sfx(mem_, SFX_MUSICAL_COIN);
		finish_soon();
		return;
	}
	if (t != 20)
		return;
	// the level's square takes its colour
	uint8_t level = wram_.all_musical_coins_level();
	uint8_t pal = mem_[static_cast<uint16_t>(ADDR_MusicalCoinLevelBGPals + level)];
	uint8_t column;
	uint8_t row = level_row(level, column);
	uint16_t addr = static_cast<uint16_t>(v0BGMap0 + (9 + row) * TILEMAP_WIDTH + 8 + column);
	mem_[wBGMapTileVRAM0Queue + 0] = static_cast<uint8_t>(addr >> 8);
	mem_[wBGMapTileVRAM0Queue + 1] = static_cast<uint8_t>(addr);
	mem_[wBGMapTileVRAM0Queue + 2] = pal;
	wram_.unk_cee4() = 0x81;
}

void ClearScreen::draw_pencil()
{
	object(wMenuObj1).load_sprite(ADDR_OAM_d65e1);
}

// A or a while: on to Action Help after a new power-up, or the map.
void ClearScreen::exit()
{
	update_coin_box();
	if (!wram_.joypad_pressed().bit(B_PAD_A)) {
		if (--mem_[wTimer + 0])
			return;
		if (--mem_[wTimer + 1])
			return;
	}
	mem_[wTimer + 0] = 0;
	mem_[wTimer + 1] = 0;
	if (!wram_.game_mode_flags().bit(MODE_TIME_ATTACK_F) &&
		(wram_.power_up_level() & ACTION_HELP_LEVEL_CLEAR)) {
		routines_.call(ADDR_OpenActionHelp);
		return;
	}
	routines_.farcall(BANK_Func_4628, ADDR_Func_4628);
}

// The squares of the levels whose musical coins were all found, coloured.
void ClearScreen::draw_coin_flags()
{
	mem_[rVBK] = VRAM1;
	uint16_t dest = v0BGMap0 + 9 * TILEMAP_WIDTH + 8;
	Byte level = hram_.unk_ffa0();
	level = 0xff;
	for (int byte = 0; byte < NUM_LEVELS / 8; byte++) {
		uint8_t flags = mem_[static_cast<uint16_t>(wMusicalCoinFlags + byte)];
		for (int bit = 0; bit < 8; bit++) {
			++level;
			if (flags & (1 << bit))
				mem_[dest] = mem_[static_cast<uint16_t>(ADDR_MusicalCoinLevelBGPals + level)];
			// five squares a row
			uint8_t e = static_cast<uint8_t>(dest + 1);
			if ((e & 0x1f) == 13)
				e = static_cast<uint8_t>(e + TILEMAP_WIDTH - 5);
			dest = static_cast<uint16_t>((dest & 0xff00) | e);
		}
	}
	// the byte after the flags, as the original checks it
	if (mem_[static_cast<uint16_t>(wMusicalCoinFlags + NUM_LEVELS / 8)] & 0x01)
		mem_[dest] = mem_[static_cast<uint16_t>(ADDR_MusicalCoinLevelBGPals + level)];
	mem_[rVBK] = 0;
}

// The level's flag: wAllMusicalCoinsLevel is the level, or $ff if it was
// already set.
void ClearScreen::set_musical_coin_flag()
{
	uint8_t level = wram_.level() >> 3;
	wram_.all_musical_coins_level() = level;
	Byte flags = mem_[static_cast<uint16_t>(wMusicalCoinFlags + (level >> 3))];
	uint8_t bit = level & 0x07;
	if (flags.bit(bit))
		wram_.all_musical_coins_level() = 0xff;
	else
		flags.set_bit(bit);
}

} // namespace wl3::clear

// Called from ClearScreenStateTable (see src/ported/engine/clear/common.c).
PORT_EX(_ClearScreenStateTable, 0, ALL_REGISTERS)
{
	wl3::clear::ClearScreen(gb).run_state();
}

// The routines implemented inside this module (they have no hooks).
PORT_COVERED(InitClearScreen);
PORT_COVERED(UpdateClearScreen);
PORT_COVERED(OpenMusicalCoinsScreen);
PORT_COVERED(LoadClearScreenSprite);
PORT_COVERED(LoadClearScreenWarioSprite);
PORT_COVERED(UpdateNumCollectedTreasures);
PORT_COVERED(SetTreasureTransitionParam);
PORT_COVERED(InitTimeAttackClearScreen);
PORT_COVERED(Func_d434c);
PORT_COVERED(InitMusicalCoinScreen);
PORT_COVERED(Func_d4472);
PORT_COVERED(Func_d45e1);
PORT_COVERED(Func_d4507);
PORT_COVERED(Func_d454c);
PORT_COVERED(ExitClearScreen);
PORT_COVERED(Func_d464e);
PORT_COVERED(Func_d465b);
PORT_COVERED(Func_d4668);
PORT_COVERED(LoadPencilSprite);
PORT_COVERED(DoPencilMovement);
PORT_COVERED(DoPencilScribbling);
PORT_COVERED(Func_d47f1);
PORT_COVERED(SetMusicalCoinFlag);
PORT_COVERED(LoadClearScreenPals);
PORT_COVERED(Func_1e000d);
PORT_COVERED(LoadClearScreenGfx);
PORT_COVERED(Func_1e0045);
PORT_COVERED(Func_1e005f);
PORT_COVERED(Func_1e015b);
PORT_COVERED(Func_1e028a);
PORT_COVERED(VBlank_ClearScreen);
PORT_COVERED(VBlank_MusicalCoinsScreen);
