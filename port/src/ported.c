// Runtime support for hand-ported routines.
#include "ported.h"

#include <stdio.h>
#include <stdlib.h>

void port_register(const PortInfo *info)
{
	hook_register(info->name, info->fn, info->flags, info->clobbers);
}

// A plain call or jump into a ROMX routine only works if its bank is
// mapped; catch porting mistakes early rather than running the wrong code.
static void check_bank(GB *gb, int bank, uint16_t addr, const char *what, const char *name)
{
	if (addr >= 0x4000 && addr < 0x8000 && gb->rom_bank != bank) {
		fprintf(stderr, "port: %s %s (bank %02x) while bank %02x is mapped\n", what, name, bank,
			gb->rom_bank);
		abort();
	}
}

void port_call(GB *gb, int bank, uint16_t addr, const char *name)
{
	check_bank(gb, bank, addr, "CALL", name);
	gb_call(gb, addr);
}

void port_jump(GB *gb, int bank, uint16_t addr, const char *name)
{
	check_bank(gb, bank, addr, "JUMP", name);
	gb->cpu.pc = addr;
}

void port_hcall(GB *gb, uint16_t addr)
{
	// Same as the hcall macro: ld a, LOW / ldh / ld a, HIGH / ldh /
	// call hCallFunc
	wr8(gb, hCallFuncPointer, (uint8_t)addr);
	gb->cpu.a = (uint8_t)(addr >> 8);
	wr8(gb, hCallFuncPointer + 1, gb->cpu.a);
	gb_call(gb, hCallFunc);
}

void port_farcall(GB *gb, int bank, uint16_t addr)
{
	// Same as the farcall macro: ld a, BANK / ldh [hCallFuncBank], a / hcall
	wr8(gb, hCallFuncBank, (uint8_t)bank);
	port_hcall(gb, addr);
}
