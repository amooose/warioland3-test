// Port of src/engine/golf/hole.asm
#include "ported.h"

// rr r (the CB one, which sets z)
static uint8_t alu_rr(GB *gb, uint8_t v)
{
	uint8_t r = (uint8_t)(v >> 1 | ((gb->cpu.f & FLAG_C) ? 0x80 : 0));
	gb->cpu.f = (uint8_t)((r ? 0 : FLAG_Z) | ((v & 1) ? FLAG_C : 0));
	return r;
}

PORT_JUMP(PrintGolfStrokeNumber)
{
	SET_HL(gb, ADDR_GolfStrokeNumbersGfx);
	gb->cpu.a = rd8(gb, wGolfStroke);
	gb->cpu.d = 0;
	gb->cpu.e = gb->cpu.a;
	for (;;) {
		alu_cp(gb, gb->cpu.a, 10);
		if (gb->cpu.f & FLAG_C)
			break;
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 10);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.d = alu_inc(gb, gb->cpu.d);
	}
	gb_push16(gb, REG_DE(gb)); // push de
	gb->cpu.a = alu_and(gb, gb->cpu.d, gb->cpu.d);
	if (gb->cpu.a) {
		SET_BC(gb, 2 * 16);
		do {
			SET_HL(gb, alu_add16(gb, REG_HL(gb), REG_BC(gb)));
			gb->cpu.d = alu_dec(gb, gb->cpu.d);
		} while (gb->cpu.d);
	}
	SET_DE(gb, w1db90);
	gb->cpu.b = 2 * 16;
	CALL(CopyHLToDE);
	SET_DE(gb, gb_pop16(gb)); // pop de

	SET_HL(gb, ADDR_GolfStrokeNumbersGfx);
	gb->cpu.a = alu_and(gb, gb->cpu.e, gb->cpu.e);
	if (gb->cpu.a) {
		SET_BC(gb, 2 * 16);
		do {
			SET_HL(gb, alu_add16(gb, REG_HL(gb), REG_BC(gb)));
			gb->cpu.e = alu_dec(gb, gb->cpu.e);
		} while (gb->cpu.e);
	}
	SET_DE(gb, w1db90 + 2 * 16);
	gb->cpu.b = 2 * 16;
	JUMP(CopyHLToDE);
}

PORT(SetGolfParaGoomRespawnYPosIfEnoughStrokes)
{
	gb->cpu.a = rd8(gb, wGolfMaxStrokes);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, wGolfStroke);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = PARAGOOM_GROUND_LEVEL & 0xff;
	wr8(gb, wGolfParaGoomYPos, gb->cpu.a);
	wr8(gb, wGolfParaGoomYPos + 1, PARAGOOM_GROUND_LEVEL >> 8);
	SET_HL(gb, wGolfParaGoomYPos + 1);
	gb->cpu.a = TRUE;
	wr8(gb, w1dc20, gb->cpu.a);
}

PORT(HoldGolfWarioLastFrame)
{
	gb->cpu.a = rd8(gb, wGolfWarioState);
	static const uint8_t states[] = {
		GOLF_WARIO_CHARGING, GOLF_WARIO_ATTACKING_1, GOLF_WARIO_MISSING, GOLF_WARIO_TURNING,
	};
	bool ok = false;
	for (int i = 0; i < 4 && !ok; i++) {
		alu_cp(gb, gb->cpu.a, states[i]);
		ok = gb->cpu.f & FLAG_Z;
	}
	if (!ok)
		return;
	SET_HL(gb, wGolfCounter);
	wr8(gb, wGolfCounter, alu_inc(gb, rd8(gb, wGolfCounter)));
	gb->cpu.a = rd8(gb, wGolfAnimLastFrame);
	wr8(gb, wGolfWarioCurrentFrame, gb->cpu.a);
}

PORT(Func_1c8f56)
{
	SET_HL(gb, wGolfXScroll);
	gb_call(gb, ADDR_Sub16Bit);
	gb->cpu.a = alu_and(gb, gb->cpu.d, gb->cpu.d);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = gb->cpu.e;
		alu_cp(gb, gb->cpu.a, 0xb0);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			return;
		}
		gb->cpu.a = TRUE;
		return;
	}
	alu_cp(gb, gb->cpu.a, 0xff);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		return;
	}
	gb->cpu.a = gb->cpu.e;
	alu_cp(gb, gb->cpu.a, 0xe0);
	if (gb->cpu.f & FLAG_C) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		return;
	}
	gb->cpu.a = TRUE;
}

PORT(LoadGolfWarioStateGfx)
{
	const uint16_t gfx_banks = ADDR_LoadGolfWarioStateGfx + 0x1d;
	const uint16_t gfx_pointers = ADDR_LoadGolfWarioStateGfx + 0x29;
	gb->cpu.a = rd8(gb, wGolfWarioState);
	gb->cpu.b = 0;
	gb->cpu.c = gb->cpu.a;
	uint16_t hl = alu_add16(gb, gfx_banks, REG_BC(gb));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wGolfWarioTilesBank, gb->cpu.a);
	gb->cpu.c = alu_sla(gb, gb->cpu.c);
	hl = alu_add16(gb, gfx_pointers, REG_BC(gb));
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wGolfWarioTilesPtr + 1, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wGolfWarioTilesPtr + 0, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT(Func_1c8fb7)
{
}

PORT(UpdateGolfDisplayMode)
{
	SET_HL(gb, wGolfXScroll);
	gb->cpu.a = rd8(gb, wGolfXScroll + 1);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.a) {
		gb->cpu.a = rd8(gb, wGolfXScroll);
		wr8(gb, wSCX, gb->cpu.a);
		gb->cpu.a = GOLF_DISPLAY_BGMAP1;
		wr8(gb, wGolfDisplayMode, gb->cpu.a);
		return;
	}
	uint8_t x = rd8(gb, wGolfXScroll);
	gb->cpu.a = x;
	wr8(gb, wSCX, x);
	alu_cp(gb, x, MAX_GOLF_X_SCROLL & 0xff);
	if (gb->cpu.f & FLAG_C) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wGolfDisplayMode, GOLF_DISPLAY_BGMAP0);
		return;
	}
	gb->cpu.a = alu_sub(gb, 167, x);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 89 + WX_OFS);
	wr8(gb, wGolfWX, gb->cpu.a);
	gb->cpu.a = GOLF_DISPLAY_SPLIT;
	wr8(gb, wGolfDisplayMode, gb->cpu.a);
}

PORT(AlignGolfXScrollWithParaGoom)
{
	gb->cpu.e = rd8(gb, wGolfParaGoomXPos);
	gb->cpu.d = rd8(gb, wGolfParaGoomXPos + 1);
	gb->cpu.a = gb->cpu.e;
	gb->cpu.d = alu_srl(gb, gb->cpu.d);
	gb->cpu.e = alu_rr(gb, gb->cpu.e);
	gb->cpu.d = alu_srl(gb, gb->cpu.d);
	gb->cpu.e = alu_rr(gb, gb->cpu.e);
	SET_HL(gb, wGolfXScroll + 1);
	gb->cpu.a = gb->cpu.d;
	alu_cp(gb, gb->cpu.a, 2);
	bool cap = !(gb->cpu.f & FLAG_C);
	if (!cap) {
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!gb->cpu.a) {
			gb->cpu.a = gb->cpu.e;
			alu_cp(gb, gb->cpu.a, 0x50);
			if (gb->cpu.f & FLAG_C) {
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z; // xor a
				wr8(gb, wGolfXScroll, 0);
				wr8(gb, wGolfXScroll + 1, 0);
				return;
			}
			gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x50);
			wr8(gb, wGolfXScroll, gb->cpu.a);
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wGolfXScroll + 1, 0);
			return;
		}
		gb->cpu.a = gb->cpu.e;
		alu_cp(gb, gb->cpu.a, 0xb0);
		if (gb->cpu.f & FLAG_C) {
			gb->cpu.a = alu_sub(gb, gb->cpu.a, 0x50);
			wr8(gb, wGolfXScroll, gb->cpu.a);
			gb->cpu.a = alu_sbc(gb, gb->cpu.d, 0);
			wr8(gb, wGolfXScroll + 1, gb->cpu.a);
			return;
		}
	}
	gb->cpu.a = 0x60;
	wr8(gb, wGolfXScroll, 0x60);
	gb->cpu.a = 0x01;
	wr8(gb, wGolfXScroll + 1, 0x01);
}

PORT(Sub16Bit)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.e = gb->cpu.a = alu_sub(gb, gb->cpu.e, rd8(gb, hl));
	hl++;
	gb->cpu.d = gb->cpu.a = alu_sbc(gb, gb->cpu.d, rd8(gb, hl));
	SET_HL(gb, hl);
}

static void copy_vblank_func(GB *gb, uint16_t func, uint8_t len)
{
	SET_HL(gb, func);
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = len;
	JUMP(CopyHLToDE);
}

PORT_JUMP(VBlank_1c9d1d)
{
	// .UpdatePinFlagTiles
	gb->cpu.a = rd8(gb, w1dc0b);
	uint8_t tile;
	alu_cp(gb, gb->cpu.a, 0x18);
	if (!(gb->cpu.f & FLAG_C)) {
		tile = 0x0c;
	} else {
		alu_cp(gb, gb->cpu.a, 0x10);
		if (!(gb->cpu.f & FLAG_C)) {
			tile = 0x08;
		} else {
			alu_cp(gb, gb->cpu.a, 0x08);
			tile = (gb->cpu.f & FLAG_C) ? 0x00 : 0x04;
		}
	}
	SET_HL(gb, ADDR_GolfPinFlagGfx + tile * 16);
	SET_DE(gb, wGolfPinFlagTiles);
	gb->cpu.b = 4 * 16;
	CALL(CopyHLToDE);
	gb->cpu.a = alu_and(gb, alu_inc(gb, rd8(gb, w1dc0b)), 0x1f);
	wr8(gb, w1dc0b, gb->cpu.a);

	copy_vblank_func(gb, ADDR_VBlank_1c9d1d + 0x0e, 0x2f);
}

PORT_JUMP(VBlank_1c9e8d) { copy_vblank_func(gb, ADDR_VBlank_1c9e8d + 0x0b, 0x1b); }
PORT_JUMP(VBlank_1c9eb3) { copy_vblank_func(gb, ADDR_VBlank_1c9eb3 + 0x0b, 0x33); }
PORT_JUMP(VBlank_1c9ef1) { copy_vblank_func(gb, ADDR_VBlank_1c9ef1 + 0x0b, 0x32); }
PORT_JUMP(VBlank_1c9f36) { copy_vblank_func(gb, ADDR_VBlank_1c9f36 + 0x0b, 0x39); }
PORT_JUMP(VBlank_1c9f7a) { copy_vblank_func(gb, ADDR_VBlank_1c9f7a + 0x0b, 0x29); }
PORT_JUMP(VBlank_1c9fe1) { copy_vblank_func(gb, ADDR_VBlank_1c9fe1 + 0x0b, 0x0e); }

PORT(Func_1c9fbd)
{
	gb->cpu.a = alu_add(gb, rd8(gb, wGolfNumCoins + 0), rd8(gb, wGolfNumCoins + 0));
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0xa0);
	wr8(gb, wVirtualOAMSprite00Frame, gb->cpu.a);
	gb->cpu.a = rd8(gb, wGolfNumCoins + 1);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_swap(gb, alu_and(gb, gb->cpu.a, 0xf0));
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0xa0);
	wr8(gb, wVirtualOAMSprite01Frame, gb->cpu.a);
	gb->cpu.a = alu_and(gb, gb->cpu.b, 0x0f);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0xa0);
	SET_HL(gb, wVirtualOAMSprite02Frame);
	wr8(gb, wVirtualOAMSprite02Frame, gb->cpu.a);
}

PORT(GolfHoleState_BetweenShots)
{
	gb->cpu.a = alu_inc(gb, rd8(gb, wGolfCounter));
	wr8(gb, wGolfCounter, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (gb->cpu.f & FLAG_Z) {
		// .check_exceeded_strokes
		gb->cpu.a = rd8(gb, wGolfMaxStrokes);
		gb->cpu.c = gb->cpu.a;
		gb->cpu.a = rd8(gb, wGolfStroke);
		alu_cp(gb, gb->cpu.a, gb->cpu.c);
		if (gb->cpu.f & FLAG_C)
			return;
		// play_music2 MUSIC_GOLF_GAME_OVER
		gb->cpu.a = (uint8_t)(MUSIC_GOLF_GAME_OVER >> 8);
		wr8(gb, hMusicIDBackup + 0, gb->cpu.a);
		wr8(gb, hMusicID + 0, gb->cpu.a);
		gb->cpu.a = (uint8_t)MUSIC_GOLF_GAME_OVER;
		wr8(gb, hMusicIDBackup + 1, gb->cpu.a);
		wr8(gb, hMusicID + 1, gb->cpu.a);

		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wGolfCounter, 0);
		wr8(gb, wGolfWarioDuration, 0);
		wr8(gb, wGolfWarioFramesetOffset, 0);
		wr8(gb, wGolfOverlayTextDuration, 0);
		wr8(gb, wGolfOverlayTextFramesetOffset, 0);
		wr8(gb, wGolfObj4State, 0x01);
		wr8(gb, wGolfWarioState, GOLF_WARIO_LOST);
		wr8(gb, wGolfVBlankMode, 0x02);
		gb->cpu.a = GOLFHOLESTATE_GAME_OVER;
		wr8(gb, wGolfHoleState, gb->cpu.a);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x20);
	if (gb->cpu.f & FLAG_C)
		return;
	// continue playing
	wr8(gb, wShotPowerGaugeCursor, BASE_SHOT_POWER);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wGolfCounter, 0);
	wr8(gb, w1dc51, 0);
	wr8(gb, wIsPowerGaugeFalling, 0);
	wr8(gb, wGolfObj4State, 0);
	wr8(gb, wGolfObj5, 0);
	wr8(gb, wGolfHoleState, GOLFHOLESTATE_WAIT_INPUT);
}

PORT(ShowGolfScore)
{
	gb->cpu.a = rd8(gb, wGolfResult);
	alu_cp(gb, gb->cpu.a, GOLFRESULT_PAR);
	if (gb->cpu.f & FLAG_Z)
		return;
	alu_cp(gb, gb->cpu.a, GOLFRESULT_OVER_PAR);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wGolfPar);
		gb->cpu.c = gb->cpu.a;
		gb->cpu.a = alu_sub(gb, rd8(gb, wGolfStroke), gb->cpu.c);
		alu_cp(gb, gb->cpu.a, 10);
		if (!(gb->cpu.f & FLAG_C)) {
			SET_HL(gb, wVirtualOAMSprite01Frame);
			wr8(gb, wVirtualOAMSprite01Frame, 0x82);
			gb->cpu.a = alu_sub(gb, gb->cpu.a, 10);
		}
	} else {
		gb->cpu.a = rd8(gb, wGolfStroke);
		gb->cpu.c = gb->cpu.a;
		gb->cpu.a = alu_sub(gb, rd8(gb, wGolfPar), gb->cpu.c);
	}
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0x80);
	SET_HL(gb, wVirtualOAMSprite02Frame);
	wr8(gb, wVirtualOAMSprite02Frame, gb->cpu.a);
}

PORT(UpdateGolfParaGoomXPos)
{
	wr8(gb, w1dc51, alu_inc(gb, rd8(gb, w1dc51)));
	gb->cpu.a = alu_and(gb, rd8(gb, w1dc51), 0x07);
	if (!gb->cpu.a)
		wr8(gb, wShotXVel, alu_dec(gb, rd8(gb, wShotXVel)));
	gb->cpu.a = alu_and(gb, rd8(gb, wShotDirection), rd8(gb, wShotDirection));
	bool oob = false;
	if (!gb->cpu.a) {
		// going right
		gb->cpu.a = alu_add(gb, rd8(gb, wShotXVel), rd8(gb, wGolfParaGoomXPos));
		wr8(gb, wGolfParaGoomXPos, gb->cpu.a);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = alu_adc(gb, rd8(gb, wGolfParaGoomXPos + 1), 0);
		wr8(gb, wGolfParaGoomXPos + 1, gb->cpu.a);
		gb->cpu.d = gb->cpu.a;
	} else {
		gb->cpu.a = rd8(gb, wShotXVel);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_sub(gb, rd8(gb, wGolfParaGoomXPos), gb->cpu.b);
		wr8(gb, wGolfParaGoomXPos, gb->cpu.a);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = alu_sbc(gb, rd8(gb, wGolfParaGoomXPos + 1), 0);
		wr8(gb, wGolfParaGoomXPos + 1, gb->cpu.a);
		gb->cpu.d = gb->cpu.a;
		alu_cp(gb, gb->cpu.a, 0xff);
		if (gb->cpu.f & FLAG_Z) {
			// gone out of bounds to the left
			wr8(gb, wGolfParaGoomXPos + 1, 0x00);
			gb->cpu.a = GOLF_TERRAIN_END;
			wr8(gb, wGolfTerrain, gb->cpu.a);
			oob = true;
		}
	}
	if (!oob) {
		for (int i = 0; i < 5; i++) {
			gb->cpu.d = alu_srl(gb, gb->cpu.d);
			gb->cpu.e = alu_rr(gb, gb->cpu.e);
		}
		wr8(gb, w1dc3e + 0, gb->cpu.e);
		gb->cpu.a = gb->cpu.d;
		wr8(gb, w1dc3e + 1, gb->cpu.d);
		uint16_t hl = alu_add16(gb, wGolfTerrainBlocks, REG_DE(gb));
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, wGolfTerrain, gb->cpu.a);
		gb->cpu.a = alu_and(gb, gb->cpu.a, GOLF_TERRAIN_MASK);
		if (gb->cpu.f & FLAG_Z) {
			// .tee_box
			gb->cpu.a = alu_and(gb, rd8(gb, w1dc47), rd8(gb, w1dc47));
			if (!gb->cpu.a) {
				gb->cpu.a = PARAGOOM_TEE_BOX_LEVEL & 0xff;
				wr8(gb, wGroundLevelYPos, gb->cpu.a);
				return;
			}
			gb->cpu.a = alu_and(gb, rd8(gb, wGolfTerrain), GOLF_TERRAIN_FLAGS_MASK);
			if (gb->cpu.a) {
				gb->cpu.c = gb->cpu.a;
				gb->cpu.a = GOLF_TERRAIN_TEE_OB;
				wr8(gb, wGolfTerrain, gb->cpu.a);
			}
			gb->cpu.a = PARAGOOM_GROUND_LEVEL & 0xff;
			wr8(gb, wGroundLevelYPos, gb->cpu.a);
			return;
		}
		alu_cp(gb, gb->cpu.a, GOLF_TERRAIN_END);
		if (!(gb->cpu.f & FLAG_Z)) {
			gb->cpu.a = PARAGOOM_GROUND_LEVEL & 0xff;
			wr8(gb, wGroundLevelYPos, gb->cpu.a);
			return;
		}
	}
	// .out_of_bounds
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wGolfParaGoomDuration, 0);
	wr8(gb, wGolfWarioDuration, 0);
	wr8(gb, wGolfParaGoomFramesetOffset, 0);
	wr8(gb, wGolfWarioFramesetOffset, 0);
	wr8(gb, wGolfParaGoomState, GOLF_PARAGOOM_8);
	wr8(gb, wGolfWarioState, GOLF_WARIO_IDLING);
	wr8(gb, wGolfVBlankMode, 0x02);
	gb->cpu.a = GOLFHOLESTATE_SPECIAL_TERRAIN;
	wr8(gb, wGolfHoleState, gb->cpu.a);
	SET_HL(gb, wGolfStroke);
	wr8(gb, wGolfStroke, alu_inc(gb, rd8(gb, wGolfStroke)));
}

PORT_JUMP(GolfHoleState_WaitInput)
{
	gb->cpu.a = rd8(gb, wGolfXScroll);
	gb->cpu.d = rd8(gb, wGolfXScroll + 1);
	gb->cpu.e = gb->cpu.a;
	SET_HL(gb, wGolfXScroll + 1);

	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (alu_bit(gb, gb->cpu.a, B_PAD_SELECT)) {
		play_sfx(gb, SFX_0E4);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wGolfObj3Duration, 0);
		wr8(gb, wGolfObj3FramesetOffset, 0);
		gb->cpu.a = GOLFHOLESTATE_PAUSE;
		wr8(gb, wGolfHoleState, gb->cpu.a);
		RET();
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		play_sfx(gb, SFX_0E9);
		// clear wGolfOverlayText
		uint16_t hl = wGolfOverlayText;
		gb->cpu.a = 0;
		gb->cpu.c = 8;
		do {
			wr8(gb, hl++, 0);
			gb->cpu.c = alu_dec(gb, gb->cpu.c);
		} while (gb->cpu.c);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wGolfResult, 0);
		gb->cpu.a = GOLFHOLESTATE_SELECT_SHOT_POWER;
		wr8(gb, wGolfHoleState, gb->cpu.a);
		gb->cpu.a = rd8(gb, wGolfTerrain);
		wr8(gb, wStrokeGolfTerrain, gb->cpu.a);
		gb->cpu.a = rd8(gb, w1dc47);
		wr8(gb, w1dc53, gb->cpu.a);
		// save ParaGoom's current position for future reference
		SET_HL(gb, wGolfParaGoomYPos);
		SET_DE(gb, wLastGolfParaGoomYPos);
		gb->cpu.b = 4;
		JUMP(CopyHLToDE);
		return;
	}
	uint8_t dir;
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		gb->cpu.a = alu_and(gb, gb->cpu.d, gb->cpu.d);
		if (gb->cpu.a) {
			gb->cpu.a = gb->cpu.e;
			alu_cp(gb, gb->cpu.a, MAX_GOLF_X_SCROLL & 0xff);
			if (!(gb->cpu.f & FLAG_C)) {
				RET(); // can't scroll right
				return;
			}
		}
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		dir = 0;
	} else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		gb->cpu.a = alu_or(gb, gb->cpu.d, gb->cpu.e);
		if (!gb->cpu.a) {
			RET(); // can't scroll left
			return;
		}
		dir = 1;
	} else {
		RET();
		return;
	}
	// .begin_scroll
	gb->cpu.a = dir;
	wr8(gb, wGolfHoleScrollingDir, dir);
	gb->cpu.a = rd8(gb, wGolfXScroll);
	wr8(gb, wTempGolfXScroll + 0, gb->cpu.a);
	gb->cpu.a = rd8(gb, wGolfXScroll + 1);
	wr8(gb, wTempGolfXScroll + 1, gb->cpu.a);
	SET_HL(gb, wGolfXScroll + 1);
	gb->cpu.a = GOLFHOLESTATE_SCROLL;
	wr8(gb, wGolfHoleState, gb->cpu.a);
	RET();
}

PORT(GolfHoleState_SelectShotSpin)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wIsPowerGaugeFalling), rd8(gb, wIsPowerGaugeFalling));
	bool check_a = true;
	if (!gb->cpu.a) {
		gb->cpu.b = 1;
		gb->cpu.a = rd8(gb, wSelectedShotPowerGauge);
		alu_cp(gb, gb->cpu.a, 16);
		if (!(gb->cpu.f & FLAG_C))
			gb->cpu.b = 2;
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.b);
		wr8(gb, wSelectedShotPowerGauge, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, MAX_SHOT_POWER);
		if (!(gb->cpu.f & FLAG_C)) {
			play_sfx(gb, SFX_0EA);
			gb->cpu.a = TRUE;
			wr8(gb, wIsPowerGaugeFalling, gb->cpu.a);
		}
	} else {
		gb->cpu.a = alu_sub(gb, rd8(gb, wSelectedShotPowerGauge), 2);
		wr8(gb, wSelectedShotPowerGauge, gb->cpu.a);
		if (gb->cpu.f & FLAG_C) {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wSelectedShotPowerGauge, 0);
			check_a = false;
		}
	}
	if (check_a) {
		gb->cpu.a = rd8(gb, wJoypadPressed);
		if (!alu_bit(gb, gb->cpu.a, B_PAD_A))
			return; // no A button
	}
	// .check_spin
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wGolfCounter, 0);
	gb->cpu.a = GOLFHOLESTATE_START_SHOT;
	wr8(gb, wGolfHoleState, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSelectedShotPowerGauge);
	uint8_t spin;
	alu_cp(gb, gb->cpu.a, 2);
	if (gb->cpu.f & FLAG_C)
		goto missed;
	alu_cp(gb, gb->cpu.a, 5);
	if (gb->cpu.f & FLAG_C) {
		spin = SHOT_BACKSPIN;
		goto done;
	}
	alu_cp(gb, gb->cpu.a, 11);
	if (gb->cpu.f & FLAG_C) {
		spin = SHOT_NORMAL;
		goto done;
	}
	alu_cp(gb, gb->cpu.a, 14);
	if (gb->cpu.f & FLAG_C) {
		spin = SHOT_TOPSPIN;
		goto done;
	}
missed:
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a (SHOT_MISS)
	wr8(gb, wShotSpin, 0);
	return;
done:
	gb->cpu.a = spin;
	wr8(gb, wShotSpin, spin);
}

PORT(GolfHoleState_SelectShotPower)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wIsPowerGaugeFalling), rd8(gb, wIsPowerGaugeFalling));
	if (!gb->cpu.a) {
		// if < 16 add 1 to shot power, otherwise add 2
		gb->cpu.b = 1;
		gb->cpu.a = rd8(gb, wShotPowerGaugeCursor);
		alu_cp(gb, gb->cpu.a, 16);
		if (!(gb->cpu.f & FLAG_C))
			gb->cpu.b = 2;
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.b);
		wr8(gb, wShotPowerGaugeCursor, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, MAX_SHOT_POWER);
		if (!(gb->cpu.f & FLAG_C)) {
			play_sfx(gb, SFX_0EA);
			gb->cpu.a = TRUE;
			wr8(gb, wIsPowerGaugeFalling, gb->cpu.a);
		}
	} else {
		gb->cpu.a = alu_sub(gb, rd8(gb, wShotPowerGaugeCursor), 2);
		wr8(gb, wShotPowerGaugeCursor, gb->cpu.a);
		if (gb->cpu.f & FLAG_C) {
			// player let the cursor go down to 0
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wGolfCounter, 0);
			wr8(gb, wShotPowerGaugeCursor, 0);
			gb->cpu.a = GOLFHOLESTATE_START_SHOT;
			wr8(gb, wGolfHoleState, gb->cpu.a);
			return;
		}
	}
	// .check_a_btn
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_A))
		return; // no A button
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wGolfWarioDuration, 0);
	wr8(gb, wGolfWarioFramesetOffset, 0);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wGolfObj5State, gb->cpu.a);
	gb->cpu.a = GOLFHOLESTATE_SELECT_SHOT_SPIN;
	wr8(gb, wGolfHoleState, gb->cpu.a);

	// translate selected power gauge into shot power level
	gb->cpu.a = rd8(gb, wShotPowerGaugeCursor);
	wr8(gb, wSelectedShotPowerGauge, gb->cpu.a);
	static const uint8_t limits[] = { 16, 24, 32, 40, 48, 56, 64, 70, 74 };
	static const uint8_t levels[] = {
		SHOT_POWER_0, SHOT_POWER_1, SHOT_POWER_2, SHOT_POWER_3, SHOT_POWER_4,
		SHOT_POWER_5, SHOT_POWER_6, SHOT_POWER_7, SHOT_POWER_8, SHOT_POWER_9,
	};
	int lvl = 9;
	for (int i = 0; i < 9; i++) {
		if (gb->cpu.a < limits[i]) {
			lvl = i;
			break;
		}
	}
	gb->cpu.a = levels[lvl];
	wr8(gb, wShotPowerLevel, gb->cpu.a);

	// .got_power_lvl
	gb->cpu.a = alu_sla(gb, rd8(gb, wShotPowerLevel));
	gb->cpu.b = 0;
	gb->cpu.c = gb->cpu.a;
	uint16_t hl = alu_add16(gb, ADDR_GolfHoleState_SelectShotPower + 0x10e, REG_BC(gb)); // .data
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, wShotYVel, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wShotYVel + 1, 0); // w1dc4f
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wShotYVel + 2, gb->cpu.a); // wShotXVel
	SET_DE(gb, wShotYVel + 2);
	SET_HL(gb, hl);

	gb->cpu.a = rd8(gb, wGolfTerrain);
	alu_cp(gb, gb->cpu.a, GOLF_TERRAIN_ROUGH);
	if (gb->cpu.f & FLAG_Z) {
		uint8_t v = rd8(gb, wShotYVel);
		gb->cpu.a = alu_add(gb, v, v);
		gb->cpu.a = alu_add(gb, gb->cpu.a, v);
		gb->cpu.a = alu_add(gb, gb->cpu.a, 3);
		gb->cpu.a = alu_srl(gb, gb->cpu.a);
		gb->cpu.a = alu_srl(gb, gb->cpu.a);
		wr8(gb, wShotYVel, gb->cpu.a); // (a*3 + 3) / 4
		SET_HL(gb, wShotYVel + 1);
		gb->cpu.a = SHOT_MODIFIER_ROUGH;
		wr8(gb, wShotTerrainModifier, gb->cpu.a);
		return;
	}
	alu_cp(gb, gb->cpu.a, GOLF_TERRAIN_BUNKER);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = alu_inc(gb, rd8(gb, wShotYVel));
		gb->cpu.a = alu_srl(gb, gb->cpu.a);
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		wr8(gb, wShotYVel, gb->cpu.a); // ((a + 1) / 2) - 1
		SET_HL(gb, wShotYVel + 1);
		gb->cpu.a = SHOT_MODIFIER_BUNKER;
		wr8(gb, wShotTerrainModifier, gb->cpu.a);
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a (SHOT_MODIFIER_NONE)
	wr8(gb, wShotTerrainModifier, 0);
}

// outputs in a: $0 if de < *hl, $1 if de == *hl, $2 if de > *hl
PORT(Compare16Bit)
{
	gb_push16(gb, REG_DE(gb));
	gb_call(gb, ADDR_Sub16Bit);
	if (gb->cpu.f & FLAG_C) {
		// .less_than
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
	} else {
		gb->cpu.a = alu_or(gb, gb->cpu.d, gb->cpu.e);
		if (gb->cpu.f & FLAG_Z)
			gb->cpu.a = alu_inc(gb, gb->cpu.a); // .equal
		else
			gb->cpu.a = 0x2;
	}
	SET_DE(gb, gb_pop16(gb));
}

PORT_JUMP(Func_1c9cf9)
{
	gb->cpu.a = rd8(gb, wGolfDisplayMode);
	alu_cp(gb, gb->cpu.a, GOLF_DISPLAY_SPLIT);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = LCDC_BG_ON | LCDC_OBJ_ON | LCDC_OBJ_16 | LCDC_WIN_ON | LCDC_WIN_9C00 | LCDC_ON;
	} else {
		alu_cp(gb, gb->cpu.a, GOLF_DISPLAY_BGMAP1);
		if (gb->cpu.f & FLAG_Z)
			gb->cpu.a = LCDC_BG_ON | LCDC_OBJ_ON | LCDC_OBJ_16 | LCDC_BG_9C00 | LCDC_ON;
		else
			gb->cpu.a = LCDC_DEFAULT;
	}
	// .got_lcd_config
	wr8(gb, wGolfLCDConfig, gb->cpu.a);
	wr8(gb, rVBK, gb->cpu.a);
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	JUMP_TO(hTransferVirtualOAM);
}

// rst JumpTable, with the table at `table` (right after the rst). Leaves
// every register as JumpTable does and returns the target (= hl).
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

PORT_JUMP(GolfLobbyVBlank)
{
	gb->cpu.a = rd8(gb, wGolfVBlankMode);
	JUMP_TO(jumptable(gb, ADDR_GolfLobbyVBlank + 4));
}

PORT_JUMP(HandleGolfParaGoomRespawn)
{
	gb->cpu.a = rd8(gb, wGolfParaGoomState);
	alu_cp(gb, gb->cpu.a, GOLF_PARAGOOM_2);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_1c8e32
		uint16_t hl = wLastGolfParaGoomYPos;
		gb->cpu.a = (uint8_t)PARAGOOM_GROUND_LEVEL;
		wr8(gb, hl++, gb->cpu.a);
		wr8(gb, hl, (uint8_t)(PARAGOOM_GROUND_LEVEL >> 8));
		hl = wGolfParaGoomYPos;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl++, 0);
		wr8(gb, hl++, 0);

		gb->cpu.a = alu_and(gb, rd8(gb, wGolfTerrain), GOLF_TERRAIN_2ND_WATER);
		if (gb->cpu.f & FLAG_Z)
			gb->cpu.a = rd8(gb, wGolfWaterSpawn1);
		else
			gb->cpu.a = rd8(gb, wGolfWaterSpawn2); // .second_water_set
		// .got_water_spawn_block
		gb->cpu.d = 0x00;
		gb->cpu.e = gb->cpu.a;
		for (int i = 0; i < 5; i++) {
			gb->cpu.e = alu_sla(gb, gb->cpu.e);
			gb->cpu.d = alu_rl(gb, gb->cpu.d);
		}
		// * 32
		gb->cpu.a = gb->cpu.e;
		wr8(gb, hl++, gb->cpu.a); // wGolfParaGoomXPos
		wr8(gb, hl, gb->cpu.d);
		SET_HL(gb, hl);

		gb_call(gb, ADDR_AlignGolfXScrollWithParaGoom);
		gb_call(gb, ADDR_UpdateGolfDisplayMode);
		gb->cpu.b = TRUE;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wGolfParaGoomFramesetOffset, 0);
		gb->cpu.a = GOLF_PARAGOOM_4;
		wr8(gb, wGolfParaGoomState, gb->cpu.a);
		JUMP_TO(ADDR_SetGolfParaGoomRespawnYPosIfEnoughStrokes);
		return;
	}
	alu_cp(gb, gb->cpu.a, GOLF_PARAGOOM_3);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	// .asm_1c8e7c
	uint16_t hl = wGolfParaGoomYPos;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, hl++, 0);
	uint16_t de = wLastGolfParaGoomXPos;
	gb->cpu.a = rd8(gb, de);
	wr8(gb, hl++, gb->cpu.a); // wGolfParaGoomXPos
	de++;
	gb->cpu.a = rd8(gb, de);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, de);
	gb->cpu.b = TRUE;
	gb->cpu.a = GOLF_PARAGOOM_8;
	wr8(gb, wGolfParaGoomState, gb->cpu.a);
	JUMP_TO(ADDR_SetGolfParaGoomRespawnYPosIfEnoughStrokes); // fallthrough
}

// ld de, dst / ld b, n / call CopyHLToDE
static void copy_hl_to_de(GB *gb, uint16_t dst, uint8_t n)
{
	SET_DE(gb, dst);
	gb->cpu.b = n;
	CALL(CopyHLToDE);
}

PORT(ShowGolfHoleFlagAndPar)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);

	// if playing Golf in the Golf Building,
	// then the number of strokes can exceed the par
	gb->cpu.a = rd8(gb, w1d800);
	alu_cp(gb, gb->cpu.a, 0x02);
	bool building = (gb->cpu.f & FLAG_Z) != 0;
	if (!building) {
		gb->cpu.a = alu_add(gb, rd8(gb, wGolfPar), MAX_GOLF_COURSE_STROKES);
		wr8(gb, wGolfMaxStrokes, gb->cpu.a);
		gb->cpu.a = rd8(gb, wGolfCourseHole);
	} else {
		gb->cpu.a = rd8(gb, wGolfPar);
		wr8(gb, wGolfMaxStrokes, gb->cpu.a);
		gb->cpu.a = alu_dec(gb, rd8(gb, wGolfPar));
	}
	// .got_max_strokes
	gb->cpu.a = alu_swap(gb, gb->cpu.a); // *$10
	gb->cpu.b = 0x00;
	gb->cpu.c = gb->cpu.a;
	SET_HL(gb, alu_add16(gb, ADDR_GolfFlagNumbersGfx, REG_BC(gb)));
	copy_hl_to_de(gb, v1Tiles0 + 0x01 * 0x10, 0x10);

	gb->cpu.a = alu_dec(gb, rd8(gb, wGolfPar));
	gb->cpu.a = alu_swap(gb, gb->cpu.a); // *$10
	gb->cpu.a = alu_sub(gb, gb->cpu.a, 2 * 0x10);
	gb->cpu.c = gb->cpu.a;
	static const uint8_t par_tiles[] = {0x2b, 0x4b, 0x6b, 0x8b};
	for (int i = 0; i < 4; i++) {
		SET_HL(gb, alu_add16(gb, ADDR_GolfParNumbersGfx, REG_BC(gb)));
		copy_hl_to_de(gb, v1Tiles0 + par_tiles[i] * 0x10, 0x10);
	}

	gb->cpu.a = rd8(gb, w1d800);
	alu_cp(gb, gb->cpu.a, 0x02);
	if (!(gb->cpu.f & FLAG_Z)) {
		SET_HL(gb, ADDR_GolfHoleFlagGfx);
		copy_hl_to_de(gb, v1Tiles0 + 0x20 * 0x10, 6 * 0x10);
		copy_hl_to_de(gb, v1Tiles0 + 0x40 * 0x10, 6 * 0x10);
		copy_hl_to_de(gb, v1Tiles0 + 0x60 * 0x10, 6 * 0x10);
		copy_hl_to_de(gb, v0Tiles1, 6 * 0x10);
	}

	// .stroke_number
	gb_call(gb, ADDR_PrintGolfStrokeNumber);
	SET_HL(gb, w1db90);
	copy_hl_to_de(gb, v1Tiles0 + 0xf0 * 0x10, 4 * 0x10);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

PORT_JUMP(GolfHoleState_StartShot)
{
	SET_HL(gb, wGolfCounter);
	gb->cpu.a = rd8(gb, wGolfCounter);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		// .StartShot
		wr8(gb, wGolfCounter, alu_inc(gb, rd8(gb, wGolfCounter)));
		SET_HL(gb, wGolfStroke);
		wr8(gb, wGolfStroke, alu_inc(gb, rd8(gb, wGolfStroke)));
		gb->cpu.a = 0x03;
		wr8(gb, wGolfVBlankMode, gb->cpu.a);
		JUMP_TO(ADDR_PrintGolfStrokeNumber);
		return;
	}
	alu_cp(gb, gb->cpu.a, 1);
	if (gb->cpu.f & FLAG_Z) {
		// .ChargeShot
		wr8(gb, wGolfCounter, alu_inc(gb, rd8(gb, wGolfCounter)));
		play_sfx(gb, SFX_0ED);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wGolfWarioDuration, 0);
		wr8(gb, wGolfWarioFramesetOffset, 0);
		gb->cpu.a = GOLF_WARIO_CHARGING;
		wr8(gb, wGolfWarioState, gb->cpu.a);
		gb->cpu.a = 0x02;
		wr8(gb, wGolfVBlankMode, gb->cpu.a);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 3);
	if (gb->cpu.f & FLAG_Z) {
		// .HitOrMiss
		wr8(gb, wGolfCounter, alu_inc(gb, rd8(gb, wGolfCounter)));
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wGolfWarioDuration, 0);
		wr8(gb, wGolfWarioFramesetOffset, 0);
		gb->cpu.a = rd8(gb, wShotPowerGaugeCursor);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z)) {
			gb->cpu.a = rd8(gb, wShotSpin);
			alu_and(gb, gb->cpu.a, gb->cpu.a);
		}
		if (!(gb->cpu.f & FLAG_Z)) {
			// hit
			play_sfx(gb, SFX_017);
			gb->cpu.a = GOLF_WARIO_ATTACKING_1;
			wr8(gb, wGolfWarioState, gb->cpu.a);
		} else {
			// .missed_1
			play_sfx(gb, SFX_0E8);
			gb->cpu.a = GOLF_WARIO_MISSING;
			wr8(gb, wGolfWarioState, gb->cpu.a);
			gb->cpu.a = 0x02;
			wr8(gb, wGolfVBlankMode, gb->cpu.a);
		}
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 5);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	// .FinishShot
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wGolfCounter, 0);
	wr8(gb, wGolfWarioDuration, 0);
	wr8(gb, wGolfWarioFramesetOffset, 0);

	gb->cpu.a = rd8(gb, wGolfWarioState);
	alu_cp(gb, gb->cpu.a, GOLF_WARIO_MISSING);
	if (gb->cpu.f & FLAG_Z) {
		// .missed_2
		gb->cpu.a = 0x02;
		wr8(gb, wGolfVBlankMode, gb->cpu.a);
		gb->cpu.a = GOLF_WARIO_IDLING;
		wr8(gb, wGolfWarioState, gb->cpu.a);
		gb->cpu.a = GOLFHOLESTATE_BETWEEN_SHOTS;
		wr8(gb, wGolfHoleState, gb->cpu.a);
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wGolfHoleDirection);
	wr8(gb, wShotDirection, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wGolfParaGoomFramesetOffset, 0);
	gb->cpu.a = alu_inc(gb, gb->cpu.a); // GOLF_PARAGOOM_1
	wr8(gb, wGolfParaGoomState, gb->cpu.a);
	gb->cpu.a = GOLF_WARIO_ATTACKING_2;
	wr8(gb, wGolfWarioState, gb->cpu.a);
	gb->cpu.a = GOLFHOLESTATE_BOUNCE;
	wr8(gb, w1dc01, gb->cpu.a);
	gb->cpu.a = GOLFHOLESTATE_SHOT;
	wr8(gb, wGolfHoleState, gb->cpu.a);
	RET();
}

PORT_JUMP(Func_1c9fae)
{
	gb_call(gb, ADDR_Func_1c9fbd);
	gb->cpu.a = rd8(gb, wGolfVBlankMode);
	JUMP_TO(jumptable(gb, ADDR_Func_1c9fae + 6));
}

// ld a, HIGH(v1BGMap0 + 6 * TILEMAP_WIDTH) - $80 / ld [wdc11 + 0], a /
// ld a, LOW(...) / ld [wdc11 + 1], a / ld a, 7 dma_rows / ld [w1dc13], a
static void set_attrmap_dma(GB *gb)
{
	wr8(gb, wdc11 + 0, (uint8_t)(((v1BGMap0 + 6 * TILEMAP_WIDTH) >> 8) - 0x80));
	wr8(gb, wdc11 + 1, (uint8_t)(v1BGMap0 + 6 * TILEMAP_WIDTH));
	gb->cpu.a = 0x0d; // 7 dma_rows
	wr8(gb, w1dc13, gb->cpu.a);
}

PORT_JUMP(VBlank_1c9ffa)
{
	set_attrmap_dma(gb);
	copy_vblank_func(gb, ADDR_VBlank_1c9ffa + 0x1a, 0x1f);
}

PORT_JUMP(VBlank_1ca033) { copy_vblank_func(gb, ADDR_VBlank_1ca033 + 0x0b, 0x18); }

PORT_JUMP(VBlank_1ca056)
{
	set_attrmap_dma(gb);
	copy_vblank_func(gb, ADDR_VBlank_1ca056 + 0x1a, 0x1f);
}

// ld a, y / ld [obj YCoord], a / ld a, x / ld [obj XCoord], a /
// ld de, frameset / ld hl, obj Duration / call UpdateGolfObjectAnimation /
// ld a, [obj CurrentFrame] / ld [obj Frame], a / ld hl, obj Sprite
static void golf_obj(GB *gb, uint16_t ycoord, uint8_t y, uint16_t xcoord, uint8_t x,
	uint16_t frameset, uint16_t duration, uint16_t cur_frame, uint16_t frame, uint16_t sprite)
{
	gb->cpu.a = y;
	wr8(gb, ycoord, y);
	gb->cpu.a = x;
	wr8(gb, xcoord, x);
	SET_DE(gb, frameset);
	SET_HL(gb, duration);
	CALL(UpdateGolfObjectAnimation);
	gb->cpu.a = rd8(gb, cur_frame);
	wr8(gb, frame, gb->cpu.a);
	SET_HL(gb, sprite);
}
#define GOLF_OBJ(o, y, x, fs) golf_obj(gb, w##o##YCoord, (y), w##o##XCoord, (x), (fs), \
	w##o##Duration, w##o##CurrentFrame, w##o##Frame, w##o##Sprite)

PORT(UpdateGolfHoleDirection)
{
	uint16_t hl = wGolfParaGoomXPos;
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.d = gb->cpu.a;
	SET_HL(gb, hl); // wGolfHoleHoleXPos
	gb_call(gb, ADDR_Compare16Bit);
	alu_cp(gb, gb->cpu.a, 0x2);
	bool right = !(gb->cpu.f & FLAG_C);
	if (right) {
		gb->cpu.a = 0x1;
		wr8(gb, wGolfHoleDirection, 0x1);
	} else {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wGolfHoleDirection, 0);
	}
	hl = wGolfParaGoomXPos;
	uint16_t de = wGolfWarioTargetXPos;
	if (right) {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), 0x40);
		wr8(gb, de, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl), 0);
	} else {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x40);
		wr8(gb, de, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl), 0);
	}
	de++;
	wr8(gb, de, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, de);
}

PORT_JUMP(ShowGolfResultOverlayText)
{
	gb->cpu.a = 0x50;
	wr8(gb, wGolfOverlayTextYCoord, 0x50);
	gb->cpu.a = 0x58;
	wr8(gb, wGolfOverlayTextXCoord, 0x58);
	gb->cpu.a = alu_add(gb, rd8(gb, wGolfResult), rd8(gb, wGolfResult)); // *2
	gb->cpu.b = 0x00;
	gb->cpu.c = gb->cpu.a;
	uint16_t hl = alu_add16(gb, ADDR_ShowGolfResultOverlayText + 0x2a, REG_BC(gb)); // .Framesets
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = rd8(gb, hl);
	SET_HL(gb, wGolfOverlayTextDuration);
	CALL(UpdateGolfObjectAnimation);
	gb->cpu.a = rd8(gb, wGolfOverlayTextCurrentFrame);
	wr8(gb, wGolfOverlayTextFrame, gb->cpu.a);
	SET_HL(gb, wGolfOverlayTextSprite);
	JUMP(LoadGolfSprite);
}

PORT(GolfHoleState_GameOver)
{
	GOLF_OBJ(GolfOverlayText, 0x50, 0x58, ADDR_Frameset_1cafa0);
	CALL(LoadGolfSprite);

	SET_HL(gb, wGolfCounter);
	gb->cpu.a = rd8(gb, wGolfCounter);
	alu_cp(gb, gb->cpu.a, 0x50);
	if (gb->cpu.f & FLAG_C) {
		wr8(gb, wGolfCounter, alu_inc(gb, rd8(gb, wGolfCounter)));
		return;
	}
	// .asm_1c9be5
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_A))
		return;
	gb->cpu.a = rd8(gb, w1d800);
	alu_cp(gb, gb->cpu.a, 0x02);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0x01;
		wr8(gb, wGolfMenuOption, 0x01);
	}
	// .asm_1c9bf7
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSubState, 0);
}

PORT(GolfHoleState_Pause)
{
	GOLF_OBJ(GolfObj3, 0x9c, 0x58, ADDR_Frameset_1cb0da);
	CALL(LoadGolfSprite);

	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		// .asm_1c9c27
		gb->cpu.a = rd8(gb, w1d800);
		alu_cp(gb, gb->cpu.a, 0x01);
		if (!(gb->cpu.f & FLAG_Z)) {
			gb->cpu.a = 0x01;
			wr8(gb, wGolfMenuOption, 0x01);
		}
		// .asm_1c9c33
		play_sfx(gb, SFX_SELECTION);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wSubState, 0);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		// .asm_1c9c40
		play_sfx(gb, SFX_0E7);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wGolfObj3Duration, 0);
		wr8(gb, wGolfObj3FramesetOffset, 0);
		wr8(gb, wGolfHoleState, GOLFHOLESTATE_WAIT_INPUT);
	}
}

PORT(GolfHoleState_Result)
{
	gb_call(gb, ADDR_ShowGolfResultOverlayText);

	// .SetOverlayTextScore
	SET_HL(gb, wVirtualOAMSprite00YCoord);
	wr8(gb, wVirtualOAMSprite00YCoord, alu_dec(gb, rd8(gb, wVirtualOAMSprite00YCoord)));
	gb->cpu.d = 0x00;
	gb->cpu.a = rd8(gb, wGolfCourseScore);
	gb->cpu.c = gb->cpu.a;
	alu_cp(gb, gb->cpu.a, GOLF_BASE_SCORE + 1);
	if (!(gb->cpu.f & FLAG_C)) {
		// .positive_score
		gb->cpu.a = alu_sub(gb, gb->cpu.a, GOLF_BASE_SCORE);
		gb->cpu.e = gb->cpu.a;
	} else {
		alu_cp(gb, gb->cpu.a, GOLF_BASE_SCORE);
		if (!(gb->cpu.f & FLAG_C)) {
			wr8(gb, wVirtualOAMSprite00YCoord, alu_dec(gb, rd8(gb, wVirtualOAMSprite00YCoord)));
			SET_HL(gb, wVirtualOAMSprite00Frame);
			wr8(gb, wVirtualOAMSprite00Frame, 0xfe);
		}
		// .negative_score
		gb->cpu.a = alu_sub(gb, GOLF_BASE_SCORE, gb->cpu.c);
		gb->cpu.e = gb->cpu.a;
	}
	// .loop_get_digits
	for (;;) {
		alu_cp(gb, gb->cpu.a, 10);
		if (gb->cpu.f & FLAG_C)
			break;
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 10);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.d = alu_inc(gb, gb->cpu.d);
	}
	// .got_tens_digit
	gb->cpu.a = gb->cpu.d;
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
		gb->cpu.a = alu_add(gb, gb->cpu.a, 0x80);
		SET_HL(gb, wVirtualOAMSprite01Frame);
		wr8(gb, wVirtualOAMSprite01Frame, gb->cpu.a);
	}
	// .got_ones_digit
	gb->cpu.a = alu_add(gb, gb->cpu.e, gb->cpu.e);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0x80);
	SET_HL(gb, wVirtualOAMSprite02Frame);
	wr8(gb, wVirtualOAMSprite02Frame, gb->cpu.a);

	// show "TOTAL" text overlay
	GOLF_OBJ(GolfParaGoomGhost, 0x3c, 0x50, ADDR_Frameset_1cb0d7);
	CALL(LoadGolfSprite);

	SET_HL(gb, wGolfCounter);
	gb->cpu.a = rd8(gb, wGolfCounter);
	alu_cp(gb, gb->cpu.a, 0x20);
	if (gb->cpu.f & FLAG_C) {
		wr8(gb, wGolfCounter, alu_inc(gb, rd8(gb, wGolfCounter)));
		return;
	}
	// .input_allowed
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_A))
		return;
	gb->cpu.a = rd8(gb, wGolfCourse);
	gb->cpu.d = 0x00;
	gb->cpu.e = gb->cpu.a;
	uint16_t hl = alu_add16(gb, wGolfBestScores, REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, wGolfCourseScore);
	alu_cp(gb, gb->cpu.a, rd8(gb, hl));
	if (gb->cpu.f & FLAG_C)
		wr8(gb, hl, gb->cpu.a);
	// .skip_overwrite_score
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSubState, 0);
}

// xor a / ld [wGolfParaGoomDuration], a / ld [wGolfWarioDuration], a /
// ld [wGolfParaGoomFramesetOffset], a / ld [wGolfWarioFramesetOffset], a /
// ld [wGolfParaGoomState], a / ld a, GOLF_WARIO_IDLING /
// ld [wGolfWarioState], a / ld a, $02 / ld [wGolfVBlankMode], a
static void golf_idle(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wGolfParaGoomDuration, 0);
	wr8(gb, wGolfWarioDuration, 0);
	wr8(gb, wGolfParaGoomFramesetOffset, 0);
	wr8(gb, wGolfWarioFramesetOffset, 0);
	wr8(gb, wGolfParaGoomState, 0);
	gb->cpu.a = GOLF_WARIO_IDLING;
	wr8(gb, wGolfWarioState, gb->cpu.a);
	gb->cpu.a = 0x02;
	wr8(gb, wGolfVBlankMode, 0x02);
}

PORT(UpdateGolfParaGoomYPos)
{
	// apply gravity
	uint16_t hl = wShotYVel;
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.d = rd8(gb, hl);
	gb->cpu.e = gb->cpu.a;
	SET_DE(gb, (uint16_t)(REG_DE(gb) - 1));
	gb->cpu.a = gb->cpu.d;
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.e);

	// apply y velocity
	hl = wGolfParaGoomYPos;
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.d = rd8(gb, hl);
	gb->cpu.e = gb->cpu.a;
	SET_HL(gb, wShotYVel);
	gb_call(gb, ADDR_Sub16Bit); // treat wShotYVel as negative
	hl = wGolfParaGoomYPos + 1;
	gb->cpu.a = gb->cpu.d;
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.e);
	SET_HL(gb, hl);
	alu_cp(gb, gb->cpu.a, (uint8_t)(PARAGOOM_GROUND_LEVEL >> 8));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	// y pos >= $100
	gb->cpu.a = rd8(gb, wGroundLevelYPos);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.e);
	if (!(gb->cpu.f & FLAG_C))
		return;

	// Para-Goom has reached the ground
	// snap its Y position to ground
	gb->cpu.a = gb->cpu.d;
	wr8(gb, hl++, gb->cpu.a); // wGolfParaGoomYPos
	gb->cpu.a = (uint8_t)(PARAGOOM_GROUND_LEVEL >> 8);
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);

	gb->cpu.a = alu_and(gb, rd8(gb, wGolfTerrain), GOLF_TERRAIN_MASK);
	gb->cpu.d = gb->cpu.a;
	if (!(gb->cpu.f & FLAG_Z)) {
		alu_cp(gb, gb->cpu.a, GOLF_TERRAIN_OB);
		if (gb->cpu.f & FLAG_C)
			wr8(gb, w1dc47, gb->cpu.a);
	}
	// .asm_1c9521
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w1dc51, 0);
	gb->cpu.a = gb->cpu.d;
	alu_cp(gb, gb->cpu.a, SPECIAL_GOLF_TERRAIN);
	if (gb->cpu.f & FLAG_C) {
		// is either GOLF_TERRAIN_TEE_BOX or GOLF_TERRAIN_PLAIN
		play_sfx(gb, SFX_BUMP);
		gb->cpu.a = rd8(gb, w1dc01);
		wr8(gb, wGolfHoleState, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, GOLFHOLESTATE_BOUNCE);
		if (gb->cpu.f & FLAG_Z)
			return;
		gb->cpu.a = GOLFHOLESTATE_BETWEEN_SHOTS;
		wr8(gb, w1dc01, gb->cpu.a);
		gb_call(gb, ADDR_UpdateGolfHoleDirection);
		golf_idle(gb);
		return;
	}

	// .no_bounce
	gb->cpu.a = GOLFHOLESTATE_SPECIAL_TERRAIN;
	wr8(gb, wGolfHoleState, gb->cpu.a);
	golf_idle(gb);

	gb->cpu.a = rd8(gb, wGolfTerrain);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.a, GOLF_TERRAIN_MASK);
	alu_cp(gb, gb->cpu.a, GOLF_TERRAIN_HOLE);
	if (gb->cpu.f & FLAG_Z) {
		// .hole
		play_sfx(gb, SFX_0EF);
		goto check_edges;
	}
	alu_cp(gb, gb->cpu.a, GOLF_TERRAIN_WATER);
	if (gb->cpu.f & FLAG_Z) {
		// .water
		play_sfx(gb, SFX_SPLASH);
		goto golf_stroke_penalty;
	}
	alu_cp(gb, gb->cpu.a, GOLF_TERRAIN_OB);
	if (gb->cpu.f & FLAG_Z) {
		// .ob
		play_sfx(gb, SFX_073);
		gb->cpu.a = rd8(gb, w1dc53);
		wr8(gb, w1dc47, gb->cpu.a);
		goto golf_stroke_penalty;
	}
	alu_cp(gb, gb->cpu.a, GOLF_TERRAIN_TEE_OB);
	if (gb->cpu.f & FLAG_Z) {
		// .tee_ob
		SET_HL(gb, wGolfStroke);
		wr8(gb, wGolfStroke, alu_inc(gb, rd8(gb, wGolfStroke)));
		return;
	}
	alu_cp(gb, gb->cpu.a, GOLF_TERRAIN_BUNKER);
	if (gb->cpu.f & FLAG_Z) {
		// .bunker
		play_sfx(gb, SFX_065);
		gb->cpu.a = GOLF_PARAGOOM_5;
		wr8(gb, wGolfParaGoomState, gb->cpu.a);
		return;
	}
	// rough
	play_sfx(gb, SFX_0A1);
	return;

golf_stroke_penalty:
	gb->cpu.a = GOLF_PARAGOOM_6;
	wr8(gb, wGolfParaGoomState, gb->cpu.a);
	SET_HL(gb, wGolfStroke);
	wr8(gb, wGolfStroke, alu_inc(gb, rd8(gb, wGolfStroke)));

check_edges:
	hl = w1dc3e;
	SET_HL(gb, hl);
	if (alu_bit(gb, gb->cpu.c, GOLF_TERRAIN_EDGE_RIGHT_F)) {
		gb->cpu.a = rd8(gb, hl++); // .right_edge
	} else if (alu_bit(gb, gb->cpu.c, GOLF_TERRAIN_EDGE_LEFT_F)) {
		gb->cpu.a = rd8(gb, hl++); // .left_edge
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
	} else {
		return;
	}
	// .snap_x_pos
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = rd8(gb, hl);
	for (int i = 0; i < 5; i++) {
		gb->cpu.e = alu_sla(gb, gb->cpu.e);
		gb->cpu.d = alu_rl(gb, gb->cpu.d);
	}
	hl = wGolfParaGoomXPos;
	gb->cpu.a = gb->cpu.e;
	wr8(gb, hl++, gb->cpu.a);
	wr8(gb, hl, gb->cpu.d);
	SET_HL(gb, hl);
}

PORT(GolfHoleState_Shot)
{
	gb_call(gb, ADDR_UpdateGolfParaGoomXPos);
	gb_call(gb, ADDR_UpdateGolfParaGoomYPos);
	gb_call(gb, ADDR_AlignGolfXScrollWithParaGoom);
	gb_call(gb, ADDR_UpdateGolfDisplayMode);

	gb->cpu.a = rd8(gb, wGolfHoleState);
	alu_cp(gb, gb->cpu.a, GOLFHOLESTATE_SHOT);
	if (gb->cpu.f & FLAG_Z)
		return;

	// being here means we changed the hole state
	// and we are doing a bounce
	gb->cpu.a = GOLFHOLESTATE_WALK;
	wr8(gb, w1dc01, gb->cpu.a);
	gb->cpu.a = rd8(gb, wShotSpin);
	uint16_t table = ADDR_GolfHoleState_Shot + 0x69; // .NormalBounceVels
	alu_cp(gb, gb->cpu.a, SHOT_NORMAL);
	if (!(gb->cpu.f & FLAG_Z)) {
		alu_cp(gb, gb->cpu.a, SHOT_TOPSPIN);
		if (gb->cpu.f & FLAG_Z) {
			table = ADDR_GolfHoleState_Shot + 0x7d; // .TopspinBounceVels
		} else {
			// backspin shot
			gb->cpu.a = alu_xor(gb, rd8(gb, wShotDirection), 0x1);
			wr8(gb, wShotDirection, gb->cpu.a);
		}
	}
	// .got_vel_table
	uint16_t de = wShotYVel;
	gb->cpu.a = alu_sla(gb, rd8(gb, wShotPowerLevel));
	gb->cpu.b = 0x00;
	gb->cpu.c = gb->cpu.a;
	uint16_t hl = alu_add16(gb, table, REG_BC(gb));
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, de++, gb->cpu.a); // wShotYVel
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, de++, 0); // w1dc4f
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, de, gb->cpu.a); // wShotXVel
	SET_HL(gb, hl);
	SET_DE(gb, de);
	gb->cpu.a = rd8(gb, wShotTerrainModifier);
	alu_cp(gb, gb->cpu.a, SHOT_MODIFIER_ROUGH);
	if (gb->cpu.f & FLAG_Z) {
		// .rough: (a*3 + 3) / 4
		uint8_t v = rd8(gb, wShotYVel);
		gb->cpu.a = alu_add(gb, v, v);
		gb->cpu.a = alu_add(gb, gb->cpu.a, v);
		gb->cpu.a = alu_add(gb, gb->cpu.a, 3);
		gb->cpu.a = alu_srl(gb, gb->cpu.a);
		gb->cpu.a = alu_srl(gb, gb->cpu.a);
		wr8(gb, wShotYVel, gb->cpu.a);
		SET_HL(gb, wShotYVel + 1);
		return;
	}
	alu_cp(gb, gb->cpu.a, SHOT_MODIFIER_BUNKER);
	if (gb->cpu.f & FLAG_Z) {
		// .bunker: (a + 1) / 2
		gb->cpu.a = alu_inc(gb, rd8(gb, wShotYVel));
		gb->cpu.a = alu_srl(gb, gb->cpu.a);
		wr8(gb, wShotYVel, gb->cpu.a);
		SET_HL(gb, wShotYVel + 1);
	}
}

PORT_JUMP(GolfHoleStateTable)
{
	gb->cpu.a = rd8(gb, wGolfHoleState);
	JUMP_TO(jumptable(gb, ADDR_GolfHoleStateTable + 4));
}

PORT_JUMP(GolfHoleState_Bounce)
{
	gb_call(gb, ADDR_UpdateGolfParaGoomXPos);
	gb_call(gb, ADDR_UpdateGolfParaGoomYPos);
	gb_call(gb, ADDR_AlignGolfXScrollWithParaGoom);
	JUMP_TO(ADDR_UpdateGolfDisplayMode); // same bank
}

PORT_JUMP(UpdateGolfHoleUI)
{
	GOLF_OBJ(GolfObj1, 0x30, 0x0c, ADDR_Frameset_1caf6f);
	CALL(LoadGolfSprite);
	GOLF_OBJ(GolfObj2, 0x30, 0x08, ADDR_Frameset_1caf81);
	CALL(LoadGolfSprite);

	gb->cpu.a = rd8(gb, wGolfHoleState);
	alu_cp(gb, gb->cpu.a, GOLFHOLESTATE_PAUSE);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	GOLF_OBJ(GolfObj3, SCREEN_HEIGHT_PX, 0x58, ADDR_Frameset_1caf8a);
	CALL(LoadGolfSprite);

	gb->cpu.a = rd8(gb, wGolfObj4State);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.a = alu_add(gb, rd8(gb, wShotPowerGaugeCursor), 0x32);
	GOLF_OBJ(GolfObj4, SCREEN_HEIGHT_PX, gb->cpu.a, ADDR_Frameset_1caf84);
	CALL(LoadGolfSprite);

	gb->cpu.a = rd8(gb, wGolfObj5State);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	gb->cpu.a = alu_add(gb, rd8(gb, wSelectedShotPowerGauge), 0x32);
	GOLF_OBJ(GolfObj5, SCREEN_HEIGHT_PX, gb->cpu.a, ADDR_Frameset_1caf87);
	JUMP(LoadGolfSprite);
}

// ld hl, pos / ld a, [hli] / ld d, [hl] / ld e, a / srl d / rr e (x2):
// de = [pos] / 4
static void load_pos_div4(GB *gb, uint16_t pos)
{
	uint16_t hl = pos;
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.d = rd8(gb, hl);
	gb->cpu.e = gb->cpu.a;
	SET_HL(gb, hl);
	for (int i = 0; i < 2; i++) {
		gb->cpu.d = alu_srl(gb, gb->cpu.d);
		gb->cpu.e = alu_rr(gb, gb->cpu.e);
	}
}

PORT_JUMP(UpdateGolfParaGoom)
{
	gb->cpu.a = rd8(gb, w1dc20);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wGolfParaGoomState);
	alu_cp(gb, gb->cpu.a, GOLF_PARAGOOM_8);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	load_pos_div4(gb, wGolfParaGoomXPos);
	gb_push16(gb, REG_DE(gb));
	gb_call(gb, ADDR_Func_1c8f56);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	SET_DE(gb, gb_pop16(gb));
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	// .asm_1c8d96: calculate ParaGoom's coordinates in the screen
	gb->cpu.a = rd8(gb, wSCX);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_add(gb, gb->cpu.e, OAM_X_OFS);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	wr8(gb, wGolfParaGoomXCoord, gb->cpu.a);
	gb->cpu.b = 0x12;
	gb->cpu.a = rd8(gb, wGolfParaGoomState);
	gb->cpu.c = gb->cpu.a;
	alu_cp(gb, gb->cpu.a, GOLF_PARAGOOM_8);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, GOLF_PARAGOOM_5);
	if (gb->cpu.f & FLAG_Z)
		gb->cpu.b = 0x13;
	// .asm_1c8db1
	load_pos_div4(gb, wGolfParaGoomYPos);
	gb->cpu.a = alu_add(gb, gb->cpu.e, gb->cpu.b);
	wr8(gb, wGolfParaGoomYCoord, gb->cpu.a);

	gb->cpu.b = 0x00;
	gb->cpu.c = alu_sla(gb, gb->cpu.c);
	uint16_t hl = alu_add16(gb, ADDR_UpdateGolfParaGoom + 0xa1, REG_BC(gb)); // .Framesets
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = rd8(gb, hl);
	SET_HL(gb, wGolfParaGoomDuration);
	CALL(UpdateGolfObjectAnimation);

	gb->cpu.b = FALSE;
	if (alu_bit(gb, gb->cpu.c, 0))
		gb_call(gb, ADDR_HandleGolfParaGoomRespawn);
	if (alu_bit(gb, gb->cpu.b, 0)) {
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wGolfParaGoomState);
	alu_cp(gb, gb->cpu.a, 0x02);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wGolfParaGoomDuration);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = rd8(gb, wGolfParaGoomFramesetOffset);
			alu_cp(gb, gb->cpu.a, 0x1e);
			if (gb->cpu.f & FLAG_Z) {
				play_sfx(gb, SFX_0E5); // .asm_1c8e01
			} else {
				alu_cp(gb, gb->cpu.a, 0x14);
				if (gb->cpu.f & FLAG_Z)
					play_sfx(gb, SFX_00F);
			}
		}
	}
	// .add_sprite
	gb->cpu.a = rd8(gb, wGolfParaGoomCurrentFrame);
	wr8(gb, wGolfParaGoomFrame, gb->cpu.a);
	SET_HL(gb, wGolfParaGoomSprite);
	JUMP(LoadGolfSprite);
}

PORT(UpdateGolfWario)
{
	load_pos_div4(gb, wGolfWarioXPos);
	gb_push16(gb, REG_DE(gb));
	gb_call(gb, ADDR_Func_1c8f56);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		SET_DE(gb, gb_pop16(gb));
		gb->cpu.a = rd8(gb, w1dc21);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z)
			return;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, w1dc21, 0);

		gb->cpu.a = rd8(gb, wGolfWarioDir);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		bool left = !(gb->cpu.f & FLAG_Z);
		uint16_t hl = wGolfXScroll;
		gb->cpu.a = rd8(gb, hl++);
		gb->cpu.d = rd8(gb, hl);
		gb->cpu.e = gb->cpu.a;
		for (int i = 0; i < 2; i++) {
			gb->cpu.e = alu_sla(gb, gb->cpu.e);
			gb->cpu.d = alu_rl(gb, gb->cpu.d);
		}
		if (!left) {
			gb->cpu.a = alu_sub(gb, gb->cpu.e, 40);
			gb->cpu.e = gb->cpu.a;
			gb->cpu.a = alu_sbc(gb, gb->cpu.d, 0);
			gb->cpu.d = gb->cpu.a;
		} else {
			// .asm_1c8ee5
			gb->cpu.a = alu_add(gb, gb->cpu.e, (uint8_t)680);
			gb->cpu.e = gb->cpu.a;
			gb->cpu.a = alu_adc(gb, gb->cpu.d, (uint8_t)(680 >> 8));
			gb->cpu.d = gb->cpu.a;
		}
		// .asm_1c8efb
		hl = wGolfWarioXPos;
		gb->cpu.a = gb->cpu.e;
		wr8(gb, hl++, gb->cpu.a);
		wr8(gb, hl, gb->cpu.d);
		SET_HL(gb, hl);
		return;
	}
	// .asm_1c8f02
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w1dc21, 0);
	SET_DE(gb, gb_pop16(gb));
	gb->cpu.a = rd8(gb, wSCX);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_add(gb, gb->cpu.e, OAM_X_OFS);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
	wr8(gb, wGolfWarioXCoord, gb->cpu.a);
	gb->cpu.a = alu_add(gb, rd8(gb, wGolfWarioYPos), 0x11);
	wr8(gb, wGolfWarioYCoord, gb->cpu.a);
	CALL(UpdateGolfWarioAnimation);
	gb->cpu.a = rd8(gb, wGolfWarioState);
	alu_cp(gb, gb->cpu.a, GOLF_WARIO_WALKING);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = rd8(gb, wGolfWarioFramesetOffset);
	alu_cp(gb, gb->cpu.a, 0x2);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = rd8(gb, wGolfWarioDuration);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	play_sfx(gb, SFX_WALK);
}

PORT_JUMP(HandleGolfHole)
{
	gb->cpu.a = (uint8_t)(ADDR_OAM_1ca6cf >> 8);
	wr8(gb, wGolfOAMPtr + 0, gb->cpu.a);
	gb->cpu.a = (uint8_t)ADDR_OAM_1ca6cf;
	wr8(gb, wGolfOAMPtr + 1, gb->cpu.a);

	gb_call(gb, ADDR_GolfHoleStateTable);
	gb_call(gb, ADDR_UpdateGolfHoleUI);
	gb_call(gb, ADDR_UpdateGolfParaGoom);
	gb_call(gb, ADDR_UpdateGolfWario);
	CALL(ClearUnusedVirtualOAM);
	gb_call(gb, ADDR_LoadGolfWarioStateGfx);
	JUMP_TO(ADDR_Func_1c9cf9); // same bank
}
