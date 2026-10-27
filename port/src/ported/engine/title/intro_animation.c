// Port of src/engine/title/intro_animation.asm
#include "ported.h"

static void dec_hl(GB *gb, uint16_t addr)
{
	SET_HL(gb, addr);
	wr8(gb, addr, alu_dec(gb, rd8(gb, addr)));
}

// decreases wIntroBGXOffsetFar every 4 ticks
// decreases wIntroBGXOffsetCentre by 1
// decreases wIntroBGXOffsetNear by 2
PORT(ScrollIntroBackgroundLayers)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
	if (!gb->cpu.a)
		dec_hl(gb, wIntroBGXOffsetFar);
	dec_hl(gb, wIntroBGXOffsetCentre);
	dec_hl(gb, wIntroBGXOffsetNear);
	dec_hl(gb, wIntroBGXOffsetNear);
}

// Scrolls a layer by 1 until it's aligned to 32 pixels, then sets it to
// wSCX.
static void align_layer(GB *gb, uint16_t layer)
{
	SET_HL(gb, layer);
	gb->cpu.a = alu_and(gb, rd8(gb, layer), 0x1f);
	if (gb->cpu.a) {
		wr8(gb, layer, alu_dec(gb, rd8(gb, layer)));
		return;
	}
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, layer, gb->cpu.a);
}

// scrolls the intro background layers
// until they align to the centre
PORT(FinishIntroBackgroundScroll)
{
	// only update far layer every 4 ticks
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
	if (!gb->cpu.a)
		align_layer(gb, wIntroBGXOffsetFar);
	align_layer(gb, wIntroBGXOffsetCentre);

	SET_HL(gb, wIntroBGXOffsetNear);
	gb->cpu.a = rd8(gb, wIntroBGXOffsetNear);
	alu_cp(gb, gb->cpu.a, 224);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 224;
		wr8(gb, wIntroBGXOffsetNear, 224);
		return;
	}
	dec_hl(gb, wIntroBGXOffsetNear);
	dec_hl(gb, wIntroBGXOffsetNear);
}

// Moves the obj's x right by 1, unless it's past 176 and either in
// wSubState 5 on or in wSubState 4 with wPlaneWarioVar set. Returns
// whether it went on (rather than returning, for the second obj).
static bool move_intro_obj(GB *gb, uint16_t x)
{
	SET_HL(gb, x);
	gb->cpu.a = rd8(gb, x);
	alu_cp(gb, gb->cpu.a, 176);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = rd8(gb, wSubState);
		alu_cp(gb, gb->cpu.a, 0x05);
		if (!(gb->cpu.f & FLAG_C))
			return false;
		alu_cp(gb, gb->cpu.a, 0x04);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.a = rd8(gb, wPlaneWarioVar);
			alu_and(gb, gb->cpu.a, gb->cpu.a);
			if (!(gb->cpu.f & FLAG_Z))
				return false;
		}
	}
	wr8(gb, x, alu_inc(gb, rd8(gb, x)));
	return true;
}

PORT(Func_4d45)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x01);
	if (gb->cpu.a)
		move_intro_obj(gb, wMenuObj5XCoord);
	move_intro_obj(gb, wMenuObj4XCoord);
}

// waits until rLY reaches ly, letting time pass
static void wait_ly(GB *gb, uint8_t ly)
{
	for (;;) {
		gb->cpu.a = rd8(gb, rLY);
		alu_cp(gb, gb->cpu.a, ly);
		if (!(gb->cpu.f & FLAG_C))
			break;
		idle(gb, 32);
	}
}

// pans the background of the intro airplane scene
// each horizontal layer scrolls at different speed
PORT(PanIntroBackgroundLayers)
{
	gb->cpu.a = rd8(gb, wceef);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.a)
		return;
	wait_ly(gb, 0x5b); // sky
	CALL(WaitNextHBlank);
	gb->cpu.a = rd8(gb, wIntroBGXOffsetFar);
	wr8(gb, rSCX, gb->cpu.a);
	wait_ly(gb, 0x63); // far layer
	CALL(WaitNextHBlank);
	gb->cpu.a = rd8(gb, wIntroBGXOffsetCentre);
	wr8(gb, rSCX, gb->cpu.a);
	wait_ly(gb, 0x73); // centre layer
	CALL(WaitNextHBlank);
	gb->cpu.a = rd8(gb, wIntroBGXOffsetNear);
	wr8(gb, rSCX, gb->cpu.a);
	wait_ly(gb, 0x83); // near layer
	CALL(WaitNextHBlank);
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
}
