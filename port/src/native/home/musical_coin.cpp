// home/musical_coin (src/home/musical_coin.asm) as plain C++, but for
// ResumeNormalVolume's audio mod (register arguments).
#include "../flags.hpp"
#include "../routines.hpp"
#include "memory.hpp"
#include "system.hpp"

namespace wl3::home {

bool level_musical_coins_collected(GB *gb)
{
	bool collected = false;
	farcall(gb, BANK__CheckLevelMusicalCoinFlag, ADDR__CheckLevelMusicalCoinFlag,
		[&] { collected = level_musical_coin_flag(gb); });
	return collected;
}

} // namespace wl3::home

// nz if collected (Func_9aaf4, C++, reads nothing else)
PORT_EX(CheckLevelMusicalCoinFlag, 0, ALL_REGISTERS & ~(R_A | R_F))
{
	gb->cpu.a = wl3::home::level_musical_coins_collected(gb);
	gb->cpu.f = wl3::flags::and_(gb->cpu.a);
}

PORT_EX(ResumeNormalVolume, 0, 0)
{
	// resume normal volume level
	gb->cpu.a = AUDIOMOD_VOLUME;
	gb->cpu.b = 0x40; // 1.0q6, volume modifier
	gb->cpu.d = SFX_CHANNELS;
	gb->cpu.e = MUSIC_CHANNELS >> 4;
	wl3::Routines r(gb);
	r.call(ADDR_ExecuteAudioMod);
	r.call(ADDR_UpdateAudio);
}
