// home/disable_lcd (src/home/disable_lcd.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeDisableLcd : public AsmModule {
public:
	using AsmModule::AsmModule;

	// Turns the screen off, which the hardware only allows during VBlank.
	void port_DisableLCD()
	{
		cpu_.a = rd8(rLCDC);
		if (!alu_.bit(cpu_.a, B_LCDC_ENABLE))
			return; // already disabled

		cpu_.a = rd8(rIE);
		wr8(wTempIE, cpu_.a);
		// disable VBlank interrupt
		cpu_.a &= (uint8_t)~IE_VBLANK;
		wr8(rIE, cpu_.a);

		// wait until VBlank would normally happen
		for (;;) {
			cpu_.a = rd8(rLY);
			alu_.cp(cpu_.a, LY_VBLANK + 1);
			if (cpu_.f & FLAG_Z)
				break;
			idle(32);
		}

		// turn LCD off
		cpu_.a = alu_.and_(rd8(rLCDC), 0xff ^ LCDC_ON);
		wr8(rLCDC, cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rIF, 0);
		cpu_.a = rd8(wTempIE);
		wr8(rIE, cpu_.a);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeDisableLcd;

PORT_EX(DisableLCD, 0, 0) { HomeDisableLcd(gb).port_DisableLCD(); }
