// The map scenes' objects at their start (bank2d.asm). See overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

constexpr uint8_t PLAY = 0x80; // w2d065: the event's playing
constexpr uint8_t DONE = 0x01; // and after it

} // namespace

void SceneInit::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

// InitSceneObjParams: the scene object's place and state, from the start
void SceneInit::place(uint16_t obj, Place place)
{
	mem_[obj] = place.y;
	mem_[obj + 1] = place.x;
	mem_[obj + 4] = 0; // duration
	mem_[obj + 5] = 0; // frameset offset
	mem_[obj + 6] = place.state;
}

// Func_b586d: a block of the map's tiles (Func_854ee's data)
void SceneInit::tiles(uint16_t data)
{
	SET_HL(gb_, data);
	routines_.farcall(BANK_Func_854ee, ADDR_Func_854ee);
}

// SetBGMapBytesPriority
void SceneInit::priority(uint16_t attrs, uint8_t count)
{
	do {
		mem_[attrs++] |= 1 << B_BG_PRIO;
	} while (--count);
}

// The decompress_tilemap and decompress_attrmap macros
void SceneInit::decompress(uint8_t bank, uint16_t src, uint16_t dest, bool vram1)
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

// decompress_tiles0: the scene's tiles, from tile
void SceneInit::gfx(uint8_t bank, uint16_t src, uint8_t tile)
{
	decompress(bank, src, static_cast<uint16_t>(v0Tiles0 + tile * TILE_SIZE), false);
}

// Whether the connection unlocking is this scene's
bool SceneInit::unlockable() const
{
	return wram_.temp_unlockable_connection_id() == wram_.ow_scene_action_param();
}

void SceneInit::run()
{
	uint8_t c = wram_.unk_2d065();
	uint8_t param = wram_.ow_scene_action_param();
	switch (wram_.ow_scene_map_side()) {
	case NORTH: north(param, c); return;
	case WEST: west(param, c); return;
	case SOUTH: south(param, c); return;
	case EAST: east(param, c); return;
	default: unrealistic(); return;
	}
}

// ---------------------------------------------------------------------------
// The north

void SceneInit::north(uint8_t param, uint8_t c)
{
	switch (param) {
	case 0: // Func_b4001: falls back into _InitOWObjects, forever
		unrealistic();
		return;
	case NOWFUNC_CUT_TREE: // Func_b403e
		if (c != DONE)
			place(wSceneObj8, {0x44, 0x30, 0x01});
		if (!wram_.cur_event()) { // .Func_b4055
			place(wSceneObj15, {0x56, 0x1c, 0x19});
			place(wSceneObj14, {0x36, 0x50, 0x1b});
			if (wram_.unk_2d011()) {
				mem_[wSceneObj15State] = 0x18;
				mem_[wSceneObj14State] = 0x1a;
			}
		}
		place(wSceneObj13, {0x84, 0x82, 0x00}); // Func_b4082
		return;
	case NOWFUNC_OPEN_GATE: // Func_b408e
		if (c != DONE)
			place(wSceneObj10, {0x40, 0x80, 0x03});
		return;
	case NOWFUNC_GARLIC: // Func_b409c
		gb_->cpu.a = GREEN_MUSIC_BOX;
		routines_.call(ADDR_IsTreasureCollected);
		if (!(gb_->cpu.f & FLAG_Z))
			return;
		if (!wram_.cur_event() && !wram_.next_map_side()
			&& wram_.map_side_level_id() == OWNORTH_SEA_TURTLE_ROCKS)
			return;
		place(wSceneObj3, {0x66, 0x74, 0x05});
		return;
	case NOWFUNC_MAGIC_SEED: // Func_b40c0
		if (!c)
			return;
		place(wSceneObj4, c == PLAY ? Place{0x00, 0x68, 0x07} : Place{0x4c, 0x66, 0x0b});
		return;
	case NOWFUNC_SUMMON_SNAKE: // Func_b40d9
		if (!c)
			return;
		if (c != DONE)
			place(wSceneObj1, {0x51, SCREEN_WIDTH_PX, 0x0f});
		place(wSceneObj2, {0x59, 0x2d, 0x1d});
		if (c != DONE)
			mem_[wSceneObj2State] = 0;
		return;
	case NOWFUNC_FALL_LEAVES: // Func_b40ff
		if (!c)
			return;
		if (c & 0x80) {
			place(wSceneObj6, {0x38, 0x15, 0x14});
			place(wSceneObj7, {0x41, 0x19, 0x14});
		} else {
			mem_[w2d12c] = 0x01;
			place(wSceneObj6, {0x48, 0x14, 0x11});
			mem_[w2d12e] = 0x04;
			place(wSceneObj7, {0x38, 0x0f, 0x10});
		}
		return;
	case NOWFUNC_BLUE_MUSIC_BOX: case NOWFUNC_GOLD_MUSIC_BOX: // Func_b4140
		return;
	case NOWFUNC_CRAYON_RED: case NOWFUNC_CRAYON_BROWN: case NOWFUNC_CRAYON_YELLOW:
	case NOWFUNC_CRAYON_GREEN: case NOWFUNC_CRAYON_CYAN: case NOWFUNC_CRAYON_BLUE:
	case NOWFUNC_CRAYON_PINK: { // Func_b4141 to Func_b4176, Func_b4143: the
		// crayon, its palette by colour, and its shadow
		static constexpr uint8_t PALETTES[] = {0, 0, 2, 3, 3, 2, 1};
		if (!(c & 0x80))
			return;
		place(wSceneObj5, {0x00, 0x70, 0x1f});
		mem_[wSceneObj5Attributes] = PALETTES[param - NOWFUNC_CRAYON_RED];
		place(wSceneObj1, {0x00, 0x70, 0x21});
		return;
	}
	case NOWFUNC_BOTTOM_BAR: // Func_b417a
		if (c == PLAY)
			bottom_bar();
		return;
	case NOWFUNC_DAY_NIGHT_SPELL: // Func_b4247
		if (c == PLAY)
			top_bar();
		return;
	case NOWFUNC_PROLOGUE: { // Func_b42e7: only in the epilogues
		uint8_t transition = wram_.transition_param();
		if (transition != TRANSITION_EPILOGUE_NOT_PERFECT && transition != TRANSITION_EPILOGUE_PERFECT)
			return;
		for (uint16_t i = 0; i < 15 * 8; i++)
			mem_[wSceneObj1 + i] = 0;
		place(wSceneObj1, {0x28, 0x50, 0x00});
		return;
	}
	default:
		unrealistic();
		return;
	}
}

// Func_b417a: the bottom bar's B button indicator and treasure slots,
// below the screen, and the bar's tiles
void SceneInit::bottom_bar()
{
	place(wSceneObj1, {0xa0, 0x08, 0x22});
	place(wSceneObj5, {0xa8, 0x18, 0x24});
	place(wSceneObj8, {0xa8, 0x34, 0x24});
	place(wSceneObj9, {0xa8, 0x50, 0x24});
	place(wSceneObj10, {0xa8, 0x6c, 0x24});
	for (uint16_t attributes : {wSceneObj5Attributes, wSceneObj8Attributes,
		wSceneObj9Attributes, wSceneObj10Attributes})
		mem_[attributes] |= 1 << 2;
	routines_.farcall(BANK_Func_80bd9, ADDR_Func_80bd9);
	routines_.farcall(BANK_VBlank_Overworld, ADDR_VBlank_Overworld);
	decompress(BANK_OverworldBottomBarTreasuresTilemap, ADDR_OverworldBottomBarTreasuresTilemap, v0BGMap1, false);
	decompress(BANK_OverworldBottomBarTreasuresAttrmap, ADDR_OverworldBottomBarTreasuresAttrmap, v1BGMap1, true);
	mem_[wWX] = 0;
	mem_[rWX] = 0;
	mem_[wWY] = SCREEN_HEIGHT_PX;
	mem_[rWY] = SCREEN_HEIGHT_PX;
	wram_.bottom_bar_action() = BOTBAR_CLOSED;
	// the empty treasure's tiles
	SET_HL(gb_, ADDR_TreasureGfx);
	SET_DE(gb_, v0Tiles1 + 0x10 * TILE_SIZE);
	gb_->cpu.c = 4 * TILE_SIZE;
	gb_->cpu.b = BANK_TreasureGfx;
	routines_.call(ADDR_CopyFarBytes);
}

// Func_b4247: the top bar's day and night button, below the screen
// (rows 19 to 22 of wAttrmap keep its tiles and attributes for the spell)
void SceneInit::top_bar()
{
	decompress(BANK_OverworldTopBarDayNightAttrmap, ADDR_OverworldTopBarDayNightAttrmap,
		v1BGMap0 + 29 * TILEMAP_WIDTH, true);
	decompress(BANK_OverworldTopBarDayNightTilemap, ADDR_OverworldTopBarDayNightTilemap,
		v0BGMap0 + 29 * TILEMAP_WIDTH, false);
	routines_.copy(v0BGMap0 + 30 * TILEMAP_WIDTH, wAttrmap + 21 * TILEMAP_WIDTH, 2 * TILEMAP_WIDTH);
	mem_[rVBK] = 1;
	routines_.copy(v0BGMap0 + 30 * TILEMAP_WIDTH, wAttrmap + 19 * TILEMAP_WIDTH, 2 * TILEMAP_WIDTH);
	mem_[rVBK] = 0;
	SET_HL(gb_, ADDR_Pals_84a20);
	SET_DE(gb_, wTempPals2 + 4 * PAL_SIZE);
	gb_->cpu.c = 4 * PAL_SIZE;
	gb_->cpu.b = BANK_Pals_84a20;
	routines_.call(ADDR_CopyFarBytes);
	place(wSceneObj5, {0x00, 0x60, 0x00});
	routines_.farcall(BANK_VBlank_Overworld, ADDR_VBlank_Overworld);
	routines_.farcall(BANK_DrawCoinCount, ADDR_DrawCoinCount);
	mem_[wSceneObj6State] = 0;
	mem_[wSceneObj7State] = 0;
	mem_[wSceneObj2State] = 0;
}

// ---------------------------------------------------------------------------
// The west

void SceneInit::west(uint8_t param, uint8_t c)
{
	static constexpr Place VOLCANO_SMOKE = {0x10, 0x54, 0x18}; // Func_b43dd.data_1
	static constexpr Place VOLCANO = {0x52, 0x5c, 0x00}; // Func_b43dd.data_6

	switch (param) {
	case 0: // Func_b4001
		unrealistic();
		return;
	case WOWFUNC_RAIN: // Func_b431f
		if (c == PLAY) {
			place(wSceneObj15, {0x56, 0x5c, 0x00});
			place(wSceneObj14, {0x3a, 0x5c, 0x03});
		} else if (c) {
			tiles(ADDR_Data_8550b);
		}
		return;
	case WOWFUNC_CLEAR_TORNADO: // Func_b4351
		if (c != DONE)
			place(wSceneObj1, {0x44, 0x28, 0x02});
		return;
	case WOWFUNC_ELEVATOR_WORKING: // Func_b435f
		place(wSceneObj8, {0x3b, 0x80, 0x09});
		if (!static_cast<uint8_t>(c * 2) || wram_.cur_event())
			return;
		place(wSceneObj7, {0x3b, 0x80, 0x00});
		mem_[wSceneObj8State] = wram_.unk_2d011() ? 0x08 : 0x06;
		return;
	case WOWFUNC_MAGIC_SEED: // Func_b438f
		if (!c)
			return;
		place(wSceneObj4, c == PLAY ? Place{0x00, 0x68, 0x0a} : Place{0x5b, 0x60, 0x0e});
		return;
	case WOWFUNC_SUMMON_SNAKE: // Func_b43a8
		if (!c)
			return;
		if (c != DONE)
			place(wSceneObj1, {0x32, SCREEN_WIDTH_PX, 0x12});
		place(wSceneObj5, {0x3a, 0x2e, 0x13});
		if (c != DONE) {
			mem_[wSceneObj5State] = 0;
			mem_[wAttrmap + 5 * TILEMAP_WIDTH + 16] &= ~(1 << B_BG_PRIO);
			mem_[wAttrmap + 6 * TILEMAP_WIDTH + 16] &= ~(1 << B_BG_PRIO);
			mem_[wAttrmap + 7 * TILEMAP_WIDTH + 0] &= ~(1 << B_BG_PRIO);
		}
		return;
	case WOWFUNC_EARTHQUAKE: // Func_b43dd
		if (!c)
			return;
		if (c == DONE) {
			tiles(ADDR_Data_85549);
			return;
		}
		place(wSceneObj11, VOLCANO_SMOKE);
		place(wSceneObj12, {0x3c, 0x4b, 0x00});
		place(wSceneObj13, {0x3c, 0x51, 0x00});
		place(wSceneObj14, {0x3c, 0x57, 0x00});
		place(wSceneObj15, {0x3c, 0x5d, 0x00});
		place(wSceneObj10, VOLCANO);
		routines_.copy(ADDR_Func_b43dd + 0x63, wTempPals2 + 2 * PAL_SIZE, PAL_SIZE); // .pal
		gfx(BANK_OverworldRocksGfx, ADDR_OverworldRocksGfx, 0x58); // LoadOverworldRocksGfx
		return;
	case WOWFUNC_VULCANO_ERUPTION: // Func_b4448
		if (!c)
			return;
		if ((c & 0x80) && unlockable()) {
			place(wSceneObj11, VOLCANO_SMOKE);
			place(wSceneObj10, VOLCANO);
			gfx(BANK_Overworld4Gfx, ADDR_Overworld4Gfx, 0x00); // LoadOverworld4Gfx
			return;
		}
		tiles(ADDR_Data_85536);
		return;
	case WOWFUNC_RED_MUSIC_BOX: // Func_b4474
		if (!c)
			return;
		gfx(BANK_Overworld3Gfx, ADDR_Overworld3Gfx, 0x00); // LoadOverworld3Gfx
		place(wSceneObj6, {0x78, 0x3c, 0x23});
		if (static_cast<uint8_t>(c * 2)) {
			tiles(ADDR_Data_8556e);
			return;
		}
		mem_[wSceneObj6State] = 0;
		++mem_[wSceneObj6YCoord];
		return;
	default:
		unrealistic();
		return;
	}
}

// ---------------------------------------------------------------------------
// The south

void SceneInit::south(uint8_t param, uint8_t c)
{
	static constexpr Place THUNDER_CLOUD = {0x68, 0x44, 0x0a}; // Func_b4520.data_3
	static constexpr Place FAN_STONE = {0x4d, 0x8a, 0x10}; // Func_b45c8.data_1
	static constexpr Place SNAKE_PLACE = {0x3f, 0x10, 0x1d}; // Func_b45f5.data_2

	switch (param) {
	case 0: // Func_b4001
		unrealistic();
		return;
	case SOWFUNC_SEND_SEEDS: // Func_b44b7
		if (c != PLAY || !unlockable())
			return;
		place(wSceneObj2, {0x38, 0x44, 0x00});
		place(wSceneObj3, {0x38, 0x54, 0x00});
		place(wSceneObj4, {0x38, 0x4c, 0x00});
		return;
	case SOWFUNC_MAGIC_SEED: // Func_b44e7
		if (!c)
			return;
		if (c == PLAY) {
			if (unlockable())
				place(wSceneObj4, {0x00, 0x16, 0x05});
			return;
		}
		place(wSceneObj4, {0x56, 0x1a, 0x01});
		return;
	case SOWFUNC_YELLOW_MUSIC_BOX: // Func_b4509
		place(wSceneObj6, {0x3d, 0x31, 0x07});
		if (c == DONE)
			mem_[wSceneObj6State] += 2;
		return;
	case SOWFUNC_CANYON_THUNDER: // Func_b4520
		if (c == PLAY) {
			place(wSceneObj2, {0x48, 0x38, 0x00});
			place(wSceneObj10, {0x3d, 0x2f, 0x00});
			place(wSceneObj5, THUNDER_CLOUD);
			mem_[wSceneObj5State] = 0;
			gfx(BANK_OverworldThunderGfx, ADDR_OverworldThunderGfx, 0x00); // LoadOverworldThunderGfx
		} else if (c == DONE) {
			tiles(ADDR_Data_855b2);
			place(wSceneObj5, THUNDER_CLOUD);
		}
		return;
	case SOWFUNC_RAISE_TOWER: // Func_b4567
		if (c == DONE) {
			tiles(ADDR_Data_85579);
			if (wram_.cur_event())
				return;
			place(wSceneObj15, {0x29, 0x78, 0x1f});
			if (wram_.unk_2d011())
				mem_[wSceneObj15State] = 0x1e;
			return;
		}
		if (!(c & 0x80))
			return;
		place(wSceneObj2, {0x49, 0x78, 0x00});
		place(wSceneObj3, {0x50, 0x78, 0x0c});
		for (int row = 9; row < 9 + 3; row++)
			priority(static_cast<uint16_t>(wAttrmap + row * TILEMAP_WIDTH + 14), 2);
		gfx(BANK_OverworldTowerGfx, ADDR_OverworldTowerGfx, 0x60); // LoadOverworldTowerGfx
		return;
	case SOWFUNC_FAN: // Func_b45c8
		if (c == DONE)
			return;
		if (c) {
			place(wSceneObj1, {0x5c, 0x82, 0x00});
			place(wSceneObj8, {0x64, 0x78, 0x19});
			gfx(BANK_OverworldFanGfx, ADDR_OverworldFanGfx, 0x60); // LoadOverworldFanGfx
		}
		place(wSceneObj9, FAN_STONE);
		return;
	case SOWFUNC_SUMMON_SNAKE: // Func_b45f5
		if (!c)
			return;
		if (c != DONE)
			place(wSceneObj1, {0x37, SCREEN_WIDTH_PX, 0x16});
		place(wSceneObj7, SNAKE_PLACE);
		if (c != DONE)
			mem_[wSceneObj7State] = 0;
		return;
	case SOWFUNC_EARTHQUAKE: // Func_b461b
	case SOWFUNC_GREEN_MUSIC_BOX: { // Func_b4637
		bool quake = param == SOWFUNC_EARTHQUAKE;
		if (!c)
			return;
		if (c & 0x80) {
			place(wSceneObj2, quake ? Place{0x5f, 0x3d, 0x00} : Place{0x20, 0x48, 0x00});
			gfx(BANK_OverworldThunderGfx, ADDR_OverworldThunderGfx, 0x00); // LoadOverworldThunderGfx
			return;
		}
		tiles(quake ? ADDR_Data_855f9 : ADDR_Data_8560e);
		return;
	}
	case SOWFUNC_EXPLOSIVES: // Func_b4653
		if (!(c & 0x80))
			return;
		place(wSceneObj2, {0x20, 0x44, 0x00});
		gfx(BANK_OverworldExplosion1Gfx, ADDR_OverworldExplosion1Gfx, 0x00); // LoadOverworldExplosion1Gfx
		return;
	case SOWFUNC_FULL_MOON: // Func_b4665
		if (!c)
			return;
		place(wSceneObj14, {0x25, 0x70, 0x20});
		if (c == PLAY || !wram_.unk_2d011())
			return;
		mem_[wSceneObj14XCoord] = 0x80;
		mem_[wSceneObj14YCoord] = 0x15;
		return;
	default:
		unrealistic();
		return;
	}
}

// ---------------------------------------------------------------------------
// The east

void SceneInit::east(uint8_t param, uint8_t c)
{
	static constexpr Place TOWER = {0x55, 0x28, 0x02}; // Func_b46d9.data_1
	static constexpr Place PIPE = {0x37, 0x7c, 0x1c}; // Func_b4832.data_1
	static constexpr Place VINES = {0x45, 0x87, 0x17}; // Func_b47f7.data_1

	switch (param) {
	case 0: // Func_b4001
		unrealistic();
		return;
	case EOWFUNC_FREEZE_SEA: // Func_b46a0
		if (!static_cast<uint8_t>(c * 2))
			routines_.copy(ADDR_Func_b4874 + 0xb, wTempPals1 + 4 * PAL_SIZE, PAL_SIZE); // Func_b4874: the sea's palette
		if (c == PLAY) {
			place(wSceneObj1, {0x64, 0xe8, 0x01});
		} else if (c == DONE) { // the frozen sea
			mem_[w2d03e] = 0;
			mem_[w2d046] = 0;
			SET_HL(gb_, ADDR_OWSceneFreezeSeaTilemap + 10 * TILEMAP_WIDTH + 15);
			SET_DE(gb_, wTilemap + 10 * TILEMAP_WIDTH + 15);
			gb_->cpu.c = 0x91;
			gb_->cpu.b = BANK_OWSceneFreezeSeaTilemap;
			routines_.call(ADDR_CopyFarBytes);
			SET_HL(gb_, ADDR_OWSceneFreezeSeaAttrmap + 15);
			SET_DE(gb_, wAttrmap + 10 * TILEMAP_WIDTH + 15);
			gb_->cpu.c = 0x91;
			gb_->cpu.b = BANK_OWSceneFreezeSeaAttrmap;
			routines_.call(ADDR_CopyFarBytes);
		}
		return;
	case EOWFUNC_EARTHQUAKE: // Func_b46d9
		if (!c) {
			place(wSceneObj6, TOWER);
		} else if (c == PLAY) {
			place(wSceneObj6, TOWER);
			place(wSceneObj15, {0x10, 0x5c, 0x07});
			priority(wAttrmap + 10 * TILEMAP_WIDTH + 3, 4);
			priority(wAttrmap + 11 * TILEMAP_WIDTH + 3, 4);
		} else {
			tiles(ADDR_Data_8563e);
		}
		return;
	case EOWFUNC_FORM_CASTLE: // Func_b4712
		if (!c)
			return;
		if (wram_.temp_ow_scene_action() != UNLOCK_LEVEL) { // .Func_b4757: the castle
			place(wSceneObj7, {0x28, 0x28, 0x12});
			if (!wram_.unk_2d011())
				mem_[wSceneObj7State] = 0x13;
		}
		if (c == PLAY) {
			place(wSceneObj1, {0x3f, 0x1c, 0x00});
			mem_[wSceneObj7State] = 0;
			gfx(BANK_OverworldExplosion2Gfx, ADDR_OverworldExplosion2Gfx, 0x00); // LoadOverworldExplosion2Gfx
			return;
		}
		for (uint16_t data : {ADDR_Data_85af4, ADDR_Data_85afd, ADDR_Data_85b0a,
			ADDR_Data_85b1b, ADDR_Data_85b2c, ADDR_Data_85b3d})
			tiles(data);
		return;
	case EOWFUNC_EXPLOSIVES: // Func_b4771
		if (!(c & 0x80))
			return;
		place(wSceneObj2, {0x54, 0x22, 0x00});
		place(wSceneObj1, {0x4c, 0x48, 0x00});
		gfx(BANK_OverworldExplosion3Gfx, ADDR_OverworldExplosion3Gfx, 0x00); // LoadOverworldExplosion3Gfx
		gfx(BANK_OverworldSmokeGfx, ADDR_OverworldSmokeGfx, 0x32); // LoadOverworldSmokeGfx
		return;
	case EOWFUNC_VULCANO_ERUPTION: // Func_b4792
		if (!c)
			return;
		if ((c & 0x80) && unlockable()) {
			place(wSceneObj10, {0x54, 0x78, 0x09});
			place(wSceneObj12, {0x58, 0x44, 0x0c});
			place(wSceneObj13, {0x44, 0x3c, 0x0c});
			place(wSceneObj14, {0x3d, 0x45, 0x0f});
			place(wSceneObj15, {0x3e, 0x74, 0x0f});
			place(wSceneObj11, {0x10, 0x5c, 0x1e});
			gfx(BANK_OverworldFireGfx, ADDR_OverworldFireGfx, 0x00); // LoadOverworldFireGfx
		}
		tiles(ADDR_Data_85b48);
		return;
	case EOWFUNC_BURN_VINES: // Func_b47f7
		if (c == DONE)
			return;
		place(wSceneObj9, VINES);
		if (c) {
			place(wSceneObj1, {0x45, 0x87, 0x00});
			gfx(BANK_OverworldTorchGfx, ADDR_OverworldTorchGfx, 0x40); // LoadOverworldTorchGfx
		}
		return;
	case EOWFUNC_SHOW_WARPED_VOID: // Func_b481b
		if (!c)
			return;
		place(wSceneObj5, {0x60, 0x88, 0x19});
		if (c & 0x80)
			mem_[wSceneObj5State] = 0;
		return;
	case EOWFUNC_RAISE_PIPE: // Func_b4832
		if (!c)
			return;
		place(wSceneObj8, PIPE);
		if (c == DONE)
			return;
		mem_[wSceneObj8YCoord] += 0x08;
		place(wSceneObj2, {0x40, 0x7c, 0x21});
		priority(wAttrmap + 7 * TILEMAP_WIDTH + 14, 3);
		priority(wAttrmap + 8 * TILEMAP_WIDTH + 14, 3);
		priority(wAttrmap + 9 * TILEMAP_WIDTH + 14, 2);
		return;
	case EOWFUNC_DAYTIME: // Func_b4873
		return;
	default:
		unrealistic();
		return;
	}
}

} // namespace wl3::overworld

// Farcalled by the scenes (C++), which read no registers after.
PORT_EX(_InitOWObjects, 0, ALL_REGISTERS) { wl3::overworld::SceneInit(gb).run(); }

// Only _InitOWObjects runs these (Func_b4000, the bank's first byte, is a
// lone ret).
PORT_COVERED(Func_b4000);
PORT_COVERED(Func_b4874);
PORT_COVERED(Func_b586d);
PORT_COVERED(InitSceneObjParams);
PORT_COVERED(SetBGMapBytesPriority);
PORT_COVERED(LoadOverworld3Gfx);
PORT_COVERED(LoadOverworld4Gfx);
PORT_COVERED(LoadOverworldRocksGfx);
PORT_COVERED(LoadOverworldTowerGfx);
PORT_COVERED(LoadOverworldFanGfx);
PORT_COVERED(LoadOverworldExplosion1Gfx);
PORT_COVERED(LoadOverworldThunderGfx);
PORT_COVERED(LoadOverworldExplosion2Gfx);
PORT_COVERED(LoadOverworldTorchGfx);
PORT_COVERED(LoadOverworldFireGfx);
PORT_COVERED(LoadOverworldExplosion3Gfx);
PORT_COVERED(LoadOverworldSmokeGfx);
PORT_COVERED(Func_b4001);
PORT_COVERED(InitOWObjects_North);
PORT_COVERED(InitOWObjects_West);
PORT_COVERED(InitOWObjects_South);
PORT_COVERED(InitOWObjects_East);
PORT_COVERED(Func_b403e);
PORT_COVERED(Func_b4082);
PORT_COVERED(Func_b408e);
PORT_COVERED(Func_b409c);
PORT_COVERED(Func_b40c0);
PORT_COVERED(Func_b40d9);
PORT_COVERED(Func_b40ff);
PORT_COVERED(Func_b4140);
PORT_COVERED(Func_b4141);
PORT_COVERED(Func_b4143);
PORT_COVERED(Func_b4162);
PORT_COVERED(Func_b4166);
PORT_COVERED(Func_b416a);
PORT_COVERED(Func_b416e);
PORT_COVERED(Func_b4172);
PORT_COVERED(Func_b4176);
PORT_COVERED(Func_b417a);
PORT_COVERED(Func_b4247);
PORT_COVERED(Func_b42e7);
PORT_COVERED(Func_b431f);
PORT_COVERED(Func_b4351);
PORT_COVERED(Func_b435f);
PORT_COVERED(Func_b438f);
PORT_COVERED(Func_b43a8);
PORT_COVERED(Func_b43dd);
PORT_COVERED(Func_b4448);
PORT_COVERED(Func_b4474);
PORT_COVERED(Func_b44b7);
PORT_COVERED(Func_b44e7);
PORT_COVERED(Func_b4509);
PORT_COVERED(Func_b4520);
PORT_COVERED(Func_b4567);
PORT_COVERED(Func_b45c8);
PORT_COVERED(Func_b45f5);
PORT_COVERED(Func_b461b);
PORT_COVERED(Func_b4637);
PORT_COVERED(Func_b4653);
PORT_COVERED(Func_b4665);
PORT_COVERED(Func_b46a0);
PORT_COVERED(Func_b46d9);
PORT_COVERED(Func_b4712);
PORT_COVERED(Func_b4771);
PORT_COVERED(Func_b4792);
PORT_COVERED(Func_b47f7);
PORT_COVERED(Func_b481b);
PORT_COVERED(Func_b4832);
PORT_COVERED(Func_b4873);
