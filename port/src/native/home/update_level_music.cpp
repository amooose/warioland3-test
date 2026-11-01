// home/update_level_music (src/home/update_level_music.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeUpdateLevelMusic : public AsmModule {
public:
	using AsmModule::AsmModule;

	#define BOSS_MUSIC_IDS (ADDR_UpdateLevelMusic + 0x70) // .BossMusicIDs

	// d = 0 / add a / ld e, a / rl d: de = a * 2
	void double_index()
	{
		cpu_.d = 0x00;
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.e = cpu_.a;
		cpu_.d = (cpu_.f & FLAG_C) ? 1 : 0; // rl d
		cpu_.f = cpu_.d ? 0 : FLAG_Z;
	}

	// hl = music ID (little-endian) to load to hMusicID (big-endian)
	void load_music_id(uint16_t hl)
	{
		cpu_.a = rd8(hl++);
		wr8(hMusicID + 1, cpu_.a);
		cpu_.a = rd8(hl);
		wr8(hMusicID + 0, cpu_.a);
		SET_HL(gb_, hl);
	}

	// loads a music ID to hMusicID, depending on wLevel
	// and whether there's a new transformation music
	void port_UpdateLevelMusic()
	{
		cpu_.a = rd8(wBossState);
		alu_.and_(cpu_.a, cpu_.a);
		if (!(cpu_.f & FLAG_Z)) {
			// .boss_music: play music corresponding to wBossState
			cpu_.a = alu_.dec(cpu_.a);
			cpu_.d = 0x00;
			cpu_.a = alu_.add(cpu_.a, cpu_.a);
			cpu_.e = cpu_.a;
			load_music_id(alu_.add16(BOSS_MUSIC_IDS, REG_DE(gb_)));
			return;
		}

		cpu_.a = rd8(wTransformation);
		alu_.and_(cpu_.a, cpu_.a);
		if (!(cpu_.f & FLAG_Z)) {
			// .transformation
			cpu_.a = alu_.and_(rd8(wTransformation), 0x1f);
			double_index();
			cpu_.a = rd8(wROMBank);
			push_af();
			cpu_.a = BANK_TransformationMusicIDs;
			bankswitch(cpu_.a);
			uint16_t hl = alu_.add16(ADDR_TransformationMusicIDs, REG_DE(gb_));
			cpu_.a = rd8(hl);
			alu_.cp(cpu_.a, 0xff);
			if (!(cpu_.f & FLAG_Z)) {
				load_music_id(hl);
				pop_af();
				bankswitch(cpu_.a);
				return;
			}
			// null: .level_music, with the bank already pushed
		} else {
			cpu_.a = rd8(wROMBank);
			push_af();
			cpu_.a = BANK_LevelMusic;
			bankswitch(cpu_.a);
		}

		// .level_music
		cpu_.a = rd8(wLevel);
		double_index();
		load_music_id(alu_.add16(ADDR_LevelMusic, REG_DE(gb_)));
		pop_af();
		bankswitch(cpu_.a);
	}

	void port_InitGameState()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wState, 0);
		wr8(wSubState, 0);
	}

	void port_SetPerfectState()
	{
		SET_HL(gb_, wState);
		wr8(wState, ST_PERFECT);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wSubState, 0);
	}

	void port_SaveGameOrEnterPerfectScreen()
	{
		bool perfect = false;
		cpu_.a = rd8(wGameModeFlags);
		if (!alu_.bit(cpu_.a, MODE_TIME_ATTACK_F)) {
			cpu_.a = rd8(wTransitionParam);
			alu_.cp(cpu_.a, TRANSITION_EPILOGUE_NOT_PERFECT);
			if (cpu_.f & FLAG_Z) goto dont_set_perfect;
			alu_.cp(cpu_.a, TRANSITION_EPILOGUE_PERFECT);
			if (cpu_.f & FLAG_Z) goto dont_set_perfect;
			alu_.cp(cpu_.a, TRANSITION_GAME_OVER);
			if (cpu_.f & FLAG_Z) goto dont_set_perfect;
			cpu_.a = alu_.and_(rd8(wGameModeFlags), MODE_GAME_CLEARED);
			if (cpu_.f & FLAG_Z) goto dont_set_perfect;
			cpu_.a = alu_.dec(rd8(wNumCollectedTreasures));
			perfect = (cpu_.f & FLAG_Z) != 0;
		}
		if (perfect) {
			// SetPerfectState
			SET_HL(gb_, wState);
			wr8(wState, ST_PERFECT);
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
			wr8(wSubState, 0);
			return;
		}
	dont_set_perfect:
		cpu_.a = rd8(wSubState);
		wr8(wPendingSubState, cpu_.a);
		cpu_.a = ST_PAUSE_MENU;
		wr8(wState, cpu_.a);
		cpu_.a = SST_PAUSE_18;
		wr8(wSubState, cpu_.a);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeUpdateLevelMusic;

PORT_EX(UpdateLevelMusic, 0, 0) { HomeUpdateLevelMusic(gb).port_UpdateLevelMusic(); }
PORT_EX(InitGameState, 0, 0) { HomeUpdateLevelMusic(gb).port_InitGameState(); }
PORT_EX(SetPerfectState, 0, 0) { HomeUpdateLevelMusic(gb).port_SetPerfectState(); }
PORT_EX(SaveGameOrEnterPerfectScreen, 0, 0) { HomeUpdateLevelMusic(gb).port_SaveGameOrEnterPerfectScreen(); }
