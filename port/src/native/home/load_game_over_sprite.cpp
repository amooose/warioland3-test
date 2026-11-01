// home/load_game_over_sprite (src/home/load_game_over_sprite.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeLoadGameOverSprite : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_LoadGameOverSprite()
	{
		uint16_t hl = REG_HL(gb_);
		cpu_.a = alu_.add(rd8(hl++), OAM_Y_OFS);
		wr8(wCurSpriteYCoord, cpu_.a);
		cpu_.a = alu_.add(rd8(hl++), OAM_X_OFS);
		wr8(wCurSpriteXCoord, cpu_.a);
		cpu_.a = rd8(hl++);
		wr8(wCurSpriteFrame, cpu_.a);
		cpu_.a = rd8(hl);
		wr8(wCurSpriteAttributes, cpu_.a);
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = BANK_OAM_aab5c;
		bankswitch(cpu_.a);
		SET_HL(gb_, ADDR_OAM_aab5c);
		CALL(LoadSprite);
		pop_af();
		bankswitch(cpu_.a);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeLoadGameOverSprite;

PORT_EX(LoadGameOverSprite, 0, 0) { HomeLoadGameOverSprite(gb).port_LoadGameOverSprite(); }
