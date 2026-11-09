// overworld/bank20 (src/engine/overworld/bank20.asm): what's left outside
// the overworld module. The bank's other routines are unreferenced in the
// original, so they have no port.
#include "../native.hpp"
#include "../routines.hpp"
#include "overworld.hpp"

namespace wl3::overworld {

// MapLevelCoords: a pointer per map side to its levels' coordinates, two
// words a level (GetCthWordFromAthTable with c = level * 2)
uint16_t map_level_coords(GB *gb, uint8_t level)
{
	Memory mem(gb);
	uint16_t side = mem.word(static_cast<uint16_t>(ADDR_MapLevelCoords + mem[wCurMapSide] * 2));
	return static_cast<uint16_t>(side + static_cast<uint8_t>(level * 2) * 2);
}

} // namespace wl3::overworld
