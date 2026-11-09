// Home's floor transitions (src/home/floor_transition.asm) as plain C++;
// see floor_transition.hpp. No caller reads a register these leave.
#include "floor_transition.hpp"

#include "../routines.hpp"
#include "memory.hpp"

namespace wl3::home {

// returns:
// 6 if   $0 <= ypos <  $80
// 5 if  $80 <= ypos < $100
// 4 if $100 <= ypos < $180
// 3 if $180 <= ypos < $200
// 2 if $200 <= ypos < $280
// 1 if $280 <= ypos < $300
uint8_t floor_for_y_pos(GB *gb)
{
	Memory mem(gb);
	uint8_t hi = mem[wWarioYPos + 0];
	uint8_t floor = hi == 1 ? 3 : hi == 2 ? 1 : 5;
	if (mem[wWarioYPos + 1] < 0x80)
		floor++;
	return floor;
}

void update_floor(GB *gb)
{
	Memory mem(gb);
	mem[wFloor] = floor_for_y_pos(gb);
}

void start_floor_transition(GB *gb, uint8_t floor, bool up)
{
	Memory mem(gb);
	mem[wFloor] = floor;
	uint8_t timer = (!up && floor == 5) ? 0x68 : 0x80;
	if (!up)
		mem[wFloorTransitionTimer] = timer;
	play_sfx(mem, SFX_PIPE_EXIT);
	mem[wFloorTransitionDir] = up ? FLOOR_TRANSITION_DOWN : FLOOR_TRANSITION_UP;
	mem[wIsFloorTransition] = TRUE;
	if (up)
		mem[wFloorTransitionTimer] = timer;
	mem[wCameraRoomYScrollLimit] = 0;
}

namespace {

// wFloor less Wario's floor, if the camera does transitions
bool floor_delta(GB *gb, int &delta, uint8_t &floor)
{
	Memory mem(gb);
	if ((mem[wCameraConfigFlags] & CAM_SCROLLING_MASK) < CAM_TRANSITIONS)
		return false;
	floor = floor_for_y_pos(gb);
	delta = mem[wFloor] - floor;
	return true;
}

} // namespace

bool handle_upwards_floor_transition(GB *gb)
{
	int delta;
	uint8_t floor;
	if (!floor_delta(gb, delta, floor) || delta >= 0)
		return false;
	start_floor_transition(gb, floor, true);
	return true;
}

void handle_downwards_floor_transition(GB *gb)
{
	int delta;
	uint8_t floor;
	if (floor_delta(gb, delta, floor) && delta > 0)
		start_floor_transition(gb, floor, false);
}

void handle_floor_transition(GB *gb)
{
	int delta;
	uint8_t floor;
	if (floor_delta(gb, delta, floor) && delta)
		start_floor_transition(gb, floor, delta < 0);
}

} // namespace wl3::home

using namespace wl3::home;

// c = the floor
PORT_EX(GetFloorForYPos, 0, ALL_REGISTERS & ~R_C) { gb->cpu.c = floor_for_y_pos(gb); }
PORT_EX(UpdateFloor, 0, ALL_REGISTERS) { update_floor(gb); }
// c = the new floor
PORT_EX(StartDownwardsFloorTransition, 0, ALL_REGISTERS) { start_floor_transition(gb, gb->cpu.c, false); }
PORT_EX(StartUpwardsFloorTransition, 0, ALL_REGISTERS) { start_floor_transition(gb, gb->cpu.c, true); }
PORT_EX(HandleDownwardsFloorTransition, 0, ALL_REGISTERS) { handle_downwards_floor_transition(gb); }
// unreferenced?
PORT_EX(Func_1180, 0, ALL_REGISTERS) { handle_floor_transition(gb); }
PORT_EX(HandleUpwardsFloorTransition, 0, ALL_REGISTERS) { handle_upwards_floor_transition(gb); }
