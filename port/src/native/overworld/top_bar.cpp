// The map's top bar (bank20.asm): opened with Select, its buttons pick the
// day/night spell, the cutscene, the treasure collection or the next map
// side. Two UI objects draw it: the selected button, and the NEXT MAP
// indicator. See overworld.hpp.
#include "overworld.hpp"

#include <initializer_list>

namespace wl3::overworld {

namespace {

static_assert(TOPBAR_DAY_NIGHT_F == 0 && TOPBAR_PREV_MAP_F == 4);
static_assert(OWBARS_INPUT_TOP == 4 && OWBARS_DAY_NIGHT_TRANSITION == 11);

// the window's attributes, from row 19 of wAttrmap
constexpr uint16_t ATTRS = wAttrmap + 19 * TILEMAP_WIDTH;
constexpr uint16_t DAY_NIGHT_ATTRS = ATTRS + 11;
constexpr uint16_t CUTSCENE_ATTRS = ATTRS + 9;
constexpr uint16_t COLLECTION_ATTRS = ATTRS + 7;

// a button's 2x2 tiles' attributes: the top row and the bottom row
using ButtonAttrs = uint8_t[8];
constexpr ButtonAttrs NOT_SELECTABLE = {0x3c, 0x3c, 0x3d, 0x3d, 0x0f, 0x2f, 0x0f, 0x2f}; // Data_826be
constexpr ButtonAttrs DAY_NIGHT = {0x3e, 0x3e, 0x3f, 0x3f, 0x0f, 0x2f, 0x0f, 0x2f}; // Data_826c6
constexpr ButtonAttrs CUTSCENE = {0x3a, 0x3a, 0x3b, 0x3b, 0x0f, 0x2f, 0x0f, 0x2f}; // Data_826ce
constexpr ButtonAttrs COLLECTION = {0x38, 0x38, 0x39, 0x39, 0x0f, 0x2f, 0x0f, 0x2f}; // Data_826d6
constexpr ButtonAttrs SELECTED = {0x34, 0x35, 0x36, 0x37, 0x0f, 0x0f, 0x0f, 0x0f}; // Data_826de
constexpr ButtonAttrs COLLECTION_SELECTED = {0x30, 0x31, 0x32, 0x33, 0x0f, 0x0f, 0x0f, 0x0f}; // Data_826e6
constexpr ButtonAttrs PRESSED = {0x48, 0x49, 0x4a, 0x4b, 0x0f, 0x0f, 0x0f, 0x0f}; // Data_826ee

// ShowTopBarSelectedButton.button_data: the selected button's x and state
struct Button {
	uint8_t x, state;
};
constexpr Button BUTTONS[6] = {
	{0x60, 0x07}, // Day Spell
	{0x50, 0x03}, // Cutscene
	{0x40, 0x01}, // Collection
	{0x2c, 0x0d}, // Next Map
	{0x0c, 0x09}, // Prev Map
	{0x60, 0x05}, // Night Spell
};
constexpr uint8_t NEXT_MAP_INDICATOR = 0x0b;
constexpr uint8_t PREV_MAP_PRESSED = 0x0a;

// UpdateTopBar.Indices: the level on a map side to start at, coming from
// its left (next map) or its right (previous map)
constexpr uint8_t MAP_SIDE_ENTRIES[4][2] = {
	{OWNORTH_THE_PEACEFUL_VILLAGE, OWNORTH_THE_VAST_PLAIN},
	{OWWEST_DESERT_RUINS, OWWEST_A_TOWN_IN_CHAOS},
	{OWSOUTH_THE_GRASSLANDS, OWSOUTH_TOWER_OF_REVIVAL},
	{OWEAST_THE_STAGNANT_SWAMP, OWEAST_FOREST_OF_FEAR},
};

// Func_82521: the UI objects' framesets, by state (0 for none); states 6
// and 8 then go on to state 7 and 5 when their animation ends
constexpr uint16_t FRAMESETS[15] = {
	0, ADDR_Frameset_aa091, ADDR_Frameset_aa094, ADDR_Frameset_aa097,
	ADDR_Frameset_aa09a, ADDR_Frameset_aa09d, ADDR_Frameset_aa0bc, ADDR_Frameset_aa0a3,
	ADDR_Frameset_aa0c1, ADDR_Frameset_aa0a9, ADDR_Frameset_aa0ae, ADDR_Frameset_aa0b1,
	ADDR_Frameset_aa0c6, ADDR_Frameset_aa0b4, ADDR_Frameset_aa0b9,
};
constexpr uint8_t OW_ANIMATION_BANK = 0x2a;

} // namespace

void TopBar::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

void TopBar::update()
{
	++wram_.ow_bars_state_counter();
	switch (wram_.ow_bars_state()) {
	case OWBARS_NONE: return;
	case OWBARS_START_CLOSE_BOT: start_close_bottom_bar(); return;
	case OWBARS_HANDLE_CLOSE_BOT: handle_bottom_bar_close(); return;
	case OWBARS_OPEN_TOP: open(); return;
	case OWBARS_INPUT_TOP: handle_input(); return;
	case OWBARS_CLOSE_TOP: close(); return;
	case OWBARS_EVENT: cutscene(); return;
	case OWBARS_COLLECTION: collection(); return;
	case OWBARS_NEXT_MAP: next_map(); return;
	case OWBARS_DAY_NIGHT: day_night(); return;
	case OWBARS_DAY_NIGHT_DELAY: delay_day_night_transition(); return;
	case OWBARS_DAY_NIGHT_TRANSITION: do_day_night_transition(); return;
	default: unrealistic(); return;
	}
}

// ---------------------------------------------------------------------------
// The states

void TopBar::start_close_bottom_bar()
{
	routines_.call(ADDR_DrawBottomBar);
	wram_.bottom_bar_action() = BOTBAR_CLOSING;
	wram_.bottom_bar_state() = 0;
	wram_.magnifying_glass_counter() = 0;
	++wram_.ow_bars_state();
}

void TopBar::handle_bottom_bar_close()
{
	routines_.call(ADDR_HandleBottomBar);
	routines_.call(ADDR_DrawBottomBar);
	if (wram_.bottom_bar_action())
		return;
	play_sfx(mem_, SFX_0E9);
	wram_.top_bar_selection() = TOPBAR_COLLECTION;
	init_buttons();
	apply_button_attributes();
	set_obj_pals_to_load();
	++wram_.ow_bars_state();
}

// UpdateTopBar.InitTopBarButtons
void TopBar::init_buttons()
{
	show_selected_button();
	load_button_attributes();
	wram_.owui_obj1_y_coord() = 0;
	wram_.owui_obj2_y_coord() = 0;
	wram_.owui_obj2_x_coord() = 0x1c;
	draw();
	copy_far_bytes(BANK_Pals_84a20, ADDR_Pals_84a20, wTempPals2 + 4 * PAL_SIZE, 4 * PAL_SIZE);
}

void TopBar::open()
{
	Byte scy = wram_.scy();
	scy -= 2;
	if (scy == 0xec) {
		wram_.unk_2d02c() = wram_.transition_param();
		++wram_.ow_bars_state();
	}
	draw();
}

void TopBar::handle_input()
{
	if (!handle_selection() && !handle_dpad()
		&& (wram_.joypad_pressed() & (PAD_B | PAD_SELECT))) {
		play_sfx(mem_, SFX_0EA);
		++wram_.ow_bars_state();
	}
	draw();
}

void TopBar::close()
{
	Byte scy = wram_.scy();
	scy += 2;
	if (scy == 0x04) {
		wram_.unk_2d013() = 0;
		wram_.ow_bars_state() = OWBARS_NONE;
		// the map side's object palettes, back from the top bar's
		uint16_t pals = static_cast<uint16_t>(ADDR_Pals_84900 + 4 * PAL_SIZE + wram_.cur_map_side() * 8 * PAL_SIZE);
		copy_far_bytes(BANK_Pals_84900, pals, wTempPals2 + 4 * PAL_SIZE, 4 * PAL_SIZE);
		set_obj_pals_to_load();
	}
	draw();
}

// The cutscene, collection and next map buttons wait 30 frames, or for A,
// before doing their thing; the bar is then back to taking input.
bool TopBar::wait_for_selection()
{
	if (!wram_.joypad_pressed().bit(B_PAD_A) && wram_.ow_bars_state_counter() < 30)
		return false;
	wram_.ow_bars_state_counter() = 0;
	wram_.ow_bars_state() = OWBARS_INPUT_TOP;
	return true;
}

void TopBar::cutscene()
{
	if (wait_for_selection()) {
		wram_.transition_param() = wram_.stored_transition_param();
		wram_.sub_state() = 0;
	}
	draw();
}

void TopBar::collection()
{
	if (wait_for_selection())
		wram_.sub_state() = SST_OVERWORLD_COLLECTION;
	draw();
}

void TopBar::next_map()
{
	if (wait_for_selection()) {
		bool prev = wram_.owui_obj1_state() == PREV_MAP_PRESSED;
		Byte side = wram_.next_map_side();
		side = (side + (prev ? 0xff : 1)) & 0x03;
		uint8_t level = MAP_SIDE_ENTRIES[side][prev ? 1 : 0];
		wram_.map_side_level_id() = level;
		wram_.unk_2d068() = level;
		stop_music2(mem_);
		wram_.unk_2d013() = 0;
		wram_.unk_2d014() = 0;
		wram_.sub_state() = SST_OVERWORLD_08;
	}
	draw();
}

// The spell button's animation casts the spell at its frame 4 or 6.
void TopBar::day_night()
{
	draw();
	uint8_t frame = wram_.owui_obj1_frame();
	if (frame != 4 && frame != 6)
		return;
	routines_.call(ADDR_DoDayNightSpell);
	load_button_attributes();
	apply_button_attributes();
	++wram_.ow_bars_state();
}

void TopBar::delay_day_night_transition()
{
	uint8_t state = wram_.owui_obj1_state();
	if (state == 0x05 || state == 0x07)
		++wram_.ow_bars_state();
	draw();
}

void TopBar::do_day_night_transition()
{
	routines_.farcall(BANK_DayNightTransition, ADDR_DayNightTransition);
	if (!wram_.ow_pal_transition_state()) {
		gb_->cpu.ime = false; // di
		routines_.call(ADDR_VBlank_Overworld);
		gb_->cpu.ime = true; // ei
		gb_->cpu.ime_delay = 0;
		wram_.ow_bars_state() = OWBARS_INPUT_TOP;
	}
	draw();
}

// ---------------------------------------------------------------------------
// Input

// HandleTopBarSelection: A does the selected button's thing; whether it did
bool TopBar::handle_selection()
{
	if (!wram_.joypad_pressed().bit(B_PAD_A))
		return false;
	Byte selection = wram_.top_bar_selection();
	if (selection.bit(TOPBAR_DAY_NIGHT_F)) {
		press_button(DAY_NIGHT_ATTRS);
		wram_.ow_bars_state() = OWBARS_DAY_NIGHT;
	} else if (selection.bit(TOPBAR_CUTSCENE_F)) {
		if (!wram_.stored_transition_param())
			return false;
		press_button(CUTSCENE_ATTRS);
		wram_.ow_bars_state() = OWBARS_EVENT;
		wram_.ow_bars_state_counter() = 0;
	} else if (selection.bit(TOPBAR_COLLECTION_F)) {
		press_button(COLLECTION_ATTRS);
		wram_.ow_bars_state() = OWBARS_COLLECTION;
		wram_.ow_bars_state_counter() = 0;
		wram_.is_collection_open() = FALSE;
	} else if (selection.bit(TOPBAR_NEXT_MAP_F) || selection.bit(TOPBAR_PREV_MAP_F)) {
		advance_obj_state(wOWUIObj1State);
		advance_obj_state(wOWUIObj2State); // the NEXT MAP indicator flashes
		wram_.ow_bars_state() = OWBARS_NEXT_MAP;
		wram_.ow_bars_state_counter() = 0;
	} else {
		return false;
	}
	play_sfx(mem_, SFX_SELECTION);
	return true;
}

// HandleTopBarSelection.ShowPressedButton and .ApplyPressedButtonAttrs
void TopBar::press_button(uint16_t attrs)
{
	advance_obj_state(wOWUIObj1State);
	write_button_attrs(attrs, PRESSED);
	apply_button_attributes();
}

// A UI object's next state (the pressed button's animation), from its start.
void TopBar::advance_obj_state(uint16_t state)
{
	++mem_[state];
	mem_[state - 1] = 0; // frameset offset
	mem_[state - 2] = 0; // duration
}

// HandleTopBarDPad: right and left move the selection to the next button
// that can be selected; whether it moved
bool TopBar::handle_dpad()
{
	Byte pressed = wram_.joypad_pressed();
	uint8_t selectable = wram_.top_bar_selectable_buttons();
	uint8_t selection = wram_.top_bar_selection();
	if (!selection && (pressed.bit(B_PAD_RIGHT) || pressed.bit(B_PAD_LEFT))) {
		unrealistic(); // the asm looks for a button forever
		return false;
	}
	if (pressed.bit(B_PAD_RIGHT)) {
		// rrc b, stopping when the bit falls off the right
		do {
			bool out = selection & 1;
			selection = static_cast<uint8_t>(selection >> 1 | selection << 7);
			if (out)
				return false;
		} while (!(selectable & selection));
	} else if (pressed.bit(B_PAD_LEFT)) {
		do {
			selection = static_cast<uint8_t>(selection << 1);
			if (selection & (1 << (TOPBAR_PREV_MAP_F + 1)))
				return false;
			if (!selection) {
				unrealistic(); // only bits above the buttons': forever
				return false;
			}
		} while (!(selectable & selection));
	} else {
		return false;
	}
	wram_.top_bar_selection() = selection;
	show_selected_button();
	load_button_attributes();
	apply_button_attributes();
	play_sfx(mem_, SFX_0E2);
	return true;
}

// ---------------------------------------------------------------------------
// Drawing

// ShowTopBarSelectedButton: the selected button, and with the next or
// previous map selected, the NEXT MAP indicator
void TopBar::show_selected_button()
{
	Byte selection = wram_.top_bar_selection();
	int button;
	if (selection.bit(TOPBAR_DAY_NIGHT_F))
		button = wram_.unk_2d011() ? 5 : 0; // the night spell
	else if (selection.bit(TOPBAR_CUTSCENE_F))
		button = 1;
	else if (selection.bit(TOPBAR_COLLECTION_F))
		button = 2;
	else if (selection.bit(TOPBAR_NEXT_MAP_F))
		button = 3;
	else
		button = 4;
	wram_.owui_obj1_x_coord() = BUTTONS[button].x;
	wram_.owui_obj1_state() = BUTTONS[button].state;
	wram_.owui_obj1_frameset_offset() = 0;
	wram_.owui_obj1_duration() = 0;

	bool map = selection & (TOPBAR_NEXT_MAP | TOPBAR_PREV_MAP);
	wram_.owui_obj2_state() = map ? NEXT_MAP_INDICATOR : 0;
	wram_.owui_obj2_frameset_offset() = 0;
	wram_.owui_obj2_duration() = 0;
}

// LoadTopBarButtonAttributes: the day/night, cutscene and collection
// buttons, greyed out when they can't be selected, lit when selected
void TopBar::load_button_attributes()
{
	Byte selectable = wram_.top_bar_selectable_buttons();
	write_button_attrs(DAY_NIGHT_ATTRS, selectable.bit(TOPBAR_DAY_NIGHT_F) ? DAY_NIGHT : NOT_SELECTABLE);
	write_button_attrs(CUTSCENE_ATTRS, selectable.bit(TOPBAR_CUTSCENE_F) ? CUTSCENE : NOT_SELECTABLE);
	write_button_attrs(COLLECTION_ATTRS, selectable.bit(TOPBAR_COLLECTION_F) ? COLLECTION : NOT_SELECTABLE);

	Byte selection = wram_.top_bar_selection();
	if (selection.bit(TOPBAR_DAY_NIGHT_F))
		write_button_attrs(DAY_NIGHT_ATTRS, SELECTED);
	if (selection.bit(TOPBAR_CUTSCENE_F))
		write_button_attrs(CUTSCENE_ATTRS, SELECTED);
	if (selection.bit(TOPBAR_COLLECTION_F))
		write_button_attrs(COLLECTION_ATTRS, COLLECTION_SELECTED);
}

// Func_8269f: a button's 2x2 tiles at dest, then the same again two rows
// down (the rows the window shows, and the rows kept for it). The column
// steps stay in dest's low byte, as inc e does.
void TopBar::write_button_attrs(uint16_t dest, const uint8_t *attrs)
{
	auto at = [](uint16_t addr, uint8_t offset) {
		return static_cast<uint16_t>((addr & 0xff00) | static_cast<uint8_t>(addr + offset));
	};
	for (uint16_t row : {static_cast<uint16_t>(dest + 2 * TILEMAP_WIDTH), dest}) {
		mem_[row] = attrs[0];
		mem_[at(row, 1)] = attrs[1];
		mem_[at(row, TILEMAP_WIDTH)] = attrs[2];
		mem_[at(row, TILEMAP_WIDTH + 1)] = attrs[3];
		attrs += 4;
	}
}

// ApplyTopBarButtonAttributes: the buttons' attributes to the window, by
// the HDMA (both rows: the two DMA channels)
void TopBar::apply_button_attributes()
{
	constexpr uint16_t SOURCE = wAttrmap + 0x2a * TILE_SIZE;
	constexpr uint16_t SOURCE2 = wAttrmap + 0x26 * TILE_SIZE;
	wram_.hdma_source_hi() = SOURCE >> 8;
	wram_.hdma_source_lo() = SOURCE & 0xff;
	wram_.hdma_dest_hi() = 0x1b;
	wram_.hdma_dest_lo() = 0xc0;
	wram_.hdma_mode() = 4 - 1; // 4 tiles
	wram_.unk_2d0b5_source_hi() = SOURCE2 >> 8;
	wram_.unk_2d0b5_source_lo() = SOURCE2 & 0xff;
	wram_.unk_2d0b5_dest_hi() = 0x1b;
	wram_.unk_2d0b5_dest_lo() = 0xc0;
	wram_.unk_2d0b5_mode() = 4 - 1;
}

// DrawTopBar: the selected button, and the NEXT MAP indicator if shown
void TopBar::draw()
{
	wram_.ow_animation_finished() = 0;
	draw_obj(wram_.owui_obj1_state(), wOWUIObj1Duration, wOWUIObj1); // Func_824f1
	uint8_t indicator = wram_.owui_obj2_state(); // Func_8250a
	if (indicator)
		draw_obj(indicator, wOWUIObj2Duration, wOWUIObj2);
}

void TopBar::draw_obj(uint8_t state, uint16_t duration, uint16_t obj)
{
	animate(state, duration);
	gb_->cpu.b = BANK_OAM_a9fe2;
	SET_DE(gb_, ADDR_OAM_a9fe2);
	SET_HL(gb_, obj);
	routines_.call(ADDR_LoadOWSpriteWithScroll);
}

// Func_82521: the object's animation in its state. States 6 and 8 end in
// state 7 and 5: when their animation ends, the next state's animation
// starts (written through the hl UpdateOWAnimation leaves, with bit 2 set).
void TopBar::animate(uint8_t state, uint16_t duration)
{
	uint16_t hl = duration;
	for (;;) {
		if (state >= 15) {
			unrealistic();
			return;
		}
		if (!state)
			return;
		SET_DE(gb_, FRAMESETS[state]);
		SET_HL(gb_, hl);
		gb_->cpu.b = OW_ANIMATION_BANK;
		routines_.call(ADDR_UpdateOWAnimation);
		uint8_t next = state == 6 ? 0x07 : state == 8 ? 0x05 : 0;
		if (!next || wram_.ow_animation_finished() != 0xff)
			return;
		hl = static_cast<uint16_t>(REG_HL(gb_) | 1 << 2);
		uint8_t low = static_cast<uint8_t>(hl);
		uint16_t page = hl & 0xff00;
		mem_[hl] = 0;
		mem_[page | static_cast<uint8_t>(low + 1)] = 0;
		mem_[page | static_cast<uint8_t>(low + 2)] = next;
		state = next;
	}
}

// ---------------------------------------------------------------------------
// Palettes

// CopyFarBytes: count bytes from bank:src to dest
void TopBar::copy_far_bytes(uint8_t bank, uint16_t src, uint16_t dest, uint8_t count)
{
	SET_HL(gb_, src);
	SET_DE(gb_, dest);
	gb_->cpu.c = count;
	gb_->cpu.b = bank;
	routines_.call(ADDR_CopyFarBytes);
}

// The four object palettes from wTempPals2 palette 4, for the V-Blank.
void TopBar::set_obj_pals_to_load()
{
	constexpr uint16_t PALS = wTempPals2 + 4 * PAL_SIZE;
	wram_.pal_config1_register() = static_cast<uint8_t>(rOBPI);
	wram_.pal_config1_source_hi() = PALS >> 8;
	wram_.pal_config1_source_lo() = PALS & 0xff;
	wram_.pal_config1_index() = OBPI_AUTOINC | 4 * PAL_SIZE;
	wram_.pal_config1_number() = 4;
}

} // namespace wl3::overworld

// Func_80e89 calls it, and reloads every register it uses after it (a from
// wOWBarsState, and the bottom bar and scene objects theirs).
PORT_EX(UpdateTopBar, 0, ALL_REGISTERS)
{
	wl3::overworld::TopBar(gb).update();
}

PORT_COVERED(HandleTopBarDPad);
PORT_COVERED(ShowTopBarSelectedButton);
PORT_COVERED(DrawTopBar);
PORT_COVERED(Func_824f1);
PORT_COVERED(Func_8250a);
PORT_COVERED(Func_82521);
PORT_COVERED(HandleTopBarSelection);
PORT_COVERED(LoadTopBarButtonAttributes);
PORT_COVERED(Func_8269f);
PORT_COVERED(ApplyTopBarButtonAttributes);
