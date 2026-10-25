// Port of src/home/clear_ram.asm
#include "ported.h"

// clear VRAM if hCGB != 0
PORT_JUMP(ClearVRAM)
{
	gb->cpu.a = rd8(gb, hCGB);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a) {
		RET();
		return;
	}
	gb->cpu.a = BANK_v1Tiles0;
	wr8(gb, rVBK, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	SET_HL(gb, 0x8000);
	SET_BC(gb, 0x2000);
	CALL(WriteAToHL_BCTimes);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, rVBK, 0);
	SET_HL(gb, 0x8000);
	SET_BC(gb, 0x2000);
	JUMP(WriteAToHL_BCTimes);
}

PORT(ClearWRAM)
{
	gb->cpu.e = 1; // WRAM1
	do {
		gb->cpu.a = gb->cpu.e;
		wr8(gb, rWBK, gb->cpu.a);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z;
		SET_HL(gb, 0xd000);
		SET_BC(gb, 0x1000);
		CALL(WriteAToHL_BCTimes);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
	} while (!alu_bit(gb, gb->cpu.e, 3));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, rWBK, 0);
}
