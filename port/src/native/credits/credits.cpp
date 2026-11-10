// credits/credits (src/engine/credits/credits.asm): the credits' V-Blank
// function (the rest is C++, src/native/screens/credits.cpp).
#include "../asm_module.hpp"
#include "../home/memory.hpp"
#include "../home/pals.hpp"

namespace wl3::credits {

namespace {

class CreditsCredits : public AsmModule {
public:
	using AsmModule::AsmModule;

	// VBlank_160439.Func, as copied to wVBlankFunc, plain over memory: the
	// V-Blank handler pushes and pops every register around it. The
	// scroll, then the step wcee4 bit 7 asks for: 1 the text line, 2 the
	// palettes and the window, else the line's attributes and blanks (then
	// step 1); hTransferVirtualOAM (a = wVirtualOAM's page) at the end.
	void port_VBlank_160439__Func()
	{
		mem_[rSCY] = mem_.scy();
		mem_[rSCX] = mem_.scx();
		uint8_t step = mem_.unk_cee4();
		if (step & 0x80) {
			uint16_t bg_map = static_cast<uint16_t>(mem_.credits_bg_map_ptr() << 8 | mem_[wCreditsBGMapPtr + 1]);
			switch (step & 0x7f) {
			case 1: // .write_text, to the '\n' (the e of de wraps in its page)
				for (uint16_t src = wCreditsTextBuffer;; src++) {
					uint8_t c = mem_[src];
					if (c == 0x7f)
						break;
					mem_[bg_map] = c;
					bg_map = static_cast<uint16_t>((bg_map & 0xff00) | static_cast<uint8_t>(bg_map + 1));
				}
				mem_.unk_cee4() &= 0x7f;
				break;
			case 2: // .asm_160499
				home::apply_bg_pals(gb_, wTempPals1);
				home::apply_ob_pals(gb_, wTempPals2);
				mem_[rLCDC] |= LCDC_WIN_ON | LCDC_WIN_9C00;
				mem_.unk_cee4() &= 0x7f;
				break;
			default:
				mem_[rVBK] = 1; // BANK("VRAM1")
				home::copy(gb_, wCreditsTextPals, bg_map, CREDITS_TEXT_LENGTH);
				mem_[rVBK] = 0;
				home::fill(gb_, bg_map, 0x7e, CREDITS_TEXT_LENGTH);
				mem_.unk_cee4() = 0x01 | (1 << 7);
				break;
			}
		}
		cpu_.a = static_cast<uint8_t>(wVirtualOAM >> 8);
		transfer_virtual_oam(gb_);
	}
};

} // namespace

} // namespace wl3::credits

using wl3::credits::CreditsCredits;

PORT_LOCAL_EX(VBlank_160439, Func, 0, ALL_REGISTERS) { CreditsCredits(gb).port_VBlank_160439__Func(); }
