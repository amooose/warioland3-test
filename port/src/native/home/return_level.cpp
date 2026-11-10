// home/return_level (src/home/return_level.asm) as plain C++. The callers
// are the golf and pause state tables (C++, by address), whose hooks leave
// b to l as they please but a and f as the asm does: these leave a and f
// too.
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"
#include "memory.hpp"
#include "sprites.hpp"

namespace wl3::home {

namespace {

// ReturnToPendingLevelState: back to the level's pending sub-state
uint8_t return_to_pending_level_state(Memory mem)
{
	mem.state() = ST_LEVEL;
	uint8_t sub_state = mem.pending_sub_state();
	mem.sub_state() = sub_state;
	return sub_state;
}

// ReturnToMap: the overworld, by its "return to map" transition (a and f
// as the farcall leaves them)
void return_to_map(GB *gb)
{
	Memory mem(gb);
	mem.transition_param() = TRANSITION_RETURN_TO_MAP;
	Routines(gb).farcall(BANK_StartOverworldState, ADDR_StartOverworldState);
}

// ReturnToLevelFromGolf: the level's animated tiles and VRAM back, Wario
// out of the door
void return_to_level_from_golf(GB *gb)
{
	Memory mem(gb);
	disable_lcd(gb);
	uint8_t wbk = mem[rWBK]; // push_wram $03
	mem[rWBK] = 0x03;
	mem.animated_tiles_frame_duration() = mem.temp_animated_tiles_frame_duration();
	mem.animated_tiles_gfx() = mem.temp_animated_tiles_group();
	mem[rWBK] = wbk;
	mem.animated_tiles_frame_count() = 0;
	mem.animated_tiles_frame() = 0;
	mem.room_animated_tiles_enabled() = TRUE;
	load_backup_vram(gb);
	uint8_t param = (mem.room_transition_param() & ROOMTRANSITION_FLAGS_MASK) | ROOMTRANSITION_DOOR;
	mem.room_transition_param() = param;
	gb->cpu.f = flags::or_(param); // the or's flags survive the farcall, for the state table
	Routines(gb).farcall(BANK_DrawLevelObjectsAfterLevelReturn, ADDR_DrawLevelObjectsAfterLevelReturn);
	mem[rLCDC] = LCDC_DEFAULT;
}

} // namespace

} // namespace wl3::home

using namespace wl3::home;

PORT_EX(ReturnToPendingLevelState, 0, ALL_REGISTERS & ~(R_A | R_F))
{
	gb->cpu.a = return_to_pending_level_state(wl3::Memory(gb));
}
PORT_EX(ReturnToMap, 0, ALL_REGISTERS & ~(R_A | R_F)) { return_to_map(gb); }
PORT_EX(ReturnToMapFromLevel, 0, ALL_REGISTERS & ~(R_A | R_F))
{
	wl3::Memory mem(gb);
	mem.day_night() = mem.day_night() | 0x80;
	return_to_map(gb);
}
// f from the or before the farcall
PORT_EX(ReturnToLevelFromGolf, 0, ALL_REGISTERS & ~(R_A | R_F))
{
	return_to_level_from_golf(gb);
	gb->cpu.a = return_to_pending_level_state(wl3::Memory(gb));
}
