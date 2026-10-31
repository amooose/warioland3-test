// Port of src/engine/overworld/bank56.asm: only the best time list's V-Blank
// function, run from wVBlankFunc; the rest is C++ (src/native/overworld).
#include "ported.h"

// _InitBestTimeList.func, as copied to wVBlankFunc (see ram_code.c)
PORT_LOCAL_JUMP(_InitBestTimeList, func)
{
	gb->cpu.a = 0x02;
	wr8(gb, rWBK, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCY);
	wr8(gb, rSCY, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	JUMP_TO(hTransferVirtualOAM);
}
