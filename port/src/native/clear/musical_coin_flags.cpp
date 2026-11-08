// clear/musical_coin_flags (src/engine/clear/musical_coin_flags.asm): the
// level's musical coin flag, which home code farcalls (the rest is C++,
// src/native/clear).
#include "../native.hpp"
#include "../home/system.hpp"

namespace wl3::home {

// _CheckLevelMusicalCoinFlag: wMusicalCoinFlags' bit for wOWLevel (the
// asm rotates the byte to it and back, leaving it as it was)
bool level_musical_coin_flag(GB *gb)
{
	Memory mem(gb);
	auto level = static_cast<uint8_t>(mem[wOWLevel] - 1);
	return mem[static_cast<uint16_t>(wMusicalCoinFlags + (level >> 3))] >> (level & 0x07) & 1;
}

} // namespace wl3::home

// b = TRUE if set (CheckLevelMusicalCoinFlag, C++, reads nothing else)
PORT_EX(_CheckLevelMusicalCoinFlag, 0, ALL_REGISTERS & ~R_B) { gb->cpu.b = wl3::home::level_musical_coin_flag(gb); }
