// Home's Wario primitives as plain functions (src/home/move_wario.asm,
// wario_mechanics2.asm), for C++ code to call directly; their hooks
// (wario.cpp) wrap them for register-level callers (the Wario states),
// leaving the registers the asm leaves.
#pragma once

#include "../native.hpp"

namespace wl3::home {

// MoveWarioRight / Left / Down / Up: Wario's 16-bit position by delta, and
// the camera's delta with it (the _WithoutCameraOffset ones leave it)
void move_wario_x(GB *gb, int delta);
void move_wario_y(GB *gb, int delta, bool with_camera = true);

void clear_particles(GB *gb); // ClearParticles
// DoGroundShake: the ground shakes for `frames`, unless it already does
void do_ground_shake(GB *gb, uint8_t frames);
void open_action_help(GB *gb); // OpenActionHelp
// Func_1762: caps wWalkVelIndex at $0c from $10 (on the ground or rising),
// at $04 from $08 while falling
void cap_walk_vel(GB *gb);

} // namespace wl3::home
