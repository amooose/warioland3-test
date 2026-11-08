// home/return_level (src/home/return_level.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeReturnLevel : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_ReturnToPendingLevelState()
	{
		SET_HL(gb_, wState);
		wr8(wState, ST_LEVEL);
		cpu_.a = rd8(wPendingSubState);
		wr8(wSubState, cpu_.a);
	}

	void port_ReturnToMap()
	{
		cpu_.a = TRANSITION_RETURN_TO_MAP;
		wr8(wTransitionParam, TRANSITION_RETURN_TO_MAP);
		FARCALL(StartOverworldState);
	}

	void port_ReturnToMapFromLevel()
	{
		SET_HL(gb_, wDayNight);
		wr8(wDayNight, (uint8_t)(rd8(wDayNight) | 0x80));
		JUMP(ReturnToMap); // fallthrough
	}

	void port_ReturnToLevelFromGolf()
	{
		CALL(DisableLCD);
		// push_wram $03
		cpu_.a = rd8(rWBK);
		push_af();
		cpu_.a = 0x03;
		wr8(rWBK, cpu_.a);
		cpu_.a = rd8(wTempAnimatedTilesFrameDuration);
		wr8(wAnimatedTilesFrameDuration, cpu_.a);
		cpu_.a = rd8(wTempAnimatedTilesGroup);
		wr8(wAnimatedTilesGfx, cpu_.a);
		// pop_wram
		pop_af();
		wr8(rWBK, cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wAnimatedTilesFrameCount, 0);
		wr8(wAnimatedTilesFrame, 0);
		cpu_.a = TRUE;
		wr8(wRoomAnimatedTilesEnabled, TRUE);
		CALL(LoadBackupVRAM);
		cpu_.a = alu_.and_(rd8(wRoomTransitionParam), ROOMTRANSITION_FLAGS_MASK);
		cpu_.a = alu_.or_(cpu_.a, ROOMTRANSITION_DOOR);
		wr8(wRoomTransitionParam, cpu_.a);
		FARCALL(DrawLevelObjectsAfterLevelReturn);
		cpu_.a = LCDC_DEFAULT;
		wr8(rLCDC, cpu_.a);
		JUMP(ReturnToPendingLevelState); // fallthrough
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeReturnLevel;

PORT_EX(ReturnToPendingLevelState, 0, 0) { HomeReturnLevel(gb).port_ReturnToPendingLevelState(); }
PORT_EX(ReturnToMap, 0, 0) { HomeReturnLevel(gb).port_ReturnToMap(); }
PORT_EX(ReturnToMapFromLevel, HOOK_MANUAL_RET, 0) { HomeReturnLevel(gb).port_ReturnToMapFromLevel(); }
// ends on DrawLevelObjectsAfterLevelReturn (b to l as it leaves them); only
// _GolfStateTable reaches it, and nothing reads b to l after (live.py)
PORT_EX(ReturnToLevelFromGolf, HOOK_MANUAL_RET, R_B | R_C | R_D | R_E | R_H | R_L) { HomeReturnLevel(gb).port_ReturnToLevelFromGolf(); }
