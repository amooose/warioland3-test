// Wario on a side of the map (bank20.asm, Func_80e89): standing on a level
// or walking its paths to the next one, entering levels, and leaving the
// side by its exits. w2d013 is the state, w2d014 counts its frames. See
// overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

static_assert(PAD_RIGHT == 0x10 && PAD_LEFT == 0x20 && PAD_UP == 0x40 && PAD_DOWN == 0x80);

enum State : uint8_t {
	ARRIVE, // Func_80ee7: Wario at wMapSideLevelID
	START_MUSIC, // Func_80f0d
	START_ON_LEVEL, // Func_80f3c
	SHOW_LEVEL_NAME, // Func_80fb8
	FADE_IN_LEVEL_NAME, // Func_80fd6
	ON_LEVEL, // Func_80ff7: waiting for input
	START_WALK, // Func_810bb
	WALK, // Func_8115c
	WALK_OUT, // Func_811cf: off the side, by an exit
	NEXT_SIDE, // Func_81200
	TEMPLE, // Func_81346: going into the temple
	NUM_STATES,
};

// the frames between Wario's footsteps' sound, walking and running
constexpr uint8_t STEP_SFX_FRAMES = 0x24;

// Func_8208d's animation bank
constexpr uint8_t OW_ANIMATION_BANK = 0x2a;

} // namespace

void MapWalk::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

// Func_80e89
void MapWalk::update()
{
	routines_.call(ADDR_ProcessDPadRepeat);
	routines_.farcall(BANK_UpdateMapSideOWAnimations, ADDR_UpdateMapSideOWAnimations);
	routines_.farcall(BANK_UpdateCommonOWAnimations, ADDR_UpdateCommonOWAnimations);
	TopBar(gb_).update();
	if (wram_.ow_bars_state()) {
		update_objects();
		return;
	}
	BottomBar bottom_bar(gb_);
	bottom_bar.handle();
	bottom_bar.draw();
	routines_.farcall(BANK_Func_852e5, ADDR_Func_852e5);
	++wram_.unk_2d014();
	switch (wram_.unk_2d013()) {
	case ARRIVE: arrive(); return;
	case START_MUSIC: start_music(); return;
	case START_ON_LEVEL: start_on_level(); return;
	case SHOW_LEVEL_NAME: show_level_name(); return;
	case FADE_IN_LEVEL_NAME: fade_in_level_name(); return;
	case ON_LEVEL: on_level(); return;
	case START_WALK: start_walk(); return;
	case WALK: walk(); return;
	case WALK_OUT: walk_out(); return;
	case NEXT_SIDE: next_side(); return;
	case TEMPLE: temple(); return;
	default: unrealistic(); return;
	}
}

void MapWalk::next_state()
{
	++wram_.unk_2d013();
}

// ---------------------------------------------------------------------------
// The states

void MapWalk::arrive()
{
	place_on_level();
	get_allowed_dpad_input();
	uint8_t level = wram_.map_side_level_id();
	wram_.level2() = level;
	wram_.unk_2d068() = level;
	wram_.pal_config1_total_steps() = 0;
	wram_.unk_2d0e0() = 0;
	wram_.ow_pal_transition_state() = 0;
	update_objects();
	wram_.unk_2d014() = 0;
	next_state();
}

// The side's music, or by night first the day/night palettes.
void MapWalk::start_music()
{
	if (!wram_.day_night().bit(7)) {
		routines_.call(ADDR_PlayOverworldMusic);
		update_objects();
		next_state();
		return;
	}
	routines_.farcall(BANK_DayNightTransition, ADDR_DayNightTransition);
	update_objects();
	if (wram_.ow_pal_transition_state())
		return;
	gb_->cpu.ime = false; // di
	routines_.call(ADDR_VBlank_Overworld);
	gb_->cpu.ime = true; // ei
	gb_->cpu.ime_delay = 0;
}

// On an exit Wario walks on into the next side; on a level, a d-pad
// direction still held walks on along it.
void MapWalk::start_on_level()
{
	wram_.magnifying_glass_counter() = 0;
	place_on_level();
	get_allowed_dpad_input();
	uint8_t level = wram_.map_side_level_id();
	if (level == OW_EXIT_RIGHT || level == OW_EXIT_LEFT) {
		wram_.level_arrow_directions1() = level == OW_EXIT_RIGHT ? PAD_LEFT : PAD_RIGHT;
		set_next_level();
		update_objects();
		wram_.unk_2d014() = 0;
		wram_.unk_2d013() = START_WALK;
		return;
	}
	load_map_level_from_level_index();
	routines_.farcall(BANK_Func_9aab5, ADDR_Func_9aab5);
	wram_.unk_2d014() = 0;
	uint8_t direction = dpad(wram_.joypad_down()) & wram_.ow_allowed_d_pad_input();
	if (direction) {
		wram_.unk_2d013() = ON_LEVEL;
		walk_towards(direction);
		return;
	}
	update_objects();
	wram_.level_arrow_directions1() = 0;
	wram_.unk_2d10e() = 0;
	next_state();
}

// Func_80fb8: once the palettes are done, the level's name
void MapWalk::show_level_name()
{
	update_objects();
	if (wram_.unk_2d0e0())
		return;
	Events(gb_).load_level_name_if_valid();
	next_state();
}

void MapWalk::fade_in_level_name()
{
	update_objects();
	if (wram_.map_side_level_id() < OW_EXITS) {
		routines_.farcall(BANK_FadeInLevelName, ADDR_FadeInLevelName);
		if (gb_->cpu.c & 1) // still fading in
			return;
	}
	next_state();
}

// Func_80ff7: on a level, waiting for A (to enter it), Select (the top
// bar), B (the temple's arrows) or a direction to walk to
void MapWalk::on_level()
{
	if (enter_level() || wram_.ow_pal_transition_state()) {
		update_objects();
		return;
	}
	if (!wram_.joypad_down().bit(B_PAD_B) && wram_.joypad_pressed().bit(B_PAD_SELECT)) {
		++wram_.ow_bars_state();
		update_objects();
		return;
	}
	PathPreview(gb_).update();
	if (wram_.unk_2d0e3() || go_to_temple()) {
		update_objects();
		return;
	}
	routines_.call(ADDR_Func_80de0);
	if (gb_->cpu.f & FLAG_C) {
		update_objects();
		return;
	}
	uint8_t direction = dpad(wram_.joypad_pressed()) & wram_.ow_allowed_d_pad_input();
	if (!direction) {
		update_objects();
		return;
	}
	walk_towards(direction);
}

// Func_81034: off along the path in direction
void MapWalk::walk_towards(uint8_t direction)
{
	wram_.level_arrow_directions1() = direction;
	set_next_level();
	wram_.level2() = wram_.unk_2d068();
	wram_.unk_2d0e0() = 3;
	wram_.pal_config1_total_steps() = 0;
	update_objects();
	wram_.unk_2d014() = 0;
	next_state();
}

// Func_810bb: the path's start, from where Wario is to the next level
void MapWalk::start_walk()
{
	wram_.owy_coord2() = wram_.owy_coord1();
	wram_.owx_coord2() = wram_.owx_coord1();
	uint16_t coords = map_level_coords(wram_.unk_2d068());
	uint8_t y = mem_[coords];
	uint8_t x = mem_[static_cast<uint16_t>(coords + 1)];
	wram_.unk_2d06f() = static_cast<uint8_t>(mem_[static_cast<uint16_t>(coords + 2)] + y);
	wram_.unk_2d070() = static_cast<uint8_t>(mem_[static_cast<uint16_t>(coords + 3)] + x);
	next_path_leg();
	path_distances();
	update_objects();
	wram_.unk_2d014() = 0;
	next_state();
}

// Func_8115c: a step along the path (two, running: a direction held for
// 6 frames sets w2d10e's bit 7)
void MapWalk::walk()
{
	path_step();
	Byte run = wram_.unk_2d10e();
	if (!run.bit(7) && (wram_.joypad_down() & PAD_CTRL_PAD)) {
		++run;
		if (run >= 6)
			run.set_bit(7);
	}
	if (run.bit(7))
		path_step();

	wram_.unk_2d100() = wram_.owy_coord1();
	wram_.unk_2d101() = wram_.owx_coord1();
	if (wram_.unk_2d0a1()) { // still on this leg
		update_objects();
		return;
	}
	if (next_path_leg() != 0xff) {
		path_distances();
		face_path(); // Func_81142
		update_objects();
		return;
	}

	// arrived
	uint8_t level = wram_.unk_2d068();
	wram_.map_side_level_id() = level;
	if (level < OW_EXITS) {
		wram_.unk_2d013() = START_ON_LEVEL;
	} else {
		stop_music2(mem_);
		play_sfx(mem_, SFX_0EB);
		wram_.unk_2d014() = 0;
		next_state();
	}
	update_objects();
}

// Func_811cf: off the side's edge, a pixel every other frame
void MapWalk::walk_out()
{
	if (wram_.unk_2d014() >= 2) {
		// xor a / ld [hl], a with hl still the jumptable's target, this
		// routine: a write to the MBC's RAM bank register
		mem_[ADDR_Func_811cf] = 0;
		uint8_t step, end;
		if (!exit_step(step, end, 0xa8, 0xf0))
			return;
		Byte x = wram_.unk_2d101();
		x += step;
		if (x == end)
			next_state();
	}
	update_objects();
}

// Func_81200: on to the next side
void MapWalk::next_side()
{
	update_objects();
	uint8_t step, end;
	if (!exit_step(step, end, 0, 0))
		return;
	Byte side = wram_.next_map_side();
	side = (side + step) & 0x03;
	Byte level = wram_.map_side_level_id();
	level += step;
	wram_.unk_2d068() = level;
	uint8_t next = side;
	// between the north and west sides, SST_OVERWORLD_1E
	if (!(next & 0x02) && (static_cast<uint8_t>(next + step) & 0x02))
		wram_.sub_state() = SST_OVERWORLD_1E;
	else
		wram_.sub_state() = SST_OVERWORLD_08;
}

// Func_81346
void MapWalk::temple()
{
	routines_.farcall(BANK_Func_852e5, ADDR_Func_852e5);
	temple_step();
	routines_.farcall(BANK_Func_b4a3d, ADDR_Func_b4a3d);
	draw_wario();
	routines_.farcall(BANK_Func_b4aa9, ADDR_Func_b4aa9);
}

// Func_8137a: at the temple, the hint for the next treasure: the level it
// is in, named, until a button closes it. w2d0e4 is the step, w2d0e5
// counts its frames.
void MapWalk::temple_step()
{
	++wram_.unk_2d0e5();
	switch (wram_.unk_2d0e4()) {
	case 0: // Func_81398
		break;
	case 1: // Func_8139f
	case 8:
		if (wram_.unk_2d0e5() < 8)
			return;
		wram_.unk_2d0e0() = 3;
		break;
	case 2: // Func_813ad
	case 9:
		if (wram_.unk_2d0e0())
			return;
		break;
	case 3: { // Func_813b4: the next treasure's level name
		Events::SideLevel at = Events(gb_).next_treasure_side_level();
		gb_->cpu.b = at.side;
		gb_->cpu.d = at.level;
		routines_.call(ADDR_LoadLevelName);
		break;
	}
	case 4: // Func_813ef
	case 6:
		wram_.pal_config1_total_steps() = 0;
		break;
	case 5: // Func_813f6
		routines_.farcall(BANK_FadeInLevelName, ADDR_FadeInLevelName);
		if (gb_->cpu.c & 1) // still fading in
			return;
		set_scene_obj_state(wSceneObj13State, 0x1c);
		break;
	case 7: // Func_81413
		if (wram_.joypad_pressed() & (PAD_B | PAD_RIGHT | PAD_LEFT | PAD_DOWN))
			set_scene_obj_state(wSceneObj13State, 0);
		else if (wram_.scene_obj13_state())
			return;
		break;
	case 10: // Func_8142d: back to the map
		set_wario_oam(OWWARIO_DOWN);
		wram_.unk_2d013() = ARRIVE;
		wram_.unk_2d014() = 0;
		wram_.unk_2d0e4() = 0;
		return;
	default:
		unrealistic();
		return;
	}
	// Func_81398: the next step
	wram_.unk_2d0e5() = 0;
	++wram_.unk_2d0e4();
}

void MapWalk::set_scene_obj_state(uint16_t state, uint8_t value)
{
	SET_HL(gb_, state);
	gb_->cpu.a = value;
	routines_.call(ADDR_SetSceneObjState);
}

// ---------------------------------------------------------------------------
// Helpers

// A map exit's step (1 to the right, -1 to the left) and end; false (the
// asm's debug_nop: it hangs) when Wario isn't on an exit.
bool MapWalk::exit_step(uint8_t &step, uint8_t &end, uint8_t right_end, uint8_t left_end)
{
	uint8_t level = wram_.map_side_level_id();
	if (level == OW_EXIT_RIGHT) {
		step = 1;
		end = right_end;
		return true;
	}
	if (level == OW_EXIT_LEFT) {
		step = 0xff;
		end = left_end;
		return true;
	}
	unrealistic();
	return false;
}

// Func_810a9, Func_80f3c.GetDirectionalInput: up or down if held, else
// right or left
uint8_t MapWalk::dpad(uint8_t pad)
{
	pad &= PAD_CTRL_PAD;
	if (pad & (PAD_DOWN | PAD_UP))
		return pad & (PAD_UP | PAD_DOWN);
	return pad;
}

// Func_81240: A on a level enters it (the temple, its own sub-state)
bool MapWalk::enter_level()
{
	if (wram_.ow_pal_transition_state() || wram_.ow_level() == 0x80
		|| !wram_.joypad_pressed().bit(B_PAD_A))
		return false;
	load_map_level_from_level_index();
	if (!wram_.ow_level()) {
		play_sfx(mem_, SFX_SELECTION);
		wram_.sub_state() = SST_OVERWORLD_1A;
		return true;
	}
	wram_.sub_state() = 0;
	++wram_.state(); // ST_LEVEL
	return true;
}

// Func_81077: up at the temple (with treasures left to find, or before the
// end) goes in
bool MapWalk::go_to_temple()
{
	if (wram_.next_map_side() | wram_.map_side_level_id())
		return false;
	if (!wram_.joypad_pressed().bit(B_PAD_UP))
		return false;
	if (wram_.next_treasure() == TREASURES_END && wram_.game_mode_flags().bit(MODE_GAME_CLEARED_F))
		return false;
	routines_.call(ADDR_Func_3c76);
	set_wario_oam(OWWARIO_UP);
	play_sfx(mem_, SFX_113);
	wram_.unk_2d013() = TEMPLE;
	return true;
}

// LoadMapLevelFromLevelIndex: wOWLevel, from the side and its level
void MapWalk::load_map_level_from_level_index()
{
	uint8_t side = wram_.next_map_side();
	uint8_t level = wram_.map_side_level_id();
	if (!side && level >= OWNORTH_JUNCTION) {
		wram_.ow_level() = 0x80;
		return;
	}
	uint8_t first = mem_[static_cast<uint16_t>(ADDR_MapSideInitialLevels + side)];
	wram_.ow_level() = static_cast<uint8_t>(level + first);
}

// GetMapLevelCoords: the level's entry in MapLevelCoords
uint16_t MapWalk::map_level_coords(uint8_t level)
{
	gb_->cpu.a = level;
	routines_.call(ADDR_GetMapLevelCoords);
	return REG_HL(gb_);
}

// Func_8212c: Wario at the level, w2d069 / w2d06a its coords
void MapWalk::place_on_level()
{
	uint8_t level = wram_.map_side_level_id();
	wram_.level1() = level;
	uint16_t coords = map_level_coords(level);
	uint8_t y = mem_[coords];
	uint8_t x = mem_[static_cast<uint16_t>(coords + 1)];
	wram_.unk_2d069() = y;
	wram_.unk_2d06a() = x;
	uint8_t wario_y = static_cast<uint8_t>(mem_[static_cast<uint16_t>(coords + 2)] + y);
	uint8_t wario_x = static_cast<uint8_t>(mem_[static_cast<uint16_t>(coords + 3)] + x);
	wram_.owy_coord1() = wario_y;
	wram_.unk_2d100() = wario_y;
	wram_.owx_coord1() = wario_x;
	wram_.unk_2d101() = wario_x;
}

// GetOWAllowedDPadInput: the directions with a path from the level (the
// tile next to it is a path, 1 or 2, drawn in VRAM bank 1: an arrow)
void MapWalk::get_allowed_dpad_input()
{
	if (wram_.map_side_level_id() == OWNORTH_JUNCTION) {
		wram_.ow_allowed_d_pad_input() = PAD_RIGHT | PAD_UP | PAD_DOWN;
		return;
	}
	SET_HL(gb_, w2d069);
	routines_.call(ADDR_GetOWCoordInTilemap);
	uint16_t tile = REG_HL(gb_);
	uint8_t allowed = 0;
	struct {
		int offset;
		uint8_t direction;
	} constexpr NEIGHBOURS[4] = {
		{1, PAD_RIGHT}, {-1, PAD_LEFT}, {-TILEMAP_WIDTH, PAD_UP}, {TILEMAP_WIDTH, PAD_DOWN}};
	for (auto n : NEIGHBOURS) {
		uint16_t at = static_cast<uint16_t>(tile + n.offset);
		uint8_t t = mem_[at];
		if (t != 1 && t != 2)
			continue;
		if (Byte(gb_, static_cast<uint16_t>(at - 0x300)).bit(B_BG_BANK1)) // wAttrmap
			allowed |= n.direction;
	}
	wram_.ow_allowed_d_pad_input() = allowed;
}

// Func_820e6: the level (w2d068) and path shape in wLevelArrowDirections1's
// direction, from the level's paths (Func_82111)
void MapWalk::set_next_level()
{
	uint16_t paths = level_paths();
	uint8_t arrows = wram_.level_arrow_directions1();
	int i = 3; // down (or none: debug_nop is never reached)
	if (arrows & PAD_RIGHT)
		i = 0;
	else if (arrows & PAD_LEFT)
		i = 1;
	else if (arrows & PAD_UP)
		i = 2;
	uint16_t entry = static_cast<uint16_t>(paths + i);
	wram_.unk_2d068() = mem_[entry];
	wram_.ow_path_shape() = mem_[static_cast<uint16_t>(entry + 4)];
	wram_.unk_2d072() = 0;
}

// Func_810fc: the path's next leg; its new shape (0, 1 or 2), or $ff at its
// end
uint8_t MapWalk::next_path_leg()
{
	Byte shape = mem_[wOWPathShape];
	switch (shape) {
	case PATH_NONE: // the end (marked in the byte after)
		mem_[static_cast<uint16_t>((wOWPathShape & 0xff00) | static_cast<uint8_t>(wOWPathShape + 1))] = 0xff;
		return 0xff;
	case PATH_BEELINE: // to (w2d06f, w2d070)
		wram_.owy_coord2() = wram_.unk_2d06f();
		wram_.owx_coord2() = wram_.unk_2d070();
		shape = 0;
		return 0;
	case PATH_Y_THEN_X:
		wram_.owy_coord2() = wram_.unk_2d06f();
		shape = 1;
		return 1;
	case PATH_X_THEN_Y:
		wram_.owx_coord2() = wram_.unk_2d070();
		shape = 1;
		return 1;
	case PATH_S_SHAPE: { // halfway across first
		uint8_t x = wram_.owx_coord1();
		int8_t half = static_cast<int8_t>(static_cast<uint8_t>(wram_.unk_2d070() - x)) >> 1;
		wram_.owx_coord2() = static_cast<uint8_t>(x + half);
		shape = 2;
		return 2;
	}
	default:
		unrealistic();
		return 0;
	}
}

// Func_821a1: the leg's distances (wOWYDistance, wOWXDistance) from
// wOWCoords1 to wOWCoords2, and the line's step: w2d0a0's bit 0 when x's
// is the longer, bits 1 and 2 going up or left; w2d0a1 the steps left,
// w2d0a3 the shorter axis' accumulator, w2d0a5/6 the y and x steps
void MapWalk::path_distances()
{
	Byte flags = mem_[w2d0a0];
	flags = 0;
	auto distance = [&](uint8_t from, uint8_t to, uint8_t backwards) { // .CalculateDistance
		if (to >= from)
			return static_cast<uint8_t>(to - from);
		flags = flags | backwards;
		return static_cast<uint8_t>(from - to);
	};
	uint8_t dy = distance(wram_.owy_coord1(), wram_.owy_coord2(), 1 << 1);
	wram_.owy_distance() = dy;
	uint8_t dx = distance(wram_.owx_coord1(), wram_.owx_coord2(), 1 << 2);
	wram_.owx_distance() = dx;
	uint8_t longer = dy, shorter = dx;
	uint8_t bits = flags;
	if (dy < dx) {
		longer = dx;
		shorter = dy;
		bits |= 1;
	}
	mem_[w2d0a1] = longer;
	mem_[w2d0a2] = longer;
	mem_[w2d0a3] = shorter;
	mem_[w2d0a4] = shorter;
	flags = bits;
	mem_[w2d0a5] = (bits & (1 << 1)) ? 0xff : 0x01;
	mem_[w2d0a6] = (bits & (1 << 2)) ? 0xff : 0x01;
}

// Func_82208: a step along the leg: the longer axis each time, the
// shorter when its accumulator passes the longer distance
void MapWalk::path_step()
{
	Byte left = mem_[w2d0a1];
	if (!left)
		return;
	--left;
	auto step = [this](bool x) { // .asm_82234
		uint16_t coord = x ? wOWXCoord1 : wOWYCoord1;
		mem_[coord] = static_cast<uint8_t>(mem_[coord] + mem_[x ? w2d0a6 : w2d0a5]);
	};
	uint8_t longer = mem_[w2d0a2];
	Byte accumulated = mem_[w2d0a3];
	accumulated = static_cast<uint8_t>(mem_[w2d0a4] + accumulated);
	bool x_longer = mem_[w2d0a0] & 1;
	if (accumulated >= longer) { // .Func_82224
		accumulated = static_cast<uint8_t>(accumulated - longer);
		step(!x_longer);
	}
	step(x_longer); // .Func_8222f
}

// Func_82111: the level's paths (OWLevelPaths, or OWExitPaths at the
// exits), by side
uint16_t MapWalk::level_paths()
{
	uint8_t level = wram_.map_side_level_id();
	uint16_t table = ADDR_OWLevelPaths;
	if (level >= OW_EXITS) {
		level = static_cast<uint8_t>(level - OW_EXIT_RIGHT);
		table = ADDR_OWExitPaths;
	}
	uint8_t index = static_cast<uint8_t>(level * 4);
	// GetCthWordFromAthTable
	uint16_t side = mem_.word(static_cast<uint16_t>(table + wram_.next_map_side() * 2));
	return static_cast<uint16_t>(side + index * 2);
}

// Func_81142: Wario facing along the path's leg
void MapWalk::face_path()
{
	Byte flags = wram_.unk_2d0a0();
	uint8_t direction;
	if (flags.bit(0)) // along x: right, or left
		direction = flags.bit(2) ? PAD_LEFT : PAD_RIGHT;
	else // along y: down, or up
		direction = flags.bit(1) ? PAD_UP : PAD_DOWN;
	wram_.level_arrow_directions1() = direction;
}

// Func_81055: the scene objects around Wario (behind and in front of him)
void MapWalk::update_objects()
{
	routines_.farcall(BANK_Func_b4a3d, ADDR_Func_b4a3d);
	update_wario();
	routines_.farcall(BANK_Func_b4aa9, ADDR_Func_b4aa9);
}

// Func_82041: Wario, with his footsteps' sound while he walks, and facing
// the way he goes
void MapWalk::update_wario()
{
	Byte steps = wram_.unk_2d10d();
	bool reset = wram_.unk_2d068() == wram_.map_side_level_id();
	if (!reset) {
		if (!steps)
			play_sfx(mem_, SFX_12A);
		++steps;
		uint8_t period = wram_.unk_2d10e().bit(7) ? STEP_SFX_FRAMES / 4 : STEP_SFX_FRAMES;
		reset = steps >= period;
	}
	if (reset)
		steps = 0;

	uint8_t arrows = wram_.level_arrow_directions1();
	if (arrows != wram_.unk_2d10c()) {
		wram_.unk_2d10c() = arrows;
		uint8_t oam = OWWARIO_DOWN;
		if (arrows & PAD_UP)
			oam = OWWARIO_UP;
		else if (arrows & PAD_RIGHT)
			oam = OWWARIO_RIGHT;
		else if (arrows & PAD_LEFT)
			oam = OWWARIO_LEFT;
		set_wario_oam(oam);
	}
	draw_wario();
}

// SetOWWarioOAM: Wario's frameset by direction (OWWARIO_*, 0 for none),
// with the walking or the other sprites
void MapWalk::set_wario_oam(uint8_t oam)
{
	static constexpr uint16_t FRAMESETS = ADDR_SetOWWarioOAM + 0x2d; // .framesets
	mem_[w2d106] = oam;
	if (!oam)
		return;
	uint16_t sprites = oam < 0x05 ? ADDR_OAM_aa329 : ADDR_OAM_aa0cf;
	uint16_t frameset = mem_.word(static_cast<uint16_t>(FRAMESETS + static_cast<uint8_t>((oam - 1) * 2)));
	mem_[w2d109] = static_cast<uint8_t>(frameset);
	mem_[w2d109 + 1] = static_cast<uint8_t>(frameset >> 8);
	mem_[w2d107] = static_cast<uint8_t>(sprites);
	mem_[w2d107 + 1] = static_cast<uint8_t>(sprites >> 8);
	mem_[w2d104] = 0;
	mem_[w2d104 + 1] = 0;
}

// Func_8208d: Wario's animation and sprite
void MapWalk::draw_wario()
{
	if (!wram_.unk_2d106())
		return;
	SET_DE(gb_, mem_.word(w2d109));
	SET_HL(gb_, w2d104);
	gb_->cpu.b = OW_ANIMATION_BANK;
	routines_.call(ADDR_UpdateOWAnimation);
	SET_DE(gb_, mem_.word(w2d107));
	SET_HL(gb_, w2d100);
	gb_->cpu.b = OW_ANIMATION_BANK;
	routines_.call(ADDR_LoadOWSpriteWithScroll);
}

} // namespace wl3::overworld

// The overworld's state Func_805d7 calls it, then LoadCompassSprite, which
// loads its own registers.
PORT_EX(Func_80e89, 0, ALL_REGISTERS)
{
	wl3::overworld::MapWalk(gb).update();
}

// Called from InitMapSide (init.asm, now C++ too) and from here only.
PORT_COVERED(Func_80ee7);
PORT_COVERED(Func_80f0d);
PORT_COVERED(Func_80f3c);
PORT_COVERED(Func_80fb8);
PORT_COVERED(Func_80fd6);
PORT_COVERED(Func_80ff7);
PORT_COVERED(Func_81034);
PORT_COVERED(Func_81077);
PORT_COVERED(Func_810a9);
PORT_COVERED(Func_810bb);
PORT_COVERED(Func_81142);
PORT_COVERED(Func_8115c);
PORT_COVERED(Func_811cf);
PORT_COVERED(Func_81200);
PORT_COVERED(Func_81240);
PORT_COVERED(Func_81346);
PORT_COVERED(LoadMapLevelFromLevelIndex);
PORT_COVERED(PlaySFX_12A);
PORT_COVERED(Func_81055);
PORT_COVERED(Func_82041);
PORT_COVERED(Func_8208d);
PORT_COVERED(Func_820e6);
PORT_COVERED(Func_8212c);
PORT_COVERED(GetOWAllowedDPadInput);
PORT_COVERED(Func_8137a);
PORT_COVERED(Func_81398);
PORT_COVERED(Func_8139f);
PORT_COVERED(Func_813ad);
PORT_COVERED(Func_813b4);
PORT_COVERED(Func_813ef);
PORT_COVERED(Func_813f6);
PORT_COVERED(Func_81413);
PORT_COVERED(Func_8142d);

// Only the map's C++ runs these.
PORT_COVERED(Func_810fc);
PORT_COVERED(Func_821a1);
PORT_COVERED(Func_82208);
PORT_COVERED(Func_82111);
