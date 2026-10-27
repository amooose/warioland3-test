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
