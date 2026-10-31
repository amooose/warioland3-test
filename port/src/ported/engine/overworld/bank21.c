// Port of src/engine/overworld/bank21.asm
#include "ported.h"


// xor a / ld hl, start / ld bc, size / call WriteAToHL_BCTimes
static void clear_bytes(GB *gb, uint16_t start, uint16_t size)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, start);
	SET_BC(gb, size);
	CALL(WriteAToHL_BCTimes);
}

PORT(Func_84fd5)
{
	clear_bytes(gb, wTempBGPals, 8 * PAL_SIZE);
	clear_bytes(gb, wTempOBPals, 8 * PAL_SIZE);
}

// hl = palette config: applies its palettes and clears its register
PORT(ApplyPalConfig_Bank21)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.c = rd8(gb, hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, (uint16_t)(0xff00 | gb->cpu.c), gb->cpu.a);
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.l = rd8(gb, hl);
	gb->cpu.h = gb->cpu.a;
	hl = REG_HL(gb);
	do {
		for (int i = 0; i < 8; i++) {
			gb->cpu.a = rd8(gb, hl++);
			wr8(gb, (uint16_t)(0xff00 | gb->cpu.c), gb->cpu.a);
		}
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	SET_HL(gb, hl);
}

PORT_JUMP(ApplyPalConfig1)
{
	SET_HL(gb, wPalConfig1);
	JUMP_TO(ADDR_ApplyPalConfig_Bank21); // fallthrough
}

PORT_JUMP(ApplyPalConfig2)
{
	SET_HL(gb, wPalConfig2);
	JUMP_TO(ADDR_ApplyPalConfig_Bank21);
}

#define DNT_LOAD_BASE_AND_TARGET_PALS (ADDR_DayNightTransition + 0x2e)
#define DNT_ADVANCE_TABLE (ADDR_DayNightTransition + 0x47)
#define DNT_FADE_COLOURS (ADDR_DayNightTransition + 0x57)
#define DNT_LOAD_PAL1 (ADDR_DayNightTransition + 0xdc)
#define DNT_LOAD_PAL2 (ADDR_DayNightTransition + 0xf2)


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


// ld a, $02 / ldh [rWBK], a / apply wSCY and wSCX
static void vbf_wram2_scroll(GB *gb)
{
	gb->cpu.a = 0x02; // BANK("WRAM2")
	wr8(gb, rWBK, gb->cpu.a);
	wr8(gb, rSCY, rd8(gb, wSCY));
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
}

// VBlank_OWPalTransition.Func, as copied to wVBlankFunc (see ram_code.c).
// Its calls return to base + n, the block's start wherever it runs from.
PORT_LOCAL_JUMP(VBlank_OWPalTransition, Func)
{
	uint16_t base = gb->cpu.pc;
	vbf_wram2_scroll(gb);
	wr8(gb, rWY, rd8(gb, wWY));
	gb->cpu.a = alu_add(gb, rd8(gb, wWX), WX_OFS);
	wr8(gb, rWX, gb->cpu.a);

	gb->cpu.b = BANK_Func_1d8c2c;
	vbf_push_bank_switch(gb);
	gb_call_ret(gb, ADDR_Func_1d8c2c, (uint16_t)(base + 0x2a));
	vbf_pop_bank(gb);

	SET_HL(gb, wTempBGPals);
	gb->cpu.c = (uint8_t)rBGPI;
	gb->cpu.a = BGPI_AUTOINC | 0; // palette 0
	vbf_ldh_c(gb);
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.b = 8; // num pals
	do {
		// .loop_pals
		for (int i = 0; i < 8; i++) {
			vbf_ld_a_hli(gb);
			vbf_ldh_c(gb);
		}
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	JUMP_TO(hTransferVirtualOAM);
}
