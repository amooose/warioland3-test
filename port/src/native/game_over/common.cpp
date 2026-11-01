// game_over/common (src/engine/game_over/common.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::game_over {

namespace {

class GameOverCommon : public AsmModule {
public:
	using AsmModule::AsmModule;

	// Its state routines leave bc, de and hl as they end, and MainStateTable
	// returns to the main loop, where none is live (see src/native/title).
	void port_GameOverStateTable()
	{
		// push_wram BANK("GFX RAM")
		cpu_.a = rd8(rWBK);
		push_af();
		cpu_.a = 0x03;
		wr8(rWBK, cpu_.a);
		FARCALL(_GameOverStateTable);
		// pop_wram
		pop_af();
		wr8(rWBK, cpu_.a);
	}
};

} // namespace

} // namespace wl3::game_over

using wl3::game_over::GameOverCommon;

PORT_EX(GameOverStateTable, 0, R_B | R_C | R_D | R_E | R_H | R_L) { GameOverCommon(gb).port_GameOverStateTable(); }
