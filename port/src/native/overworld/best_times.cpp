// The time attack's best time list (bank56.asm): every level's best time,
// on a screen that scrolls by itself or with up and down. See
// overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

constexpr uint8_t BOTTOM_SCY = 0x70;
constexpr uint8_t PAUSE_FRAMES = 0xb4;

// the digit tiles (two rows each: top, then bottom one tile on) and the colon
constexpr uint8_t DIGIT_TILES = 0xa0;
constexpr uint8_t COLON_TILE = 0xb6;
constexpr uint8_t TIME_WIDTH = 5;

constexpr uint16_t TIME_TILES = w2df00;
constexpr uint16_t TIME_POSITIONS = ADDR_PrintBestTimes + 0x63; // .bg_map_address_table

// _InitBestTimeList.VBlank_158a14's .func, copied to wVBlankFunc
constexpr uint16_t VBLANK_FUNC = ADDR__InitBestTimeList + 0xb4;
constexpr uint8_t VBLANK_FUNC_SIZE = 0x16;

constexpr uint16_t SCREEN_WRAM_SIZE = 0x800; // from wCollectionRow

} // namespace

void BestTimes::unrealistic()
{
	if (gb_->test.active)
		gb_->test.unrealistic = true;
}

// _InitBestTimeList
void BestTimes::init()
{
	routines_.disable_lcd();
	routines_.call(ADDR_FillBGMap0_With7f);
	routines_.clear_virtual_oam();
	play_music2(mem_, MUSIC_TREASURE_COLLECTION);
	mem_[rSCX] = 0;
	wram_.scx() = 0;
	// from the bottom, coming back from a level's (rra: the carry is clear)
	uint8_t scy = (wram_.obj1_sub_state() >> 1) ? BOTTOM_SCY : 0;
	mem_[rSCY] = scy;
	wram_.scy() = scy;
	mem_[rWBK] = 2;
	routines_.copy(VBLANK_FUNC, wVBlankFunc, VBLANK_FUNC_SIZE);
	gb_->cpu.a = 0;
	SET_HL(gb_, wCollectionRow);
	SET_BC(gb_, SCREEN_WRAM_SIZE);
	routines_.call(ADDR_WriteAToHL_BCTimes);
	routines_.load_pals_to_temp_pals1(ADDR_Pals_158b63);

	decompress(BANK_BestTimeListGfx, ADDR_BestTimeListGfx, v0Tiles1 + 0x20 * TILE_SIZE);
	mem_[rVBK] = 1;
	decompress(BANK_BestTimeListAttrmap, ADDR_BestTimeListAttrmap, v1BGMap0);
	mem_[rVBK] = 0;
	decompress(BANK_BestTimeListTilemap, ADDR_BestTimeListTilemap, v0BGMap0);

	print_times();
	mem_[rLCDC] = LCDC_BG_ON | LCDC_OBJ_ON | LCDC_ON;
	++wram_.sub_state();
}

// the decompress_* macros: through hCallFunc, with wTempBank
void BestTimes::decompress(uint8_t bank, uint16_t src, uint16_t dest)
{
	wram_.temp_bank() = bank;
	SET_HL(gb_, src);
	SET_BC(gb_, dest);
	routines_.farcall(bank, ADDR_Decompress);
}

// PrintBestTimes: each level's, as mm:ss
void BestTimes::print_times()
{
	Byte level = wram_.unk_2dfff();
	level = 0; // LEVEL_OUT_OF_THE_WOODS
	do {
		print_time(level);
		++level;
	} while (level <= NUM_LEVELS - 1);
}

// PrintBestTimes.PrintLevelTime: its five tiles to w2df00, then twice to
// the BG map (the digits' top and bottom halves)
void BestTimes::print_time(uint8_t level)
{
	uint16_t score = static_cast<uint16_t>(wLevelTimeAttackScores + static_cast<uint8_t>(level * 2));
	uint8_t minutes = mem_[score];
	uint8_t seconds = mem_[static_cast<uint16_t>(score + 1)];
	auto digit = [](uint8_t d) { return static_cast<uint8_t>(d * 2 + DIGIT_TILES); };
	uint8_t tiles[TIME_WIDTH] = {
		digit(minutes >> 4), digit(minutes & 0x0f), COLON_TILE, digit(seconds >> 4), digit(seconds & 0x0f)};
	for (uint16_t i = 0; i < TIME_WIDTH; i++)
		mem_[static_cast<uint16_t>(TIME_TILES + i)] = tiles[i];

	uint16_t dest = mem_.word(static_cast<uint16_t>(TIME_POSITIONS + static_cast<uint8_t>(level * 2)));
	for (uint8_t row = 0; row < 2; row++) {
		for (uint16_t i = 0; i < TIME_WIDTH; i++) {
			mem_[dest] = static_cast<uint8_t>(mem_[static_cast<uint16_t>(TIME_TILES + i)] + row);
			dest = static_cast<uint16_t>((dest & 0xff00) | static_cast<uint8_t>(dest + 1)); // inc e
		}
		dest = static_cast<uint16_t>(dest + TILEMAP_WIDTH - TIME_WIDTH);
	}
}

// _BestTimeList: scrolled with up and down, or after a while by itself
// (down, a pause, up, a pause) until a button
void BestTimes::update()
{
	if (wram_.collection_row()) {
		auto_scroll();
		return;
	}
	Byte scy = wram_.scy();
	Byte joypad = wram_.joypad_down();
	if (joypad.bit(B_PAD_UP)) {
		if (!scy)
			return;
		--scy;
		wram_.collection_col() = 0;
		return;
	}
	if (joypad.bit(B_PAD_DOWN)) {
		if (scy == BOTTOM_SCY)
			return;
		++scy;
		wram_.collection_col() = 0;
		return;
	}
	// 256 frames without input start the auto scroll
	Byte idle = wram_.collection_col();
	++idle;
	if (!idle) {
		++wram_.collection_row();
		wram_.colour_fade_diff() = 0;
	}
	if (wram_.joypad_pressed() & (PAD_B | PAD_START))
		wram_.sub_state() = 0x08;
}

void BestTimes::auto_scroll()
{
	if (wram_.joypad_pressed() & (PAD_A | PAD_B | PAD_START | PAD_UP | PAD_DOWN)) {
		wram_.collection_row() = 0; // .Func_158ac2
		return;
	}
	Byte scy = wram_.scy();
	Byte counter = wram_.collection_link_state_counter();
	switch (wram_.colour_fade_diff()) {
	case 0: // down
		if (scy < BOTTOM_SCY) {
			++scy;
			return;
		}
		break;
	case 1: // a pause
	case 3:
		++counter;
		if (counter < PAUSE_FRAMES)
			return;
		if (wram_.colour_fade_diff() == 3) {
			counter = 0;
			wram_.colour_fade_diff() = 0;
			return;
		}
		break;
	case 2: // up
		if (scy) {
			--scy;
			return;
		}
		break;
	default: // debug_nop
		unrealistic();
		return;
	}
	// .asm_158a96: the next phase
	counter = 0;
	++wram_.colour_fade_diff();
}

} // namespace wl3::overworld

// Only farcalled (best_times/common.asm), by the overworld's state table,
// which returns to the main loop; hCallFunc restores a and f.
PORT_EX(_InitBestTimeList, 0, ALL_REGISTERS)
{
	wl3::overworld::BestTimes(gb).init();
}

PORT_EX(_BestTimeList, 0, ALL_REGISTERS)
{
	wl3::overworld::BestTimes(gb).update();
}

PORT_COVERED(PrintBestTimes);
