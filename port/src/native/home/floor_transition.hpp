// Home's floor transitions as plain functions (src/home/floor_transition.asm),
// for C++ modules to call directly; their hooks (floor_transition.cpp)
// wrap them, leaving the registers the asm leaves.
#pragma once

#include "../native.hpp"

namespace wl3::home {

// GetFloorForYPos: Wario's floor by his y position (6 at the top, to 1)
uint8_t floor_for_y_pos(GB *gb);
void update_floor(GB *gb); // UpdateFloor: wFloor by it
// StartUpwardsFloorTransition / StartDownwardsFloorTransition: to floor
void start_floor_transition(GB *gb, uint8_t floor, bool up);
// HandleUpwardsFloorTransition / HandleDownwardsFloorTransition: with
// CAM_TRANSITIONS, a floor transition when Wario's moved a floor up (down)
bool handle_upwards_floor_transition(GB *gb); // true if one started
void handle_downwards_floor_transition(GB *gb);
// Func_1180: either way
void handle_floor_transition(GB *gb);

} // namespace wl3::home
