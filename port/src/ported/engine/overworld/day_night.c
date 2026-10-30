// Port of src/engine/overworld/day_night.asm
#include "ported.h"

// flips bit 4 of wDayNight
PORT(SwitchDayNight)
{
	SET_HL(gb, wDayNight);
	gb->cpu.a = alu_xor(gb, 1 << 4, rd8(gb, wDayNight));
	wr8(gb, wDayNight, gb->cpu.a);
}

// switches day/night, except on the East side without the Sun Medallion
PORT_JUMP(DoDayNightSpell)
{
	gb->cpu.a = rd8(gb, wCurMapSide);
	alu_cp(gb, gb->cpu.a, EAST);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wGotSunMedallion);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!gb->cpu.a) {
			SET_HL(gb, wDayNight);
			wr8(gb, wDayNight, (uint8_t)(rd8(gb, wDayNight) & ~(1 << 4)));
			RET();
			return;
		}
	}
	JUMP_TO(ADDR_SwitchDayNight);
}
