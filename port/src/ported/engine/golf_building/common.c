// Port of src/engine/golf_building/common.asm
#include "ported.h"

// bc and de are dead after it (MainStateTable returns to the main loop): its
// states can leave them as the audio entry points do (see src/native/audio).
PORT_EX(GolfBuildingStateTable, 0, R_B | R_C | R_D | R_E)
{
	// push_wram BANK("Golf RAM")
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 0x01;
	wr8(gb, rWBK, gb->cpu.a);
	FARCALL(_GolfBuildingStateTable);
	// pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}
