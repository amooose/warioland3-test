// Port of src/engine/language_selection/core.asm
#include "ported.h"

PORT_JUMP(LanguageSelection)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (alu_bit(gb, gb->cpu.a, B_PAD_A)) {
		JUMP_TO(ADDR_StartOverworldStateAndNewGameParam);
		return;
	}
	bool change = true;
	if (alu_bit(gb, gb->cpu.a, B_PAD_DOWN)) {
		gb->cpu.a = rd8(gb, wLanguage);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!gb->cpu.a) {
			RET();
			return;
		}
		gb->cpu.a = 86;
		wr8(gb, wMenuObj1YCoord, 86);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
	} else if (alu_bit(gb, gb->cpu.a, B_PAD_UP)) {
		gb->cpu.a = alu_dec(gb, rd8(gb, wLanguage));
		if (!gb->cpu.a) {
			RET();
			return;
		}
		SET_HL(gb, wMenuObj1);
		gb->cpu.a = 70;
		wr8(gb, wMenuObj1YCoord, 70);
		gb->cpu.a = 0x01;
	} else {
		change = false;
	}
	if (change) {
		wr8(gb, wLanguage, gb->cpu.a);
		gb->cpu.a = alu_add(gb, gb->cpu.a, 0x80);
		wr8(gb, wcee4, gb->cpu.a);
		play_sfx(gb, SFX_0E2);
	}
	SET_HL(gb, wMenuObj1End - 1);
	CALL(UpdateObjAnim);
	SET_HL(gb, wMenuObj1);
	gb_call(gb, ADDR_LoadLanguageSelectionSprite);
	CALL(ClearUnusedVirtualOAM);
	RET();
}
