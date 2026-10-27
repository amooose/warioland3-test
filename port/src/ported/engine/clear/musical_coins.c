// Port of src/engine/clear/musical_coins.asm
#include "ported.h"

PORT(Func_d45e1)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTimer + 0, 0);
	gb->cpu.a = 2;
	wr8(gb, wTimer + 1, 2);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, SST_CLEAR_EXIT);
}

PORT(Func_d464e)
{
	SET_HL(gb, ADDR_Pals_d50a4);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_d50e4);
	CALL(LoadPalsToTempPals2);
}

PORT(Func_d465b)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_GolfMenuGfx);
	SET_BC(gb, v0Tiles0);
	CALL(Decompress);
}

PORT(Func_d4668)
{
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, ADDR_GolfMenuAttrmap);
	SET_BC(gb, v1BGMap0);
	CALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_GolfMenuTilemap);
	SET_BC(gb, v0BGMap0);
	CALL(Decompress);
}
