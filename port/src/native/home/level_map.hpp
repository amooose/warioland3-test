// The level's block map in SRAM (src/home/decompress_level_data.asm), for
// C++ modules to call directly.
#pragma once

#include "../native.hpp"

namespace wl3::home {

struct BlockPtr {
	uint16_t ptr;
	uint8_t bank;
};

// GetBlockPtr: the block at the position at pos (a big-endian y, then x)
// in the level's block map, also in wBlockPtr and wBlockPtrBank (x, if
// given: the x bytes read)
BlockPtr block_ptr(GB *gb, uint16_t pos, uint8_t *x = nullptr);

// GetBlockPtrBGMapAddress: the v0BGMap0 address of the block at ptr, also
// in wBGPtr
uint16_t block_bg_map_address(GB *gb, uint16_t ptr);

} // namespace wl3::home
