// Port of src/engine/golf/building/building.asm
#include "ported.h"

// loads attribute map to wGolfCourseTileMap
// and palettes to w1db00 depending on wGolfCourse
PORT_JUMP(Func_1c87db)
{
	gb->cpu.a = BANK_GolfBuildingCourse1Tilemap; // BANK("Golf Gfx 2")
	wr8(gb, wTempBank, gb->cpu.a);
	gb->cpu.a = rd8(gb, wGolfCourse);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.b = 0;
	gb->cpu.c = gb->cpu.a;
	uint16_t hl = alu_add16(gb, ADDR_Func_1c87db + 0x2d, REG_BC(gb)); // .data
	gb->cpu.e = gb->cpu.a = rd8(gb, hl++);
	gb->cpu.d = gb->cpu.a = rd8(gb, hl++);
	gb_push16(gb, hl); // push hl
	SET_HL(gb, REG_DE(gb));
	SET_DE(gb, wGolfCourseTileMap);
	SET_BC(gb, 7 * TILEMAP_WIDTH);
	CALL(FarCopyHLToDE_BC2);
	hl = gb_pop16(gb); // pop hl
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.h = rd8(gb, hl);
	gb->cpu.l = gb->cpu.a;
	SET_DE(gb, w1db00);
	gb->cpu.b = 8 * 8;
	JUMP(CopyHLToDE);
}

// gets the golf course scroll X value and writes it as the target SCX
PORT(GetGolfCourseTargetSCX)
{
	gb->cpu.a = rd8(gb, wGolfCourse);
	gb->cpu.b = 0;
	gb->cpu.c = gb->cpu.a;
	uint16_t hl = alu_add16(gb, ADDR_GetGolfCourseTargetSCX + 0x0f, REG_BC(gb)); // .scroll_x
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wGolfMenuTargetSCX, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT(Func_1c88dc)
{
	gb->cpu.a = rd8(gb, wGolfMenuTargetSCX);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, wGolfMenuScrollingDir);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	bool left = gb->cpu.a != 0;
	gb->cpu.a = rd8(gb, wSCX);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0x02;
		wr8(gb, w1dc00, 0x02);
		wr8(gb, wGolfVBlankMode, 0x02);
		return;
	}
	if (left)
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 4);
	else
		gb->cpu.a = alu_add(gb, gb->cpu.a, 4);
	wr8(gb, wSCX, gb->cpu.a);
}

PORT(Func_1c8907)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w1dc00, 0);
	gb->cpu.a = 0x03;
	wr8(gb, wGolfVBlankMode, 0x03);
}

PORT(Func_1c8911)
{
	gb->cpu.a = rd8(gb, wGolfCounter);
	alu_cp(gb, gb->cpu.a, 0x32);
	if (gb->cpu.f & FLAG_C) {
		wr8(gb, wGolfCounter, alu_inc(gb, rd8(gb, wGolfCounter)));
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x03);
		if (gb->cpu.f & FLAG_Z)
			play_sfx(gb, SFX_12B);
		gb->cpu.a = rd8(gb, wGolfNumCoins + 1);
		gb->cpu.a = alu_sub(gb, gb->cpu.a, 1);
		gb->cpu.a = alu_daa(gb, gb->cpu.a);
		wr8(gb, wGolfNumCoins + 1, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, wGolfNumCoins), 0);
		wr8(gb, wGolfNumCoins, gb->cpu.a);
		SET_HL(gb, wGolfNumCoins);
		return;
	}
	play_sfx(gb, SFX_SELECTION);
	// .SetGolfHole
	gb->cpu.a = rd8(gb, wGolfMenuOption);
	gb->cpu.b = 0;
	gb->cpu.c = gb->cpu.a;
	uint16_t hl = alu_add16(gb, ADDR_Func_1c8911 + 0x42, REG_BC(gb)); // .first_golf_holes
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wPredeterminedGolfHole, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = 0x04;
	wr8(gb, wSubState, 0x04);
}

// ld a, e / add (or sub) n / ld e, a
static void e_add(GB *gb, uint8_t n)
{
	gb->cpu.a = alu_add(gb, gb->cpu.e, n);
	gb->cpu.e = gb->cpu.a;
}
static void e_sub(GB *gb, uint8_t n)
{
	gb->cpu.a = alu_sub(gb, gb->cpu.e, n);
	gb->cpu.e = gb->cpu.a;
}

// ld a, t1 / ld [de], a / e += TILEMAP_WIDTH / ld a, t2 / ld [de], a
static void put_two_tiles(GB *gb, uint8_t t1, uint8_t t2)
{
	gb->cpu.a = t1;
	wr8(gb, REG_DE(gb), t1);
	e_add(gb, TILEMAP_WIDTH);
	gb->cpu.a = t2;
	wr8(gb, REG_DE(gb), t2);
}

PORT(Func_1c8604)
{
	gb->cpu.a = BANK_GolfMenuGfx;
	wr8(gb, wTempBank, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_GolfMenuGfx);
	SET_BC(gb, v0Tiles0);
	CALL(FarDecompress);

	gb->cpu.a = BANK_GolfBuildingCoursesAttrmap; // BANK("Golf Gfx 2")
	wr8(gb, wTempBank, gb->cpu.a);
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_GolfBuildingCoursesAttrmap);
	SET_BC(gb, v1BGMap0);
	CALL(FarDecompress);

	gb->cpu.a = rd8(gb, wGolfMenuOption);
	wr8(gb, wGolfCourse, gb->cpu.a);
	gb_call(gb, ADDR_Func_1c87db);
	gb_call(gb, ADDR_GetGolfCourseTargetSCX);
	SET_HL(gb, wGolfCourseTileMap);
	SET_DE(gb, v0BGMap0 + 6 * TILEMAP_WIDTH); // debgcoord 0, 6
	gb->cpu.b = 7 * TILEMAP_WIDTH;
	CALL(CopyHLToDE);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	gb->cpu.a = rd8(gb, wHasAllMusicalCoinFlags);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		SET_HL(gb, ADDR_GolfBuildingCoursesLockedTilemap);
	else
		SET_HL(gb, ADDR_GolfBuildingCoursesUnlockedTilemap);
	// .got_tilemap
	SET_BC(gb, v0BGMap0);
	CALL(FarDecompress);

	uint16_t hl = wGolfBestScores;
	SET_DE(gb, v0BGMap0 + 13 * TILEMAP_WIDTH + 2); // debgcoord 2, 13
	for (;;) {
		// .loop_courses
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, GOLF_BASE_SCORE + 100);
		if (!(gb->cpu.f & FLAG_C)) {
			// .invalid_score
			gb->cpu.b = 10;
			gb->cpu.c = 10;
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, REG_DE(gb), 0);
			e_add(gb, TILEMAP_WIDTH);
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, REG_DE(gb), 0);
		} else {
			alu_cp(gb, gb->cpu.a, GOLF_BASE_SCORE + 1);
			bool calc = true;
			if (!(gb->cpu.f & FLAG_C)) {
				// .positive_score
				put_two_tiles(gb, 0xd2, 0xd3);
				gb->cpu.a = alu_sub(gb, rd8(gb, hl), 0x14);
				gb->cpu.c = gb->cpu.a;
			} else {
				alu_cp(gb, gb->cpu.a, GOLF_BASE_SCORE + 0);
				if (gb->cpu.f & FLAG_Z) {
					// .zero_score
					gb->cpu.b = 0;
					gb->cpu.c = 0;
					put_two_tiles(gb, 0xbe, 0xbf);
					calc = false;
				} else {
					// negative score
					put_two_tiles(gb, 0xd4, 0xd5);
					gb->cpu.a = alu_sub(gb, GOLF_BASE_SCORE, rd8(gb, hl));
					gb->cpu.c = gb->cpu.a;
				}
			}
			if (calc) {
				// .calculate_digits
				gb->cpu.b = 0;
				for (;;) {
					alu_cp(gb, gb->cpu.a, 10);
					if (gb->cpu.f & FLAG_C)
						break;
					gb->cpu.a = alu_sub(gb, gb->cpu.a, 10);
					gb->cpu.c = gb->cpu.a;
					gb->cpu.b = alu_inc(gb, gb->cpu.b);
				}
			}
		}

		// .got_digits: b = tens digit, c = ones digit
		e_sub(gb, TILEMAP_WIDTH - 1);
		gb->cpu.a = alu_add(gb, gb->cpu.b, gb->cpu.b); // *2
		gb->cpu.a = alu_add(gb, gb->cpu.a, 0xa0);
		gb->cpu.b = gb->cpu.a;
		wr8(gb, REG_DE(gb), gb->cpu.a);
		e_add(gb, TILEMAP_WIDTH);
		gb->cpu.a = alu_inc(gb, gb->cpu.b);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		e_sub(gb, TILEMAP_WIDTH - 1);
		gb->cpu.a = alu_add(gb, gb->cpu.c, gb->cpu.c); // *2
		gb->cpu.a = alu_add(gb, gb->cpu.a, 0xa0);
		gb->cpu.c = gb->cpu.a;
		wr8(gb, REG_DE(gb), gb->cpu.a);
		e_add(gb, TILEMAP_WIDTH);
		gb->cpu.a = alu_inc(gb, gb->cpu.c);
		wr8(gb, REG_DE(gb), gb->cpu.a);

		// next course
		gb->cpu.a = (uint8_t)hl;
		alu_cp(gb, gb->cpu.a, (uint8_t)(wGolfBestScores + NUM_GOLF_COURSES - 1));
		if (gb->cpu.f & FLAG_Z)
			break;
		hl++;
		e_sub(gb, 0x1a);
	}
	SET_HL(gb, hl);
}

PORT_JUMP(Func_1c8837)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		// .a_btn
		gb->cpu.a = rd8(gb, wNumCoins + 0);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = rd8(gb, wNumCoins + 1);
			alu_cp(gb, gb->cpu.a, GOLF_BUILDING_PRICE);
			if (gb->cpu.f & FLAG_C) {
				play_sfx(gb, SFX_0E5);
				RET();
				return;
			}
		}
		// .enough_coins
		gb->cpu.a = alu_sub(gb, rd8(gb, wNumCoins + 1), GOLF_BUILDING_PRICE);
		gb->cpu.a = alu_daa(gb, gb->cpu.a);
		wr8(gb, wNumCoins + 1, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, wNumCoins + 0), 0);
		wr8(gb, wNumCoins + 0, gb->cpu.a);
		gb->cpu.a = 0x03;
		wr8(gb, w1dc00, gb->cpu.a);
		RET();
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_B)) {
		// .b_btn
		gb->cpu.a = 0x08;
		wr8(gb, wSubState, gb->cpu.a);
		RET();
		return;
	}
	uint16_t hl = wGolfMenuOption;
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		// .d_right
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wGolfMenuScrollingDir, 0);
		gb->cpu.a = rd8(gb, hl);
		bool wrap;
		alu_cp(gb, gb->cpu.a, GOLF_COURSE_4);
		if (gb->cpu.f & FLAG_Z) {
			wrap = true;
		} else {
			alu_cp(gb, gb->cpu.a, GOLF_COURSE_3);
			wrap = false;
			if (gb->cpu.f & FLAG_Z) {
				gb->cpu.a = rd8(gb, wHasAllMusicalCoinFlags);
				alu_and(gb, gb->cpu.a, gb->cpu.a);
				wrap = (gb->cpu.f & FLAG_Z) != 0;
			}
		}
		if (wrap)
			wr8(gb, hl, GOLF_COURSE_1); // .wrap_around_right
		else
			wr8(gb, hl, alu_inc(gb, rd8(gb, hl))); // .scroll_right
	} else if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		// .d_left
		gb->cpu.a = 0x1; // left
		wr8(gb, wGolfMenuScrollingDir, gb->cpu.a);
		gb->cpu.a = rd8(gb, hl);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z)) {
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl))); // .scroll_left
		} else {
			gb->cpu.a = rd8(gb, wHasAllMusicalCoinFlags);
			alu_and(gb, gb->cpu.a, gb->cpu.a);
			if (gb->cpu.f & FLAG_Z)
				wr8(gb, hl, GOLF_COURSE_3);
			else
				wr8(gb, hl, GOLF_COURSE_4); // .wrap_around_left
		}
	} else {
		RET();
		return;
	}

	// .scrolling
	SET_HL(gb, hl);
	play_sfx(gb, SFX_0E2);
	gb->cpu.a = 0x01;
	wr8(gb, w1dc00, gb->cpu.a);
	wr8(gb, wGolfVBlankMode, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wGolfCourse, gb->cpu.a);
	gb_call(gb, ADDR_Func_1c87db);
	SET_HL(gb, ADDR_GolfBuildingCourseScrollingAttrmap);
	SET_DE(gb, wGoldBuildingCourseAttrMap);
	SET_BC(gb, 7 * TILEMAP_WIDTH);
	CALL(FarCopyHLToDE_BC2);
	JUMP_TO(ADDR_GetGolfCourseTargetSCX);
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

PORT_JUMP(Func_1c86f8)
{
	gb->cpu.a = (uint8_t)(ADDR_OAM_1cb3e0 >> 8);
	wr8(gb, wGolfOAMPtr + 0, gb->cpu.a);
	gb->cpu.a = (uint8_t)ADDR_OAM_1cb3e0;
	wr8(gb, wGolfOAMPtr + 1, gb->cpu.a);
	GOLF_OBJ(GolfObj1, 0x42, 0x58, ADDR_Frameset_1cb59b);
	CALL(LoadGolfSprite);
	GOLF_OBJ(GolfObj3, 0x2a, 0x58, ADDR_Frameset_1cb598);
	CALL(LoadGolfSprite);
	GOLF_OBJ(GolfObj4, SCREEN_WIDTH_PX, 0x58, ADDR_Frameset_1cb59e);
	CALL(LoadGolfSprite);

	gb->cpu.a = rd8(gb, w1dc00);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	GOLF_OBJ(GolfObj5, 0x76, 0x6c, ADDR_Frameset_1cb5a1);
	CALL(LoadGolfSprite);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wGolfWarioDuration, 0);
	wr8(gb, wGolfWarioFramesetOffset, 0);
	gb->cpu.a = 0x76;
	wr8(gb, wGolfWarioYCoord, 0x76);
	gb->cpu.a = 0x6c;
	wr8(gb, wGolfWarioXCoord, 0x6c);
	gb->cpu.a = alu_add(gb, rd8(gb, wGolfMenuOption), rd8(gb, wGolfMenuOption));
	gb->cpu.b = 0x00;
	gb->cpu.c = gb->cpu.a;
	uint16_t hl = alu_add16(gb, ADDR_Func_1c86f8 + 0xdb, REG_BC(gb)); // .framesets
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = rd8(gb, hl);
	SET_HL(gb, wGolfWarioDuration);
	CALL(UpdateGolfObjectAnimation);
	gb->cpu.a = rd8(gb, wGolfWarioCurrentFrame);
	wr8(gb, wGolfWarioFrame, gb->cpu.a);
	SET_HL(gb, wGolfWarioSprite);
	CALL(LoadGolfSprite);

	GOLF_OBJ(GolfParaGoom, 0x50, 0x55, ADDR_Frameset_1cb5c3);
	JUMP(LoadGolfSprite);
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

PORT_JUMP(Func_1c882b)
{
	gb->cpu.a = rd8(gb, w1dc00);
	JUMP_TO(jumptable(gb, ADDR_Func_1c882b + 4));
}

PORT_JUMP(Func_1c86dd)
{
	gb_call(gb, ADDR_Func_1c882b);
	gb_call(gb, ADDR_Func_1c86f8);
	CALL(ClearUnusedVirtualOAM);
	gb->cpu.a = rd8(gb, wSubState);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, w1d800, 0);
		wr8(gb, wGolfMenuOption, 0);
	}
	JUMP_TO(ADDR_Func_1c9fae); // same bank
}

PORT(Func_1c8586)
{
	CALL(DisableLCD);
	CALL(ClearVirtualOAM);
	gb->cpu.a = 0x08;
	wr8(gb, wAnimatedTilesFrameDuration, 0x08);
	gb->cpu.a = 0x0f;
	wr8(gb, wAnimatedTilesGfx, 0x0f);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wAnimatedTilesFrameCount, 0);
	wr8(gb, wAnimatedTilesFrame, 0);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, wRoomAnimatedTilesEnabled, gb->cpu.a);
	gb->cpu.a = alu_and(gb, rd8(gb, w1d800), rd8(gb, w1d800));
	if (gb->cpu.f & FLAG_Z)
		CALL(SaveBackupVRAM);

	gb->cpu.a = alu_and(gb, rd8(gb, wGolfMenuOption), rd8(gb, wGolfMenuOption));
	if (!gb->cpu.a) {
		gb->cpu.a = 0x01;
		wr8(gb, w1d800, 0x01);
	}

	// .asm_1c85b3: clear all Golf RAM starting from wPredeterminedGolfHole onwards
	uint16_t hl = wPredeterminedGolfHole;
	for (uint16_t bc = 0x800 - (wPredeterminedGolfHole - 0xd800); bc; bc--)
		wr8(gb, hl++, 0);
	SET_HL(gb, hl);
	SET_BC(gb, 0);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // or c

	CALL(CheckHasAllMusicalCoinFlags);
	wr8(gb, wHasAllMusicalCoinFlags, gb->cpu.a);

	gb_call(gb, ADDR_Func_1c8604);
	SET_HL(gb, w1db00);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_1ca34f);
	CALL(LoadPalsToTempPals2);

	gb->cpu.a = rd8(gb, wNumCoins + 0);
	wr8(gb, wGolfNumCoins + 0, gb->cpu.a);
	gb->cpu.a = rd8(gb, wNumCoins + 1);
	wr8(gb, wGolfNumCoins + 1, gb->cpu.a);

	gb_call(gb, ADDR_Func_1c86f8);
	gb_call(gb, ADDR_Func_1c9fae);

	SET_BC(gb, MUSIC_GOLF_LOBBY);
	CALL(PlayNewMusic);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCY, 0);
	wr8(gb, wSCY, 0);
	gb->cpu.a = rd8(gb, wGolfMenuTargetSCX);
	wr8(gb, rSCX, gb->cpu.a);
	wr8(gb, wSCX, gb->cpu.a);
	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}
