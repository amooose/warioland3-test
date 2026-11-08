// Home's floor transitions (src/home/floor_transition.asm) as plain C++;
// see floor_transition.hpp. The hooks leave the registers the asm leaves
// (Func_19720 jumps to them, and its callers are register-level).
#include "floor_transition.hpp"

#include "../flags.hpp"
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
namespace flags = wl3::flags;

namespace {

// GetFloorForYPos's registers: c = the floor, a and f from the cp $80 (and
// the inc c after it)
void floor_regs(GB *gb)
{
	wl3::Memory mem(gb);
	uint8_t floor = floor_for_y_pos(gb);
	uint8_t lo = mem[wWarioYPos + 1];
	gb->cpu.a = lo;
	gb->cpu.f = flags::cp(lo, 0x80);
	if (lo < 0x80)
		gb->cpu.f = flags::inc(gb->cpu.f, static_cast<uint8_t>(floor - 1));
	gb->cpu.c = floor;
}

// The camera and floor check: false (with carry) without CAM_TRANSITIONS,
// else a = wFloor - c and its flags
bool check_regs(GB *gb)
{
	wl3::Memory mem(gb);
	uint8_t a = mem[wCameraConfigFlags] & CAM_SCROLLING_MASK;
	gb->cpu.a = a;
	gb->cpu.f = flags::cp(a, CAM_TRANSITIONS);
	if (a < CAM_TRANSITIONS)
		return false;
	floor_regs(gb);
	uint8_t floor = mem[wFloor];
	gb->cpu.a = static_cast<uint8_t>(floor - gb->cpu.c);
	gb->cpu.f = flags::sub(floor, gb->cpu.c);
	return true;
}

// StartUpwards / DownwardsFloorTransition (c = the new floor), ending on xor a
void start_regs(GB *gb, bool up)
{
	start_floor_transition(gb, gb->cpu.c, up);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
}

} // namespace

PORT_EX(GetFloorForYPos, 0, 0) { floor_regs(gb); }
PORT_EX(UpdateFloor, 0, 0)
{
	floor_regs(gb);
	gb->cpu.a = gb->cpu.c;
	update_floor(gb);
}
PORT_EX(StartDownwardsFloorTransition, 0, 0) { start_regs(gb, false); }
PORT_EX(StartUpwardsFloorTransition, 0, 0) { start_regs(gb, true); }
PORT_EX(HandleDownwardsFloorTransition, 0, 0)
{
	if (check_regs(gb) && !(gb->cpu.f & (FLAG_Z | FLAG_C)))
		start_regs(gb, false);
}
// unreferenced?
PORT_EX(Func_1180, 0, 0)
{
	if (check_regs(gb) && !(gb->cpu.f & FLAG_Z))
		start_regs(gb, gb->cpu.f & FLAG_C);
}
PORT_EX(HandleUpwardsFloorTransition, 0, 0)
{
	if (check_regs(gb) && (gb->cpu.f & FLAG_C))
		start_regs(gb, true);
}
