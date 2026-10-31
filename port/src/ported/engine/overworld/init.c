// Port of src/engine/overworld/init.asm
#include "ported.h"

// decompress_tiles0: ld a, BANK(gfx) / ld [wTempBank], a / ld hl, gfx /
// ld bc, dest / ld a, [wTempBank] / ldh [hCallFuncBank], a / hcall Decompress
static void decompress_tiles0(GB *gb, uint8_t bank, uint16_t gfx, uint16_t dest)
{
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
	wr8(gb, rVBK, BANK_v1Tiles0);
	decompress_tiles0(gb, bank, gfx, dest);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

// d:bc = tilemap, e:hl = attrmap
PORT(LoadBGMapsToWRAM)
{
	gb_push16(gb, REG_BC(gb)); // push bc
	gb->cpu.a = gb->cpu.d;
	wr8(gb, w2dfff, gb->cpu.a);
	wr8(gb, rVBK, BANK_v1Tiles0);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, wTempBank, gb->cpu.a);
	SET_BC(gb, wAttrmap);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, gb_pop16(gb)); // pop hl

	gb->cpu.a = rd8(gb, w2dfff);
	wr8(gb, wTempBank, gb->cpu.a);
	SET_BC(gb, w3d500);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
}

PORT(LoadOverworldCommonGfx)
{
	decompress_tiles1(gb, BANK_OverworldCommonGfx, ADDR_OverworldCommonGfx, v1Tiles0);
	decompress_tiles1(gb, BANK_OverworldNumbersGfx, ADDR_OverworldNumbersGfx, v1Tiles0 + 0x60 * 0x10);
}

PORT(LoadOverworld1Gfx) { decompress_tiles0(gb, BANK_Overworld1Gfx, ADDR_Overworld1Gfx, v0Tiles0); }
PORT(LoadOverworld2Gfx) { decompress_tiles0(gb, BANK_Overworld2Gfx, ADDR_Overworld2Gfx, v0Tiles0); }
PORT(LoadOverworld5Gfx) { decompress_tiles0(gb, BANK_Overworld5Gfx, ADDR_Overworld5Gfx, v0Tiles0); }
PORT(LoadOverworld6Gfx) { decompress_tiles0(gb, BANK_Overworld6Gfx, ADDR_Overworld6Gfx, v0Tiles0); }
PORT(LoadOverworldArrowsGfx) { decompress_tiles0(gb, BANK_OverworldArrowsGfx, ADDR_OverworldArrowsGfx, v0Tiles1); }

PORT(LoadOverworldGlowGfx)
{
	decompress_tiles1(gb, BANK_OverworldGlowGfx, ADDR_OverworldGlowGfx, v1Tiles0 + 0x20 * 0x10);
}

PORT(ClearOWWRAM)
{
	SET_HL(gb, 0xd060); // STARTOF("OW WRAM")
	SET_BC(gb, 0x07a0); // SIZEOF("OW WRAM")
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	CALL(WriteAToHL_BCTimes);
}

PORT(SetStoredTransitionParamAsReturnToMap)
{
	gb->cpu.a = rd8(gb, wOWBarsState);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return; // is manual cutscene
	gb->cpu.a = TRANSITION_RETURN_TO_MAP;
	wr8(gb, wStoredTransitionParam, gb->cpu.a);
}

PORT(UpdateStoredTransitionParam)
{
	gb->cpu.a = rd8(gb, wOWBarsState);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return; // exit if any bar is open
	gb->cpu.a = rd8(gb, wOWSceneAction);
	alu_cp(gb, gb->cpu.a, SPECIAL_ACTION);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = rd8(gb, wLastTransitionParam);
	alu_cp(gb, gb->cpu.a, TRANSITION_EPILOGUE_NOT_PERFECT);
	if (gb->cpu.f & FLAG_Z)
		return;
	alu_cp(gb, gb->cpu.a, TRANSITION_EPILOGUE_PERFECT);
	if (gb->cpu.f & FLAG_Z)
		return;
	wr8(gb, wStoredTransitionParam, gb->cpu.a);
}

// waits for A button input
PORT(Func_804ec)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_A))
		return;
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}

PORT(Func_80540)
{
	gb_call(gb, ADDR_Func_818ad);
	gb_call(gb, ADDR_Func_82bb8);
	gb_call(gb, ADDR_Func_82bda);
	FARCALL(Func_b4a37);
	gb_call(gb, ADDR_LoadCompassSprite);
	CALL(ClearUnusedVirtualOAM);
}

PORT(Func_805d7)
{
	gb_call(gb, ADDR_Func_80e89);
	gb_call(gb, ADDR_LoadCompassSprite);
	CALL(ClearUnusedVirtualOAM);
}

PORT(LoadMapSidePals)
{
	FARCALL(_LoadMapSidePals);
}

PORT(LoadPalsForOWScene)
{
	// .CheckIfIsUnlockedExit
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWExitUnlocked, 0);
	gb->cpu.a = rd8(gb, wOWSceneAction);
	alu_cp(gb, gb->cpu.a, UNLOCK_LEVEL);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wOWSceneMapSide);
		gb->cpu.c = rd8(gb, wOWSceneMapSide + 1); // wOWSceneActionParam
		SET_HL(gb, ADDR_UnlockableConnections);
		CALL(GetCthWordFromAthTable);
		gb->cpu.a = rd8(gb, REG_HL(gb));
		alu_cp(gb, gb->cpu.a, OW_EXITS);
		if (!(gb->cpu.f & FLAG_C)) {
			// either OW_EXIT_RIGHT or OW_EXIT_LEFT
			gb->cpu.a = TRUE;
			wr8(gb, wOWExitUnlocked, gb->cpu.a);
		}
	}
	FARCALL(_LoadPalsForOWScene);
}

static void load_bg_maps(GB *gb, uint8_t tbank, uint16_t tilemap, uint8_t abank, uint16_t attrmap)
{
	SET_BC(gb, tilemap);
	gb->cpu.d = tbank;
	SET_HL(gb, attrmap);
	gb->cpu.e = abank;
	gb_call(gb, ADDR_LoadBGMapsToWRAM);
}

PORT(Func_80ae7)
{
	load_bg_maps(gb, BANK_OverworldWestTilemap, ADDR_OverworldWestTilemap,
		BANK_OverworldWestAttrmap, ADDR_OverworldWestAttrmap);
}

PORT(Func_80af5)
{
	load_bg_maps(gb, BANK_OverworldSouthTilemap, ADDR_OverworldSouthTilemap,
		BANK_OverworldSouthAttrmap, ADDR_OverworldSouthAttrmap);
	gb->cpu.a = EVENT_REVEAL_CASTLE;
	gb_call(gb, ADDR_Func_819c6);
	if (gb->cpu.f & FLAG_Z)
		return;
	SET_HL(gb, ADDR_Data_8561f);
	FARCALL(Func_854ee);
}

PORT(Func_80b1b)
{
	load_bg_maps(gb, BANK_OverworldEastTilemap, ADDR_OverworldEastTilemap,
		BANK_OverworldEastAttrmap, ADDR_OverworldEastAttrmap);
}

// copies from wTilemap to v0BGMap0
// and from wAttrmap to v1BGMap0
PORT(WriteBGMapFromWRAM)
{
	SET_HL(gb, wTilemap);
	SET_DE(gb, v0BGMap0);
	SET_BC(gb, 19 * TILEMAP_WIDTH);
	CALL(CopyHLToDE_BC);
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, wAttrmap);
	SET_DE(gb, v1BGMap0);
	SET_BC(gb, 19 * TILEMAP_WIDTH);
	CALL(CopyHLToDE_BC);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

PORT(Func_80b54)
{
	wr8(gb, w2d032, 0x1e);
	wr8(gb, w2d035, 0x04);
	wr8(gb, w2d038, 0x24);
	wr8(gb, w2d03b, 0x24);
	wr8(gb, w2d03e, 0x0e);
	gb->cpu.a = 0x14;
	wr8(gb, w2d041, 0x14);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	static const uint16_t zero1[] = {
		w2d031, wCloudType, w2d049, w2d033, w2d036, w2d039, w2d03c, w2d03f,
		w2d042, w2d034, w2d037, w2d03a, w2d03d, w2d040, w2d043,
	};
	for (unsigned i = 0; i < sizeof(zero1) / sizeof(zero1[0]); i++)
		wr8(gb, zero1[i], 0);
	wr8(gb, w2d046, 0x10);
	wr8(gb, w2d044, 0);
	wr8(gb, w2d047, 0);
	wr8(gb, w2d048, 0);

	gb->cpu.a = rd8(gb, wCurMapSide);
	alu_cp(gb, gb->cpu.a, WEST);
	if (gb->cpu.f & FLAG_Z)
		goto cloud;
	alu_cp(gb, gb->cpu.a, SOUTH);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0x04;
		wr8(gb, w2d03b, gb->cpu.a);
		return;
	}
	alu_cp(gb, gb->cpu.a, EAST);
	if (gb->cpu.f & FLAG_Z)
		goto cloud;
	return;
cloud:
	gb->cpu.a = 0x01;
	wr8(gb, wCloudType, gb->cpu.a);
}

PORT(Func_80bd9)
{
	uint16_t hl = wAttrmap + 14 * TILEMAP_WIDTH;
	wr8(gb, hl, rd8(gb, hl) & ~(1 << B_BG_PRIO));

	// reset BG priority inside a
	// 3x4 rectangle, at coord (0, 15)
	SET_DE(gb, 15 * TILEMAP_WIDTH);
	gb->cpu.c = 4;
	for (;;) {
		hl = alu_add16(gb, wAttrmap, REG_DE(gb));
		gb->cpu.b = 3;
		do {
			wr8(gb, hl, rd8(gb, hl) & ~(1 << B_BG_PRIO));
			hl++;
			gb->cpu.b = alu_dec(gb, gb->cpu.b);
		} while (!(gb->cpu.f & FLAG_Z));
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		if (gb->cpu.f & FLAG_Z)
			break;
		hl = alu_add16(gb, TILEMAP_WIDTH, REG_DE(gb));
		SET_DE(gb, hl);
	}
	SET_HL(gb, hl);
}

PORT_JUMP(Func_803e6)
{
	gb->cpu.a = rd8(gb, wTransitionParam);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wOWBarsState);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	alu_cp(gb, gb->cpu.a, OW_EXITS);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	JUMP(SaveGameOrEnterPerfectScreen);
}

PORT(LoadCloudAndSeaGfx) { FARCALL(_LoadCloudAndSeaGfx); }

#define OW_LCDC_WIN (LCDC_ON | LCDC_WIN_9C00 | LCDC_WIN_ON | LCDC_OBJ_16 | LCDC_OBJ_ON | LCDC_BG_ON)

// xor a / ld hl, wTempPals1 palette 7 color 1 / ld bc, $6 / call WriteAToHL_BCTimes
static void clear_pal7_colors(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, wTempPals1 + PAL_SIZE * 7 + COLOR_SIZE * 1);
	SET_BC(gb, 0x6);
	CALL(WriteAToHL_BCTimes);
}

// ld hl, src / ld de, dest / ld b, n / call CopyHLToDE
static void copy_bytes(GB *gb, uint16_t hl, uint16_t de, uint8_t b)
{
	SET_HL(gb, hl);
	SET_DE(gb, de);
	gb->cpu.b = b;
	CALL(CopyHLToDE);
}

static void set_vbk1(GB *gb)
{
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, 1);
}

static void set_vbk0(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

PORT(Func_8065e)
{
	gb->cpu.a = gb->cpu.c = rd8(gb, wGameModeFlags);
	gb->cpu.a = alu_and(gb, gb->cpu.a, MODE_GAME_CLEARED);
	if (alu_bit(gb, gb->cpu.c, MODE_TIME_ATTACK_F)) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
	}
	wr8(gb, wMissingTreasureIndicatorsEnabled, gb->cpu.a);
	gb_call(gb, ADDR_Func_80b54);

	decompress_tiles0(gb, BANK_OverworldTopBarTilemap, ADDR_OverworldTopBarTilemap,
		v0BGMap0 + 29 * TILEMAP_WIDTH);
	decompress_tiles1(gb, BANK_OverworldTopBarAttrmap, ADDR_OverworldTopBarAttrmap,
		v1BGMap0 + 29 * TILEMAP_WIDTH);

	gb->cpu.a = rd8(gb, wOWBarsState);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a) ? 0xec : 0x04;
	wr8(gb, rSCY, gb->cpu.a);
	wr8(gb, wSCY, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCX, 0);
	wr8(gb, wSCX, 0);
	wr8(gb, wVirtualOAMByteSize, 0);

	gb->cpu.a = rd8(gb, wCurMapSide); // a gets overwritten
	gb_call(gb, ADDR_Func_81dce);
	gb_call(gb, ADDR_ShowMissingTreasureIndicators);
	gb_call(gb, ADDR_Func_80bd9);
	gb_call(gb, ADDR_WriteBGMapFromWRAM);

	gb_call(gb, ADDR_Func_8212c);
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	wr8(gb, wLevel2, gb->cpu.a);
	wr8(gb, w2d068, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wLevelArrowDirections1, 0);

	gb->cpu.a = OWWARIO_DOWN;
	gb_call(gb, ADDR_SetOWWarioOAM);
	FARCALL(Func_b4a3d);
	gb_call(gb, ADDR_Func_82041);
	FARCALL(Func_b4aa9);
	gb_call(gb, ADDR_LoadCloudAndSeaGfx);

	gb->cpu.a = rd8(gb, wGameModeFlags);
	if (!alu_bit(gb, gb->cpu.a, MODE_TIME_ATTACK_F)) {
		decompress_tiles0(gb, BANK_OverworldBottomBarTreasuresTilemap,
			ADDR_OverworldBottomBarTreasuresTilemap, v0BGMap1);
		decompress_tiles1(gb, BANK_OverworldBottomBarTreasuresAttrmap,
			ADDR_OverworldBottomBarTreasuresAttrmap, v1BGMap1);
	} else {
		decompress_tiles0(gb, BANK_OverworldBottomBarTimeAttackTilemap,
			ADDR_OverworldBottomBarTimeAttackTilemap, v0BGMap1);
		decompress_tiles1(gb, BANK_OverworldBottomBarTimeAttackAttrmap,
			ADDR_OverworldBottomBarTimeAttackAttrmap, v1BGMap1);
	}
	// .asm_80790
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wWX, 0);
	wr8(gb, rWX, 0);
	gb->cpu.a = SCREEN_HEIGHT_PX;
	wr8(gb, wWY, gb->cpu.a);
	wr8(gb, rWY, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wBottomBarAction, 0); // BOTBAR_CLOSED

	gb->cpu.a = MAGNIFYING_GLASS;
	CALL(IsTreasureCollected);
	wr8(gb, wHasMagnifyingGlass, gb->cpu.a);

	// collection button is always selectable
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, wTopBarSelectableButtons);
	wr8(gb, wTopBarSelectableButtons, 0);
	wr8(gb, wTopBarSelectableButtons, 1 << TOPBAR_COLLECTION_F);

	gb_call(gb, ADDR_SetDayNightSpellSelectable);
	gb_call(gb, ADDR_SetCutsceneButtonSelectable);
	gb_call(gb, ADDR_SetNextPrevMapButtonsSelectable);
	gb_call(gb, ADDR_UpdateNextPrevMapButtonsSelectable);

	copy_bytes(gb, v0BGMap0 + 30 * TILEMAP_WIDTH, wAttrmap + 21 * TILEMAP_WIDTH, 2 * TILEMAP_WIDTH);
	set_vbk1(gb);
	copy_bytes(gb, v0BGMap0 + 30 * TILEMAP_WIDTH, wAttrmap + 19 * TILEMAP_WIDTH, 2 * TILEMAP_WIDTH);
	set_vbk0(gb);

	gb->cpu.a = rd8(gb, wOWBarsState);
	alu_cp(gb, gb->cpu.a, OWBARS_INPUT_TOP);
	if (gb->cpu.f & FLAG_Z) {
		gb_call(gb, ADDR_UpdateTopBar + 0x72); // .InitTopBarButtons
		copy_bytes(gb, wAttrmap + 21 * TILEMAP_WIDTH, v0BGMap0 + 30 * TILEMAP_WIDTH, 2 * TILEMAP_WIDTH);
		set_vbk1(gb);
		copy_bytes(gb, wAttrmap + 19 * TILEMAP_WIDTH, v0BGMap0 + 30 * TILEMAP_WIDTH, 2 * TILEMAP_WIDTH);
		set_vbk0(gb);

		gb->cpu.a = rd8(gb, w2d02c);
		wr8(gb, wTransitionParam, gb->cpu.a);
		CALL(PlayOverworldMusic);
	}
	// .asm_80809
	gb_call(gb, ADDR_LoadLevelNameIfValid);
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	alu_cp(gb, gb->cpu.a, OW_EXITS);
	gb->cpu.b = 0x01;
	if (!(gb->cpu.f & FLAG_C)) {
		clear_pal7_colors(gb);
		gb->cpu.b = 0x00;
	}
	// .asm_80821
	gb->cpu.a = gb->cpu.b;
	wr8(gb, w2d0e0, gb->cpu.a);
	FARCALL(DrawCoinCount);
	gb_call(gb, ADDR_SetCompassSprite);

	gb->cpu.a = rd8(gb, wOWBarsState);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		gb_call(gb, ADDR_DrawBottomBar);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wLevelArrowDirections1, 0);
	wr8(gb, wOWPalTransitionState, 0);
	wr8(gb, w2d013, 0);

	gb->cpu.a = OW_LCDC_WIN;
	wr8(gb, rLCDC, gb->cpu.a);

	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}

// InitOverworld.load_map_without_scene: simply loads and shows the
// overworld map for player to navigate
static void load_map_without_scene(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurEvent, 0); // EVENT_00
	gb->cpu.a = SST_OVERWORLD_09;
	wr8(gb, wSubState, gb->cpu.a);
	gb_call(gb, ADDR_Func_803e6);
}

// InitOverworld.after_cutscene, which cutscenes jump back to
PORT_LOCAL(InitOverworld, after_cutscene)
{
	// check if there's a OW scene to play
	gb->cpu.a = rd8(gb, wCurEvent);
	gb_call(gb, ADDR_GetOWSceneParams);
	if (gb->cpu.f & FLAG_Z) {
		load_map_without_scene(gb);
		return;
	}
	// .play_ow_scene
	gb_call(gb, ADDR_UpdateStoredTransitionParam);
	gb->cpu.a = SST_OVERWORLD_OW_SCENE;
	wr8(gb, wSubState, gb->cpu.a);
}

PORT(InitOverworld)
{
	gb->cpu.a = 2; // BANK("WRAM2")
	wr8(gb, rWBK, 2);
	// stop_music2
	gb->cpu.a = 0xff;
	wr8(gb, hMusicIDBackup + 0, 0xff);
	wr8(gb, hMusicID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hMusicIDBackup + 1, 0x00);
	wr8(gb, hMusicID + 1, 0x00);

	gb->cpu.a = rd8(gb, wTransitionParam);
	wr8(gb, wLastTransitionParam, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wRoomAnimatedTilesEnabled, 0);

	// if transition was from New Game,
	// set 0th treasure as collected
	gb->cpu.a = alu_inc(gb, rd8(gb, wLastTransitionParam));
	if (gb->cpu.f & FLAG_Z) {
		// TRANSITION_NEW_GAME
		SET_HL(gb, wTreasuresCollected);
		wr8(gb, wTreasuresCollected, rd8(gb, wTreasuresCollected) | 1);
	}

	// load crayon flags from collected treasures
	gb_call(gb, ADDR_GetCrayonFlags);

	gb->cpu.a = rd8(gb, wOWBarsState);
	alu_cp(gb, gb->cpu.a, OWBARS_INPUT_TOP);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = rd8(gb, wOWLevel);
		alu_cp(gb, gb->cpu.a, LEVEL_GOLF_BUILDING);
		if (!(gb->cpu.f & FLAG_C)) {
			// .golf_building
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wNextMapSide, 0);
			wr8(gb, wMapSideLevelID, 0);
			gb->cpu.a = SST_OVERWORLD_GOLF_BUILDING;
			wr8(gb, wSubState, gb->cpu.a);
			gb_call(gb, ADDR_Func_803e6);
			return;
		}
		gb->cpu.b = gb->cpu.a;
		gb_call(gb, ADDR_GetMapLevelID);
		gb->cpu.a = gb->cpu.b;
		wr8(gb, wNextMapSide, gb->cpu.a);
		gb->cpu.a = gb->cpu.d;
		wr8(gb, wMapSideLevelID, gb->cpu.a);
	}
	// .skip_get_level_index
	gb_call(gb, ADDR_ClearOWWRAM);

	// jump to corresponding handler depending
	// on what transition to map it is
	enum { NO_SCENE, OW_SCENE, CUTSCENE } next;
	gb->cpu.a = rd8(gb, wLastTransitionParam);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		goto load_map_without_scene;
	alu_cp(gb, gb->cpu.a, TRANSITION_RETURN_TO_MAP);
	if (gb->cpu.f & FLAG_Z)
		goto load_map_without_scene;
	alu_cp(gb, gb->cpu.a, TRANSITION_EPILOGUE_NOT_PERFECT);
	if (gb->cpu.f & FLAG_Z)
		goto epilogue;
	alu_cp(gb, gb->cpu.a, TRANSITION_EPILOGUE_PERFECT);
	if (gb->cpu.f & FLAG_Z)
		goto epilogue;
	alu_cp(gb, gb->cpu.a, TRANSITION_GAME_OVER);
	if (gb->cpu.f & FLAG_Z) {
		// .game_over
		gb_call(gb, ADDR_Func_803e6);
		return;
	}
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		// TRANSITION_NEW_GAME
		gb->cpu.a = EVENT_PROLOGUE;
		wr8(gb, wCurEvent, gb->cpu.a);
	} else {
		// .check_if_unlocked_event
		// transition was from collected treasure
		// if an event is unlocked, play cutscene
		gb_call(gb, ADDR_CheckIfTreasureUnlocksEvent);
		if (gb->cpu.f & FLAG_Z)
			goto load_map_without_scene;
	}
	// .cutscene_check
	gb_call(gb, ADDR_SetStoredTransitionParamAsReturnToMap);
	FARCALL(CheckIfEventHasCutscene);
	gb->cpu.a = rd8(gb, wEventWithCutscene);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		next = CUTSCENE;
		goto play;
	}
after_cutscene:
	// check if there's a OW scene to play
	gb->cpu.a = rd8(gb, wCurEvent);
	gb_call(gb, ADDR_GetOWSceneParams);
	if (gb->cpu.f & FLAG_Z)
		goto load_map_without_scene;
	next = OW_SCENE;
play:
	gb_call(gb, ADDR_UpdateStoredTransitionParam);
	gb->cpu.a = next == OW_SCENE ? SST_OVERWORLD_OW_SCENE : SST_OVERWORLD_CUTSCENE;
	wr8(gb, wSubState, gb->cpu.a);
	return;

epilogue:
	// plays epilogue sequence
	gb->cpu.a = TRANSITION_RETURN_TO_MAP;
	wr8(gb, wStoredTransitionParam, gb->cpu.a);
	gb->cpu.a = EVENT_EPILOGUE;
	wr8(gb, wCurEvent, gb->cpu.a);
	goto after_cutscene;

load_map_without_scene:
	load_map_without_scene(gb);
}

// call LoadMapSidePals / call gfx / call LoadOverworldCommonGfx /
// call LoadOverworldArrowsGfx
static void init_map_side_gfx(GB *gb, uint16_t gfx)
{
	gb_call(gb, ADDR_LoadMapSidePals);
	gb_call(gb, gfx);
	gb_call(gb, ADDR_LoadOverworldCommonGfx);
	gb_call(gb, ADDR_LoadOverworldArrowsGfx);
}

PORT_JUMP(InitNorthMapSide)
{
	init_map_side_gfx(gb, ADDR_LoadOverworld1Gfx);
	load_bg_maps(gb, BANK_OverworldNorthTilemap, ADDR_OverworldNorthTilemap,
		BANK_OverworldNorthAttrmap, ADDR_OverworldNorthAttrmap);
	JUMP_TO(ADDR_Func_8065e);
}

PORT_JUMP(InitWestMapSide)
{
	init_map_side_gfx(gb, ADDR_LoadOverworld2Gfx);
	gb_call(gb, ADDR_Func_80ae7);
	JUMP_TO(ADDR_Func_8065e);
}

PORT_JUMP(InitSouthMapSide)
{
	init_map_side_gfx(gb, ADDR_LoadOverworld5Gfx);
	gb_call(gb, ADDR_Func_80af5);
	JUMP_TO(ADDR_Func_8065e);
}

PORT_JUMP(InitEastMapSide)
{
	gb->cpu.a = EVENT_SUMMON_SUN;
	gb_call(gb, ADDR_Func_819c6);
	wr8(gb, wGotSunMedallion, gb->cpu.a);
	gb->cpu.a = rd8(gb, wGotSunMedallion); // unnecessary
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = 0x01;
		wr8(gb, w2d011, 0x01);
		SET_HL(gb, wDayNight);
		wr8(gb, wDayNight, (uint8_t)((rd8(gb, wDayNight) | 1) & ~0x80)); // set 0 / res 7
	}
	// .got_medallion
	gb->cpu.a = rd8(gb, wTransitionParam);
	alu_cp(gb, gb->cpu.a, KEY_CARD_BLUE);
	if (gb->cpu.f & FLAG_Z) {
		// .Func_80655
		gb->cpu.a = KEY_CARD_RED;
		CALL(IsTreasureCollected);
		if (gb->cpu.f & FLAG_Z)
			gb_call(gb, ADDR_SetStoredTransitionParamAsReturnToMap);
	}
	init_map_side_gfx(gb, ADDR_LoadOverworld6Gfx);
	gb_call(gb, ADDR_Func_80b1b);
	JUMP_TO(ADDR_Func_8065e);
}

// rst JumpTable, with the table at `table` (right after the rst): leaves
// every register as JumpTable does and returns the target (= hl).
static uint16_t init_jumptable(GB *gb, uint16_t table)
{
	gb_push16(gb, table); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	return REG_HL(gb);
}

// ld hl, wDayNight / srl [hl] / rra / rl [hl]: swaps bit 0 of a and [hl]
static void swap_day_night_bit0(GB *gb)
{
	SET_HL(gb, wDayNight);
	wr8(gb, wDayNight, alu_srl(gb, rd8(gb, wDayNight)));
	gb->cpu.a = alu_rra(gb, gb->cpu.a);
	wr8(gb, wDayNight, alu_rl(gb, rd8(gb, wDayNight)));
}

PORT_JUMP(InitMapSide)
{
	CALL(DisableLCD);
	CALL(FillBGMap0_With7f);

	// stop_music2
	gb->cpu.a = 0xff;
	wr8(gb, hMusicIDBackup + 0, 0xff);
	wr8(gb, hMusicID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hMusicIDBackup + 1, 0x00);
	wr8(gb, hMusicID + 1, 0x00);

	gb->cpu.a = 0x02;
	wr8(gb, rWBK, 0x02);
	CALL(ClearVirtualOAM);
	gb_call(gb, ADDR_VBlank_Overworld);

	gb_call(gb, ADDR_ClearOWWRAM);
	gb_call(gb, ADDR_GetNextTreasureToCollect);

	gb->cpu.a = gb->cpu.b = rd8(gb, wDayNight);
	if (!alu_bit(gb, gb->cpu.a, 7)) {
		gb->cpu.a = alu_swap(gb, rd8(gb, wDayNight));
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
		wr8(gb, w2d011, gb->cpu.a);
		// swap bit 0 in a and [hl]
		swap_day_night_bit0(gb);
	} else {
		// .asm_8059b
		gb_call(gb, ADDR_SwitchDayNight);
		gb->cpu.a = gb->cpu.b & ~(1 << 7) & ~(1 << 4); // res 7, a / res 4, a
		wr8(gb, w2d011, gb->cpu.a);

		gb->cpu.a = rd8(gb, wTransitionParam);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			gb->cpu.a = alu_xor(gb, rd8(gb, w2d011), 0x1);
			wr8(gb, w2d011, gb->cpu.a);
			swap_day_night_bit0(gb);
			wr8(gb, wDayNight, rd8(gb, wDayNight) & ~(1 << 7)); // res 7, [hl]
		}
	}
	// .asm_805be
	gb->cpu.a = rd8(gb, w2d011);
	wr8(gb, w2d07c, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d07d, 0);
	gb->cpu.a = rd8(gb, wNextMapSide);
	wr8(gb, wCurMapSide, gb->cpu.a);
	JUMP_TO(init_jumptable(gb, ADDR_InitMapSide + 0x70));
}

static void copy_level_name_gfx(GB *gb, uint16_t de)
{
	gb->cpu.b = 0x24;
	gb->cpu.a = alu_add(gb, rd8(gb, wLanguage), gb->cpu.b);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.c = 0x00; // $10 tiles
	SET_HL(gb, ADDR_LevelNamesNorthJPGfx + (7 * 0x20 * TILE_SIZE));
	SET_DE(gb, de);
	CALL(CopyFarBytes);
}

PORT(Func_8086f)
{
	gb_call(gb, ADDR_Func_80b54);

	gb->cpu.a = rd8(gb, wCurMapSide); // a gets overwritten
	gb_call(gb, ADDR_Func_81dce);
	gb_call(gb, ADDR_WriteBGMapFromWRAM);

	gb->cpu.a = 0x04;
	wr8(gb, rSCY, 0x04);
	wr8(gb, wSCY, 0x04);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCX, 0);
	wr8(gb, wSCX, 0);
	wr8(gb, w2d015, rd8(gb, w2d018));
	wr8(gb, wOWSceneActionParam, rd8(gb, wTempUnlockableConnectionID));
	wr8(gb, wOWSceneAction, rd8(gb, wTempOWSceneAction));
	wr8(gb, wOWSceneMapSide, rd8(gb, wCurMapSide));
	wr8(gb, wOWSceneParamsPtr + 0, rd8(gb, wTempOWSceneParamsPtr + 0));
	gb->cpu.a = rd8(gb, wTempOWSceneParamsPtr + 1);
	wr8(gb, wOWSceneParamsPtr + 1, gb->cpu.a);

	FARCALL(Func_b4a37);
	gb_call(gb, ADDR_LoadCloudAndSeaGfx);

	clear_pal7_colors(gb);

	set_vbk1(gb);
	copy_level_name_gfx(gb, v1Tiles2 + TILE_SIZE * 0x50);
	copy_level_name_gfx(gb, v1Tiles2 + TILE_SIZE * 0x60);
	set_vbk0(gb);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d0e0, 0);
	gb_call(gb, ADDR_SetCompassSprite);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wLevelArrowDirections1, 0);
	wr8(gb, wOWPalTransitionState, 0);
	wr8(gb, w2d013, 0);

	gb->cpu.a = rd8(gb, wCurMapSide);
	bool window = false;
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = rd8(gb, wCurEvent);
		alu_cp(gb, gb->cpu.a, EVENT_MAGNIFYING_GLASS);
		window = (gb->cpu.f & FLAG_Z) != 0;
	}
	gb->cpu.a = window ? OW_LCDC_WIN : LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a); // .apply_lcd

	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}


// call LoadPalsForOWScene / call gfx / call LoadOverworldCommonGfx /
// call LoadOverworldGlowGfx / call func / jp Func_8086f
static void init_map_side_for_scene(GB *gb, uint16_t gfx, uint16_t func)
{
	gb_call(gb, ADDR_LoadPalsForOWScene);
	gb_call(gb, gfx);
	gb_call(gb, ADDR_LoadOverworldCommonGfx);
	gb_call(gb, ADDR_LoadOverworldGlowGfx);
	gb_call(gb, func);
	JUMP_TO(ADDR_Func_8086f);
}

PORT_JUMP(InitMapSideForScene_North)
{
	gb_call(gb, ADDR_LoadPalsForOWScene);
	gb_call(gb, ADDR_LoadOverworldCommonGfx);
	gb_call(gb, ADDR_LoadOverworld1Gfx);
	gb->cpu.a = rd8(gb, wCurEvent);
	alu_cp(gb, gb->cpu.a, EVENT_DAY_OR_NIGHT_SPELL);
	if (!(gb->cpu.f & FLAG_Z))
		gb_call(gb, ADDR_LoadOverworldGlowGfx);
	SET_BC(gb, ADDR_OverworldNorthTilemap);
	gb->cpu.d = BANK_OverworldNorthTilemap;
	SET_HL(gb, ADDR_OverworldNorthAttrmap);
	gb->cpu.e = BANK_OverworldNorthAttrmap;
	gb_call(gb, ADDR_LoadBGMapsToWRAM);
	JUMP_TO(ADDR_Func_8086f); // fallthrough
}

PORT_JUMP(InitMapSideForScene_West)
{
	init_map_side_for_scene(gb, ADDR_LoadOverworld2Gfx, ADDR_Func_80ae7);
}

PORT_JUMP(InitMapSideForScene_South)
{
	gb->cpu.a = rd8(gb, wCurEvent);
	alu_cp(gb, gb->cpu.a, EVENT_SUMMON_MOON);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0x01;
		wr8(gb, w2d011, 0x01);
	}
	// .asm_8093c
	init_map_side_for_scene(gb, ADDR_LoadOverworld5Gfx, ADDR_Func_80af5);
}

PORT_JUMP(InitMapSideForScene_East)
{
	gb->cpu.a = EVENT_SUMMON_SUN;
	gb_call(gb, ADDR_Func_819c6);
	wr8(gb, wGotSunMedallion, gb->cpu.a);
	gb->cpu.a = rd8(gb, wGotSunMedallion); // unnecessary
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = 0x01;
		wr8(gb, w2d011, 0x01);
	}
	// .got_medallion
	init_map_side_for_scene(gb, ADDR_LoadOverworld6Gfx, ADDR_Func_80b1b);
}

PORT_JUMP(InitOWScene)
{
	CALL(DisableLCD);
	// stop_music2
	gb->cpu.a = 0xff;
	wr8(gb, hMusicIDBackup + 0, 0xff);
	wr8(gb, hMusicID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hMusicIDBackup + 1, 0x00);
	wr8(gb, hMusicID + 1, 0x00);

	gb_call(gb, ADDR_ClearOWWRAM);
	gb_call(gb, ADDR_VBlank_OWScene);
	CALL(ClearVirtualOAM);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d011, 0);
	wr8(gb, w2d018, rd8(gb, w2d015));
	wr8(gb, wTempUnlockableConnectionID, rd8(gb, wOWSceneActionParam));
	wr8(gb, wTempOWSceneAction, rd8(gb, wOWSceneAction));
	wr8(gb, wTempOWSceneParamsPtr + 0, rd8(gb, wOWSceneParamsPtr + 0));
	wr8(gb, wTempOWSceneParamsPtr + 1, rd8(gb, wOWSceneParamsPtr + 1));

	gb->cpu.a = rd8(gb, wOWSceneMapSide);
	wr8(gb, wCurMapSide, gb->cpu.a);
	JUMP_TO(init_jumptable(gb, ADDR_InitOWScene + 0x41));
}
