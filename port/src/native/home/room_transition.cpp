// home/room_transition (src/home/room_transition.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeRoomTransition : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_TriggerRoomTransition()
	{
		cpu_.a = 0;
		wr8(wGrabState, 0);
		wr8(wRoomPalCycleDuration, 0);
		wr8(wRoomAnimatedPalsEnabled, 0);
		wr8(wWaterCurrent, 0);
		wr8(wLastWaterCurrent, 0);
		wr8(wCurWaterCurrent, 0);
		wr8(wBossState, 0);
		wr8(wIsBossBattle, 0);
		cpu_.a = TRUE;
		cpu_.f = 0; // xor a / inc a
		wr8(wIsIntangible, cpu_.a);

		cpu_.a = rd8(wRoomTransitionParam);
		if (alu_.bit(cpu_.a, ROOMTRANSITIONF_DELAY_F))
			return; // delayed, don't transition for now

		uint16_t hl = wSubState;
		SET_HL(gb_, hl);
		cpu_.a = rd8(wRoomTransitionParam);
		if (alu_.bit(cpu_.a, ROOMTRANSITIONF_DOOR_F)) {
			wr8(hl, alu_.inc(rd8(hl))); // door transition
			FARCALL(GetWarioBGPtr);
			// push_wram $03
			cpu_.a = rd8(rWBK);
			push_af();
			cpu_.a = 0x03;
			wr8(rWBK, cpu_.a);
			cpu_.a = cpu_.h;
			wr8(wWarioTileBGPtr + 1, cpu_.a);
			cpu_.a = cpu_.l;
			wr8(wWarioTileBGPtr + 0, cpu_.a);
			// pop_wram
			pop_af();
			wr8(rWBK, cpu_.a);
			return;
		}

		// .not_door
		wr8(hl, alu_.inc(rd8(hl)));
		wr8(hl, alu_.inc(rd8(hl)));
		// stop_music
		wr8(hMusicID + 0, 0xff);
		wr8(hMusicID + 1, 0x00);
		// stop_sfx
		wr8(hSFXID + 0, 0xff);
		cpu_.a = 0x00;
		wr8(hSFXID + 1, 0x00);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeRoomTransition;

PORT_EX(TriggerRoomTransition, 0, 0) { HomeRoomTransition(gb).port_TriggerRoomTransition(); }
