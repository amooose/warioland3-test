// The block functions (src/engine/level/block_functions.asm), for C++
// callers.
#pragma once

#include "../native.hpp"

namespace wl3::level {

// ProcessBlock: runs the function of the block at wBlockPtrBank:wBlockPtr
// and returns its result (the asm's a). b: the block a function that
// doesn't set one replaces this one with (the caller's b in the asm).
// Leaves b = the result's replacement and hl as the asm does.
uint8_t process_block(GB *gb, uint8_t b);

} // namespace wl3::level
