// Port of src/engine/level/objects/dummy.asm
#include "ported.h"

PORT(DummyObjectFunc)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}
