// engine/print_counts (src/engine/print_counts.asm) as plain C++. The
// callers are C++ (the overworld's events), which read no register after.
#include "../home/ow.hpp"
#include "../native.hpp"
#include "../routines.hpp"

// sets flags in wKeyAndTreasureFlags corresponding to the collected
// treasures in wOWLevel
PORT_EX(GetOWLevelCollectedTreasures, 0, ALL_REGISTERS)
{
	static constexpr uint8_t BITS[4] = {GREY_TREASURE_F, RED_TREASURE_F, GREEN_TREASURE_F, BLUE_TREASURE_F};
	wl3::Memory mem(gb);
	mem[wKeyAndTreasureFlags] = 0;
	uint8_t wbk = mem[rWBK]; // push_wram BANK(wOWLevel)
	mem[rWBK] = BANK_wOWLevel;
	uint8_t entry = static_cast<uint8_t>((mem[wOWLevel] - 1) * 4);
	uint16_t ids = static_cast<uint16_t>(ADDR_LevelTreasureIDs + NUM_LEVEL_TREASURES + entry);
	for (int i = 0; i < 4; i++) {
		if (wl3::home::is_treasure_collected(gb, mem[static_cast<uint16_t>(ids + i)]))
			mem[wKeyAndTreasureFlags] = mem[wKeyAndTreasureFlags] | (1 << BITS[i]);
	}
	mem[rWBK] = wbk;
}
