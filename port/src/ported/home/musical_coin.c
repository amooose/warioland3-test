// Port of src/home/musical_coin.asm
#include "ported.h"

// return nz if level has had all its
// Musical Coins collected already
PORT(CheckLevelMusicalCoinFlag)
{
	FARCALL(_CheckLevelMusicalCoinFlag);
	gb->cpu.a = alu_and(gb, gb->cpu.b, gb->cpu.b);
}

PORT(ResumeNormalVolume)
{
	// resume normal volume level
	gb->cpu.a = AUDIOMOD_VOLUME;
	gb->cpu.b = 0x40; // 1.0q6, volume modifier
	gb->cpu.d = SFX_CHANNELS;
	gb->cpu.e = MUSIC_CHANNELS >> 4;
	CALL(ExecuteAudioMod);
	CALL(UpdateAudio);
}

// rrca: z, n and h cleared, carry is the bit rotated out
static uint8_t rrca(GB *gb, uint8_t v)
{
	gb->cpu.f = (v & 1) ? FLAG_C : 0;
	return (uint8_t)(v >> 1 | v << 7);
}

// returns nz if has all the levels
// with their corresponding Musical Coin flag set
PORT(CheckHasAllMusicalCoinFlags)
{
	uint16_t hl = wMusicalCoinFlags;
	gb->cpu.c = 3;
	do {
		gb->cpu.b = 8;
		gb->cpu.a = rd8(gb, hl++);
		do {
			gb->cpu.a = rrca(gb, gb->cpu.a);
			if (!(gb->cpu.f & FLAG_C))
				goto false_;
			gb->cpu.b = alu_dec(gb, gb->cpu.b);
		} while (gb->cpu.b);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
	} while (gb->cpu.c);
	// 1 bit still missing
	gb->cpu.a = rrca(gb, rd8(gb, hl));
	if (!(gb->cpu.f & FLAG_C))
		goto false_;
	SET_HL(gb, hl);
	gb->cpu.a = TRUE;
	return;
false_:
	SET_HL(gb, hl);
	gb->cpu.a = FALSE;
	gb->cpu.f = FLAG_Z;
}
