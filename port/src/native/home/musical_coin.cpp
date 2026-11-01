// home/musical_coin (src/home/musical_coin.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeMusicalCoin : public AsmModule {
public:
	using AsmModule::AsmModule;

	// return nz if level has had all its
	// Musical Coins collected already
	void port_CheckLevelMusicalCoinFlag()
	{
		FARCALL(_CheckLevelMusicalCoinFlag);
		cpu_.a = alu_.and_(cpu_.b, cpu_.b);
	}

	void port_ResumeNormalVolume()
	{
		// resume normal volume level
		cpu_.a = AUDIOMOD_VOLUME;
		cpu_.b = 0x40; // 1.0q6, volume modifier
		cpu_.d = SFX_CHANNELS;
		cpu_.e = MUSIC_CHANNELS >> 4;
		CALL(ExecuteAudioMod);
		CALL(UpdateAudio);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeMusicalCoin;

PORT_EX(CheckLevelMusicalCoinFlag, 0, 0) { HomeMusicalCoin(gb).port_CheckLevelMusicalCoinFlag(); }
PORT_EX(ResumeNormalVolume, 0, 0) { HomeMusicalCoin(gb).port_ResumeNormalVolume(); }
