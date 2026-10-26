// Port of src/engine/title/plane_sfx.asm
#include "ported.h"

// play_sfx_rept 12, sfx: plays sfx every 12 + 1 frames
static void play_sfx_rept(GB *gb, uint16_t sfx)
{
	gb->cpu.a = alu_sub(gb, rd8(gb, wSFXTimer), 1);
	wr8(gb, wSFXTimer, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_C))
		return;
	gb->cpu.a = 12;
	wr8(gb, wSFXTimer, 12);
	play_sfx(gb, sfx);
}

// The jr / jp of the other routines here end up in these, then ret.
PORT(PlayIntroSFXPlane2) { play_sfx_rept(gb, SFX_PLANE2); }
PORT(PlayIntroSFXPlane3) { play_sfx_rept(gb, SFX_PLANE3); }

PORT(PlayIntroSFXPlane_Close)
{
	gb->cpu.a = rd8(gb, wPlaneWarioXCoord);
	alu_cp(gb, gb->cpu.a, SCREEN_WIDTH_PX);
	if (!(gb->cpu.f & FLAG_C)) {
		play_sfx_rept(gb, SFX_PLANE3);
		return;
	}
	alu_cp(gb, gb->cpu.a, 112);
	play_sfx_rept(gb, (gb->cpu.f & FLAG_C) ? SFX_PLANE1 : SFX_PLANE2);
}

PORT(PlayIntroSFXPlane_Far)
{
	gb->cpu.a = rd8(gb, wPlaneWarioXCoord);
	alu_cp(gb, gb->cpu.a, SCREEN_WIDTH_PX);
	play_sfx_rept(gb, (gb->cpu.f & FLAG_C) ? SFX_PLANE2 : SFX_PLANE3);
}
