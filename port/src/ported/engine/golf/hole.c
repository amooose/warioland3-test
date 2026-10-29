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
