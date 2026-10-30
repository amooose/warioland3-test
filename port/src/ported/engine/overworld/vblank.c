// Port of src/engine/overworld/vblank.asm
#include "ported.h"

// Both copy their .Func, the VBlank handler that runs from WRAM, to
// wVBlankFunc. The handlers themselves stay asm.

PORT(VBlank_Overworld)
{
	SET_HL(gb, ADDR_VBlank_Overworld + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = (uint8_t)(ADDR_VBlank_OWScene - (ADDR_VBlank_Overworld + 0x0c)); // .end - .Func
	CALL(CopyHLToDE);
}

PORT(VBlank_OWScene)
{
	SET_HL(gb, ADDR_VBlank_OWScene + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = (uint8_t)(ADDR_UnreferencedOpenTreasureCollection - (ADDR_VBlank_OWScene + 0x0c)); // .end - .Func
	CALL(CopyHLToDE);
}

// ld [$ff00+c], a
static void ldh_c(GB *gb)
{
	wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), gb->cpu.a);
}

// ld a, [hli]
static void ld_a_hli(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl);
	SET_HL(gb, (uint16_t)(hl + 1));
}

// ld b, a / ld a, BANK(x) or [wHDMABank] / ...: push the current bank and
// map b (ld a, [wROMBank] / push af / ld a, b / bankswitch)
static void push_bank_switch(GB *gb)
{
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = gb->cpu.b;
	bankswitch(gb, gb->cpu.a);
}

// pop af / bankswitch
static void pop_bank(GB *gb)
{
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}

// [hl] is nonzero: xor a / ld [hli], a, then the 5 HDMA registers from
// b and the 4 bytes after it
static void start_hdma(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, 0);
	SET_HL(gb, (uint16_t)(hl + 1));
	gb->cpu.a = gb->cpu.b;
	gb->cpu.c = (uint8_t)rVDMA_SRC_HIGH;
	ldh_c(gb);
	for (int i = 0; i < 4; i++) {
		ld_a_hli(gb);
		gb->cpu.c = alu_inc(gb, gb->cpu.c);
		ldh_c(gb);
	}
}

// VBlank_Overworld.Func, as copied to wVBlankFunc (see ram_code.c). Its calls
// return to base + n, the block's start wherever it runs from.
PORT_LOCAL_JUMP(VBlank_Overworld, Func)
{
	uint16_t base = gb->cpu.pc;
	gb->cpu.a = 0x02; // BANK("WRAM2")
	wr8(gb, rWBK, gb->cpu.a);
	wr8(gb, rSCY, rd8(gb, wSCY));
	wr8(gb, rSCX, rd8(gb, wSCX));
	wr8(gb, rWY, rd8(gb, wWY));
	gb->cpu.a = alu_add(gb, rd8(gb, wWX), WX_OFS);
	wr8(gb, rWX, gb->cpu.a);

	gb->cpu.b = BANK_Func_1d8c2c;
	push_bank_switch(gb);
	gb_call_ret(gb, ADDR_Func_1d8c2c, (uint16_t)(base + 0x2a));
	pop_bank(gb);

	SET_HL(gb, wPalConfig1);
	gb->cpu.a = alu_and(gb, rd8(gb, wPalConfig1), rd8(gb, wPalConfig1));
	if (gb->cpu.a) {
		gb->cpu.c = gb->cpu.a;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wPalConfig1, 0);
		SET_HL(gb, wPalConfig1 + 1);
		ld_a_hli(gb);
		ldh_c(gb);
		gb->cpu.c = alu_inc(gb, gb->cpu.c);
		ld_a_hli(gb);
		gb->cpu.b = gb->cpu.a;
		ld_a_hli(gb);
		gb->cpu.l = rd8(gb, REG_HL(gb));
		gb->cpu.h = gb->cpu.a;
		do {
			// .asm_80c48
			for (int i = 0; i < 8; i++) {
				ld_a_hli(gb);
				ldh_c(gb);
			}
			gb->cpu.b = alu_dec(gb, gb->cpu.b);
		} while (gb->cpu.b);
	}

	// .asm_80c5b
	gb->cpu.a = rd8(gb, wHDMABank);
	gb->cpu.b = gb->cpu.a;
	push_bank_switch(gb);
	SET_HL(gb, wHDMA);
	gb->cpu.a = alu_and(gb, rd8(gb, wHDMA), rd8(gb, wHDMA));
	if (gb->cpu.a) {
		gb->cpu.b = rd8(gb, wHDMA);
		start_hdma(gb);
	}

	// .asm_80c84
	SET_HL(gb, w2d0b5);
	gb->cpu.a = alu_and(gb, rd8(gb, w2d0b5), rd8(gb, w2d0b5));
	if (gb->cpu.a) {
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = 0x01; // BANK("VRAM1")
		wr8(gb, rVBK, gb->cpu.a);
		start_hdma(gb);
	}

	// .asm_80ca2
	pop_bank(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	JUMP_TO(hTransferVirtualOAM);
}

// ld c, LOW(rVDMA_SRC_HIGH), then a 15-row HDMA from src to dest
static void hdma_15_rows(GB *gb, uint16_t src, uint16_t dest)
{
	gb->cpu.c = (uint8_t)rVDMA_SRC_HIGH;
	gb->cpu.a = (uint8_t)(src >> 8);
	ldh_c(gb);
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	ldh_c(gb);
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.a = (uint8_t)((dest >> 8) - 0x80);
	ldh_c(gb);
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	ldh_c(gb);
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.a = 15 * 2 - 1; // 15 dma_rows
	ldh_c(gb);
}

// VBlank_OWScene.Func, as copied to wVBlankFunc
PORT_LOCAL_JUMP(VBlank_OWScene, Func)
{
	uint16_t base = gb->cpu.pc;
	gb->cpu.a = 0x02;
	wr8(gb, rWBK, gb->cpu.a);
	wr8(gb, rSCY, rd8(gb, wSCY));
	wr8(gb, rSCX, rd8(gb, wSCX));

	gb->cpu.a = alu_and(gb, rd8(gb, wOWPendingTileUpdate), rd8(gb, wOWPendingTileUpdate));
	if (gb->cpu.a) {
		hdma_15_rows(gb, wTilemap, v0BGMap0);
		gb->cpu.a = 0x01; // BANK("VRAM1")
		wr8(gb, rVBK, gb->cpu.a);
		hdma_15_rows(gb, wAttrmap, v1BGMap0);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wOWPendingTileUpdate, 0);
	}

	// .skip_update_tiles
	gb->cpu.a = alu_and(gb, rd8(gb, wHDMABank), rd8(gb, wHDMABank));
	if (gb->cpu.a) {
		gb->cpu.b = gb->cpu.a;
		push_bank_switch(gb);
		gb_call_ret(gb, ADDR_Func_3c03, (uint16_t)(base + 0x57));
		pop_bank(gb);
	}

	// .asm_80d1b
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	gb->cpu.b = BANK_Func_1d8c2c;
	push_bank_switch(gb);
	gb_call_ret(gb, ADDR_Func_1d8c2c, (uint16_t)(base + 0x71));
	pop_bank(gb);

	gb->cpu.b = BANK_ApplyPalConfig1;
	push_bank_switch(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, wPalConfig1Register), rd8(gb, wPalConfig1Register));
	if (gb->cpu.a)
		gb_call_ret(gb, ADDR_ApplyPalConfig1, (uint16_t)(base + 0x8c));
	gb->cpu.a = alu_and(gb, rd8(gb, wPalConfig2), rd8(gb, wPalConfig2));
	if (gb->cpu.a)
		gb_call_ret(gb, ADDR_ApplyPalConfig2, (uint16_t)(base + 0x93));
	pop_bank(gb);

	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	JUMP_TO(hTransferVirtualOAM);
}
