// Port of src/engine/overworld/bank27.asm
#include "ported.h"

PORT(LoadCutscenes5Gfx_Vram0)
{
	// decompress_tiles0 Cutscenes5Gfx, v0Tiles2
	wr8(gb, wTempBank, BANK_Cutscenes5Gfx);
	SET_HL(gb, ADDR_Cutscenes5Gfx);
	SET_BC(gb, v0Tiles2);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
}

// decompress_tiles0: ld a, BANK(gfx) / ld [wTempBank], a / ld hl, gfx /
// ld bc, dest / ld a, [wTempBank] / ldh [hCallFuncBank], a / hcall Decompress
static void decompress_tiles0(GB *gb, uint8_t bank, uint16_t gfx, uint16_t dest)
{
	gb->cpu.a = bank;
	wr8(gb, wTempBank, bank);
	SET_HL(gb, gfx);
	SET_BC(gb, dest);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
}

// decompress_tiles1: the same into VRAM bank 1, then back to bank 0
static void decompress_tiles1(GB *gb, uint8_t bank, uint16_t gfx, uint16_t dest)
{
	gb->cpu.a = BANK_v1Tiles0;
	wr8(gb, rVBK, BANK_v1Tiles0);
	decompress_tiles0(gb, bank, gfx, dest);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

#define CUTSCENE_GFX0(name, gfx, dest) \
	static void name(GB *gb) { decompress_tiles0(gb, BANK_##gfx, ADDR_##gfx, dest); }
#define CUTSCENE_GFX1(name, gfx, dest) \
	static void name(GB *gb) { decompress_tiles1(gb, BANK_##gfx, ADDR_##gfx, dest); }

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
PORT(LoadCutscenes1Gfx_Vram0) { cutscenes1_v0(gb); cutscenes5_v0(gb); }
PORT(LoadCutscenes1Gfx_Vram1) { cutscenes1_v1(gb); cutscenes5_v1(gb); }
PORT(LoadCutscenes2Gfx) { cutscenes2_v0(gb); cutscenes5_v0(gb); }
PORT(LoadCutscenes3Gfx) { cutscenes3_v0(gb); cutscenes5_v0(gb); }
PORT(LoadCutscenes4Gfx) { cutscenes4_v0(gb); cutscenes5_v0(gb); }
PORT(LoadCutscenes5Gfx_Vram1) { cutscenes5_v1(gb); }
PORT(LoadCutscenes7Gfx_Vram1) { cutscenes7_v1(gb); cutscenes5_v1(gb); }
PORT(Func_9c931) { cutscenes7_v0(gb); cutscenes5_v0(gb); }
PORT(Func_9c937) { cutscenes7_v0(gb); cutscenes10_v0(gb); }
PORT(LoadCutscenes7Gfx_Vram0) { cutscenes7_v0(gb); }
PORT(LoadCutscenes8Gfx_Vram0) { cutscenes8_v0(gb); cutscenes5_v0(gb); }
PORT(Func_9c977) { cutscenes8_v1(gb); cutscenes5_v1(gb); }
PORT(Func_9c97d) { cutscenes8_v1(gb); cutscenes10_v1(gb); }
PORT(LoadCutscenes8Gfx_Vram1) { cutscenes8_v1(gb); }
PORT(LoadCutscenes9Gfx_Vram1) { cutscenes9_v1(gb); cutscenes10_v1(gb); }
PORT(LoadCutscenes9Gfx_Vram0) { cutscenes9_v0(gb); cutscenes10_v0(gb); }
PORT(LoadCutscenes10Gfx_Vram1) { cutscenes10_v1(gb); }
PORT(LoadCutscenes10Gfx_Vram0) { cutscenes10_v0(gb); }

PORT(LoadLevelMainTiles29_Vram1)
{
	gb->cpu.a = BANK_v1Tiles0;
	wr8(gb, rVBK, gb->cpu.a);
	gb->cpu.b = BANK_LevelMainTiles29;
	SET_HL(gb, ADDR_LevelMainTiles29);
	CALL(LoadFarTiles);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

PORT(LoadLevelMainTiles29_Vram0)
{
	gb->cpu.b = BANK_LevelMainTiles29;
	SET_HL(gb, ADDR_LevelMainTiles29);
	CALL(LoadFarTiles);
}

// xor a / ld hl, start / ld bc, size / call WriteAToHL_BCTimes
static void clear_bytes(GB *gb, uint16_t start, uint16_t size)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, start);
	SET_BC(gb, size);
	CALL(WriteAToHL_BCTimes);
}

PORT(Func_9cc87)
{
	clear_bytes(gb, wTempBGPals, 8 * PAL_SIZE);
	clear_bytes(gb, wTempOBPals, 8 * PAL_SIZE);
}

PORT(ClearTempPals_Bank27)
{
	clear_bytes(gb, wTempPals1, 8 * PAL_SIZE);
	clear_bytes(gb, wTempPals2, 8 * PAL_SIZE);
}

// ld hl, src / ld de, dest / ld b, n / call CopyHLToDE
static void copy_bytes(GB *gb, uint16_t src, uint16_t dest, uint8_t n)
{
	SET_HL(gb, src);
	SET_DE(gb, dest);
	gb->cpu.b = n;
	CALL(CopyHLToDE);
}

PORT(Func_9cd74)
{
	gb_push16(gb, REG_HL(gb)); // push hl
	copy_bytes(gb, wTempPals2, wTempOBPals, 8 * PAL_SIZE);
	copy_bytes(gb, wTempPals1, wTempBGPals, 8 * PAL_SIZE);
	SET_HL(gb, gb_pop16(gb)); // pop hl
}

static void set_top_rows_attr(GB *gb, uint16_t bgmap, uint8_t attr)
{
	gb->cpu.a = BANK_v1Tiles0;
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, bgmap);
	SET_BC(gb, 5 * TILEMAP_WIDTH);
	gb->cpu.a = attr;
	CALL(WriteAToHL_BCTimes);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

PORT(SetBGPriorityOnTopRows_BGMap1) { set_top_rows_attr(gb, v1BGMap1, BG_BANK0 | BG_PRIO); }
// d = attributes
PORT(SetBGPriorityOnTopRows) { set_top_rows_attr(gb, v1BGMap0, gb->cpu.d); }

// sets BG priority on the 6x7 block at (0, 5) of v1BGMap0
PORT(Func_9cc4f)
{
	gb->cpu.a = BANK_v1Tiles0;
	wr8(gb, rVBK, gb->cpu.a);
	gb->cpu.d = BG_BANK0 | BG_PRIO;
	uint16_t hl = v1BGMap0 + 5 * TILEMAP_WIDTH;
	gb->cpu.e = 7;
	for (;;) {
		gb->cpu.b = 6;
		gb->cpu.a = gb->cpu.d;
		do {
			wr8(gb, hl++, gb->cpu.a);
			gb->cpu.b = alu_dec(gb, gb->cpu.b);
		} while (gb->cpu.b);
		gb->cpu.e = alu_dec(gb, gb->cpu.e);
		if (!gb->cpu.e)
			break;
		gb->cpu.a = alu_add(gb, TILEMAP_WIDTH - 6, (uint8_t)hl);
		uint8_t l = gb->cpu.a;
		gb->cpu.a = alu_adc(gb, 0, (uint8_t)(hl >> 8));
		hl = (uint16_t)(gb->cpu.a << 8 | l);
	}
	SET_HL(gb, hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

// Scene object animations. The helpers they use are in this bank, so they
// are called with gb_call, and the tail jumps (followed by the caller's
// return) become calls too.

// hl = compressed BG map, bc = destination
PORT(LoadCutsceneBGMap)
{
	wr8(gb, wTempBank, BANK_Cutscene01Tilemap); // BANK("Cutscene BG Maps")
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

PORT(PrologueFunc)
{
	FARCALL(Prologue);
}

PORT_JUMP(LoadCutsceneTilemap0)
{
	SET_BC(gb, v0BGMap0);
	JUMP_TO(ADDR_LoadCutsceneBGMap); // fallthrough
}

PORT_JUMP(LoadCutsceneTilemap1)
{
	SET_BC(gb, v0BGMap1);
	JUMP_TO(ADDR_LoadCutsceneBGMap);
}

PORT(VBlank_Cutscene)
{
	SET_HL(gb, ADDR_VBlank_Cutscene + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 0x51; // .end - .Func
	CALL(CopyHLToDE);
}

// loads the attrmap at `attrmap` and the tilemap at `tilemap`
static void load_cutscene_bg_maps(GB *gb, uint16_t attrmap, uint16_t load_attrmap, uint16_t tilemap, uint16_t load_tilemap)
{
	SET_HL(gb, attrmap);
	gb_call(gb, load_attrmap);
	SET_HL(gb, tilemap);
	gb_call(gb, load_tilemap);
}

PORT(LoadCutsceneBGMaps01)
{
	load_cutscene_bg_maps(gb, ADDR_Cutscene01Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene01Tilemap, ADDR_LoadCutsceneTilemap0);
}

PORT(LoadCutsceneBGMaps02)
{
	load_cutscene_bg_maps(gb, ADDR_Cutscene02Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene02Tilemap, ADDR_LoadCutsceneTilemap1);
}

PORT(LoadCutsceneBGMaps07)
{
	load_cutscene_bg_maps(gb, ADDR_Cutscene07Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene07Tilemap, ADDR_LoadCutsceneTilemap0);
}

PORT_JUMP(LoadCutsceneAttrmap0)
{
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, 1);
	JUMP_TO(ADDR_LoadCutsceneTilemap0); // fallthrough
}

PORT_JUMP(LoadCutsceneAttrmap1)
{
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, 1);
	JUMP_TO(ADDR_LoadCutsceneTilemap1); // fallthrough
}

// --- Cutscene 04 and small helpers ---

PORT(LoadCutsceneBGMaps03)
{
	load_cutscene_bg_maps(gb, ADDR_Cutscene03Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene03Tilemap, ADDR_LoadCutsceneTilemap0);
}

PORT_JUMP(SetBGPriorityOnTopRows_Vram1)
{
	gb->cpu.d = BG_BANK1 | BG_PRIO;
	JUMP_TO(ADDR_SetBGPriorityOnTopRows);
}

PORT_JUMP(SetBGPriorityOnTopRows_Vram0)
{
	gb->cpu.d = BG_BANK0 | BG_PRIO;
	JUMP_TO(ADDR_SetBGPriorityOnTopRows); // fallthrough
}

// --- the remaining LoadCutsceneBGMapsNN ---
PORT(LoadCutsceneBGMaps12) { load_cutscene_bg_maps(gb, ADDR_Cutscene12Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene12Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps13) { load_cutscene_bg_maps(gb, ADDR_Cutscene13Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene13Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps15) { load_cutscene_bg_maps(gb, ADDR_Cutscene15Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene15Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps16) { load_cutscene_bg_maps(gb, ADDR_Cutscene16Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene16Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps17) { load_cutscene_bg_maps(gb, ADDR_Cutscene17Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene17Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps18) { load_cutscene_bg_maps(gb, ADDR_Cutscene18Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene18Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps20) { load_cutscene_bg_maps(gb, ADDR_Cutscene20Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene20Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps22) { load_cutscene_bg_maps(gb, ADDR_Cutscene22Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene22Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps23) { load_cutscene_bg_maps(gb, ADDR_Cutscene23Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene23Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps24) { load_cutscene_bg_maps(gb, ADDR_Cutscene24Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene24Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps26) { load_cutscene_bg_maps(gb, ADDR_Cutscene26Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene26Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps27) { load_cutscene_bg_maps(gb, ADDR_Cutscene27Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene27Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps28) { load_cutscene_bg_maps(gb, ADDR_Cutscene28Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene28Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps29) { load_cutscene_bg_maps(gb, ADDR_Cutscene29Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene29Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps30) { load_cutscene_bg_maps(gb, ADDR_Cutscene30Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene30Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps31) { load_cutscene_bg_maps(gb, ADDR_Cutscene31Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene31Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps32) { load_cutscene_bg_maps(gb, ADDR_Cutscene32Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene32Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps34) { load_cutscene_bg_maps(gb, ADDR_Cutscene34Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene34Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps36) { load_cutscene_bg_maps(gb, ADDR_Cutscene36Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene36Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps37) { load_cutscene_bg_maps(gb, ADDR_Cutscene37Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene37Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps39) { load_cutscene_bg_maps(gb, ADDR_Cutscene39Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene39Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps04) { load_cutscene_bg_maps(gb, ADDR_Cutscene04Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene04Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps05) { load_cutscene_bg_maps(gb, ADDR_Cutscene05Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene05Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps14) { load_cutscene_bg_maps(gb, ADDR_Cutscene14Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene14Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps19) { load_cutscene_bg_maps(gb, ADDR_Cutscene19Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene19Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps21) { load_cutscene_bg_maps(gb, ADDR_Cutscene21Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene21Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps25) { load_cutscene_bg_maps(gb, ADDR_Cutscene25Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene25Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps35) { load_cutscene_bg_maps(gb, ADDR_Cutscene35Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene35Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps38) { load_cutscene_bg_maps(gb, ADDR_Cutscene38Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene38Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps040) { load_cutscene_bg_maps(gb, ADDR_Cutscene40Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene40Tilemap, ADDR_LoadCutsceneTilemap1); }

// --- more scene object animations ---

// ---- InitCutsceneNN ----

// ld b, BANK(pals) / ld hl, pals / call LoadFarPalsToTempPals1 (or 2)
#define PALS1(p) do { gb->cpu.b = BANK_##p; SET_HL(gb, ADDR_##p); CALL(LoadFarPalsToTempPals1); } while (0)
#define PALS2(p) do { gb->cpu.b = BANK_##p; SET_HL(gb, ADDR_##p); CALL(LoadFarPalsToTempPals2); } while (0)
// ld b, BANK(tiles) / ld hl, tiles / call LoadFarTiles
#define FAR_TILES(t) do { gb->cpu.b = BANK_##t; SET_HL(gb, ADDR_##t); CALL(LoadFarTiles); } while (0)
// the same into VRAM bank 1
#define FAR_TILES1(t) do { \
	gb->cpu.a = 1; wr8(gb, rVBK, 1); \
	FAR_TILES(t); \
	gb->cpu.a = 0; gb->cpu.f = FLAG_Z; wr8(gb, rVBK, 0); \
} while (0)
// decompress_attrmap / decompress_tilemap
#define ATTRMAP(m, dest) decompress_tiles1(gb, BANK_##m, ADDR_##m, dest)
#define TILEMAP(m, dest) decompress_tiles0(gb, BANK_##m, ADDR_##m, dest)

// ---- cutscene scene object setups ----

// Scene object animations with per-state hooks. A hook is called with
// a = the state the animation returned; it returns what the asm does next.
enum { ANIM_SPRITE, ANIM_AGAIN, ANIM_RET };
typedef int (*anim_hook)(GB *gb);

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
	gb_call(gb, ADDR_Func_9cc87); \
	PALS1(p); \
	PALS2(p); \
	advance(gb); \
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
static void vbf_push_bank_switch(GB *gb)
{
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = gb->cpu.b;
	bankswitch(gb, gb->cpu.a);
}

// pop af / bankswitch
static void vbf_pop_bank(GB *gb)
{
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}

// ld a, [wLCDCFlagsToFlip] / and a / jr z, .skip / ld hl, rLCDC / xor [hl] /
// ld [hl], a / xor a / ld [wLCDCFlagsToFlip], a
static void vbf_flip_lcdc(GB *gb)
{
	gb->cpu.a = rd8(gb, wLCDCFlagsToFlip);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a)
		return;
	SET_HL(gb, rLCDC);
	gb->cpu.a = alu_xor(gb, gb->cpu.a, rd8(gb, rLCDC));
	wr8(gb, rLCDC, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wLCDCFlagsToFlip, 0);
}

// ld a, $02 / ldh [rWBK], a / apply wSCY and wSCX
static void vbf_wram2_scroll(GB *gb)
{
	gb->cpu.a = 0x02; // BANK("WRAM2")
	wr8(gb, rWBK, gb->cpu.a);
	wr8(gb, rSCY, rd8(gb, wSCY));
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
}

// VBlank_Cutscene.Func, as copied to wVBlankFunc (see ram_code.c). Its calls
// return to base + n, the block's start wherever it runs from.
PORT_LOCAL_JUMP(VBlank_Cutscene, Func)
{
	uint16_t base = gb->cpu.pc;
	vbf_wram2_scroll(gb);
	wr8(gb, rWY, rd8(gb, wCutsceneWY));
	gb->cpu.a = rd8(gb, wCutsceneWX);
	wr8(gb, rWX, gb->cpu.a);

	vbf_flip_lcdc(gb);

	// .skip_lcd_flip
	gb->cpu.b = BANK_ApplyPalConfig1;
	vbf_push_bank_switch(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, wPalConfig1Register), rd8(gb, wPalConfig1Register));
	if (gb->cpu.a)
		gb_call_ret(gb, ADDR_ApplyPalConfig1, (uint16_t)(base + 0x3b));
	gb->cpu.a = alu_and(gb, rd8(gb, wPalConfig2), rd8(gb, wPalConfig2));
	if (gb->cpu.a)
		gb_call_ret(gb, ADDR_ApplyPalConfig2, (uint16_t)(base + 0x42));
	vbf_pop_bank(gb);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	JUMP_TO(hTransferVirtualOAM);
}
