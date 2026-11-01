// overworld/bank27 (src/engine/overworld/bank27.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldBank27 : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_LoadCutscenes5Gfx_Vram0()
	{
		// decompress_tiles0 Cutscenes5Gfx, v0Tiles2
		wr8(wTempBank, BANK_Cutscenes5Gfx);
		SET_HL(gb_, ADDR_Cutscenes5Gfx);
		SET_BC(gb_, v0Tiles2);
		cpu_.a = rd8(wTempBank);
		wr8(hCallFuncBank, cpu_.a);
		HCALL(Decompress);
	}

	// decompress_tiles0: ld a, BANK(gfx) / ld [wTempBank], a / ld hl, gfx /
	// ld bc, dest / ld a, [wTempBank] / ldh [hCallFuncBank], a / hcall Decompress
	void decompress_tiles0(uint8_t bank, uint16_t gfx, uint16_t dest)
	{
		cpu_.a = bank;
		wr8(wTempBank, bank);
		SET_HL(gb_, gfx);
		SET_BC(gb_, dest);
		cpu_.a = rd8(wTempBank);
		wr8(hCallFuncBank, cpu_.a);
		HCALL(Decompress);
	}

	// decompress_tiles1: the same into VRAM bank 1, then back to bank 0
	void decompress_tiles1(uint8_t bank, uint16_t gfx, uint16_t dest)
	{
		cpu_.a = BANK_v1Tiles0;
		wr8(rVBK, BANK_v1Tiles0);
		decompress_tiles0(bank, gfx, dest);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
	}

	#define CUTSCENE_GFX0(name, gfx, dest) \
		void name() { decompress_tiles0(BANK_##gfx, ADDR_##gfx, dest); }
	#define CUTSCENE_GFX1(name, gfx, dest) \
		void name() { decompress_tiles1(BANK_##gfx, ADDR_##gfx, dest); }

	CUTSCENE_GFX0(cutscenes1_v0, Cutscenes1Gfx, v0Tiles0)
	CUTSCENE_GFX1(cutscenes1_v1, Cutscenes1Gfx, v1Tiles0)
	CUTSCENE_GFX0(cutscenes2_v0, Cutscenes2Gfx, v0Tiles0)
	CUTSCENE_GFX0(cutscenes3_v0, Cutscenes3Gfx, v0Tiles0)
	CUTSCENE_GFX0(cutscenes4_v0, Cutscenes4Gfx, v0Tiles0)
	CUTSCENE_GFX0(cutscenes5_v0, Cutscenes5Gfx, v0Tiles2)
	CUTSCENE_GFX1(cutscenes5_v1, Cutscenes5Gfx, v1Tiles2)
	CUTSCENE_GFX0(cutscenes7_v0, Cutscenes7Gfx, v0Tiles0)
	CUTSCENE_GFX1(cutscenes7_v1, Cutscenes7Gfx, v1Tiles0)
	CUTSCENE_GFX0(cutscenes8_v0, Cutscenes8Gfx, v0Tiles0)
	CUTSCENE_GFX1(cutscenes8_v1, Cutscenes8Gfx, v1Tiles0)
	CUTSCENE_GFX0(cutscenes9_v0, Cutscenes9Gfx, v0Tiles0)
	CUTSCENE_GFX1(cutscenes9_v1, Cutscenes9Gfx, v1Tiles0)
	CUTSCENE_GFX0(cutscenes10_v0, Cutscenes10Gfx, v0Tiles2)
	CUTSCENE_GFX1(cutscenes10_v1, Cutscenes10Gfx, v1Tiles2)

	// The jumps and calls between these loaders stay in bank $27, which
	// Decompress restores, so running the next one natively matches the asm.
	void port_LoadCutscenes1Gfx_Vram0() { cutscenes1_v0(); cutscenes5_v0(); }
	void port_LoadCutscenes1Gfx_Vram1() { cutscenes1_v1(); cutscenes5_v1(); }
	void port_LoadCutscenes2Gfx() { cutscenes2_v0(); cutscenes5_v0(); }
	void port_LoadCutscenes3Gfx() { cutscenes3_v0(); cutscenes5_v0(); }
	void port_LoadCutscenes4Gfx() { cutscenes4_v0(); cutscenes5_v0(); }
	void port_LoadCutscenes5Gfx_Vram1() { cutscenes5_v1(); }
	void port_LoadCutscenes7Gfx_Vram1() { cutscenes7_v1(); cutscenes5_v1(); }
	void port_Func_9c931() { cutscenes7_v0(); cutscenes5_v0(); }
	void port_Func_9c937() { cutscenes7_v0(); cutscenes10_v0(); }
	void port_LoadCutscenes7Gfx_Vram0() { cutscenes7_v0(); }
	void port_LoadCutscenes8Gfx_Vram0() { cutscenes8_v0(); cutscenes5_v0(); }
	void port_Func_9c977() { cutscenes8_v1(); cutscenes5_v1(); }
	void port_Func_9c97d() { cutscenes8_v1(); cutscenes10_v1(); }
	void port_LoadCutscenes8Gfx_Vram1() { cutscenes8_v1(); }
	void port_LoadCutscenes9Gfx_Vram1() { cutscenes9_v1(); cutscenes10_v1(); }
	void port_LoadCutscenes9Gfx_Vram0() { cutscenes9_v0(); cutscenes10_v0(); }
	void port_LoadCutscenes10Gfx_Vram1() { cutscenes10_v1(); }
	void port_LoadCutscenes10Gfx_Vram0() { cutscenes10_v0(); }

	void port_LoadLevelMainTiles29_Vram1()
	{
		cpu_.a = BANK_v1Tiles0;
		wr8(rVBK, cpu_.a);
		cpu_.b = BANK_LevelMainTiles29;
		SET_HL(gb_, ADDR_LevelMainTiles29);
		CALL(LoadFarTiles);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
	}

	void port_LoadLevelMainTiles29_Vram0()
	{
		cpu_.b = BANK_LevelMainTiles29;
		SET_HL(gb_, ADDR_LevelMainTiles29);
		CALL(LoadFarTiles);
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

	void port_Func_9cc87()
	{
		clear_bytes(wTempBGPals, 8 * PAL_SIZE);
		clear_bytes(wTempOBPals, 8 * PAL_SIZE);
	}

	void port_ClearTempPals_Bank27()
	{
		clear_bytes(wTempPals1, 8 * PAL_SIZE);
		clear_bytes(wTempPals2, 8 * PAL_SIZE);
	}

	// ld hl, src / ld de, dest / ld b, n / call CopyHLToDE
	void copy_bytes(uint16_t src, uint16_t dest, uint8_t n)
	{
		SET_HL(gb_, src);
		SET_DE(gb_, dest);
		cpu_.b = n;
		CALL(CopyHLToDE);
	}

	void port_Func_9cd74()
	{
		gb_push16(gb_, REG_HL(gb_)); // push hl
		copy_bytes(wTempPals2, wTempOBPals, 8 * PAL_SIZE);
		copy_bytes(wTempPals1, wTempBGPals, 8 * PAL_SIZE);
		SET_HL(gb_, gb_pop16(gb_)); // pop hl
	}

	void set_top_rows_attr(uint16_t bgmap, uint8_t attr)
	{
		cpu_.a = BANK_v1Tiles0;
		wr8(rVBK, cpu_.a);
		SET_HL(gb_, bgmap);
		SET_BC(gb_, 5 * TILEMAP_WIDTH);
		cpu_.a = attr;
		CALL(WriteAToHL_BCTimes);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
	}

	void port_SetBGPriorityOnTopRows_BGMap1() { set_top_rows_attr(v1BGMap1, BG_BANK0 | BG_PRIO); }
	// d = attributes
	void port_SetBGPriorityOnTopRows() { set_top_rows_attr(v1BGMap0, cpu_.d); }

	// sets BG priority on the 6x7 block at (0, 5) of v1BGMap0
	void port_Func_9cc4f()
	{
		cpu_.a = BANK_v1Tiles0;
		wr8(rVBK, cpu_.a);
		cpu_.d = BG_BANK0 | BG_PRIO;
		uint16_t hl = v1BGMap0 + 5 * TILEMAP_WIDTH;
		cpu_.e = 7;
		for (;;) {
			cpu_.b = 6;
			cpu_.a = cpu_.d;
			do {
				wr8(hl++, cpu_.a);
				cpu_.b = alu_.dec(cpu_.b);
			} while (cpu_.b);
			cpu_.e = alu_.dec(cpu_.e);
			if (!cpu_.e)
				break;
			cpu_.a = alu_.add(TILEMAP_WIDTH - 6, (uint8_t)hl);
			uint8_t l = cpu_.a;
			cpu_.a = alu_.adc(0, (uint8_t)(hl >> 8));
			hl = (uint16_t)(cpu_.a << 8 | l);
		}
		SET_HL(gb_, hl);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
	}

	// Scene object animations. The helpers they use are in this bank, so they
	// are called with gb_call, and the tail jumps (followed by the caller's
	// return) become calls too.

	// hl = compressed BG map, bc = destination
	void port_LoadCutsceneBGMap()
	{
		wr8(wTempBank, BANK_Cutscene01Tilemap); // BANK("Cutscene BG Maps")
		cpu_.a = rd8(wTempBank);
		wr8(hCallFuncBank, cpu_.a);
		HCALL(Decompress);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
	}

	void port_PrologueFunc()
	{
		FARCALL(Prologue);
	}

	void port_LoadCutsceneTilemap0()
	{
		SET_BC(gb_, v0BGMap0);
		JUMP_TO(ADDR_LoadCutsceneBGMap); // fallthrough
	}

	void port_LoadCutsceneTilemap1()
	{
		SET_BC(gb_, v0BGMap1);
		JUMP_TO(ADDR_LoadCutsceneBGMap);
	}

	void port_VBlank_Cutscene()
	{
		SET_HL(gb_, ADDR_VBlank_Cutscene + 0x0c); // .Func
		SET_DE(gb_, wVBlankFunc);
		cpu_.b = 0x51; // .end - .Func
		CALL(CopyHLToDE);
	}

	// loads the attrmap at `attrmap` and the tilemap at `tilemap`
	void load_cutscene_bg_maps(uint16_t attrmap, uint16_t load_attrmap, uint16_t tilemap, uint16_t load_tilemap)
	{
		SET_HL(gb_, attrmap);
		gb_call(gb_, load_attrmap);
		SET_HL(gb_, tilemap);
		gb_call(gb_, load_tilemap);
	}

	void port_LoadCutsceneBGMaps01()
	{
		load_cutscene_bg_maps(ADDR_Cutscene01Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene01Tilemap, ADDR_LoadCutsceneTilemap0);
	}

	void port_LoadCutsceneBGMaps02()
	{
		load_cutscene_bg_maps(ADDR_Cutscene02Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene02Tilemap, ADDR_LoadCutsceneTilemap1);
	}

	void port_LoadCutsceneBGMaps07()
	{
		load_cutscene_bg_maps(ADDR_Cutscene07Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene07Tilemap, ADDR_LoadCutsceneTilemap0);
	}

	void port_LoadCutsceneAttrmap0()
	{
		cpu_.a = 1; // BANK("VRAM1")
		wr8(rVBK, 1);
		JUMP_TO(ADDR_LoadCutsceneTilemap0); // fallthrough
	}

	void port_LoadCutsceneAttrmap1()
	{
		cpu_.a = 1; // BANK("VRAM1")
		wr8(rVBK, 1);
		JUMP_TO(ADDR_LoadCutsceneTilemap1); // fallthrough
	}

	// --- Cutscene 04 and small helpers ---

	void port_LoadCutsceneBGMaps03()
	{
		load_cutscene_bg_maps(ADDR_Cutscene03Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene03Tilemap, ADDR_LoadCutsceneTilemap0);
	}

	void port_SetBGPriorityOnTopRows_Vram1()
	{
		cpu_.d = BG_BANK1 | BG_PRIO;
		JUMP_TO(ADDR_SetBGPriorityOnTopRows);
	}

	void port_SetBGPriorityOnTopRows_Vram0()
	{
		cpu_.d = BG_BANK0 | BG_PRIO;
		JUMP_TO(ADDR_SetBGPriorityOnTopRows); // fallthrough
	}

	// --- the remaining LoadCutsceneBGMapsNN ---
	void port_LoadCutsceneBGMaps12() { load_cutscene_bg_maps(ADDR_Cutscene12Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene12Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps13() { load_cutscene_bg_maps(ADDR_Cutscene13Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene13Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps15() { load_cutscene_bg_maps(ADDR_Cutscene15Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene15Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps16() { load_cutscene_bg_maps(ADDR_Cutscene16Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene16Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps17() { load_cutscene_bg_maps(ADDR_Cutscene17Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene17Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps18() { load_cutscene_bg_maps(ADDR_Cutscene18Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene18Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps20() { load_cutscene_bg_maps(ADDR_Cutscene20Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene20Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps22() { load_cutscene_bg_maps(ADDR_Cutscene22Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene22Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps23() { load_cutscene_bg_maps(ADDR_Cutscene23Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene23Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps24() { load_cutscene_bg_maps(ADDR_Cutscene24Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene24Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps26() { load_cutscene_bg_maps(ADDR_Cutscene26Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene26Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps27() { load_cutscene_bg_maps(ADDR_Cutscene27Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene27Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps28() { load_cutscene_bg_maps(ADDR_Cutscene28Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene28Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps29() { load_cutscene_bg_maps(ADDR_Cutscene29Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene29Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps30() { load_cutscene_bg_maps(ADDR_Cutscene30Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene30Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps31() { load_cutscene_bg_maps(ADDR_Cutscene31Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene31Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps32() { load_cutscene_bg_maps(ADDR_Cutscene32Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene32Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps34() { load_cutscene_bg_maps(ADDR_Cutscene34Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene34Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps36() { load_cutscene_bg_maps(ADDR_Cutscene36Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene36Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps37() { load_cutscene_bg_maps(ADDR_Cutscene37Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene37Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps39() { load_cutscene_bg_maps(ADDR_Cutscene39Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene39Tilemap, ADDR_LoadCutsceneTilemap0); }
	void port_LoadCutsceneBGMaps04() { load_cutscene_bg_maps(ADDR_Cutscene04Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene04Tilemap, ADDR_LoadCutsceneTilemap1); }
	void port_LoadCutsceneBGMaps05() { load_cutscene_bg_maps(ADDR_Cutscene05Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene05Tilemap, ADDR_LoadCutsceneTilemap1); }
	void port_LoadCutsceneBGMaps14() { load_cutscene_bg_maps(ADDR_Cutscene14Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene14Tilemap, ADDR_LoadCutsceneTilemap1); }
	void port_LoadCutsceneBGMaps19() { load_cutscene_bg_maps(ADDR_Cutscene19Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene19Tilemap, ADDR_LoadCutsceneTilemap1); }
	void port_LoadCutsceneBGMaps21() { load_cutscene_bg_maps(ADDR_Cutscene21Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene21Tilemap, ADDR_LoadCutsceneTilemap1); }
	void port_LoadCutsceneBGMaps25() { load_cutscene_bg_maps(ADDR_Cutscene25Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene25Tilemap, ADDR_LoadCutsceneTilemap1); }
	void port_LoadCutsceneBGMaps35() { load_cutscene_bg_maps(ADDR_Cutscene35Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene35Tilemap, ADDR_LoadCutsceneTilemap1); }
	void port_LoadCutsceneBGMaps38() { load_cutscene_bg_maps(ADDR_Cutscene38Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene38Tilemap, ADDR_LoadCutsceneTilemap1); }
	void port_LoadCutsceneBGMaps040() { load_cutscene_bg_maps(ADDR_Cutscene40Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene40Tilemap, ADDR_LoadCutsceneTilemap1); }

	// --- more scene object animations ---

	// ---- InitCutsceneNN ----

	// ld b, BANK(pals) / ld hl, pals / call LoadFarPalsToTempPals1 (or 2)
	#define PALS1(p) do { cpu_.b = BANK_##p; SET_HL(gb_, ADDR_##p); CALL(LoadFarPalsToTempPals1); } while (0)
	#define PALS2(p) do { cpu_.b = BANK_##p; SET_HL(gb_, ADDR_##p); CALL(LoadFarPalsToTempPals2); } while (0)
	// ld b, BANK(tiles) / ld hl, tiles / call LoadFarTiles
	#define FAR_TILES(t) do { cpu_.b = BANK_##t; SET_HL(gb_, ADDR_##t); CALL(LoadFarTiles); } while (0)
	// the same into VRAM bank 1
	#define FAR_TILES1(t) do { \
		cpu_.a = 1; wr8(rVBK, 1); \
		FAR_TILES(t); \
		cpu_.a = 0; cpu_.f = FLAG_Z; wr8(rVBK, 0); \
	} while (0)
	// decompress_attrmap / decompress_tilemap
	#define ATTRMAP(m, dest) decompress_tiles1(BANK_##m, ADDR_##m, dest)
	#define TILEMAP(m, dest) decompress_tiles0(BANK_##m, ADDR_##m, dest)

	// ---- cutscene scene object setups ----


	// Cutscene state machines: CutsceneNNFunc calls its local dispatcher (whose
	// rst JumpTable table follows at +$a), then jumps to its animation routine.

	// Cutscene06Func

	// Cutscene0aFunc

	// Cutscene27Func

	// Cutscene0cFunc

	// Cutscene18Func

	// Cutscene0dFunc

	// Cutscene19Func

	// Cutscene13Func

	// --- batch 6: more cutscene state machines ---

	// Cutscene07Func

	// Cutscene11Func (also Cutscene0eFunc)

	// Cutscene1bFunc

	// Cutscene09Func

	// Func_9d6f0: b = scene obj 2's first state

	// Cutscene24Func

	// Cutscene1fFunc

	// Cutscene21Func

	// Cutscene37Func

	// Cutscene38Func

	// Cutscene30Func

	// Cutscene39Func

	// call Func_9cc87 / load pals into TempPals1 and TempPals2 / jp AdvanceCutsceneFunc
	#define CLEAR_LOAD_PALS_ADVANCE(p) do { \
		gb_call(gb_, ADDR_Func_9cc87); \
		PALS1(p); \
		PALS2(p); \
		advance(); \
	} while (0)

	// Cutscene3aFunc

	// Cutscene3cFunc

	// Cutscene43Func

	// Cutscene54Func

	// Cutscene3dFunc

	// Cutscene4aFunc

	// Cutscene4cFunc

	// Cutscene4eFunc

	// Cutscene4fFunc

	// Cutscene51Func

	// Cutscene56Func

	// Cutscene57Func

	// Cutscene58Func

	// Cutscene49Func

	// Cutscene55Func

	// Cutscene44Func

	// Cutscene14Func



	// ld a, [wROMBank] / push af / ld a, b / bankswitch
	void vbf_push_bank_switch()
	{
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = cpu_.b;
		bankswitch(cpu_.a);
	}

	// pop af / bankswitch
	void vbf_pop_bank()
	{
		pop_af();
		bankswitch(cpu_.a);
	}

	// ld a, [wLCDCFlagsToFlip] / and a / jr z, .skip / ld hl, rLCDC / xor [hl] /
	// ld [hl], a / xor a / ld [wLCDCFlagsToFlip], a
	void vbf_flip_lcdc()
	{
		cpu_.a = rd8(wLCDCFlagsToFlip);
		cpu_.a = alu_.and_(cpu_.a, cpu_.a);
		if (!cpu_.a)
			return;
		SET_HL(gb_, rLCDC);
		cpu_.a = alu_.xor_(cpu_.a, rd8(rLCDC));
		wr8(rLCDC, cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wLCDCFlagsToFlip, 0);
	}

	// ld a, $02 / ldh [rWBK], a / apply wSCY and wSCX
	void vbf_wram2_scroll()
	{
		cpu_.a = 0x02; // BANK("WRAM2")
		wr8(rWBK, cpu_.a);
		wr8(rSCY, rd8(wSCY));
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
	}

	// VBlank_Cutscene.Func, as copied to wVBlankFunc (see ram_code.c). Its calls
	// return to base + n, the block's start wherever it runs from.
	void port_VBlank_Cutscene__Func()
	{
		uint16_t base = cpu_.pc;
		vbf_wram2_scroll();
		wr8(rWY, rd8(wCutsceneWY));
		cpu_.a = rd8(wCutsceneWX);
		wr8(rWX, cpu_.a);

		vbf_flip_lcdc();

		// .skip_lcd_flip
		cpu_.b = BANK_ApplyPalConfig1;
		vbf_push_bank_switch();
		cpu_.a = alu_.and_(rd8(wPalConfig1Register), rd8(wPalConfig1Register));
		if (cpu_.a)
			gb_call_ret(gb_, ADDR_ApplyPalConfig1, (uint16_t)(base + 0x3b));
		cpu_.a = alu_.and_(rd8(wPalConfig2), rd8(wPalConfig2));
		if (cpu_.a)
			gb_call_ret(gb_, ADDR_ApplyPalConfig2, (uint16_t)(base + 0x42));
		vbf_pop_bank();

		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		JUMP_TO(hTransferVirtualOAM);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldBank27;

PORT_EX(LoadCutscenes5Gfx_Vram0, 0, 0) { OverworldBank27(gb).port_LoadCutscenes5Gfx_Vram0(); }
PORT_EX(LoadCutscenes1Gfx_Vram0, 0, 0) { OverworldBank27(gb).port_LoadCutscenes1Gfx_Vram0(); }
PORT_EX(LoadCutscenes1Gfx_Vram1, 0, 0) { OverworldBank27(gb).port_LoadCutscenes1Gfx_Vram1(); }
PORT_EX(LoadCutscenes2Gfx, 0, 0) { OverworldBank27(gb).port_LoadCutscenes2Gfx(); }
PORT_EX(LoadCutscenes3Gfx, 0, 0) { OverworldBank27(gb).port_LoadCutscenes3Gfx(); }
PORT_EX(LoadCutscenes4Gfx, 0, 0) { OverworldBank27(gb).port_LoadCutscenes4Gfx(); }
PORT_EX(LoadCutscenes5Gfx_Vram1, 0, 0) { OverworldBank27(gb).port_LoadCutscenes5Gfx_Vram1(); }
PORT_EX(LoadCutscenes7Gfx_Vram1, 0, 0) { OverworldBank27(gb).port_LoadCutscenes7Gfx_Vram1(); }
PORT_EX(Func_9c931, 0, 0) { OverworldBank27(gb).port_Func_9c931(); }
PORT_EX(Func_9c937, 0, 0) { OverworldBank27(gb).port_Func_9c937(); }
PORT_EX(LoadCutscenes7Gfx_Vram0, 0, 0) { OverworldBank27(gb).port_LoadCutscenes7Gfx_Vram0(); }
PORT_EX(LoadCutscenes8Gfx_Vram0, 0, 0) { OverworldBank27(gb).port_LoadCutscenes8Gfx_Vram0(); }
PORT_EX(Func_9c977, 0, 0) { OverworldBank27(gb).port_Func_9c977(); }
PORT_EX(Func_9c97d, 0, 0) { OverworldBank27(gb).port_Func_9c97d(); }
PORT_EX(LoadCutscenes8Gfx_Vram1, 0, 0) { OverworldBank27(gb).port_LoadCutscenes8Gfx_Vram1(); }
PORT_EX(LoadCutscenes9Gfx_Vram1, 0, 0) { OverworldBank27(gb).port_LoadCutscenes9Gfx_Vram1(); }
PORT_EX(LoadCutscenes9Gfx_Vram0, 0, 0) { OverworldBank27(gb).port_LoadCutscenes9Gfx_Vram0(); }
PORT_EX(LoadCutscenes10Gfx_Vram1, 0, 0) { OverworldBank27(gb).port_LoadCutscenes10Gfx_Vram1(); }
PORT_EX(LoadCutscenes10Gfx_Vram0, 0, 0) { OverworldBank27(gb).port_LoadCutscenes10Gfx_Vram0(); }
PORT_EX(LoadLevelMainTiles29_Vram1, 0, 0) { OverworldBank27(gb).port_LoadLevelMainTiles29_Vram1(); }
PORT_EX(LoadLevelMainTiles29_Vram0, 0, 0) { OverworldBank27(gb).port_LoadLevelMainTiles29_Vram0(); }
PORT_EX(Func_9cc87, 0, 0) { OverworldBank27(gb).port_Func_9cc87(); }
PORT_EX(ClearTempPals_Bank27, 0, 0) { OverworldBank27(gb).port_ClearTempPals_Bank27(); }
PORT_EX(Func_9cd74, 0, 0) { OverworldBank27(gb).port_Func_9cd74(); }
PORT_EX(SetBGPriorityOnTopRows_BGMap1, 0, 0) { OverworldBank27(gb).port_SetBGPriorityOnTopRows_BGMap1(); }
PORT_EX(SetBGPriorityOnTopRows, 0, 0) { OverworldBank27(gb).port_SetBGPriorityOnTopRows(); }
PORT_EX(Func_9cc4f, 0, 0) { OverworldBank27(gb).port_Func_9cc4f(); }
PORT_EX(LoadCutsceneBGMap, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMap(); }
PORT_EX(PrologueFunc, 0, 0) { OverworldBank27(gb).port_PrologueFunc(); }
PORT_EX(LoadCutsceneTilemap0, HOOK_MANUAL_RET, 0) { OverworldBank27(gb).port_LoadCutsceneTilemap0(); }
PORT_EX(LoadCutsceneTilemap1, HOOK_MANUAL_RET, 0) { OverworldBank27(gb).port_LoadCutsceneTilemap1(); }
PORT_EX(VBlank_Cutscene, 0, 0) { OverworldBank27(gb).port_VBlank_Cutscene(); }
PORT_EX(LoadCutsceneBGMaps01, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps01(); }
PORT_EX(LoadCutsceneBGMaps02, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps02(); }
PORT_EX(LoadCutsceneBGMaps07, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps07(); }
PORT_EX(LoadCutsceneAttrmap0, HOOK_MANUAL_RET, 0) { OverworldBank27(gb).port_LoadCutsceneAttrmap0(); }
PORT_EX(LoadCutsceneAttrmap1, HOOK_MANUAL_RET, 0) { OverworldBank27(gb).port_LoadCutsceneAttrmap1(); }
PORT_EX(LoadCutsceneBGMaps03, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps03(); }
PORT_EX(SetBGPriorityOnTopRows_Vram1, HOOK_MANUAL_RET, 0) { OverworldBank27(gb).port_SetBGPriorityOnTopRows_Vram1(); }
PORT_EX(SetBGPriorityOnTopRows_Vram0, HOOK_MANUAL_RET, 0) { OverworldBank27(gb).port_SetBGPriorityOnTopRows_Vram0(); }
PORT_EX(LoadCutsceneBGMaps12, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps12(); }
PORT_EX(LoadCutsceneBGMaps13, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps13(); }
PORT_EX(LoadCutsceneBGMaps15, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps15(); }
PORT_EX(LoadCutsceneBGMaps16, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps16(); }
PORT_EX(LoadCutsceneBGMaps17, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps17(); }
PORT_EX(LoadCutsceneBGMaps18, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps18(); }
PORT_EX(LoadCutsceneBGMaps20, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps20(); }
PORT_EX(LoadCutsceneBGMaps22, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps22(); }
PORT_EX(LoadCutsceneBGMaps23, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps23(); }
PORT_EX(LoadCutsceneBGMaps24, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps24(); }
PORT_EX(LoadCutsceneBGMaps26, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps26(); }
PORT_EX(LoadCutsceneBGMaps27, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps27(); }
PORT_EX(LoadCutsceneBGMaps28, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps28(); }
PORT_EX(LoadCutsceneBGMaps29, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps29(); }
PORT_EX(LoadCutsceneBGMaps30, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps30(); }
PORT_EX(LoadCutsceneBGMaps31, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps31(); }
PORT_EX(LoadCutsceneBGMaps32, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps32(); }
PORT_EX(LoadCutsceneBGMaps34, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps34(); }
PORT_EX(LoadCutsceneBGMaps36, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps36(); }
PORT_EX(LoadCutsceneBGMaps37, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps37(); }
PORT_EX(LoadCutsceneBGMaps39, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps39(); }
PORT_EX(LoadCutsceneBGMaps04, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps04(); }
PORT_EX(LoadCutsceneBGMaps05, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps05(); }
PORT_EX(LoadCutsceneBGMaps14, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps14(); }
PORT_EX(LoadCutsceneBGMaps19, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps19(); }
PORT_EX(LoadCutsceneBGMaps21, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps21(); }
PORT_EX(LoadCutsceneBGMaps25, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps25(); }
PORT_EX(LoadCutsceneBGMaps35, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps35(); }
PORT_EX(LoadCutsceneBGMaps38, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps38(); }
PORT_EX(LoadCutsceneBGMaps040, 0, 0) { OverworldBank27(gb).port_LoadCutsceneBGMaps040(); }
PORT_LOCAL_EX(VBlank_Cutscene, Func, HOOK_MANUAL_RET, 0) { OverworldBank27(gb).port_VBlank_Cutscene__Func(); }
