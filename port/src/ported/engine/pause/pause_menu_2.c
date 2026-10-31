// Port of src/engine/pause/pause_menu_2.asm: DrawLevelObjectsAfterLevelReturn,
// which home code calls (the rest is C++, src/native/pause)
#include "ported.h"

// push_wram BANK("Level Objects WRAM") / farcall / pop_wram
static void farcall_obj_wram(GB *gb, int bank, uint16_t addr)
{
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 0x01;
	wr8(gb, rWBK, gb->cpu.a);
	port_farcall(gb, bank, addr);
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

PORT(DrawLevelObjectsAfterLevelReturn)
{
	FARCALL(VBlank_Level);
	FARCALL(SetTileQueueProcessingFunctions);
	farcall_obj_wram(gb, BANK_DrawObjects_NoPriority, ADDR_DrawObjects_NoPriority);
	CALL(DrawWario);
	farcall_obj_wram(gb, BANK_DrawObjects_WithPriority, ADDR_DrawObjects_WithPriority);
}
