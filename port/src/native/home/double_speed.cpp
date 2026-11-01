// home/double_speed (src/home/double_speed.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeDoubleSpeed : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_EnableDoubleSpeed()
	{
		cpu_.a = rd8(rSPD);
		alu_.bit(cpu_.a, 7);
		if (!(cpu_.f & FLAG_Z))
			return; // already in double speed

		cpu_.a = SPD_PREPARE;
		wr8(rSPD, cpu_.a);
		cpu_.a = rd8(rIE);
		push_af();
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a: disable all interrupts
		wr8(rIE, 0);
		cpu_.a = JOYP_GET_NONE;
		wr8(rJOYP, cpu_.a);
		// stop: with the switch armed, it changes the CPU speed
		if (cpu_.speed_switch_armed) {
			cpu_.double_speed = !cpu_.double_speed;
			cpu_.speed_switch_armed = false;
		}
		gb_->div_counter = 0;

		do { // .wait
			cpu_.a = rd8(rSPD);
			alu_.bit(cpu_.a, 7);
		} while (cpu_.f & FLAG_Z);

		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rJOYP, 0);
		wr8(rIF, 0);
		pop_af();
		wr8(rIE, cpu_.a);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeDoubleSpeed;

PORT_EX(EnableDoubleSpeed, 0, 0) { HomeDoubleSpeed(gb).port_EnableDoubleSpeed(); }
