// overworld/crayon_flags (src/engine/overworld/crayon_flags.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldCrayonFlags : public AsmModule {
public:
	using AsmModule::AsmModule;

	// gets from wTreasuresCollected the crayon treasure flags into wCrayonFlags
	void port_GetCrayonFlags()
	{
		uint16_t hl = wTreasuresCollected + (CRAYON_YELLOW_T / 8);
		cpu_.a = rd8(hl--);
		cpu_.c = rd8(hl);
		cpu_.c = alu_.sla(cpu_.c);
		cpu_.a = alu_.rla(cpu_.a);
		cpu_.c = alu_.sla(cpu_.c);
		cpu_.a = alu_.rla(cpu_.a);
		cpu_.a &= (uint8_t)~0x80; // res 7, a
		wr8(wCrayonFlags, cpu_.a);
		SET_HL(gb_, hl);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldCrayonFlags;

PORT_EX(GetCrayonFlags, 0, 0) { OverworldCrayonFlags(gb).port_GetCrayonFlags(); }
