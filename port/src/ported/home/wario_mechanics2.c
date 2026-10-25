// Port of src/home/wario_mechanics2.asm
#include "ported.h"

PORT(OpenActionHelp)
{
	gb->cpu.a = ST_ACTION_HELP;
	wr8(gb, wState, ST_ACTION_HELP);
	wr8(gb, wState + 1, 0); // wSubState
	SET_HL(gb, wState + 1);
}
