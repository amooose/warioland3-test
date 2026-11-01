// home/dpad_repeat (src/home/dpad_repeat.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeDpadRepeat : public AsmModule {
public:
	using AsmModule::AsmModule;

	// adds a Repeat Delay and Rate to any DPad key
	void port_ProcessDPadRepeat()
	{
		// push_wram BANK("GFX RAM")
		cpu_.a = rd8(rWBK);
		push_af();
		cpu_.a = BANK_wDPadRepeatCounter;
		wr8(rWBK, cpu_.a);

		uint16_t hl = wDPadRepeatCounter;
		SET_HL(gb_, hl);
		uint8_t v = 0x10; // reset_counter
		bool set = true;
		if (rd8(hl) && !(rd8(wJoypadPressed) & PAD_CTRL_PAD)) {
			uint8_t down = rd8(wJoypadDown) & PAD_CTRL_PAD;
			if (down) {
				uint8_t cnt = (uint8_t)(rd8(hl) - 1);
				wr8(hl, cnt);
				if (cnt) {
					set = false;
				} else {
					// add down Joypad buttons to the pressed buttons
					cpu_.b = down;
					wr8(wJoypadPressed, rd8(wJoypadPressed) | down);
					v = 0x08;
				}
			}
		}
		if (set)
			wr8(hl, v);

		// pop_wram
		pop_af();
		wr8(rWBK, cpu_.a);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeDpadRepeat;

PORT_EX(ProcessDPadRepeat, 0, 0) { HomeDpadRepeat(gb).port_ProcessDPadRepeat(); }
