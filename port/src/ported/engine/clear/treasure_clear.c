// Port of src/engine/clear/treasure_clear.asm
#include "ported.h"

PORT(OpenMusicalCoinsScreen)
{
	gb->cpu.a = SST_CLEAR_MUSICAL_COINS;
	wr8(gb, wSubState, SST_CLEAR_MUSICAL_COINS);
}

static void load_clear_screen_sprite(GB *gb, uint16_t obj)
{
	SET_HL(gb, obj);
	gb_call(gb, ADDR_LoadClearScreenSprite);
}

PORT(UpdateClearScreen)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wPowerUpLevel), ACTION_HELP_LEVEL_CLEAR);
	if (gb->cpu.a) {
		SET_HL(gb, wMenuObj8FramesetPtr + 1);
		CALL(UpdateObjAnim);
		load_clear_screen_sprite(gb, wMenuObj8);
	}
	SET_HL(gb, wClearScreenWarioFramesetPtr + 1);
	CALL(UpdateObjAnim);
	gb->cpu.a = alu_and(gb, rd8(gb, wClearScreenWarioVar), rd8(gb, wClearScreenWarioVar));
	if (!gb->cpu.a) {
		gb->cpu.a = alu_and(gb, rd8(gb, wObjAnimWasReset), rd8(gb, wObjAnimWasReset));
		if (gb->cpu.a) {
			gb->cpu.a = TRUE;
			wr8(gb, wClearScreenWarioVar, TRUE);
			// reset animation
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wClearScreenWarioFramesetOffset + 0, 0);
			wr8(gb, wClearScreenWarioFramesetOffset + 1, 0);
			gb->cpu.a = rd8(gb, wTransitionParam);
			alu_cp(gb, gb->cpu.a, TRANSITION_RETURN_TO_MAP);
			uint16_t fs = (gb->cpu.f & FLAG_Z) ? ADDR_Frameset_d4d5f : ADDR_Frameset_d4d1b;
			wr8(gb, wClearScreenWarioFramesetOffset + 2, fs >> 8);
			gb->cpu.a = fs & 0xff;
			wr8(gb, wClearScreenWarioFramesetOffset + 3, gb->cpu.a);
			SET_HL(gb, wClearScreenWarioFramesetOffset + 3);
			CALL(UpdateObjAnim);
		}
	}
	SET_HL(gb, wClearScreenWario);
	gb_call(gb, ADDR_LoadClearScreenWarioSprite);

	load_clear_screen_sprite(gb, wMenuObj2);
	gb->cpu.a = rd8(gb, wLevelEndScreen);
	alu_cp(gb, gb->cpu.a, LVLEND_NO_TREASURE);
	if (!(gb->cpu.f & FLAG_Z)) {
		SET_HL(gb, wMenuObj1FramesetPtr + 1);
		CALL(UpdateObjAnim);
		load_clear_screen_sprite(gb, wMenuObj1);
	}

	// .draw_music_boxes
	gb->cpu.a = alu_and(gb, rd8(gb, wNumMusicBoxes), rd8(gb, wNumMusicBoxes));
	int n = 0;
	if (gb->cpu.a) {
		n = 5;
		for (int i = 1; i <= 4; i++) {
			gb->cpu.a = alu_dec(gb, gb->cpu.a);
			if (!gb->cpu.a) {
				n = i;
				break;
			}
		}
	}
	static const uint16_t boxes[] = { wMenuObj7, wMenuObj6, wMenuObj5, wMenuObj4, wMenuObj3 };
	for (int i = n - 1; i >= 0; i--)
		load_clear_screen_sprite(gb, boxes[i]);
	CALL(ClearUnusedVirtualOAM);

	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_A))
		return;
	// fallthrough to OpenMusicalCoinsScreen
	gb->cpu.a = SST_CLEAR_MUSICAL_COINS;
	wr8(gb, wSubState, gb->cpu.a);
}
