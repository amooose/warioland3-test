// Port of src/home/clear_memory.asm
#include "ported.h"

// clears whole wVirtualOAM
PORT(ClearVirtualOAM)
{
	SET_HL(gb, wVirtualOAM);
	gb->cpu.b = OAM_SIZE;
	gb->cpu.a = 0;
	CALL(WriteAToHL_BTimes);
	wr8(gb, wVirtualOAMByteSize, 0);
	gb->cpu.f = FLAG_Z; // xor a
}

// fills v0BGMap0 and v1BGMap0 with byte in a
PORT(FillBGMap0)
{
	gb->cpu.d = gb->cpu.a;
	wr8(gb, rVBK, BANK_v1BGMap0);
	SET_HL(gb, v1BGMap0);
	SET_BC(gb, v1BGMap1 - v1BGMap0);
	gb->cpu.a = 0;
	CALL(WriteAToHL_BCTimes);

	wr8(gb, rVBK, gb->cpu.a); // a is still 0: VRAM0
	gb->cpu.a = gb->cpu.d;
	SET_HL(gb, v0BGMap0);
	SET_BC(gb, v0BGMap1 - v0BGMap0);
	CALL(WriteAToHL_BCTimes);
}

// clears unused sprites that are in wVirtualOAM
PORT(ClearUnusedVirtualOAM)
{
	uint8_t used = rd8(gb, wVirtualOAMByteSize);
	gb->cpu.l = used;
	gb->cpu.h = (uint8_t)(wVirtualOAM >> 8);
	if (used < OAM_SIZE) {
		// one 4-byte OAM struct per iteration. As in the asm, a remainder
		// under 4 would make b = 0 and loop 256 times; it never happens
		// because sprites are always added 4 bytes at a time.
		gb->cpu.b = (uint8_t)((OAM_SIZE - used) >> 2);
		int n = gb->cpu.b ? gb->cpu.b : 256;
		uint16_t hl = REG_HL(gb);
		for (int i = 0; i < n * 4; i++)
			wr8(gb, hl++, 0);
		SET_HL(gb, hl);
		gb->cpu.b = 0;
	}
	gb->cpu.a = 0;
	wr8(gb, wVirtualOAMByteSize, 0);
	gb->cpu.f = FLAG_Z; // xor a
}

// fills v0BGMap0 and v1BGMap0 with $7f
PORT(FillBGMap0_With7f)
{
	gb->cpu.a = 0x7f;
	CALL(FillBGMap0);
}

PORT(ClearBGMap1)
{
	gb->cpu.a = 0xff;
	SET_HL(gb, v0BGMap1);
	SET_BC(gb, v0End - v0BGMap1);
	CALL(WriteAToHL_BCTimes);
}
