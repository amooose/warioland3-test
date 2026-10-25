// Port of src/home/clear_save.asm
#include "ported.h"

// clears some SRAM stuff
PORT_JUMP(ClearSaveData)
{
	static const uint16_t areas[] = {sCheckVals, sBackupCheckVals, s0a000, s0a800, s0a400, s0ac00};
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	for (int i = 0; i < 6; i++) {
		SET_HL(gb, areas[i]);
		gb->cpu.b = 8;
		CALL(WriteAToHL_BTimes);
	}
	gb->cpu.a = 0;
	wr8(gb, hUnused_fffd, 0);
	JUMP(Init);
}
