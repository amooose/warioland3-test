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
