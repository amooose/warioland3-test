// The cutscenes' dispatch and shared helpers (bank27.asm). See
// cutscenes.hpp.
#include "cutscenes.hpp"

namespace wl3::cutscenes {

namespace {

constexpr uint16_t CUTSCENE_TABLE = ADDR_DoCutsceneFunc + 0x12; // DoCutsceneFunc.table
constexpr uint16_t INVALID_CUTSCENE = ADDR_DoCutsceneFunc + 0xcb; // .InvalidCutscene

static_assert(w2d013 + 1 == w2d014);

} // namespace

void Cutscenes::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

// DoCutsceneFunc
void Cutscenes::update()
{
	routines_.call(ADDR_ProcessDPadRepeat);
	++wram_.unk_2d014();
	uint8_t event = wram_.event_with_cutscene();
	if (event >= NUM_EVENTS) { // .OutOfBoundsCutscene: debug_nop
		unrealistic();
		return;
	}
	if (play(event))
		return;
	uint16_t target = mem_.word(static_cast<uint16_t>(CUTSCENE_TABLE + event * 2));
	if (target == INVALID_CUTSCENE) { // debug_nop
		unrealistic();
		return;
	}
	SET_BC(gb_, w2d014);
	call_c(CUTSCENE_TABLE, event, target);
}

// InitEventCutscene: the cutscene's graphics and objects
void Cutscenes::init()
{
	wram_.lcdc_flags_to_flip() = 0;
	routines_.clear_virtual_oam();
	if (wram_.cur_event() != EVENT_PROLOGUE) { // decompress_tiles1
		mem_[rVBK] = 1;
		wram_.temp_bank() = BANK_Cutscenes6Gfx;
		SET_HL(gb_, ADDR_Cutscenes6Gfx);
		SET_BC(gb_, v1Tiles0);
		routines_.farcall(BANK_Cutscenes6Gfx, ADDR_Decompress);
		mem_[rVBK] = 0;
	}
	uint8_t event = wram_.cur_event();
	wram_.event_with_cutscene() = event;
	if (init(event))
		return;
	if (event >= NUM_EVENTS) { // past CutsceneInitFunctions
		unrealistic();
		return;
	}
	uint16_t target = mem_.word(static_cast<uint16_t>(ADDR_CutsceneInitFunctions + event * 2));
	call_c(ADDR_CutsceneInitFunctions, event, target);
}

bool Cutscenes::init(uint8_t event)
{
	switch (event) {
	case EVENT_CUT_TREE:
		init_north(ADDR_Pals_b8c00, BANK_Pals_b8c00, ADDR_LoadCutscenes1Gfx_Vram0, ADDR_LoadCutsceneBGMaps01,
			ADDR_LoadCutsceneBGMaps02);
		return true;
	case EVENT_OPEN_NORTH_GATE: // InitCutscene03: Pals_b8240, one BG map
		load_pals1(BANK_Pals_b8240, ADDR_Pals_b8240);
		load_pals2(BANK_Pals_b88c0, ADDR_Pals_b88c0);
		routines_.call(ADDR_LoadCutscenes3Gfx);
		routines_.call(ADDR_LoadCutsceneBGMaps07);
		return true;
	case EVENT_EXPEL_TORNADO:
		init_north(ADDR_Pals_b8ec0, BANK_Pals_b8ec0, ADDR_LoadCutscenes8Gfx_Vram0, ADDR_LoadCutsceneBGMaps01,
			ADDR_LoadCutsceneBGMaps02);
		return true;
	case EVENT_RAINSTORM: init_rainstorm(); return true;
	case EVENT_FIX_ELEVATOR: init_fix_elevator(); return true;
	case EVENT_YELLOW_MUSIC_BOX: init_music_box(ADDR_Pals_b8ac0); return true;
	case EVENT_BLUE_MUSIC_BOX: init_music_box(ADDR_Pals_b8b00); return true;
	case EVENT_GREEN_MUSIC_BOX: init_music_box(ADDR_Pals_b8b40); return true;
	case EVENT_RED_MUSIC_BOX: init_music_box(ADDR_Pals_b8b80); return true;
	case EVENT_GOLD_MUSIC_BOX: init_music_box(ADDR_Pals_b8bc0); return true;
	case EVENT_PLANT_SEEDS: init_plant_seeds(); return true;
	case EVENT_RAISE_TOWER: init_raise_tower(); return true;
	case EVENT_BLOW_MIST: init_blow_mist(); return true;
	case EVENT_SUMMON_SNAKES: init_summon_snakes(); return true;
	case EVENT_FREEZE_SEA: init_freeze_sea(ADDR_Pals_b8200, ADDR_Pals_b8980); return true;
	case EVENT_SUMMON_LIGHTNING: init_freeze_sea(ADDR_Pals_b8040, ADDR_Pals_b8c40); return true;
	case EVENT_FOOT_STONE: init_foot_stone(); return true;
	case EVENT_PURIFY_WATER: init_purify_water(); return true;
	case EVENT_REVEAL_CASTLE: init_reveal_castle(); return true;
	case EVENT_SUMMON_SUN: init_summon_sun(); return true;
	case EVENT_EXPLODE_BOMBS: init_explode_bombs(); return true;
	case EVENT_LEAVES_FALL: init_leaves_fall(); return true;
	case EVENT_TREASURE_MAP: init_treasure_map(); return true;
	case EVENT_OPEN_TREE_MOUTH: init_open_tree_mouth(); return true;
	case EVENT_PLACE_CART_WHEELS: init_cart_wheels(); return true;
	case EVENT_MAKE_WIRE: init_make_wire(); return true;
	case EVENT_RAISE_ICE_BLOCKS: init_raise_ice_blocks(); return true;
	case EVENT_TORCH_FOREST: init_torch_forest(); return true;
	case EVENT_REVEAL_WARPED_VOID: init_reveal_warped_void(); return true;
	case EVENT_RAISE_PIPE: init_raise_pipe(); return true;
	case EVENT_REMOVE_WARPS: init_remove_warps(); return true;
	case EVENT_COLLECT_DEMONS_BLOOD: init_demons_blood(); return true;
	case EVENT_OPEN_PARAGOOM_CAGE: init_paragoom_cage(); return true;
	case EVENT_PLACE_PROPELLER: init_place_propeller(); return true;
	case EVENT_FEED_OCTOHON: init_feed_octohon(); return true;
	case EVENT_OPEN_SKY_DOOR: init_sky_door(); return true;
	case EVENT_EXTINGUISH_FIRE: init_extinguish_fire(); return true;
	case EVENT_PLACE_BRICK: init_place_brick(); return true;
	case EVENT_SPRAY_IRON_WALL: init_spray_iron_wall(); return true;
	case EVENT_MIX_CHEMICALS: init_mix_chemicals(); return true;
	case EVENT_CUT_BALOON: init_cut_balloon(); return true;
	case EVENT_REVEAL_DARK_ROOM: init_reveal_dark_room(); return true;
	case EVENT_SUMMON_MOON: init_summon_moon(); return true;
	case EVENT_OPEN_CRATER_HOLE: init_crater_hole(); return true;
	case EVENT_DRILL_HOLES: init_drill_holes(); return true;
	case EVENT_VOLCANO_ERUPTION: init_volcano(); return true;
	case EVENT_OPEN_BLUE_SNAKE_DOOR: init_snake_door(ADDR_Pals_b81c0, ADDR_Pals_b8e80); return true;
	case EVENT_OPEN_GOLDEN_SNAKE_DOOR: init_snake_door(ADDR_Pals_b8180, ADDR_Pals_b8e40); return true;
	default: return false;
	}
}

bool Cutscenes::play(uint8_t event)
{
	switch (event) {
	case EVENT_CUT_TREE: cut_tree(); return true;
	case EVENT_OPEN_NORTH_GATE: open_north_gate(); return true;
	case EVENT_EXPEL_TORNADO: expel_tornado(); return true;
	case EVENT_RAINSTORM: rainstorm(); return true;
	case EVENT_FIX_ELEVATOR: fix_elevator(); return true;
	case EVENT_YELLOW_MUSIC_BOX: music_box(1); return true;
	case EVENT_BLUE_MUSIC_BOX: music_box(3); return true;
	case EVENT_GREEN_MUSIC_BOX: music_box(5); return true;
	case EVENT_RED_MUSIC_BOX: music_box(7); return true;
	case EVENT_GOLD_MUSIC_BOX: music_box(9); return true;
	case EVENT_PLANT_SEEDS: plant_seeds(); return true;
	case EVENT_RAISE_TOWER: raise_tower(); return true;
	case EVENT_BLOW_MIST: blow_mist(); return true;
	case EVENT_SUMMON_SNAKES: summon_snakes(); return true;
	case EVENT_FREEZE_SEA:
	case EVENT_SUMMON_LIGHTNING: freeze_sea(); return true;
	case EVENT_FOOT_STONE: foot_stone(); return true;
	case EVENT_PURIFY_WATER: purify_water(); return true;
	case EVENT_REVEAL_CASTLE: reveal_castle(); return true;
	case EVENT_SUMMON_SUN: summon_sun(); return true;
	case EVENT_EXPLODE_BOMBS: explode_bombs(); return true;
	case EVENT_LEAVES_FALL: leaves_fall(); return true;
	case EVENT_TREASURE_MAP: treasure_map(); return true;
	case EVENT_OPEN_TREE_MOUTH: open_tree_mouth(); return true;
	case EVENT_PLACE_CART_WHEELS: cart_wheels(); return true;
	case EVENT_MAKE_WIRE: make_wire(); return true;
	case EVENT_RAISE_ICE_BLOCKS: raise_ice_blocks(); return true;
	case EVENT_TORCH_FOREST: torch_forest(); return true;
	case EVENT_REVEAL_WARPED_VOID: reveal_warped_void(); return true;
	case EVENT_RAISE_PIPE: raise_pipe(); return true;
	case EVENT_REMOVE_WARPS: remove_warps(); return true;
	case EVENT_COLLECT_DEMONS_BLOOD: demons_blood(); return true;
	case EVENT_OPEN_PARAGOOM_CAGE: paragoom_cage(); return true;
	case EVENT_PLACE_PROPELLER: place_propeller(); return true;
	case EVENT_FEED_OCTOHON: feed_octohon(); return true;
	case EVENT_OPEN_SKY_DOOR: sky_door(); return true;
	case EVENT_EXTINGUISH_FIRE: extinguish_fire(); return true;
	case EVENT_PLACE_BRICK: place_brick(); return true;
	case EVENT_SPRAY_IRON_WALL: spray_iron_wall(); return true;
	case EVENT_MIX_CHEMICALS: mix_chemicals(); return true;
	case EVENT_CUT_BALOON: cut_balloon(); return true;
	case EVENT_REVEAL_DARK_ROOM: reveal_dark_room(); return true;
	case EVENT_SUMMON_MOON: summon_moon(); return true;
	case EVENT_OPEN_CRATER_HOLE: crater_hole(); return true;
	case EVENT_DRILL_HOLES: drill_holes(); return true;
	case EVENT_VOLCANO_ERUPTION: volcano(); return true;
	case EVENT_OPEN_BLUE_SNAKE_DOOR:
	case EVENT_OPEN_GOLDEN_SNAKE_DOOR: snake_door(); return true;
	default: return false;
	}
}

// The registers the jumptable leaves: a and hl the target, de the table
// offset (DoCutsceneFunc's steps also read their frame from [bc]).
void Cutscenes::call_c(uint16_t table, uint8_t event, uint16_t addr)
{
	uint8_t offset = static_cast<uint8_t>(event * 2);
	uint16_t sum = static_cast<uint16_t>(table + offset);
	uint8_t f = offset ? 0 : FLAG_Z; // add a
	if (((table & 0x0fff) + offset) > 0x0fff) // add hl, de
		f |= FLAG_H;
	if (sum < table)
		f |= FLAG_C;
	gb_->cpu.f = f;
	gb_->cpu.a = static_cast<uint8_t>(addr);
	SET_HL(gb_, addr);
	SET_DE(gb_, offset);
	routines_.call(addr);
}

// ---------------------------------------------------------------------------
// Helpers

// AdvanceCutsceneFunc: the next step, from its frame 0
void Cutscenes::advance()
{
	wram_.unk_2d014() = 0;
	++wram_.unk_2d013();
}

// WaitCutsceneFunc
void Cutscenes::wait(uint8_t frames)
{
	if (frame() < frames)
		return;
	advance();
}

// EndCutscene, Func_9cd16: the cutscene's over, after frames
void Cutscenes::end(uint8_t frames)
{
	if (frame() < frames)
		return;
	finish();
}

void Cutscenes::finish()
{
	++wram_.sub_state();
	stop_sfx();
}

void Cutscenes::stop_sfx()
{
	mem_[hSFXID + 0] = 0xff;
	mem_[hSFXID + 1] = 0x00;
}

// Func_9ccf9 (and the steps' own copies): the object at (x, y) in state
void Cutscenes::place(uint16_t obj, uint8_t y, uint8_t x, uint8_t state)
{
	mem_[obj] = y;
	mem_[static_cast<uint16_t>(obj + 1)] = x;
	set_state(obj, state);
}

void Cutscenes::set_state(uint16_t obj, uint8_t state)
{
	gb_->cpu.a = state;
	SET_HL(gb_, static_cast<uint16_t>(obj + 6));
	routines_.call(ADDR_SetSceneObjState);
}

// Func_9cd40
void Cutscenes::set_bg_pals_to_load()
{
	wram_.pal_config1_source_hi() = wTempPals1 >> 8;
	wram_.pal_config1_source_lo() = wTempPals1 & 0xff;
	wram_.pal_config1_index() = BGPI_AUTOINC | 0;
	wram_.pal_config1_number() = 8;
	wram_.pal_config1_register() = static_cast<uint8_t>(rBGPI);
}

// Func_9cd5a
void Cutscenes::set_ob_pals_to_load()
{
	wram_.pal_config2_source_hi() = wTempPals2 >> 8;
	wram_.pal_config2_source_lo() = wTempPals2 & 0xff;
	wram_.pal_config2_index() = OBPI_AUTOINC | 0;
	wram_.pal_config2_number() = 8;
	wram_.pal_config2_register() = static_cast<uint8_t>(rOBPI);
}

void Cutscenes::copy_far(uint8_t bank, uint16_t src, uint16_t dest, uint8_t count)
{
	gb_->cpu.b = bank;
	gb_->cpu.c = count;
	SET_HL(gb_, src);
	SET_DE(gb_, dest);
	routines_.call(ADDR_CopyFarBytes);
}

// Func_9cdcd: every 6 frames, wSCY by the next of 7 steps, then back by a
// pixel and the next step
void Cutscenes::shake_screen()
{
	static_assert(wCollectionLinkStateCounter + 1 == wCollectionScrollMode);
	static constexpr uint8_t DY[7] = {0xfc, 0x04, 0xfd, 0x03, 0xfe, 0x02, 0xff};
	Byte frames = wram_.collection_scroll_mode();
	++frames;
	Byte step = wram_.collection_link_state_counter();
	if (frames < 6)
		return;
	if (step < 7) {
		wram_.scy() += DY[static_cast<uint8_t>(step)];
		frames = 0;
		++step;
		return;
	}
	wram_.scy() += 1;
	frames = 0;
	step = 0;
	advance();
}

// .asm_9e77c and the like
void Cutscenes::next_fade(uint8_t bank, uint16_t pals)
{
	routines_.call(ADDR_Func_9cd74);
	load_pals1(bank, pals);
	advance();
}

// Func_9cd8d
void Cutscenes::fade_in()
{
	routines_.farcall(BANK_Func_85046, ADDR_Func_85046);
}

// the decompress_* macros (decompress_attrmap and _tiles1 in VRAM bank 1)
void Cutscenes::decompress(uint8_t bank, uint16_t src, uint16_t dest, bool vram1)
{
	if (vram1)
		mem_[rVBK] = 1;
	wram_.temp_bank() = bank;
	SET_HL(gb_, src);
	SET_BC(gb_, dest);
	routines_.farcall(bank, ADDR_Decompress);
	if (vram1)
		mem_[rVBK] = 0;
}

void Cutscenes::load_pals1(uint8_t bank, uint16_t pals)
{
	gb_->cpu.b = bank;
	SET_HL(gb_, pals);
	routines_.call(ADDR_LoadFarPalsToTempPals1);
}

void Cutscenes::load_pals2(uint8_t bank, uint16_t pals)
{
	gb_->cpu.b = bank;
	SET_HL(gb_, pals);
	routines_.call(ADDR_LoadFarPalsToTempPals2);
}

void Cutscenes::rise(uint16_t obj, uint8_t sound_at, uint8_t stop_at, uint8_t dx)
{
	Byte at = mem_[obj];
	--at;
	if (dx)
		mem_[static_cast<uint16_t>(obj + 1)] += dx;
	if (at == sound_at) {
		play_sfx(mem_, SFX_112); // Func_9ce1f
		return;
	}
	if (at == stop_at)
		advance();
}

// Func_9f0a5: the state's frameset in table (1-based)
uint16_t Cutscenes::frameset(uint16_t table, uint8_t state)
{
	uint8_t index = static_cast<uint8_t>(state - 1);
	return mem_.word(static_cast<uint16_t>(table + index * 2));
}

// Func_9f0ae (UpdateOWAnimation): the object's animation, by the address of
// its state; returns the object
uint16_t Cutscenes::animate(uint16_t state, uint16_t frameset, uint8_t bank)
{
	// dec c / dec c: the duration, two bytes before the state
	uint16_t duration = static_cast<uint16_t>((state & 0xff00) | static_cast<uint8_t>(state - 2));
	SET_DE(gb_, frameset);
	SET_HL(gb_, duration);
	gb_->cpu.b = bank;
	routines_.call(ADDR_UpdateOWAnimation);
	return static_cast<uint16_t>((duration & 0xff00) | (duration & 0xf8));
}

// Func_9f0d0: when the animation's over, the object's next state from its
// first frame; whether it was over
bool Cutscenes::next_state_when_done(uint16_t obj, uint8_t next)
{
	if (wram_.ow_animation_finished() != 0xff)
		return false;
	uint16_t duration = static_cast<uint16_t>(obj | 1 << 2);
	mem_[duration] = 0;
	mem_[static_cast<uint16_t>(duration + 1)] = 0; // frameset offset
	mem_[static_cast<uint16_t>(duration + 2)] = next; // state
	return true;
}

// Func_9f0bc and the like (LoadOWSprite)
void Cutscenes::draw(uint16_t obj, uint16_t oam, uint8_t bank)
{
	SET_HL(gb_, obj);
	SET_DE(gb_, oam);
	gb_->cpu.b = bank;
	routines_.call(ADDR_LoadOWSprite);
}

// Func_9fb53: while wSceneObj1YCoord is y_check, a sprite of frame at the
// object's coords, through a 4-byte slot at dest (in wCurSceneObj)
void Cutscenes::draw_shadow(uint16_t obj, uint16_t dest, uint8_t y_check, uint8_t frame)
{
	if (mem_[wSceneObj1YCoord] != y_check)
		return;
	mem_[dest] = mem_[obj];
	mem_[static_cast<uint16_t>(dest + 1)] = mem_[static_cast<uint16_t>((obj & 0xff00) | static_cast<uint8_t>(obj + 1))];
	mem_[static_cast<uint16_t>(dest + 2)] = frame;
	mem_[static_cast<uint16_t>(dest + 3)] = 0; // attributes
	draw(static_cast<uint16_t>(dest & 0xfffc), ADDR_OAM_bc000); // Func_9fb61
}

// .Func_9f52e and the like: the sound at count 1, the count back to 0 at
// period
void Cutscenes::loop_sound(uint16_t counter, uint8_t period, uint16_t sfx)
{
	Byte c = mem_[counter];
	++c;
	if (c == 1) {
		play_sfx(mem_, sfx);
		return;
	}
	if (c >= period)
		c = 0;
}

// .Func_9f4d8 and the like
void Cutscenes::periodic_sound(uint16_t counter, uint8_t period, uint16_t sfx)
{
	Byte c = mem_[counter];
	++c;
	if (c < period)
		return;
	c = 0;
	play_sfx(mem_, sfx);
}

// Func_9f3cd and the like: each object (by the address of its state) in
// its state's frameset, drawn with oam
void Cutscenes::animate_all(std::initializer_list<uint16_t> states, uint16_t framesets, uint16_t oam)
{
	for (uint16_t state : states) {
		uint8_t s = mem_[state];
		if (!s)
			continue;
		draw(animate(state, frameset(framesets, s)), oam);
	}
}

void Cutscenes::animate_all(std::initializer_list<uint16_t> states, uint16_t framesets, uint16_t oam,
	std::initializer_list<Next> next)
{
	for (uint16_t state : states) {
		for (;;) {
			uint8_t s = mem_[state];
			if (!s)
				break;
			uint16_t obj = animate(state, frameset(framesets, s));
			uint8_t now = mem_[state];
			bool again = false;
			for (const Next &n : next) {
				if (now == n.from) {
					again = next_state_when_done(obj, n.to);
					break;
				}
			}
			if (again)
				continue;
			draw(obj, oam);
			break;
		}
	}
}

} // namespace wl3::cutscenes

// Only farcalled, by UpdateCutscene, which then calls ClearUnusedVirtualOAM.
PORT_EX(DoCutsceneFunc, 0, ALL_REGISTERS)
{
	wl3::cutscenes::Cutscenes(gb).update();
}

// _InitCutscene calls it, and loads every register it uses after.
PORT_EX(InitEventCutscene, 0, ALL_REGISTERS)
{
	wl3::cutscenes::Cutscenes(gb).init();
}

// bank27's shared helpers, all reimplemented here (the steps' helpers and
// the scene object animation).
PORT_COVERED(AdvanceCutsceneFunc);
PORT_COVERED(WaitCutsceneFunc);
PORT_COVERED(StartCutsceneDelay);
PORT_COVERED(EndCutscene);
PORT_COVERED(EndCutsceneDelay_100Frames);
PORT_COVERED(EndCutsceneDelay_120Frames);
PORT_COVERED(EndCutsceneDelay_20Frames);
PORT_COVERED(EndCutsceneDelay_40Frames);
PORT_COVERED(EndCutsceneDelay_60Frames);
PORT_COVERED(EndCutsceneDelay_80Frames);
PORT_COVERED(Func_9ccf9);
PORT_COVERED(Func_9cd16);
PORT_COVERED(Func_9cd40);
PORT_COVERED(Func_9cd5a);
PORT_COVERED(Func_9cd8d);
PORT_COVERED(Func_9cd9d);
PORT_COVERED(Func_9cdad);
PORT_COVERED(Func_9cdbd);
PORT_COVERED(Func_9cdcd);
PORT_COVERED(Func_9ce1f);
PORT_COVERED(Func_9f0a5);
PORT_COVERED(Func_9f0ae);
PORT_COVERED(Func_9f0b5);
PORT_COVERED(Func_9f0bc);
PORT_COVERED(Func_9f0c1);
PORT_COVERED(Func_9f0c6);
PORT_COVERED(Func_9f0cb);
PORT_COVERED(Func_9f0d0);
PORT_COVERED(Func_9fb4c);
PORT_COVERED(Func_9fb53);
PORT_COVERED(Func_9fb61);
PORT_COVERED(Func_9fb6c);
PORT_COVERED(Func_9fb75);
PORT_COVERED(Func_9fb7e);
PORT_COVERED(Func_9fb97);
PORT_COVERED(Func_9fba0);
