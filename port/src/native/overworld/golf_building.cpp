// The golf building, outside (bank2b.asm). See overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

// The scene's state, in a palette fade's bytes (unused here)
constexpr uint16_t PHASE = wPalFade6Colour2GreenCurrent;
constexpr uint16_t STEP = wPalFade6Colour2GreenTarget;
constexpr uint16_t FRAMES = wPalFade6Colour2GreenSign;

} // namespace

void GolfBuilding::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

void GolfBuilding::wario_state(uint8_t state)
{
	gb_->cpu.a = state;
	routines_.call(ADDR_SetSceneWarioState);
}

// ApplySceneWarioMovementLeft / Right: Wario's x after the step
uint8_t GolfBuilding::walk(bool right)
{
	routines_.call(right ? ADDR_ApplySceneWarioMovementRight : ADDR_ApplySceneWarioMovementLeft);
	return gb_->cpu.a;
}

// _GolfBuilding: the building's screen, Wario coming in from the side
// he came from (or on it, with all the crayons)
void GolfBuilding::init()
{
	routines_.disable_lcd();
	routines_.call(ADDR_FillBGMap0_With7f);
	routines_.clear_virtual_oam();
	stop_music2(mem_);
	mem_[rWBK] = 2; // BANK("WRAM2")
	mem_[rSCX] = 0;
	wram_.scx() = 0;
	mem_[rSCY] = 0;
	wram_.scy() = 0;
	// .VBlank_GolfBuilding: its V-Blank function, run from wVBlankFunc
	routines_.copy(ADDR__GolfBuilding + 0xfa, wVBlankFunc, 0x140 - 0xfa);
	routines_.call(ADDR_ClearGeneralSceneRAM);
	routines_.call(ADDR_ClearSceneObjsRAM);
	fill_bg_pals();
	SET_HL(gb_, ADDR_Pals_84900);
	SET_DE(gb_, wTempPals2);
	gb_->cpu.b = BANK_Pals_84900;
	gb_->cpu.c = 8 * PAL_SIZE;
	routines_.call(ADDR_CopyFarBytes);
	SET_HL(gb_, ADDR_PrologueBackgroundGfx);
	gb_->cpu.b = BANK_PrologueBackgroundGfx;
	routines_.call(ADDR_LoadFarTiles);
	decompress(BANK_GolfBuildingAttrmap, ADDR_GolfBuildingAttrmap, v1BGMap0, true);
	decompress(BANK_GolfBuildingTilemap, ADDR_GolfBuildingTilemap, v0BGMap0, false);
	place_wario();
	routines_.call(ADDR_UpdateSceneWario);
	mem_[rLCDC] = LCDC_DEFAULT;
	++wram_.sub_state();
}

// The decompress_tilemap and decompress_attrmap macros
void GolfBuilding::decompress(uint8_t bank, uint16_t src, uint16_t dest, bool vram1)
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

// _GolfBuilding.Func_ada43: Wario walking in from the north (left) or
// the west (right); with all the crayons, at the door when he came out
// of it (phase 4), else walking in
void GolfBuilding::place_wario()
{
	auto walk_in = [this] { // .Func_ada4a
		static constexpr uint16_t DATA = ADDR__GolfBuilding + 0xbc; // .data
		uint16_t side = static_cast<uint16_t>(DATA + static_cast<uint8_t>(wram_.next_map_side() * 4));
		mem_[wOWUIObj1YCoord] = 0x58;
		mem_[wOWUIObj1XCoord] = mem_[side];
		mem_[PHASE] = mem_[side + 2];
		mem_[wOWUIObj2YCoord] = 0xff;
		wario_state(0);
	};
	if (wram_.crayon_flags() != ALL_CRAYONS) {
		walk_in();
		return;
	}
	if (wram_.ow_level() == LEVEL_GOLF_BUILDING) {
		mem_[wOWUIObj1YCoord] = 0x58;
		mem_[wOWUIObj1XCoord] = 0x50;
		mem_[PHASE] = 0x04;
		mem_[wOWUIObj2YCoord] = 0xff;
		wario_state(1);
		return;
	}
	walk_in();
	mem_[PHASE] += 2; // walking up to the door
}

// _GolfBuilding.FillBGPalettes: the building's palettes, each crayon
// colouring in its parts
void GolfBuilding::fill_bg_pals()
{
	struct Part {
		uint8_t offset, count; // in the palettes
	};
	static constexpr Part RED[] = {{7 * PAL_SIZE, PAL_SIZE}};
	static constexpr Part BROWN[] = {{0 * PAL_SIZE + 2, 2}, {1 * PAL_SIZE, 2}, {2 * PAL_SIZE, 6}, {3 * PAL_SIZE + 2, 2}};
	static constexpr Part YELLOW[] = {{4 * PAL_SIZE + 2, 6}};
	static constexpr Part GREEN[] = {{1 * PAL_SIZE + 2, 6}, {5 * PAL_SIZE + 2, 6}};
	static constexpr Part CYAN[] = {{0 * PAL_SIZE + 4, 2}, {3 * PAL_SIZE, 2}, {4 * PAL_SIZE, 2}, {5 * PAL_SIZE, 2}};
	static constexpr Part BLUE[] = {{0 * PAL_SIZE + 6, 2}, {2 * PAL_SIZE + 6, 2}, {3 * PAL_SIZE + 4, 4}};
	static constexpr Part PINK[] = {{6 * PAL_SIZE, PAL_SIZE}};
	struct Crayon {
		int bit;
		const Part *parts;
		size_t count;
	};
	static constexpr Crayon CRAYONS[] = {
		{CRAYON_RED_F, RED, 1}, {CRAYON_BROWN_F, BROWN, 4}, {CRAYON_YELLOW_F, YELLOW, 1},
		{CRAYON_GREEN_F, GREEN, 2}, {CRAYON_CYAN_F, CYAN, 4}, {CRAYON_BLUE_F, BLUE, 3},
		{CRAYON_PINK_F, PINK, 1},
	};

	SET_HL(gb_, ADDR_Pals_addc9);
	routines_.call(ADDR_LoadPalsToTempPals1);
	routines_.copy(wTempPals1, wTempBGPals, 8 * PAL_SIZE);
	for (const Crayon &crayon : CRAYONS) {
		if (!(wram_.crayon_flags() & (1 << crayon.bit)))
			continue;
		for (size_t i = 0; i < crayon.count; i++) {
			const Part &part = crayon.parts[i];
			routines_.copy(ADDR_Pals_ade09 + part.offset, wTempBGPals + part.offset, part.count);
		}
	}
	routines_.copy(wTempBGPals, wTempPals1, 8 * PAL_SIZE);
}

// Func_adbfe: a frame outside the building (B leaves, but in the door's
// phase)
void GolfBuilding::update()
{
	// .Func_adc0b
	if (mem_[PHASE] == 0x04 || !(wram_.joypad_pressed() & (1 << B_PAD_B)))
		step();
	else
		leave();
	routines_.call(ADDR_UpdateSceneWario);
	routines_.clear_unused_virtual_oam();
}

// Func_adbfe.exit: back to the map
void GolfBuilding::leave()
{
	mem_[hSFXID + 0] = 0xff; // stop_sfx
	mem_[hSFXID + 1] = 0x00;
	mem_[wOWUIObj2Duration] = 0x01;
	wram_.sub_state() = 0x08;
}

// Func_adbfe.Func_adc2f: the scene's phases (walking in from either side,
// up to the door; at the door: in, or off either side) and their steps
void GolfBuilding::step()
{
	Byte frames = mem_[FRAMES];
	++frames;
	auto next = [&] { // .asm_adc43
		frames = 0;
		++mem_[STEP];
	};
	auto phase = [&](uint8_t value) { // .asm_adc4a
		mem_[PHASE] = value;
		frames = 0;
		mem_[STEP] = 0;
	};
	uint8_t s = mem_[STEP];
	uint8_t p = mem_[PHASE];
	switch (p) {
	case 0: case 1: { // .Func_adc54 / .Func_adc7e: in from the left or
		// the right, then off back to the map
		bool right = p == 0;
		switch (s) {
		case 0: wario_state(right ? 0x08 : 0x07); next(); return;
		case 1:
			if (walk(right) != (right ? 0x90 : 0x10))
				return;
			wario_state(0);
			next();
			return;
		case 2:
			if (frames >= 0x14)
				next();
			return;
		case 3: leave(); return;
		default: return;
		}
	}
	case 2: case 3: { // .Func_adca9 / .Func_adcda: up to the door
		bool right = p == 2;
		switch (s) {
		case 0: wario_state(right ? 0x08 : 0x07); next(); return;
		case 1:
			if (walk(right) != 0x50)
				return;
			wario_state(right ? 0x0d : 0x0c);
			next();
			return;
		case 2:
			if (frames < 0x08)
				return;
			wario_state(right ? 0x34 : 0x35);
			phase(0x04);
			return;
		default: return;
		}
	}
	case 4: // .Func_add0b: at the door
		switch (s) {
		case 0: case 3: case 5: case 7: { // .Func_add21 and the like: once
			// Wario's in the state
			static constexpr uint8_t STATES[] = {0x01, 0, 0, 0x02, 0, 0x0c, 0, 0x0d};
			if (mem_[wOWUIObj1State] == STATES[s])
				next();
			return;
		}
		case 1: // .Func_add2a
			if (frames < 0x10)
				return;
			wario_state(0x3c);
			next();
			return;
		case 2: { // .Func_add36: in, or off to a side
			uint8_t pressed = wram_.joypad_pressed();
			if (pressed & (1 << B_PAD_RIGHT)) {
				wram_.next_map_side() = WEST;
				wram_.map_side_level_id() = OW_EXIT_LEFT;
				wram_.unk_2d068() = OW_EXIT_LEFT;
				wario_state(0x37);
				frames = 0;
				mem_[STEP] = 0x07;
			} else if (pressed & (1 << B_PAD_LEFT)) {
				wram_.next_map_side() = NORTH;
				wram_.map_side_level_id() = OW_EXIT_RIGHT;
				wram_.unk_2d068() = OW_EXIT_RIGHT;
				wario_state(0x36);
				frames = 0;
				mem_[STEP] = 0x05;
			} else if (pressed & (PAD_A | PAD_UP)) {
				wario_state(0x30);
				play_sfx(mem_, SFX_12A);
				next();
			}
			return;
		}
		case 4: // .Func_add93: into the building
			if (frames < 0x14)
				return;
			wram_.ow_level() = LEVEL_GOLF_BUILDING;
			wram_.sub_state() = 0;
			++wram_.state();
			return;
		case 6: // .Func_addae
			if (frames >= 0x04)
				phase(0x01);
			return;
		case 8: // .Func_addc0
			if (frames >= 0x04)
				phase(0x00);
			return;
		default:
			unrealistic();
			return;
		}
	default:
		unrealistic();
		return;
	}
}

} // namespace wl3::overworld

// Farcalled by the map's sub-state table (C++).
PORT_EX(_GolfBuilding, 0, ALL_REGISTERS) { wl3::overworld::GolfBuilding(gb).init(); }
PORT_EX(Func_adbfe, 0, ALL_REGISTERS) { wl3::overworld::GolfBuilding(gb).update(); }
