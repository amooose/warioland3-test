// language_selection/vblank (src/engine/language_selection/vblank.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::language_selection {

namespace {

class LanguageSelectionVblank : public AsmModule {
public:
	using AsmModule::AsmModule;

	// (the rest is C++, src/native/screens)

	// ld a, BANK("VRAM1") / ldh [rVBK], a / then hlbgcoord 5, 7 and 5, 9 filled
	// with 13 times `top` and `bottom` (the call returns at base + ret1, ret2)
	void lang_pals(uint16_t base, uint8_t top, uint8_t bottom, uint8_t ret1, uint8_t ret2)
	{
		cpu_.a = 1;
		wr8(rVBK, cpu_.a);
		SET_HL(gb_, v0BGMap0 + 7 * TILEMAP_WIDTH + 5);
		cpu_.b = 13;
		cpu_.a = top;
		gb_call_ret(gb_, ADDR_WriteAToHL_BTimes, (uint16_t)(base + ret1));
		SET_HL(gb_, v0BGMap0 + 9 * TILEMAP_WIDTH + 5);
		cpu_.b = 13;
		cpu_.a = bottom;
		gb_call_ret(gb_, ADDR_WriteAToHL_BTimes, (uint16_t)(base + ret2));
	}

	// VBlank_LanguageSelection.Func, as copied to wVBlankFunc
	void port_VBlank_LanguageSelection__Func()
	{
		uint16_t base = cpu_.pc;
		cpu_.a = rd8(wcee4);
		if (alu_.bit(cpu_.a, 7)) {
			cpu_.a = alu_.and_(cpu_.a, 0x7f);
			cpu_.a = alu_.dec(cpu_.a);
			if (!(cpu_.f & FLAG_Z))
				lang_pals(base, 0x07, 0x06, 0x1a, 0x24); // japanese
			else
				lang_pals(base, 0x06, 0x07, 0x34, 0x3e); // .english
			SET_HL(gb_, wcee4);
			wr8(wcee4, (uint8_t)(rd8(wcee4) & 0x7f));
		}
		cpu_.a = rd8(wSCY);
		wr8(rSCY, cpu_.a);
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		gb_call_ret(gb_, hTransferVirtualOAM, (uint16_t)(base + 0x52));
		RET();
	}
};

} // namespace

} // namespace wl3::language_selection

using wl3::language_selection::LanguageSelectionVblank;

PORT_LOCAL_EX(VBlank_LanguageSelection, Func, HOOK_MANUAL_RET, 0) { LanguageSelectionVblank(gb).port_VBlank_LanguageSelection__Func(); }
