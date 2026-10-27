// Port of src/engine/title/start_menu.asm
#include "ported.h"

PORT(StartOverworldState)
{
	gb->cpu.a = ST_OVERWORLD;
	wr8(gb, wState, ST_OVERWORLD);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSubState, 0);
}
