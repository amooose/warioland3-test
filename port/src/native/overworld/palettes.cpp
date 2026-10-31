// The map's palettes and tile updates (bank21.asm). See overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

constexpr uint8_t FADE_BASE = 0, FADE_TARGET = 1, FADE_SIGN = 2, FADE_DIFF = 3,
	FADE_ACCUMULATOR = 4, FADE_CURRENT = 5, FADE_LENGTH = 8;
constexpr uint8_t FADE_COLOURS = 8 * 4; // 8 palettes of 4
constexpr uint8_t TRANSITION_STEPS = 32; // DAYNIGHT_TRANSITION_STEPS

uint8_t rotate_right(uint8_t v, int n) { return static_cast<uint8_t>(v >> n | v << (8 - n)); }
uint8_t rotate_left(uint8_t v, int n) { return static_cast<uint8_t>(v << n | v >> (8 - n)); }

constexpr uint8_t HIGH_TEMP_PALS1 = wTempPals1 >> 8;
constexpr uint8_t HIGH_TEMP_BG_PALS = wTempBGPals >> 8; // and wTempOBPals

} // namespace

void Palettes::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

// ---------------------------------------------------------------------------
// Fading a PalConfig

// FadePalConfig: a step (wPalFadeRepeat of them) of wPalConfigToFade's
// palettes, from wTempBGPals or wTempOBPals toward wTempPals1 or
// wTempPals2 (0x100 below), each colour a unit closer; the steps counted
// at steps. Whether it's still short of 0x20 steps (the asm's carry).
bool Palettes::fade_pal_config(uint16_t steps)
{
	Byte total = mem_[wCurPalTotalSteps];
	total = mem_[steps];
	do {
		uint16_t config = mem_[wPalConfigToFade] ? wPalConfig2Number : wPalConfig1Number;
		uint8_t colours = rotate_left(mem_[config], 2); // palettes * 4
		uint8_t hi = mem_[config + 1];
		uint8_t e = mem_[config + 2];
		uint8_t target_hi = static_cast<uint8_t>(hi - 1);
		auto at = [](uint8_t h, uint8_t l) { return static_cast<uint16_t>(h << 8 | l); };
		// each colour's bytes are read and written in the asm's order (a
		// source in wTargetRGB's page would see its own writes)
		auto unpack = [&](uint8_t page, uint16_t rgb) {
			uint16_t lo = at(page, e), up = at(page, static_cast<uint8_t>(e + 1));
			mem_[rgb] = mem_[lo] & 0x1f; // red
			mem_[rgb + 1] = (mem_[lo] & 0xe0) >> 5;
			mem_[rgb + 1] = static_cast<uint8_t>((mem_[up] & 0x03) << 3 | mem_[rgb + 1]); // green
			mem_[rgb + 2] = (mem_[up] & 0x7c) >> 2; // blue
		};
		auto toward = [&](uint16_t cur, uint16_t target) {
			uint8_t to = mem_[target];
			Byte value = mem_[cur];
			if (value < to)
				++value;
			else if (value > to)
				--value;
		};
		do {
			unpack(target_hi, wTargetRGB); // the target colour
			unpack(HIGH_TEMP_BG_PALS, wTargetRGB + 3); // the current one
			toward(wTargetRGB + 5, wTargetRGB + 2);
			toward(wTargetRGB + 4, wTargetRGB + 1);
			toward(wTargetRGB + 3, wTargetRGB + 0);
			uint8_t r = mem_[wTargetRGB + 3];
			mem_[at(HIGH_TEMP_BG_PALS, e)] = static_cast<uint8_t>((mem_[wTargetRGB + 4] & 0x07) << 5 | r);
			uint8_t g = (mem_[wTargetRGB + 4] & 0x18) >> 3;
			mem_[at(HIGH_TEMP_BG_PALS, static_cast<uint8_t>(e + 1))] =
				static_cast<uint8_t>(rotate_left(mem_[wTargetRGB + 5], 2) | g);
			gb_->cpu.c = g;
			e = static_cast<uint8_t>(e + 2);
			gb_->cpu.e = e;
			target_hi = HIGH_TEMP_PALS1; // the asm resets d to wTempPals1's page
		} while (--colours);
		++total;
		Byte repeat = mem_[wPalFadeRepeat];
		--repeat;
		if (!repeat)
			break;
	} while (true);
	mem_[steps] = total;
	// the registers as the asm leaves them (_PrologueFade's C caller
	// passes them on): c from the last colour, de past it
	uint8_t steps_done = total;
	gb_->cpu.a = steps_done;
	gb_->cpu.f = static_cast<uint8_t>(FLAG_N | (steps_done == 0x20 ? FLAG_Z : 0) | (steps_done < 0x20 ? FLAG_C : 0)
		| ((steps_done & 0x0f) < (0x20 & 0x0f) ? FLAG_H : 0));
	gb_->cpu.b = 0;
	gb_->cpu.d = HIGH_TEMP_PALS1;
	SET_HL(gb_, steps);
	return steps_done < 0x20;
}

// FadeBGPalConfig: wPalConfig1, fast (4 steps a frame) outside events
bool Palettes::fade_bg()
{
	mem_[wPalConfig1Register] = static_cast<uint8_t>(rBGPI);
	mem_[wPalConfigToFade] = 0;
	mem_[wPalFadeRepeat] = wram_.cur_event() ? 1 : 4;
	return fade_pal_config(wPalConfig1TotalSteps);
}

// Func_851bc: wPalConfig1, slowly
bool Palettes::fade_bg_slow()
{
	mem_[wPalConfig1Register] = static_cast<uint8_t>(rBGPI);
	mem_[wPalConfigToFade] = 0;
	mem_[wPalFadeRepeat] = 1;
	return fade_pal_config(wPalConfig1TotalSteps);
}

// Func_851d1: wPalConfig2, slowly
bool Palettes::fade_ob_slow()
{
	mem_[wPalConfig2Register] = static_cast<uint8_t>(rOBPI);
	mem_[wPalConfigToFade] = 1;
	mem_[wPalFadeRepeat] = 1;
	return fade_pal_config(wPalConfig2TotalSteps);
}

// The configs' sources: pals palettes from first (BG or OB)
void Palettes::config(uint16_t config, uint16_t src, uint8_t index, uint8_t count)
{
	mem_[config + 1] = index; // index
	mem_[config + 2] = count; // number
	mem_[config + 3] = static_cast<uint8_t>(src >> 8);
	mem_[config + 4] = static_cast<uint8_t>(src);
}

// FadeInLevelName: the level name's palette (7) fading in, blue for
// w2d0e4, else by the time of day; whether it's still going (the asm's
// carry, left in c)
bool Palettes::fade_in_level_name()
{
	bool going = true;
	if (!wram_.pal_config1_total_steps()) { // start it
		mem_[w2d0e0] = 0x02;
		routines_.copy(wTempPals1 + 7 * PAL_SIZE, wTempBGPals + 7 * PAL_SIZE, PAL_SIZE);
		if (mem_[w2d0e4])
			routines_.copy(ADDR_Pals_852dd + COLOR_SIZE, wTempPals1 + 7 * PAL_SIZE + COLOR_SIZE, 6);
		else
			routines_.copy((mem_[w2d011] ? ADDR_Pals_840c0 : ADDR_Pals_84000) + 7 * PAL_SIZE,
				wTempPals1 + 7 * PAL_SIZE, PAL_SIZE);
		config(wPalConfig1, wTempBGPals + 7 * PAL_SIZE, OBPI_AUTOINC | (7 * PAL_SIZE), 1);
		++wram_.pal_config1_total_steps();
	} else if (!fade_bg()) { // done
		wram_.pal_config1_total_steps() = 0;
		mem_[w2d0e0] = 0x01;
		going = false;
	}
	gb_->cpu.c = going;
	return going;
}

// Func_852e5: the level name fading out (w2d0e0 3), unless the bottom
// bar's busy
void Palettes::fade_out_level_name()
{
	if (mem_[w2d0e0] != 0x03 || wram_.bottom_bar_action())
		return;
	if (!wram_.pal_config1_total_steps()) {
		routines_.copy(wTempPals1 + 7 * PAL_SIZE + COLOR_SIZE, wTempBGPals + 7 * PAL_SIZE + COLOR_SIZE, 6);
		for (uint16_t i = 0; i < 6; i++)
			mem_[wTempPals1 + 7 * PAL_SIZE + COLOR_SIZE + i] = 0;
		++wram_.pal_config1_total_steps();
		config(wPalConfig1, wTempBGPals + 7 * PAL_SIZE, OBPI_AUTOINC | (7 * PAL_SIZE), 1);
		return;
	}
	if (fade_bg())
		return;
	wram_.pal_config1_total_steps() = 0;
	mem_[w2d0e0] = 0;
}

// The fades of all the palettes (Func_84e43, Func_84fff, Func_85145,
// Func_8518c, ...): first the configs, then a step a frame until done.
// Whether they're done.
bool Palettes::fade_all(bool ob)
{
	if (!wram_.pal_config1_total_steps()) {
		++wram_.pal_config1_total_steps();
		config(wPalConfig1, wTempBGPals, BGPI_AUTOINC | 0, 8);
		if (ob)
			config(wPalConfig2, wTempOBPals, OBPI_AUTOINC | 0, 8);
		return false;
	}
	if (ob)
		fade_ob_slow();
	return !fade_bg_slow();
}

// Func_84e43
void Palettes::func_84e43()
{
	SET_HL(gb_, wPalConfig1TotalSteps); // as the asm leaves it (Func_ac9ca, in C, passes it on)
	if (fade_all(true)) {
		wram_.pal_config1_total_steps() = 0;
		mem_[wPalConfig2TotalSteps] = 0;
	}
}

// Func_84fff, Func_85046, Func_850ff: when done, the next of w2d013's
// steps
void Palettes::next_w2d013()
{
	wram_.pal_config1_total_steps() = 0;
	mem_[w2d014] = 0;
	++mem_[w2d013];
}

void Palettes::func_84fff()
{
	if (fade_all(true))
		next_w2d013();
}

// Func_85046: palettes 1 to 7 and the objects' from white (0xff)
void Palettes::func_85046()
{
	if (wram_.pal_config1_total_steps()) {
		fade_ob_slow();
		if (!fade_bg_slow())
			next_w2d013();
		return;
	}
	routines_.copy(wTempPals1 + 1 * PAL_SIZE, wTempBGPals + 1 * PAL_SIZE, 7 * PAL_SIZE);
	for (uint16_t i = 0; i < 7 * PAL_SIZE; i++)
		mem_[wTempPals1 + 1 * PAL_SIZE + i] = 0xff;
	routines_.copy(wTempPals2, wTempOBPals, 8 * PAL_SIZE);
	for (uint16_t i = 0; i < 8 * PAL_SIZE; i++)
		mem_[wTempPals2 + i] = 0xff;
	++wram_.pal_config1_total_steps();
	config(wPalConfig1, wTempBGPals + 1 * PAL_SIZE, BGPI_AUTOINC | (1 * PAL_SIZE), 7);
	config(wPalConfig2, wTempOBPals, OBPI_AUTOINC | 0, 8);
}

// Func_850b9: the objects' palettes from white
void Palettes::func_850b9()
{
	if (mem_[wPalConfig2TotalSteps]) {
		if (fade_ob_slow())
			return;
		mem_[wPalConfig2TotalSteps] = 0;
		mem_[w2d014] = 0;
		++mem_[w2d013];
		return;
	}
	routines_.copy(wTempPals2, wTempOBPals, 8 * PAL_SIZE);
	for (uint16_t i = 0; i < 8 * PAL_SIZE; i++)
		mem_[wTempPals2 + i] = 0xff;
	++mem_[wPalConfig2TotalSteps];
	config(wPalConfig2, wTempOBPals, OBPI_AUTOINC | 0, 8);
}

// Func_850ff: BG palettes 1 to 7 from white
void Palettes::func_850ff()
{
	if (wram_.pal_config1_total_steps()) {
		if (!fade_bg_slow())
			next_w2d013();
		return;
	}
	routines_.copy(wTempPals1 + 1 * PAL_SIZE, wTempBGPals + 1 * PAL_SIZE, 7 * PAL_SIZE);
	for (uint16_t i = 0; i < 7 * PAL_SIZE; i++)
		mem_[wTempPals1 + 1 * PAL_SIZE + i] = 0xff;
	++wram_.pal_config1_total_steps();
	config(wPalConfig1, wTempBGPals + 1 * PAL_SIZE, BGPI_AUTOINC | (1 * PAL_SIZE), 7);
}

// Func_85145: for the map functions, then their next step
void Palettes::func_85145()
{
	if (fade_all(true)) {
		wram_.pal_config1_total_steps() = 0;
		wram_.ow_func_counter() = 0;
		++wram_.unk_2d062();
	}
}

// Func_8518c: the BG palettes only (with OBPI's index, as the asm)
void Palettes::func_8518c()
{
	if (!wram_.pal_config1_total_steps()) {
		config(wPalConfig1, wTempBGPals, OBPI_AUTOINC | 0, 8);
		++wram_.pal_config1_total_steps();
		return;
	}
	if (fade_bg_slow())
		return;
	wram_.pal_config1_total_steps() = 0;
	wram_.ow_func_counter() = 0;
	++wram_.unk_2d062();
}

// LoadOWStarIndicatorPals
void Palettes::star_indicator_pals()
{
	routines_.copy(ADDR_Pals_848e0, wTempPals2 + 4 * PAL_SIZE, 2 * PAL_SIZE);
	config(wPalConfig1, wTempPals2 + 4 * PAL_SIZE, OBPI_AUTOINC | (4 * PAL_SIZE), 2);
	mem_[wPalConfig1Register] = static_cast<uint8_t>(rOBPI);
}

// ---------------------------------------------------------------------------
// The day and night transitions: each colour's red, green and blue in a
// ColourFadeStruct (8 bytes: base, target, sign, diff, accumulator,
// current), stepping from base to target over DAYNIGHT_TRANSITION_STEPS


// CopyPalToColourFadeStructs: the 32 colours at src to the structs from
// dest (base or target): red, green, blue a struct apart
void Palettes::to_fade_structs(uint16_t src, uint16_t dest)
{
	Byte count = mem_[w2d807];
	count = FADE_COLOURS;
	do {
		uint8_t lo = mem_[src++];
		uint8_t hi = mem_[src++];
		mem_[dest] = lo & 0x1f;
		dest += FADE_LENGTH;
		mem_[dest] = static_cast<uint8_t>(((hi << 8 | lo) >> 5) & 0x1f);
		dest += FADE_LENGTH;
		mem_[dest] = (hi >> 2) & 0x1f;
		dest += FADE_LENGTH;
		--count;
	} while (count);
}

// InitColourFadeStructs: each struct's direction and distance
void Palettes::init_fade_structs()
{
	Byte count = mem_[w2d807];
	count = FADE_COLOURS * 3;
	uint16_t s = wFadePals;
	do {
		uint8_t base = mem_[s + FADE_BASE];
		uint8_t target = mem_[s + FADE_TARGET];
		uint8_t sign = 0, diff = 0;
		if (target > base) {
			sign = 1; // increase
			diff = static_cast<uint8_t>(target - base);
		} else if (target < base) {
			sign = 2; // decrease
			diff = static_cast<uint8_t>(base - target);
		}
		mem_[s + FADE_SIGN] = sign;
		mem_[s + FADE_DIFF] = diff;
		mem_[s + FADE_ACCUMULATOR] = diff;
		mem_[s + FADE_CURRENT] = base;
		s += FADE_LENGTH;
		--count;
	} while (count);
}

// ApplyPalFadeColours: the structs' current colours to wTempBGPals
void Palettes::apply_fade_structs()
{
	Byte count = mem_[w2d807];
	count = FADE_COLOURS;
	uint16_t s = wFadePals + FADE_CURRENT;
	uint16_t dest = wTempBGPals;
	for (;;) {
		uint8_t r = mem_[s];
		uint8_t g = mem_[s + FADE_LENGTH];
		uint8_t b = mem_[s + 2 * FADE_LENGTH];
		mem_[dest++] = static_cast<uint8_t>((rotate_right(g, 3) & 0xe0) | r);
		uint8_t c = rotate_left(g, 3);
		mem_[dest] = static_cast<uint8_t>((b << 2 | (c >> 7) << 1 | ((c >> 6) & 1)) & 0x7f);
		--count;
		if (!count)
			return;
		++dest;
		s += 3 * FADE_LENGTH;
	}
}

// DayNightTransition.AdvanceTable
void Palettes::next_transition()
{
	mem_[wOWPalTransitionCounter] = 0;
	++mem_[wOWPalTransitionState];
}

// GetOWPals: the entry of OWPals for the time of day, the map side (or
// north and west with all the crayons) and which
uint16_t Palettes::ow_pals(uint8_t which)
{
	uint8_t c = static_cast<uint8_t>(which * 2);
	uint8_t side = wram_.cur_map_side();
	if ((wram_.crayon_flags() & ALL_CRAYONS) == ALL_CRAYONS) { // .CheckCrayons
		if (side == NORTH)
			side = 4;
		else if (side == WEST)
			side = 5;
	}
	uint8_t e = rotate_left(side, 4);
	uint8_t time = rotate_right(rotate_left(mem_[w2d011], 4), 1);
	return static_cast<uint16_t>(ADDR_OWPals + static_cast<uint8_t>(time | e | c));
}

// LoadBaseAndTargetOWPals: entry which's base and target palettes, to fade
void Palettes::load_base_and_target(uint8_t which)
{
	uint16_t entry = ow_pals(which);
	uint16_t base = mem_.word(entry);
	uint16_t target = mem_.word(static_cast<uint16_t>(entry + 2));
	to_fade_structs(target, wFadePals + FADE_TARGET);
	to_fade_structs(base, wFadePals + FADE_BASE);
	init_fade_structs();
	routines_.copy(base, wTempBGPals, 8 * PAL_SIZE);
	mem_[wDayNightTransistionSteps] = TRANSITION_STEPS;
	mem_[w2d809] = 0;
}

// VBlank_OWPalTransition: its V-Blank function, the palettes each frame,
// copied to wVBlankFunc (it runs from there)
void Palettes::transition_vblank()
{
	gb_->cpu.ime = false; // di
	routines_.copy(ADDR_VBlank_OWPalTransition + 0xc, wVBlankFunc, 0x63 - 0xc);
	gb_->cpu.ime = true; // ei
	gb_->cpu.ime_delay = 0;
}

// DayNightTransition.LoadBaseAndTargetPals: the night's last palettes as
// the target, and the first fade
void Palettes::start_transition()
{
	transition_vblank();
	routines_.copy(mem_.word(ow_pals(3)), wTempPals1, 8 * PAL_SIZE);
	load_base_and_target(0);
	next_transition();
}

// DayNightTransition.FadeColours: a step every w2d809 frames, each colour
// (by its accumulated distance) a unit closer; B or a direction in the
// map's state 0x0b skips it
void Palettes::fade_colours()
{
	if (mem_[wSubState] == SST_OVERWORLD_0B) {
		uint8_t pressed = mem_[wJoypadPressed];
		if ((pressed & (1 << B_PAD_B)) || (mem_[wOWAllowedDPadInput] & pressed)) {
			skip_transition();
			return;
		}
	}
	if (mem_[wOWPalTransitionCounter] < mem_[w2d809])
		return;
	mem_[wOWPalTransitionCounter] = 0;
	Byte count = mem_[w2d807];
	count = FADE_COLOURS * 3;
	uint16_t s = wFadePals;
	do {
		uint8_t sign = mem_[s + FADE_SIGN];
		if (sign) {
			uint8_t step = sign == 1 ? 1 : 0xff;
			mem_[wColourFadeStep] = step;
			uint8_t diff = mem_[s + FADE_DIFF];
			mem_[wColourFadeDiff] = diff;
			uint8_t accumulated = mem_[s + FADE_ACCUMULATOR];
			mem_[wColourFadeAccumulator] = accumulated;
			uint8_t sum = static_cast<uint8_t>(accumulated + diff);
			if (sum >= TRANSITION_STEPS) {
				mem_[s + FADE_ACCUMULATOR] = static_cast<uint8_t>(sum - TRANSITION_STEPS);
				mem_[s + FADE_CURRENT] += step;
			} else {
				mem_[s + FADE_ACCUMULATOR] = sum;
			}
		}
		s += FADE_LENGTH;
		--count;
	} while (count);
	apply_fade_structs();
	Byte steps = mem_[wDayNightTransistionSteps];
	--steps;
	if (!steps)
		next_transition();
}

// SkipDayNightTransition: straight to the target palettes
void Palettes::skip_transition()
{
	routines_.copy(wTempPals1, wTempBGPals, 8 * PAL_SIZE);
	mem_[wDayNightTransistionSteps] = 0;
	mem_[wOWPalTransitionCounter] = 0;
	mem_[wOWPalTransitionState] = mem_[w2d011] ? 0x04 : 0x06;
}

// DayNightTransition.SwitchDayNight
void Palettes::switch_day_night()
{
	uint8_t night = mem_[w2d011] ^ 1;
	mem_[w2d011] = night;
	Byte day_night = mem_[wDayNight];
	day_night = static_cast<uint8_t>(((day_night >> 1) << 1 | (night & 1)) & 0x7f);
	routines_.call(ADDR_PlayOverworldMusic);
	mem_[w2d0e3] = 0;
	mem_[wOWPalTransitionState] = 0;
	mem_[wOWPalTransitionCounter] = 0;
}

// DayNightTransition: the map fading from day to night (through two
// palettes between) or back
void Palettes::day_night()
{
	auto fade_until = [this](uint8_t steps) { // and from there, every frame
		fade_colours();
		if (mem_[wDayNightTransistionSteps] == steps)
			mem_[w2d809] = 0;
	};

	++mem_[wOWPalTransitionCounter];
	uint8_t state = mem_[wOWPalTransitionState];
	if (mem_[w2d011]) { // .NightToDayTable
		switch (state) {
		case 0: // .InitNightToDay
			start_transition();
			mem_[w2d07c] = 0;
			mem_[w2d07d] = 0;
			mem_[w2d809] = 0;
			return;
		case 1: fade_until(10); return; // .FadeDayToPal1
		case 2: load_base_and_target(1); next_transition(); return; // .LoadPal1
		case 3: fade_until(0x16); return; // .FadePal1ToPal2_Night
		case 4: switch_day_night(); return;
		default: unrealistic(); return;
		}
	}
	switch (state) {
	case 0: // .InitDayToNight
		start_transition();
		mem_[w2d07c] = 0x01;
		mem_[w2d07d] = 0;
		mem_[w2d809] = 0;
		return;
	case 1: case 5: fade_colours(); return;
	case 2: load_base_and_target(1); next_transition(); return; // .LoadPal1_Day
	case 3: fade_until(12); return; // .FadePal1ToPal2_Day
	case 4: load_base_and_target(2); next_transition(); return; // .LoadPal2
	case 6: switch_day_night(); return;
	default: unrealistic(); return;
	}
}

// Func_84b97: the daytime scene's fade to day, slower, with the east's
// sun (scene object 7)
void Palettes::to_daytime()
{
	auto wait = [this](uint8_t frames) {
		if (mem_[wOWPalTransitionCounter] >= frames)
			next_transition();
	};

	++mem_[wOWPalTransitionCounter];
	switch (mem_[wOWPalTransitionState]) {
	case 0: // .Func_84bad
		start_transition();
		mem_[w2d809] = 0x03;
		return;
	case 1: wait(4); return; // .Func_84bb6
	case 2: // .Func_84bbf
		fade_colours();
		if (mem_[wDayNightTransistionSteps] == 0x0a)
			mem_[w2d809] = 0;
		return;
	case 3: wait(6); return; // .Func_84bcd
	case 4: // .Func_84bd6
		load_base_and_target(1);
		next_transition();
		mem_[w2d809] = 0x02;
		gb_->cpu.a = 0x13;
		SET_HL(gb_, wSceneObj7State);
		routines_.call(ADDR_SetSceneObjState);
		return;
	case 5: // .Func_84be7
		fade_colours();
		if (mem_[wDayNightTransistionSteps] == 0x16)
			mem_[w2d809] = 0x03;
		return;
	case 6: // .Func_84bf6
		if (!wram_.ow_bars_state())
			mem_[wDayNight] = 0;
		mem_[w2d011] = 0;
		mem_[w2d0e3] = 0;
		mem_[wOWPalTransitionState] = 0;
		mem_[wOWPalTransitionCounter] = 0;
		return;
	default:
		unrealistic();
		return;
	}
}

// Func_84c0d: the day and night spell's transition (no music)
void Palettes::spell_transition()
{
	++mem_[wOWPalTransitionCounter];
	switch (mem_[wOWPalTransitionState]) {
	case 0: // .Init
		transition_vblank();
		load_base_and_target(0);
		next_transition();
		return;
	case 1: case 3: case 5: fade_colours(); return;
	case 2: load_base_and_target(1); next_transition(); return;
	case 4: load_base_and_target(2); next_transition(); return;
	case 6: // .SwitchDayNight
		mem_[w2d011] = mem_[w2d011] ^ 0x01;
		mem_[w2d0e3] = 0;
		mem_[wOWPalTransitionState] = 0;
		mem_[wOWPalTransitionCounter] = 0;
		return;
	default:
		unrealistic();
		return;
	}
}

// OWGreyTransition: the map fading to grey (an exit unlocking)
void Palettes::grey_transition()
{
	++mem_[wOWPalTransitionCounter];
	switch (mem_[wOWPalTransitionState]) {
	case 0: // .Start
		to_fade_structs(wTempPals1, wFadePals + FADE_BASE);
		routines_.copy(wTempPals1, wTempBGPals, 8 * PAL_SIZE);
		load_grey();
		to_fade_structs(wTempPals1, wFadePals + FADE_TARGET);
		init_fade_structs();
		mem_[wDayNightTransistionSteps] = TRANSITION_STEPS;
		mem_[w2d809] = 0;
		transition_vblank();
		next_transition();
		return;
	case 1: fade_colours(); return;
	case 2: // .Finish
		mem_[w2d0e3] = 0;
		mem_[wOWPalTransitionState] = 0;
		mem_[wOWPalTransitionCounter] = 0;
		return;
	default:
		unrealistic();
		return;
	}
}

// LoadGreyOWPals: the side's grey palettes (the east's without its ice
// before the Blue Book and the Magic Wand), without the level name's
void Palettes::load_grey()
{
	uint8_t side = wram_.cur_map_side();
	uint16_t entry = static_cast<uint16_t>(ADDR_Data_84e39 + static_cast<uint8_t>(side * 2));
	if (side == EAST) { // .GetEastMapNoIcePalette
		auto collected = [this](uint8_t treasure) {
			gb_->cpu.a = treasure;
			routines_.call(ADDR_IsTreasureCollected);
			return !(gb_->cpu.f & FLAG_Z);
		};
		if (!collected(BLUE_BOOK) || !collected(MAGIC_WAND))
			entry += 2;
	}
	routines_.copy(mem_.word(entry), wTempPals1, 8 * PAL_SIZE);
	for (uint16_t i = 0; i < 6; i++)
		mem_[wTempPals1 + 7 * PAL_SIZE + COLOR_SIZE + i] = 0;
}

// _PrologueFade: the prologue's and epilogue's fades, by w2d880, from
// wTempBGPals/wTempOBPals to the scene's palettes; then w2d013's next step
void Palettes::prologue_fade()
{
	// hl as the asm leaves it (PrologueFade, in C, passes it on)
	SET_HL(gb_, wPalConfig1TotalSteps);
	if (wram_.pal_config1_total_steps()) { // .do_fade
		fade_ob_slow();
		if (fade_bg_slow())
			return;
		wram_.pal_config1_total_steps() = 0;
		mem_[wPalConfig2TotalSteps] = 0;
		mem_[w2d014] = 0;
		++mem_[w2d013];
		SET_HL(gb_, w2d013);
		return;
	}
	auto from_current = [this] { // .CopyPalsToBGAndOBPals
		routines_.copy(wTempPals1, wTempBGPals, 8 * PAL_SIZE);
		routines_.copy(wTempPals2, wTempOBPals, 8 * PAL_SIZE);
	};
	auto zero = [this](uint16_t dest, uint16_t count) { // WriteAToHL_BCTimes
		gb_->cpu.a = 0;
		SET_HL(gb_, dest);
		SET_BC(gb_, count);
		routines_.call(ADDR_WriteAToHL_BCTimes);
	};
	switch (mem_[w2d880]) {
	case 0: // .zero: to black
		from_current();
		zero(wTempPals1, 8 * PAL_SIZE); // ClearTempPals_Bank21
		zero(wTempPals2, 8 * PAL_SIZE);
		break;
	case 1: case 6: // .one / .six
		from_current();
		routines_.copy(ADDR_Pals_869cd, wTempPals1, 8 * PAL_SIZE);
		routines_.copy(ADDR_Pals_86a8d, wTempPals2, 8 * PAL_SIZE);
		if (mem_[w2d880] == 6)
			routines_.copy(ADDR_Pals_86acd, wTempPals2, PAL_SIZE);
		break;
	case 2: // .two
		from_current();
		routines_.copy(ADDR_Pals_86a4d, wTempPals1, 8 * PAL_SIZE);
		routines_.copy(ADDR_Pals_86a8d, wTempPals2, 8 * PAL_SIZE);
		routines_.copy(ADDR_Pals_86ad5, wTempPals2, PAL_SIZE);
		break;
	case 3: // .three
		from_current();
		zero(wTempPals1, 6 * PAL_SIZE);
		zero(wTempPals2 + PAL_SIZE, 7 * PAL_SIZE);
		break;
	case 4: // .four
		from_current();
		SET_HL(gb_, ADDR_Pals_db000);
		SET_DE(gb_, wTempPals1);
		gb_->cpu.c = 8 * PAL_SIZE;
		gb_->cpu.b = BANK_Pals_db000;
		routines_.call(ADDR_CopyFarBytes);
		SET_HL(gb_, ADDR_Pals_db040);
		SET_DE(gb_, wTempPals2 + 3 * PAL_SIZE);
		gb_->cpu.c = 5 * PAL_SIZE;
		gb_->cpu.b = BANK_Pals_db040;
		routines_.call(ADDR_CopyFarBytes);
		break;
	case 5: // .five
		from_current();
		routines_.copy(ADDR_Pals_86a4d + 6 * PAL_SIZE, wTempPals1 + 6 * PAL_SIZE, 2 * PAL_SIZE);
		routines_.copy(ADDR_Pals_86a8d, wTempPals2, 8 * PAL_SIZE);
		routines_.copy(ADDR_Pals_86ad5, wTempPals2, PAL_SIZE);
		break;
	default: // straight to the configs
		break;
	}
	config(wPalConfig1, wTempBGPals, BGPI_AUTOINC | 0, 8); // .set_pal_configs
	config(wPalConfig2, wTempOBPals, OBPI_AUTOINC | 0, 8);
	++wram_.pal_config1_total_steps();
	SET_HL(gb_, wPalConfig1TotalSteps);
}

// ---------------------------------------------------------------------------
// The map's tiles

// Func_85a68 / Func_85aa2 / Func_85acb: a column of the frozen sea's
// tiles (wFrozen... at base: count, then tile and source addresses),
// each with its attribute 3 pages below from 0xa0 on, stepping down rows
void Palettes::freeze_column(uint16_t base)
{
	for (;;) {
		uint16_t dest = mem_.word(static_cast<uint16_t>(base + 1));
		uint16_t src = mem_.word(static_cast<uint16_t>(base + 3));
		mem_[dest] = mem_[src];
		mem_[static_cast<uint16_t>(dest - 0x300)] = mem_[static_cast<uint16_t>(src + 0xa0)];
		Byte count = mem_[base];
		--count;
		if (!count)
			return;
		for (uint16_t field : {static_cast<uint16_t>(base + 1), static_cast<uint16_t>(base + 3)}) { // Func_85a91
			uint16_t row = static_cast<uint16_t>(mem_.word(field) + TILEMAP_WIDTH);
			mem_[field] = static_cast<uint8_t>(row);
			mem_[field + 1] = static_cast<uint8_t>(row >> 8);
		}
	}
}

// Func_854ee: runs of tiles and their attributes (3 pages up), each
// address then bytes to 0x70, until a 0 address
void Palettes::update_tiles(uint16_t data)
{
	while (uint8_t lo = mem_[data++]) {
		uint8_t hi = mem_[data++];
		uint16_t tiles = static_cast<uint16_t>(hi << 8 | lo);
		uint16_t attrs = static_cast<uint16_t>((hi - 3) << 8 | lo);
		for (uint8_t b; (b = mem_[data++]) != 0x70;)
			mem_[tiles++] = b;
		for (uint8_t b; (b = mem_[data++]) != 0x70;)
			mem_[attrs++] = b;
	}
}

} // namespace wl3::overworld

// Farcalled by the map's C++ (and _PrologueFade, Func_84e43 by bank2b's C,
// which reads no registers after); FadeInLevelName leaves c (1 while
// fading).
PORT_EX(DayNightTransition, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).day_night(); }
PORT_EX(Func_84b97, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).to_daytime(); }
PORT_EX(Func_84c0d, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).spell_transition(); }
PORT_EX(OWGreyTransition, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).grey_transition(); }
PORT_EX(LoadGreyOWPals, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).load_grey(); }
PORT_EX(FadeInLevelName, 0, R_A | R_F | R_B | R_D | R_E | R_H | R_L) { wl3::overworld::Palettes(gb).fade_in_level_name(); }
PORT_EX(Func_852e5, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).fade_out_level_name(); }
PORT_EX(Func_84e43, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).func_84e43(); }
PORT_EX(Func_84fff, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).func_84fff(); }
PORT_EX(Func_85046, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).func_85046(); }
PORT_EX(Func_850b9, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).func_850b9(); }
PORT_EX(Func_850ff, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).func_850ff(); }
PORT_EX(Func_85145, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).func_85145(); }
PORT_EX(Func_8518c, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).func_8518c(); }
PORT_EX(_PrologueFade, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).prologue_fade(); }
PORT_EX(LoadOWStarIndicatorPals, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).star_indicator_pals(); }
PORT_EX(Func_854ee, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).update_tiles(REG_HL(gb)); }
PORT_EX(Func_85a68, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).freeze_column(wOWUIObj1Attributes); }
PORT_EX(Func_85aa2, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).freeze_column(wOWUIObj2Attributes); }
PORT_EX(Func_85acb, 0, ALL_REGISTERS) { wl3::overworld::Palettes(gb).freeze_column(wd153); }

// Only the routines above run these.
PORT_COVERED(LoadBaseAndTargetOWPals);
PORT_COVERED(GetOWPals);
PORT_COVERED(InitColourFadeStructs);
PORT_COVERED(CopyPalToColourFadeStructs);
PORT_COVERED(ApplyPalFadeColours);
PORT_COVERED(SkipDayNightTransition);
PORT_COVERED(VBlank_OWPalTransition);
PORT_COVERED(ClearTempPals_Bank21);
PORT_COVERED(Func_851bc);
PORT_COVERED(Func_851d1);
PORT_COVERED(FadeBGPalConfig);
PORT_COVERED(FadePalConfig);
PORT_COVERED(Func_85a91);
PORT_COVERED(GetMapSidePalGroup1);
PORT_COVERED(GetMapSidePalGroup2);
