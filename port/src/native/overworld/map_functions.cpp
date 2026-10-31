// The map's functions (bank2d.asm): the scenes each side's events play
// (_DoOWFunction runs them). See overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

constexpr uint16_t FALLING_OFFSETS = ADDR_GetSceneFallingYOffset + 0x11; // .yoffsets
constexpr uint8_t FALLING_OFFSETS_SIZE = 0x64;
constexpr uint16_t LEVEL_NAMES = ADDR_LoadOWSceneLevelName + 0x25; // .MapSidePointers
constexpr uint8_t SCENE_OBJ_FRAME_OFFSET = 2;
constexpr uint8_t SCENE_OBJ_STATE_OFFSET = 6;

} // namespace

void MapFunctions::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

// _DoOWFunction: a frame of the scene in wOWSceneActionParam, by side
// (NorthOWFunctions, WestOWFunctions, SouthOWFunctions, EastOWFunctions)
void MapFunctions::run()
{
	using Scene = void (MapFunctions::*)();
	static constexpr Scene NORTH_SCENES[] = {
		&MapFunctions::cut_tree, &MapFunctions::open_gate, &MapFunctions::finish, // NOWFunc_Garlic
		&MapFunctions::north_magic_seed, &MapFunctions::north_summon_snake,
		&MapFunctions::fall_leaves, &MapFunctions::play_music_box, &MapFunctions::play_music_box,
		&MapFunctions::crayon, &MapFunctions::crayon, &MapFunctions::crayon, &MapFunctions::crayon,
		&MapFunctions::crayon, &MapFunctions::crayon, &MapFunctions::crayon,
		&MapFunctions::north_bottom_bar, &MapFunctions::day_night_spell, &MapFunctions::prologue,
	};
	static constexpr Scene WEST_SCENES[] = {
		&MapFunctions::rain, &MapFunctions::clear_tornado, &MapFunctions::elevator_working,
		&MapFunctions::west_magic_seed, &MapFunctions::west_summon_snake,
		&MapFunctions::west_earthquake, &MapFunctions::west_volcano, &MapFunctions::red_music_box,
	};
	static constexpr Scene SOUTH_SCENES[] = {
		&MapFunctions::send_seeds, &MapFunctions::south_magic_seed,
		&MapFunctions::yellow_music_box, &MapFunctions::canyon_thunder,
		&MapFunctions::raise_tower, &MapFunctions::fan, &MapFunctions::south_summon_snake,
		&MapFunctions::south_earthquake, &MapFunctions::green_music_box,
		&MapFunctions::explosives, &MapFunctions::full_moon,
	};
	static constexpr Scene EAST_SCENES[] = {
		&MapFunctions::freeze_sea, &MapFunctions::east_earthquake, &MapFunctions::form_castle,
		&MapFunctions::explosives, &MapFunctions::east_volcano, &MapFunctions::burn_vines,
		&MapFunctions::show_warped_void, &MapFunctions::raise_pipe, &MapFunctions::daytime,
	};
	struct Side {
		const Scene *scenes;
		size_t count;
	};
	static constexpr Side SIDES[] = {
		{NORTH_SCENES, sizeof(NORTH_SCENES) / sizeof(NORTH_SCENES[0])},
		{WEST_SCENES, sizeof(WEST_SCENES) / sizeof(WEST_SCENES[0])},
		{SOUTH_SCENES, sizeof(SOUTH_SCENES) / sizeof(SOUTH_SCENES[0])},
		{EAST_SCENES, sizeof(EAST_SCENES) / sizeof(EAST_SCENES[0])},
	};

	++wram_.ow_func_counter();
	uint8_t side = wram_.cur_map_side();
	uint8_t scene = static_cast<uint8_t>(wram_.ow_scene_action_param() - 1);
	if (side >= 4 || scene >= SIDES[side].count) {
		unrealistic();
		return;
	}
	(this->*SIDES[side].scenes[scene])();
}

// ---------------------------------------------------------------------------
// The steps' helpers

// AdvanceOWFunc
void MapFunctions::next_step()
{
	wram_.ow_func_counter() = 0;
	++wram_.unk_2d062();
}

// WaitOWFunc: past frames, the next step
void MapFunctions::wait(uint8_t frames)
{
	if (frames >= wram_.ow_func_counter())
		return;
	next_step();
}

// WaitOWObjState (WaitOWObjDisappear: value 0)
void MapFunctions::wait_state(uint16_t state, uint8_t value)
{
	if (mem_[state] == value)
		next_step();
}

// EndOWFunc
void MapFunctions::finish()
{
	wram_.ow_scene_action_param() = 0;
	wram_.ow_func_counter() = 0;
	wram_.unk_2d062() = 0;
}

// LoadOWSceneLevelName: the name of the level the function happens at
// (outside the north, the table's values are 1 more)
void MapFunctions::load_level_name()
{
	uint8_t side = wram_.cur_map_side();
	uint16_t names = mem_.word(static_cast<uint16_t>(LEVEL_NAMES + static_cast<uint8_t>(side * 2)));
	uint8_t index = static_cast<uint8_t>(wram_.ow_scene_action_param() - 1);
	uint16_t name = static_cast<uint16_t>(names + index);
	uint8_t level = mem_[name];
	if (side)
		--level;
	SET_HL(gb_, name);
	gb_->cpu.e = index;
	gb_->cpu.b = side;
	gb_->cpu.d = level;
	routines_.call(ADDR_LoadLevelName);
	wram_.pal_config1_total_steps() = 0;
	next_step();
}

// WaitOWSceneLevelNameFadeIn
void MapFunctions::wait_level_name()
{
	routines_.farcall(BANK_FadeInLevelName, ADDR_FadeInLevelName);
	if (gb_->cpu.c & 1) // still fading in
		return;
	next_step();
}

// GetSceneFallingYOffset: a falling object's step by its frame (from the
// counter at counter, held near the table's end)
uint8_t MapFunctions::falling_offset(uint16_t counter)
{
	Byte frame = mem_[counter];
	if (frame >= FALLING_OFFSETS_SIZE)
		frame = FALLING_OFFSETS_SIZE - 4;
	return mem_[static_cast<uint16_t>(FALLING_OFFSETS + frame)];
}

bool MapFunctions::move(uint16_t obj, uint16_t data)
{
	SET_DE(gb_, data);
	SET_HL(gb_, obj);
	routines_.call(ADDR_ApplyOWMovement);
	return !(gb_->cpu.f & FLAG_Z);
}

void MapFunctions::set_state(uint16_t state, uint8_t value)
{
	gb_->cpu.a = value;
	SET_HL(gb_, state);
	routines_.call(ADDR_SetSceneObjState);
}

// ---------------------------------------------------------------------------
// The north side

// NOWFunc_CutTree: the tree falls and disappears
void MapFunctions::cut_tree()
{
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(4); return; // .Wait1
	case 2: // .SetTreeFall
		set_state(wSceneObj8State, 0x02);
		play_sfx(mem_, SFX_102);
		next_step();
		return;
	case 3: // .WaitFall
		if (mem_[wSceneObj8Frame] != 0x07)
			return;
		play_sfx(mem_, SFX_01A);
		next_step();
		return;
	case 4: wait_state(wSceneObj8State, 0); return; // .WaitDisappear
	case 5: wait(4); return; // .Wait2
	case 6: finish(); return;
	default: unrealistic(); return;
	}
}

// NOWFunc_OpenGate
void MapFunctions::open_gate()
{
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(4); return; // .Wait1
	case 2: // .OpenDoor
		set_state(wSceneObj10State, 0x04);
		play_sfx(mem_, SFX_108);
		next_step();
		return;
	case 3: wait_state(wSceneObj10State, 0); return; // .WaitAnim
	case 4: wait(4); return; // .Wait2
	case 5: finish(); return;
	default: unrealistic(); return;
	}
}

// The magic seed (NOWFunc_MagicSeed, WOWFunc_MagicSeed, SOWFunc_MagicSeed):
// the seed (scene object 4) falls to land_y and a stalk grows
void MapFunctions::magic_seed(const Seed &seed)
{
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(20); return; // .Wait1
	case 2: load_level_name(); return;
	case 3: wait_level_name(); return;
	case 4: wait(16); return; // WaitOWScene16Frames
	case 5: // .PlayFallSFX
		play_sfx(mem_, SFX_119);
		next_step();
		return;
	case 6: { // .SeedFall
		uint8_t dy = falling_offset(wOWFuncCounter);
		Byte y = mem_[wSceneObj4YCoord];
		y += dy;
		if (y < seed.land_y)
			return;
		set_state(wSceneObj4State, seed.land);
		play_sfx(mem_, SFX_014);
		next_step();
		return;
	}
	case 7: wait_state(wSceneObj4State, seed.landed); return; // .WaitSeedFall
	case 8: wait(30); return; // .Wait2
	case 9: // .GrowStalk
		set_state(wSceneObj4State, seed.grow);
		play_sfx(mem_, SFX_11A);
		next_step();
		return;
	case 10: wait_state(wSceneObj4State, seed.grown); return; // .WaitGrowStalk
	case 11: finish(); return;
	default: unrealistic(); return;
	}
}

void MapFunctions::north_magic_seed() { magic_seed({0x4c, 0x08, 0x09, 0x0a, 0x0b}); }

// The snake (NOWFunc_SummonSnake, WOWFunc_SummonSnake, SOWFunc_SummonSnake):
// the flute's notes (scene object 1) float left while flag is set, and when
// they reach notes_x the snake comes out (and, in the north, goes back)
void MapFunctions::summon_snake(const Snake &snake)
{
	// .MoveNotes: a pixel a frame, until off the screen
	if (mem_[snake.flag]) {
		uint8_t x = --mem_[wSceneObj1XCoord];
		if (static_cast<uint8_t>(x + 0x18) >= 0xf0)
			mem_[snake.flag] = 0;
	}
	uint16_t snake_state = static_cast<uint16_t>(snake.obj + SCENE_OBJ_STATE_OFFSET);
	uint16_t snake_frame = static_cast<uint16_t>(snake.obj + SCENE_OBJ_FRAME_OFFSET);
	uint8_t s = step();
	if (s == 8 && snake.in_frame >= 0) { // .WaitSnakeComeIn
		if (mem_[snake_frame] == snake.in_frame)
			next_step();
		return;
	}
	if (s == 8 || (s == 9 && snake.in_frame >= 0)) {
		finish();
		return;
	}
	switch (s) {
	case 0: next_step(); return;
	case 1: wait(snake.wait); return; // .Wait1
	case 2: load_level_name(); return;
	case 3: wait_level_name(); return;
	case 4: wait(16); return;
	case 5: // .PlayFluteSFX
		mem_[snake.flag] = 0x01;
		play_sfx(mem_, SFX_118);
		next_step();
		return;
	case 6: // .WaitNotesAndShowSnake
		if (mem_[wSceneObj1XCoord] != snake.notes_x)
			return;
		set_state(snake_state, snake.state);
		play_sfx(mem_, SFX_TEMPLE_ROCK);
		next_step();
		return;
	case 7: // .WaitSnakeComeOut
		if (mem_[snake_frame] == snake.out_frame)
			next_step();
		return;
	default: unrealistic(); return;
	}
}

void MapFunctions::north_summon_snake()
{
	summon_snake({8, wSceneObj1Unk7, 0x25, wSceneObj2, 0x0c, 0x37, 0x35});
}

// NOWFunc_FallLeaves: two leaves lift and blow away, each splitting in two
void MapFunctions::fall_leaves()
{
	move_leaf(w2d12c, wSceneObj6, wSceneObj8, w2d130, false); // .HandleLeaf1Movement
	move_leaf(w2d12e, wSceneObj7, wSceneObj9, w2d132, true); // .HandleLeaf3Movement
	move_twin(w2d130, wSceneObj8); // .HandleLeaf2Movement
	move_twin(w2d132, wSceneObj9); // .HandleLeaf4Movement
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(20); return; // .Wait1
	case 2: load_level_name(); return;
	case 3: wait_level_name(); return;
	case 4: wait(16); return;
	case 5: // .Func_b5316
		set_state(wSceneObj7State, 0x14);
		next_step();
		return;
	case 6: // .StartLeaf1MovementAndPlaySFX
		mem_[w2d12c] = 0x01;
		play_sfx(mem_, SFX_08E);
		next_step();
		return;
	case 7: wait(8); return; // .Wait2
	case 8: // .StartLeaf3Movement
		mem_[w2d12e] = 0x01;
		next_step();
		return;
	case 9: // .WaitLeafMovement
		if (!mem_[w2d12e])
			next_step();
		return;
	case 10: wait(40); return; // .Wait3
	case 11: finish(); return;
	default: unrealistic(); return;
	}
}

// .HandleLeaf1Movement / .HandleLeaf3Movement: the leaf lifts
// (Data_b6101), splits off its twin, then blows away (Data_b6196)
void MapFunctions::move_leaf(uint16_t flag, uint16_t leaf, uint16_t twin, uint16_t twin_flag, bool sfx)
{
	Byte phase = mem_[flag];
	if (phase == 1) { // .lift_leaf
		if (move(leaf, ADDR_Data_b6101))
			return;
		++phase;
		set_state(static_cast<uint16_t>(leaf + 6), 0x15);
		set_state(static_cast<uint16_t>(twin + 6), 0x15);
		mem_[twin_flag] = 0x01;
		mem_[static_cast<uint16_t>(leaf + 1)] -= 2;
		mem_[twin] = mem_[leaf];
		mem_[static_cast<uint16_t>(twin + 1)] = static_cast<uint8_t>(mem_[static_cast<uint16_t>(leaf + 1)] + 6);
		if (sfx)
			play_sfx(mem_, SFX_08E);
	} else if (phase == 2) { // .blow_away_leaf
		if (move(leaf, ADDR_Data_b6196))
			return;
		phase = 0;
	}
}

// .HandleLeaf2Movement / .HandleLeaf4Movement: the twin, along Data_b6239
void MapFunctions::move_twin(uint16_t flag, uint16_t leaf)
{
	if (!mem_[flag])
		return;
	if (move(leaf, ADDR_Data_b6239))
		return;
	mem_[flag] = 0;
}

// NOWFunc_PlayMusicBox
void MapFunctions::play_music_box()
{
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(20); return; // .Wait1
	case 2: // .PlayMusicBox
		play_sfx(mem_, wram_.ow_scene_action_param() == NOWFUNC_GOLD_MUSIC_BOX ? SFX_122 : SFX_11F);
		next_step();
		return;
	case 3: wait(254); return; // .Wait2
	case 4: finish(); return;
	default: unrealistic(); return;
	}
}

// NOWFunc_Crayon: the crayon falls, lifts and scribbles; with all seven, the
// map's colours fade in
void MapFunctions::crayon()
{
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(20); return; // .Wait1
	case 2: // .PlayFallSFX
		play_sfx(mem_, SFX_119);
		next_step();
		return;
	case 3: { // .CrayonFall, with its shadow 6 above
		uint8_t dy = falling_offset(wOWFuncCounter);
		Byte y = mem_[wSceneObj5YCoord];
		y += dy;
		mem_[wSceneObj1YCoord] = static_cast<uint8_t>(y - 6);
		if (y < 0x40)
			return;
		mem_[wSceneObj1State] = 0;
		play_sfx(mem_, SFX_014);
		next_step();
		return;
	}
	case 4: wait(4); return; // .Wait2
	case 5: // .CrayonLift
		set_state(wSceneObj5State, 0x1e);
		next_step();
		return;
	case 6: wait_state(wSceneObj5State, 0x20); return; // .WaitCrayonLift
	case 7: wait(10); return; // .Wait3
	case 8: { // .Scribble
		move(wSceneObj5, ADDR_Data_b6294);
		uint8_t scribble = mem_[wSceneObj5Unk7];
		if (scribble == 0x0a) {
			play_sfx(mem_, SFX_12D);
			return;
		}
		if (!scribble)
			next_step();
		return;
	}
	case 9: // .AdvanceIfCollectedAllCrayons
		if ((wram_.crayon_flags() & ALL_CRAYONS) == ALL_CRAYONS) {
			next_step();
			return;
		}
		wram_.unk_2d062() = 0x0c;
		wram_.ow_func_counter() = 0;
		return;
	case 10: // .LoadNewOWPal
		routines_.copy(wTempPals1, wTempBGPals, 8 * PAL_SIZE);
		SET_HL(gb_, ADDR_Pals_84500);
		SET_DE(gb_, wTempPals1);
		gb_->cpu.c = 8 * PAL_SIZE;
		gb_->cpu.b = BANK_Pals_84500;
		routines_.call(ADDR_CopyFarBytes);
		wram_.pal_config1_total_steps() = 0;
		next_step();
		return;
	case 11: // .FadePal: every 8 frames (Func_b587d)
		if (!(wram_.ow_func_counter() & 0x07))
			routines_.farcall(BANK_Func_8518c, ADDR_Func_8518c);
		return;
	case 12: wait(20); return; // .Wait4
	case 13: finish(); return;
	default: unrealistic(); return;
	}
}

// NOWFunc_BottomBar: the B button's indicator (scene object 1) rises,
// blinks, and the bottom bar opens
void MapFunctions::north_bottom_bar()
{
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(40); return; // .Wait1
	case 2: // .LoadBottomBarPal
		load_one_pal(BANK_Pals_84000, ADDR_Pals_84000, 7);
		next_step();
		return;
	case 3: { // .RaiseBBtnIndicator: a pixel every 2 frames
		if (wram_.ow_func_counter() & 1)
			return;
		Byte y = mem_[wSceneObj1];
		--y;
		if (y != 0x94)
			return;
		play_sfx(mem_, SFX_113);
		next_step();
		return;
	}
	case 4: wait(45); return; // .Wait2
	case 5: { // .BlinkBBtnIndicator
		uint8_t counter = wram_.ow_func_counter();
		mem_[wSceneObj1State] = (counter & (1 << 3)) ? 0x23 : 0x22;
		if (counter > 47)
			next_step();
		return;
	}
	case 6: // .StartOpenBottomBar
		if (wram_.ow_func_counter() < 45)
			return;
		mem_[wSceneObj1State] = 0x23;
		play_sfx(mem_, SFX_113);
		next_step();
		return;
	case 7: wait(10); return; // .Wait3
	case 8: { // .OpenBottomBar: up with the window, to 0x78
		uint8_t wy = --mem_[wWY];
		--mem_[wSceneObj1];
		--mem_[wSceneObj5YCoord];
		--mem_[wSceneObj8YCoord];
		--mem_[wSceneObj9YCoord];
		--mem_[wSceneObj10YCoord];
		if (wy == 0x78)
			next_step();
		return;
	}
	case 9: wait(60); return; // .Wait4
	case 10: finish(); return;
	default: unrealistic(); return;
	}
}

// NOWFunc_DayNightSpell.apply_attribute: the top bar's button, a 2x2 block
// of tiles and attributes from the table at block (.Func_b5827), to copy
// by HDMA (.Func_b57f4)
void MapFunctions::show_button(uint16_t block)
{
	// rows 21 and 22 take the tiles, 19 and 20 the attributes (the low
	// byte wrapping as the asm's does)
	auto copy_2x2 = [&](uint16_t dest) {
		uint8_t d = dest >> 8;
		uint8_t e = static_cast<uint8_t>(dest);
		auto at = [d](uint8_t lo) { return static_cast<uint16_t>(d << 8 | lo); };
		mem_[at(e)] = mem_[block++];
		mem_[at(static_cast<uint8_t>(e + 1))] = mem_[block++];
		e = static_cast<uint8_t>(e + TILEMAP_WIDTH);
		mem_[at(e)] = mem_[block++];
		mem_[at(static_cast<uint8_t>(e + 1))] = mem_[block++];
	};
	constexpr uint16_t BUTTON = wAttrmap + 19 * TILEMAP_WIDTH + 11;
	copy_2x2(BUTTON + 2 * TILEMAP_WIDTH);
	copy_2x2(BUTTON);

	constexpr uint16_t TILES = wAttrmap + 21 * TILEMAP_WIDTH;
	constexpr uint16_t ATTRS = wAttrmap + 19 * TILEMAP_WIDTH;
	constexpr uint16_t BG_MAP = v0BGMap0 + 30 * TILEMAP_WIDTH;
	mem_[wHDMASourceHi] = static_cast<uint8_t>(TILES >> 8);
	mem_[wHDMASourceLo] = static_cast<uint8_t>(TILES);
	mem_[wHDMADestHi] = static_cast<uint8_t>((BG_MAP >> 8) - 0x80);
	mem_[wHDMADestLo] = static_cast<uint8_t>(BG_MAP);
	mem_[wHDMAMode] = 2 * 2 - 1; // 2 dma_rows
	mem_[w2d0b5SourceHi] = static_cast<uint8_t>(ATTRS >> 8);
	mem_[w2d0b5SourceLo] = static_cast<uint8_t>(ATTRS);
	mem_[w2d0b5DestHi] = static_cast<uint8_t>((BG_MAP >> 8) - 0x80);
	mem_[w2d0b5DestLo] = static_cast<uint8_t>(BG_MAP);
	mem_[w2d0b5Mode] = 2 * 2 - 1;
}

// NOWFunc_DayNightSpell: the top bar opens, its button blinks in, and
// it's pressed twice: day, then night
void MapFunctions::day_night_spell()
{
	static constexpr uint16_t VISIBLE = ADDR_NOWFunc_DayNightSpell + 0xe3; // .visible_button_attr
	static constexpr uint16_t INVISIBLE = ADDR_NOWFunc_DayNightSpell + 0xeb; // .invisible_button_attr
	static constexpr uint16_t PRESSED = ADDR_NOWFunc_DayNightSpell + 0xf3; // .pressed_button_attr

	auto button = [this](uint8_t frames, uint8_t state, uint16_t block) {
		if (wram_.ow_func_counter() < frames)
			return;
		set_state(wSceneObj5State, state);
		if (block == PRESSED) // .play_sfx_and_press_button
			play_sfx(mem_, SFX_SELECTION);
		show_button(block);
		next_step();
	};
	auto pressed = [this](uint8_t frame) { // .WaitDayButtonPress / .WaitNightButtonPress
		if (mem_[wSceneObj5Frame] != frame)
			return;
		show_button(VISIBLE);
		next_step();
	};
	auto transition = [this] { // .DayNightTransition
		routines_.farcall(BANK_Func_84c0d, ADDR_Func_84c0d);
		if (mem_[wOWPalTransitionState])
			return;
		gb_->cpu.ime = false; // di
		routines_.farcall(BANK_VBlank_Overworld, ADDR_VBlank_Overworld);
		gb_->cpu.ime = true; // ei
		gb_->cpu.ime_delay = 0;
		next_step();
	};

	uint8_t s = step();
	if (s == 3) { // .StartOpenTopBar
		play_sfx(mem_, SFX_0E9);
		next_step();
		s = 4;
	}
	switch (s) {
	case 0: next_step(); return;
	case 1: wait(30); return; // .Wait1
	case 2: wait_level_name(); return;
	case 4: { // .OpenTopBar: 2 a frame
		Byte scy = mem_[wSCY];
		scy -= 2;
		if (scy == 0xec)
			next_step();
		return;
	}
	case 5: wait(40); return; // .Wait2
	case 6: case 8: case 10: button(20, 0x25, VISIBLE); return; // .SetButtonVisible
	case 7: case 9: button(20, 0x00, INVISIBLE); return; // .SetButtonInvisible
	case 11: button(60, 0x26, PRESSED); return; // .WaitAndPressDayButton
	case 12: pressed(0x04); return;
	case 13: case 16: transition(); return;
	case 14: button(60, 0x28, PRESSED); return; // .WaitAndPressNightButton
	case 15: pressed(0x06); return;
	case 17: wait(60); return; // .Wait3
	case 18: finish(); return;
	default: unrealistic(); return;
	}
}

// NOWFunc_Prologue: the map's prologue: the ground shakes, the temple
// (scene object 1) rises, and the music box notes fly out one by one, the
// wind blowing every 0x30 frames
void MapFunctions::prologue()
{
	auto note = [this](uint16_t flag) { // .asm_b59b9
		if (wram_.ow_func_counter() < 6)
			return;
		mem_[flag] = 0x01;
		next_step();
	};

	shake();
	routines_.farcall(BANK_Func_15868e, ADDR_Func_15868e);
	if (mem_[wOWUIObj2Attributes]) { // .Func_b59e5
		Byte wind = mem_[wOWUIObj2Duration];
		++wind;
		if (wind == 0x01)
			play_sfx(mem_, SFX_08E);
		else if (wind >= 0x30)
			wind = 0;
	}
	switch (step()) {
	case 0: next_step(); return;
	case 1: // .Func_b5949
		if (wram_.ow_func_counter() < 60)
			return;
		mem_[wOWUIObj1YCoord] = 0x01;
		quake(true);
		next_step();
		return;
	case 2: // .Func_b5965
		if (wram_.ow_func_counter() < 134)
			return;
		mem_[wOWUIObj1YCoord] = 0x04;
		next_step();
		return;
	case 3: // .Func_b5973
		if (mem_[wOWUIObj1YCoord] != 0x05)
			return;
		set_state(wSceneObj1State, 0x2b);
		play_sfx(mem_, SFX_061);
		next_step();
		return;
	case 4: // .Func_b598c
		if (mem_[wOWUIObj1YCoord])
			return;
		quake(false);
		next_step();
		return;
	case 5: wait(86); return; // .Func_b59a0
	case 6: // .Func_b59a5
		if (wram_.ow_func_counter() < 6)
			return;
		mem_[w2d150] = 0x01;
		mem_[wOWUIObj2Attributes] = 0x01;
		next_step();
		return;
	case 7: note(wd153); return; // .Func_b59ca
	case 8: note(wd152); return; // .Func_b59c5
	case 9: note(wd155); return; // .Func_b59d4
	case 10: note(wd154); return; // .Func_b59cf
	case 11: note(w2d151); return; // .Func_b59b6
	case 12: case 13: wait(120); return; // .Func_b59d9
	case 14: // .Func_b59de
		mem_[wOWUIObj2Attributes] = 0;
		finish();
		return;
	default: unrealistic(); return;
	}
}

// ---------------------------------------------------------------------------
// The quakes' shaking

// Func_b71fc: the screen's shaking for the volcanoes, quakes and the
// prologue. wOWUIObj1YCoord picks the script's phase (counting frames in
// wOWUIObj1XCoord): start a run of shake steps at a step, then wait for
// it to come back to one (.Func_b7273 runs them).
void MapFunctions::shake()
{
	shake_steps();
	Byte phase = mem_[wOWUIObj1YCoord];
	if (!phase)
		return;
	++mem_[wOWUIObj1XCoord];
	auto start = [&](uint8_t step) { // .asm_b7221
		mem_[wOWUIObj1Frame] = step;
		mem_[wOWUIObj1Attributes] = 0;
		++phase;
		mem_[wOWUIObj1XCoord] = 0;
	};
	auto stop_at = [&](uint8_t step) { // .Func_b7231 / .Func_b7249
		if (mem_[wOWUIObj1Frame] != step)
			return;
		mem_[wOWUIObj1Frame] = 0;
		phase = 0;
	};
	switch (phase) {
	case 1: start(0x01); return; // .Func_b721f: the small shake
	case 2: return; // .Func_b721e: on until told
	case 3: stop_at(0x01); return; // .Func_b7231
	case 4: // .Func_b723f: the big one, at the small one's end
		if (mem_[wOWUIObj1Frame] == 0x01)
			start(0x11);
		return;
	case 5: case 7: stop_at(0x11); return; // .Func_b7249
	case 6: start(0x11); return; // .Func_b7245
	case 8: start(0x15); return; // .Func_b7257: side to side
	case 9: case 11: // .Func_b725b / .Func_b7269: once it's done
		if (!mem_[wOWUIObj1Frame])
			phase = 0;
		return;
	case 10: start(0x1e); return; // .Func_b7265: the small side to side
	default: unrealistic(); return;
	}
}

// Func_b71fc.Func_b7273: the shake's steps, from wOWUIObj1Frame (counting
// frames in wOWUIObj1Attributes): moves of the scroll, and waits (some
// looping back)
void MapFunctions::shake_steps()
{
	struct Step {
		int8_t scy, scx;
		uint16_t sfx; // or 0
		uint8_t wait; // frames, for a wait (or 0)
		uint8_t back; // steps a wait loops back
		bool end; // the shake stops after
	};
	static constexpr Step STEPS[] = {
		{-1, 0, SFX_01F, 0, 0, false}, // .Func_b72f8: up and down a pixel
		{0, 0, 0, 1, 0, false}, // .Func_b72e0
		{1, 0, 0, 0, 0, false}, // .Func_b730b
		{0, 0, 0, 1, 0, false},
		{-1, 0, 0, 0, 0, false}, // .Func_b7300
		{0, 0, 0, 1, 0, false},
		{1, 0, 0, 0, 0, false},
		{0, 0, 0, 1, 0, false},
		{-1, 0, 0, 0, 0, false},
		{0, 0, 0, 1, 0, false},
		{1, 0, 0, 0, 0, false},
		{0, 0, 0, 1, 0, false},
		{-1, 0, 0, 0, 0, false},
		{0, 0, 0, 1, 0, false},
		{1, 0, 0, 0, 0, false},
		{0, 0, 0, 1, 0x10, false}, // .Func_b72e8: again
		{-4, 0, SFX_01F, 0, 0, false}, // .Func_b7311: up and down 4
		{0, 0, 0, 3, 0, false}, // .Func_b72c8
		{4, 0, 0, 0, 0, false}, // .Func_b7322
		{0, 0, 0, 3, 4, false}, // .Func_b72d0: again
		{0, 4, SFX_063, 0, 0, false}, // .Func_b733b: side to side by 8
		{0, 0, 0, 3, 0, false},
		{0, -8, 0, 0, 0, false}, // .Func_b7379
		{0, 0, 0, 3, 0, false},
		{0, 8, 0, 0, 0, false}, // .Func_b7387
		{0, 0, 0, 3, 0, false},
		{0, -8, 0, 0, 0, false},
		{0, 0, 0, 3, 0, false},
		{0, 4, 0, 0, 0, true}, // .Func_b734c
		{0, 1, SFX_063, 0, 0, false}, // .Func_b7363: side to side by 2
		{0, 0, 0, 3, 0, false},
		{0, -2, 0, 0, 0, false}, // .Func_b7334
		{0, 0, 0, 3, 0, false},
		{0, 2, 0, 0, 0, false}, // .Func_b735c
		{0, 0, 0, 3, 0, false},
		{0, -2, 0, 0, 0, false},
		{0, 0, 0, 3, 0, false},
		{0, 1, 0, 0, 0, true}, // .Func_b7371
	};
	Byte step = mem_[wOWUIObj1Frame];
	if (!step)
		return;
	Byte frames = mem_[wOWUIObj1Attributes];
	++frames;
	uint8_t i = step - 1;
	if (i >= sizeof(STEPS) / sizeof(STEPS[0])) {
		unrealistic();
		return;
	}
	const Step &s = STEPS[i];
	if (s.wait) {
		if (frames < s.wait)
			return;
		step -= s.back;
	} else {
		if (s.sfx)
			play_sfx(mem_, s.sfx);
		mem_[wSCY] += static_cast<uint8_t>(s.scy);
		mem_[wSCX] += static_cast<uint8_t>(s.scx);
	}
	frames = 0; // .asm_b7304
	++step;
	if (s.end)
		step = 0;
}

// ---------------------------------------------------------------------------
// The west side

// Func_b5868: a block of the map's tiles to update (Func_854ee's data)
void MapFunctions::tile_update(uint16_t data)
{
	mem_[wOWPendingTileUpdate] = TRUE;
	SET_HL(gb_, data);
	routines_.farcall(BANK_Func_854ee, ADDR_Func_854ee);
}

// WOWFunc_Rain: lightning flashes, the clouds rain and the pool fills
void MapFunctions::rain()
{
	rain_pals();
	// .PlayThunderSFX: every 0x1a frames while w2d13f
	Byte thunder = mem_[w2d13f];
	if (thunder) {
		++thunder;
		if (thunder >= 0x1b) {
			play_sfx(mem_, SFX_076);
			thunder = 0x01;
		}
	}
	switch (step()) {
	case 0: next_step(); return;
	case 1: // .InitPalConfig
		wram_.pal_config1_total_steps() = 0x5a;
		mem_[w2d13f] = 0x01;
		next_step();
		return;
	case 2: wait(180); return; // .Wait1
	case 3: // .ShowRainClouds
		set_state(wSceneObj15State, 0x01);
		next_step();
		return;
	case 4: // .WaitRainClouds
		if (mem_[wSceneObj15Frame] == 0x0e)
			next_step();
		return;
	case 5: // .FillPool
		if (wram_.ow_func_counter() < 2)
			return;
		tile_update(ADDR_Data_8550b);
		next_step();
		return;
	case 6: wait(120); return; // .Wait2
	case 7: { // .WaitRainAndHideClouds
		Byte steps = wram_.pal_config1_total_steps();
		if (steps < 0x3c)
			return;
		steps = 0;
		mem_[w2d13f] = 0;
		set_state(wSceneObj14State, 0x04);
		next_step();
		return;
	}
	case 8: wait_state(wSceneObj14State, 0); return; // .WaitCloudsDisappear
	case 9: wait(60); return; // .Wait3
	case 10: finish(); return;
	default: unrealistic(); return;
	}
}

// WOWFunc_Rain.UpdatePals: the lightning's flashes, counted in
// wPalConfig1TotalSteps (from 1 to 0x81, again)
void MapFunctions::rain_pals()
{
	Byte steps = wram_.pal_config1_total_steps();
	if (!steps)
		return;
	++steps;
	uint16_t pals;
	switch (steps) {
	case 0x09: case 0x16: case 0x1e: case 0x26: pals = ADDR_Pals_8698d; break; // .pals_flash2
	case 0x12: case 0x1a: case 0x22: case 0x29: pals = ADDR_Pals_8694d; break; // .pals_flash1
	case 0x2c: pals = ADDR_Pals_84140; break; // .pals_regular
	default:
		if (steps >= 0x82)
			steps = 0x01;
		return;
	}
	// .apply_pals: the first BG palette to load
	SET_HL(gb_, pals);
	SET_DE(gb_, wTempPals1);
	gb_->cpu.b = BANK_Pals_84140;
	gb_->cpu.c = 8 * PAL_SIZE;
	routines_.call(ADDR_CopyFarBytes);
	pal_config(wPalConfig1, static_cast<uint8_t>(rBGPI), wTempPals1);
}

// UpdateOWTornado: the tornado's spinning, with its sound when the map is
// idle
void MapFunctions::update_tornado()
{
	if (move(wSceneObj1, ADDR_Data_b6337))
		return;
	if (wram_.cur_event() || wram_.ow_bars_state())
		return;
	play_sfx(mem_, SFX_11D);
}

// WOWFunc_ClearTornado
void MapFunctions::clear_tornado()
{
	switch (step()) {
	case 0: next_step(); return;
	case 1: { // .ShowTornado
		update_tornado();
		uint8_t moving = mem_[wSceneObj1Unk7];
		if (moving == 0x01) {
			play_sfx(mem_, SFX_11D);
			return;
		}
		if (!moving)
			next_step();
		return;
	}
	case 2: // .ClearTornado
		if (move(wSceneObj1, ADDR_Data_b6478))
			return;
		next_step();
		return;
	case 3: // .HideTornado
		set_state(wSceneObj1State, 0x00);
		next_step();
		return;
	case 4: wait(20); return; // .Wait1
	case 5: finish(); return;
	default: unrealistic(); return;
	}
}

// WOWFunc_ElevatorWorking
void MapFunctions::elevator_working()
{
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(20); return; // .Wait1
	case 2: // .SetElevatorWorking
		set_state(wSceneObj8State, 0x06);
		next_step();
		return;
	case 3: wait(200); return; // .Wait2
	case 4: finish(); return;
	default: unrealistic(); return;
	}
}

void MapFunctions::west_magic_seed() { magic_seed({0x5b, 0x0b, 0x0c, 0x0d, 0x0e}); }

void MapFunctions::west_summon_snake()
{
	summon_snake({8, wSceneObj1Unk7, 0x26, wSceneObj5, 0x0f, 0x0a, -1});
}

// The shared palette and shaking steps

// wPalConfig1/2: load the first palette from src (to rBGPI or rOBPI)
void MapFunctions::pal_config(uint16_t config, uint8_t reg, uint16_t src)
{
	mem_[config] = reg;
	mem_[config + 1] = BGPI_AUTOINC | 0;
	mem_[config + 2] = PAL_SIZE;
	mem_[config + 3] = static_cast<uint8_t>((src) >> 8);
	mem_[config + 4] = static_cast<uint8_t>(src);
}

// CopyFarBytes and wPalConfig1: BG palette pal from bank:pals (a table of
// 8) to wTempPals1's, to load
void MapFunctions::load_one_pal(uint8_t bank, uint16_t pals, uint8_t pal)
{
	uint16_t dest = static_cast<uint16_t>(wTempPals1 + pal * PAL_SIZE);
	SET_HL(gb_, static_cast<uint16_t>(pals + pal * PAL_SIZE));
	SET_DE(gb_, dest);
	gb_->cpu.b = bank;
	gb_->cpu.c = PAL_SIZE;
	routines_.call(ADDR_CopyFarBytes);
	mem_[wPalConfig1] = static_cast<uint8_t>(rBGPI);
	mem_[wPalConfig1Index] = BGPI_AUTOINC | (pal * PAL_SIZE);
	mem_[wPalConfig1Number] = 1;
	mem_[wPalConfig1SourceHi] = static_cast<uint8_t>(dest >> 8);
	mem_[wPalConfig1SourceLo] = static_cast<uint8_t>(dest);
}

// Func_b69a6: colours at colours to the flash's white
void MapFunctions::flash(uint16_t colours, uint8_t count)
{
	do {
		mem_[colours++] = 0x9c;
		mem_[colours++] = 0x73;
	} while (--count);
}

// The volcano and the earthquakes' shaking (Func_b71fc runs it): the
// shaking object's script, the two scene objects' and w2d03e's quake
void MapFunctions::start_shaking(uint8_t script)
{
	mem_[wOWUIObj1YCoord] = script;
	set_state(wSceneObj11State, 0x17);
	set_state(wSceneObj10State, 0x19);
	mem_[w2d049] = 0x01;
	mem_[w2d03e] = 0x03;
	mem_[w2d03e + 1] = 0;
	mem_[w2d03e + 2] = 0;
}

void MapFunctions::stop_shaking()
{
	set_state(wSceneObj10State, 0x1a);
	mem_[w2d03e] = 0x0e;
	mem_[w2d03e + 1] = 0;
	mem_[w2d03e + 2] = 0;
	mem_[w2d049] = 0;
}

// Func_b6055: the thunder every 0x1a frames while wOWUIObj2Attributes
void MapFunctions::rumble()
{
	if (!mem_[wOWUIObj2Attributes])
		return;
	Byte frames = mem_[wOWUIObj2Duration];
	++frames;
	if (frames < 0x1a)
		return;
	frames = 0;
	play_sfx(mem_, SFX_076);
}

// A flying rock (the volcano's): flag counts 1, put it at the start, 2 (or 4
// for the far ones), its fall, until it's off the bottom. Odd values put
// it at places[value / 2], even ones move it dx and 2 or 4 down.
void MapFunctions::fly_rock(const Rock &rock)
{
	Byte phase = mem_[rock.flag];
	if (!phase)
		return;
	uint8_t i = phase - 1;
	if (i >= rock.phases) {
		if (rock.phases == 8)
			unrealistic();
		return;
	}
	if (!(i & 1)) { // place it
		const Place &place = rock.places[i / 2];
		mem_[rock.obj] = place.y;
		mem_[rock.obj + 1] = place.x;
		set_state(static_cast<uint16_t>(rock.obj + SCENE_OBJ_STATE_OFFSET), place.state);
		mem_[rock.flag + 1] = 0;
		++phase;
		return;
	}
	Byte y = mem_[rock.obj];
	Byte x = mem_[rock.obj + 1];
	x += static_cast<uint8_t>(rock.dx);
	y += (i & 2) ? 4 : 2;
	if (y < 0x88)
		return;
	mem_[rock.flag + 1] = 0;
	phase = 0;
}

// WOWFunc_Earthquake: the ground shakes, the cliffs crumble one by one (in
// wOWUIObj1Duration, every 10 frames) and rocks drop
void MapFunctions::west_earthquake()
{
	shake();
	crumble_cliffs();
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(20); return; // .Wait1
	case 2: load_level_name(); return;
	case 3: wait_level_name(); return;
	case 4: wait(16); return;
	case 5: // .Func_b5c94
		mem_[wOWUIObj1Duration] = 0x01;
		start_shaking(0x01);
		next_step();
		return;
	case 6: wait(120); return; // .WaitShake
	case 7: // .Func_b5cc4
		mem_[wOWUIObj1Duration] = 0;
		mem_[wOWUIObj1YCoord] = 0x03;
		set_state(wSceneObj11State, 0x18);
		stop_shaking();
		next_step();
		return;
	case 8: wait(60); return; // .WaitPostShake
	case 9: // .DropRocks
		set_state(wSceneObj13State, 0x16);
		tile_update(ADDR_Data_85549);
		play_sfx(mem_, SFX_061);
		next_step();
		return;
	case 10: wait(60); return; // .Func_b5c8f
	case 11: finish(); return;
	default: unrealistic(); return;
	}
}

// WOWFunc_Earthquake.Func_b5d0a: the cliffs' crumbling, a step every 10
// frames (counted in wOWUIObj1FramesetOffset), then again
void MapFunctions::crumble_cliffs()
{
	static constexpr struct {
		uint16_t state;
		uint8_t value;
	} CLIFFS[] = {
		{wSceneObj13State, 0x14}, // .Func_b5d25
		{wSceneObj15State, 0x15}, // .Func_b5d34
		{wSceneObj14State, 0x14}, // .Func_b5d3e
		{wSceneObj12State, 0x15}, // .Func_b5d48
	};
	Byte phase = mem_[wOWUIObj1Duration];
	if (!phase)
		return;
	Byte frames = mem_[wOWUIObj1FramesetOffset];
	++frames;
	uint8_t i = phase - 1;
	if (i == 8) { // .Func_b5d5a
		phase = 0x01;
		frames = 0;
		return;
	}
	if (i > 8) {
		unrealistic();
		return;
	}
	if (!(i & 1))
		set_state(CLIFFS[i / 2].state, CLIFFS[i / 2].value);
	else if (frames < 0x0a) // .Func_b5d52
		return;
	frames = 0;
	++phase;
}

// ---------------------------------------------------------------------------
// The west side's volcano

// WOWFunc_VulcanoEruption: the volcano shakes, erupts with a flash, and
// throws rocks
void MapFunctions::west_volcano()
{
	static constexpr Rock LEFT_ROCK = {w2d122, wSceneObj1, -2, 8,
		{{8, 46, 0x1c}, {8, 75, 0x1c}, {8, 46, 0x1f}, {8, 75, 0x1f}}};
	static constexpr Rock RIGHT_ROCK = {w2d126, wSceneObj3, 2, 8,
		{{8, 98, 0x1d}, {8, 93, 0x1d}, {8, 98, 0x20}, {8, 93, 0x20}}};
	static constexpr Rock BIG_ROCK = {w2d12c, wSceneObj6, -2, 2, {{8, 0x2e, 0x1c}}};

	shake();
	fly_rock(LEFT_ROCK); // .Func_b5f58
	fly_rock(RIGHT_ROCK); // .Func_b5fb9
	fly_rock(BIG_ROCK); // .Func_b601a
	rumble();

	// .Func_b5eb7 to .Func_b5efb: after frames, a rock, with a crash
	auto throw_rock = [this](uint8_t frames, uint16_t flag, uint8_t phase) {
		if (wram_.ow_func_counter() < frames)
			return;
		mem_[flag] = phase;
		play_sfx(mem_, SFX_TEMPLE_ROCK);
		next_step();
	};

	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(60); return; // .Wait1
	case 2: // .Func_b5dac
		start_shaking(0x01);
		next_step();
		return;
	case 3: wait(58); return; // .Func_b5dd2
	case 4: // .Func_b5dd7: the smoke, and palette 5's colour 0
		set_state(wSceneObj11State, 0x1f);
		mem_[wSceneObj11] = 0x10;
		mem_[wSceneObj11 + 1] = 0x52;
		mem_[wPalConfig1Index] = BGPI_AUTOINC | (5 * PAL_SIZE);
		mem_[wPalConfig1Number] = 1;
		mem_[wPalConfig1SourceHi] = 0xc1;
		mem_[wPalConfig1SourceLo] = 0x28;
		next_step();
		return;
	case 5: // .Func_b5dfa
		if (!mem_[wSceneObj11State])
			next_step();
		return;
	case 6: // .Func_b5e02
		if (wram_.ow_func_counter() < 6)
			return;
		mem_[wOWUIObj1YCoord] = 0x04;
		next_step();
		return;
	case 7: // .Func_b5e10: the flash
		stop_shaking();
		flash(wTempBGPals, 3 * PAL_SIZE);
		routines_.copy(wTempPals1 + 6 * PAL_SIZE, wTempBGPals + 6 * PAL_SIZE, PAL_SIZE);
		flash(wTempBGPals + 7 * PAL_SIZE, 4);
		flash(wTempOBPals, 4 * PAL_SIZE);
		pal_config(wPalConfig1, static_cast<uint8_t>(rBGPI), wTempBGPals);
		pal_config(wPalConfig2, static_cast<uint8_t>(rOBPI), wTempOBPals);
		mem_[wOWUIObj2Attributes] = 0x01;
		next_step();
		return;
	case 8: // .Func_b5e73: the eruption
		if (wram_.ow_func_counter() < 8)
			return;
		set_state(wSceneObj11State, 0x1b);
		mem_[wSceneObj11] = 0x10;
		mem_[wSceneObj11 + 1] = 0x54;
		tile_update(ADDR_Data_85536);
		pal_config(wPalConfig1, static_cast<uint8_t>(rBGPI), wTempPals1);
		pal_config(wPalConfig2, static_cast<uint8_t>(rOBPI), wTempPals2);
		next_step();
		return;
	case 9: throw_rock(0, w2d126, 0x03); return; // .Func_b5eb7
	case 10: throw_rock(12, w2d122, 0x03); return; // .Func_b5ec7
	case 11: throw_rock(36, w2d12c, 0x01); return; // .Func_b5ed4
	case 12: throw_rock(12, w2d126, 0x03); return; // .Func_b5ee1
	case 13: throw_rock(20, w2d122, 0x03); return; // .Func_b5eee
	case 14: throw_rock(38, w2d126, 0x01); return; // .Func_b5efb
	case 15: // .Func_b5f08
		if (!mem_[w2d126])
			next_step();
		return;
	case 16: wait(16); return; // .Func_b5f10
	case 17: // .Func_b5f15
		mem_[wOWUIObj1YCoord] = 0x08;
		set_state(wSceneObj10State, 0x19);
		mem_[w2d049] = 0x01;
		mem_[w2d03e] = 0x03;
		mem_[w2d03e + 1] = 0;
		mem_[w2d03e + 2] = 0;
		next_step();
		return;
	case 18: // .Func_b5f33
		if (mem_[wOWUIObj1Frame] != 0x1b)
			return;
		stop_shaking();
		next_step();
		return;
	case 19: // .Func_b5f50
		if (!mem_[wOWUIObj1YCoord])
			next_step();
		return;
	case 20: wait(24); return; // .Func_b5da7
	case 21: finish(); return;
	default: unrealistic(); return;
	}
}

// WOWFunc_RedMusicBox: the music, then the storm (thunder every 0x1a frames
// while wSceneObj6Unk7) over the tower
void MapFunctions::red_music_box()
{
	// .Func_b608a
	Byte thunder = mem_[wSceneObj6Unk7];
	if (thunder) {
		if (thunder == 0x01)
			play_sfx(mem_, SFX_076);
		++thunder;
		if (thunder >= 0x1b)
			thunder = 0x01;
	}
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(20); return; // .Wait1
	case 2: // .Func_b60aa
		play_sfx(mem_, SFX_121);
		next_step();
		return;
	case 3: wait(254); return; // .Func_b60b5
	case 4: // .Func_b60ba
		set_state(wSceneObj6State, 0x20);
		tile_update(ADDR_Data_8556e);
		mem_[wSceneObj6Unk7] = 0x01;
		next_step();
		return;
	case 5: wait(23); return; // .Func_b60d0
	case 6: // .Func_b60d5
		set_state(wSceneObj6State, 0x21);
		--mem_[wSceneObj6];
		next_step();
		return;
	case 7: wait(10); return; // .Func_b60e4
	case 8: // .Func_b60e9
		set_state(wSceneObj6State, 0x22);
		next_step();
		return;
	case 9: // .Func_b60f4
		if (wram_.ow_func_counter() < 106)
			return;
		mem_[wSceneObj6Unk7] = 0;
		next_step();
		return;
	case 10: finish(); return;
	default: unrealistic(); return;
	}
}

// ---------------------------------------------------------------------------
// The south side

void MapFunctions::south_magic_seed() { magic_seed({0x56, 0x06, 0x03, 0x02, 0x01}); }

void MapFunctions::south_summon_snake()
{
	summon_snake({20, wSceneObj7Unk7, 0x08, wSceneObj7, 0x13, 0x31, -1});
}

// SOWFunc_YellowMusicBox: the music, then the bridge lowers
void MapFunctions::yellow_music_box()
{
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(20); return; // .Wait1
	case 2: // .PlayMusicBoxSFX
		play_sfx(mem_, SFX_11E);
		next_step();
		return;
	case 3: wait(254); return; // .WaitMusic
	case 4: // .LowerBridge
		play_sfx(mem_, SFX_123);
		set_state(wSceneObj6State, 0x08);
		next_step();
		return;
	case 5: wait_state(wSceneObj6State, 0x09); return; // .ShowLoweredBridge
	case 6: wait(40); return; // .Wait2
	case 7: finish(); return;
	default: unrealistic(); return;
	}
}

// CopyFarBytes: 8 palettes from bank:src to dest
void MapFunctions::load_pals(uint8_t bank, uint16_t src, uint16_t dest)
{
	SET_HL(gb_, src);
	SET_DE(gb_, dest);
	gb_->cpu.b = bank;
	gb_->cpu.c = 8 * PAL_SIZE;
	routines_.call(ADDR_CopyFarBytes);
}

// SOWFunc_CanyonThunder: lightning strikes the canyon, flashing, then a
// run of bolts (canyon_bolts) and the canyon's new palettes
void MapFunctions::canyon_thunder()
{
	// .asm_b67d6: a strike, starting the flash
	auto strike = [this](uint16_t state, uint8_t value) {
		set_state(state, value);
		wram_.pal_config1_total_steps() = 0x01;
		canyon_flash();
		play_sfx(mem_, SFX_124);
		next_step();
	};

	canyon_bolts();
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(34); return; // .Func_b67c7
	case 2: strike(wSceneObj2State, 0x12); return; // .Func_b67d1
	case 3: // .Func_b67ec
		canyon_flash();
		wait(16);
		return;
	case 4: strike(wSceneObj10State, 0x12); return; // .Func_b67f4
	case 5: // .Func_b67f9
		canyon_flash();
		wait(54);
		return;
	case 6: // .Func_b6801
		mem_[wSceneObj2YCoord] = 0x56;
		mem_[wSceneObj2XCoord] = 0x47;
		strike(wSceneObj2State, 0x17);
		return;
	case 7: // .Func_b6810
		canyon_flash();
		if (!wram_.pal_config1_total_steps())
			next_step();
		return;
	case 8: wait(30); return; // .Func_b681b
	case 9: // .Func_b6820
		mem_[w2d124] = 0x01;
		next_step();
		return;
	case 10: // .Func_b6828
		if (mem_[w2d124] >= 0x07)
			next_step();
		return;
	case 11: wait(7); return; // .Func_b6831
	case 12: // .Func_b6971: the faded palettes, for Func_85145 to fade to
		routines_.copy(wTempPals1, wTempBGPals, 8 * PAL_SIZE);
		routines_.copy(wTempPals2, wTempOBPals, 8 * PAL_SIZE);
		flash(wTempPals1, 3 * PAL_SIZE);
		flash(wTempPals1 + 7 * PAL_SIZE, 4);
		flash(wTempPals2, 4 * PAL_SIZE);
		wram_.pal_config1_total_steps() = 0;
		next_step();
		return;
	case 13: // Func_b588d
		routines_.farcall(BANK_Func_85145, ADDR_Func_85145);
		return;
	case 14: // .Func_b6836
		set_state(wSceneObj5State, 0x0a);
		tile_update(ADDR_Data_855b2);
		next_step();
		return;
	case 15: wait(45); return; // .Func_b6847
	case 16: // .Func_b684c
		load_pals(BANK_Pals_84280, ADDR_Pals_84280, wTempPals1);
		load_pals(BANK_Pals_84980, ADDR_Pals_84980, wTempPals2);
		next_step();
		return;
	case 17: // .Func_b6968: every other frame
		if (wram_.ow_func_counter() & 1)
			routines_.farcall(BANK_Func_85145, ADDR_Func_85145);
		return;
	case 18: wait(90); return; // .Func_b67cc
	case 19: finish(); return;
	default: unrealistic(); return;
	}
}

// SOWFunc_CanyonThunder.Func_b6879: a strike's flash, by
// wPalConfig1TotalSteps: white, the flash, then back (or, in step 7, the
// white again)
void MapFunctions::canyon_flash()
{
	Byte steps = wram_.pal_config1_total_steps();
	uint8_t old = steps;
	if (!old)
		return;
	++steps;
	if (old == 0x01) {
		canyon_pals(ADDR_Pals_8698d); // .flash1_pal
	} else if (old == 0x05) {
		canyon_white(); // .flash2_pal
	} else if (old == 0x09) {
		steps = 0;
		canyon_pals(step() == 0x07 ? ADDR_Pals_8698d : ADDR_Pals_84280);
	}
}

// SOWFunc_CanyonThunder.Func_b6869: a bolt's flash
void MapFunctions::bolt_flash()
{
	Byte steps = wram_.pal_config1_total_steps();
	uint8_t old = steps;
	if (!old)
		return;
	++steps;
	if (old == 0x05)
		canyon_white(); // .flash2_pal
	else if (old == 0x09)
		canyon_pals(ADDR_Pals_8698d); // .flash1_pal
}

// .got_pal
void MapFunctions::canyon_pals(uint16_t pals)
{
	load_pals(BANK_Pals_84280, pals, wTempBGPals);
	pal_config(wPalConfig1, static_cast<uint8_t>(rBGPI), wTempBGPals);
}

// .flash2_pal
void MapFunctions::canyon_white()
{
	flash(wTempBGPals, 3 * PAL_SIZE);
	flash(wTempBGPals + 7 * PAL_SIZE, 4);
	pal_config(wPalConfig1, static_cast<uint8_t>(rBGPI), wTempBGPals);
}

// SOWFunc_CanyonThunder.Func_b68cd: the bolts, while w2d124 (counting
// frames in w2d125): a strike, then 8 frames of its flash, four times, then
// the last
void MapFunctions::canyon_bolts()
{
	static constexpr struct {
		uint16_t obj;
		uint8_t y, x, state;
	} BOLTS[] = {
		{wSceneObj2, 0x50, 0x38, 0x17}, // .Func_b68ee
		{wSceneObj1, 0x48, 0x46, 0x12}, // .Func_b692e
		{wSceneObj2, 0x42, 0x2e, 0x17}, // .Func_b6938
		{wSceneObj1, 0x3e, 0x38, 0x12}, // .Func_b6942
		{wSceneObj10, 0x3b, 0x1f, 0x12}, // .Func_b694c
	};
	Byte bolt = mem_[w2d124];
	if (!bolt)
		return;
	Byte frames = mem_[w2d125];
	++frames;
	uint8_t i = bolt - 1;
	if (i == 10) { // .Func_b6960
		bolt = 0;
		frames = 0;
		return;
	}
	if (i > 10) {
		unrealistic();
		return;
	}
	if (!(i & 1)) { // .asm_b68f6
		const auto &b = BOLTS[i / 2];
		mem_[b.obj] = b.y;
		mem_[b.obj + 1] = b.x;
		set_state(static_cast<uint16_t>(b.obj + SCENE_OBJ_STATE_OFFSET), b.state);
		if (i != 8) {
			wram_.pal_config1_total_steps() = 0x01;
			bolt_flash();
		}
		play_sfx(mem_, SFX_124); // .asm_b6908
	} else {
		if (i != 9) // .Func_b6919 (.Func_b6925 doesn't flash)
			bolt_flash();
		if (frames <= 0x07)
			return;
	}
	++bolt; // .asm_b6910
	frames = 0;
}

// SOWFunc_RaiseTower.Func_b6a16 / .Func_b6a2e: the tower (scene object 3)
// bobbing as it rises, by wGlobalCounter: down a pixel every period + 1
// frames, else up one on the frames step_mask clears. zero is the routines'
// z flag (down to 0, or up to 0x48); a, their a.
MapFunctions::Bob MapFunctions::bob_tower(uint8_t period, uint8_t step_mask)
{
	Byte y = mem_[wSceneObj3YCoord];
	uint8_t c = wram_.global_counter() & period;
	if (!c) { // .asm_b6a28
		uint8_t old = y;
		++y;
		return {y == 0, old};
	}
	if (c & step_mask)
		return {false, static_cast<uint8_t>(c & step_mask)};
	--y;
	return {y == 0x48, y};
}

// SOWFunc_RaiseTower: dust, and the tower rises in three steps
void MapFunctions::raise_tower()
{
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(30); return; // .Wait1
	case 2: // .StartDust
		set_state(wSceneObj2State, 0x0b);
		play_sfx(mem_, SFX_11B);
		next_step();
		return;
	case 3: wait(45); return; // .Wait2
	case 4: case 5: case 6: // .RaiseTower
		if (!bob_tower(0x07, 0x01).zero)
			return;
		mem_[wSceneObj3YCoord] += 0x10;
		set_state(wSceneObj3State, static_cast<uint8_t>(mem_[wSceneObj3State] + 1));
		next_step();
		return;
	case 7: // .FinishRaiseTower
		if (bob_tower(0x07, 0x01).a == 0x50)
			next_step();
		return;
	case 8: // .Func_b6a0f
		if (bob_tower(0x0f, 0x03).zero)
			next_step();
		return;
	case 9: wait(40); return; // .Wait3
	case 10: // .Func_b6a4b
		set_state(wSceneObj2State, 0x00);
		set_state(wSceneObj3State, 0x00);
		tile_update(ADDR_Data_85579);
		next_step();
		return;
	case 11: wait(40); return; // .Wait4
	case 12: finish(); return;
	default: unrealistic(); return;
	}
}

// SOWFunc_Fan.Func_b6af2 / .Func_b6b48: three gusts of wind from the fan,
// each up-right a pixel every 2 frames (counted at flag + 1) to x 0x8c.
// Returns whether the gusts are done (flag past them).
bool MapFunctions::blow_gusts(uint16_t flag, uint16_t obj)
{
	Byte phase = mem_[flag];
	Byte frames = mem_[flag + 1];
	uint8_t i = phase - 1;
	if (i >= 6)
		return true;
	if (!(i & 1)) { // .Func_b6b08
		set_state(static_cast<uint16_t>(obj + SCENE_OBJ_STATE_OFFSET), 0x18);
		mem_[obj] = 0x5c;
		mem_[obj + 1] = 0x82;
		++phase;
		frames = 0;
		return false;
	}
	++frames; // .Func_b6b22
	if (frames != 0x02)
		return false;
	frames = 0;
	--mem_[obj];
	Byte x = mem_[obj + 1];
	++x;
	if (x == 0x8c)
		++phase;
	return false;
}

// SOWFunc_Fan: the fan blows, gusts of wind, and the stone (scene object
// 9) lifts
void MapFunctions::fan()
{
	// .Func_b6af2
	Byte gusts1 = mem_[w2d122];
	if (gusts1 && blow_gusts(w2d122, wSceneObj1)) {
		if (gusts1 == 7) { // .Func_b6b3b
			set_state(wSceneObj1State, 0x00);
			gusts1 = 0;
		} else {
			unrealistic();
		}
	}
	// .Func_b6b48
	Byte gusts2 = mem_[w2d124];
	if (gusts2 && blow_gusts(w2d124, wSceneObj2)) {
		Byte frames = mem_[w2d125];
		switch (gusts2) {
		case 7: // .Func_b6b95
			set_state(wSceneObj2State, 0x00);
			++gusts2;
			break;
		case 8: // .Func_b6ba2: the fan stops
			if (mem_[wSceneObj8Frame] != 0x4a)
				break;
			set_state(wSceneObj8State, 0x19);
			++gusts2;
			frames = 0;
			break;
		case 9: // .Func_b6bb8
			++frames;
			if (frames != 0x18)
				break;
			set_state(wSceneObj8State, 0x00);
			gusts2 = 0;
			break;
		default:
			unrealistic();
			break;
		}
	}
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(20); return; // .Func_b6a86
	case 2: // .Func_b6a90
		set_state(wSceneObj8State, 0x1a);
		play_sfx(mem_, SFX_12F);
		next_step();
		return;
	case 3: wait(24); return; // .Func_b6aa3
	case 4: // .Func_b6aa8
		mem_[w2d122] = 0x01;
		next_step();
		return;
	case 5: // .Func_b6ab0
		if (wram_.ow_func_counter() != 0x0a)
			return;
		mem_[w2d124] = 0x01;
		next_step();
		return;
	case 6: // .Func_b6abe
		if (mem_[wSceneObj9Frame] != 0x21)
			return;
		set_state(wSceneObj9State, 0x11);
		next_step();
		return;
	case 7: wait(12); return; // .Func_b6acf
	case 8: { // .Func_b6ad4: up a pixel every 8 frames, to 0x45
		Byte counter = wram_.ow_func_counter();
		if (counter != 0x08)
			return;
		counter = 0;
		Byte y = mem_[wSceneObj9YCoord];
		--y;
		if (y == 0x45)
			next_step();
		return;
	}
	case 9: // .Func_b6aea
		if (!mem_[w2d124])
			next_step();
		return;
	case 10: wait(20); return; // .Func_b6a8b
	case 11: finish(); return;
	default: unrealistic(); return;
	}
}

// w2d03e and w2d049: the map's quake on (from 3) or off (from 0x0e)
void MapFunctions::quake(bool on)
{
	mem_[w2d03e] = on ? 0x03 : 0x0e;
	mem_[w2d03e + 1] = 0;
	mem_[w2d03e + 2] = 0;
	mem_[w2d049] = on ? 0x01 : 0x00;
}

// SOWFunc_Earthquake: the ground shakes and the rocks (scene object 2)
// crumble twice
void MapFunctions::south_earthquake()
{
	auto crumble = [this] { // .asm_b6c86
		set_state(wSceneObj2State, 0x1b);
		next_step();
	};

	shake();
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(20); return; // .Func_b6c53
	case 2: load_level_name(); return;
	case 3: wait_level_name(); return;
	case 4: wait(16); return;
	case 5: // .Func_b6c5d
		mem_[wOWUIObj1YCoord] = 0x01;
		quake(true);
		next_step();
		return;
	case 6: wait(120); return; // .Func_b6c73
	case 7: // .Func_b6c78
		tile_update(ADDR_Data_855f9);
		play_sfx(mem_, SFX_061);
		crumble();
		return;
	case 8: wait_state(wSceneObj2State, 0); return; // .Func_b6c91
	case 9: // .Func_b6c97
		mem_[wSceneObj2YCoord] += 0x02;
		mem_[wSceneObj2XCoord] += 0xfe;
		crumble();
		return;
	case 10: // .Func_b6ca4
		mem_[wOWUIObj1YCoord] = 0x03;
		quake(false);
		next_step();
		return;
	case 11: wait(60); return; // .Func_b6c58
	case 12: finish(); return;
	default: unrealistic(); return;
	}
}

// SOWFunc_GreenMusicBox: the music, then the cave blows open
void MapFunctions::green_music_box()
{
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(20); return; // .Wait1
	case 2: // .PlayMusicBoxSFX
		play_sfx(mem_, SFX_120);
		next_step();
		return;
	case 3: wait(254); return; // .WaitMusic
	case 4: // .BlowUpCave
		tile_update(ADDR_Data_8560e);
		set_state(wSceneObj2State, 0x1b);
		play_sfx(mem_, SFX_129);
		next_step();
		return;
	case 5: wait_state(wSceneObj2State, 0); return; // .HideSmoke
	case 6: wait(40); return; // .Wait2
	case 7: finish(); return;
	default: unrealistic(); return;
	}
}

// SOWFunc_Explosives / EOWFunc_Explosives: in the south, three blasts; in
// the east, a blast, the hole's tiles, its name, and two more
void MapFunctions::explosives()
{
	// .Func_b6d5a / .asm_b6d62
	auto blast = [this](uint16_t state, uint8_t value) {
		set_state(state, value);
		play_sfx(mem_, SFX_129);
		next_step();
	};
	// .asm_b6d76
	auto blast_at = [&](int8_t dy, int8_t dx) {
		mem_[wSceneObj2YCoord] = static_cast<uint8_t>(0x20 + dy);
		mem_[wSceneObj2XCoord] = static_cast<uint8_t>(0x44 + dx);
		blast(wSceneObj2State, 0x1c);
	};

	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(20); return; // .Func_b6d3d
	case 2: load_level_name(); return;
	case 3: wait_level_name(); return;
	case 4: wait(16); return;
	case 5: // .CheckMapSide
		if (wram_.cur_map_side() == SOUTH)
			next_step();
		else
			wram_.unk_2d062() = 0x0e;
		return;
	// the south
	case 6: blast(wSceneObj2State, 0x1c); return; // .Func_b6d5a
	case 7: case 9: case 11: case 15: // .Func_b6d6d
		wait_state(wSceneObj2State, 0);
		return;
	case 8: blast_at(-5, 4); return; // .Func_b6d73
	case 10: blast_at(3, 5); return; // .Func_b6d83
	case 12: wait(40); return; // .Func_b6d42
	case 13: case 25: finish(); return;
	// the east
	case 14: blast(wSceneObj2State, 0x08); return; // .Func_b6d88
	case 16: wait(20); return; // .Func_b6d92
	case 17: // .Func_b6dcd: the hole's tiles
		mem_[w2d0e0] = 0x03;
		routines_.farcall(BANK_Func_852e5, ADDR_Func_852e5);
		if (!mem_[w2d0e0])
			next_step();
		return;
	case 18: wait(10); return; // .Func_b6d97
	case 19: // .Func_b6d9c
		gb_->cpu.b = EAST;
		gb_->cpu.d = OWEAST_THE_COLOSSAL_HOLE;
		routines_.call(ADDR_LoadLevelName);
		wram_.pal_config1_total_steps() = 0;
		next_step();
		return;
	case 20: wait_level_name(); return;
	case 21: wait(16); return;
	case 22: blast(wSceneObj1State, 0x22); return; // .Func_b6daa
	case 23: // .Func_b6db4
		if (wram_.ow_func_counter() < 0x14)
			return;
		mem_[wSceneObj2YCoord] = 0x46;
		mem_[wSceneObj2XCoord] = 0x50;
		blast(wSceneObj2State, 0x22);
		return;
	case 24: wait(50); return; // .Func_b6d47
	default: unrealistic(); return;
	}
}

// SOWFunc_FullMoon: the gong, and the moon (scene object 14) rises
// up-right, a pixel every 8 frames
void MapFunctions::full_moon()
{
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(40); return; // .Wait1
	case 2: // .PlayGongSFX
		play_sfx(mem_, SFX_105);
		next_step();
		return;
	case 3: wait(30); return; // .Wait2
	case 4: { // .MoveMoon
		if (wram_.global_counter() & 0x07)
			return;
		--mem_[wSceneObj14];
		Byte x = mem_[wSceneObj14 + 1];
		++x;
		if (x == 0x7b)
			play_sfx(mem_, SFX_11A);
		else if (x == 0x80)
			next_step();
		return;
	}
	case 5: wait(60); return; // .Wait3
	case 6: finish(); return;
	default: unrealistic(); return;
	}
}

// SOWFunc_SendSeeds.GetXOffset: the seed's step right by its frame (counted
// at counter, held at 0x18: the byte after the table)
uint8_t MapFunctions::seed_x_offset(uint16_t counter)
{
	static constexpr uint16_t X_OFFSETS = ADDR_SOWFunc_SendSeeds + 0xe5;
	Byte frame = mem_[counter];
	uint8_t i = frame;
	if (i < 0x18)
		++frame;
	return mem_[static_cast<uint16_t>(X_OFFSETS + i)];
}

// SOWFunc_SendSeeds: three seeds show, then fly off one after another (the
// second up-left, the first straight up, the third up-right)
void MapFunctions::send_seeds()
{
	auto show = [this](uint16_t state) { // .asm_b6606
		set_state(state, 0x06);
		play_sfx(mem_, SFX_TEMPLE_ROCK);
		next_step();
	};

	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(30); return; // .Wait1
	case 2: show(wSceneObj4State); return; // .ShowSeed1
	case 3: case 5: case 7: wait(30); return; // .WaitShowSeed
	case 4: show(wSceneObj2State); return; // .ShowSeed2
	case 6: // .ShowSeed3
		mem_[w2d125] = 0x01;
		show(wSceneObj3State);
		return;
	case 8: // .PlayFlyingSFX
		play_sfx(mem_, SFX_07F);
		next_step();
		return;
	case 9: { // .MoveSeeds
		if (mem_[w2d125]) { // .MoveSeed2
			uint8_t dx = seed_x_offset(w2d125);
			mem_[wSceneObj2XCoord] -= dx;
			Byte y = mem_[wSceneObj2YCoord];
			y -= 3;
			if (y == 0x1a) {
				mem_[w2d129] = 0x01;
				play_sfx(mem_, SFX_07F);
			} else if (y >= 0xf0) {
				mem_[w2d125] = 0;
			}
		}
		if (mem_[w2d129]) { // .MoveSeed1
			Byte y = mem_[wSceneObj4YCoord];
			y -= 3;
			if (y == 0x1a) {
				mem_[w2d127] = 0x01;
				play_sfx(mem_, SFX_07F);
			} else if (y >= 0xf0) {
				mem_[w2d129] = 0;
			}
		}
		if (mem_[w2d127]) { // .MoveSeed3
			uint8_t dx = seed_x_offset(w2d127);
			mem_[wSceneObj3XCoord] += dx;
			mem_[wSceneObj3YCoord] -= 3;
		}
		if (mem_[wSceneObj3XCoord] < 0xf0)
			return;
		mem_[w2d127] = 0;
		next_step();
		return;
	}
	case 10: wait(10); return; // .Wait2
	case 11: finish(); return;
	default: unrealistic(); return;
	}
}

// ---------------------------------------------------------------------------
// The east side

// EOWFunc_FreezeSea.Func_b6fae / .Func_b6ffe / .Func_b7056: a stream of
// the ice's tile updates, laid out like a scene object at base: every 4
// frames (counted at base), the next 5 bytes of the list (its pointer at
// base + 1) go to base + 3 and on, the last two a word plus offset, for
// update to apply; 0x80 ends it.
void MapFunctions::freeze_tiles(uint16_t base, uint16_t offset, uint8_t bank, uint16_t update)
{
	Byte frames = mem_[base];
	if (!frames)
		return;
	++frames;
	if (frames < 0x05)
		return;
	frames = 0x01;
	uint16_t list = mem_.word(static_cast<uint16_t>(base + 1));
	uint8_t first = mem_[list++];
	if (first == 0x80) {
		frames = 0;
		return;
	}
	mem_[base + 3] = first;
	mem_[base + 4] = mem_[list++];
	mem_[base + 5] = mem_[list++];
	uint16_t target = mem_.word(list);
	list += 2;
	mem_[base + 1] = static_cast<uint8_t>(list);
	mem_[base + 2] = static_cast<uint8_t>(list >> 8);
	target = static_cast<uint16_t>(target + offset);
	mem_[base + 6] = static_cast<uint8_t>(target);
	mem_[base + 7] = static_cast<uint8_t>(target >> 8);
	routines_.farcall(bank, update);
	mem_[wOWPendingTileUpdate] = TRUE;
}

// EOWFunc_FreezeSea: the sea freezes over in three streams of tiles while
// the boat (scene object 1) sails off
void MapFunctions::freeze_sea()
{
	static constexpr uint16_t ICE = ADDR_EOWFunc_FreezeSea + 0x1ef; // .data

	// .Func_b6f77: the boat, 2 right a frame, rising a pixel every 4 once
	// past 0x60, until off the screen
	Byte boat = mem_[wSceneObj1Unk7];
	if (boat == 1) {
		mem_[w2d122] = 0;
		++boat;
	} else if (boat == 2) {
		uint8_t frame = ++mem_[w2d122];
		Byte x = mem_[wSceneObj1XCoord];
		x += 2;
		uint8_t right = static_cast<uint8_t>(x + 0x18);
		if (right >= 0xc0)
			boat = 0;
		else if (right >= 0x78 && !(frame & 0x03))
			--mem_[wSceneObj1YCoord];
	}
	freeze_tiles(wOWUIObj1, 0, BANK_Func_85a68, ADDR_Func_85a68); // .Func_b6fae
	freeze_tiles(w2d150, 0x280, BANK_Func_85acb, ADDR_Func_85acb); // .Func_b7056
	freeze_tiles(wOWUIObj2, 0x140, BANK_Func_85aa2, ADDR_Func_85aa2); // .Func_b6ffe

	// .asm_b6f31
	auto start_tiles = [this](uint16_t base) {
		mem_[base] = 0x01;
		mem_[base + 1] = static_cast<uint8_t>(ICE);
		mem_[base + 2] = static_cast<uint8_t>(ICE >> 8);
		next_step();
	};

	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(60); return; // .Func_b6ee5
	case 2: // .Func_b6ef4, with .Func_b6f52: palette 4, the ice's
		tile_update(ADDR_Data_85657);
		load_one_pal(BANK_Pals_84480, ADDR_Pals_84480, 4);
		next_step();
		return;
	case 3: // .Func_b6f00
		mem_[w2d046] = 0;
		mem_[wSceneObj1Unk7] = 0x01;
		next_step();
		return;
	case 4: wait(14); return; // .Func_b6f0c
	case 5: // .Func_b6f11
		play_sfx(mem_, SFX_11C);
		start_tiles(w2d150);
		return;
	case 6: // .Func_b6f23
		if (wram_.ow_func_counter() == 0x04)
			start_tiles(wOWUIObj1);
		return;
	case 7: // .Func_b6f3a
		if (wram_.ow_func_counter() == 0x04)
			start_tiles(wOWUIObj2);
		return;
	case 8: // .Func_b6f4a
		if (!mem_[wSceneObj1Unk7])
			next_step();
		return;
	case 9: wait(60); return; // .Func_b6eea
	case 10: finish(); return;
	default: unrealistic(); return;
	}
}

// EOWFunc_Earthquake: the ground shakes and the tower (scene object 6)
// sinks a pixel at a time
void MapFunctions::east_earthquake()
{
	auto sink = [this](uint8_t frames) { // .Func_b7188 to .Func_b71bc
		if (wram_.ow_func_counter() < frames)
			return;
		++mem_[wSceneObj6YCoord];
		next_step();
	};

	shake();
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(20); return; // .Func_b7138
	case 2: load_level_name(); return;
	case 3: wait_level_name(); return;
	case 4: wait(16); return;
	case 5: // .Func_b7142: the ground in front of the tower
		for (int row = 10; row <= 11; row++) {
			for (int col = 2; col < 2 + 4; col++) {
				uint16_t attr = static_cast<uint16_t>(wAttrmap + row * TILEMAP_WIDTH + col);
				mem_[attr] |= 1 << B_BG_PRIO;
			}
		}
		mem_[wOWPendingTileUpdate] = TRUE;
		next_step();
		return;
	case 6: // .Func_b7160
		set_state(wSceneObj6State, 0x03);
		next_step();
		return;
	case 7: wait(10); return; // .Func_b716b
	case 8: // .Func_b7170
		mem_[wOWUIObj1YCoord] = 0x01;
		set_state(wSceneObj15State, 0x06);
		set_state(wSceneObj6State, 0x04);
		next_step();
		return;
	case 9: sink(0x0a); return;
	case 10: sink(0x20); return;
	case 11: sink(0x1c); return;
	case 12: sink(0x18); return;
	case 13: sink(0x14); return;
	case 14: // .Func_b71c9
		mem_[wOWUIObj1YCoord] = 0x03;
		set_state(wSceneObj15State, 0x07);
		next_step();
		return;
	case 15: // .Func_b71d9
		set_state(wSceneObj6State, 0x05);
		next_step();
		return;
	case 16: // .Func_b71e4
		if (mem_[wSceneObj6Frame] == 0x09)
			next_step();
		return;
	case 17: // .Func_b71ed
		if (wram_.ow_func_counter() < 0x09)
			return;
		tile_update(ADDR_Data_8563e);
		next_step();
		return;
	case 18: wait(40); return; // .Func_b713d
	case 19: finish(); return;
	default: unrealistic(); return;
	}
}

// EOWFunc_BurnVines: the torch (scene object 1) burns the vines, crackling
// every 15 frames
void MapFunctions::burn_vines()
{
	// .Func_b77ce
	Byte frames = mem_[w2d123];
	++frames;
	if (frames >= 0x0f) {
		frames = 0;
		switch (mem_[wSceneObj1State]) {
		case 0x15: play_sfx(mem_, SFX_126); break;
		case 0x16: play_sfx(mem_, SFX_125); break;
		case 0x14: play_sfx(mem_, SFX_01E); break;
		}
	}
	auto torch = [this](uint8_t state) {
		set_state(wSceneObj1State, state);
		next_step();
	};
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(40); return; // .Func_b7803
	case 2: torch(0x15); return; // .Func_b780d
	case 3: wait(60); return; // .Func_b7818
	case 4: torch(0x16); return; // .Func_b781d
	case 5: wait(30); return; // .Func_b7828
	case 6: torch(0x14); return; // .Func_b782d
	case 7: wait(90); return; // .Func_b7838
	case 8: // .Func_b783d
		set_state(wSceneObj9State, 0x00);
		torch(0x15);
		return;
	case 9: wait(40); return; // .Func_b7847
	case 10: torch(0x18); return; // .Func_b784c
	case 11: wait_state(wSceneObj1State, 0); return; // .Func_b7857
	case 12: wait(30); return; // .Func_b7808
	case 13: finish(); return;
	default: unrealistic(); return;
	}
}

// EOWFunc_ShowWarpedVoid
void MapFunctions::show_warped_void()
{
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(60); return; // .Func_b786d
	case 2: // .Func_b7877
		set_state(wSceneObj5State, 0x1b);
		play_sfx(mem_, SFX_128);
		next_step();
		return;
	case 3: wait_state(wSceneObj5State, 0x19); return; // .Func_b788a
	case 4: wait(160); return; // .Func_b7872
	case 5: finish(); return;
	default: unrealistic(); return;
	}
}

// EOWFunc_RaisePipe
void MapFunctions::raise_pipe()
{
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(20); return; // .Func_b78aa
	case 2: load_level_name(); return;
	case 3: wait_level_name(); return;
	case 4: wait(16); return;
	case 5: // .Func_b78b4
		mem_[wSceneObj8] = 0x37;
		set_state(wSceneObj8State, 0x1d);
		next_step();
		return;
	case 6: // .Func_b78c4
		if (wram_.ow_func_counter() < 0x10)
			return;
		play_sfx(mem_, SFX_FAT_WALK);
		next_step();
		return;
	case 7: wait_state(wSceneObj8State, 0x1c); return; // .Func_b78d5
	case 8: wait(60); return; // .Func_b78af
	case 9: finish(); return;
	default: unrealistic(); return;
	}
}

// EOWFunc_Daytime: the palettes fade to day
void MapFunctions::daytime()
{
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(60); return; // .Func_b78eb
	case 2: // .Func_b78f5
		routines_.farcall(BANK_Func_84b97, ADDR_Func_84b97);
		if (mem_[wOWPalTransitionState])
			return;
		gb_->cpu.ime = false; // di
		routines_.farcall(BANK_VBlank_OWScene, ADDR_VBlank_OWScene);
		gb_->cpu.ime = true; // ei
		gb_->cpu.ime_delay = 0;
		next_step();
		return;
	case 3: wait(60); return; // .Func_b78f0
	case 4: finish(); return;
	default: unrealistic(); return;
	}
}

// EOWFunc_FormCastle.Func_b74cb: the castle's palettes flicker in steps (in
// wOWUIObj2YCoord, counting frames in wOWUIObj2XCoord), each step's
// length from .data by wOWUIObj2Frame
void MapFunctions::castle_pals()
{
	static constexpr uint16_t STEPS = ADDR_EOWFunc_FormCastle + 0x1bb; // .data
	Byte phase = mem_[wOWUIObj2YCoord];
	if (!phase)
		return;
	Byte frames = mem_[wOWUIObj2XCoord];
	++frames;
	uint16_t lengths = static_cast<uint16_t>(STEPS + static_cast<uint8_t>(mem_[wOWUIObj2Frame] * 2));
	switch (phase) {
	case 1: // .asm_b74df: the configs, waiting for their registers
		mem_[wPalConfig1Index] = BGPI_AUTOINC | 0;
		mem_[wPalConfig1Number] = 6;
		mem_[wPalConfig1SourceHi] = static_cast<uint8_t>(wTempBGPals >> 8);
		mem_[wPalConfig1SourceLo] = static_cast<uint8_t>(wTempBGPals);
		mem_[wPalConfig2Index] = BGPI_AUTOINC | 0;
		mem_[wPalConfig2Number] = 6;
		mem_[wPalConfig2SourceHi] = static_cast<uint8_t>(wTempPals1 >> 8);
		mem_[wPalConfig2SourceLo] = static_cast<uint8_t>(wTempPals1);
		SET_HL(gb_, ADDR_Pals_84840);
		SET_DE(gb_, wTempBGPals);
		gb_->cpu.c = 6 * PAL_SIZE;
		gb_->cpu.b = BANK_Pals_84840;
		routines_.call(ADDR_CopyFarBytes);
		frames = 0; // .asm_b750a
		++phase;
		return;
	case 2: // .asm_b7511: the castle's
		if (frames < mem_[lengths])
			return;
		mem_[wPalConfig1] = static_cast<uint8_t>(rBGPI);
		frames = 0;
		++phase;
		return;
	case 3: // .asm_b7522: back to the map's
		if (frames < mem_[static_cast<uint16_t>(lengths + 1)])
			return;
		mem_[wPalConfig2] = static_cast<uint8_t>(rBGPI);
		frames = 0;
		--phase;
		return;
	case 4: // .asm_b7539
		mem_[wPalConfig2] = static_cast<uint8_t>(rBGPI);
		phase = 0;
		return;
	}
}

// EOWFunc_FormCastle: the castle (scene object 1) rises, flickering in,
// and its tiles appear
void MapFunctions::form_castle()
{
	auto tiles = [this](uint8_t frames, uint16_t data) { // .Func_b743b to .Func_b7486
		if (wram_.ow_func_counter() < frames)
			return;
		tile_update(data);
		next_step();
	};

	castle_pals();
	switch (step()) {
	case 0: next_step(); return;
	case 1: wait(60); return; // .Func_b73c4
	case 2: // .Func_b73ce
		set_state(wSceneObj1State, 0x10);
		play_sfx(mem_, SFX_12E);
		next_step();
		return;
	case 3: wait(40); return; // .Func_b73e1
	case 4: // .Func_b73e6
		mem_[w2d121] = 0x01;
		next_step();
		return;
	case 5: { // .Func_b73ee: rising 3 frames of 4, the flicker starting
		// at 0x30 and quickening at 0x50
		bool moving = wram_.ow_func_counter() & 0x03;
		if (moving) // .Func_b741b
			move(wSceneObj1YCoord, ADDR_Data_b64cd);
		uint8_t rise = mem_[wSceneObj1Unk7];
		if (rise == 0x30) {
			if (moving)
				mem_[wOWUIObj2YCoord] = 0x01;
		} else if (rise == 0x50 || !rise) { // .asm_b7410
			if (moving)
				++mem_[wOWUIObj2Frame];
			if (!rise)
				next_step();
		}
		return;
	}
	case 6: wait(20); return; // .Func_b742b
	case 7: // .Func_b7430
		set_state(wSceneObj1State, 0x11);
		next_step();
		return;
	case 8: tiles(0x28, ADDR_Data_85b3d); return; // .Func_b743b
	case 9: tiles(0x05, ADDR_Data_85b2c); return;
	case 10: tiles(0x05, ADDR_Data_85b1b); return;
	case 11: tiles(0x05, ADDR_Data_85b0a); return;
	case 12: tiles(0x05, ADDR_Data_85afd); return;
	case 13: tiles(0x05, ADDR_Data_85af4); return;
	case 14: // .Func_b7495
		set_state(wSceneObj7State, 0x12);
		next_step();
		return;
	case 15: wait(10); return; // .Func_b74a0
	case 16: // .Func_b74a5
		set_state(wSceneObj1State, 0x00);
		next_step();
		return;
	case 17: { // .Func_b74b0: the flicker slows, and ends
		uint8_t counter = wram_.ow_func_counter();
		if (counter == 0x0a || counter == 0x14) {
			--mem_[wOWUIObj2Frame];
		} else if (counter == 0x21) {
			mem_[wOWUIObj2YCoord] = 0x04;
			next_step();
		}
		return;
	}
	case 18: wait(40); return; // .Func_b73c9
	case 19: finish(); return;
	default: unrealistic(); return;
	}
}

// EOWFunc_VulcanoEruption.Func_b762d to .Func_b7719: a lava fountain:
// started, until its object reaches done (or straight on, done < 0), 0x5a
// frames (counted at flag + 1), then hidden
void MapFunctions::lava_fountain(const Fountain &fountain)
{
	Byte phase = mem_[fountain.flag];
	if (!phase)
		return;
	Byte frames = mem_[fountain.flag + 1];
	++frames;
	uint8_t i = phase - 1;
	if (fountain.done < 0 && i >= 1)
		++i; // no waiting for the object
	auto advance = [&] { // .asm_b7649
		++phase;
		frames = 0;
	};
	switch (i) {
	case 0:
		set_state(fountain.state, fountain.start);
		advance();
		return;
	case 1:
		if (mem_[fountain.state] == fountain.done)
			advance();
		return;
	case 2:
		if (frames > 0x5a)
			advance();
		return;
	case 3:
		set_state(fountain.state, 0x00);
		phase = 0;
		return;
	default:
		unrealistic();
		return;
	}
}

// EOWFunc_VulcanoEruption: the east's volcano: it shakes, then the lava
// fountains and the flying rocks
void MapFunctions::east_volcano()
{
	static constexpr Rock LEFT_ROCK = {w2d122, wSceneObj1, -2, 8,
		{{8, 46, 0x1c}, {8, 75, 0x1c}, {8, 46, 0x1f}, {8, 75, 0x1f}}};
	static constexpr Rock RIGHT_ROCK = {w2d126, wSceneObj3, 2, 8,
		{{8, 98, 0x1d}, {8, 93, 0x1d}, {8, 98, 0x20}, {8, 93, 0x20}}};
	static constexpr Fountain FOUNTAINS[] = {
		{w2d134, wSceneObj10State, 0x0a, 0x0b}, // .Func_b762d
		{w2d138, wSceneObj12State, 0x0d, 0x0e}, // .Func_b766c
		{w2d13a, wSceneObj13State, 0x0d, 0x0e}, // .Func_b76a9
		{w2d13c, wSceneObj14State, 0x0e, -1}, // .Func_b76e6
		{w2d13e, wSceneObj15State, 0x0e, -1}, // .Func_b7719
	};

	// .Func_b774c: the rocks, thrown one after another (counting frames
	// in wOWUIObj1Unk7)
	fly_rock(LEFT_ROCK);
	fly_rock(RIGHT_ROCK);
	Byte thrower = mem_[wOWUIObj1State];
	if (thrower) {
		Byte frames = mem_[wOWUIObj1Unk7];
		++frames;
		auto throw_rock = [&](uint8_t wait_frames, uint16_t flag, uint8_t phase) {
			if (frames < wait_frames)
				return;
			mem_[flag] = phase;
			play_sfx(mem_, SFX_TEMPLE_ROCK); // .Func_b7763
			frames = 0;
			++thrower;
		};
		switch (thrower) {
		case 1: throw_rock(0x50, w2d122, 0x05); break; // .Func_b7772
		case 2: throw_rock(0x3c, w2d126, 0x07); break; // .Func_b777f
		case 3: throw_rock(0x50, w2d122, 0x07); break; // .Func_b778c
		case 4: thrower = 0; break; // .Func_b77a6
		default: unrealistic(); break;
		}
	}
	shake();
	for (const Fountain &fountain : FOUNTAINS)
		lava_fountain(fountain);
	rumble();

	auto start = [this](uint8_t frames, uint16_t flag) { // .Func_b75ed to .Func_b7617
		if (wram_.ow_func_counter() < frames)
			return;
		mem_[flag] = 0x01;
		next_step();
	};

	switch (step()) {
	case 0: next_step(); return;
	case 1: // .Func_b75b6
		mem_[wOWUIObj2Attributes] = 0x01;
		next_step();
		return;
	case 2: // .Func_b75c3
		mem_[wOWUIObj1YCoord] = 0x0a;
		mem_[wOWUIObj1State] = 0x01;
		next_step();
		return;
	case 3: // .Func_b75d0
		if (!mem_[wOWUIObj1YCoord])
			next_step();
		return;
	case 4: wait(56); return; // .Func_b75d8
	case 5: // .Func_b75dd
		play_sfx(mem_, SFX_127);
		mem_[w2d134] = 0x01;
		next_step();
		return;
	case 6: start(0x08, w2d13a); return;
	case 7: start(0x3c, w2d13c); return;
	case 8: start(0x08, w2d138); return;
	case 9: start(0x08, w2d13e); return;
	case 10: // .Func_b7625
		if (!mem_[w2d13e])
			next_step();
		return;
	case 11: wait(180); return; // .Func_b75be
	case 12: finish(); return;
	default: unrealistic(); return;
	}
}

} // namespace wl3::overworld

// Each run by _DoOWFunction's jumptable (only farcalled, by the scenes'
// play_action: C++); nothing reads registers after.
PORT_EX(NOWFunc_CutTree, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).cut_tree(); }
PORT_EX(NOWFunc_OpenGate, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).open_gate(); }
PORT_EX(NOWFunc_Garlic, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).finish(); }
PORT_EX(NOWFunc_MagicSeed, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).north_magic_seed(); }
PORT_EX(NOWFunc_SummonSnake, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).north_summon_snake(); }
PORT_EX(NOWFunc_FallLeaves, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).fall_leaves(); }
PORT_EX(NOWFunc_PlayMusicBox, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).play_music_box(); }
PORT_EX(NOWFunc_Crayon, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).crayon(); }
PORT_EX(WOWFunc_Rain, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).rain(); }
PORT_EX(WOWFunc_ClearTornado, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).clear_tornado(); }
PORT_EX(WOWFunc_ElevatorWorking, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).elevator_working(); }
PORT_EX(WOWFunc_MagicSeed, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).west_magic_seed(); }
PORT_EX(WOWFunc_SummonSnake, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).west_summon_snake(); }
PORT_EX(SOWFunc_MagicSeed, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).south_magic_seed(); }
PORT_EX(SOWFunc_YellowMusicBox, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).yellow_music_box(); }
PORT_EX(SOWFunc_SummonSnake, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).south_summon_snake(); }
PORT_EX(WOWFunc_VulcanoEruption, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).west_volcano(); }
PORT_EX(WOWFunc_RedMusicBox, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).red_music_box(); }
PORT_EX(WOWFunc_Earthquake, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).west_earthquake(); }
PORT_EX(SOWFunc_CanyonThunder, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).canyon_thunder(); }
PORT_EX(SOWFunc_RaiseTower, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).raise_tower(); }
PORT_EX(SOWFunc_Fan, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).fan(); }
PORT_EX(SOWFunc_Earthquake, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).south_earthquake(); }
PORT_EX(SOWFunc_GreenMusicBox, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).green_music_box(); }
PORT_EX(EOWFunc_Explosives, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).explosives(); }
PORT_EX(SOWFunc_FullMoon, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).full_moon(); }
PORT_EX(SOWFunc_SendSeeds, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).send_seeds(); }
PORT_EX(EOWFunc_FreezeSea, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).freeze_sea(); }
PORT_EX(EOWFunc_Earthquake, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).east_earthquake(); }
PORT_EX(EOWFunc_BurnVines, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).burn_vines(); }
PORT_EX(EOWFunc_ShowWarpedVoid, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).show_warped_void(); }
PORT_EX(EOWFunc_RaisePipe, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).raise_pipe(); }
PORT_EX(EOWFunc_Daytime, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).daytime(); }
PORT_EX(EOWFunc_FormCastle, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).form_castle(); }
PORT_EX(EOWFunc_VulcanoEruption, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).east_volcano(); }
PORT_EX(NOWFunc_BottomBar, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).north_bottom_bar(); }
PORT_EX(NOWFunc_DayNightSpell, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).day_night_spell(); }
PORT_EX(NOWFunc_Prologue, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).prologue(); }

// Only the map functions above run these; hooked for the fuzzer.
PORT_EX(Func_b71fc, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).shake(); }
PORT_EX(Func_b6055, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).rumble(); }
PORT_EX(_DoOWFunction, 0, ALL_REGISTERS) { wl3::overworld::MapFunctions(gb).run(); }

// Only run by the scenes above.
PORT_COVERED(NorthOWFunctions);
PORT_COVERED(WestOWFunctions);
PORT_COVERED(SouthOWFunctions);
PORT_COVERED(EastOWFunctions);
PORT_COVERED(AdvanceOWFunc);
PORT_COVERED(WaitOWFunc);
PORT_COVERED(Func_b5849);
PORT_COVERED(EndOWFunc);
PORT_COVERED(WaitOWObjState);
PORT_COVERED(WaitOWObjDisappear);
PORT_COVERED(GetSceneFallingYOffset);
PORT_COVERED(LoadOWSceneLevelName);
PORT_COVERED(WaitOWSceneLevelNameFadeIn);
PORT_COVERED(WaitOWScene16Frames);
PORT_COVERED(Func_b5868);
PORT_COVERED(Func_b587d);
PORT_COVERED(Func_b588d);
PORT_COVERED(Func_b69a6);
