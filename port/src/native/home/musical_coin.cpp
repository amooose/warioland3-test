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

	// rrca: z, n and h cleared, carry is the bit rotated out
	uint8_t rrca(uint8_t v)
	{
		cpu_.f = (v & 1) ? FLAG_C : 0;
		return (uint8_t)(v >> 1 | v << 7);
	}

	// returns nz if has all the levels
	// with their corresponding Musical Coin flag set
	void port_CheckHasAllMusicalCoinFlags()
	{
		uint16_t hl = wMusicalCoinFlags;
		cpu_.c = 3;
		do {
			cpu_.b = 8;
			cpu_.a = rd8(hl++);
			do {
				cpu_.a = rrca(cpu_.a);
				if (!(cpu_.f & FLAG_C))
					goto false_;
				cpu_.b = alu_.dec(cpu_.b);
			} while (cpu_.b);
			cpu_.c = alu_.dec(cpu_.c);
		} while (cpu_.c);
		// 1 bit still missing
		cpu_.a = rrca(rd8(hl));
		if (!(cpu_.f & FLAG_C))
			goto false_;
		SET_HL(gb_, hl);
		cpu_.a = TRUE;
		return;
	false_:
		SET_HL(gb_, hl);
		cpu_.a = FALSE;
		cpu_.f = FLAG_Z;
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeMusicalCoin;

PORT_EX(CheckLevelMusicalCoinFlag, 0, 0) { HomeMusicalCoin(gb).port_CheckLevelMusicalCoinFlag(); }
PORT_EX(ResumeNormalVolume, 0, 0) { HomeMusicalCoin(gb).port_ResumeNormalVolume(); }
PORT_EX(CheckHasAllMusicalCoinFlags, 0, 0) { HomeMusicalCoin(gb).port_CheckHasAllMusicalCoinFlags(); }
