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
// Under test, a fuzzed state can get there with any bank mapped (the asm
// would run whatever the bank holds): the check is unrealistic, and the
// call is skipped. Returns false then.
static bool check_bank(GB *gb, int bank, uint16_t addr, const char *what, const char *name)
{
	if (addr >= 0x4000 && addr < 0x8000 && gb->rom_bank != bank) {
		if (gb->test.active) {
			gb->test.unrealistic = true;
			return false;
		}
		fprintf(stderr, "port: %s %s (bank %02x) while bank %02x is mapped\n", what, name, bank,
			gb->rom_bank);
		abort();
	}
	return true;
}

void port_call(GB *gb, int bank, uint16_t addr, const char *name)
{
	// in play, a native callee runs straight from its bank
	if (!gb->test.active && gb_call_native_in(gb, bank, addr))
		return;
	if (check_bank(gb, bank, addr, "CALL", name))
		gb_call(gb, addr);
}

void port_jump(GB *gb, int bank, uint16_t addr, const char *name)
{
	check_bank(gb, bank, addr, "JUMP", name);
	gb->cpu.pc = addr;
}

static void hcall_ret(GB *gb, uint16_t addr, uint16_t ret)
{
	// Same as the hcall macro: ld a, LOW / ldh / ld a, HIGH / ldh /
	// call hCallFunc
	wr8(gb, hCallFuncPointer, (uint8_t)addr);
	gb->cpu.a = (uint8_t)(addr >> 8);
	wr8(gb, hCallFuncPointer + 1, gb->cpu.a);
	uint8_t entry_bank = rd8(gb, wROMBank);
	// In play, a native callee runs straight from its bank and address, no
	// bank switched: hCallFunc's effects but the bank's (the callee gets a
	// = the bank; after, a = the entry bank and f as at the call, from its
	// push af / pop af)
	if (!ret && !gb->test.active) {
		uint8_t bank = rd8(gb, hCallFuncBank), f = gb->cpu.f, a = gb->cpu.a;
		gb->cpu.a = bank;
		if (gb_call_native_in(gb, bank, addr)) {
			gb->cpu.a = entry_bank;
			gb->cpu.f = f;
			return;
		}
		gb->cpu.a = a;
	}
	if (ret)
		gb_call_ret(gb, hCallFunc, ret);
	else
		gb_call(gb, hCallFunc);
	// hCallFunc always maps the entry bank back before it returns. Coming
	// back with another bank mapped means the callee unwound hCallFunc's
	// own frame (a fuzzed jump table landing mid-routine on a pop), so the
	// asm caller goes on to run code from the wrong bank, which C code
	// can't follow.
	if (gb->test.active && gb->rom_bank != entry_bank)
		gb->test.unrealistic = true;
}

void port_hcall(GB *gb, uint16_t addr)
{
	hcall_ret(gb, addr, 0);
}

void port_farcall(GB *gb, int bank, uint16_t addr)
{
	port_farcall_ret(gb, bank, addr, 0);
}

void port_farcall_ret(GB *gb, int bank, uint16_t addr, uint16_t ret)
{
	// Same as the farcall macro: ld a, BANK / ldh [hCallFuncBank], a / hcall
	wr8(gb, hCallFuncBank, (uint8_t)bank);
	hcall_ret(gb, addr, ret);
}
