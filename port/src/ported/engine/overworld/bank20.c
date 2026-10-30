// Port of src/engine/overworld/bank20.asm
#include "ported.h"

PORT(Func_82a8d)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWFuncCounter, 0);
	SET_HL(gb, w2d062);
	wr8(gb, w2d062, alu_inc(gb, rd8(gb, w2d062)));
}

// hl = scene params
// output:
// - z set if scene finished
PORT(GetOWSceneParams_GotPtr)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wOWSceneAction, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (!(gb->cpu.f & FLAG_Z)) { // else finished
		wr8(gb, wOWSceneMapSide, rd8(gb, hl++));
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, wOWSceneActionParam, gb->cpu.a);
		wr8(gb, wOWSceneParamsPtr + 0, (uint8_t)hl);
		gb->cpu.a = (uint8_t)(hl >> 8);
		wr8(gb, wOWSceneParamsPtr + 1, gb->cpu.a);
	}
	SET_HL(gb, hl);
}

PORT(LoadCompassSprite)
{
	SET_HL(gb, wCompassObj);
	SET_DE(gb, ADDR_OAM_aa580);
	gb->cpu.b = BANK_OAM_aa580;
	CALL(LoadOWSpriteWithScroll);
}

// rst JumpTable, with the table at `table` (right after the rst): pushes the
// return address, pops it into hl, a = a * 2, de = a, hl += de, hl = [hl].
// Leaves every register as JumpTable does and returns the target (= hl).
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

// a = map side level index
PORT(GetMapLevelCoords)
{
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a); // *2
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, wCurMapSide);
	SET_HL(gb, ADDR_MapLevelCoords);
	CALL(GetCthWordFromAthTable);
}

// bc = path shape, then the path state
PORT_JUMP(Func_810fc)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	uint16_t target = jumptable(gb, ADDR_Func_810fc + 2);
	if (gb->rom_bank != BANK_Func_810fc) {
		JUMP_TO(target); // runs whatever is mapped, like the asm
		return;
	}
	uint16_t bc = REG_BC(gb);
	switch (target - ADDR_Func_810fc) {
	case 0x0c: // .PathNone
		gb->cpu.c = alu_inc(gb, gb->cpu.c);
		gb->cpu.a = 0xff;
		wr8(gb, REG_BC(gb), 0xff);
		break;
	case 0x11: // .PathBeeline
		wr8(gb, wOWYCoord2, rd8(gb, w2d06f));
		wr8(gb, wOWXCoord2, rd8(gb, w2d070));
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, bc, 0);
		break;
	case 0x20: // .PathYThenX
		wr8(gb, wOWYCoord2, rd8(gb, w2d06f));
		gb->cpu.a = 0x01;
		wr8(gb, bc, 0x01);
		break;
	case 0x2a: // .PathXThenY
		wr8(gb, wOWXCoord2, rd8(gb, w2d070));
		gb->cpu.a = 0x01;
		wr8(gb, bc, 0x01);
		break;
	case 0x34: // .PathSShape: halfway along x
		gb->cpu.e = rd8(gb, wOWXCoord1);
		gb->cpu.a = alu_sub(gb, rd8(gb, w2d070), gb->cpu.e);
		gb->cpu.a = alu_sra(gb, gb->cpu.a);
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.e);
		wr8(gb, wOWXCoord2, gb->cpu.a);
		gb->cpu.a = 0x02;
		wr8(gb, bc, 0x02);
		break;
	default:
		JUMP_TO(target);
		return;
	}
	RET();
}

PORT(Func_814bd)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d0e2, 0);
	SET_HL(gb, w2d0e2 + 1);
	wr8(gb, w2d0e2 + 1, alu_inc(gb, rd8(gb, w2d0e2 + 1))); // w2d0e3
}

// a = EVENT_* constant
PORT_JUMP(GetOWSceneParams)
{
	SET_HL(gb, ADDR_OWSceneParams);
	CALL(GetPointerFromTableHL);
	JUMP_TO(ADDR_GetOWSceneParams_GotPtr); // fallthrough
}

// a = PADF_* direction: moves wOWXCoord1 or wOWYCoord1 8 pixels that way
PORT(GetConnectedLevel1ArrowCoord)
{
	uint8_t b = gb->cpu.b = gb->cpu.a;
	gb->cpu.a = 0x8;
	uint16_t hl = wOWXCoord1;
	bool add;
	if (alu_bit(gb, b, B_PAD_RIGHT))
		add = true;
	else if (alu_bit(gb, b, B_PAD_LEFT))
		add = false;
	else {
		hl = wOWYCoord1;
		SET_HL(gb, hl);
		if (alu_bit(gb, b, B_PAD_UP))
			add = false;
		else if (alu_bit(gb, b, B_PAD_DOWN))
			add = true;
		else
			return;
	}
	SET_HL(gb, hl);
	if (!add) {
		gb->cpu.a = alu_cpl(gb, gb->cpu.a);
		gb->cpu.a = alu_inc(gb, gb->cpu.a); // -$8
	}
	gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, hl));
	wr8(gb, hl, gb->cpu.a);
}

PORT(GetConnectedLevel1CoordsInTilemap)
{
	wr8(gb, wTempOWTilemapPtr + 0, rd8(gb, wOWTilemapPtr + 0));
	gb->cpu.a = rd8(gb, wOWTilemapPtr + 1);
	wr8(gb, wTempOWTilemapPtr + 1, gb->cpu.a);
	SET_HL(gb, wOWCoords1);
	CALL(GetOWCoordInTilemap);
}

// a = OWWARIO_* constant
PORT(SetOWWarioOAM)
{
	uint8_t a = gb->cpu.a;
	wr8(gb, w2d106, a);
	if (!alu_and(gb, a, a))
		return;
	uint16_t bc = ADDR_OAM_aa329;
	alu_cp(gb, a, 0x05);
	if (!(gb->cpu.f & FLAG_C))
		bc = ADDR_OAM_aa0cf;
	SET_BC(gb, bc);
	a = alu_dec(gb, a);
	a = alu_add(gb, a, a); // *2
	gb->cpu.e = a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, ADDR_SetOWWarioOAM + 0x2d, a); // .framesets
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, w2d109, gb->cpu.a);
	gb->cpu.e = alu_inc(gb, (uint8_t)w2d109);
	gb->cpu.d = (uint8_t)(w2d109 >> 8);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, REG_DE(gb), gb->cpu.a);
	wr8(gb, w2d107, gb->cpu.c);
	gb->cpu.l = alu_inc(gb, (uint8_t)w2d107);
	wr8(gb, (uint16_t)((w2d107 & 0xff00) | gb->cpu.l), gb->cpu.b);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d104, 0);
	wr8(gb, w2d104 + 1, 0);
	SET_HL(gb, w2d104 + 1);
}

PORT_JUMP(Func_82a26)
{
	gb->cpu.a = 20;
	SET_HL(gb, wOWFuncCounter);
	alu_cp(gb, 20, rd8(gb, wOWFuncCounter));
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_82a8d); // jr
}

// hl = attribute in the tilemap
PORT(ShowLevelPointInOW)
{
	gb->cpu.b = gb->cpu.a;
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0); // attribute
	wr8(gb, hl, 0x00); // tile
	SET_HL(gb, hl);
}

// a = PADF_* direction, hl = attribute in the tilemap
PORT(ShowLevelArrowInOW)
{
	uint8_t b = gb->cpu.b = gb->cpu.a;
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0); // attribute
	uint8_t tile;
	if (alu_bit(gb, b, B_PAD_RIGHT))
		tile = 0x02; // horizontal arrow
	else if (alu_bit(gb, b, B_PAD_LEFT)) {
		wr8(gb, hl, rd8(gb, hl) | (1 << 5)); // x-flip
		tile = 0x02;
	} else if (alu_bit(gb, b, B_PAD_UP))
		tile = 0x01; // vertical arrow
	else if (alu_bit(gb, b, B_PAD_DOWN)) {
		wr8(gb, hl, rd8(gb, hl) | (1 << 6)); // y-flip
		tile = 0x01;
	} else
		return;
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	gb->cpu.a = tile;
	wr8(gb, REG_HL(gb), tile);
}

PORT(Func_8212c)
{
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	wr8(gb, wLevel1, gb->cpu.a);
	gb_call(gb, ADDR_GetMapLevelCoords);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, w2d069, gb->cpu.a);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, w2d06a, gb->cpu.a);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_add(gb, rd8(gb, hl++), gb->cpu.b);
	wr8(gb, wOWYCoord1, gb->cpu.a);
	wr8(gb, w2d100, gb->cpu.a);
	gb->cpu.a = alu_add(gb, rd8(gb, hl), gb->cpu.c);
	wr8(gb, wOWXCoord1, gb->cpu.a);
	wr8(gb, w2d101, gb->cpu.a);
	SET_HL(gb, hl);
}

// .CalculateDistance: hl = coord 1, de = coord 2, c = flag to set in
// w2d0a0 if coord 2 < coord 1. Returns a = |coord 2 - coord 1|.
static void calculate_distance(GB *gb, uint16_t hl, uint16_t de, uint8_t c)
{
	gb->cpu.c = c;
	SET_HL(gb, hl);
	SET_DE(gb, de);
	gb->cpu.b = rd8(gb, hl);
	gb->cpu.a = alu_sub(gb, rd8(gb, de), gb->cpu.b);
	if (!(gb->cpu.f & FLAG_C))
		return;
	gb->cpu.a = alu_cpl(gb, gb->cpu.a);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = gb->cpu.c;
	SET_HL(gb, w2d0a0);
	gb->cpu.a = alu_or(gb, gb->cpu.a, rd8(gb, w2d0a0));
	wr8(gb, w2d0a0, gb->cpu.a);
	gb->cpu.a = gb->cpu.b;
}

PORT(Func_821a1)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d0a0, 0);
	calculate_distance(gb, wOWYCoord1, wOWYCoord2, 0x02);
	wr8(gb, wOWYDistance, gb->cpu.a);
	calculate_distance(gb, wOWXCoord1, wOWXCoord2, 0x04);
	wr8(gb, wOWXDistance, gb->cpu.a);
	gb->cpu.a = rd8(gb, w2d0a0);
	gb->cpu.b = gb->cpu.a;
	uint16_t hl = wOWYDistance;
	gb->cpu.a = rd8(gb, hl++);
	alu_cp(gb, gb->cpu.a, rd8(gb, hl));
	if (gb->cpu.f & FLAG_C) {
		gb->cpu.a = rd8(gb, hl--);
		gb->cpu.b |= 1 << 0;
	}
	wr8(gb, w2d0a1, gb->cpu.a);
	wr8(gb, w2d0a2, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, w2d0a3, gb->cpu.a);
	wr8(gb, w2d0a4, gb->cpu.a);
	gb->cpu.a = gb->cpu.b;
	wr8(gb, w2d0a0, gb->cpu.a);
	gb->cpu.d = 1;
	gb->cpu.e = 1;
	if (alu_bit(gb, rd8(gb, w2d0a0), 1))
		gb->cpu.d = 0xff;
	if (alu_bit(gb, rd8(gb, w2d0a0), 2))
		gb->cpu.e = 0xff;
	SET_HL(gb, w2d0a5);
	wr8(gb, w2d0a5, gb->cpu.d);
	gb->cpu.l = alu_inc(gb, gb->cpu.l); // inc l
	wr8(gb, REG_HL(gb), gb->cpu.e);
}

PORT(Func_80e75)
{
	FARCALL(Func_af13c);
}

// a = value to compare with wOWFuncCounter
PORT_JUMP(Func_82c1d)
{
	SET_HL(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, rd8(gb, wOWFuncCounter));
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_82c22); // fallthrough
}

PORT(Func_82c22)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWFuncCounter, 0);
	SET_HL(gb, wOWFuncCounter + 1);
	wr8(gb, wOWFuncCounter + 1, alu_inc(gb, rd8(gb, wOWFuncCounter + 1)));
}

PORT(Func_82c29)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWSceneActionParam, 0);
	wr8(gb, wOWFuncCounter, 0);
	wr8(gb, wOWFuncCounter + 1, 0);
	SET_HL(gb, wOWFuncCounter + 1);
}

PORT(Func_8142d)
{
	gb->cpu.a = OWWARIO_DOWN;
	gb_call(gb, ADDR_SetOWWarioOAM);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d013, 0);
	wr8(gb, w2d014, 0);
	wr8(gb, w2d0e4, 0);
}

PORT(Func_81398)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d0e5, 0);
	SET_HL(gb, w2d0e5 - 1);
	wr8(gb, w2d0e5 - 1, alu_inc(gb, rd8(gb, w2d0e5 - 1)));
}

PORT_JUMP(Func_814b2)
{
	gb->cpu.a = rd8(gb, w2d100);
	wr8(gb, w2d88e, gb->cpu.a);
	gb->cpu.a = rd8(gb, w2d101);
	wr8(gb, w2d88e + 1, gb->cpu.a);
	SET_HL(gb, w2d88e + 1);
	JUMP_TO(ADDR_Func_814bd); // fallthrough
}

PORT(Func_818ed)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d014, 0);
	SET_HL(gb, w2d013);
	wr8(gb, w2d013, alu_inc(gb, rd8(gb, w2d013)));
}

PORT_JUMP(Func_818e7)
{
	gb->cpu.a = rd8(gb, w2d014);
	alu_cp(gb, gb->cpu.a, 0x04);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_818ed); // fallthrough
}

// returns c = a value for wNextMapSide
PORT(Func_812b0)
{
	uint16_t hl = ADDR_Func_812b0 + 0x0c; // .data
	gb->cpu.a = rd8(gb, wNextMapSide);
	gb->cpu.b = 0x00;
	gb->cpu.c = gb->cpu.a;
	hl = alu_add16(gb, hl, REG_BC(gb));
	gb->cpu.c = rd8(gb, hl);
	SET_HL(gb, hl);
}

// a = transition param
static void set_cutscene_button_selectable(GB *gb)
{
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	SET_HL(gb, wTopBarSelectableButtons);
	wr8(gb, wTopBarSelectableButtons, rd8(gb, wTopBarSelectableButtons) | (1 << TOPBAR_CUTSCENE_F));
}

PORT(SetCutsceneButtonSelectable)
{
	gb->cpu.a = rd8(gb, wStoredTransitionParam);
	alu_cp(gb, gb->cpu.a, TRANSITION_RETURN_TO_MAP);
	if (gb->cpu.f & FLAG_Z)
		return;
	set_cutscene_button_selectable(gb);
}

PORT(SetCutsceneButtonSelectable_GotTransitionParam)
{
	set_cutscene_button_selectable(gb);
}

PORT(SetDayNightSpellSelectable)
{
	gb->cpu.a = DAY_OR_NIGHT_SPELL;
	CALL(IsTreasureCollected);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = rd8(gb, wCurMapSide);
	alu_cp(gb, gb->cpu.a, EAST);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wGotSunMedallion);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z)
			return;
	}
	SET_HL(gb, wTopBarSelectableButtons);
	wr8(gb, wTopBarSelectableButtons, rd8(gb, wTopBarSelectableButtons) | (1 << TOPBAR_DAY_NIGHT_F));
}

PORT(PlaySFX_12A) { play_sfx(gb, SFX_12A); }

PORT(GetAccessibleMapFlags)
{
	gb->cpu.a = rd8(gb, wAccessibleMapSides);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = rd8(gb, wCurMapSide);
	for (;;) {
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 1);
		if (gb->cpu.f & FLAG_C)
			break;
		gb->cpu.b = alu_sla(gb, gb->cpu.b);
	}
	gb->cpu.a = alu_and(gb, gb->cpu.b, TOPBAR_NEXT_MAP | TOPBAR_PREV_MAP);
}

// returns z if wLevel1 is the left exit or the north junction
PORT(IsLeftExitOrJunction)
{
	gb->cpu.a = rd8(gb, wLevel1);
	alu_cp(gb, gb->cpu.a, OW_EXIT_LEFT);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, wOWSceneMapSide);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = OWNORTH_JUNCTION;
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
}

PORT(Func_82111)
{
	uint16_t table;
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	alu_cp(gb, gb->cpu.a, OW_EXITS);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = alu_sub(gb, gb->cpu.a, OW_EXIT_RIGHT);
		table = ADDR_OWExitPaths;
	} else {
		table = ADDR_OWLevelPaths;
	}
	SET_HL(gb, table);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, wNextMapSide);
	CALL(GetCthWordFromAthTable);
}

// b = OW level
// output: b = map side, d = level ID in the map side
PORT(GetMapLevelID)
{
	uint16_t hl = ADDR_MapSideInitialLevels;
	gb->cpu.c = 0xff;
	do {
		gb->cpu.e = rd8(gb, hl++);
		gb->cpu.c = alu_inc(gb, gb->cpu.c);
		gb->cpu.a = alu_dec(gb, rd8(gb, hl));
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
	} while (gb->cpu.f & FLAG_C);
	SET_HL(gb, hl);
	gb->cpu.a = alu_sub(gb, gb->cpu.b, gb->cpu.e);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.b = gb->cpu.c;
}

PORT_JUMP(LoadLevelNameIfValid)
{
	gb->cpu.a = rd8(gb, wNextMapSide);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	gb->cpu.d = gb->cpu.a;
	alu_cp(gb, gb->cpu.a, 0x08);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	JUMP(LoadLevelName);
}

PORT(SetCompassSprite)
{
	gb->cpu.a = rd8(gb, wCurMapSide);
	gb->cpu.c = gb->cpu.a;
	SET_HL(gb, ADDR_SetCompassSprite + 0x12); // .coords
	SET_DE(gb, wCompassObj);
	gb->cpu.b = 2;
	CALL(CopyHLToDE);
	gb->cpu.a = gb->cpu.c;
	wr8(gb, REG_DE(gb), gb->cpu.a);
}

PORT_JUMP(Func_82a2f)
{
	gb->cpu.a = rd8(gb, wOWSceneMapSide);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = rd8(gb, wLevel2);
	gb->cpu.d = gb->cpu.a;
	alu_cp(gb, gb->cpu.a, OW_EXITS);
	if (gb->cpu.f & FLAG_C) {
		CALL(LoadLevelName);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wPalConfig1TotalSteps, 0);
		JUMP_TO(ADDR_Func_82a8d); // jr
		return;
	}
	gb->cpu.a = alu_add(gb, 0x2, rd8(gb, w2d062));
	wr8(gb, w2d062, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d062 - 1, 0);
	SET_HL(gb, w2d062 - 1);
	RET();
}

PORT(LoadMapLevelFromLevelIndex)
{
	gb->cpu.a = rd8(gb, wNextMapSide);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) { // north
		gb->cpu.a = rd8(gb, wMapSideLevelID);
		alu_cp(gb, gb->cpu.a, OWNORTH_JUNCTION);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.a = 0x80;
			wr8(gb, wOWLevel, 0x80);
			return;
		}
	}
	gb->cpu.b = 0x00;
	uint16_t hl = alu_add16(gb, ADDR_MapSideInitialLevels, REG_BC(gb));
	SET_HL(gb, hl);
	gb->cpu.a = alu_add(gb, rd8(gb, wMapSideLevelID), rd8(gb, hl));
	wr8(gb, wOWLevel, gb->cpu.a);
}

// waits until wOWFuncCounter reaches n, unless there are tiles to place
static void wait_func_counter_unless_tiles(GB *gb, uint8_t n)
{
	gb->cpu.a = rd8(gb, w2d07f);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wOWFuncCounter, 0);
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, n);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_82a8d);
}

PORT_JUMP(Func_82a9b) { wait_func_counter_unless_tiles(gb, 4); }
PORT_JUMP(Func_82b67) { wait_func_counter_unless_tiles(gb, 30); }

PORT(Func_82baa)
{
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, 30);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWSceneActionParam, 0);
	wr8(gb, w2d062, 0);
}

PORT_JUMP(Func_82b7b)
{
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, 0x01);
	if (!(gb->cpu.f & FLAG_Z)) {
		alu_cp(gb, gb->cpu.a, 0x40);
		if (gb->cpu.f & FLAG_C) {
			RET();
			return;
		}
		JUMP_TO(ADDR_Func_82a8d);
		return;
	}
	gb->cpu.a = rd8(gb, wLevel2);
	alu_cp(gb, gb->cpu.a, OW_EXITS);
	if (!(gb->cpu.f & FLAG_C)) {
		JUMP_TO(ADDR_Func_82a8d);
		return;
	}
	gb->cpu.a = rd8(gb, wOWYCoord2);
	wr8(gb, wCurSceneObjYCoord, gb->cpu.a);
	gb->cpu.a = rd8(gb, wOWXCoord2);
	wr8(gb, wCurSceneObjXCoord, gb->cpu.a);
	gb->cpu.a = 0x01;
	wr8(gb, wCurSceneObjState, 0x01);
	play_sfx(gb, SFX_104);
	RET();
}

PORT(Func_81142)
{
	SET_HL(gb, w2d0a0);
	uint8_t v = rd8(gb, w2d0a0);
	bool do_rlca;
	if (!alu_bit(gb, v, 0)) {
		gb->cpu.a = 0x40;
		do_rlca = !alu_bit(gb, v, 1);
	} else {
		gb->cpu.a = 0x10;
		do_rlca = alu_bit(gb, v, 2);
	}
	if (do_rlca)
		gb->cpu.a = alu_rlca(gb, gb->cpu.a);
	wr8(gb, wLevelArrowDirections1, gb->cpu.a);
}

PORT_JUMP(Func_814c4)
{
	gb->cpu.b = 0x0a;
	gb->cpu.a = rd8(gb, w2d0e2);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	play_sfx(gb, SFX_113);
	gb->cpu.a = rd8(gb, w2d069);
	wr8(gb, wOWYCoord1, gb->cpu.a);
	wr8(gb, w2d100, gb->cpu.a);
	gb->cpu.a = rd8(gb, w2d06a);
	wr8(gb, wOWXCoord1, gb->cpu.a);
	wr8(gb, w2d101, gb->cpu.a);
	gb->cpu.a = OWWARIO_ARROWS;
	gb_call(gb, ADDR_SetOWWarioOAM);
	JUMP_TO(ADDR_Func_814bd); // jr
}

// places the w2d07f queued tiles and attributes in wTileToPlaceInOW
PORT(Func_81f21)
{
	gb->cpu.a = rd8(gb, w2d07f);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	SET_DE(gb, wTileToPlaceInOW);
	do {
		gb->cpu.a = rd8(gb, REG_DE(gb));
		gb->cpu.c = gb->cpu.a;
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = rd8(gb, REG_DE(gb));
		gb->cpu.b = gb->cpu.a;
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = rd8(gb, REG_DE(gb));
		gb->cpu.l = gb->cpu.a;
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = rd8(gb, REG_DE(gb));
		gb->cpu.h = gb->cpu.a;
		wr8(gb, REG_HL(gb), gb->cpu.c);
		gb->cpu.h = alu_dec(gb, gb->cpu.h);
		gb->cpu.h = alu_dec(gb, gb->cpu.h);
		gb->cpu.h = alu_dec(gb, gb->cpu.h);
		wr8(gb, REG_HL(gb), gb->cpu.b);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = alu_dec(gb, rd8(gb, w2d07f));
		wr8(gb, w2d07f, gb->cpu.a);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	} while (!(gb->cpu.f & FLAG_Z));
	gb->cpu.a = TRUE;
	wr8(gb, wOWPendingTileUpdate, TRUE);
}

// b = y, d = x (relative to the window), e = frame, c = attributes
PORT(LoadSpriteInsideWindow)
{
	gb->cpu.a = alu_add(gb, rd8(gb, wWY), gb->cpu.b);
	wr8(gb, wCurSpriteYCoord, gb->cpu.a);
	gb->cpu.a = alu_add(gb, rd8(gb, wWX), gb->cpu.d);
	wr8(gb, wCurSpriteXCoord, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, wCurSpriteFrame, gb->cpu.a);
	gb->cpu.a = gb->cpu.c;
	wr8(gb, wCurSpriteAttributes, gb->cpu.a);
	SET_HL(gb, ADDR_OAM_aa0cf);
	gb->cpu.b = BANK_OAM_aa0cf;
	CALL(LoadOWSpriteWithScroll_GotParams);
}

PORT(Func_8208d)
{
	gb->cpu.a = rd8(gb, w2d106);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = rd8(gb, w2d109);
	gb->cpu.d = rd8(gb, w2d109 + 1);
	gb->cpu.e = gb->cpu.a;
	SET_HL(gb, w2d104);
	gb->cpu.b = 0x2a;
	CALL(UpdateOWAnimation);
	gb->cpu.a = rd8(gb, w2d107);
	gb->cpu.d = rd8(gb, w2d107 + 1);
	gb->cpu.e = gb->cpu.a;
	SET_HL(gb, w2d100);
	gb->cpu.b = 0x2a;
	CALL(LoadOWSpriteWithScroll);
}

PORT(DrawBottomBarButtonPrompt)
{
	gb->cpu.a = rd8(gb, wHasMagnifyingGlass);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.b = 0x04;
	gb->cpu.a = rd8(gb, wOWLevel);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.b = 0x06;
	} else {
		gb->cpu.a = rd8(gb, wBottomBarFlags);
		if (alu_bit(gb, gb->cpu.a, 4))
			gb->cpu.b = 0x05;
	}
	gb->cpu.a = gb->cpu.b;
	wr8(gb, wCurSpriteFrame, gb->cpu.a);
	gb->cpu.a = alu_add(gb, rd8(gb, wWY), 0x10);
	wr8(gb, wCurSpriteYCoord, gb->cpu.a);
	gb->cpu.a = alu_add(gb, rd8(gb, wWX), 0x10);
	wr8(gb, wCurSpriteXCoord, gb->cpu.a);
	gb->cpu.a = 0x00;
	wr8(gb, wCurSpriteAttributes, 0x00);
	SET_HL(gb, ADDR_OAM_aa580);
	gb->cpu.b = BANK_OAM_aa580;
	CALL(LoadOWSpriteWithScroll_GotParams);
}

// a = treasure
// output: carry if not collected (and not wLastTransitionParam)
PORT(CheckEventTreasure)
{
	alu_cp(gb, gb->cpu.a, INVALID_TREASURE);
	if (!(gb->cpu.f & FLAG_Z)) {
		SET_HL(gb, wLastTransitionParam);
		alu_cp(gb, gb->cpu.a, rd8(gb, wLastTransitionParam));
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.c = FALSE;
		} else {
			CALL(IsTreasureCollected);
			if (gb->cpu.f & FLAG_Z) {
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z | FLAG_C; // xor a / scf
				return;
			}
		}
	}
	gb->cpu.a = TRUE;
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
}

// a = EVENT_* constant: copies its 3 treasures to w2dfff downwards
PORT(LoadEventTreasures)
{
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	gb->cpu.d = alu_rl(gb, gb->cpu.d);
	uint16_t hl = alu_add16(gb, ADDR_EventTreasures, REG_DE(gb));
	SET_DE(gb, w2dfff);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, REG_DE(gb), gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.e = alu_dec(gb, gb->cpu.e);
	wr8(gb, REG_DE(gb), gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.e = alu_dec(gb, gb->cpu.e);
	wr8(gb, REG_DE(gb), gb->cpu.a);
	SET_HL(gb, hl);
}

// b = treasure
// output: c = EVENT_* constant with that treasure (or EVENT_00)
PORT(GetEventWithTreasure)
{
	gb->cpu.c = EVENT_00;
	uint16_t hl = ADDR_EventTreasures;
	for (;;) {
		gb->cpu.a = rd8(gb, hl++);
		alu_cp(gb, gb->cpu.a, TREASURES_END);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.c = EVENT_00;
			break;
		}
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		if (gb->cpu.f & FLAG_Z)
			break;
		gb->cpu.a = rd8(gb, hl++);
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		if (gb->cpu.f & FLAG_Z)
			break;
		gb->cpu.a = rd8(gb, hl++);
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		if (gb->cpu.f & FLAG_Z)
			break;
		gb->cpu.c = alu_inc(gb, gb->cpu.c);
	}
	SET_HL(gb, hl);
}

// GetCthWordFromAthTable on table, with a = wOWSceneMapSide and c = the
// byte after it
static void get_unlocked_word(GB *gb, uint16_t table)
{
	gb->cpu.a = rd8(gb, wOWSceneMapSide);
	gb->cpu.c = rd8(gb, wOWSceneMapSide + 1);
	SET_HL(gb, table);
	CALL(GetCthWordFromAthTable);
}

PORT(GetUnlockedOWLevelData)
{
	get_unlocked_word(gb, ADDR_UnlockableConnections);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wLevel1, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wLevel2, gb->cpu.a);
	get_unlocked_word(gb, ADDR_UnlockableLevelArrows);
	hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wUnlockedLevelArrowsPtr + 0, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wUnlockedLevelArrowsPtr + 1, gb->cpu.a);
	gb->cpu.a = rd8(gb, wLevel1);
	gb_call(gb, ADDR_GetMapLevelCoords);
	hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, w2d069, gb->cpu.a);
	wr8(gb, wOWYCoord1, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, w2d06a, gb->cpu.a);
	wr8(gb, wOWXCoord1, gb->cpu.a);
	gb->cpu.a = rd8(gb, wLevel2);
	gb_call(gb, ADDR_GetMapLevelCoords);
	hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wOWYCoord2, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wOWXCoord2, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT(GetUnlockedLevelArrows)
{
	gb->cpu.a = rd8(gb, wUnlockedLevelArrowsPtr + 0);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.a = rd8(gb, wUnlockedLevelArrowsPtr + 1);
	gb->cpu.h = gb->cpu.a;
	gb->cpu.a = rd8(gb, wOWPathShape);
	CALL(AddOffsetInPointerTable);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wLevelArrowDirections1, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, w2d074, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wLevelArrowDirections2, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0xff);
	gb->cpu.a = rd8(gb, hl);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
	}
	wr8(gb, w2d076, gb->cpu.a);
	SET_HL(gb, hl);
}

// output: c = TRUE and carry if one of the event's treasures is collected
PORT(CheckIfEventIsUnlocked)
{
	gb_call(gb, ADDR_LoadEventTreasures);
	gb->cpu.c = TRUE;
	gb->cpu.a = rd8(gb, w2dffd);
	gb_call(gb, ADDR_CheckEventTreasure);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = rd8(gb, w2dffe);
	gb_call(gb, ADDR_CheckEventTreasure);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = rd8(gb, w2dfff);
	gb_call(gb, ADDR_CheckEventTreasure);
}

PORT_JUMP(LoadSpriteInsideWindow_Low)
{
	gb->cpu.b = 0x24;
	JUMP_TO(ADDR_LoadSpriteInsideWindow); // fallthrough
}

PORT_JUMP(Func_82a62)
{
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, 16);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	FARCALL(LoadOWStarIndicatorPals);
	JUMP_TO(ADDR_Func_82a8d);
}

PORT_JUMP(Func_82a96)
{
	gb_call(gb, ADDR_GetUnlockedLevelArrows);
	JUMP_TO(ADDR_Func_82a8d);
}

PORT_JUMP(Func_82a79)
{
	gb_call(gb, ADDR_GetUnlockedOWLevelData);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWPathShape, 0);
	wr8(gb, w2d072, 0);
	SET_HL(gb, wCurSceneObj);
	SET_BC(gb, 0x8);
	CALL(WriteAToHL_BCTimes);
	JUMP_TO(ADDR_Func_82a8d); // fallthrough
}

// output: carry if up was pressed to enter the level
PORT(Func_81077)
{
	SET_HL(gb, wMapSideLevelID);
	gb->cpu.a = alu_or(gb, rd8(gb, wNextMapSide), rd8(gb, wMapSideLevelID));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_UP))
		return;
	gb->cpu.a = rd8(gb, wNextTreasure);
	alu_cp(gb, gb->cpu.a, TREASURES_END);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wGameModeFlags);
		if (alu_bit(gb, gb->cpu.a, MODE_GAME_CLEARED_F))
			return;
	}
	CALL(Func_3c76);
	gb->cpu.a = OWWARIO_UP;
	gb_call(gb, ADDR_SetOWWarioOAM);
	play_sfx(gb, SFX_113);
	gb->cpu.a = 0x0a;
	wr8(gb, w2d013, gb->cpu.a);
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | FLAG_C); // scf
}

// a = d-pad pressed, with only up/down if either is
PORT(Func_810a9)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadPressed), PAD_CTRL_PAD);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_rla(gb, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = alu_rla(gb, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.a = gb->cpu.b;
			return;
		}
	}
	gb->cpu.a = alu_and(gb, PAD_UP | PAD_DOWN, gb->cpu.b);
}

PORT(Func_82bb8)
{
	gb->cpu.a = rd8(gb, wOWSceneAction);
	alu_cp(gb, gb->cpu.a, UNLOCK_LEVEL);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = alu_and(gb, rd8(gb, wCurSceneObjState), rd8(gb, wCurSceneObjState));
	if (gb->cpu.f & FLAG_Z)
		return;
	SET_DE(gb, ADDR_Frameset_aa544);
	SET_HL(gb, wCurSceneObjDuration);
	gb->cpu.b = BANK_Frameset_aa544;
	CALL(UpdateOWAnimation);
	SET_HL(gb, wCurSceneObj);
	SET_DE(gb, ADDR_OAM_aa445);
	gb->cpu.b = BANK_OAM_aa445;
	CALL(LoadOWSpriteWithScroll);
}

PORT(InitHighlightLevelScene)
{
	gb->cpu.a = rd8(gb, w2d065);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	SET_HL(gb, ADDR_Pals_848e0);
	SET_DE(gb, wTempPals2 + 4 * PAL_SIZE);
	gb->cpu.c = 2 * PAL_SIZE;
	gb->cpu.b = BANK_Pals_848e0;
	CALL(CopyFarBytes);
}

// Func_82208.asm_82234: z = add w2d0a5 to wOWYCoord1, nz = w2d0a6 to the x
static void func_82234(GB *gb)
{
	uint16_t hl = wOWYCoord1, de = w2d0a5;
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.e = alu_inc(gb, (uint8_t)de);
		de = (uint16_t)((de & 0xff00) | gb->cpu.e);
		hl = (uint16_t)((hl & 0xff00) | alu_inc(gb, (uint8_t)hl));
	}
	SET_DE(gb, de);
	SET_HL(gb, hl);
	gb->cpu.a = alu_add(gb, rd8(gb, de), rd8(gb, hl));
	wr8(gb, hl, gb->cpu.a);
}

PORT(Func_82208)
{
	SET_HL(gb, w2d0a1);
	gb->cpu.a = alu_and(gb, rd8(gb, w2d0a1), rd8(gb, w2d0a1));
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	wr8(gb, w2d0a1, gb->cpu.a);
	gb->cpu.a = rd8(gb, w2d0a2);
	gb->cpu.d = gb->cpu.a;
	SET_HL(gb, w2d0a3);
	gb->cpu.a = alu_add(gb, rd8(gb, w2d0a4), rd8(gb, w2d0a3));
	wr8(gb, w2d0a3, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, gb->cpu.d);
	if (!(gb->cpu.f & FLAG_C)) {
		// .Func_82224
		gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.d);
		wr8(gb, REG_HL(gb), gb->cpu.a);
		gb->cpu.a = alu_and(gb, rd8(gb, w2d0a0), 0x1);
		gb->cpu.a = alu_xor(gb, gb->cpu.a, 0x1);
		func_82234(gb);
	}
	// .Func_8222f
	gb->cpu.a = alu_and(gb, rd8(gb, w2d0a0), 0x1);
	func_82234(gb);
}

PORT_JUMP(Func_82041)
{
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, w2d068);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	bool reset = gb->cpu.f & FLAG_Z;
	if (!reset) {
		SET_HL(gb, w2d10d);
		gb->cpu.a = alu_and(gb, rd8(gb, w2d10d), rd8(gb, w2d10d));
		if (gb->cpu.f & FLAG_Z)
			gb_call(gb, ADDR_PlaySFX_12A);
		uint16_t hl = REG_HL(gb);
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
		gb->cpu.a = alu_rlca(gb, rd8(gb, w2d10e));
		gb->cpu.c = 0x24;
		if (gb->cpu.f & FLAG_C) {
			gb->cpu.c = alu_srl(gb, gb->cpu.c);
			gb->cpu.c = alu_srl(gb, gb->cpu.c);
		}
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, gb->cpu.c);
		reset = !(gb->cpu.f & FLAG_C);
	}
	if (reset) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, w2d10d, 0);
	}
	// .asm_82068
	gb->cpu.a = rd8(gb, w2d10c);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = rd8(gb, wLevelArrowDirections1);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_Func_8208d);
		return;
	}
	wr8(gb, w2d10c, gb->cpu.a);
	gb->cpu.c = OWWARIO_UP;
	if (!alu_bit(gb, gb->cpu.a, 6)) {
		gb->cpu.c = OWWARIO_RIGHT;
		if (!alu_bit(gb, gb->cpu.a, 4)) {
			gb->cpu.c = OWWARIO_LEFT;
			if (!alu_bit(gb, gb->cpu.a, 5))
				gb->cpu.c = OWWARIO_DOWN;
		}
	}
	gb->cpu.a = gb->cpu.c;
	gb_call(gb, ADDR_SetOWWarioOAM);
	JUMP_TO(ADDR_Func_8208d); // fallthrough
}

// GetOWAllowedDPadInput.CheckAllowedDirection: b = direction, c |= b if
// the tile at hl is a path (1 or 2) with BG_BANK1 in its attribute
static void check_allowed_direction(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x02);
	if (!(gb->cpu.f & FLAG_Z)) {
		alu_cp(gb, gb->cpu.a, 0x01);
		if (!(gb->cpu.f & FLAG_Z))
			return;
	}
	gb->cpu.e = gb->cpu.l;
	gb->cpu.d = gb->cpu.h;
	gb->cpu.d = alu_dec(gb, gb->cpu.d);
	gb->cpu.d = alu_dec(gb, gb->cpu.d);
	gb->cpu.d = alu_dec(gb, gb->cpu.d);
	gb->cpu.a = rd8(gb, REG_DE(gb));
	if (!alu_bit(gb, gb->cpu.a, B_BG_BANK1))
		return;
	gb->cpu.a = alu_or(gb, gb->cpu.c, gb->cpu.b);
	gb->cpu.c = gb->cpu.a;
}

PORT(GetOWAllowedDPadInput)
{
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	alu_cp(gb, gb->cpu.a, OWNORTH_JUNCTION);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = PAD_RIGHT | PAD_UP | PAD_DOWN;
		wr8(gb, wOWAllowedDPadInput, gb->cpu.a);
		return;
	}
	SET_HL(gb, w2d069);
	CALL(GetOWCoordInTilemap);
	gb->cpu.c = 0x00;
	SET_HL(gb, REG_HL(gb) + 1);
	gb->cpu.b = PAD_RIGHT;
	check_allowed_direction(gb);
	SET_HL(gb, REG_HL(gb) - 2);
	gb->cpu.b = PAD_LEFT;
	check_allowed_direction(gb);
	SET_HL(gb, REG_HL(gb) + 1);
	SET_DE(gb, (uint16_t)-TILEMAP_WIDTH);
	SET_HL(gb, alu_add16(gb, REG_HL(gb), REG_DE(gb)));
	gb->cpu.b = PAD_UP;
	check_allowed_direction(gb);
	SET_DE(gb, 2 * TILEMAP_WIDTH);
	SET_HL(gb, alu_add16(gb, REG_HL(gb), REG_DE(gb)));
	gb->cpu.b = PAD_DOWN;
	check_allowed_direction(gb);
	gb->cpu.a = gb->cpu.c;
	wr8(gb, wOWAllowedDPadInput, gb->cpu.a);
}

// rrca: bit 0 into bit 7 and carry, z clear
static uint8_t rrca(GB *gb, uint8_t v)
{
	gb->cpu.f = (v & 1) ? FLAG_C : 0;
	return (uint8_t)(v >> 1 | v << 7);
}

PORT_JUMP(Func_820e6)
{
	gb_call(gb, ADDR_Func_82111);
	gb->cpu.a = alu_swap(gb, rd8(gb, wLevelArrowDirections1));
	uint16_t hl = REG_HL(gb);
	int i;
	for (i = 0; i < 4; i++) {
		if (i)
			hl++;
		gb->cpu.a = rrca(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_C)
			break;
	}
	SET_HL(gb, hl);
	if (i == 4) {
		JUMP_TO(ADDR_Func_820e6 + 0x1a - 3); // debug_nop
		return;
	}
	// .asm_82100
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, w2d068, gb->cpu.a);
	SET_DE(gb, 0x4);
	hl = alu_add16(gb, hl, 0x4);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wOWPathShape, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d072, 0);
	RET();
}

// output: b = map side, d = level
PORT(GetMapSideAndLevelForNextTreasure)
{
	gb->cpu.a = rd8(gb, wNextTreasure);
	alu_cp(gb, gb->cpu.a, TREASURES_END);
	bool get_level = false;
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.b = gb->cpu.a;
		gb_call(gb, ADDR_GetEventWithTreasure);
		gb->cpu.a = EVENT_GOLD_MUSIC_BOX;
		alu_cp(gb, gb->cpu.a, gb->cpu.c);
		if (!(gb->cpu.f & FLAG_C)) {
			get_level = true;
		} else {
			gb->cpu.a = rd8(gb, wGameModeFlags);
			get_level = alu_bit(gb, gb->cpu.a, MODE_GAME_CLEARED_F);
		}
	}
	if (!get_level) {
		gb->cpu.b = NORTH;
		gb->cpu.d = OWNORTH_THE_TEMPLE;
		return;
	}
	gb->cpu.a = rd8(gb, wNextTreasure);
	gb->cpu.c = gb->cpu.a;
	FARCALL(GetLevelThatContainsTreasure);
	gb_call(gb, ADDR_GetMapLevelID);
}

PORT(UpdateNextPrevMapButtonsSelectable)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wOWBarsState), rd8(gb, wOWBarsState));
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = alu_and(gb, rd8(gb, wTopBarSelection), TOPBAR_NEXT_MAP | TOPBAR_PREV_MAP);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.c = gb->cpu.a;
	gb_call(gb, ADDR_GetAccessibleMapFlags);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.c);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	SET_HL(gb, wTopBarSelection);
	uint8_t v = rd8(gb, wTopBarSelection);
	if (alu_bit(gb, v, TOPBAR_NEXT_MAP_F))
		v = (uint8_t)((v & ~TOPBAR_NEXT_MAP) | TOPBAR_PREV_MAP);
	else
		v = (uint8_t)((v & ~TOPBAR_PREV_MAP) | TOPBAR_NEXT_MAP);
	wr8(gb, wTopBarSelection, v);
}

PORT_JUMP(Func_82a4e)
{
	FARCALL(FadeInLevelName);
	gb->cpu.c = alu_srl(gb, gb->cpu.c);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_82a8d);
}

PORT_JUMP(DrawBottomBar8Coin)
{
	gb->cpu.a = rd8(gb, wBottomBarFlags);
	if (!alu_bit(gb, gb->cpu.a, 7)) {
		RET();
		return;
	}
	SET_HL(gb, wCompassObjState);
	SET_DE(gb, ADDR_Frameset_aa2fb);
	gb->cpu.b = BANK_Frameset_aa2fb;
	CALL(UpdateOWAnimation);
	gb->cpu.d = 0x90;
	gb->cpu.a = rd8(gb, wCompassObjDuration);
	SET_HL(gb, wCompassObjDuration + 1);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.c = 0x00;
	JUMP_TO(ADDR_LoadSpriteInsideWindow_Low);
}

PORT(Func_82bda)
{
	gb->cpu.a = rd8(gb, wOWSceneAction);
	alu_cp(gb, gb->cpu.a, HIGHLIGHT_LEVEL);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = alu_and(gb, rd8(gb, wCurSceneObjState), rd8(gb, wCurSceneObjState));
	if (gb->cpu.f & FLAG_Z)
		return;
	SET_HL(gb, wCurSceneObjDuration);
	SET_DE(gb, ADDR_Frameset_aa555);
	gb->cpu.b = BANK_Frameset_aa555;
	CALL(UpdateOWAnimation);
	gb->cpu.a = rd8(gb, wOWAnimationFinished);
	alu_cp(gb, gb->cpu.a, 0xff);
	if (gb->cpu.f & FLAG_Z) {
		SET_HL(gb, wCurSceneObjState);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurSceneObjState, 0);
		return;
	}
	SET_HL(gb, wCurSceneObj);
	SET_DE(gb, ADDR_OAM_aa445);
	gb->cpu.b = BANK_OAM_aa445;
	CALL(LoadOWSpriteWithScroll);
}

// output: carry if a level was selected
PORT(Func_81240)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wOWPalTransitionState), rd8(gb, wOWPalTransitionState));
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wOWLevel);
		alu_cp(gb, gb->cpu.a, 0x80);
		if (!(gb->cpu.f & FLAG_Z)) {
			gb->cpu.a = rd8(gb, wJoypadPressed);
			if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
				gb_call(gb, ADDR_LoadMapLevelFromLevelIndex);
				gb->cpu.a = alu_and(gb, rd8(gb, wOWLevel), rd8(gb, wOWLevel));
				if (gb->cpu.f & FLAG_Z) {
					// .temple
					play_sfx(gb, SFX_SELECTION);
					gb->cpu.a = SST_OVERWORLD_1A;
					wr8(gb, wSubState, gb->cpu.a);
				} else {
					gb->cpu.a = 0;
					gb->cpu.f = FLAG_Z; // xor a
					wr8(gb, wSubState, 0);
					SET_HL(gb, wState);
					wr8(gb, wState, alu_inc(gb, rd8(gb, wState)));
				}
				gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | FLAG_C); // scf
				return;
			}
		}
	}
	// .no_carry
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
}

PORT(CheckIfTreasureUnlocksEvent)
{
	gb->cpu.a = rd8(gb, wLastTransitionParam);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	gb->cpu.c = 0;
	wr8(gb, wCurEvent, 0);
	uint16_t hl = ADDR_EventTreasures;
	for (;;) {
		gb->cpu.a = rd8(gb, hl++);
		alu_cp(gb, gb->cpu.a, TREASURES_END);
		if (gb->cpu.f & FLAG_Z) {
			SET_HL(gb, hl);
			break; // .no_event_unlocked
		}
		bool found = false;
		for (int i = 0; i < 3; i++) {
			if (i)
				gb->cpu.a = rd8(gb, hl++);
			alu_cp(gb, gb->cpu.a, gb->cpu.b);
			if (gb->cpu.f & FLAG_Z) {
				found = true;
				break;
			}
		}
		if (found) {
			SET_HL(gb, hl);
			gb->cpu.a = gb->cpu.c;
			wr8(gb, wCurEvent, gb->cpu.a);
			gb_call(gb, ADDR_CheckIfEventIsUnlocked);
			if (!(gb->cpu.f & FLAG_Z))
				return;
			break; // .no_event_unlocked
		}
		gb->cpu.c = alu_inc(gb, gb->cpu.c);
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurEvent, 0);
}

// a = event: ors its CheckIfEventIsUnlocked result, rotated left n times,
// into wAccessibleMapSides
static void add_accessible_map_side(GB *gb, uint8_t event, int n)
{
	gb->cpu.a = event;
	gb_call(gb, ADDR_CheckIfEventIsUnlocked);
	for (int i = 0; i < n; i++)
		gb->cpu.a = alu_rlca(gb, gb->cpu.a);
	SET_HL(gb, wAccessibleMapSides);
	gb->cpu.a = alu_or(gb, gb->cpu.a, rd8(gb, wAccessibleMapSides));
	wr8(gb, wAccessibleMapSides, gb->cpu.a);
}

PORT(SetNextPrevMapButtonsSelectable)
{
	// .CheckAccessibleMapSides
	add_accessible_map_side(gb, EVENT_OPEN_NORTH_GATE, 3);
	add_accessible_map_side(gb, EVENT_FIX_ELEVATOR, 2);
	add_accessible_map_side(gb, EVENT_BLOW_MIST, 1);
	add_accessible_map_side(gb, EVENT_TORCH_FOREST, 0);
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x1);
	gb->cpu.a = alu_swap(gb, gb->cpu.a);
	gb->cpu.a = alu_or(gb, gb->cpu.a, rd8(gb, wAccessibleMapSides));
	wr8(gb, wAccessibleMapSides, gb->cpu.a);

	gb_call(gb, ADDR_GetAccessibleMapFlags);
	SET_HL(gb, wTopBarSelectableButtons);
	gb->cpu.a = alu_or(gb, gb->cpu.a, rd8(gb, wTopBarSelectableButtons));
	wr8(gb, wTopBarSelectableButtons, gb->cpu.a);
}

PORT(GetNextTreasureToCollect)
{
	gb->cpu.a = rd8(gb, wNextTreasure);
	CALL(IsTreasureCollected);
	SET_HL(gb, wNextTreasure);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = alu_and(gb, rd8(gb, wTransitionParam), rd8(gb, wTransitionParam));
		if (gb->cpu.f & FLAG_Z)
			return;
		static const uint8_t skip[] = {
			TRANSITION_RETURN_TO_MAP, TRANSITION_EPILOGUE_NOT_PERFECT,
			TRANSITION_EPILOGUE_PERFECT, TRANSITION_GAME_OVER,
		};
		for (int i = 0; i < 4; i++) {
			alu_cp(gb, gb->cpu.a, skip[i]);
			if (gb->cpu.f & FLAG_Z)
				return;
		}
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z))
			gb->cpu.a = alu_dec(gb, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, rd8(gb, wNextTreasure));
		if (!(gb->cpu.f & FLAG_Z))
			return;
	}
	// .collected
	gb->cpu.b = rd8(gb, wNextTreasure);
	gb_call(gb, ADDR_GetEventWithTreasure);
	for (;;) {
		// .next_treasure
		uint16_t hl = REG_HL(gb);
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, INVALID_TREASURE);
		if (!(gb->cpu.f & FLAG_Z)) {
			gb_push16(gb, hl);
			CALL(IsTreasureCollected);
			SET_HL(gb, gb_pop16(gb));
			if (gb->cpu.f & FLAG_Z) {
				// .not_collected
				gb->cpu.a = rd8(gb, REG_HL(gb));
				wr8(gb, wNextTreasure, gb->cpu.a);
				return;
			}
		}
		// .check_end
		hl = REG_HL(gb);
		gb->cpu.a = rd8(gb, hl++);
		SET_HL(gb, hl);
		alu_cp(gb, gb->cpu.a, TREASURES_END);
		if (gb->cpu.f & FLAG_Z)
			break;
	}
	// .all_collected
	gb->cpu.a = TREASURES_END;
	wr8(gb, wNextTreasure, gb->cpu.a);
}

// ld a, BOTBAR_CLOSING / ld [wBottomBarAction], a / xor a / ld [wBottomBarState], a
static void close_bottom_bar(GB *gb)
{
	gb->cpu.a = BOTBAR_CLOSING;
	wr8(gb, wBottomBarAction, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wBottomBarState, 0);
}

// HandleBottomBar.CloseBottomBar
static void bottom_bar_close(GB *gb)
{
	gb->cpu.a = rd8(gb, wBottomBarState);
	JUMP_TO(jumptable(gb, ADDR_HandleBottomBar + 0x11e));
}

// HandleBottomBar.OpenBottomBar
static void bottom_bar_open(GB *gb)
{
	gb->cpu.a = rd8(gb, wJoypadDown);
	if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		gb->cpu.a = rd8(gb, wBottomBarState);
		JUMP_TO(jumptable(gb, ADDR_HandleBottomBar + 0x94));
		return;
	}
	// .no_b_btn
	SET_HL(gb, wBottomBarFlags);
	wr8(gb, wBottomBarFlags, (uint8_t)(rd8(gb, wBottomBarFlags) & ~(1 << 4)));
	close_bottom_bar(gb);
	RET();
}

PORT_JUMP(HandleBottomBar)
{
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, w2d068);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (!(gb->cpu.f & FLAG_Z)) {
		// .asm_82788
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, w2d096, 0);
		gb->cpu.a = alu_and(gb, rd8(gb, wBottomBarAction), rd8(gb, wBottomBarAction));
		if (gb->cpu.f & FLAG_Z) {
			RET();
			return;
		}
		alu_cp(gb, gb->cpu.a, BOTBAR_CLOSING);
		if (gb->cpu.f & FLAG_Z) {
			bottom_bar_close(gb);
			return;
		}
		close_bottom_bar(gb);
		wr8(gb, wMagnifyingGlassCounter, 0);
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wBottomBarAction);
	alu_cp(gb, gb->cpu.a, BOTBAR_CLOSING);
	if (gb->cpu.f & FLAG_Z) {
		bottom_bar_close(gb);
		return;
	}
	alu_cp(gb, gb->cpu.a, BOTBAR_OPENING);
	if (gb->cpu.f & FLAG_Z) {
		bottom_bar_open(gb);
		return;
	}
	// .CheckBottomBarPossible
	gb->cpu.a = alu_add(gb, rd8(gb, wOWLevel), rd8(gb, wOWLevel));
	bool possible = false;
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = rd8(gb, w2d013);
		alu_cp(gb, gb->cpu.a, 0x05);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
			possible = true;
		}
	}
	if (!possible) {
		gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | FLAG_C); // scf
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wBottomBarAction);
	alu_cp(gb, gb->cpu.a, BOTBAR_OPENED);
	if (gb->cpu.f & FLAG_Z) {
		// .HandleOpenedBottomBar
		gb->cpu.a = rd8(gb, wJoypadDown);
		if (!alu_bit(gb, gb->cpu.a, B_PAD_B)) {
			SET_HL(gb, wBottomBarFlags);
			wr8(gb, wBottomBarFlags, (uint8_t)(rd8(gb, wBottomBarFlags) & ~(1 << 4)));
			close_bottom_bar(gb);
		}
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, BOTBAR_CLOSED);
	if (gb->cpu.f & FLAG_Z) {
		// .HandleClosedBottomBar
		gb->cpu.a = alu_and(gb, rd8(gb, wHasMagnifyingGlass), rd8(gb, wHasMagnifyingGlass));
		if (!(gb->cpu.f & FLAG_Z)) {
			SET_HL(gb, wMagnifyingGlassCounter);
			gb->cpu.a = rd8(gb, wJoypadDown);
			if (!alu_bit(gb, gb->cpu.a, B_PAD_B)) {
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z; // xor a
				wr8(gb, wMagnifyingGlassCounter, 0);
			} else {
				// .b_btn_down
				wr8(gb, wMagnifyingGlassCounter, alu_inc(gb, rd8(gb, wMagnifyingGlassCounter)));
				gb->cpu.a = rd8(gb, wMagnifyingGlassCounter);
				alu_cp(gb, gb->cpu.a, 10);
				if (!(gb->cpu.f & FLAG_C)) {
					gb->cpu.a = BOTBAR_OPENING;
					wr8(gb, wBottomBarAction, gb->cpu.a);
					gb->cpu.a = 0;
					gb->cpu.f = FLAG_Z; // xor a
					wr8(gb, wBottomBarState, 0);
				}
			}
		}
	}
	RET();
}

PORT_LOCAL(HandleBottomBar, InitBar)
{
	wr8(gb, wBottomBarFlags, (uint8_t)(rd8(gb, wBottomBarFlags) | (1 << 4)));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wHDMA, 0);
	wr8(gb, w2d0b5, 0);
	SET_HL(gb, wBottomBarState);
	wr8(gb, wBottomBarState, alu_inc(gb, rd8(gb, wBottomBarState)));
}

PORT_LOCAL(HandleBottomBar, OpenWindow)
{
	SET_HL(gb, wWY);
	gb->cpu.b = rd8(gb, wWY);
	gb->cpu.a = 0x78;
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C) {
		for (int i = 0; i < 6; i++)
			wr8(gb, wWY, alu_dec(gb, rd8(gb, wWY)));
		return;
	}
	// .cap_min_wy
	wr8(gb, wWY, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wMagnifyingGlassCounter, 0);
	SET_HL(gb, wBottomBarState);
	wr8(gb, wBottomBarState, alu_inc(gb, rd8(gb, wBottomBarState)));
}

PORT_LOCAL(HandleBottomBar, SetBottomBarAsOpen)
{
	gb->cpu.a = BOTBAR_OPENED;
	wr8(gb, wBottomBarAction, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wBottomBarState, 0);
	SET_HL(gb, wBottomBarFlags);
	if (!alu_bit(gb, rd8(gb, wBottomBarFlags), 6))
		return;
	wr8(gb, wBottomBarFlags, (uint8_t)(rd8(gb, wBottomBarFlags) | (1 << 7)));
}

PORT_LOCAL(HandleBottomBar, CloseWindow)
{
	SET_HL(gb, wWY);
	gb->cpu.a = rd8(gb, wWY);
	alu_cp(gb, gb->cpu.a, SCREEN_HEIGHT_PX);
	if (gb->cpu.f & FLAG_C) {
		for (int i = 0; i < 6; i++)
			wr8(gb, wWY, alu_inc(gb, rd8(gb, wWY)));
		return;
	}
	// .cap_max_wy
	wr8(gb, wWY, SCREEN_HEIGHT_PX);
	SET_HL(gb, wBottomBarState);
	wr8(gb, wBottomBarState, alu_inc(gb, rd8(gb, wBottomBarState)));
}

PORT_LOCAL(HandleBottomBar, DeinitBar)
{
	SET_HL(gb, wBottomBarFlags);
	gb->cpu.a = alu_and(gb, 1 << 6, rd8(gb, wBottomBarFlags));
	wr8(gb, wBottomBarFlags, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCompassObjState, 0);
	wr8(gb, wCompassObjUnk7, 0);
	wr8(gb, wBottomBarAction, 0);
	wr8(gb, wBottomBarState, 0);
	wr8(gb, wMagnifyingGlassCounter, 0);
}

PORT(Func_81055)
{
	FARCALL(Func_b4a3d);
	gb_call(gb, ADDR_Func_82041);
	FARCALL(Func_b4aa9);
}

PORT_JUMP(Func_80e89)
{
	CALL(ProcessDPadRepeat);
	FARCALL(UpdateMapSideOWAnimations);
	FARCALL(UpdateCommonOWAnimations);
	gb_call(gb, ADDR_UpdateTopBar);
	gb->cpu.a = alu_and(gb, rd8(gb, wOWBarsState), rd8(gb, wOWBarsState));
	if (!(gb->cpu.f & FLAG_Z)) {
		JUMP_TO(ADDR_Func_81055);
		return;
	}
	gb_call(gb, ADDR_HandleBottomBar);
	gb_call(gb, ADDR_DrawBottomBar);
	FARCALL(Func_852e5);
	SET_HL(gb, w2d014);
	wr8(gb, w2d014, alu_inc(gb, rd8(gb, w2d014)));
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Func_80ee7 - 11 * 2));
}

PORT_JUMP(Func_818ad)
{
	FARCALL(UpdateCommonOWAnimations);
	FARCALL(UpdateMapSideOWAnimations);
	gb->cpu.a = alu_and(gb, rd8(gb, wOWBarsState), rd8(gb, wOWBarsState));
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = rd8(gb, wJoypadPressed);
		if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
			JUMP_TO(ADDR_Func_81931);
			return;
		}
	}
	// .no_top_bar
	SET_HL(gb, w2d014);
	wr8(gb, w2d014, alu_inc(gb, rd8(gb, w2d014)));
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Func_818e7 - 3 * 2));
}

PORT_JUMP(Func_8195d)
{
	gb->cpu.a = rd8(gb, wOWSceneAction);
	JUMP_TO(jumptable(gb, ADDR_Func_8195d + 4));
}

PORT_JUMP(Func_818f6)
{
	gb_call(gb, ADDR_Func_8195d);
	gb->cpu.a = alu_and(gb, rd8(gb, wOWSceneActionParam), rd8(gb, wOWSceneActionParam));
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_818ed);
}

PORT_JUMP(UnlockLevel)
{
	SET_HL(gb, wOWFuncCounter);
	wr8(gb, wOWFuncCounter, alu_inc(gb, rd8(gb, wOWFuncCounter)));
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_UnlockLevel + 8));
}

PORT(DoOWFunction)
{
	FARCALL(_DoOWFunction);
}

PORT(InitOWObjects)
{
	FARCALL(_InitOWObjects);
}

PORT(Func_80e55)
{
	FARCALL(Func_adbfe);
}

// ld hl, w2d013 / inc [hl]
static void inc_2d013(GB *gb)
{
	SET_HL(gb, w2d013);
	wr8(gb, w2d013, alu_inc(gb, rd8(gb, w2d013)));
}

PORT(Func_80ee7)
{
	gb_call(gb, ADDR_Func_8212c);
	gb_call(gb, ADDR_GetOWAllowedDPadInput);
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	wr8(gb, wLevel2, gb->cpu.a);
	wr8(gb, w2d068, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPalConfig1TotalSteps, 0);
	wr8(gb, w2d0e0, 0);
	wr8(gb, wOWPalTransitionState, 0);
	gb_call(gb, ADDR_Func_81055);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d014, 0);
	inc_2d013(gb);
}

PORT(Func_80f0d)
{
	gb->cpu.a = rd8(gb, wDayNight);
	if (!alu_bit(gb, gb->cpu.a, 7)) {
		CALL(PlayOverworldMusic);
		gb_call(gb, ADDR_Func_81055);
		inc_2d013(gb);
		return;
	}
	FARCALL(DayNightTransition);
	gb_call(gb, ADDR_Func_81055);
	gb->cpu.a = alu_and(gb, rd8(gb, wOWPalTransitionState), rd8(gb, wOWPalTransitionState));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.ime = false; // di
	gb_call(gb, ADDR_VBlank_Overworld);
	gb->cpu.ime = true; // ei
	gb->cpu.ime_delay = 0;
}

PORT(Func_80fd6)
{
	gb_call(gb, ADDR_Func_81055);
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	alu_cp(gb, gb->cpu.a, OW_EXITS);
	if (gb->cpu.f & FLAG_C) {
		FARCALL(FadeInLevelName);
		gb->cpu.c = alu_srl(gb, gb->cpu.c);
		if (gb->cpu.f & FLAG_C)
			return;
	}
	inc_2d013(gb);
}

PORT(Func_80fb8)
{
	gb_call(gb, ADDR_Func_81055);
	gb->cpu.a = alu_and(gb, rd8(gb, w2d0e0), rd8(gb, w2d0e0));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb_call(gb, ADDR_LoadLevelNameIfValid);
	inc_2d013(gb);
}

// a = level arrow directions
PORT(Func_81034)
{
	wr8(gb, wLevelArrowDirections1, gb->cpu.a);
	gb_call(gb, ADDR_Func_820e6);
	gb->cpu.a = rd8(gb, w2d068);
	wr8(gb, wLevel2, gb->cpu.a);
	gb->cpu.a = 0x03;
	wr8(gb, w2d0e0, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPalConfig1TotalSteps, 0);
	gb_call(gb, ADDR_Func_81055);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d014, 0);
	inc_2d013(gb);
}

PORT_JUMP(Func_80ff7)
{
	gb_call(gb, ADDR_Func_81240);
	if (gb->cpu.f & FLAG_C)
		goto func_81055;
	gb->cpu.a = alu_and(gb, rd8(gb, wOWPalTransitionState), rd8(gb, wOWPalTransitionState));
	if (!(gb->cpu.f & FLAG_Z))
		goto func_81055;
	gb->cpu.a = rd8(gb, wJoypadDown);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		gb->cpu.a = rd8(gb, wJoypadPressed);
		if (alu_bit(gb, gb->cpu.a, B_PAD_SELECT)) {
			SET_HL(gb, wOWBarsState);
			wr8(gb, wOWBarsState, alu_inc(gb, rd8(gb, wOWBarsState)));
			goto func_81055;
		}
	}
	// .asm_81016
	gb_call(gb, ADDR_Func_81477);
	gb->cpu.a = alu_and(gb, rd8(gb, w2d0e3), rd8(gb, w2d0e3));
	if (!(gb->cpu.f & FLAG_Z))
		goto func_81055;
	gb_call(gb, ADDR_Func_81077);
	if (gb->cpu.f & FLAG_C)
		goto func_81055;
	gb_call(gb, ADDR_Func_80de0);
	if (gb->cpu.f & FLAG_C)
		goto func_81055;
	gb_call(gb, ADDR_Func_810a9);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, rd8(gb, wOWAllowedDPadInput), gb->cpu.b);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		goto func_81055;
	JUMP_TO(ADDR_Func_81034); // fallthrough
	return;
func_81055:
	JUMP_TO(ADDR_Func_81055);
}

PORT_JUMP(Func_82a0a)
{
	FARCALL(OWGreyTransition);
	gb->cpu.a = alu_and(gb, rd8(gb, wOWPalTransitionState), rd8(gb, wOWPalTransitionState));
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.ime = false; // di
	gb_call(gb, ADDR_VBlank_OWScene);
	gb->cpu.ime = true; // ei
	gb->cpu.ime_delay = 0;
	JUMP_TO(ADDR_Func_82a8d);
}

PORT_JUMP(Func_82ad2)
{
	gb_call(gb, ADDR_Func_81feb);
	gb_call(gb, ADDR_Func_81f21);
	JUMP_TO(ADDR_Func_82a8d);
}

PORT_JUMP(Func_82b22)
{
	gb->cpu.a = rd8(gb, wLevel2);
	alu_cp(gb, gb->cpu.a, OW_EXIT_RIGHT);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_Func_82a8d);
		return;
	}
	SET_HL(gb, wOWCoords2);
	CALL(GetOWCoordInTilemap);
	SET_HL(gb, wAttrToPlaceInOW);
	gb_call(gb, ADDR_ShowLevelPointInOW);
	gb->cpu.a = 0x01;
	wr8(gb, w2d07f, gb->cpu.a);
	JUMP_TO(ADDR_Func_82ad2);
}

// shows the arrow to the connected level 1
static void show_connected_level_arrow(GB *gb)
{
	gb_call(gb, ADDR_GetConnectedLevel1ArrowCoord);
	gb_call(gb, ADDR_GetConnectedLevel1CoordsInTilemap);
	gb->cpu.a = rd8(gb, wLevelArrowDirections1);
	SET_HL(gb, wAttrToPlaceInOW);
	gb_call(gb, ADDR_ShowLevelArrowInOW);
	gb->cpu.a = TRUE;
	wr8(gb, w2d07f, gb->cpu.a);
	play_sfx(gb, SFX_103);
	JUMP_TO(ADDR_Func_82ad2);
}

PORT_JUMP(Func_82aae)
{
	gb_call(gb, ADDR_IsLeftExitOrJunction);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_Func_82a8d);
		return;
	}
	gb->cpu.a = rd8(gb, wLevelArrowDirections1);
	show_connected_level_arrow(gb); // falls into Func_82ad2
}

PORT_JUMP(Func_82aea)
{
	gb->cpu.a = rd8(gb, wLevel2);
	alu_cp(gb, gb->cpu.a, OW_EXIT_RIGHT);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_Func_82a8d);
		return;
	}
	gb->cpu.a = rd8(gb, wOWXCoord2);
	wr8(gb, wOWXCoord1, gb->cpu.a);
	gb->cpu.a = rd8(gb, wOWYCoord2);
	wr8(gb, wOWYCoord1, gb->cpu.a);
	gb->cpu.a = rd8(gb, wLevelArrowDirections2);
	wr8(gb, wLevelArrowDirections1, gb->cpu.a);
	show_connected_level_arrow(gb);
}

PORT_JUMP(Func_81477)
{
	gb->cpu.a = rd8(gb, wJoypadDown);
	if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		SET_HL(gb, w2d0e2);
		wr8(gb, w2d0e2, alu_inc(gb, rd8(gb, w2d0e2)));
		gb->cpu.a = rd8(gb, w2d0e3);
		JUMP_TO(jumptable(gb, ADDR_Func_81477 + 15));
		return;
	}
	// .asm_8148e
	gb->cpu.a = alu_and(gb, rd8(gb, w2d0e3), rd8(gb, w2d0e3));
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	gb->cpu.a = OWWARIO_DOWN;
	SET_HL(gb, w2d106);
	alu_cp(gb, gb->cpu.a, rd8(gb, w2d106));
	if (!(gb->cpu.f & FLAG_Z))
		gb_call(gb, ADDR_SetOWWarioOAM);
	gb->cpu.a = rd8(gb, w2d88e);
	wr8(gb, wOWYCoord1, gb->cpu.a);
	wr8(gb, w2d100, gb->cpu.a);
	SET_HL(gb, w2d88e + 1);
	gb->cpu.a = rd8(gb, w2d88e + 1);
	wr8(gb, wOWXCoord1, gb->cpu.a);
	wr8(gb, w2d101, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d0e3, 0);
	RET();
}

// stop_sfx
static void stop_sfx(GB *gb)
{
	wr8(gb, hSFXID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hSFXID + 1, 0x00);
}

PORT_JUMP(Func_81900)
{
	gb->cpu.a = rd8(gb, w2d014);
	alu_cp(gb, gb->cpu.a, 0x04);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	SET_HL(gb, wOWSceneParamsPtr);
	CALL(GetByteFromPointerInHL);
	gb_call(gb, ADDR_GetOWSceneParams_GotPtr);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_Func_81931);
		return;
	}
	gb->cpu.a = rd8(gb, wCurMapSide);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = rd8(gb, wOWSceneMapSide);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, w2d014, 0);
		wr8(gb, w2d013, 0);
	} else {
		stop_sfx(gb);
		gb->cpu.a = 0x04;
		wr8(gb, wSubState, gb->cpu.a);
	}
	RET();
}

PORT(Func_810bb)
{
	gb->cpu.a = rd8(gb, wOWYCoord1);
	wr8(gb, wOWYCoord2, gb->cpu.a);
	gb->cpu.a = rd8(gb, wOWXCoord1);
	wr8(gb, wOWXCoord2, gb->cpu.a);
	gb->cpu.a = rd8(gb, w2d068);
	gb_call(gb, ADDR_GetMapLevelCoords);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_add(gb, rd8(gb, hl++), gb->cpu.b);
	wr8(gb, w2d06f, gb->cpu.a);
	gb->cpu.a = alu_add(gb, rd8(gb, hl++), gb->cpu.c);
	wr8(gb, w2d070, gb->cpu.a);
	SET_HL(gb, hl);
	SET_BC(gb, wOWPathShape);
	gb_call(gb, ADDR_Func_810fc);
	gb_call(gb, ADDR_Func_821a1);
	gb_call(gb, ADDR_Func_81055);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d014, 0);
	inc_2d013(gb);
}

PORT(Func_81931)
{
	stop_sfx(gb);
	gb->cpu.a = rd8(gb, wCurEvent);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wLastTransitionParam, 0);
	wr8(gb, wCurEvent, 0);
	wr8(gb, wOWSceneAction, 0);
	wr8(gb, wTempOWSceneAction, 0);
	gb->cpu.a = EVENT_EPILOGUE;
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = SST_OVERWORLD_EPILOGUE;
		wr8(gb, wSubState, gb->cpu.a);
		return;
	}
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
	gb_call(gb, ADDR_Func_803e6);
}

// b = step, c = end x for the map exit at wMapSideLevelID; false (after
// jumping to the debug_nop at `debug`) if it isn't an exit
static bool get_exit_step(GB *gb, uint8_t right_c, uint8_t left_c, uint16_t debug)
{
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	gb->cpu.b = 0x01;
	gb->cpu.c = right_c;
	alu_cp(gb, gb->cpu.a, OW_EXIT_RIGHT);
	if (gb->cpu.f & FLAG_Z)
		return true;
	gb->cpu.b = 0xff;
	gb->cpu.c = left_c;
	alu_cp(gb, gb->cpu.a, OW_EXIT_LEFT);
	if (gb->cpu.f & FLAG_Z)
		return true;
	JUMP_TO(debug); // debug_nop
	return false;
}

PORT_JUMP(Func_811cf)
{
	gb->cpu.a = rd8(gb, w2d014);
	alu_cp(gb, gb->cpu.a, 0x02);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, REG_HL(gb), 0);
		if (!get_exit_step(gb, 0xa8, 0xf0, ADDR_Func_811cf + 0x1f - 3))
			return;
		// .is_exit
		gb->cpu.a = alu_add(gb, rd8(gb, w2d101), gb->cpu.b);
		wr8(gb, w2d101, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, gb->cpu.c);
		if (gb->cpu.f & FLAG_Z)
			inc_2d013(gb);
	}
	// .asm_811fc
	gb_call(gb, ADDR_Func_81055);
	RET();
}

PORT_JUMP(Func_81200)
{
	gb_call(gb, ADDR_Func_81055);
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	gb->cpu.b = 1;
	alu_cp(gb, gb->cpu.a, OW_EXIT_RIGHT);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.b = (uint8_t)-1;
		alu_cp(gb, gb->cpu.a, OW_EXIT_LEFT);
		if (!(gb->cpu.f & FLAG_Z)) {
			JUMP_TO(ADDR_Func_81200 + 0x15 - 3); // debug_nop
			return;
		}
	}
	// .is_exit
	gb->cpu.a = alu_add(gb, rd8(gb, wNextMapSide), gb->cpu.b);
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x3);
	wr8(gb, wNextMapSide, gb->cpu.a);
	gb->cpu.a = alu_add(gb, rd8(gb, wMapSideLevelID), gb->cpu.b);
	wr8(gb, wMapSideLevelID, gb->cpu.a);
	wr8(gb, w2d068, gb->cpu.a);
	gb->cpu.a = rd8(gb, wNextMapSide);
	bool sst_1e = false;
	if (!alu_bit(gb, gb->cpu.a, 1)) {
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.b);
		sst_1e = alu_bit(gb, gb->cpu.a, 1);
	}
	gb->cpu.a = sst_1e ? SST_OVERWORLD_1E : SST_OVERWORLD_08;
	wr8(gb, wSubState, gb->cpu.a);
	RET();
}

PORT_JUMP(DrawBottomBar)
{
	gb->cpu.a = rd8(gb, wGameModeFlags);
	if (alu_bit(gb, gb->cpu.a, MODE_TIME_ATTACK_F)) {
		JUMP_TO(ADDR_Func_82997);
		return;
	}
	static const uint16_t pals[] = {
		wLevelGreyTreasurePal, wLevelRedTreasurePal, wLevelGreenTreasurePal, wLevelBlueTreasurePal,
	};
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, pals[i]);
		gb->cpu.c = gb->cpu.a;
		gb->cpu.e = (uint8_t)i;
		gb->cpu.a = rd8(gb, wBottomBarFlags);
		if (!alu_bit(gb, gb->cpu.a, i)) {
			gb->cpu.e = 0x09;
			gb->cpu.c = 0x00;
		}
		gb->cpu.d = (uint8_t)(0x20 + i * 0x1c);
		gb_call(gb, ADDR_LoadSpriteInsideWindow_Low);
	}
	gb_call(gb, ADDR_DrawBottomBarButtonPrompt);
	gb_call(gb, ADDR_DrawBottomBar8Coin);
	RET();
}

PORT(Func_8115c)
{
	gb_call(gb, ADDR_Func_82208);
	SET_HL(gb, w2d10e);
	if (!alu_bit(gb, rd8(gb, w2d10e), 7)) {
		gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_CTRL_PAD);
		if (!(gb->cpu.f & FLAG_Z)) {
			wr8(gb, w2d10e, alu_inc(gb, rd8(gb, w2d10e)));
			gb->cpu.a = rd8(gb, w2d10e);
			alu_cp(gb, gb->cpu.a, 0x06);
			if (!(gb->cpu.f & FLAG_C))
				wr8(gb, w2d10e, (uint8_t)(rd8(gb, w2d10e) | (1 << 7)));
		}
		// .asm_81175
		alu_bit(gb, rd8(gb, w2d10e), 7);
	}
	// .asm_81177
	if (!(gb->cpu.f & FLAG_Z))
		gb_call(gb, ADDR_Func_82208);
	gb->cpu.a = rd8(gb, wOWYCoord1);
	wr8(gb, w2d100, gb->cpu.a);
	gb->cpu.a = rd8(gb, wOWXCoord1);
	wr8(gb, w2d101, gb->cpu.a);
	gb->cpu.a = alu_and(gb, rd8(gb, w2d0a1), rd8(gb, w2d0a1));
	if (gb->cpu.f & FLAG_Z) {
		SET_BC(gb, wOWPathShape);
		gb_call(gb, ADDR_Func_810fc);
		alu_cp(gb, gb->cpu.a, 0xff);
		if (gb->cpu.f & FLAG_Z) {
			// .asm_811a0
			gb->cpu.a = rd8(gb, w2d068);
			wr8(gb, wMapSideLevelID, gb->cpu.a);
			alu_cp(gb, gb->cpu.a, OW_EXITS);
			if (gb->cpu.f & FLAG_C) {
				gb->cpu.a = 0x02;
				wr8(gb, w2d013, gb->cpu.a);
			} else {
				// stop_music2
				gb->cpu.a = 0xff;
				wr8(gb, hMusicIDBackup + 0, 0xff);
				wr8(gb, hMusicID + 0, 0xff);
				gb->cpu.a = 0x00;
				wr8(gb, hMusicIDBackup + 1, 0x00);
				wr8(gb, hMusicID + 1, 0x00);
				play_sfx(gb, SFX_0EB);
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z; // xor a
				wr8(gb, w2d014, 0);
				inc_2d013(gb);
			}
		} else {
			gb_call(gb, ADDR_Func_821a1);
			gb_call(gb, ADDR_Func_81142);
		}
	}
	// .asm_8119c
	gb_call(gb, ADDR_Func_81055);
}

PORT_JUMP(Func_80f3c)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wMagnifyingGlassCounter, 0);
	gb_call(gb, ADDR_Func_8212c);
	gb_call(gb, ADDR_GetOWAllowedDPadInput);
	gb->cpu.c = 0x00;
	gb->cpu.b = 0x00;
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	gb->cpu.c |= 1 << B_PAD_LEFT;
	alu_cp(gb, gb->cpu.a, OW_EXIT_RIGHT);
	bool exit = gb->cpu.f & FLAG_Z;
	if (!exit) {
		gb->cpu.c = gb->cpu.b;
		gb->cpu.c |= 1 << B_PAD_RIGHT;
		alu_cp(gb, gb->cpu.a, OW_EXIT_LEFT);
		exit = gb->cpu.f & FLAG_Z;
	}
	if (exit) {
		gb->cpu.a = gb->cpu.c;
		wr8(gb, wLevelArrowDirections1, gb->cpu.a);
		gb_call(gb, ADDR_Func_820e6);
		gb_call(gb, ADDR_Func_81055);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, w2d014, 0);
		gb->cpu.a = 0x06;
		wr8(gb, w2d013, gb->cpu.a);
		RET();
		return;
	}
	gb_call(gb, ADDR_LoadMapLevelFromLevelIndex);
	FARCALL(Func_9aab5);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d014, 0);
	// .GetDirectionalInput
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_CTRL_PAD);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_rla(gb, gb->cpu.a);
	bool up_down = gb->cpu.f & FLAG_C;
	if (!up_down) {
		gb->cpu.a = alu_rla(gb, gb->cpu.a);
		up_down = gb->cpu.f & FLAG_C;
	}
	if (up_down)
		gb->cpu.a = alu_and(gb, PAD_UP | PAD_DOWN, gb->cpu.b);
	else
		gb->cpu.a = gb->cpu.b;
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, rd8(gb, wOWAllowedDPadInput), gb->cpu.b);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		SET_HL(gb, w2d013);
		wr8(gb, w2d013, 0x05);
		JUMP_TO(ADDR_Func_81034);
		return;
	}
	// .not_allowed
	gb_call(gb, ADDR_Func_81055);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wLevelArrowDirections1, 0);
	wr8(gb, w2d10e, 0);
	inc_2d013(gb);
	RET();
}

PORT_JUMP(UpdateTopBar)
{
	SET_HL(gb, wOWBarsStateCounter);
	wr8(gb, wOWBarsStateCounter, alu_inc(gb, rd8(gb, wOWBarsStateCounter)));
	gb->cpu.a = alu_and(gb, rd8(gb, wOWBarsState), rd8(gb, wOWBarsState));
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	JUMP_TO(jumptable(gb, ADDR_UpdateTopBar + 11));
}

// ld hl, wOWBarsState / inc [hl]
static void inc_ow_bars_state(GB *gb)
{
	SET_HL(gb, wOWBarsState);
	wr8(gb, wOWBarsState, alu_inc(gb, rd8(gb, wOWBarsState)));
}

PORT_LOCAL(UpdateTopBar, StartCloseBottomBar)
{
	// mark bottom bar as closing
	gb_call(gb, ADDR_DrawBottomBar);
	gb->cpu.a = BOTBAR_CLOSING;
	wr8(gb, wBottomBarAction, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wBottomBarState, 0);
	wr8(gb, wMagnifyingGlassCounter, 0);
	inc_ow_bars_state(gb);
}

PORT_LOCAL(UpdateTopBar, HandleBottomBarClose)
{
	// wait until bottom bar is fully closed
	gb_call(gb, ADDR_HandleBottomBar);
	gb_call(gb, ADDR_DrawBottomBar);
	gb->cpu.a = alu_and(gb, rd8(gb, wBottomBarAction), rd8(gb, wBottomBarAction));
	if (!(gb->cpu.f & FLAG_Z))
		return;

	// bottom bar was closed
	play_sfx(gb, SFX_0E9);

	// init top bar flags
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	gb->cpu.a |= 1 << TOPBAR_COLLECTION_F;
	wr8(gb, wTopBarSelection, gb->cpu.a);

	gb_call(gb, ADDR_UpdateTopBar + 0x72); // .InitTopBarButtons
	gb_call(gb, ADDR_ApplyTopBarButtonAttributes);

	gb->cpu.a = (uint8_t)rOBPI;
	wr8(gb, wPalConfig1Register, gb->cpu.a);
	gb->cpu.a = (uint8_t)((wTempPals2 + 4 * PAL_SIZE) >> 8);
	wr8(gb, wPalConfig1SourceHi, gb->cpu.a);
	gb->cpu.a = (uint8_t)(wTempPals2 + 4 * PAL_SIZE);
	wr8(gb, wPalConfig1SourceLo, gb->cpu.a);
	gb->cpu.a = OBPI_AUTOINC | 4 * PAL_SIZE;
	wr8(gb, wPalConfig1Index, gb->cpu.a);
	gb->cpu.a = 4; // number of pals
	wr8(gb, wPalConfig1Number, gb->cpu.a);
	inc_ow_bars_state(gb);
}

PORT_LOCAL(UpdateTopBar, InitTopBarButtons)
{
	gb_call(gb, ADDR_ShowTopBarSelectedButton);
	gb_call(gb, ADDR_LoadTopBarButtonAttributes);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWUIObj1YCoord, 0);
	wr8(gb, wOWUIObj2YCoord, 0);
	gb->cpu.a = 0x1c;
	wr8(gb, wOWUIObj2XCoord, gb->cpu.a);
	gb_call(gb, ADDR_DrawTopBar);
	SET_HL(gb, ADDR_Pals_84a20);
	SET_DE(gb, wTempPals2 + 4 * PAL_SIZE);
	gb->cpu.c = 4 * PAL_SIZE;
	gb->cpu.b = BANK_Pals_84a20;
	CALL(CopyFarBytes);
}

PORT_LOCAL(UpdateTopBar, OpenTopBar)
{
	gb->cpu.a = alu_dec(gb, rd8(gb, wSCY));
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	wr8(gb, wSCY, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0xec);
	if (gb->cpu.f & FLAG_Z) {
		// fully open, advance state
		gb->cpu.a = rd8(gb, wTransitionParam);
		wr8(gb, w2d02c, gb->cpu.a);
		inc_ow_bars_state(gb);
	}
	// .still_opening
	gb_call(gb, ADDR_DrawTopBar);
}

PORT_LOCAL_JUMP(UpdateTopBar, HandleInput)
{
	gb_call(gb, ADDR_HandleTopBarSelection);
	if (!(gb->cpu.f & FLAG_C)) {
		gb_call(gb, ADDR_HandleTopBarDPad);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.a = alu_and(gb, rd8(gb, wJoypadPressed), PAD_B | PAD_SELECT);
			if (!(gb->cpu.f & FLAG_Z)) {
				play_sfx(gb, SFX_0EA);
				inc_ow_bars_state(gb);
			}
		}
	}
	// .done_input
	JUMP_TO(ADDR_DrawTopBar);
}

// DrawUnlockedLevels: draws the tile in wTileToPlaceInOW, with the attribute
// in wAttrToPlaceInOW, at hl in wTilemap
static void place_ow_tile(GB *gb)
{
	gb->cpu.a = TRUE;
	wr8(gb, w2d07f, gb->cpu.a);
	gb_push16(gb, REG_HL(gb));
	gb_call(gb, ADDR_Func_81feb);
	SET_HL(gb, gb_pop16(gb));
	gb->cpu.a = rd8(gb, wTileToPlaceInOW);
	wr8(gb, REG_HL(gb), gb->cpu.a);
	gb->cpu.h = alu_dec(gb, gb->cpu.h);
	gb->cpu.h = alu_dec(gb, gb->cpu.h);
	gb->cpu.h = alu_dec(gb, gb->cpu.h);
	gb->cpu.a = rd8(gb, wAttrToPlaceInOW);
	wr8(gb, REG_HL(gb), gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d07f, 0);
}

// DrawUnlockedLevels: draws the arrow in wLevelArrowDirections1
static void show_level_arrow(GB *gb)
{
	gb->cpu.a = rd8(gb, wLevelArrowDirections1);
	SET_HL(gb, wAttrToPlaceInOW);
	gb_call(gb, ADDR_ShowLevelArrowInOW);
	gb->cpu.a = rd8(gb, wLevelArrowDirections1);
	gb_call(gb, ADDR_GetConnectedLevel1ArrowCoord);
	gb_call(gb, ADDR_GetConnectedLevel1CoordsInTilemap);
	place_ow_tile(gb);
}

// DrawUnlockedLevels.Func_81e88
static void draw_unlocked_level(GB *gb)
{
	gb_call(gb, ADDR_IsLeftExitOrJunction);
	if (!(gb->cpu.f & FLAG_Z)) {
		show_level_arrow(gb);
		gb->cpu.a = rd8(gb, wLevel2);
		alu_cp(gb, gb->cpu.a, OW_EXIT_RIGHT);
		if (gb->cpu.f & FLAG_Z)
			return;
	}
	// .second_connection
	gb->cpu.a = rd8(gb, wOWXCoord2);
	wr8(gb, wOWXCoord1, gb->cpu.a);
	gb->cpu.a = rd8(gb, wOWYCoord2);
	wr8(gb, wOWYCoord1, gb->cpu.a);
	gb->cpu.a = rd8(gb, wLevelArrowDirections2);
	wr8(gb, wLevelArrowDirections1, gb->cpu.a);
	show_level_arrow(gb);
	SET_HL(gb, wAttrToPlaceInOW);
	gb_call(gb, ADDR_ShowLevelPointInOW);
	SET_HL(gb, wOWCoords2);
	CALL(GetOWCoordInTilemap);
	place_ow_tile(gb);
}

PORT(DrawUnlockedLevels)
{
	gb->cpu.a = alu_add(gb, rd8(gb, w2d065), rd8(gb, w2d065));
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d072, 0);
	gb_call(gb, ADDR_GetUnlockedOWLevelData);
	gb_call(gb, ADDR_GetUnlockedLevelArrows);
	do {
		draw_unlocked_level(gb);
		SET_HL(gb, w2d072);
		wr8(gb, w2d072, alu_inc(gb, rd8(gb, w2d072)));
		gb->cpu.a = rd8(gb, w2d074);
		alu_cp(gb, gb->cpu.a, rd8(gb, w2d072));
	} while (!(gb->cpu.f & FLAG_Z));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d072, 0);
	wr8(gb, wOWTilemapPtr + 0, 0);
	wr8(gb, wOWTilemapPtr + 1, 0);
	wr8(gb, wAttrToPlaceInOW, 0);
	wr8(gb, wTileToPlaceInOW, 0);
	wr8(gb, wOWPathShape, 0);
}

// call addr, pushing the asm's own return address `ret` (in Func_81dce).
// A fuzzed callee can come back with another bank mapped; the asm then goes
// on running whatever that bank holds at `ret`, so this jumps there too and
// returns true (the caller must stop).
static bool call_81dce(GB *gb, uint16_t addr, uint16_t ret)
{
	gb_call_ret(gb, addr, ADDR_Func_81dce + ret);
	if (gb->rom_bank == BANK_Func_81dce)
		return false;
	JUMP_TO(ADDR_Func_81dce + ret);
	return true;
}

// sets up the OW scenes of all the unlocked events
PORT_JUMP(Func_81dce)
{
	gb->cpu.a = EVENT_PROLOGUE;
	wr8(gb, wTempEvent, gb->cpu.a);
	gb->cpu.a = rd8(gb, wLastTransitionParam);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.c = alu_dec(gb, gb->cpu.c);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	bool set = gb->cpu.f & FLAG_Z;
	if (!set) {
		alu_cp(gb, gb->cpu.a, TRANSITION_NEW_GAME);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.c = 0x00;
			set = true;
		}
	}
	if (set) {
		// .asm_81de1
		gb->cpu.a = gb->cpu.c;
		wr8(gb, wLastTransitionParam, gb->cpu.a);
	}
	for (;;) {
		// .loop_cutscenes
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, w2d078, 0);
		gb->cpu.a = rd8(gb, wTempEvent);
		if (call_81dce(gb, ADDR_CheckIfEventIsUnlocked, 0x21))
			return;
		wr8(gb, w2d065, gb->cpu.a);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z)) {
			gb->cpu.c = alu_dec(gb, gb->cpu.c);
			if (!(gb->cpu.f & FLAG_Z)) {
				gb->cpu.a = 0x80;
				wr8(gb, w2d065, gb->cpu.a);
			}
		}
		// .asm_81dfd
		if (call_81dce(gb, ADDR_Func_81dce + 0x48, 0x32)) // .PrepareScene
			return;
		gb->cpu.a = alu_inc(gb, rd8(gb, wTempEvent));
		alu_cp(gb, gb->cpu.a, NUM_EVENTS);
		if (gb->cpu.f & FLAG_Z)
			break;
		wr8(gb, wTempEvent, gb->cpu.a);
	}
	// .break
	gb->cpu.a = alu_inc(gb, rd8(gb, wLastTransitionParam));
	if (gb->cpu.f & FLAG_Z)
		wr8(gb, wLastTransitionParam, gb->cpu.a);
	RET();
}

// runs the init functions of the scene of the event in wTempEvent that are
// on the current map side
PORT_LOCAL_JUMP(Func_81dce, PrepareScene)
{
	gb->cpu.a = rd8(gb, wTempEvent);
	if (call_81dce(gb, ADDR_GetOWSceneParams, 0x4e))
		return;
	while (!(gb->cpu.f & FLAG_Z)) {
		// .loop
		gb->cpu.a = rd8(gb, wOWSceneMapSide);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = rd8(gb, wCurMapSide);
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		if (gb->cpu.f & FLAG_Z) {
			if (call_81dce(gb, ADDR_Func_81dce + 0x68, 0x5c)) // .DoInitFunc
				return;
		}
		// .diff_side
		SET_HL(gb, wOWSceneParamsPtr);
		if (call_81dce(gb, ADDR_GetByteFromPointerInHL, 0x62))
			return;
		if (call_81dce(gb, ADDR_GetOWSceneParams_GotPtr, 0x65))
			return;
	}
	RET();
}

PORT_LOCAL_JUMP(Func_81dce, DoInitFunc)
{
	gb->cpu.a = rd8(gb, wOWSceneAction);
	JUMP_TO(jumptable(gb, ADDR_Func_81dce + 0x6c));
}

PORT(ShowMissingTreasureIndicators)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wMissingTreasureIndicatorsEnabled), rd8(gb, wMissingTreasureIndicatorsEnabled));
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = rd8(gb, wOWLevel);
	wr8(gb, w2dffd, gb->cpu.a);
	gb->cpu.a = rd8(gb, wCurMapSide);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = 0x00;
	uint16_t hl = alu_add16(gb, ADDR_MapSideInitialLevels, REG_BC(gb));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wOWLevel, gb->cpu.a);
	gb->cpu.a = rd8(gb, wNextMapSide);
	static const uint8_t counts[] = {NUM_WEST_LEVELS, NUM_SOUTH_LEVELS, NUM_EAST_LEVELS};
	gb->cpu.b = NUM_NORTH_LEVELS;
	for (int i = 0; i < 3; i++) {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.b = counts[i];
			break;
		}
	}
	// .got_level_count
	gb->cpu.a = gb->cpu.b;
	wr8(gb, w2dfff, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2dffe, 0);
	gb->cpu.a = alu_and(gb, rd8(gb, wOWLevel), rd8(gb, wOWLevel));
	bool skip = gb->cpu.f & FLAG_Z;
	for (;;) {
		if (!skip) {
			// .loop_levels
			FARCALL(GetOWLevelCollectedTreasures);
			gb->cpu.a = alu_and(gb, rd8(gb, wKeyAndTreasureFlags), TREASURES_MASK);
			gb->cpu.a = alu_xor(gb, gb->cpu.a, TREASURES_MASK);
			if (!(gb->cpu.f & FLAG_Z)) {
				// .PlaceTreasureMissingIndicator
				gb->cpu.a = rd8(gb, w2dffe);
				gb_call(gb, ADDR_GetMapLevelCoords);
				hl = REG_HL(gb);
				gb->cpu.a = rd8(gb, hl++);
				wr8(gb, wOWYCoord1, gb->cpu.a);
				gb->cpu.a = rd8(gb, hl);
				wr8(gb, wOWXCoord1, gb->cpu.a);
				SET_HL(gb, wOWCoords1);
				CALL(GetOWCoordInTilemap);
				wr8(gb, REG_HL(gb), 0x7b);
				gb->cpu.h = alu_dec(gb, gb->cpu.h);
				gb->cpu.h = alu_dec(gb, gb->cpu.h);
				gb->cpu.h = alu_dec(gb, gb->cpu.h);
				wr8(gb, REG_HL(gb), 0x0d);
			}
		}
		skip = false;
		// .skip_level
		gb->cpu.a = alu_inc(gb, rd8(gb, wOWLevel));
		wr8(gb, wOWLevel, gb->cpu.a);
		wr8(gb, w2dffe, alu_inc(gb, rd8(gb, w2dffe)));
		gb->cpu.a = rd8(gb, w2dffe);
		SET_HL(gb, w2dfff);
		alu_cp(gb, gb->cpu.a, rd8(gb, w2dfff));
		if (gb->cpu.f & FLAG_Z)
			break;
	}
	gb->cpu.a = rd8(gb, w2dffd);
	wr8(gb, wOWLevel, gb->cpu.a);
}

// input:
// - a = EVENT_* constant
PORT(Func_819c6)
{
	gb_call(gb, ADDR_CheckIfEventIsUnlocked);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.c);
}

PORT_JUMP(Func_814ec)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, w2d800);
	SET_BC(gb, 0x8e);
	CALL(WriteAToHL_BCTimes);
	SET_HL(gb, wOWUIObj1);
	SET_BC(gb, 0x40);
	CALL(WriteAToHL_BCTimes);
	gb_call(gb, ADDR_Func_82111);
	gb->cpu.a = gb->cpu.l;
	wr8(gb, w2d889 + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.h;
	wr8(gb, w2d889 + 1, gb->cpu.a);
	JUMP_TO(ADDR_Func_814bd);
}

// Func_81714.Func_8175d
static void func_8175d(GB *gb, uint16_t hl)
{
	gb->cpu.a = rd8(gb, hl--); // ld a, [hld]
	SET_HL(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	SET_BC(gb, ADDR_Func_81714 + 0x6f); // .framesets
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = 0;
	gb->cpu.a = alu_adc(gb, gb->cpu.a, gb->cpu.b);
	gb->cpu.b = gb->cpu.a;
	uint16_t bc = REG_BC(gb);
	gb->cpu.a = rd8(gb, bc);
	gb->cpu.e = gb->cpu.a;
	bc++;
	SET_BC(gb, bc);
	gb->cpu.a = rd8(gb, bc);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.l = alu_dec(gb, gb->cpu.l); // dec l
	gb->cpu.b = 0x2a;
	CALL(UpdateOWAnimation);
	gb->cpu.a = alu_and(gb, 0xf8, gb->cpu.l);
	gb->cpu.l = gb->cpu.a;
	SET_DE(gb, ADDR_OAM_aa0cf);
	gb->cpu.b = BANK_OAM_aa0cf;
	CALL(LoadOWSpriteWithScroll);
}

PORT(Func_81714)
{
	static const uint16_t objs[] = {
		wDayNightTransistionSteps, wPalFade1Colour1BlueUnk3, wPalFade1Colour3RedUnk3,
		wPalFade1Colour4GreenUnk3, wOWUIObj1State, wOWUIObj2State, wd156, w2d15e,
		w2d166, w2d16e, w2d176, w2d17e,
	};
	for (unsigned i = 0; i < sizeof(objs) / sizeof(objs[0]); i++)
		func_8175d(gb, objs[i]);
}

PORT_JUMP(Func_8139f)
{
	SET_HL(gb, w2d0e5);
	gb->cpu.a = rd8(gb, w2d0e5);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	gb->cpu.a = 0x03;
	wr8(gb, w2d0e0, gb->cpu.a);
	JUMP_TO(ADDR_Func_81398);
}

PORT_JUMP(Func_813ad)
{
	gb->cpu.a = rd8(gb, w2d0e0);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_81398);
}

PORT_JUMP(Func_813b4)
{
	gb_call(gb, ADDR_GetMapSideAndLevelForNextTreasure);
	CALL(LoadLevelName);
	JUMP_TO(ADDR_Func_81398);
}

PORT_JUMP(Func_813ef)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPalConfig1TotalSteps, 0);
	JUMP_TO(ADDR_Func_81398);
}

PORT_JUMP(Func_813f6)
{
	FARCALL(FadeInLevelName);
	gb->cpu.c = alu_srl(gb, gb->cpu.c);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	gb->cpu.a = 0x1c;
	SET_HL(gb, wSceneObj13State);
	CALL(SetSceneObjState);
	JUMP_TO(ADDR_Func_81398);
}

PORT_JUMP(Func_81413)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadPressed), PAD_B | PAD_RIGHT | PAD_LEFT | PAD_DOWN);
	SET_HL(gb, wSceneObj13State);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wSceneObj13State);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			JUMP_TO(ADDR_Func_81398);
			return;
		}
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	CALL(SetSceneObjState);
	JUMP_TO(ADDR_Func_81398);
}

PORT_JUMP(LoadSpriteInsideWindow_High)
{
	gb->cpu.b = 0x20;
	JUMP_TO(ADDR_LoadSpriteInsideWindow);
}

PORT(ShowTopBarSelectedButton)
{
	gb->cpu.a = rd8(gb, wTopBarSelection);
	gb->cpu.c = 0x00;
	if (alu_bit(gb, gb->cpu.a, TOPBAR_DAY_NIGHT_F)) {
		// .day_night
		gb->cpu.a = rd8(gb, w2d011);
		if (alu_and(gb, gb->cpu.a, gb->cpu.a))
			gb->cpu.c = 0x05; // night spell
	} else {
		static const int bits[] = {TOPBAR_CUTSCENE_F, TOPBAR_COLLECTION_F, TOPBAR_NEXT_MAP_F, TOPBAR_PREV_MAP_F};
		for (int i = 0; i < 4; i++) {
			gb->cpu.c = alu_inc(gb, gb->cpu.c);
			if (alu_bit(gb, gb->cpu.a, bits[i]))
				break;
		}
	}
	// .got_selection
	gb->cpu.a = alu_add(gb, gb->cpu.c, gb->cpu.c); // *2
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	uint16_t hl = alu_add16(gb, ADDR_ShowTopBarSelectedButton + 0x4e, REG_DE(gb)); // .button_data
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wOWUIObj1XCoord, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wOWUIObj1State, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWUIObj1State - 1, 0);
	wr8(gb, wOWUIObj1State - 2, 0); // FramesetOffset

	// if either Next Map or Prev Map are selected
	// then show the NEXT MAP indicator
	gb->cpu.c = 0x00;
	gb->cpu.a = alu_and(gb, rd8(gb, wTopBarSelection), TOPBAR_NEXT_MAP | TOPBAR_PREV_MAP);
	if (!(gb->cpu.f & FLAG_Z))
		gb->cpu.c = 0x0b; // NEXT MAP indicator
	gb->cpu.a = gb->cpu.c;
	wr8(gb, wOWUIObj2State, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWUIObj2State - 1, 0);
	wr8(gb, wOWUIObj2State - 2, 0); // wOWUIObj2Duration
	SET_HL(gb, wOWUIObj2State - 2);
}

// Func_82521 with the table dispatch; false if it jumped out of the table
static bool func_82521(GB *gb)
{
	for (;;) {
		gb_push16(gb, REG_HL(gb)); // push hl
		uint16_t target = jumptable(gb, ADDR_Func_82521 + 2);
		uint16_t de;
		uint8_t next = 0;
		switch (target - ADDR_Func_82521) {
		case 0x20: SET_HL(gb, gb_pop16(gb)); return true; // .Func_82541
		case 0x22: de = ADDR_Frameset_aa091; break;
		case 0x27: de = ADDR_Frameset_aa094; break;
		case 0x2c: de = ADDR_Frameset_aa097; break;
		case 0x31: de = ADDR_Frameset_aa09a; break;
		case 0x36: de = ADDR_Frameset_aa09d; break;
		case 0x3b: de = ADDR_Frameset_aa0bc; next = 0x07; break;
		case 0x46: de = ADDR_Frameset_aa0a3; break;
		case 0x4b: de = ADDR_Frameset_aa0c1; next = 0x05; break;
		case 0x56: de = ADDR_Frameset_aa0a9; break;
		case 0x5b: de = ADDR_Frameset_aa0ae; break;
		case 0x60: de = ADDR_Frameset_aa0b1; break;
		case 0x65: de = ADDR_Frameset_aa0c6; break;
		case 0x6a: de = ADDR_Frameset_aa0b4; break;
		case 0x6f: de = ADDR_Frameset_aa0b9; break;
		default:
			JUMP_TO(target);
			return false;
		}
		SET_DE(gb, de);
		SET_HL(gb, gb_pop16(gb)); // pop hl
		gb->cpu.b = 0x2a; // .UpdateAnimation
		CALL(UpdateOWAnimation);
		if (!next)
			return true;
		gb->cpu.b = next;
		// .Func_8259a
		gb->cpu.a = rd8(gb, wOWAnimationFinished);
		alu_cp(gb, gb->cpu.a, 0xff);
		if (!(gb->cpu.f & FLAG_Z))
			return true;
		gb->cpu.l |= 1 << 2; // set 2, l
		gb->cpu.d = gb->cpu.h;
		gb->cpu.e = gb->cpu.l;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, REG_DE(gb), 0);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		wr8(gb, REG_DE(gb), 0);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = gb->cpu.b;
		wr8(gb, REG_DE(gb), gb->cpu.a);
		// call Func_82521 (a = 5 or 7: always in the table)
	}
}

PORT_JUMP(Func_82521)
{
	if (gb->rom_bank != BANK_Func_82521) {
		gb_push16(gb, REG_HL(gb));
		JUMP_TO(jumptable(gb, ADDR_Func_82521 + 2)); // runs whatever is mapped
		return;
	}
	if (func_82521(gb))
		RET();
}

// loops twice, once for de + (2 * TILEMAP_WIDTH)
// and then for the original de
PORT(Func_8269f)
{
	gb_push16(gb, REG_DE(gb)); // push de
	gb->cpu.a = alu_add(gb, gb->cpu.e, 2 * TILEMAP_WIDTH);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.a = alu_adc(gb, 0x00, gb->cpu.d);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.c = 2;
	uint16_t hl = REG_HL(gb);
	for (;;) {
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e = alu_dec(gb, gb->cpu.e);
		gb->cpu.a = alu_add(gb, gb->cpu.e, TILEMAP_WIDTH);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		if (gb->cpu.f & FLAG_Z)
			break;
		SET_DE(gb, gb_pop16(gb)); // pop de
	}
	SET_HL(gb, hl);
}

PORT(ApplyTopBarButtonAttributes)
{
	wr8(gb, wHDMASourceHi, (uint8_t)((wAttrmap + 0x2a * TILE_SIZE) >> 8));
	wr8(gb, wHDMASourceLo, (uint8_t)(wAttrmap + 0x2a * TILE_SIZE));
	wr8(gb, wHDMADestHi, 0x1b);
	wr8(gb, wHDMADestLo, 0xc0);
	wr8(gb, wHDMAMode, 0x4 - 1);

	wr8(gb, w2d0b5SourceHi, (uint8_t)((wAttrmap + 0x26 * TILE_SIZE) >> 8));
	wr8(gb, w2d0b5SourceLo, (uint8_t)(wAttrmap + 0x26 * TILE_SIZE));
	wr8(gb, w2d0b5DestHi, 0x1b);
	wr8(gb, w2d0b5DestLo, 0xc0);
	gb->cpu.a = 0x4 - 1;
	wr8(gb, w2d0b5Mode, gb->cpu.a);
}

// unreferenced
PORT(UnreferencedLCD_82729)
{
	SET_HL(gb, ADDR_UnreferencedLCD_82729 + 0xc); // .func
	SET_DE(gb, wLCDFunc);
	gb->cpu.b = 0x1f; // .func_end - .func
	CALL(CopyHLToDE);
}

// unreferenced
PORT(UnreferencedLCD_82754)
{
	SET_HL(gb, ADDR_UnreferencedLCD_82754 + 0xc); // .func
	SET_DE(gb, wLCDFunc);
	gb->cpu.b = 0x01; // .func_end - .func
	CALL(CopyHLToDE);
}

PORT_JUMP(Func_82c33)
{
	SET_HL(gb, wOWFuncCounter);
	wr8(gb, wOWFuncCounter, alu_inc(gb, rd8(gb, wOWFuncCounter)));
	gb->cpu.a = rd8(gb, w2d062);
	uint16_t target = jumptable(gb, ADDR_Func_82c33 + 8);
	if (gb->rom_bank != BANK_Func_82c33) {
		JUMP_TO(target);
		return;
	}
	switch (target - ADDR_Func_82c33) {
	case 0x1e: // .Func_82c51
		gb->cpu.a = 30;
		JUMP_TO(ADDR_Func_82c1d);
		return;
	case 0x22: // .Func_82c55
		gb->cpu.a = alu_sub(gb, rd8(gb, wCurMapSide), 1);
		gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | ((gb->cpu.f & FLAG_C) ? 0 : FLAG_C)); // ccf
		// carry set if not North
		gb->cpu.a = alu_sbc(gb, rd8(gb, wOWSceneActionParam), 0);
		wr8(gb, wLevel2, gb->cpu.a);
		gb->cpu.a = rd8(gb, wCurEvent);
		alu_cp(gb, gb->cpu.a, EVENT_GOLD_MUSIC_BOX);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = 0x07;
			wr8(gb, wOWSceneActionParam, gb->cpu.a);
			JUMP_TO(ADDR_Func_82c22);
			return;
		}
		alu_cp(gb, gb->cpu.a, EVENT_SUPER_GRAB_GLOVES);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = rd8(gb, wCurMapSide);
			alu_cp(gb, gb->cpu.a, EAST);
			if (gb->cpu.f & FLAG_Z) {
				gb->cpu.a = rd8(gb, wOWSceneActionParam);
				alu_cp(gb, gb->cpu.a, 0x07);
				if (gb->cpu.f & FLAG_Z) {
					gb->cpu.a = MYSTERY_HANDLE;
					CALL(IsTreasureCollected);
					if (gb->cpu.f & FLAG_Z) {
						JUMP_TO(ADDR_Func_82c29);
						return;
					}
				}
			}
		}
		JUMP_TO(ADDR_Func_82c22);
		return;
	case 0x5c: // .Func_82c8f
		gb->cpu.a = 12;
		JUMP_TO(ADDR_Func_82c1d);
		return;
	case 0x60: // .Func_82c93
		gb->cpu.a = rd8(gb, wLevel2);
		gb->cpu.c = gb->cpu.a;
		SET_DE(gb, wCurSceneObj);
		gb_call(gb, ADDR_SetObjPositionToLevelCoords);
		gb->cpu.a = 0x02;
		SET_HL(gb, wCurSceneObjState);
		CALL(SetSceneObjState);
		play_sfx(gb, SFX_104);
		JUMP_TO(ADDR_Func_82c22);
		return;
	case 0x7d: // .Func_82cb0
		gb->cpu.a = rd8(gb, wCurSceneObjState);
		if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			RET();
			return;
		}
		JUMP_TO(ADDR_Func_82c22);
		return;
	default: // Func_82a0a etc.
		JUMP_TO(target);
		return;
	}
}

// unreferenced
PORT_JUMP(Func_817b1)
{
	gb->cpu.a = rd8(gb, wGameModeFlags);
	if (alu_bit(gb, gb->cpu.a, MODE_GAME_CLEARED_F)) {
		RET();
		return;
	}
	gb->cpu.a = EVENT_PROLOGUE;
	for (;;) {
		wr8(gb, w2dffc, gb->cpu.a);
		gb_call(gb, ADDR_CheckIfEventIsUnlocked);
		gb->cpu.a = rd8(gb, w2dffc);
		if (gb->cpu.f & FLAG_Z)
			break;
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
		gb->cpu.b = EVENT_BLUE_CRAYON;
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		if (gb->cpu.f & FLAG_C)
			continue;
		gb->cpu.a = gb->cpu.b;
		break;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	gb_call(gb, ADDR_LoadEventTreasures);
	gb->cpu.a = rd8(gb, w2dfff);
	wr8(gb, wStoredTransitionParam, gb->cpu.a);
	JUMP_TO(ADDR_SetCutsceneButtonSelectable_GotTransitionParam);
}

PORT_JUMP(Func_8137a)
{
	SET_HL(gb, w2d0e5);
	wr8(gb, w2d0e5, alu_inc(gb, rd8(gb, w2d0e5)));
	gb->cpu.a = rd8(gb, w2d0e4);
	JUMP_TO(jumptable(gb, ADDR_Func_8137a + 8));
}

// ld hl, duration / call Func_82521 / ld b, BANK(OAM_a9fe2) / ld de, OAM_a9fe2 /
// ld hl, obj / call LoadOWSpriteWithScroll
static void update_top_bar_ui_obj(GB *gb, uint16_t duration, uint16_t obj)
{
	SET_HL(gb, duration);
	gb_call(gb, ADDR_Func_82521);
	gb->cpu.b = BANK_OAM_a9fe2;
	SET_DE(gb, ADDR_OAM_a9fe2);
	SET_HL(gb, obj);
	CALL(LoadOWSpriteWithScroll);
}

PORT(Func_824f1)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWAnimationFinished, 0);
	gb->cpu.a = rd8(gb, wOWUIObj1State);
	update_top_bar_ui_obj(gb, wOWUIObj1Duration, wOWUIObj1);
}

PORT(Func_8250a)
{
	gb->cpu.a = rd8(gb, wOWUIObj2State);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	update_top_bar_ui_obj(gb, wOWUIObj2Duration, wOWUIObj2);
}

// add $1e / ld e, a / ld c, $00 / ld d, x / call LoadSpriteInsideWindow_High
static void load_digit_sprite(GB *gb, uint8_t nybble, uint8_t x)
{
	gb->cpu.a = alu_add(gb, nybble, 0x1e);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.c = 0x00;
	gb->cpu.d = x;
	gb_call(gb, ADDR_LoadSpriteInsideWindow_High);
}

PORT(Func_829a1)
{
	gb->cpu.a = alu_and(gb, rd8(gb, w2d0ee), 0xf0);
	load_digit_sprite(gb, alu_swap(gb, gb->cpu.a), 0x50);
	gb->cpu.a = alu_and(gb, rd8(gb, w2d0ee), 0x0f);
	load_digit_sprite(gb, gb->cpu.a, 0x58);
	gb->cpu.a = alu_and(gb, rd8(gb, w2d0ef), 0xf0);
	load_digit_sprite(gb, alu_swap(gb, gb->cpu.a), 0x68);
	gb->cpu.a = alu_and(gb, rd8(gb, w2d0ef), 0x0f);
	load_digit_sprite(gb, gb->cpu.a, 0x70);
}

#define TOP_BAR_ATTR(x) (wAttrmap + 19 * TILEMAP_WIDTH + (x))

// LoadTopBarButtonAttributes.LoadSelectedAttrs
static void load_selected_attrs(GB *gb)
{
	gb->cpu.b = alu_srl(gb, gb->cpu.b);
	if (!(gb->cpu.f & FLAG_C))
		return;
	gb_call(gb, ADDR_Func_8269f);
}

// LoadTopBarButtonAttributes.LoadSelectableAttrs
static void load_selectable_attrs(GB *gb)
{
	gb->cpu.b = alu_srl(gb, gb->cpu.b);
	if (!(gb->cpu.f & FLAG_C))
		SET_HL(gb, ADDR_Data_826be); // non-selectable
	gb_call(gb, ADDR_Func_8269f);
}

PORT_LOCAL(LoadTopBarButtonAttributes, LoadSelectedAttrs) { load_selected_attrs(gb); }
PORT_LOCAL(LoadTopBarButtonAttributes, LoadSelectableAttrs) { load_selectable_attrs(gb); }

PORT(LoadTopBarButtonAttributes)
{
	gb->cpu.a = rd8(gb, wTopBarSelectableButtons);
	gb->cpu.b = gb->cpu.a;
	SET_HL(gb, ADDR_Data_826c6);
	SET_DE(gb, TOP_BAR_ATTR(11)); // Day/Night Spell
	load_selectable_attrs(gb);
	SET_HL(gb, ADDR_Data_826ce);
	SET_DE(gb, TOP_BAR_ATTR(9)); // Cutscene
	load_selectable_attrs(gb);
	SET_HL(gb, ADDR_Data_826d6);
	SET_DE(gb, TOP_BAR_ATTR(7)); // Collection
	load_selectable_attrs(gb);

	gb->cpu.a = rd8(gb, wTopBarSelection);
	gb->cpu.b = gb->cpu.a;
	SET_HL(gb, ADDR_Data_826de);
	SET_DE(gb, TOP_BAR_ATTR(11)); // Day/Night Spell
	load_selected_attrs(gb);
	SET_HL(gb, ADDR_Data_826de);
	SET_DE(gb, TOP_BAR_ATTR(9)); // Cutscene
	load_selected_attrs(gb);
	SET_HL(gb, ADDR_Data_826e6);
	SET_DE(gb, TOP_BAR_ATTR(7)); // Collection
	load_selected_attrs(gb);
}

// HandleTopBarSelection.ShowPressedButton / .ShowNextMapIndicatorFlash
static void adv_obj_state(GB *gb, uint16_t hl)
{
	gb->cpu.a = alu_inc(gb, rd8(gb, hl));
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
}

// HandleTopBarSelection.ApplyPressedButtonAttrs
static void apply_pressed_button_attrs(GB *gb, uint16_t de)
{
	SET_DE(gb, de);
	SET_HL(gb, ADDR_Data_826ee);
	gb_call(gb, ADDR_Func_8269f);
	gb_call(gb, ADDR_ApplyTopBarButtonAttributes);
}

// ld [wOWBarsState], a (a = state) / xor a / ld [wOWBarsStateCounter], a
static void set_bars_state_reset_counter(GB *gb, uint8_t state)
{
	gb->cpu.a = state;
	wr8(gb, wOWBarsState, state);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWBarsStateCounter, 0);
}

// output: carry set if a button was selected
PORT(HandleTopBarSelection)
{
	gb->cpu.f = FLAG_Z; // xor a
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_A))
		return; // no A button

	gb->cpu.a = rd8(gb, wTopBarSelection);
	if (alu_bit(gb, gb->cpu.a, TOPBAR_DAY_NIGHT_F)) {
		adv_obj_state(gb, wOWUIObj1State);
		apply_pressed_button_attrs(gb, TOP_BAR_ATTR(11));
		gb->cpu.a = OWBARS_DAY_NIGHT;
		wr8(gb, wOWBarsState, OWBARS_DAY_NIGHT);
	} else if (alu_bit(gb, gb->cpu.a, TOPBAR_CUTSCENE_F)) {
		gb->cpu.a = rd8(gb, wStoredTransitionParam);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
			return;
		adv_obj_state(gb, wOWUIObj1State);
		apply_pressed_button_attrs(gb, TOP_BAR_ATTR(9));
		set_bars_state_reset_counter(gb, OWBARS_EVENT);
	} else if (alu_bit(gb, gb->cpu.a, TOPBAR_COLLECTION_F)) {
		adv_obj_state(gb, wOWUIObj1State);
		apply_pressed_button_attrs(gb, TOP_BAR_ATTR(7));
		set_bars_state_reset_counter(gb, OWBARS_COLLECTION);
		gb->cpu.a = FALSE;
		wr8(gb, wIsCollectionOpen, FALSE);
	} else if (alu_bit(gb, gb->cpu.a, TOPBAR_NEXT_MAP_F) ||
		alu_bit(gb, gb->cpu.a, TOPBAR_PREV_MAP_F)) {
		adv_obj_state(gb, wOWUIObj1State);
		adv_obj_state(gb, wOWUIObj2State);
		set_bars_state_reset_counter(gb, OWBARS_NEXT_MAP);
	} else {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		return;
	}
	// .play_selection_sfx
	play_sfx(gb, SFX_SELECTION);
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | FLAG_C); // scf
}
