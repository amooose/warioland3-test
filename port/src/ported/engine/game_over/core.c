// Port of src/engine/game_over/core.asm
#include "ported.h"

// ld hl, obj / y / x / 4x 0 / frameset pointer (big-endian); hl ends on its
// low byte
static void init_obj(GB *gb, uint16_t hl, uint8_t y, uint8_t x, uint16_t frameset, bool clear_var)
{
	wr8(gb, hl++, y);
	gb->cpu.a = x;
	wr8(gb, hl++, x);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	for (int i = 0; i < 4; i++)
		wr8(gb, hl++, 0);
	if (clear_var) {
		wr8(gb, wGameOverWarioVar, 0);
		wr8(gb, wSFXTimer, 0);
	}
	wr8(gb, hl++, (uint8_t)(frameset >> 8));
	gb->cpu.a = (uint8_t)frameset;
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT(InitGameOverScreen)
{
	CALL(DisableLCD);
	CALL(FillBGMap0_With7f);
	CALL(ClearVirtualOAM);

	gb_call(gb, ADDR_LoadGameOverPals);
	gb_call(gb, ADDR_LoadGameOverGfx);
	gb_call(gb, ADDR_Func_db4cf);
	CALL(VBlank_GameOver);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCX, 0);
	wr8(gb, wSCX, 0);
	gb->cpu.a = 0x60;
	wr8(gb, rSCY, 0x60);
	wr8(gb, wSCY, 0x60);

	init_obj(gb, wGameOverWario, 0x80, 0x50, ADDR_Frameset_aaccb, true);
	FARCALL_BANK(BANK_Frameset_aaccb, UpdateObjAnim);
	SET_HL(gb, wGameOverWario);
	CALL(LoadGameOverSprite);

	init_obj(gb, wMenuObj1, 0x18, 0x18, ADDR_Frameset_dbbf5, false);
	CALL(UpdateObjAnim);
	SET_HL(gb, wMenuObj1);
	gb_call(gb, ADDR_Func_db4e9);

	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	gb->cpu.a = 64;
	wr8(gb, wTimer, gb->cpu.a);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}

static void inc_var(GB *gb)
{
	SET_HL(gb, wGameOverWarioVar);
	wr8(gb, wGameOverWarioVar, alu_inc(gb, rd8(gb, wGameOverWarioVar)));
}

static void play_music(GB *gb, uint16_t music)
{
	wr8(gb, hMusicID + 0, (uint8_t)(music >> 8));
	gb->cpu.a = (uint8_t)music;
	wr8(gb, hMusicID + 1, gb->cpu.a);
}

PORT(UpdateGameOverScreen)
{
	gb->cpu.a = rd8(gb, wGameOverWarioVar);
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_db338
		SET_HL(gb, wSCY);
		for (int i = 0; i < 3; i++)
			wr8(gb, wSCY, alu_dec(gb, rd8(gb, wSCY)));
		if (gb->cpu.f & FLAG_Z) {
			play_sfx(gb, SFX_061);
			play_music(gb, MUSIC_GOLF_GAME_OVER);
			gb->cpu.a = 6;
			wr8(gb, wTimer, gb->cpu.a);
			inc_var(gb);
		}
		goto update; 
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_db35b
		gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x3);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		SET_HL(gb, wTimer);
		wr8(gb, wTimer, alu_dec(gb, rd8(gb, wTimer)));
		gb->cpu.a = rd8(gb, wTimer);
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x01);
		SET_HL(gb, wSCY);
		if (!(gb->cpu.f & FLAG_Z)) {
			wr8(gb, wSCY, alu_inc(gb, rd8(gb, wSCY)));
			wr8(gb, wSCY, alu_inc(gb, rd8(gb, wSCY)));
			goto update;
		}
		// .asm_db371
		wr8(gb, wSCY, alu_dec(gb, rd8(gb, wSCY)));
		wr8(gb, wSCY, alu_dec(gb, rd8(gb, wSCY)));
		gb->cpu.a = rd8(gb, wTimer);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z) {
			play_music(gb, MUSIC_GAME_OVER);
			inc_var(gb);
		}
		goto update;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_db38a
		gb->cpu.a = rd8(gb, wJoypadPressed);
		if (!alu_bit(gb, gb->cpu.a, B_PAD_A))
			goto update;
		play_sfx(gb, SFX_SELECTION);
		uint16_t hl = wGameOverWarioFramesetOffset;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl++, 0);
		wr8(gb, hl++, 0); // duration
		wr8(gb, hl++, (uint8_t)(ADDR_Frameset_aace2 >> 8));
		gb->cpu.a = (uint8_t)ADDR_Frameset_aace2;
		wr8(gb, hl, gb->cpu.a);
		SET_HL(gb, wSubState);
		wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
		goto update;
	}
	SET_HL(gb, wTimer);
	wr8(gb, wTimer, alu_dec(gb, rd8(gb, wTimer)));
	if (!(gb->cpu.f & FLAG_Z))
		goto update;
	play_sfx(gb, SFX_134);
	inc_var(gb);

update: // .asm_db3a9
	SET_HL(gb, wGameOverWarioFramesetPtr + 1);
	FARCALL_BANK(BANK_Frameset_aace2, UpdateObjAnim);

	gb->cpu.a = rd8(gb, wObjAnimWasReset);
	wr8(gb, wGameOverWarioAnimationEnded, gb->cpu.a);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		play_sfx(gb, SFX_SNORE); // .Func_db3db
	SET_HL(gb, wGameOverWario);
	CALL(LoadGameOverSprite);
	SET_HL(gb, wMenuObj1FramesetPtr + 1);
	CALL(UpdateObjAnim);
	SET_HL(gb, wMenuObj1);
	gb_call(gb, ADDR_Func_db4e9);
	CALL(ClearUnusedVirtualOAM);
}

PORT(Func_db3e4)
{
	SET_HL(gb, wGameOverWarioFramesetPtr + 1);
	FARCALL_BANK(BANK_Frameset_aaccb, UpdateObjAnim);

	gb->cpu.a = rd8(gb, wObjAnimWasReset);
	wr8(gb, wGameOverWarioAnimationEnded, gb->cpu.a);
	SET_HL(gb, wGameOverWario);
	CALL(LoadGameOverSprite);
	SET_HL(gb, wMenuObj1FramesetPtr + 1);
	CALL(UpdateObjAnim);
	SET_HL(gb, wMenuObj1);
	gb_call(gb, ADDR_Func_db4e9);
	CALL(ClearUnusedVirtualOAM);
	gb->cpu.a = rd8(gb, wGameOverWarioAnimationEnded);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}

PORT_JUMP(GameOverReset) { JUMP(Init); }
