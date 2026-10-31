// Port of src/home/header.asm
#include "ported.h"

// rst $28: jumps to entry a of the table that follows the rst
PORT_JUMP(JumpTable)
{
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	uint16_t hl = gb_pop16(gb);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	hl = alu_add16(gb, hl, REG_DE(gb));
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	JUMP_TO(REG_HL(gb));
}

// The V-Blank interrupt handler: runs the function in wVBlankFunc when it's
// enabled, keeping the ROM, SRAM, WRAM and VRAM banks it may switch
PORT_JUMP(VBlank)
{
	push_af(gb);
	gb_push16(gb, REG_BC(gb));
	gb_push16(gb, REG_DE(gb));
	gb_push16(gb, REG_HL(gb));
	gb->cpu.a = rd8(gb, wEnableVBlankFunc);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.a) {
		gb->cpu.a = rd8(gb, wROMBank);
		push_af(gb);
		gb->cpu.a = rd8(gb, wSRAMBank);
		push_af(gb);
		gb->cpu.a = rd8(gb, rWBK);
		push_af(gb);
		gb->cpu.a = rd8(gb, rVBK);
		push_af(gb);
		gb_call_ret(gb, wVBlankFunc, ADDR_VBlank + 0x1b);
		pop_af(gb);
		wr8(gb, rVBK, gb->cpu.a);
		pop_af(gb);
		wr8(gb, rWBK, gb->cpu.a);
		pop_af(gb);
		sramswitch(gb, gb->cpu.a);
		pop_af(gb);
		bankswitch(gb, gb->cpu.a);
		gb->cpu.a = TRUE;
		wr8(gb, wVBlankFuncExecuted, gb->cpu.a);
	}
	uint16_t v = gb_pop16(gb);
	SET_HL(gb, v);
	v = gb_pop16(gb);
	SET_DE(gb, v);
	v = gb_pop16(gb);
	SET_BC(gb, v);
	pop_af(gb);
	// reti
	RET();
	gb->cpu.ime = true;
}

// Where the boot ROM hands over: nop / jp Start
PORT_JUMP(EntryPoint) { JUMP_TO(ADDR_Start); }

// The interrupt vectors
PORT_JUMP(VBlankVector) { JUMP_TO(ADDR_VBlank); }
PORT_JUMP(LCDVector) { JUMP_TO(wLCDFunc); }

// reti
static void vector_reti(GB *gb)
{
	RET();
	gb->cpu.ime = true;
}

PORT_JUMP(TimerVector) { vector_reti(gb); }
PORT_JUMP(SerialVector) { vector_reti(gb); }
PORT_JUMP(JoypadVector) { vector_reti(gb); }
