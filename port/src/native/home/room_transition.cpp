// home/room_transition (src/home/room_transition.asm) as plain C++. The
// callers are the Wario states (C++), which read no register after it.
#include "../level/room_gfx.hpp"
#include "../native.hpp"
#include "../routines.hpp"
#include "wario.hpp"

#include <initializer_list>

namespace wl3::home {

void trigger_room_transition(GB *gb)
{
	Memory mem(gb);
	for (uint16_t at : {wGrabState, wRoomPalCycleDuration, wRoomAnimatedPalsEnabled, wWaterCurrent,
			wLastWaterCurrent, wCurWaterCurrent, wBossState, wIsBossBattle})
		mem[at] = 0;
	mem[wIsIntangible] = TRUE;

	uint8_t param = mem[wRoomTransitionParam];
	if (param >> ROOMTRANSITIONF_DELAY_F & 1)
		return; // delayed, don't transition for now
	if (param >> ROOMTRANSITIONF_DOOR_F & 1) {
		mem[wSubState] = static_cast<uint8_t>(mem[wSubState] + 1);
		uint16_t ptr = 0;
		Routines(gb).in_bank(BANK_GetWarioBGPtr, ADDR_GetWarioBGPtr, [&] { ptr = level::wario_bg_ptr(gb); });
		uint8_t wbk = mem[rWBK]; // push_wram $03
		mem[rWBK] = 0x03;
		mem[wWarioTileBGPtr + 1] = static_cast<uint8_t>(ptr >> 8);
		mem[wWarioTileBGPtr + 0] = static_cast<uint8_t>(ptr);
		mem[rWBK] = wbk;
		return;
	}
	mem[wSubState] = static_cast<uint8_t>(mem[wSubState] + 2);
	mem[hMusicID + 0] = 0xff; // stop_music
	mem[hMusicID + 1] = 0x00;
	mem[hSFXID + 0] = 0xff; // stop_sfx
	mem[hSFXID + 1] = 0x00;
}

} // namespace wl3::home

PORT_EX(TriggerRoomTransition, 0, ALL_REGISTERS) { wl3::home::trigger_room_transition(gb); }
