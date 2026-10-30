// Port of src/engine/level/common.asm
#include "ported.h"

// farcall _LevelStateTable / ret
PORT_JUMP(LevelStateTable)
{
	FARCALL_RET(_LevelStateTable, ADDR_LevelStateTable + 0xf);
	RET();
}
