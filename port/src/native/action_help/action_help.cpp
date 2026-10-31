// Action Help; see action_help.hpp.
#include "action_help.hpp"
#include "demo.hpp"

namespace wl3::action_help {

namespace {

constexpr uint8_t VRAM1 = BANK_v1Tiles0;
constexpr uint8_t OBJECTS_BANK = 0x79; // the input prompts' framesets
constexpr uint8_t SLOT_SIZE = 2; // tiles wide and high
// the power-up bar, along the bottom of the screen
constexpr uint16_t BAR_ATTRS = v1BGMap1 + 16 * TILEMAP_WIDTH;
constexpr uint16_t BAR_TILES = v0BGMap1 + 16 * TILEMAP_WIDTH;
constexpr uint8_t NUM_BAR_SLOTS = 10;
constexpr uint16_t VBLANK_FUNC = ADDR_VBlank_1fb6b7 + 0x0c; // VBlank_1fb6b7.Func
constexpr uint8_t VBLANK_FUNC_SIZE = 0x34;

// writes a 2x2 tile block at addr, in the VRAM bank mapped
void fill_2x2(Memory mem, uint16_t addr, uint8_t value)
{
	mem[addr] = value;
	mem[addr + 1] = value;
	mem[addr + TILEMAP_WIDTH] = value;
	mem[addr + TILEMAP_WIDTH + 1] = value;
}

} // namespace

// ---------------------------------------------------------------------------
// The state table

void ActionHelp::run_state()
{
	switch (wram_.sub_state()) {
	case 0: routines_.call(ADDR_SlowFadeBGToWhite); break;
	case 1: init(); break;
	case 2: routines_.call(ADDR_DarkenBGToPal_Fast); break;
	case 3: update(); break;
	case 4: routines_.call(ADDR_SlowFadeBGToWhite); break;
	case 5: routines_.call(ADDR_ReturnToPauseMenuFromActionHelp); break;
	case 6: case 7: case 8: case 9: case 10: case 11: case 12: case 13: case 14:
		routines_.call(ADDR_DebugReset);
		break;
	default: unrealistic(); break;
	}
}

void ActionHelp::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

bool ActionHelp::main_power_up() const
{
	return (wram_.action_help_power_up() & POWER_UP_MASK) < NUM_MAIN_POWER_UPS;
}

// ---------------------------------------------------------------------------
// core.asm

void ActionHelp::init()
{
	routines_.disable_lcd();
	play_music(mem_, MUSIC_ACTION_HELP);
	wram_.action_help_state() = 0;
	wram_.timer() = 0;

	wram_.action_help_power_up() = pick_power_up();

	wram_.animated_tiles_frame_duration() = 0;
	wram_.animated_tiles_gfx() = 0;
	wram_.animated_tiles_frame_count() = 0;
	wram_.animated_tiles_frame() = 0;
	wram_.room_animated_tiles_enabled() = 0;

	routines_.call(ADDR_ClearBGMap0);
	routines_.clear_virtual_oam();
	routines_.in_bank(BANK_LoadActionHelpCommonPals, ADDR_LoadActionHelpCommonPals, [&] { load_common_pals(); });
	routines_.in_bank(BANK_LoadActionHelpPowerUpPals, ADDR_LoadActionHelpPowerUpPals, [&] { load_power_up_pals(); });
	routines_.in_bank(BANK_LoadActionHelpGfx, ADDR_LoadActionHelpGfx, [&] { load_gfx(); });
	routines_.in_bank(BANK_LoadActionHelpBGMaps, ADDR_LoadActionHelpBGMaps, [&] { load_bg_maps(); });
	set_vblank();

	ram(rSCX) = 0;
	wram_.scx() = 0;
	ram(rSCY) = 0x04;
	wram_.scy() = 0x04;

	// the cursor under the power-up shown, and the input prompts
	uint8_t power_up = wram_.action_help_power_up();
	uint8_t cursor_x = static_cast<uint8_t>((power_up << 4 | power_up >> 4) + 8);
	object(wMenuObj1).init(SCREEN_HEIGHT_PX, cursor_x, ADDR_Frameset_1e6b8e);
	object(wMenuObj2).init(0x77, 0x3f, ADDR_Frameset_1e6b76);
	MenuObject buttons = object(wMenuObj3);
	buttons.init(0x7d, 0x67, ADDR_Frameset_1e6b6d);
	buttons.animate(OBJECTS_BANK);
	load_sprite(buttons);

	// in the title screen's demo, the power-ups Wario has are coloured
	if ((wram_.power_up_level() & ACTION_HELP_TITLE_SCREEN) && power_up < NUM_MAIN_POWER_UPS) {
		ram(rVBK) = VRAM1;
		uint8_t level = wram_.power_up_level() & POWER_UP_MASK;
		colour_slot(level, 0x03);
		if (level >= power_up)
			colour_slot(power_up, mem_[ADDR_PowerUpOBPals + power_up]);
	}

	ram(rVBK) = 0;
	routines_.clear_unused_virtual_oam();
	ram(rLCDC) = LCDC_ON | LCDC_BG_9C00 | LCDC_OBJ_16 | LCDC_OBJ_ON | LCDC_BG_ON;
	++wram_.sub_state();
}

// Wario's power-up or transformation; in the title screen's demo, the next
// one each time.
uint8_t ActionHelp::pick_power_up()
{
	uint8_t demo = NO_POWER_UP;
	if (wram_.power_up_level() & ACTION_HELP_TITLE_SCREEN)
		demo = hram_.demo_power_up();

	uint8_t power_up;
	switch (wram_.transformation()) {
	case TRANSFORMATION_OWL_WARIO: power_up = POWER_UP_OWL; break;
	case TRANSFORMATION_RAIL: power_up = POWER_UP_RAIL; break;
	case TRANSFORMATION_VAMPIRE_WARIO: power_up = POWER_UP_VAMPIRE; break;
	default: power_up = wram_.power_up_level() & POWER_UP_MASK; break;
	}
	power_up = static_cast<uint8_t>(power_up + demo);
	if (power_up >= NUM_TOTAL_POWER_UPS)
		power_up = static_cast<uint8_t>(power_up - NUM_TOTAL_POWER_UPS);

	uint8_t next_demo = static_cast<uint8_t>(hram_.demo_power_up() + 1);
	hram_.demo_power_up() = next_demo >= NUM_TOTAL_POWER_UPS ? 0 : next_demo;
	return power_up;
}

void ActionHelp::update()
{
	// lower the overall volume
	routines_.execute_audio_mod(AUDIOMOD_VOLUME, 0x20, SFX_CHANNELS, MUSIC_CHANNELS >> 4);

	if (!(wram_.power_up_level() & 0x80) && (wram_.joypad_pressed() & (PAD_A | PAD_B | PAD_START))) {
		close();
		return;
	}
	++wram_.action_help_counter();

	if (!main_power_up()) {
		routines_.in_bank(BANK_ExecuteSpecialPowerUpActionHelp, ADDR_ExecuteSpecialPowerUpActionHelp,
			[&] { run_special_power_up(); });
	} else {
		routines_.call(ADDR_ProcessDPadRepeat);
		if (!(wram_.power_up_level() & (ACTION_HELP_TITLE_SCREEN | ACTION_HELP_LEVEL_CLEAR)))
			select_power_up();
		run_main_power_up();
		update_menu_object(object(wMenuObj1)); // the cursor
	}
	update_menu_object(object(wMenuObj2)); // the d-pad
	update_menu_object(object(wMenuObj3)); // the buttons
	routines_.clear_unused_virtual_oam();
}

void ActionHelp::close()
{
	if (wram_.power_up_level() & ACTION_HELP_TITLE_SCREEN) {
		routines_.call(ADDR_InitWithoutDemoPowerUpReset); // back to the title
		return;
	}
	// back to the pause menu
	wram_.animated_tiles_frame_duration() = 0;
	wram_.animated_tiles_gfx() = 0;
	wram_.animated_tiles_frame_count() = 0;
	wram_.animated_tiles_frame() = 0;
	wram_.unk_3d50d() = 0;
	routines_.call(ADDR_ResumeNormalVolume);
	++wram_.sub_state();
}

void ActionHelp::update_menu_object(MenuObject obj)
{
	obj.animate(OBJECTS_BANK);
	load_sprite(obj);
}

void ActionHelp::run_main_power_up()
{
	uint8_t power_up = wram_.action_help_power_up();
	if (power_up >= NUM_MAIN_POWER_UPS) {
		unrealistic();
		return;
	}
	switch (power_up) {
	case NO_POWER_UP: NoPowerUpDemo(gb_).run(); break;
	case LEAD_OVERALLS: LeadOverallsDemo(gb_).run(); break;
	case SWIMMING_FLIPPERS: SwimmingFlippersDemo(gb_).run(); break;
	case HEAD_SMASH_HELMET: HeadSmashHelmetDemo(gb_).run(); break;
	case GRAB_GLOVE: GrabGlovesDemo(gb_).run(); break;
	case GARLIC: GarlicDemo(gb_).run(); break;
	case SUPER_JUMP_SLAM_OVERALLS: SuperJumpSlamOverallsDemo(gb_).run(); break;
	case HIGH_JUMP_BOOTS: HighJumpBootsDemo(gb_).run(); break;
	case PRINCE_FROGS_GLOVES: PrinceFrogsGlovesDemo(gb_).run(); break;
	case SUPER_GRAB_GLOVES: SuperGrabGlovesDemo(gb_).run(); break;
	default: unrealistic(); break; // checked above
	}
}

void ActionHelp::run_special_power_up()
{
	uint8_t index = static_cast<uint8_t>(wram_.action_help_power_up() - POWER_UP_OWL);
	if (index >= NUM_SPECIAL_POWER_UPS) {
		unrealistic();
		return;
	}
	switch (index) {
	case POWER_UP_OWL - POWER_UP_OWL: OwlDemo(gb_).run(); break;
	case POWER_UP_RAIL - POWER_UP_OWL: RailDemo(gb_).run(); break;
	case POWER_UP_VAMPIRE - POWER_UP_OWL: VampireDemo(gb_).run(); break;
	}
}

// ---------------------------------------------------------------------------
// The power-up bar

void ActionHelp::set_vblank()
{
	routines_.copy(VBLANK_FUNC, wVBlankFunc, VBLANK_FUNC_SIZE);
}

// Left and right move between the power-ups Wario can see: the one left
// goes back to its colour, the one picked shows its own palette.
void ActionHelp::select_power_up()
{
	Byte power_up = wram_.action_help_power_up();
	uint8_t pressed = wram_.joypad_pressed();
	uint8_t previous = power_up;
	if (pressed & PAD_RIGHT) {
		if (power_up == NUM_MAIN_POWER_UPS - 1)
			return;
		++power_up;
		wram_.action_help_state() = 0;
		object(wMenuObj1).x() += 0x10;
	} else if (pressed & PAD_LEFT) {
		if (!power_up)
			return;
		--power_up;
		wram_.action_help_state() = 0;
		object(wMenuObj1).x() -= 0x10;
	} else {
		return;
	}

	wram_.action_help_blacked_out() = TRUE;
	ram(rVBK) = VRAM1;
	uint8_t level = wram_.power_up_level() & POWER_UP_MASK;
	if (level >= previous) {
		routines_.call(ADDR_WaitNextHBlank);
		colour_slot(previous, 0x03);
	}
	uint8_t picked = power_up;
	if (level >= picked) {
		uint8_t pals = mem_[ADDR_PowerUpOBPals + picked];
		routines_.call(ADDR_WaitNextHBlank);
		colour_slot(picked, pals);
	}
	ram(rVBK) = 0;
	play_sfx(mem_, SFX_0E2);
}

// the slot's attributes, with VRAM1 mapped
void ActionHelp::colour_slot(uint8_t slot, uint8_t attributes)
{
	uint8_t offset = static_cast<uint8_t>(slot * SLOT_SIZE);
	fill_2x2(mem_, static_cast<uint16_t>(BAR_ATTRS + offset), attributes);
}

// ---------------------------------------------------------------------------
// common2.asm: graphics

void ActionHelp::load_common_pals()
{
	wram_.action_help_blacked_out() = TRUE;
	routines_.load_pals_to_temp_pals1(ADDR_Pals_1e4eef);
}

// The OB palettes: garlic and the later power-ups, and the vampire, recolour
// the first two.
void ActionHelp::load_power_up_pals()
{
	bool main = main_power_up();
	routines_.load_pals_to_temp_pals2(main ? ADDR_Pals_1e4f2f : ADDR_Pals_1e4f7f);
	uint8_t power_up = wram_.action_help_power_up();
	if (main ? power_up < GARLIC : power_up != POWER_UP_VAMPIRE)
		return;
	routines_.copy_short(main ? ADDR_Pals_1e4f6f : ADDR_Pals_1e4fbf, wTempPals2, 2 * PAL_SIZE);
}

void ActionHelp::load_gfx()
{
	ram(rVBK) = VRAM1;
	routines_.decompress(main_power_up() ? ADDR_ActionHelpObjects1Gfx : ADDR_ActionHelpObjects2Gfx, v1Tiles0);
	ram(rVBK) = 0;
	routines_.decompress(ADDR_ActionHelpHUDGfx, v0Tiles0);
}

// The power-up bar: the power-ups Wario has coloured, the rest empty; all
// empty for a transformation.
void ActionHelp::load_bg_maps()
{
	ram(rVBK) = VRAM1;
	routines_.decompress(ADDR_ActionHelpAttrmap, v1BGMap1);
	ram(rVBK) = 0;
	routines_.decompress(ADDR_ActionHelpTilemap, v0BGMap1);

	uint8_t empty = NUM_BAR_SLOTS;
	if (main_power_up()) {
		uint8_t level = wram_.power_up_level() & POWER_UP_MASK;
		if (level) {
			ram(rVBK) = VRAM1;
			for (uint8_t slot = 0; slot < level; slot++)
				fill_2x2(mem_, static_cast<uint16_t>((BAR_ATTRS & 0xff00) | static_cast<uint8_t>(BAR_ATTRS + slot * SLOT_SIZE)), 0x03);
		}
		ram(rVBK) = 0;
		empty = static_cast<uint8_t>(NUM_BAR_SLOTS - 1 - level);
		if (!empty)
			return;
	}
	// from the last slot leftwards
	uint16_t addr = BAR_TILES + (NUM_BAR_SLOTS - 1) * SLOT_SIZE;
	do {
		draw_empty_slot(addr);
		addr = static_cast<uint16_t>(addr - SLOT_SIZE);
	} while (--empty);
}

void ActionHelp::draw_empty_slot(uint16_t addr)
{
	ram(rVBK) = VRAM1;
	fill_2x2(mem_, addr, 0x07);
	ram(rVBK) = 0;
	ram(addr) = 0x64;
	ram(addr + 1) = 0x65;
	ram(addr + TILEMAP_WIDTH) = 0x74;
	ram(addr + TILEMAP_WIDTH + 1) = 0x75;
}

void ActionHelp::load_sprite(MenuObject obj)
{
	routines_.in_bank(BANK_LoadActionHelpSprite, ADDR_LoadActionHelpSprite,
		[&] { obj.load_sprite(ADDR_OAM_1e697c); });
}

} // namespace wl3::action_help

// Called from ActionHelpStateTable, whose callers use no register after it
// (see src/ported/engine/action_help/common.c).
PORT_EX(_ActionHelpStateTable, 0, ALL_REGISTERS)
{
	wl3::action_help::ActionHelp(gb).run_state();
}

// The routines implemented inside this module (they have no hooks).
PORT_COVERED(InitActionHelp);
PORT_COVERED(UpdateActionHelp);
PORT_COVERED(ExecuteMainPowerUpActionHelp);
PORT_COVERED(ExecuteSpecialPowerUpActionHelp);
PORT_COVERED(VBlank_1fb6b7);
PORT_COVERED(Func_1fb6f7);
PORT_COVERED(LoadActionHelpCommonPals);
PORT_COVERED(LoadActionHelpPowerUpPals);
PORT_COVERED(LoadActionHelpGfx);
PORT_COVERED(LoadActionHelpBGMaps);
PORT_COVERED(LoadActionHelpSprite);
