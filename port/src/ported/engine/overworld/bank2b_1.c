// Port of src/engine/overworld/bank2b_1.asm: its unreferenced routines and
// the V-Blank functions copied to wVBlankFunc. The rest is C++
// (src/native/overworld: SceneCommon, SceneWario, SceneSprites, the scenes).
#include "ported.h"

// ld hl, counter / xor a / ld [hld], a / inc [hl]
static void reset_and_advance(GB *gb, uint16_t counter)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, counter, 0);
	SET_HL(gb, counter - 1);
	wr8(gb, counter - 1, alu_inc(gb, rd8(gb, counter - 1)));
}

// c = frame to wait for
static void wait_and_advance(GB *gb, uint16_t counter)
{
	gb->cpu.a = rd8(gb, counter);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (gb->cpu.f & FLAG_C)
		return;
	reset_and_advance(gb, counter);
}

PORT(Func_ad330) { wait_and_advance(gb, w2d891); }
PORT(Func_ad335) { reset_and_advance(gb, w2d891); }
PORT(Func_ad011) { wait_and_advance(gb, w2d014); }
PORT(Func_ad016) { reset_and_advance(gb, w2d014); }

// unreferenced
PORT(Func_aca63)
{
	SET_HL(gb, wOWUIObj1XCoord);
	wr8(gb, wOWUIObj1XCoord, alu_dec(gb, rd8(gb, wOWUIObj1XCoord)));
}

PORT(Func_aca68)
{
	SET_HL(gb, wOWUIObj1XCoord);
	wr8(gb, wOWUIObj1XCoord, alu_inc(gb, rd8(gb, wOWUIObj1XCoord)));
}

// unreferenced
PORT_JUMP(DebugSceneWario)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = rd8(gb, wDebugSceneWarioState);
	if (alu_bit(gb, gb->cpu.b, B_PAD_RIGHT)) {
		alu_cp(gb, gb->cpu.a, NUM_SCENEWARIO_STATES - 1);
		if (gb->cpu.f & FLAG_C)
			gb->cpu.a = alu_inc(gb, gb->cpu.a);
	} else {
		if (!alu_bit(gb, gb->cpu.b, B_PAD_LEFT)) {
			RET();
			return;
		}
		uint8_t a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (a)
			gb->cpu.a = alu_dec(gb, gb->cpu.a);
	}
	wr8(gb, wDebugSceneWarioState, gb->cpu.a);
	JUMP_TO(ADDR_SetSceneWarioState); // fallthrough
}

// unreferenced
PORT_JUMP(Func_ad329)
{
	gb->cpu.a = rd8(gb, wSceneWarioState);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_ad335);
}

PORT_JUMP(Func_ad33c)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d890, 0);
	wr8(gb, w2d891, 0);
	SET_HL(gb, w2d891);
	JUMP_TO(ADDR_Func_ad016);
}

// ld [$ff00+c], a
static void vbf_ldh_c(GB *gb)
{
	wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), gb->cpu.a);
}

// ld a, [hli]
static void vbf_ld_a_hli(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl);
	SET_HL(gb, (uint16_t)(hl + 1));
}

// ld a, [wROMBank] / push af / ld a, b / bankswitch
static void vbf_push_bank_switch(GB *gb)
{
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = gb->cpu.b;
	bankswitch(gb, gb->cpu.a);
}

// pop af / bankswitch
static void vbf_pop_bank(GB *gb)
{
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}

// ld a, [wLCDCFlagsToFlip] / and a / jr z, .skip / ld hl, rLCDC / xor [hl] /
// ld [hl], a / xor a / ld [wLCDCFlagsToFlip], a
static void vbf_flip_lcdc(GB *gb)
{
	gb->cpu.a = rd8(gb, wLCDCFlagsToFlip);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a)
		return;
	SET_HL(gb, rLCDC);
	gb->cpu.a = alu_xor(gb, gb->cpu.a, rd8(gb, rLCDC));
	wr8(gb, rLCDC, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wLCDCFlagsToFlip, 0);
}

// ld a, $02 / ldh [rWBK], a / apply wSCY and wSCX
static void vbf_wram2_scroll(GB *gb)
{
	gb->cpu.a = 0x02; // BANK("WRAM2")
	wr8(gb, rWBK, gb->cpu.a);
	wr8(gb, rSCY, rd8(gb, wSCY));
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
}

// VBlank_PrologueEpilogue.Func, as copied to wVBlankFunc (see ram_code.c).
// Its calls return to base + n, the block's start wherever it runs from.
PORT_LOCAL_JUMP(VBlank_PrologueEpilogue, Func)
{
	uint16_t base = gb->cpu.pc;
	vbf_wram2_scroll(gb);

	gb->cpu.a = alu_and(gb, rd8(gb, wHDMABank), rd8(gb, wHDMABank));
	if (gb->cpu.a) {
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wHDMABank, 0);
		vbf_push_bank_switch(gb);
		gb->cpu.a = rd8(gb, wHDMADestVRAMBank);
		wr8(gb, rVBK, gb->cpu.a);
		SET_HL(gb, wHDMA);
		vbf_ld_a_hli(gb);
		gb->cpu.c = (uint8_t)rVDMA_SRC_HIGH;
		vbf_ldh_c(gb);
		for (int i = 0; i < 4; i++) {
			vbf_ld_a_hli(gb);
			gb->cpu.c = alu_inc(gb, gb->cpu.c);
			vbf_ldh_c(gb);
		}
		vbf_pop_bank(gb);
	}

	// .skip_hdma_config
	SET_HL(gb, wPendingCharDest);
	gb->cpu.a = alu_and(gb, rd8(gb, wPendingCharDest), rd8(gb, wPendingCharDest));
	if (gb->cpu.a) {
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wPendingCharDest, 0);
		SET_HL(gb, wPendingCharDest + 1);
		vbf_ld_a_hli(gb);
		gb->cpu.d = gb->cpu.a;
		vbf_ld_a_hli(gb);
		wr8(gb, REG_DE(gb), gb->cpu.a); // wPendingCharTile
		gb->cpu.a = 0x01; // BANK("VRAM1")
		wr8(gb, rVBK, gb->cpu.a);
		gb->cpu.a = rd8(gb, REG_HL(gb)); // wPendingCharAttr
		wr8(gb, REG_DE(gb), gb->cpu.a);
	}

	// .skip_char
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, wPalConfig1);
	gb->cpu.a = alu_and(gb, rd8(gb, wPalConfig1), rd8(gb, wPalConfig1));
	if (gb->cpu.a)
		gb_call_ret(gb, ADDR_ApplyPalConfig, (uint16_t)(base + 0x62));

	SET_HL(gb, wPalConfig2);
	gb->cpu.a = alu_and(gb, rd8(gb, wPalConfig2), rd8(gb, wPalConfig2));
	if (gb->cpu.a)
		gb_call_ret(gb, ADDR_ApplyPalConfig, (uint16_t)(base + 0x6a));

	vbf_flip_lcdc(gb);

	// .dma_transfer
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	JUMP_TO(hTransferVirtualOAM);
}

// _GolfBuilding.func, as copied to wVBlankFunc (see ram_code.c): starts the
// HDMA transfer in wHDMA, if there's one
PORT_LOCAL_JUMP(_GolfBuilding, func)
{
	gb->cpu.a = 2; // BANK("WRAM2")
	wr8(gb, rWBK, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCY);
	wr8(gb, rSCY, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
	gb->cpu.a = rd8(gb, wHDMABank);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wHDMABank, 0);
		gb->cpu.a = rd8(gb, wROMBank);
		push_af(gb);
		gb->cpu.a = gb->cpu.b;
		bankswitch(gb, gb->cpu.a);
		uint16_t hl = wHDMA;
		gb->cpu.c = (uint8_t)rVDMA_SRC_HIGH;
		for (int i = 0; i < 5; i++) {
			if (i)
				gb->cpu.c = alu_inc(gb, gb->cpu.c);
			gb->cpu.a = rd8(gb, hl++);
			wr8(gb, (uint16_t)(0xff00 | gb->cpu.c), gb->cpu.a);
		}
		SET_HL(gb, hl);
		pop_af(gb);
		bankswitch(gb, gb->cpu.a);
	}
	// .done
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	JUMP_TO(hTransferVirtualOAM);
}