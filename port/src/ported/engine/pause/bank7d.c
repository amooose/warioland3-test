// Port of src/engine/pause/bank7d.asm
#include "ported.h"

PORT(LoadSaveScreenPals)
{
	SET_HL(gb, ADDR_Pals_1f628c);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_1f628c);
	CALL(LoadPalsToTempPals2);
}
