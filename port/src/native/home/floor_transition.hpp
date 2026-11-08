// Home's floor transitions as plain functions (src/home/floor_transition.asm),
// for C++ modules to call directly; the hooks (floor_transition.cpp) stay
// register-exact for the register-level callers.
#pragma once

#include "../native.hpp"

namespace wl3::home {

// GetFloorForYPos: Wario's floor by his y position (6 at the top, to 1)
uint8_t floor_for_y_pos(GB *gb);
// HandleUpwardsFloorTransition / HandleDownwardsFloorTransition: with
// CAM_TRANSITIONS, a floor transition when Wario's moved a floor up (down)
void handle_upwards_floor_transition(GB *gb);
void handle_downwards_floor_transition(GB *gb);

} // namespace wl3::home
