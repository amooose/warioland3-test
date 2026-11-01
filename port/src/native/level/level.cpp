// The level state (src/engine/level: common.asm, state_table.asm, init.asm,
// key_delay.asm, tick_time.asm, switch.asm, vblank.asm): the level's state
// table and its start, the key collection pause, the Time Attack clock,
// the switchable blocks and the V-Blank code that copies the tile queue.
//
// Most of these are called from C that still reads registers after them,
// so each entry point leaves the registers and flags the original does.
#include "../flags.hpp"
#include "../native.hpp"
#include "../ram.hpp"
#include "../routines.hpp"

namespace wl3::level {

namespace {

constexpr uint8_t SRAM0 = BANK_s0a000;

// the sramswitch macro
void sram_switch(Memory mem, uint8_t bank)
{
	mem[wSRAMBank] = bank;
	mem[rRAMB + 0x100] = bank;
}

class LevelState {
public:
	explicit LevelState(GB *gb) : gb_(gb), mem_(gb), wram_(gb), routines_(gb) {}

	void run_state(); // _LevelStateTable
	void init(); // InitLevel
	void load_common_gfx(); // LoadLevelCommonGfxAndTreasures
	void collect_key_delay(); // CollectKeyDelay
	void tick_time(); // TickLevelTime
	void update_switchable_blocks(); // UpdateSwitchableBlocks
	void set_tile_queue_functions(); // SetTileQueueProcessingFunctions

private:
	void unrealistic()
	{
		if (gb_->test.active)
			gb_->test.unrealistic = true;
	}
	bool tick(uint16_t addr);
	void switch_blocks(uint8_t bank, uint8_t end_h, bool on);
	uint16_t write_tile_copies(uint16_t at, uint8_t count);

	GB *gb_;
	Memory mem_;
	Wram wram_;
	Routines routines_;
};

void LevelState::run_state()
{
	switch (wram_.sub_state()) {
	case 0: routines_.call(ADDR_FastFadeToWhite); break;
	case 1: init(); break;
	case 2: routines_.call(ADDR_SlowFadeFromWhite); break;
	case 3: routines_.call(ADDR_UpdateLevel); break;
	case 4: routines_.call(ADDR_DoorTransition); break;
	case 5: routines_.call(ADDR_FastFadeToWhite); break;
	case 6: routines_.call(ADDR_StartRoom_FromTransition); break;
	case SST_LEVEL_MULTI_BLOCK: routines_.call(ADDR_ProcessMultiBlock); break;
	case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15:
		routines_.call(ADDR_DebugReset);
		break;
	default: unrealistic(); break;
	}
}

void LevelState::init()
{
	routines_.disable_lcd();
	routines_.call(ADDR_FillBGMap0_With7f);
	routines_.clear_virtual_oam();
	routines_.call(ADDR_ClearParticles);

	mem_[rSCY] = 0;
	mem_[wSCY] = 0;
	mem_[rSCX] = 0;
	mem_[wSCX] = 0;

	routines_.in_bank(BANK_LoadLevelCommonGfxAndTreasures, ADDR_LoadLevelCommonGfxAndTreasures, [&] { load_common_gfx(); });

	routines_.call(ADDR_StartRoom_FromLevelStart);
	routines_.call(ADDR_UpdateLevelMusic);

	mem_[wRoomAnimatedTilesEnabled] = TRUE;
	mem_[wceef] = 0;

	uint8_t sram_bank = mem_[wSRAMBank];
	sram_switch(mem_, SRAM0);
	for (int i = 0; i < 4; i++) {
		mem_[s0a000 + i] = 0;
		mem_[s0a800 + i] = 0;
	}
	sram_switch(mem_, sram_bank);

	++wram_.sub_state();
	if (wram_.level() != THE_TEMPLE) {
		mem_[rLCDC] = LCDC_DEFAULT;
		return;
	}
	// the final battle: the window shows Hidden Figure
	routines_.farcall(BANK_LoadHiddenFigureGfx, ADDR_LoadHiddenFigureGfx);
	mem_[wc08d] = 0x88;
	mem_[rWY] = 0x88;
	mem_[rWX] = 0 + WX_OFS;
	mem_[rLCDC] = LCDC_ON | LCDC_WIN_9C00 | LCDC_WIN_ON | LCDC_OBJ_16 | LCDC_OBJ_ON | LCDC_BG_ON;
}

// the levels' common tiles to v0Tiles1, and the treasures
void LevelState::load_common_gfx()
{
	SET_HL(gb_, ADDR_LevelsCommonGfx);
	SET_DE(gb_, v0Tiles1);
	SET_BC(gb_, ADDR_LevelsCommonGfxEnd - ADDR_LevelsCommonGfx);
	routines_.call(ADDR_CopyHLToDE_BC);
	routines_.farcall(BANK_LoadLevelTreasureData, ADDR_LoadLevelTreasureData);
}

// the game stays paused for 100 ticks
void LevelState::collect_key_delay()
{
	uint8_t t = ++mem_[wTimer];
	SET_HL(gb_, wTimer);
	gb_->cpu.f = flags::cp(t, 100);
	gb_->cpu.a = t;
	if (t < 100)
		return;
	mem_[wTimer] = 0;
	wram_.state() = ST_LEVEL;
	gb_->cpu.a = ST_LEVEL;
}

// add 1 / daa on a BCD pair, then cp $60: true when it reached a minute
// (the asm's carry clear)
bool LevelState::tick(uint16_t addr)
{
	uint8_t old = mem_[addr];
	uint8_t a = static_cast<uint8_t>(old + 1);
	// daa after an add: the half carry and carry of old + 1
	bool half = (old & 0x0f) == 0x0f, carry = old == 0xff;
	if (carry || a > 0x99) {
		a = static_cast<uint8_t>(a + 0x60);
		carry = true;
	}
	if (half || (a & 0x0f) > 0x09)
		a = static_cast<uint8_t>(a + 0x06);
	mem_[addr] = a;
	gb_->cpu.a = a;
	gb_->cpu.f = flags::cp(a, 0x60);
	return a >= 0x60;
}

// in Time Attack, a frame onto wLevelTime (minutes, seconds, frames, in
// BCD), stopping at 59:59
void LevelState::tick_time()
{
	uint8_t mode = mem_[wGameModeFlags];
	gb_->cpu.a = mode;
	gb_->cpu.f = flags::bit(gb_->cpu.f, mode, MODE_TIME_ATTACK_F);
	if (!(mode & (1 << MODE_TIME_ATTACK_F)))
		return;
	uint8_t end = wram_.level_end_screen();
	gb_->cpu.a = end;
	gb_->cpu.f = flags::and_(end);
	if (end)
		return;
	for (int i = 2; i > 0; i--) {
		if (!tick(wLevelTime + i))
			return;
		mem_[wLevelTime + i] = 0;
	}
	if (!tick(wLevelTime + 0))
		return;
	mem_[wLevelTime + 0] = 0x59;
	mem_[wLevelTime + 1] = 0x59;
	gb_->cpu.a = 0x59;
}

// adds (off) or takes (on) 8 from every switchable block in an SRAM bank
// of the level map, from $a000 up to end_h << 8
void LevelState::switch_blocks(uint8_t bank, uint8_t end_h, bool on)
{
	uint8_t sram_bank = mem_[wSRAMBank];
	sram_switch(mem_, bank);
	uint8_t lo = on ? 0x68 : 0x60;
	for (uint8_t h = 0xa0; h != end_h; h++) {
		for (uint8_t l = 0; l < LEVEL_WIDTH; l++) {
			Byte block = mem_[static_cast<uint16_t>(h << 8 | l)];
			uint8_t id = block & 0x7f;
			if (id >= lo && id < lo + 8)
				block = static_cast<uint8_t>(on ? block - 0x08 : block + 0x08);
		}
	}
	sram_switch(mem_, sram_bank);
}

// with wSwitchStateUpdated set, the switchable blocks turn on or off
void LevelState::update_switchable_blocks()
{
	uint8_t updated = mem_[wSwitchStateUpdated];
	if (!updated) {
		gb_->cpu.a = 0;
		gb_->cpu.f = flags::and_(0);
		return;
	}
	mem_[wSwitchStateUpdated] = 0;
	uint8_t state = mem_[wSwitchState];
	bool on = state == 0x68;
	switch_blocks(BANK_sLevelBlockObjectMap, 0xc0, on); // up to HIGH(STARTOF(SRAM) + SIZEOF(SRAM))
	switch_blocks(BANK_s2a000, 0xb0, on);
	// pop af: the SRAM bank, and cp $68's flags
	gb_->cpu.a = mem_[wSRAMBank];
	gb_->cpu.f = flags::cp(state, 0x68);
	SET_HL(gb_, 0xb000);
}

// count copies of ld a, [hli] / ld d, a / ld a, [hli] / ld e, a /
// ld a, [bc] / ld [de], a / inc c, which copy the queued tiles
uint16_t LevelState::write_tile_copies(uint16_t at, uint8_t count)
{
	static constexpr uint8_t code[] = {0x2a, 0x57, 0x2a, 0x5f, 0x0a, 0x12, 0x0c};
	do {
		for (uint8_t byte : code)
			mem_[at++] = byte;
	} while (--count);
	return at;
}

// builds wVBlankFunc's code that copies the BG map tile queue to VRAM (both
// VRAM banks), ending in jp ClearLevelTileQueue
void LevelState::set_tile_queue_functions()
{
	uint16_t at = wVBlankFunc + 0x10;
	uint8_t size = mem_[wBGMapTileQueueSize];
	if (size) {
		at = write_tile_copies(at, size);
		mem_[at++] = 0xc3; // jp
		mem_[at++] = static_cast<uint8_t>(ADDR_ProcessLevelTileQueue_VRAM1);
		mem_[at] = static_cast<uint8_t>(ADDR_ProcessLevelTileQueue_VRAM1 >> 8);
		at = write_tile_copies(wVBlankFuncExtended, mem_[wBGMapTileQueueSize]);
		gb_->cpu.c = 0;
		gb_->cpu.f = FLAG_N | FLAG_Z; // dec c: 1 -> 0
	} else {
		gb_->cpu.f = flags::and_(0);
	}
	mem_[at++] = 0xc3; // jp
	mem_[at++] = static_cast<uint8_t>(ADDR_ClearLevelTileQueue);
	mem_[at] = static_cast<uint8_t>(ADDR_ClearLevelTileQueue >> 8);
	gb_->cpu.a = static_cast<uint8_t>(ADDR_ClearLevelTileQueue >> 8);
	SET_HL(gb_, at);
}

} // namespace

} // namespace wl3::level

using wl3::level::LevelState;

// Called from MainStateTable (the farcall to _LevelStateTable): its states
// leave bc, de and hl as they end, and the main loop uses none of them. a
// and f are hCallFunc's pop af: the ROM bank, and the flags it was called
// with.
PORT_EX(LevelStateTable, 0, R_B | R_C | R_D | R_E | R_H | R_L)
{
	uint8_t flags = gb->cpu.f;
	wl3::Routines routines(gb);
	routines.in_bank(BANK__LevelStateTable, ADDR__LevelStateTable, [&] { LevelState(gb).run_state(); });
	gb->cpu.a = wl3::Memory(gb)[wROMBank];
	gb->cpu.f = flags;
}

// Called from MainStateTable: hl = wTimer, a the timer (or ST_LEVEL), the
// flags of cp 100.
PORT(CollectKeyDelay) { LevelState(gb).collect_key_delay(); }

PORT(TickLevelTime) { LevelState(gb).tick_time(); }

// a = the SRAM bank, hl = $b000, f = cp $68's (or a = 0 and and a's when
// there was nothing to do)
PORT(UpdateSwitchableBlocks) { LevelState(gb).update_switchable_blocks(); }

// hl = the last byte written, a = HIGH(ClearLevelTileQueue); with a queue,
// c = 0 and dec c's flags, else and a's
PORT(SetTileQueueProcessingFunctions) { LevelState(gb).set_tile_queue_functions(); }

// copies .Func (jp LevelVBlankFunc) to wVBlankFunc, with CopyHLToDE's
// registers
PORT(VBlank_Level)
{
	SET_HL(gb, ADDR_VBlank_Level + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 0x03; // .end - .Func
	wl3::Routines(gb).call(ADDR_CopyHLToDE);
}

// The routines implemented inside this module (they have no hooks).
PORT_COVERED(_LevelStateTable);
PORT_COVERED(InitLevel);
PORT_COVERED(LoadLevelCommonGfxAndTreasures);
