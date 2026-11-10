// A room's graphics (room_gfx.cpp): plain functions for C++ callers
#pragma once

#include "../native.hpp"

namespace wl3::level {

// GetWarioBGPtr: the BG map address 24 pixels above Wario, also in wBGPtr
uint16_t wario_bg_ptr(GB *gb);
// GetBlockBGPtr: the BG map address of the block at wBlockYPos / wBlockXPos
// (also left in wBGPtr)
uint16_t block_bg_ptr(GB *gb);

} // namespace wl3::level
