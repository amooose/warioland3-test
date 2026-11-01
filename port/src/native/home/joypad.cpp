// home/joypad (src/home/joypad.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeJoypad : public AsmModule {
public:
	using AsmModule::AsmModule;

	// Read the joypad register and translate it to something more workable for
	// use in-game: one byte with the d-pad in the high nybble and the buttons in
	// the low nybble.
	void port_UpdateJoypad()
	{
		wr8(rJOYP, JOYP_GET_CTRL_PAD);
		uint8_t dpad = (uint8_t)(~rd8(rJOYP) & 0x0f);
		cpu_.b = (uint8_t)(dpad << 4);

		wr8(rJOYP, JOYP_GET_BUTTONS);
		uint8_t buttons = (uint8_t)(~rd8(rJOYP) & 0x0f);
		cpu_.c = cpu_.b | buttons;

		// only bits newly set this frame count as pressed
		uint8_t pressed = (rd8(wJoypadDown) ^ cpu_.c) & cpu_.c;
		wr8(wJoypadPressed, pressed);
		wr8(wJoypadDown, cpu_.c);

		wr8(rJOYP, JOYP_GET_NONE);
		cpu_.a = JOYP_GET_NONE;
		// flags from the final and c
		cpu_.f = (uint8_t)(FLAG_H | (pressed ? 0 : FLAG_Z));
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeJoypad;

PORT_EX(UpdateJoypad, 0, 0) { HomeJoypad(gb).port_UpdateJoypad(); }
