// Port of src/home/hcall.asm
#include "ported.h"

#define FUNC (ADDR_InitHRAMCallFunc + 0x0c) // .Func
#define FUNC_SIZE 0x17                      // .end - .Func

// copies hCallFunc's code to HRAM
PORT(InitHRAMCallFunc)
{
	SET_HL(gb, FUNC);
	SET_DE(gb, hCallFunc);
	gb->cpu.b = FUNC_SIZE;
	CALL(CopyHLToDE);
}
