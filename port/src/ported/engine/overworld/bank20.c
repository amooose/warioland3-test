// Port of src/engine/overworld/bank20.asm
#include "ported.h"

// hl = scene params
// output:
// - z set if scene finished
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

// a = EVENT_* constant
// a = PADF_* direction: moves wOWXCoord1 or wOWYCoord1 8 pixels that way
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

// hl = attribute in the tilemap
// a = PADF_* direction, hl = attribute in the tilemap
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


// places the w2d07f queued tiles and attributes in wTileToPlaceInOW
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

PORT(Func_80e55)
{
	FARCALL(Func_adbfe);
}

// DrawUnlockedLevels: draws the tile in wTileToPlaceInOW, with the attribute
// in wAttrToPlaceInOW, at hl in wTilemap
// DrawUnlockedLevels: draws the arrow in wLevelArrowDirections1
// DrawUnlockedLevels.Func_81e88
// call addr, pushing the asm's own return address `ret` (in Func_81dce).
// A fuzzed callee can come back with another bank mapped; the asm then goes
// on running whatever that bank holds at `ret`, so this jumps there too and
// returns true (the caller must stop).
// sets up the OW scenes of all the unlocked events
// runs the init functions of the scene of the event in wTempEvent that are
// on the current map side
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

// applies palette 7 and bank1 bit to wAttrToPlaceInOW
// if w2d07f is TRUE
PORT(Func_80e85)
{
	gb_call(gb, ADDR_Func_803e6);
}

PORT(GolfBuilding) { FARCALL(_GolfBuilding); }
PORT(InitEpilogue) { FARCALL(_InitEpilogue); }

// unreferenced
PORT(Func_8143d)
{
	SET_HL(gb, w2d88e);
	gb->cpu.a = rd8(gb, w2d100);
	wr8(gb, w2d88e, gb->cpu.a);
	gb->cpu.a = rd8(gb, w2d101);
	wr8(gb, w2d88f, gb->cpu.a);
	gb->cpu.a = rd8(gb, w2d069);
	wr8(gb, wOWYCoord1, gb->cpu.a);
	wr8(gb, w2d100, gb->cpu.a);
	gb->cpu.a = rd8(gb, w2d06a);
	wr8(gb, wOWXCoord1, gb->cpu.a);
	wr8(gb, w2d101, gb->cpu.a);
	SET_HL(gb, w2d88f);
	gb_call_ret(gb, ADDR_Func_8143d + 0x32, ADDR_Func_8143d + 0x20); // call .Func_8146f
	SET_HL(gb, w2d88f);
	gb->cpu.a = rd8(gb, w2d88e);
	wr8(gb, wOWYCoord1, gb->cpu.a);
	wr8(gb, w2d100, gb->cpu.a);
	gb->cpu.a = rd8(gb, w2d88f);
	wr8(gb, wOWXCoord1, gb->cpu.a);
	wr8(gb, w2d101, gb->cpu.a);
}

// Func_8143d.Func_8146f: ld a, [w2d0e3] / jumptable
PORT_LOCAL_JUMP(Func_8143d, Func_8146f)
{
	gb->cpu.a = rd8(gb, w2d0e3);
	JUMP_TO(jumptable(gb, ADDR_Func_8143d + 0x32 + 4));
}

