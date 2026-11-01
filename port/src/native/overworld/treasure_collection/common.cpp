// overworld/treasure_collection/common (src/engine/overworld/treasure_collection/common.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldTreasureCollectionCommon : public AsmModule {
public:
	using AsmModule::AsmModule;

	// src/native/overworld/treasure_collection.cpp)

	// de = scene obj
	// c = level index in wCurMapSide
	void port_SetObjPositionToLevelCoords()
	{
		cpu_.a = cpu_.c;
		gb_push16(gb_, REG_DE(gb_));
		gb_call(gb_, ADDR_GetMapLevelCoords);
		SET_DE(gb_, gb_pop16(gb_));
		uint16_t hl = REG_HL(gb_);
		cpu_.a = rd8(hl++);
		wr8(REG_DE(gb_), cpu_.a);
		cpu_.e = alu_.inc(cpu_.e);
		cpu_.a = rd8(hl);
		wr8(REG_DE(gb_), cpu_.a);
		SET_HL(gb_, hl);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldTreasureCollectionCommon;

PORT_EX(SetObjPositionToLevelCoords, 0, 0) { OverworldTreasureCollectionCommon(gb).port_SetObjPositionToLevelCoords(); }
