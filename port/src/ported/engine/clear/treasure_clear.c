// Port of src/engine/clear/treasure_clear.asm
#include "ported.h"

PORT(OpenMusicalCoinsScreen)
{
	gb->cpu.a = SST_CLEAR_MUSICAL_COINS;
	wr8(gb, wSubState, SST_CLEAR_MUSICAL_COINS);
}
