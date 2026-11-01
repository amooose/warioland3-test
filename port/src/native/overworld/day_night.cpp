// overworld/day_night (src/engine/overworld/day_night.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldDayNight : public AsmModule {
public:
	using AsmModule::AsmModule;

	// flips bit 4 of wDayNight
	void port_SwitchDayNight()
	{
		SET_HL(gb_, wDayNight);
		cpu_.a = alu_.xor_(1 << 4, rd8(wDayNight));
		wr8(wDayNight, cpu_.a);
	}

	// switches day/night, except on the East side without the Sun Medallion
	void port_DoDayNightSpell()
	{
		cpu_.a = rd8(wCurMapSide);
		alu_.cp(cpu_.a, EAST);
		if (cpu_.f & FLAG_Z) {
			cpu_.a = rd8(wGotSunMedallion);
			cpu_.a = alu_.and_(cpu_.a, cpu_.a);
			if (!cpu_.a) {
				SET_HL(gb_, wDayNight);
				wr8(wDayNight, (uint8_t)(rd8(wDayNight) & ~(1 << 4)));
				RET();
				return;
			}
		}
		JUMP_TO(ADDR_SwitchDayNight);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldDayNight;

PORT_EX(SwitchDayNight, 0, 0) { OverworldDayNight(gb).port_SwitchDayNight(); }
PORT_EX(DoDayNightSpell, HOOK_MANUAL_RET, 0) { OverworldDayNight(gb).port_DoDayNightSpell(); }
