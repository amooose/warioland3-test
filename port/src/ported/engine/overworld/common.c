// Port of src/engine/overworld/common.asm
#include "ported.h"

PORT(HandleOverworld)
{
	// push_wram BANK("WRAM2")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 0x02;
	wr8(gb, rWBK, gb->cpu.a);
	FARCALL(OverworldStateTable);
	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);

	// if a level has been selected
	// handle the transition
	SET_HL(gb, wState);
	gb->cpu.a = rd8(gb, wState);
	alu_cp(gb, gb->cpu.a, ST_LEVEL);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	play_sfx(gb, SFX_SELECTION);
	gb_call(gb, ADDR_SelectLevel);
	gb->cpu.a = rd8(gb, wLevel);
	alu_cp(gb, gb->cpu.a, GOLF_BUILDING);
	if (gb->cpu.f & FLAG_Z) {
		// .golf_building
		gb->cpu.a = ST_GOLF_BUILDING;
		wr8(gb, wState, gb->cpu.a);
		return;
	}
	alu_cp(gb, gb->cpu.a, THE_TEMPLE);
	if (gb->cpu.f & FLAG_Z) {
		// .the_temple: stop_sfx
		gb->cpu.a = 0xff;
		wr8(gb, hSFXID + 0, gb->cpu.a);
		gb->cpu.a = 0x00;
		wr8(gb, hSFXID + 1, gb->cpu.a);
	}
}
