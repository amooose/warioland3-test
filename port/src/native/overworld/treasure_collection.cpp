// The treasure collection (bank26.asm): pages of treasures, the ones found
// in their colours, a cursor, and links between treasures that go together.
// See overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

constexpr uint8_t COLLECTION_BANK = 0x26; // the scene objects' framesets and OAM data
constexpr uint16_t VBLANK_FUNC = ADDR_VBlank_TreasureCollection + 0x0c; // .Func
constexpr uint8_t VBLANK_FUNC_SIZE = 0x16;
// the cells' positions (tables in the routines' own code)
constexpr uint16_t CELL_Y = ADDR_GetCollectionCellCoords + 0x1b;
constexpr uint16_t CELL_X = ADDR_GetCollectionCellCoords + 0x1f;
constexpr uint16_t LINK_ROW_Y = ADDR_Func_9a78e + 0x14;
constexpr uint16_t LINK_COL_X = ADDR_Func_9a78e + 0x18;
// the low bytes of the cells' places in wTilemap
constexpr uint16_t CELL_MAP_LOW = ADDR_ApplyPageTreasuresPals + 0x7e;
constexpr uint8_t NO_LINK = 0x80;

static_assert(wSceneObj1State == wSceneObj1 + 6);
static_assert(wCollectionCol == wCollectionRow + 1);

// a scene object's state, by number (1 to 10)
uint16_t scene_obj_state(int n)
{
	static constexpr uint16_t STATES[10] = {
		wSceneObj1State, wSceneObj2State, wSceneObj3State, wSceneObj4State, wSceneObj5State,
		wSceneObj6State, wSceneObj7State, wSceneObj8State, wSceneObj9State, wSceneObj10State,
	};
	return STATES[n - 1];
}

} // namespace

TreasureCollection::TreasureCollection(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

// InitTreasureCollection: farcall _InitTreasureCollection
void TreasureCollection::init_far()
{
	routines_.in_bank(BANK__InitTreasureCollection, ADDR__InitTreasureCollection, [&] { init(); });
}

// TreasureCollection: farcall _TreasureCollection
void TreasureCollection::update_far()
{
	routines_.in_bank(BANK__TreasureCollection, ADDR__TreasureCollection, [&] { update(); });
}

void TreasureCollection::set_scene_obj_state(uint16_t state, uint8_t value)
{
	SET_HL(gb_, state);
	gb_->cpu.a = value;
	routines_.call(ADDR_SetSceneObjState);
}

void TreasureCollection::decompress(uint8_t bank, uint16_t src, uint16_t dest)
{
	wram_.temp_bank() = bank;
	SET_HL(gb_, src);
	SET_BC(gb_, dest);
	routines_.farcall(bank, ADDR_Decompress);
}

void TreasureCollection::init()
{
	routines_.disable_lcd();
	routines_.call(ADDR_FillBGMap0_With7f);
	routines_.clear_virtual_oam();
	play_music2(mem_, MUSIC_TREASURE_COLLECTION);
	mem_[rSCX] = 0;
	wram_.scx() = 0;
	mem_[rSCY] = 0;
	wram_.scy() = 0;
	mem_[rWBK] = 2;
	routines_.copy(VBLANK_FUNC, wVBlankFunc, VBLANK_FUNC_SIZE);
	clear(wCollectionRow, 0x800);
	clear(wCurSceneObj, 0x200);
	wram_.hdma() = 0;

	// the first time, the page and cell of the treasure just found
	if (!wram_.is_collection_open()) {
		uint8_t page = 0, cell = 0;
		uint8_t treasure = wram_.transition_param();
		if (treasure && treasure < NUM_TREASURES) {
			while (treasure >= NUM_COLLECTION_CELLS + 1) {
				++page;
				treasure = static_cast<uint8_t>(treasure - NUM_COLLECTION_CELLS);
			}
			cell = static_cast<uint8_t>(treasure - 1);
		}
		wram_.collection_page() = page;
		wram_.collection_cell() = cell;
		wram_.is_collection_open() = TRUE;
	}
	set_row_and_column(wCollectionRow, wram_.collection_cell());
	move_cursor();

	routines_.load_pals_to_temp_pals1(ADDR_Pals_9ad45);
	routines_.load_pals_to_temp_pals2(ADDR_Pals_9ad05);
	decompress(BANK_TreasureCollectionCursorGfx, ADDR_TreasureCollectionCursorGfx, v0Tiles0);
	decompress(BANK_TreasureCollectionGfx, ADDR_TreasureCollectionGfx, v0Tiles2);
	load_page_gfx();
	mem_[rVBK] = 1;
	routines_.copy_bc(ADDR_NumbersGfx, v1Tiles1 + 0x20 * TILE_SIZE, 0x14 * TILE_SIZE);
	mem_[rVBK] = 0;
	decompress(BANK_TreasureCollectionAttrmap, ADDR_TreasureCollectionAttrmap, wAttrmap);
	decompress(BANK_TreasureCollectionTilemap, ADDR_TreasureCollectionTilemap, wTilemap);
	apply_page_pals();
	// the page number
	for (int i = 0; i < 3; i++) {
		mem_[static_cast<uint16_t>(wAttrmap + 1 * TILEMAP_WIDTH + 2 + i)] = 0x1 | BG_BANK1;
		mem_[static_cast<uint16_t>(wAttrmap + 2 * TILEMAP_WIDTH + 2 + i)] = 0x1 | BG_BANK1;
	}
	mem_[wTilemap + 1 * TILEMAP_WIDTH + 16] = static_cast<uint8_t>(wram_.collection_page() + 1 + 0x20);
	routines_.copy_bc(wTilemap, v0BGMap1, 18 * TILEMAP_WIDTH);
	mem_[rVBK] = 1;
	routines_.copy_bc(wAttrmap, v0BGMap1, 18 * TILEMAP_WIDTH);
	mem_[rVBK] = 0;
	SET_HL(gb_, v0BGMap1 + 1 * TILEMAP_WIDTH + 2);
	routines_.farcall(BANK_PrintNumCollectedTreasures, ADDR_PrintNumCollectedTreasures);

	set_scene_obj_state(wSceneObj1State, 0x01); // the cursor
	// the page arrows
	mem_[wSceneObj9XCoord] = 0x08;
	mem_[wSceneObj9XCoord - 1] = 0x10;
	mem_[wSceneObj10XCoord] = 0x98;
	mem_[wSceneObj10XCoord - 1] = 0x10;
	mem_[rLCDC] = LCDC_ON | LCDC_BG_9C00 | LCDC_OBJ_16 | LCDC_OBJ_ON | LCDC_BG_ON;
	++wram_.sub_state();
}

void TreasureCollection::clear(uint16_t addr, uint16_t size)
{
	SET_HL(gb_, addr);
	SET_BC(gb_, size);
	gb_->cpu.a = 0;
	routines_.call(ADDR_WriteAToHL_BCTimes);
}

// the page's treasures, and the 101st's
void TreasureCollection::load_page_gfx()
{
	constexpr uint16_t PAGE_SIZE = NUM_TREASURE_TILES * NUM_COLLECTION_CELLS * TILE_SIZE;
	uint16_t gfx = static_cast<uint16_t>(ADDR_TreasureGfx + 4 * TILE_SIZE + wram_.collection_page() * PAGE_SIZE);
	routines_.copy_bc(gfx, v0Tiles1 + 0x20 * TILE_SIZE, PAGE_SIZE);
	routines_.copy(ADDR_Treasure101Gfx, v0Tiles1 + 0x7c * TILE_SIZE, NUM_TREASURE_TILES * TILE_SIZE);
}

// The page's treasures: the ones found in their palettes, the others
// hidden.
void TreasureCollection::apply_page_pals()
{
	Byte cell = mem_[w2dffe];
	Byte treasure = mem_[w2dfff];
	cell = 0;
	treasure = static_cast<uint8_t>(wram_.collection_page() * NUM_COLLECTION_CELLS + 1);
	do {
		// the cell's top-left tile in wTilemap, and the attribute rows
		uint8_t high = static_cast<uint8_t>((wTilemap >> 8) + (cell >= NUM_COLLECTION_COLS ? 1 : 0));
		uint8_t low = mem_[static_cast<uint16_t>(CELL_MAP_LOW + cell)];
		uint16_t top = static_cast<uint16_t>(high << 8 | low);
		uint16_t bottom = static_cast<uint16_t>(high << 8 | static_cast<uint8_t>(low + TILEMAP_WIDTH));
		uint16_t attr_top = static_cast<uint16_t>(top - 0x300); // wAttrmap
		uint16_t attr_bottom = static_cast<uint16_t>(bottom - 0x300);
		auto next_in_page = [](uint16_t addr) {
			return static_cast<uint16_t>((addr & 0xff00) | static_cast<uint8_t>(addr + 1));
		};
		if (!routines_treasure_collected(treasure)) {
			mem_[top] = 0xfc;
			mem_[bottom] = 0xfd;
			mem_[next_in_page(top)] = 0xfe;
			mem_[static_cast<uint16_t>(bottom + 1)] = 0xff;
			mem_[attr_top] = 0x04;
			mem_[static_cast<uint16_t>(attr_top + 1)] = 0x04;
			mem_[static_cast<uint16_t>(attr_top + 1 + TILEMAP_WIDTH)] = 0x04;
			mem_[static_cast<uint16_t>(attr_top + TILEMAP_WIDTH)] = 0x04;
		} else {
			uint8_t pal = mem_[static_cast<uint16_t>(ADDR_TreasureOBPals + treasure)];
			mem_[attr_bottom] = pal;
			mem_[static_cast<uint16_t>(attr_bottom + 1)] = pal;
			mem_[attr_top] = pal;
			mem_[static_cast<uint16_t>(attr_top + 1)] = pal;
		}
		++cell;
		++treasure;
	} while (cell != NUM_COLLECTION_CELLS);
}

bool TreasureCollection::routines_treasure_collected(uint8_t treasure)
{
	gb_->cpu.a = treasure;
	routines_.call(ADDR_IsTreasureCollected);
	return !(gb_->cpu.f & FLAG_Z);
}

// a cell's row and column, at dest and dest + 1 (stepping in its page)
void TreasureCollection::set_row_and_column(uint16_t dest, uint8_t cell)
{
	uint8_t row = 0;
	while (cell >= NUM_COLLECTION_COLS) {
		cell = static_cast<uint8_t>(cell - NUM_COLLECTION_COLS);
		++row;
	}
	mem_[dest] = row;
	mem_[static_cast<uint16_t>((dest & 0xff00) | static_cast<uint8_t>(dest + 1))] = cell;
}

// GetCollectionCellCoords: the cursor (scene object 1) on the cell
void TreasureCollection::move_cursor()
{
	mem_[wSceneObj1YCoord] = mem_[static_cast<uint16_t>(CELL_Y + wram_.collection_row())];
	mem_[wSceneObj1XCoord] = mem_[static_cast<uint16_t>(CELL_X + wram_.collection_col())];
}

void TreasureCollection::update()
{
	routines_.call(ADDR_ProcessDPadRepeat);
	handle_input();
	animate();
	routines_.clear_unused_virtual_oam();
}

// B closes; A held with left or right turns the page; the d-pad moves
// the cursor, across pages at the sides.
void TreasureCollection::handle_input()
{
	if (wram_.joypad_pressed().bit(B_PAD_B)) {
		stop_music2(mem_);
		mem_[hSFXID + 0] = 0xff; // stop_sfx
		mem_[hSFXID + 1] = 0x00;
		wram_.sub_state() = 0x08;
		return;
	}
	Byte page = wram_.collection_page();
	Byte held = wram_.joypad_down();
	if (held.bit(B_PAD_A)) {
		if (!wram_.collection_scroll_mode()) {
			wram_.collection_scroll_mode() = TRUE;
			set_scene_obj_state(wSceneObj9State, 0x03);
			set_scene_obj_state(wSceneObj10State, 0x04);
		}
		if (held.bit(B_PAD_RIGHT))
			page = page == 0x04 ? 0 : static_cast<uint8_t>(page + 1);
		else if (held.bit(B_PAD_LEFT))
			page = page ? static_cast<uint8_t>(page - 1) : 0x04;
		else
			return;
		play_sfx(mem_, SFX_0EB);
		stop_music2(mem_);
		wram_.sub_state() = 0x0c;
		return;
	}

	wram_.collection_scroll_mode() = 0;
	wram_.scene_obj9_state() = 0;
	wram_.scene_obj10_state() = 0;
	Byte pressed = wram_.joypad_pressed();
	Byte col = wram_.collection_col();
	Byte row = wram_.collection_row();
	uint8_t cell;
	if (pressed.bit(B_PAD_RIGHT)) {
		if (col == NUM_COLLECTION_COLS - 1) { // the next page
			col = 0;
			wram_.collection_cell() = static_cast<uint8_t>(wram_.collection_cell() - NUM_COLLECTION_ROWS);
			if (++page == NUM_COLLECTION_PAGES)
				page = 0;
			change_page();
			return;
		}
		++col;
		cell = static_cast<uint8_t>(wram_.collection_cell() + 1);
	} else if (pressed.bit(B_PAD_LEFT)) {
		if (!col) { // the page before
			col = NUM_COLLECTION_ROWS;
			wram_.collection_cell() = static_cast<uint8_t>(wram_.collection_cell() + NUM_COLLECTION_ROWS);
			if (--page == 0xff)
				page = NUM_COLLECTION_PAGES - 1;
			change_page();
			return;
		}
		--col;
		cell = static_cast<uint8_t>(wram_.collection_cell() - 1);
	} else {
		bool up = pressed.bit(B_PAD_UP);
		if (!up && !pressed.bit(B_PAD_DOWN)) {
			process_link_state();
			return;
		}
		if (up ? !row : row >= NUM_COLLECTION_ROWS - 1) {
			play_sfx(mem_, SFX_0E5); // can't scroll
			return;
		}
		if (up) {
			--row;
			cell = static_cast<uint8_t>(wram_.collection_cell() - NUM_COLLECTION_COLS);
		} else {
			++row;
			cell = static_cast<uint8_t>(wram_.collection_cell() + NUM_COLLECTION_COLS);
		}
	}
	wram_.collection_cell() = cell;
	play_sfx(mem_, SFX_0E2);
	for (int n = 2; n <= 8; n++)
		mem_[scene_obj_state(n)] = 0;
	wram_.collection_link_state() = 0;
	mem_[wCollectionLinkState + 1] = 0;
	move_cursor();
}

void TreasureCollection::change_page()
{
	play_sfx(mem_, SFX_0EB);
	wram_.sub_state() = 0x0c;
}

// The treasures linked to the current one get a marker, after a delay.
void TreasureCollection::process_link_state()
{
	++wram_.collection_link_state_counter();
	switch (wram_.collection_link_state()) {
	case 0:
		advance_link_state();
		break;
	case 1: // wait
		if (wram_.collection_link_state_counter() >= 0x14)
			advance_link_state();
		break;
	case 2: { // the linked cells
		SET_HL(gb_, ADDR_CollectionLinkTable);
		gb_->cpu.a = wram_.collection_page();
		routines_.call(ADDR_GetPointerFromTableHL);
		gb_->cpu.a = wram_.collection_cell();
		routines_.call(ADDR_GetPointerFromTableHL);
		uint16_t links = REG_HL(gb_);
		if (mem_[links] == NO_LINK) {
			advance_link_state();
			break;
		}
		routines_.copy(links, wCollectionLinkedCells, 7);
		advance_link_state();
		advance_link_state();
		break;
	}
	case 4:
		place_link_markers();
		break;
	case 3:
	case 5:
		break;
	default: // past the table
		unrealistic();
		break;
	}
}

void TreasureCollection::advance_link_state()
{
	wram_.collection_link_state_counter() = 0;
	++wram_.collection_link_state();
}

// scene objects 2 to 8 on the linked cells, other than the cursor's
void TreasureCollection::place_link_markers()
{
	static constexpr uint16_t MARKERS[7] = {
		wSceneObj2YCoord, wSceneObj3YCoord, wSceneObj4YCoord, wSceneObj5YCoord,
		wSceneObj6YCoord, wSceneObj7YCoord, wSceneObj8YCoord,
	};
	for (int i = 0; i < 7; i++) {
		uint8_t linked = mem_[static_cast<uint16_t>(wCollectionLinkedCells + i)];
		if (linked == NO_LINK)
			break;
		uint8_t cell = static_cast<uint8_t>(linked - 1);
		if (cell == wram_.collection_cell())
			continue;
		uint16_t marker = MARKERS[i];
		set_row_and_column(marker, cell);
		set_scene_obj_state(static_cast<uint16_t>(marker + 6), 0x02);
		// row and column to y and x
		Byte y = mem_[marker];
		Byte x = mem_[static_cast<uint16_t>((marker & 0xff00) | static_cast<uint8_t>(marker + 1))];
		y = mem_[static_cast<uint16_t>(LINK_ROW_Y + y)];
		x = mem_[static_cast<uint16_t>(LINK_COL_X + x)];
	}
	advance_link_state();
}

// The scene objects: the cursor, the link markers, the page arrows.
void TreasureCollection::animate()
{
	static constexpr uint16_t FRAMESETS[4] = {
		ADDR_Frameset_9ace1, ADDR_Frameset_9acea, ADDR_Frameset_9acf3, ADDR_Frameset_9acfc,
	};
	for (int n = 1; n <= 10; n++) {
		uint16_t state = scene_obj_state(n);
		uint8_t s = mem_[state];
		if (!s)
			continue;
		if (s > 4) {
			unrealistic();
			continue;
		}
		// Func_9a9ec, then Func_9aa59
		SET_BC(gb_, state);
		gb_->cpu.h = gb_->cpu.b;
		gb_->cpu.l = static_cast<uint8_t>(gb_->cpu.c & ~(1 << 1));
		gb_->cpu.b = COLLECTION_BANK;
		SET_DE(gb_, FRAMESETS[s - 1]);
		routines_.call(ADDR_UpdateOWAnimation);
		gb_->cpu.a = mem_[REG_BC(gb_)];
		SET_DE(gb_, ADDR_OAM_9ac5d);
		gb_->cpu.b = COLLECTION_BANK;
		routines_.call(ADDR_LoadOWSpriteWithScroll);
	}
}

void TreasureCollection::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

} // namespace wl3::overworld

// The routines implemented inside this module (they have no hooks).
PORT_COVERED(InitTreasureCollection);
PORT_COVERED(TreasureCollection);
PORT_COVERED(_InitTreasureCollection);
PORT_COVERED(_TreasureCollection);
PORT_COVERED(LoadCollectionPageTreasureGfx);
PORT_COVERED(Func_9a54e);
PORT_COVERED(Func_9a559);
PORT_COVERED(VBlank_TreasureCollection);
PORT_COVERED(ApplyPageTreasuresPals);
PORT_COVERED(HandleTreasureCollectionInput);
PORT_COVERED(GetCollectionCellCoords);
PORT_COVERED(Func_9a78e);
PORT_COVERED(GetCurrentCollectionCellRowAndColumn);
PORT_COVERED(GetCollectionCellRowAndColumn);
PORT_COVERED(ProcessCollectionLinkState);
PORT_COVERED(AnimateTreasureCollection);
PORT_COVERED(Func_9a9ec);
PORT_COVERED(Func_9a9f7);
PORT_COVERED(Func_9a9fd);
PORT_COVERED(Func_9aa56);
PORT_COVERED(Func_9aa59);
PORT_COVERED(Func_9aa60);
PORT_COVERED(Func_9aa6c);
PORT_COVERED(Func_9aa6d);
PORT_COVERED(Func_9aa72);
PORT_COVERED(Func_9aa77);
PORT_COVERED(Func_9aa7c);
