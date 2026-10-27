// Port of src/home/update_level_music.asm
#include "ported.h"

#define BOSS_MUSIC_IDS (ADDR_UpdateLevelMusic + 0x70) // .BossMusicIDs

// d = 0 / add a / ld e, a / rl d: de = a * 2
static void double_index(GB *gb)
{
	gb->cpu.d = 0x00;
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = (gb->cpu.f & FLAG_C) ? 1 : 0; // rl d
	gb->cpu.f = gb->cpu.d ? 0 : FLAG_Z;
}

// hl = music ID (little-endian) to load to hMusicID (big-endian)
static void load_music_id(GB *gb, uint16_t hl)
{
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hMusicID + 1, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hMusicID + 0, gb->cpu.a);
	SET_HL(gb, hl);
}

// loads a music ID to hMusicID, depending on wLevel
// and whether there's a new transformation music
PORT(UpdateLevelMusic)
{
	gb->cpu.a = rd8(gb, wBossState);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		// .boss_music: play music corresponding to wBossState
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		gb->cpu.d = 0x00;
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
		gb->cpu.e = gb->cpu.a;
		load_music_id(gb, alu_add16(gb, BOSS_MUSIC_IDS, REG_DE(gb)));
		return;
	}

	gb->cpu.a = rd8(gb, wTransformation);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		// .transformation
		gb->cpu.a = alu_and(gb, rd8(gb, wTransformation), 0x1f);
		double_index(gb);
		gb->cpu.a = rd8(gb, wROMBank);
		push_af(gb);
		gb->cpu.a = BANK_TransformationMusicIDs;
		bankswitch(gb, gb->cpu.a);
		uint16_t hl = alu_add16(gb, ADDR_TransformationMusicIDs, REG_DE(gb));
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, 0xff);
		if (!(gb->cpu.f & FLAG_Z)) {
			load_music_id(gb, hl);
			pop_af(gb);
			bankswitch(gb, gb->cpu.a);
			return;
		}
		// null: .level_music, with the bank already pushed
	} else {
		gb->cpu.a = rd8(gb, wROMBank);
		push_af(gb);
		gb->cpu.a = BANK_LevelMusic;
		bankswitch(gb, gb->cpu.a);
	}

	// .level_music
	gb->cpu.a = rd8(gb, wLevel);
	double_index(gb);
	load_music_id(gb, alu_add16(gb, ADDR_LevelMusic, REG_DE(gb)));
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}

PORT(InitGameState)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wState, 0);
	wr8(gb, wSubState, 0);
}
