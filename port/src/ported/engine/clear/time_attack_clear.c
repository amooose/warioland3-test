// Port of src/engine/clear/time_attack_clear.asm
#include "ported.h"

static void load_clear_screen_sprite(GB *gb, uint16_t obj)
{
	SET_HL(gb, obj);
	gb_call(gb, ADDR_LoadClearScreenSprite);
}

PORT_JUMP(Func_d434c)
{
	gb->cpu.a = rd8(gb, wTimeAttackResult);
	alu_cp(gb, gb->cpu.a, TIME_ATTACK_GOT_HIGH_SCORE);
	if (gb->cpu.f & FLAG_Z) {
		SET_HL(gb, wMenuObj8FramesetPtr + 1);
		CALL(UpdateObjAnim);
		load_clear_screen_sprite(gb, wMenuObj8);
	}
	SET_HL(gb, wClearScreenWarioFramesetPtr + 1);
	CALL(UpdateObjAnim);
	gb->cpu.a = rd8(gb, wActionHelpWarioVar);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a) {
		gb->cpu.a = rd8(gb, wObjAnimWasReset);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (gb->cpu.a) {
			gb->cpu.a = 0x01;
			wr8(gb, wActionHelpWarioVar, 0x01);
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wClearScreenWarioFramesetOffset + 0, 0);
			wr8(gb, wClearScreenWarioFramesetOffset + 1, 0);
			gb->cpu.a = rd8(gb, wTimeAttackResult);
			alu_cp(gb, gb->cpu.a, TIME_ATTACK_GOT_HIGH_SCORE);
			uint16_t fs = (gb->cpu.f & FLAG_Z) ? ADDR_Frameset_d4d1b : ADDR_Frameset_d4d5f;
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
	load_clear_screen_sprite(gb, wMenuObj3);

	static const struct { int bit; uint16_t obj; } keys[] = {
		{ GREY_KEY_F, wMenuObj4 },
		{ RED_KEY_F, wMenuObj5 },
		{ GREEN_KEY_F, wMenuObj6 },
		{ BLUE_KEY_F, wMenuObj7 },
	};
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, wKeyAndTreasureFlags);
		if (alu_bit(gb, gb->cpu.a, keys[i].bit))
			load_clear_screen_sprite(gb, keys[i].obj);
	}
	CALL(ClearUnusedVirtualOAM);

	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (!alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_OpenMusicalCoinsScreen);
}
