// pause/bank7d (src/engine/pause/bank7d.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::pause {

namespace {

class PauseBank7d : public AsmModule {
public:
	using AsmModule::AsmModule;

	// menu's are C++, src/native/pause)

	void port_LoadSaveScreenPals()
	{
		SET_HL(gb_, ADDR_Pals_1f628c);
		CALL(LoadPalsToTempPals1);
		SET_HL(gb_, ADDR_Pals_1f628c);
		CALL(LoadPalsToTempPals2);
	}

	// rVBK = 1, decompress attrmap to bc; rVBK = 0, decompress tilemap to bc
	void draw_maps(uint16_t attrmap, uint16_t tilemap, uint16_t dst)
	{
		cpu_.a = BANK_v1Tiles0; // BANK("VRAM1")
		wr8(rVBK, cpu_.a);
		SET_HL(gb_, attrmap);
		SET_BC(gb_, dst);
		CALL(Decompress);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
		SET_HL(gb_, tilemap);
		SET_BC(gb_, dst);
		CALL(Decompress);
	}

	void port_LoadSaveScreenGfx()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
		SET_HL(gb_, ADDR_SaveBoxGfx);
		SET_BC(gb_, v0Tiles0);
		cpu_.a = BANK_SaveBoxGfx;
		wr8(hCallFuncBank, cpu_.a);
		HCALL(Decompress);

		SET_HL(gb_, ADDR_WarioSleepGfx);
		SET_DE(gb_, v0Tiles0);
		SET_BC(gb_, 0x800);
		cpu_.a = BANK_WarioSleepGfx;
		wr8(wTempBank, cpu_.a);
		CALL(FarCopyHLToDE_BC);
	}

	void port_DrawSaveInProgressBox()
	{
		draw_maps(ADDR_SaveInProgressBoxAttrmap, ADDR_SaveInProgressBoxTilemap, v0BGMap0);
	}

	void port_DrawSaveCompleteBox()
	{
		draw_maps(ADDR_SaveCompleteBoxAttrmap, ADDR_SaveCompleteBoxTilemap, v0BGMap0);
	}
};

} // namespace

} // namespace wl3::pause

using wl3::pause::PauseBank7d;

PORT_EX(LoadSaveScreenPals, 0, 0) { PauseBank7d(gb).port_LoadSaveScreenPals(); }
PORT_EX(LoadSaveScreenGfx, 0, 0) { PauseBank7d(gb).port_LoadSaveScreenGfx(); }
PORT_EX(DrawSaveInProgressBox, 0, 0) { PauseBank7d(gb).port_DrawSaveInProgressBox(); }
PORT_EX(DrawSaveCompleteBox, 0, 0) { PauseBank7d(gb).port_DrawSaveCompleteBox(); }
