// Port of src/home/door_transition.asm
#include "ported.h"

PORT(VBlank_DoorTransition)
{
	gb->cpu.ime = false; // di
	// back up current VBlank function
	SET_HL(gb, wVBlankFunc);
	SET_DE(gb, wBackupVBlankFunc);
	gb->cpu.b = 3;
	CALL(CopyHLToDE);

	// then overwrite it
	SET_HL(gb, ADDR_VBlank_DoorTransition + 0x19); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 3; // .end - .Func
	CALL(CopyHLToDE);
	gb->cpu.ime = true; // ei
	gb->cpu.ime_delay = 0;
}
