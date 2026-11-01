// engine/load_font (src/engine/load_font.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::engine {

namespace {

class EngineLoadFont : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_LoadFontTiles()
	{
		SET_HL(gb_, ADDR_FontGfx);
		SET_BC(gb_, v0Tiles0);
		CALL(Decompress);
	}

	void port_LoadFontPals()
	{
		SET_HL(gb_, ADDR_Pals_1f3610);
		CALL(LoadPalsToTempPals1);
		SET_HL(gb_, ADDR_Pals_1f3610);
		CALL(LoadPalsToTempPals2);
	}

	void port_LoadLanguageSelectionText()
	{
		cpu_.a = BANK_v1BGMap0; // BANK("VRAM1")
		wr8(rVBK, cpu_.a);
		SET_HL(gb_, ADDR_LanguageSelection2TextMap);
		SET_BC(gb_, v1BGMap0);
		CALL(Decompress);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
		SET_HL(gb_, ADDR_LanguageSelection1TextMap);
		SET_BC(gb_, v0BGMap0);
		CALL(Decompress);
	}

	void port_LoadGBIncompatibleScreenGfx()
	{
		SET_HL(gb_, ADDR_GBIncompatibleGfx);
		SET_BC(gb_, v0Tiles0);
		CALL(Decompress);
	}

	void port_LoadGBIncompatibleScreenMap()
	{
		SET_HL(gb_, ADDR_GBIncompatibleTilemap);
		SET_BC(gb_, v0BGMap0);
		CALL(Decompress);
		cpu_.a = 0xe4; // %11100100
		wr8(rBGP, cpu_.a);
	}
};

} // namespace

} // namespace wl3::engine

using wl3::engine::EngineLoadFont;

PORT_EX(LoadFontTiles, 0, 0) { EngineLoadFont(gb).port_LoadFontTiles(); }
PORT_EX(LoadFontPals, 0, 0) { EngineLoadFont(gb).port_LoadFontPals(); }
PORT_EX(LoadLanguageSelectionText, 0, 0) { EngineLoadFont(gb).port_LoadLanguageSelectionText(); }
PORT_EX(LoadGBIncompatibleScreenGfx, 0, 0) { EngineLoadFont(gb).port_LoadGBIncompatibleScreenGfx(); }
PORT_EX(LoadGBIncompatibleScreenMap, 0, 0) { EngineLoadFont(gb).port_LoadGBIncompatibleScreenMap(); }
