// Port of src/engine/overworld/bank26.asm
#include "ported.h"

// a = digit (low nibble): copies its 2 tiles from NumbersGfx to de
static void draw_number(GB *gb)
{
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	uint16_t hl = ADDR_NumbersGfx;
	SET_BC(gb, 0x20);
	for (;;) {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (!gb->cpu.a)
			break;
		hl = alu_add16(gb, hl, 0x20);
	}
	SET_HL(gb, hl);
	gb->cpu.b = 0x20;
	CALL(CopyHLToDE);
}

PORT(DrawCoinCount)
{
	SET_DE(gb, v0Tiles1 + 0xa * TILE_SIZE);
	gb->cpu.a = rd8(gb, wNumCoins + 0);
	draw_number(gb);
	gb->cpu.a = alu_swap(gb, rd8(gb, wNumCoins + 1));
	draw_number(gb);
	gb->cpu.a = rd8(gb, wNumCoins + 1);
	draw_number(gb);
}

PORT(ClearTreasureData)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, wTileBuffer);
	SET_BC(gb, 16 * TILE_SIZE);
	CALL(WriteAToHL_BCTimes);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, wLevelTreasureIDs);
	SET_BC(gb, 0x10);
	CALL(WriteAToHL_BCTimes);
}

PORT(LoadLevelTreasures)
{
	SET_DE(gb, wLevelTreasureIDs);
	gb->cpu.a = rd8(gb, wOWLevel);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = 0;
	uint16_t hl = alu_add16(gb, ADDR_LevelTreasureIDs, gb->cpu.c);
	gb->cpu.b = NUM_LEVEL_TREASURES;
	do {
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e++; // inc e
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	SET_HL(gb, hl);
}

PORT(LoadTreasurePals)
{
	static const uint16_t ids[] = {wLevelGreyTreasureID, wLevelRedTreasureID,
		wLevelGreenTreasureID, wLevelBlueTreasureID};
	static const uint16_t pals[] = {wLevelGreyTreasurePal, wLevelRedTreasurePal,
		wLevelGreenTreasurePal, wLevelBlueTreasurePal};
	SET_DE(gb, ADDR_TreasureOBPals);
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, ids[i]);
		uint16_t hl = alu_add16(gb, gb->cpu.a, ADDR_TreasureOBPals);
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, pals[i], gb->cpu.a);
	}
}

// a = treasure ID: copies its 4 tiles from TreasureGfx to de
static void copy_treasure_gfx(GB *gb)
{
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.b = 0;
	gb->cpu.a = alu_rla(gb, gb->cpu.a);
	gb->cpu.a = alu_rla(gb, gb->cpu.a);
	gb->cpu.b = alu_rl(gb, gb->cpu.b);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0xf0);
	gb->cpu.a = alu_or(gb, gb->cpu.a, gb->cpu.b);
	gb->cpu.a = alu_swap(gb, gb->cpu.a);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, 0x0f, gb->cpu.c);
	gb->cpu.a = alu_swap(gb, gb->cpu.a);
	gb->cpu.c = gb->cpu.a;
	SET_HL(gb, alu_add16(gb, ADDR_TreasureGfx, REG_BC(gb)));
	gb->cpu.b = 4 * TILE_SIZE;
	CALL(CopyHLToDE);
}

PORT(LoadTreasureTiles)
{
	gb->cpu.a = rd8(gb, wLevelGreyTreasureID);
	SET_DE(gb, wTileBuffer);
	copy_treasure_gfx(gb);
	gb->cpu.a = rd8(gb, wLevelRedTreasureID);
	copy_treasure_gfx(gb);
	gb->cpu.a = rd8(gb, wLevelGreenTreasureID);
	copy_treasure_gfx(gb);
	gb->cpu.a = rd8(gb, wLevelBlueTreasureID);
	copy_treasure_gfx(gb);
	copy_treasure_gfx(gb); // falls through into .CopyGfx once more
}

PORT(FillBottomBarTreasureIDs)
{
	SET_DE(gb, wBottomBarTreasureIDs);
	SET_HL(gb, wLevelTreasureIDs);
	gb->cpu.b = NUM_LEVEL_TREASURES;
	CALL(CopyHLToDE);
}

// c = treasure ID
// returns b = level
PORT(GetLevelThatContainsTreasure)
{
	gb->cpu.b = 0xff;
	uint16_t hl = ADDR_LevelTreasureIDs;
	do {
		gb->cpu.b = alu_inc(gb, gb->cpu.b);
		gb->cpu.a = rd8(gb, hl++);
		alu_cp(gb, gb->cpu.a, gb->cpu.c);
	} while (!(gb->cpu.f & FLAG_Z));
	SET_HL(gb, hl);
	gb->cpu.b = alu_srl(gb, gb->cpu.b);
	gb->cpu.b = alu_srl(gb, gb->cpu.b);
}

PORT(Func_9aaf4)
{
	gb->cpu.a = rd8(gb, wOWLevel);
	CALL(CheckLevelMusicalCoinFlag);
	if (gb->cpu.f & FLAG_Z)
		return;
	SET_HL(gb, wBottomBarFlags);
	wr8(gb, wBottomBarFlags, rd8(gb, wBottomBarFlags) | (1 << 6));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCompassObjState, 0);
	wr8(gb, wCompassObjState + 1, 0);
	SET_HL(gb, wCompassObjState + 1);
}

PORT(WriteUncollectedBottomBarTreasureIDs)
{
	gb->cpu.a = alu_swap(gb, rd8(gb, wKeyAndTreasureFlags));
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0f);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.c = NUM_LEVEL_TREASURES;
	SET_DE(gb, wLevelTreasureIDs);
	do {
		gb->cpu.b = alu_srl(gb, gb->cpu.b);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, REG_DE(gb), 0);
		}
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
	} while (gb->cpu.c);
}

// --- collection screen helpers ---

PORT(LoadCollectionPageTreasureGfx)
{
	uint16_t hl = (uint16_t)(ADDR_TreasureGfx + 4 * TILE_SIZE - NUM_TREASURE_TILES * NUM_COLLECTION_CELLS * TILE_SIZE);
	SET_DE(gb, NUM_TREASURE_TILES * NUM_COLLECTION_CELLS * TILE_SIZE);
	gb->cpu.a = alu_inc(gb, rd8(gb, wCollectionPage));
	do {
		hl = alu_add16(gb, hl, REG_DE(gb));
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
	} while (!(gb->cpu.f & FLAG_Z));
	SET_HL(gb, hl);
	SET_DE(gb, v0Tiles1 + 0x20 * TILE_SIZE);
	SET_BC(gb, NUM_TREASURE_TILES * NUM_COLLECTION_CELLS * TILE_SIZE);
	CALL(CopyHLToDE_BC);
	SET_HL(gb, ADDR_Treasure101Gfx);
	SET_DE(gb, v0Tiles1 + 0x7c * TILE_SIZE);
	gb->cpu.b = NUM_TREASURE_TILES * TILE_SIZE;
	CALL(CopyHLToDE);
}

PORT(Func_9a54e)
{
	SET_HL(gb, wCollectionRow);
	SET_BC(gb, 0x800);
	gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
	CALL(WriteAToHL_BCTimes);
}

PORT(Func_9a559)
{
	SET_HL(gb, wCurSceneObj);
	SET_BC(gb, 0x200);
	gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
	CALL(WriteAToHL_BCTimes);
}

PORT(VBlank_TreasureCollection)
{
	SET_HL(gb, ADDR_VBlank_TreasureCollection + 0xc); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 0x16; // .end - .Func
	CALL(CopyHLToDE);
}

// ApplyPageTreasuresPals.GetAttrmapAddress
static void get_attrmap_address(GB *gb)
{
	gb->cpu.a = rd8(gb, w2dffe);
	gb->cpu.d = (uint8_t)((wTilemap >> 8) + 1);
	alu_cp(gb, gb->cpu.a, NUM_COLLECTION_COLS);
	if (gb->cpu.f & FLAG_C)
		gb->cpu.d = (uint8_t)(wTilemap >> 8);
	gb->cpu.b = 0x00;
	gb->cpu.c = gb->cpu.a;
	uint16_t hl = alu_add16(gb, ADDR_ApplyPageTreasuresPals + 0x7e, REG_BC(gb)); // .lo_map_addresses
	gb->cpu.l = rd8(gb, hl);
	gb->cpu.h = gb->cpu.d;
	gb->cpu.e = gb->cpu.l;
	gb->cpu.a = TILEMAP_WIDTH;
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.l);
	gb->cpu.l = gb->cpu.a;
}

static void dec_h3(GB *gb)
{
	for (int i = 0; i < 3; i++)
		gb->cpu.h = alu_dec(gb, gb->cpu.h);
}

PORT(ApplyPageTreasuresPals)
{
	gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
	wr8(gb, w2dffe, gb->cpu.a);
	gb->cpu.a = rd8(gb, wCollectionPage);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.a = (uint8_t)-NUM_COLLECTION_CELLS;
	do {
		gb->cpu.a = alu_add(gb, gb->cpu.a, NUM_COLLECTION_CELLS);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
	} while (!(gb->cpu.f & FLAG_Z));
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, w2dfff, gb->cpu.a);
	for (;;) {
		gb->cpu.a = rd8(gb, w2dfff);
		CALL(IsTreasureCollected);
		if (gb->cpu.f & FLAG_Z) {
			// .not_collected
			get_attrmap_address(gb);
			gb_push16(gb, REG_DE(gb));
			gb->cpu.a = 0xfc;
			wr8(gb, REG_DE(gb), gb->cpu.a);
			gb->cpu.a = alu_inc(gb, gb->cpu.a);
			uint16_t hl = REG_HL(gb);
			wr8(gb, hl++, gb->cpu.a);
			gb->cpu.a = alu_inc(gb, gb->cpu.a);
			gb->cpu.e = alu_inc(gb, gb->cpu.e);
			wr8(gb, REG_DE(gb), gb->cpu.a);
			gb->cpu.a = alu_inc(gb, gb->cpu.a);
			wr8(gb, hl, gb->cpu.a);
			SET_HL(gb, gb_pop16(gb));
			dec_h3(gb);
			gb->cpu.a = 0x04;
			hl = REG_HL(gb);
			wr8(gb, hl++, gb->cpu.a);
			wr8(gb, hl, gb->cpu.a);
			SET_DE(gb, TILEMAP_WIDTH);
			hl = alu_add16(gb, hl, REG_DE(gb));
			wr8(gb, hl--, gb->cpu.a);
			wr8(gb, hl, gb->cpu.a);
			SET_HL(gb, hl);
		} else {
			// .collected
			gb->cpu.a = rd8(gb, w2dfff);
			gb->cpu.e = gb->cpu.a;
			gb->cpu.d = 0x00;
			uint16_t hl = alu_add16(gb, ADDR_TreasureOBPals, REG_DE(gb));
			SET_HL(gb, hl);
			gb->cpu.a = rd8(gb, hl);
			push_af(gb);
			get_attrmap_address(gb);
			pop_af(gb);
			dec_h3(gb);
			hl = REG_HL(gb);
			wr8(gb, hl++, gb->cpu.a);
			wr8(gb, hl, gb->cpu.a);
			SET_HL(gb, hl);
			for (int i = 0; i < 3; i++)
				gb->cpu.d = alu_dec(gb, gb->cpu.d);
			wr8(gb, REG_DE(gb), gb->cpu.a);
			SET_DE(gb, (uint16_t)(REG_DE(gb) + 1));
			wr8(gb, REG_DE(gb), gb->cpu.a);
		}
		// .next_treasure
		SET_HL(gb, w2dffe);
		wr8(gb, w2dffe, alu_inc(gb, rd8(gb, w2dffe)));
		gb->cpu.a = rd8(gb, w2dffe);
		SET_HL(gb, w2dfff);
		wr8(gb, w2dfff, alu_inc(gb, rd8(gb, w2dfff)));
		alu_cp(gb, gb->cpu.a, NUM_COLLECTION_CELLS);
		if (gb->cpu.f & FLAG_Z)
			return;
	}
}

PORT(GetCollectionCellCoords)
{
	gb->cpu.a = rd8(gb, wCollectionRow);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = 0x00;
	uint16_t hl = alu_add16(gb, ADDR_GetCollectionCellCoords + 0x1b, REG_BC(gb)); // .y_coords
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wSceneObj1YCoord, gb->cpu.a);
	gb->cpu.a = rd8(gb, wCollectionCol);
	gb->cpu.c = gb->cpu.a;
	hl = alu_add16(gb, ADDR_GetCollectionCellCoords + 0x1f, REG_BC(gb)); // .x_coords
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wSceneObj1XCoord, gb->cpu.a);
}

PORT(Func_9a78e)
{
	gb->cpu.a = rd8(gb, REG_DE(gb));
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = 0x00;
	uint16_t hl = alu_add16(gb, ADDR_Func_9a78e + 0x14, REG_BC(gb)); // .table_1
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, REG_DE(gb), gb->cpu.a);
	gb->cpu.e = alu_inc(gb, gb->cpu.e);
	gb->cpu.a = rd8(gb, REG_DE(gb));
	gb->cpu.c = gb->cpu.a;
	hl = alu_add16(gb, ADDR_Func_9a78e + 0x18, REG_BC(gb)); // .table_2
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, REG_DE(gb), gb->cpu.a);
}

// a = cell, hl = row/col destination
static void get_collection_cell_row_and_column(GB *gb)
{
	gb->cpu.b = 0;
	for (;;) {
		alu_cp(gb, gb->cpu.a, NUM_COLLECTION_COLS);
		if (gb->cpu.f & FLAG_C)
			break;
		gb->cpu.a = alu_sub(gb, gb->cpu.a, NUM_COLLECTION_COLS);
		gb->cpu.b = alu_inc(gb, gb->cpu.b);
	}
	wr8(gb, REG_HL(gb), gb->cpu.b); // row
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	wr8(gb, REG_HL(gb), gb->cpu.a); // col
	SET_HL(gb, (uint16_t)(REG_HL(gb) + 1));
}

PORT(GetCurrentCollectionCellRowAndColumn)
{
	gb->cpu.a = rd8(gb, wCollectionCell);
	SET_HL(gb, wCollectionRow);
	get_collection_cell_row_and_column(gb);
}

PORT(GetCollectionCellRowAndColumn) { get_collection_cell_row_and_column(gb); }

// --- treasure collection animations ---

PORT(Func_9a9ec)
{
	gb->cpu.h = gb->cpu.b;
	gb->cpu.l = (uint8_t)(gb->cpu.c & ~(1 << 1));
	gb->cpu.b = 0x26;
	CALL(UpdateOWAnimation);
	gb->cpu.a = rd8(gb, REG_BC(gb));
}

PORT(Func_9a9f7)
{
	gb->cpu.b = 0x26;
	CALL(LoadOWSpriteWithScroll);
}

PORT(Func_9a9fd)
{
	gb->cpu.a = rd8(gb, wOWAnimationFinished);
	alu_cp(gb, gb->cpu.a, 0xff);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	uint16_t hl = (uint16_t)(REG_HL(gb) | (1 << 2));
	gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
	wr8(gb, hl++, gb->cpu.a);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	SET_BC(gb, hl);
}

PORT(Func_9aa6c) {}

PORT(LoadLevelTreasureData)
{
	// push_wram BANK("WRAM2")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 2;
	wr8(gb, rWBK, gb->cpu.a);
	gb_call(gb, ADDR_ClearTreasureData);
	gb_call(gb, ADDR_LoadLevelTreasures);
	gb_call(gb, ADDR_LoadTreasurePals);
	gb_call(gb, ADDR_LoadTreasureTiles);
	gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a); // VRAM0
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, wTileBuffer);
	SET_DE(gb, v0Tiles1 + 0x10 * TILE_SIZE);
	gb->cpu.b = 0x00;
	CALL(CopyHLToDE);
	pop_af(gb); // pop_wram
	wr8(gb, rWBK, gb->cpu.a);
}

PORT(Func_9ab1c)
{
	gb->cpu.a = rd8(gb, wOWLevel);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z)
		return;
	FARCALL(GetOWLevelCollectedTreasures);
}

PORT(GetLevelTimeAttackScore)
{
	gb->cpu.a = rd8(gb, wOWLevel);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	SET_DE(gb, w2d0ee);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = 0x00;
	uint16_t hl = alu_add16(gb, wLevelTimeAttackScores, REG_BC(gb));
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, REG_DE(gb), gb->cpu.a);
	gb->cpu.e = alu_inc(gb, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, REG_DE(gb), gb->cpu.a);
	SET_HL(gb, hl);
}

PORT(Func_9aa59)
{
	SET_DE(gb, ADDR_OAM_9ac5d);
	gb_call(gb, ADDR_Func_9a9f7);
}

PORT(Func_9aa56)
{
	gb_call(gb, ADDR_Func_9a9ec);
	SET_DE(gb, ADDR_OAM_9ac5d); // fallthrough into Func_9aa59
	gb_call(gb, ADDR_Func_9a9f7);
}

static void func_9aa56(GB *gb, uint16_t frameset)
{
	SET_DE(gb, frameset);
	JUMP_TO(ADDR_Func_9aa56);
}

PORT_JUMP(Func_9aa6d) { func_9aa56(gb, ADDR_Frameset_9ace1); }
PORT_JUMP(Func_9aa72) { func_9aa56(gb, ADDR_Frameset_9acea); }
PORT_JUMP(Func_9aa77) { func_9aa56(gb, ADDR_Frameset_9acf3); }
PORT_JUMP(Func_9aa7c) { func_9aa56(gb, ADDR_Frameset_9acfc); }

PORT(AnimateTreasureCollection)
{
	static const uint16_t objs[10] = {
		wSceneObj1State, wSceneObj2State, wSceneObj3State, wSceneObj4State, wSceneObj5State,
		wSceneObj6State, wSceneObj7State, wSceneObj8State, wSceneObj9State, wSceneObj10State,
	};
	for (int i = 0; i < 10; i++) {
		SET_BC(gb, objs[i]);
		gb_call(gb, ADDR_AnimateTreasureCollection + 0x3d); // .Func_9aa4a
	}
}

// ld a, [bc] / jumptable
PORT_LOCAL_JUMP(AnimateTreasureCollection, Func_9aa4a)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	gb_push16(gb, ADDR_AnimateTreasureCollection + 0x3f); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	JUMP_TO(REG_HL(gb));
}

PORT_JUMP(Func_9aab5)
{
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	alu_cp(gb, gb->cpu.a, OW_EXITS);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	gb_call(gb, ADDR_ClearTreasureData);
	gb_call(gb, ADDR_Func_9aaf4);
	gb_call(gb, ADDR_Func_9ab1c);
	gb->cpu.a = rd8(gb, wGameModeFlags);
	if (alu_bit(gb, gb->cpu.a, MODE_TIME_ATTACK_F)) {
		JUMP_TO(ADDR_GetLevelTimeAttackScore);
		return;
	}
	gb_call(gb, ADDR_LoadLevelTreasures);
	gb_call(gb, ADDR_FillBottomBarTreasureIDs);
	gb_call(gb, ADDR_WriteUncollectedBottomBarTreasureIDs);
	gb_call(gb, ADDR_LoadTreasurePals);
	gb_call(gb, ADDR_LoadTreasureTiles);
	gb->cpu.a = (uint8_t)(wTileBuffer >> 8);
	wr8(gb, wHDMASourceHi, gb->cpu.a);
	gb->cpu.a = (uint8_t)wTileBuffer;
	wr8(gb, wHDMASourceLo, gb->cpu.a);
	gb->cpu.a = 0x09;
	wr8(gb, wHDMADestHi, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wHDMADestLo, 0);
	gb->cpu.a = 0x10 - 1; // $10 dma_tiles
	wr8(gb, wHDMAMode, gb->cpu.a);
	RET();
}

// rst JumpTable, with the table at `table` (right after the rst): leaves every
// register as JumpTable does and returns the target (= hl).
static uint16_t jumptable(GB *gb, uint16_t table)
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

// ProcessCollectionLinkState.AdvanceState
static void link_advance_state(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCollectionLinkStateCounter, 0);
	SET_HL(gb, wCollectionLinkStateCounter - 1);
	wr8(gb, wCollectionLinkStateCounter - 1, alu_inc(gb, rd8(gb, wCollectionLinkStateCounter - 1)));
}

// ProcessCollectionLinkState.Func_9a806: places a scene obj on each linked
// cell other than the current one
static void link_place_objs(GB *gb)
{
	static const uint16_t objs[] = { wSceneObj2YCoord, wSceneObj3YCoord, wSceneObj3End,
		wSceneObj4End, wSceneObj5End, wSceneObj6End, wSceneObj7End };
	for (int i = 0; i < 7; i++) {
		gb->cpu.a = rd8(gb, wCollectionLinkedCells + i);
		alu_cp(gb, gb->cpu.a, 0x80);
		if (gb->cpu.f & FLAG_Z)
			break;
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		SET_HL(gb, wCollectionCell);
		alu_cp(gb, gb->cpu.a, rd8(gb, wCollectionCell));
		if (gb->cpu.f & FLAG_Z)
			continue;
		SET_HL(gb, objs[i]);
		get_collection_cell_row_and_column(gb);
		// .Func_9a8d8
		gb->cpu.l |= 0x04; // set 2, l
		gb->cpu.a = 0x02;
		CALL(SetSceneObjState);
		SET_DE(gb, objs[i]);
		gb_call(gb, ADDR_Func_9a78e);
	}
	link_advance_state(gb);
}

PORT_JUMP(ProcessCollectionLinkState)
{
	SET_HL(gb, wCollectionLinkStateCounter);
	wr8(gb, wCollectionLinkStateCounter, alu_inc(gb, rd8(gb, wCollectionLinkStateCounter)));
	SET_BC(gb, wCollectionLinkStateCounter);
	gb->cpu.a = rd8(gb, wCollectionLinkState);
	uint16_t target = jumptable(gb, ADDR_ProcessCollectionLinkState + 0xa);
	switch (target - ADDR_ProcessCollectionLinkState) {
	case 0x16: // .AdvanceState
		link_advance_state(gb);
		break;
	case 0x1d: // .WaitDelay
		gb->cpu.a = rd8(gb, REG_BC(gb));
		alu_cp(gb, gb->cpu.a, 0x14);
		if (!(gb->cpu.f & FLAG_C))
			link_advance_state(gb);
		break;
	case 0x23: // .GetLinkedCells
		SET_HL(gb, ADDR_CollectionLinkTable);
		gb->cpu.a = rd8(gb, wCollectionPage);
		CALL(GetPointerFromTableHL);
		gb->cpu.a = rd8(gb, wCollectionCell);
		CALL(GetPointerFromTableHL);
		gb->cpu.a = rd8(gb, REG_HL(gb));
		alu_cp(gb, gb->cpu.a, 0x80);
		if (gb->cpu.f & FLAG_Z) {
			link_advance_state(gb);
			break;
		}
		gb->cpu.b = 7;
		SET_DE(gb, wCollectionLinkedCells);
		CALL(CopyHLToDE);
		link_advance_state(gb);
		link_advance_state(gb);
		break;
	case 0x45: // .Return
		break;
	case 0x46: // .Func_9a806
		link_place_objs(gb);
		break;
	default:
		JUMP_TO(target);
		return;
	}
	RET();
}
