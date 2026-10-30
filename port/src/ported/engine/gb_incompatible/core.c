// Port of src/engine/gb_incompatible/core.asm
#include "ported.h"

PORT(ExitGBIncompatible)
{
}

PORT(GBIncompatible)
{
	CALL(DisableLCD);
	CALL(ClearBGMap0);
	CALL(ClearVirtualOAM);

	FARCALL(LoadGBIncompatibleScreenGfx);
	FARCALL(LoadGBIncompatibleScreenMap);
	CALL(VBlank_GBIncompatible);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSCX, 0);
	wr8(gb, rSCX, 0);
	wr8(gb, wSCY, 0);
	wr8(gb, rSCY, 0);
	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}
