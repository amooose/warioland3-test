// overworld/bank2d (src/engine/overworld/bank2d.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldBank2d : public AsmModule {
public:
	using AsmModule::AsmModule;

	// scene_objects, map_functions, scene_init, side_animations).

	// unreferenced
	void port_Func_b4895()
	{
		uint16_t hl = REG_HL(gb_), de = REG_DE(gb_);
		cpu_.a = rd8(de);
		wr8(hl++, cpu_.a);
		de++;
		cpu_.a = rd8(de);
		wr8(hl--, cpu_.a);
		SET_HL(gb_, hl);
		SET_DE(gb_, de);
		CALL(GetOWCoordInTilemap);
		de = (uint16_t)(REG_DE(gb_) + 1);
		SET_DE(gb_, de);
		cpu_.a = rd8(de);
		wr8(REG_HL(gb_), cpu_.a);
		cpu_.h = alu_.dec(cpu_.h);
		cpu_.h = alu_.dec(cpu_.h);
		cpu_.h = alu_.dec(cpu_.h);
		de++;
		SET_DE(gb_, de);
		cpu_.a = rd8(de);
		wr8(REG_HL(gb_), cpu_.a);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldBank2d;

PORT_EX(Func_b4895, 0, 0) { OverworldBank2d(gb).port_Func_b4895(); }
