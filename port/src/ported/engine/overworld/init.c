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
