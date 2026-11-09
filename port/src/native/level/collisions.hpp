// Wario's collision checks (src/engine/level/collisions.asm) as plain
// functions. Each looks up the blocks at points around Wario's collision
// box and runs their block functions (ProcessBlock, still register-level,
// through the emulator by address: BANK(CheckCentreCollision) must be
// mapped, as the farcall maps it). The result is what the asm leaves in b:
// nonzero when a block collided (the block function's result), or the
// steppable object Wario stands on (wIsOnSteppableObject).
#pragma once

#include "../native.hpp"

namespace wl3::level {

uint8_t check_centre_collision(GB *gb); // CheckCentreCollision
uint8_t check_front_collision(GB *gb); // CheckFrontCollision
uint8_t check_up_collision(GB *gb); // CheckUpCollision
uint8_t check_airborne_collision(GB *gb); // CheckAirborneCollision
// Func_19b51: the block at hPos, its bottom points
uint8_t check_block_below(GB *gb);
uint8_t check_func_19741(GB *gb); // Func_19741
uint8_t check_func_197b1(GB *gb); // Func_197b1
uint8_t check_func_198c7(GB *gb); // Func_198c7
uint8_t check_func_19a77(GB *gb); // Func_19a77
uint8_t check_func_19acd(GB *gb); // Func_19acd
uint8_t check_func_19ae4(GB *gb); // Func_19ae4
uint8_t check_func_19afb(GB *gb); // Func_19afb
uint8_t check_func_19b12(GB *gb); // Func_19b12
uint8_t check_func_19b25(GB *gb); // Func_19b25

// The same, for register-level code (the Wario states, through AsmModule's
// centre_collision() etc.): the result in b and the registers the asm
// leaves (all but a and f, which the farcall's pop af restores)
void asm_centre_collision(GB *gb);
void asm_front_collision(GB *gb);
void asm_up_collision(GB *gb);
void asm_airborne_collision(GB *gb);

} // namespace wl3::level
