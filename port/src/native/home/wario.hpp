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
// ClearTransformationValues: back to normal Wario (after Blind, the
// palettes it darkened are restored, one per H-Blank)
void clear_transformation_values(GB *gb);

// wario_mechanics1.cpp:
// ApplyJumpVelocity: Wario up or down by the next entry of his jump
// velocity table (falling, the index stops at MAX_JUMP_VEL_INDEX)
void apply_jump_velocity(GB *gb);
// ApplyWalkVelocity_Right / _Left: the next walk velocity (wWalkVelIndex
// advances, from $1b back to $18), from a standstill when Wario faces the
// other way, back to 4 from 8 on a slope that way
uint8_t apply_walk_velocity(GB *gb, bool right);
// TriggerFloorTransition(_SkipUpdateYPos): Wario's y from hYPos
// (update_y), then a floor transition if he's on another floor
void trigger_floor_transition(GB *gb, bool update_y = true);
// RecoverFromTransformation(_WithoutInvincibility): back to normal, the
// level music and palette, idling or falling
void recover_from_transformation(GB *gb, bool invincible = true);

// wario_mechanics2.cpp:
// IsOnSlipperyGround: the blocks under the collision box's left and right
// edges (3 pixels in): wIsOnSlipperyGround, if either is slippery
bool is_on_slippery_ground(GB *gb);
void release_owl(GB *gb); // ReleaseOwl: the owl particle, $18 above Wario

// room_transition.cpp:
// TriggerRoomTransition: Wario through the room's exit (unless it's
// delayed), by a door or not
void trigger_room_transition(GB *gb);

} // namespace wl3::home
