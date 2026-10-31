// The title screen's register-level parts (see src/native/title): the
// raster effects, which wait on rLY as the original does, and the V-Blank
// function copied to wVBlankFunc.
#include "ported.h"

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

// VBlank_Title.Func, as copied to wVBlankFunc (see ram_code.c). Its calls
// return to base + n, the block's start wherever it runs from.
PORT_LOCAL_JUMP(VBlank_Title, Func)
{
	uint16_t base = gb->cpu.pc;
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = BANK_TitleScreenBoxTilemap;
	bankswitch(gb, gb->cpu.a);
	gb->cpu.a = rd8(gb, wcee4);
	if (alu_bit(gb, gb->cpu.a, 7)) {
		SET_HL(gb, ADDR_TitleScreenBoxTilemap);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a (VRAM0)
		wr8(gb, rVBK, gb->cpu.a);
		SET_DE(gb, v0BGMap0 + 14 * TILEMAP_WIDTH + 4); // debgcoord 4, 14
		gb_push16(gb, REG_DE(gb));
		gb->cpu.b = 12;
		gb_call_ret(gb, ADDR_CopyHLToDE, (uint16_t)(base + 0x22));
		SET_DE(gb, gb_pop16(gb));
		gb->cpu.a = alu_add(gb, gb->cpu.e, TILEMAP_WIDTH);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = alu_adc(gb, gb->cpu.d, 0);
		gb->cpu.d = gb->cpu.a;
		gb->cpu.b = 12;
		gb_call_ret(gb, ADDR_CopyHLToDE, (uint16_t)(base + 0x30));
		SET_HL(gb, wcee4);
		wr8(gb, wcee4, (uint8_t)(rd8(gb, wcee4) & 0x7f));
	}
	// .skip
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCY);
	wr8(gb, rSCY, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	gb_call_ret(gb, hTransferVirtualOAM, (uint16_t)(base + 0x4b));
	RET();
}
