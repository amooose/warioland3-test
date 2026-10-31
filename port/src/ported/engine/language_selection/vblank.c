// Port of src/engine/language_selection/vblank.asm: the V-Blank function
// (the rest is C++, src/native/screens)
#include "ported.h"

// ld a, BANK("VRAM1") / ldh [rVBK], a / then hlbgcoord 5, 7 and 5, 9 filled
// with 13 times `top` and `bottom` (the call returns at base + ret1, ret2)
static void lang_pals(GB *gb, uint16_t base, uint8_t top, uint8_t bottom, uint8_t ret1, uint8_t ret2)
{
	gb->cpu.a = 1;
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, v0BGMap0 + 7 * TILEMAP_WIDTH + 5);
	gb->cpu.b = 13;
	gb->cpu.a = top;
	gb_call_ret(gb, ADDR_WriteAToHL_BTimes, (uint16_t)(base + ret1));
	SET_HL(gb, v0BGMap0 + 9 * TILEMAP_WIDTH + 5);
	gb->cpu.b = 13;
	gb->cpu.a = bottom;
	gb_call_ret(gb, ADDR_WriteAToHL_BTimes, (uint16_t)(base + ret2));
}

// VBlank_LanguageSelection.Func, as copied to wVBlankFunc
PORT_LOCAL_JUMP(VBlank_LanguageSelection, Func)
{
	uint16_t base = gb->cpu.pc;
	gb->cpu.a = rd8(gb, wcee4);
	if (alu_bit(gb, gb->cpu.a, 7)) {
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x7f);
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z))
			lang_pals(gb, base, 0x07, 0x06, 0x1a, 0x24); // japanese
		else
			lang_pals(gb, base, 0x06, 0x07, 0x34, 0x3e); // .english
		SET_HL(gb, wcee4);
		wr8(gb, wcee4, (uint8_t)(rd8(gb, wcee4) & 0x7f));
	}
	gb->cpu.a = rd8(gb, wSCY);
	wr8(gb, rSCY, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	gb_call_ret(gb, hTransferVirtualOAM, (uint16_t)(base + 0x52));
	RET();
}
