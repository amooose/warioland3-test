// Port of src/engine/title/time_attack_jp_adjustment.asm
#include "ported.h"

PORT(AdjustJapaneseTimeAttackDescriptionWindow)
{
	gb->cpu.a = rd8(gb, wLanguage);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.a)
		return; // return if English

	static const uint8_t lines[] = { 0x2f, 0x38, 0x41, 0x5a, 0x63, 0x6c };
	for (int i = 0; i < 6; i++) {
		for (;;) {
			gb->cpu.a = rd8(gb, rLY);
			alu_cp(gb, gb->cpu.a, lines[i]);
			if (!(gb->cpu.f & FLAG_C))
				break;
			idle(gb, 32);
		}
		CALL(WaitNextHBlank);
		gb->cpu.a = alu_add(gb, rd8(gb, rSCY), 0x07);
		wr8(gb, rSCY, gb->cpu.a);
	}
}
