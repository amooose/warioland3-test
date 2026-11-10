// The map's events and their treasures (bank20.asm). See overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

void Events::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

bool Events::collected(uint8_t treasure)
{
	return home::is_treasure_collected(gb_, treasure);
}

// GetEventWithTreasure: the event (of EventTreasures' threes) with the
// treasure, or EVENT_00; and where its search stopped (after the treasure,
// or after the table's end)
Events::Found Events::event_with_treasure(uint8_t treasure)
{
	uint16_t entry = ADDR_EventTreasures;
	auto next = [&] { return RomPtr(gb_, BANK_EventTreasures, entry++)[0]; };
	for (uint8_t event = EVENT_00;; event++) {
		uint8_t first = next();
		if (first == TREASURES_END)
			return {EVENT_00, entry, false};
		if (first == treasure)
			return {event, entry, true};
		if (next() == treasure)
			return {event, entry, true};
		if (next() == treasure)
			return {event, entry, true};
	}
}

// LoadEventTreasures: the event's three treasures, to w2dfff, w2dffe and
// w2dffd
void Events::load_event_treasures(uint8_t event)
{
	RomPtr entry = ROM(gb_, EventTreasures) + event * 3;
	mem_.unk_2dfff() = entry[0];
	mem_.unk_2dffe() = entry[1];
	mem_.unk_2dffd() = entry[2];
}

// CheckIfEventIsUnlocked: whether the event's treasures are all collected
// (or are the last one collected, which clears last_was_new... in the
// asm, c = FALSE); stops at the first missing one
Events::Unlocked Events::event_unlocked(uint8_t event)
{
	load_event_treasures(event);
	Unlocked result = {true, true};
	for (uint16_t slot : {w2dffd, w2dffe, w2dfff}) { // CheckEventTreasure
		uint8_t treasure = mem_[slot];
		if (treasure == INVALID_TREASURE)
			continue;
		if (treasure == wram_.last_transition_param()) {
			result.not_last = false;
			continue;
		}
		if (!collected(treasure)) {
			result.all = false;
			return result;
		}
	}
	return result;
}

// Func_819c6: unlocked, and not by the last treasure
uint8_t Events::unlocked_before(uint8_t event)
{
	Unlocked unlocked = event_unlocked(event);
	return unlocked.all && unlocked.not_last;
}

// CheckIfTreasureUnlocksEvent: whether the last treasure collected
// unlocks its event (left in wCurEvent; else EVENT_00)
bool Events::treasure_unlocks_event()
{
	uint8_t treasure = wram_.last_transition_param();
	wram_.cur_event() = EVENT_00;
	Found found = event_with_treasure(treasure);
	if (found.found) {
		wram_.cur_event() = found.event;
		if (event_unlocked(found.event).all)
			return true;
	}
	wram_.cur_event() = EVENT_00;
	return false;
}

// GetNextTreasureToCollect: once the next treasure's collected (or was
// just), the first of its event's and the following ones not collected
void Events::next_treasure_to_collect()
{
	uint8_t next = wram_.next_treasure();
	if (!collected(next)) {
		uint8_t transition = wram_.transition_param();
		switch (transition) {
		case 0:
		case TRANSITION_RETURN_TO_MAP:
		case TRANSITION_EPILOGUE_NOT_PERFECT:
		case TRANSITION_EPILOGUE_PERFECT:
		case TRANSITION_GAME_OVER:
			return;
		}
		if (transition == TRANSITION_NEW_GAME)
			transition = 0;
		if (transition != next)
			return;
	}
	uint16_t entry = event_with_treasure(next).next;
	for (;;) {
		uint8_t treasure = mem_[entry];
		if (treasure != INVALID_TREASURE && !collected(treasure)) {
			wram_.next_treasure() = treasure;
			return;
		}
		if (mem_[entry++] == TREASURES_END) {
			wram_.next_treasure() = TREASURES_END;
			return;
		}
	}
}

// GetMapLevelID: the map side (by MapSideInitialLevels) and its level
// for a level
Events::SideLevel Events::map_level_id(uint8_t level)
{
	uint16_t entry = ADDR_MapSideInitialLevels;
	uint8_t side = 0xff;
	uint8_t first;
	do {
		first = mem_[entry++];
		++side;
	} while (static_cast<uint8_t>(mem_[entry] - 1) < level);
	return {side, static_cast<uint8_t>(level - first)};
}

// GetMapSideAndLevelForNextTreasure: where the next treasure is (before
// the game's cleared, only up to the gold music box's event), else the
// temple
Events::SideLevel Events::next_treasure_side_level()
{
	uint8_t next = wram_.next_treasure();
	if (next != TREASURES_END) {
		uint8_t event = event_with_treasure(next).event;
		if (event <= EVENT_GOLD_MUSIC_BOX || (wram_.game_mode_flags() & (1 << MODE_GAME_CLEARED_F))) {
			uint8_t level = 0;
			routines_.in_bank(BANK_GetLevelThatContainsTreasure, ADDR_GetLevelThatContainsTreasure,
				[&] { level = level_with_treasure(gb_, wram_.next_treasure()); });
			return map_level_id(level);
		}
	}
	return {NORTH, OWNORTH_THE_TEMPLE};
}

// ShowMissingTreasureIndicators: after the game's cleared, a mark on each
// of the side's levels with treasures left (but the temple)
void Events::show_missing_treasure_indicators()
{
	if (!wram_.missing_treasure_indicators_enabled())
		return;
	mem_.unk_2dffd() = wram_.ow_level();
	wram_.ow_level() = mem_[static_cast<uint16_t>(ADDR_MapSideInitialLevels + wram_.cur_map_side())];
	uint8_t count;
	switch (wram_.next_map_side()) {
	case WEST: count = NUM_WEST_LEVELS; break;
	case SOUTH: count = NUM_SOUTH_LEVELS; break;
	case EAST: count = NUM_EAST_LEVELS; break;
	default: count = NUM_NORTH_LEVELS; break;
	}
	mem_.unk_2dfff() = count;
	mem_.unk_2dffe() = 0;
	bool skip = !wram_.ow_level(); // the temple
	do {
		if (!skip) {
			routines_.farcall(BANK_GetOWLevelCollectedTreasures, ADDR_GetOWLevelCollectedTreasures);
			if ((wram_.key_and_treasure_flags() & TREASURES_MASK) != TREASURES_MASK) {
				// .PlaceTreasureMissingIndicator
				uint16_t coords = map_level_coords(gb_, mem_.unk_2dffe());
				wram_.owy_coord1() = mem_[coords];
				wram_.owx_coord1() = mem_[coords + 1];
				uint16_t tile = home::ow_coord_in_tilemap(gb_, wOWCoords1);
				mem_[tile] = 0x7b;
				mem_[static_cast<uint16_t>(tile - 0x300)] = 0x0d; // its attribute
			}
		}
		skip = false;
		++wram_.ow_level();
		++mem_.unk_2dffe();
	} while (mem_.unk_2dffe() != mem_.unk_2dfff());
	wram_.ow_level() = mem_.unk_2dffd();
}

// ---------------------------------------------------------------------------
// The top bar's buttons

// The top bar's selectable buttons: the day and night spell (once found,
// and in the east with the sun medallion), the cutscene (with one to
// replay), the map sides there's a way to
void Events::set_top_bar_buttons()
{
	// SetDayNightSpellSelectable
	if (collected(DAY_OR_NIGHT_SPELL) && (wram_.cur_map_side() != EAST || wram_.got_sun_medallion()))
		wram_.top_bar_selectable_buttons() |= 1 << TOPBAR_DAY_NIGHT_F;
	// SetCutsceneButtonSelectable
	uint8_t cutscene = wram_.stored_transition_param();
	if (cutscene != TRANSITION_RETURN_TO_MAP && cutscene)
		wram_.top_bar_selectable_buttons() |= 1 << TOPBAR_CUTSCENE_F;
	// SetNextPrevMapButtonsSelectable, .CheckAccessibleMapSides: the west
	// (bit 3), south, east (bit 1) and north (bits 0 and 4)
	static constexpr struct {
		uint8_t event, shift;
	} SIDES[] = {
		{EVENT_OPEN_NORTH_GATE, 3}, {EVENT_FIX_ELEVATOR, 2}, {EVENT_BLOW_MIST, 1}, {EVENT_TORCH_FOREST, 0},
	};
	for (auto side : SIDES)
		wram_.accessible_map_sides() |= static_cast<uint8_t>(event_unlocked(side.event).all << side.shift);
	Byte sides = wram_.accessible_map_sides();
	sides |= static_cast<uint8_t>((sides & 1) << 4);
	wram_.top_bar_selectable_buttons() |= accessible_map_flags();
	// UpdateNextPrevMapButtonsSelectable: the selection off a side there's
	// no way to
	if (!wram_.ow_bars_state())
		return;
	Byte selection = wram_.top_bar_selection();
	uint8_t selected = selection & (TOPBAR_NEXT_MAP | TOPBAR_PREV_MAP);
	if (!selected || (accessible_map_flags() & selected))
		return;
	if (selection & TOPBAR_NEXT_MAP)
		selection = static_cast<uint8_t>((selection & ~TOPBAR_NEXT_MAP) | TOPBAR_PREV_MAP);
	else
		selection = static_cast<uint8_t>((selection & ~TOPBAR_PREV_MAP) | TOPBAR_NEXT_MAP);
}

// GetAccessibleMapFlags: the next and previous sides' bits, from the
// current side's
uint8_t Events::accessible_map_flags()
{
	uint8_t sides = wram_.accessible_map_sides();
	for (uint8_t side = wram_.cur_map_side(); side; side--)
		sides = static_cast<uint8_t>(sides << 1);
	return sides & (TOPBAR_NEXT_MAP | TOPBAR_PREV_MAP);
}

// LoadLevelNameIfValid: the level name of the level Wario's at (not an
// exit)
void Events::load_level_name_if_valid()
{
	uint8_t level = wram_.map_side_level_id();
	if (level >= 0x08)
		return;
	home::load_level_name(gb_, wram_.next_map_side(), level);
}

} // namespace wl3::overworld

// Called by the map's C++, which reads only what these return.
PORT_EX(CheckIfTreasureUnlocksEvent, 0, ALL_REGISTERS) { wl3::overworld::Events(gb).treasure_unlocks_event(); }
PORT_EX(GetNextTreasureToCollect, 0, ALL_REGISTERS) { wl3::overworld::Events(gb).next_treasure_to_collect(); }
PORT_EX(ShowMissingTreasureIndicators, 0, ALL_REGISTERS) { wl3::overworld::Events(gb).show_missing_treasure_indicators(); }

// Only the routines above and the map's C++ run these.
PORT_COVERED(CheckIfEventIsUnlocked);
PORT_COVERED(Func_819c6);
PORT_COVERED(CheckEventTreasure);
PORT_COVERED(LoadEventTreasures);
PORT_COVERED(GetEventWithTreasure);
PORT_COVERED(GetMapLevelID);
PORT_COVERED(GetMapSideAndLevelForNextTreasure);
PORT_COVERED(SetDayNightSpellSelectable);
PORT_COVERED(SetCutsceneButtonSelectable);
PORT_COVERED(SetCutsceneButtonSelectable_GotTransitionParam);
PORT_COVERED(SetNextPrevMapButtonsSelectable);
PORT_COVERED(UpdateNextPrevMapButtonsSelectable);
PORT_COVERED(GetAccessibleMapFlags);
PORT_COVERED(LoadLevelNameIfValid);
