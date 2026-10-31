// Port of src/engine/overworld/bank2d.asm: the rest is C++ (src/native/overworld:
// scene_objects, map_functions, scene_init, side_animations).
#include "ported.h"

// unreferenced
PORT(Func_b4895)
{
	uint16_t hl = REG_HL(gb), de = REG_DE(gb);
	gb->cpu.a = rd8(gb, de);
	wr8(gb, hl++, gb->cpu.a);
	de++;
	gb->cpu.a = rd8(gb, de);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, de);
	CALL(GetOWCoordInTilemap);
	de = (uint16_t)(REG_DE(gb) + 1);
	SET_DE(gb, de);
	gb->cpu.a = rd8(gb, de);
	wr8(gb, REG_HL(gb), gb->cpu.a);
	gb->cpu.h = alu_dec(gb, gb->cpu.h);
	gb->cpu.h = alu_dec(gb, gb->cpu.h);
	gb->cpu.h = alu_dec(gb, gb->cpu.h);
	de++;
	SET_DE(gb, de);
	gb->cpu.a = rd8(gb, de);
	wr8(gb, REG_HL(gb), gb->cpu.a);
}
