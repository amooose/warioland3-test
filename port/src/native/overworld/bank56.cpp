// overworld/bank56 (src/engine/overworld/bank56.asm): the one V-Blank
// function, run from wVBlankFunc; the rest is C++ (src/native/overworld).
#include "../native.hpp"
#include "../routines.hpp"

// _InitBestTimeList.func, as copied to wVBlankFunc (see ram_code.c)
PORT_LOCAL_EX(_InitBestTimeList, func, 0, ALL_REGISTERS & ~R_A)
{
	wl3::Memory mem(gb);
	mem[rWBK] = 0x02;
	mem[rSCY] = mem[wSCY];
	mem[rSCX] = mem[wSCX];
	mem[rVBK] = 0;
	gb->cpu.a = static_cast<uint8_t>(wVirtualOAM >> 8);
	gb_tail_jump(gb, hTransferVirtualOAM);
}
