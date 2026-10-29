// Port of src/engine/clear/musical_coins.asm
#include "ported.h"

PORT(Func_d45e1)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTimer + 0, 0);
	gb->cpu.a = 2;
	wr8(gb, wTimer + 1, 2);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, SST_CLEAR_EXIT);
}

PORT(Func_d464e)
{
	SET_HL(gb, ADDR_Pals_d50a4);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_d50e4);
	CALL(LoadPalsToTempPals2);
}

PORT(Func_d465b)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_GolfMenuGfx);
	SET_BC(gb, v0Tiles0);
	CALL(Decompress);
}

PORT(Func_d4668)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_GolfMenuAttrmap);
	SET_BC(gb, v1BGMap0);
	CALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_GolfMenuTilemap);
	SET_BC(gb, v0BGMap0);
	CALL(Decompress);
}

// ld hl, wMenuObj3FramesetPtr + 1 / farcall $7c, UpdateObjAnim /
// ld hl, wMenuObj3 / farcall LoadPauseMenuSprite / call ClearUnusedVirtualOAM
static void update_obj3(GB *gb)
{
	SET_HL(gb, wMenuObj3FramesetPtr + 1);
	FARCALL_BANK(0x7c, UpdateObjAnim);
	SET_HL(gb, wMenuObj3);
	FARCALL(LoadPauseMenuSprite);
	CALL(ClearUnusedVirtualOAM);
}

PORT_JUMP(Func_d4472)
{
	update_obj3(gb);
	SET_HL(gb, wNumMusicalCoins);
	gb->cpu.a = rd8(gb, wMusicalCoinVisualCounter);
	alu_cp(gb, gb->cpu.a, rd8(gb, wNumMusicalCoins));
	if (!(gb->cpu.f & FLAG_Z)) {
		SET_HL(gb, wTimer);
		gb->cpu.a = rd8(gb, wTimer);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (gb->cpu.a) {
			wr8(gb, wTimer, alu_dec(gb, rd8(gb, wTimer)));
			RET();
			return;
		}
		gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
		if (gb->cpu.a) {
			RET();
			return;
		}
		play_sfx(gb, SFX_REGULAR_COIN);
		uint16_t hl = wBGMapTileVRAM0Queue;
		wr8(gb, hl++, (uint8_t)((v0BGMap0 + 0x8b) >> 8));
		wr8(gb, hl++, (uint8_t)(v0BGMap0 + 0x8b));
		gb->cpu.a = alu_inc(gb, rd8(gb, wMusicalCoinVisualCounter));
		wr8(gb, wMusicalCoinVisualCounter, gb->cpu.a);
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
		gb->cpu.a = alu_add(gb, gb->cpu.a, 0xa0);
		gb->cpu.b = gb->cpu.a;
		wr8(gb, hl++, gb->cpu.a);
		wr8(gb, hl++, 0x98);
		wr8(gb, hl++, 0xab);
		gb->cpu.b = alu_inc(gb, gb->cpu.b);
		wr8(gb, hl, gb->cpu.b);
		SET_HL(gb, hl);
		gb->cpu.a = 0x80;
		wr8(gb, wcee4, 0x80);
		RET();
		return;
	}
	gb->cpu.a = alu_inc(gb, rd8(gb, wAllMusicalCoinsLevel));
	if (!gb->cpu.a) {
		JUMP_TO(ADDR_Func_d45e1);
		return;
	}
	play_sfx(gb, SFX_03A);
	uint16_t hl = wMenuObj1;
	wr8(gb, hl++, 0x42);
	wr8(gb, hl++, 0x60);
	gb->cpu.f = FLAG_Z; // xor a
	for (int i = 0; i < 4; i++)
		wr8(gb, hl++, 0);
	wr8(gb, hl++, (uint8_t)(ADDR_Frameset_d65f0 >> 8));
	gb->cpu.a = (uint8_t)ADDR_Frameset_d65f0;
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	CALL(UpdateObjAnim);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
	RET();
}

PORT_JUMP(ExitClearScreen)
{
	update_obj3(gb);
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		SET_HL(gb, wTimer);
		wr8(gb, wTimer, alu_dec(gb, rd8(gb, wTimer)));
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		SET_HL(gb, wTimer + 1);
		wr8(gb, wTimer + 1, alu_dec(gb, rd8(gb, wTimer + 1)));
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTimer + 0, 0);
	wr8(gb, wTimer + 1, 0);
	gb->cpu.a = rd8(gb, wGameModeFlags);
	if (!alu_bit(gb, gb->cpu.a, MODE_TIME_ATTACK_F)) {
		gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), ACTION_HELP_LEVEL_CLEAR);
		if (gb->cpu.a) {
			JUMP(OpenActionHelp);
			return;
		}
	}
	FARCALL(Func_4628);
	RET();
}

PORT(InitMusicalCoinScreen)
{
	CALL(DisableLCD);
	CALL(ClearBGMap0);
	CALL(ClearVirtualOAM);

	gb_call(gb, ADDR_Func_d464e);
	gb_call(gb, ADDR_Func_d465b);
	gb_call(gb, ADDR_Func_d4668);
	gb_call(gb, ADDR_Func_d47f1);

	gb->cpu.a = 0xff;
	wr8(gb, wAllMusicalCoinsLevel, 0xff);
	gb->cpu.a = rd8(gb, wNumMusicalCoins);
	alu_cp(gb, gb->cpu.a, NUM_LEVEL_MUSICAL_COINS);
	if (gb->cpu.f & FLAG_Z)
		gb_call(gb, ADDR_SetMusicalCoinFlag);

	gb_call(gb, ADDR_VBlank_MusicalCoinsScreen);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSCX, 0);
	wr8(gb, rSCX, 0);
	wr8(gb, wSCY, 0);
	wr8(gb, rSCY, 0);

	wr8(gb, wMenuObj3 + 0, 0x38); // y
	wr8(gb, wMenuObj3 + 1, 0x40); // x
	for (int i = 2; i < 6; i++)
		wr8(gb, wMenuObj3 + i, 0);
	wr8(gb, wMenuObj3 + 6, ADDR_Frameset_1f1cd4 >> 8);
	gb->cpu.a = ADDR_Frameset_1f1cd4 & 0xff;
	wr8(gb, wMenuObj3 + 7, gb->cpu.a);
	SET_HL(gb, wMenuObj3 + 7);
	FARCALL_BANK(0x7c, UpdateObjAnim);
	SET_HL(gb, wMenuObj3);
	FARCALL(LoadPauseMenuSprite);

	gb->cpu.a = 8;
	wr8(gb, wAnimatedTilesFrameDuration, 8);
	gb->cpu.a = ANIMATED_TILES_GFX_15;
	wr8(gb, wAnimatedTilesGfx, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wAnimatedTilesFrameCount, 0);
	wr8(gb, wAnimatedTilesFrame, 0);
	gb->cpu.a = alu_inc(gb, gb->cpu.a); // TRUE
	wr8(gb, wRoomAnimatedTilesEnabled, gb->cpu.a);
	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wMusicalCoinVisualCounter, 0);
	gb->cpu.a = 60;
	wr8(gb, wTimer, 60);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}
