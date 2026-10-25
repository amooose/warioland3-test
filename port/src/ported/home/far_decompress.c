// Port of src/home/far_decompress.asm
#include "ported.h"

PORT(FarCopyHLToDE_BC2)
{
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = rd8(gb, wTempBank);
	bankswitch(gb, gb->cpu.a);
	CALL(CopyHLToDE_BC);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}

PORT(FarDecompress)
{
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = rd8(gb, wTempBank);
	bankswitch(gb, gb->cpu.a);
	CALL(Decompress);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}
