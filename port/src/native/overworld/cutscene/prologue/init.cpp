// overworld/cutscene/prologue/init (src/engine/overworld/cutscene/prologue/init.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../../../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldCutscenePrologueInit : public AsmModule {
public:
	using AsmModule::AsmModule;

	// decompress_tilemap / decompress_tiles0: decompresses src (in bank) to dest
	void decompress(uint8_t bank, uint16_t src, uint16_t dest)
	{
		cpu_.a = bank;
		wr8(wTempBank, bank);
		SET_HL(gb_, src);
		SET_BC(gb_, dest);
		cpu_.a = rd8(wTempBank);
		wr8(hCallFuncBank, cpu_.a);
		HCALL(Decompress);
	}

	// decompress_attrmap / decompress_tiles1: the same, into VRAM bank 1
	void decompress_vram1(uint8_t bank, uint16_t src, uint16_t dest)
	{
		cpu_.a = 1; // BANK("VRAM1")
		wr8(rVBK, 1);
		decompress(bank, src, dest);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
	}

	// xor a / ld hl, start / ld bc, size / call WriteAToHL_BCTimes
	void clear_bytes(uint16_t start, uint16_t size)
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		SET_HL(gb_, start);
		SET_BC(gb_, size);
		CALL(WriteAToHL_BCTimes);
	}

	void port__InitPrologue()
	{
		// clear RAM
		gb_call(gb_, ADDR_ClearGeneralSceneRAM);
		gb_call(gb_, ADDR_ClearSceneObjsRAM);

		// set V-Blank handler
		gb_call(gb_, ADDR_VBlank_PrologueEpilogue);

		// load BG maps for the prologue
		decompress_vram1(BANK_PrologueAttrmap, ADDR_PrologueAttrmap, v1BGMap1);
		decompress(BANK_PrologueTilemap, ADDR_PrologueTilemap, v0BGMap1);

		// load gfx
		SET_HL(gb_, ADDR_PrologueBackgroundGfx);
		cpu_.b = BANK_PrologueBackgroundGfx;
		CALL(LoadFarTiles);

		decompress_vram1(BANK_FontGfx, ADDR_FontGfx, v1Tiles0);
		decompress_vram1(BANK_PrologueGfx, ADDR_PrologueGfx, v1Tiles0);

		// load BG maps for the Music Box scene
		decompress_vram1(BANK_MusicBoxAttrmap, ADDR_MusicBoxAttrmap, v1BGMap0);
		decompress(BANK_MusicBoxTilemap, ADDR_MusicBoxTilemap, v0BGMap0);

		// load gfx
		SET_HL(gb_, ADDR_MusicBoxGfx);
		SET_DE(gb_, v0Tiles1);
		cpu_.b = BANK_MusicBoxGfx;
		cpu_.c = (0x30 * TILE_SIZE) >> 8; // HIGH($30 tiles)
		CALL(CopyFarBytes_Long);

		// load the prologue text to buffer
		cpu_.a = alu_.and_(rd8(wLanguage), rd8(wLanguage));
		if (!(cpu_.f & FLAG_Z))
			decompress(BANK_TextEN_HiddenFigureAreYouAware, ADDR_TextEN_HiddenFigureAreYouAware, wTextBuffer);
		else
			decompress(BANK_TextJP_HiddenFigureAreYouAware, ADDR_TextJP_HiddenFigureAreYouAware, wTextBuffer);
		gb_call(gb_, ADDR_FillClearedTextBuffer);

		// clear temp pals
		clear_bytes(wTempPals1, 8 * PAL_SIZE);
		clear_bytes(wTempPals2, 8 * PAL_SIZE);

		// init scene Wario object
		cpu_.a = 96;
		wr8(wSceneWarioYCoord + 0, 96); // y
		wr8(wSceneWarioYCoord + 1, 96); // x
		SET_HL(gb_, wSceneWarioYCoord + 2);
		cpu_.a = SCENEWARIO_GROUP_NONE;
		wr8(wSceneWarioStateGroup, cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a (SCENEWARIO_NONE)
		gb_call(gb_, ADDR_SetSceneWarioState);

		cpu_.a = LCDC_BG_MAP;
		wr8(wLCDCFlagsToFlip, cpu_.a);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldCutscenePrologueInit;

PORT_EX(_InitPrologue, 0, 0) { OverworldCutscenePrologueInit(gb).port__InitPrologue(); }
