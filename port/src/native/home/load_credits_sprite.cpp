// home/load_credits_sprite (src/home/load_credits_sprite.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeLoadCreditsSprite : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_LoadCreditsMusicBoxSprite()
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
		cpu_.a = rd8(wCreditsMusicBoxOAMBank);
		wr8(wTempBank, cpu_.a);
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = rd8(wTempBank);
		bankswitch(cpu_.a);
		cpu_.h = rd8(wCreditsMusicBoxOAMPtr + 0);
		cpu_.a = rd8(wCreditsMusicBoxOAMPtr + 1);
		cpu_.l = cpu_.a;
		CALL(LoadSprite);
		pop_af();
		bankswitch(cpu_.a);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeLoadCreditsSprite;

PORT_EX(LoadCreditsMusicBoxSprite, 0, 0) { HomeLoadCreditsSprite(gb).port_LoadCreditsMusicBoxSprite(); }
