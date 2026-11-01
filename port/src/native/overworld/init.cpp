// overworld/init (src/engine/overworld/init.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldInit : public AsmModule {
public:
	using AsmModule::AsmModule;

	// map's code (bank20.c, bank2d.c) calls with registers (the rest is C++,
	// src/native/overworld)

	void port_Func_80bd9()
	{
		uint16_t hl = wAttrmap + 14 * TILEMAP_WIDTH;
		wr8(hl, rd8(hl) & ~(1 << B_BG_PRIO));

		// reset BG priority inside a
		// 3x4 rectangle, at coord (0, 15)
		SET_DE(gb_, 15 * TILEMAP_WIDTH);
		cpu_.c = 4;
		for (;;) {
			hl = alu_.add16(wAttrmap, REG_DE(gb_));
			cpu_.b = 3;
			do {
				wr8(hl, rd8(hl) & ~(1 << B_BG_PRIO));
				hl++;
				cpu_.b = alu_.dec(cpu_.b);
			} while (!(cpu_.f & FLAG_Z));
			cpu_.c = alu_.dec(cpu_.c);
			if (cpu_.f & FLAG_Z)
				break;
			hl = alu_.add16(TILEMAP_WIDTH, REG_DE(gb_));
			SET_DE(gb_, hl);
		}
		SET_HL(gb_, hl);
	}

	void port_Func_803e6()
	{
		cpu_.a = rd8(wTransitionParam);
		if (!alu_.and_(cpu_.a, cpu_.a)) {
			RET();
			return;
		}
		cpu_.a = rd8(wOWBarsState);
		if (alu_.and_(cpu_.a, cpu_.a)) {
			RET();
			return;
		}
		cpu_.a = rd8(wMapSideLevelID);
		alu_.cp(cpu_.a, OW_EXITS);
		if (!(cpu_.f & FLAG_C)) {
			RET();
			return;
		}
		JUMP(SaveGameOrEnterPerfectScreen);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldInit;

PORT_EX(Func_80bd9, 0, 0) { OverworldInit(gb).port_Func_80bd9(); }
PORT_EX(Func_803e6, HOOK_MANUAL_RET, 0) { OverworldInit(gb).port_Func_803e6(); }
