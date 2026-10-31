// Symbol table and hook registry.
#include "hooks.h"
#include "symtab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef SymEntry Sym;

// The labels built into the executable (sym_table.c)
static const Sym *const syms = wl3_syms;
#define nsyms wl3_nsyms

#define MAX_HOOKS 8192
static Hook hooks[MAX_HOOKS];
static int nhooks;

// One bit per ROM byte: is there a hook at this offset?
static uint8_t *hook_bitmap;
static size_t hook_bitmap_bytes;
// The first hook at each ROM offset (-1: none), so the interpreter finds
// one without scanning them all: nearly every routine it enters has one.
static int32_t *hook_index;
static bool all_enabled = true;

static size_t rom_offset(int bank, uint16_t addr)
{
	if (addr < 0x4000)
		return addr;
	return (size_t)bank * 0x4000 + (addr - 0x4000);
}

bool sym_lookup(const char *name, int *bank, uint16_t *addr)
{
	for (int i = 0; i < nsyms; i++) {
		if (!strcmp(syms[i].name, name)) {
			*bank = syms[i].bank;
			*addr = syms[i].addr;
			return true;
		}
	}
	return false;
}

int sym_count(void)
{
	return nsyms;
}

bool sym_get(int i, const char **name, int *bank, uint16_t *addr)
{
	if (i < 0 || i >= nsyms)
		return false;
	*name = syms[i].name;
	*bank = syms[i].bank;
	*addr = syms[i].addr;
	return true;
}

const char *sym_name_at(int bank, uint16_t addr)
{
	const Sym *best = NULL;
	for (int i = 0; i < nsyms; i++) {
		const Sym *s = &syms[i];
		bool same_region = addr < 0x4000 ? (s->addr < 0x4000) :
			(addr < 0x8000 ? (s->bank == bank && s->addr >= 0x4000 && s->addr < 0x8000) : (s->addr >= 0x8000));
		if (!same_region || s->addr > addr)
			continue;
		if (!best || s->addr > best->addr)
			best = s;
	}
	return best ? best->name : NULL;
}

static uint8_t *label_bitmap; // one bit per ROM byte: a label starts there

bool sym_is_rom_label(int bank, uint16_t addr)
{
	if (!label_bitmap || addr >= 0x8000)
		return false;
	size_t off = rom_offset(addr < 0x4000 ? 0 : bank, addr);
	return off / 8 < hook_bitmap_bytes && (label_bitmap[off / 8] & (1 << (off % 8)));
}

void hooks_init(GB *gb)
{
	hook_bitmap_bytes = gb->rom_size / 8 + 1;
	hook_bitmap = calloc(hook_bitmap_bytes, 1);
	label_bitmap = calloc(hook_bitmap_bytes, 1);
	for (int i = 0; i < nsyms; i++) {
		if (syms[i].addr >= 0x8000)
			continue;
		size_t off = rom_offset(syms[i].addr < 0x4000 ? 0 : syms[i].bank, syms[i].addr);
		if (off / 8 < hook_bitmap_bytes)
			label_bitmap[off / 8] |= (uint8_t)(1 << (off % 8));
	}
	hook_index = malloc(gb->rom_size * sizeof(int32_t));
	for (size_t i = 0; i < gb->rom_size; i++)
		hook_index[i] = -1;
	for (int i = 0; i < nhooks; i++) {
		size_t off = rom_offset(hooks[i].bank, hooks[i].addr);
		hook_bitmap[off / 8] |= (uint8_t)(1 << (off % 8));
		if (off < gb->rom_size && hook_index[off] < 0)
			hook_index[off] = i;
	}
}

bool hook_register(const char *name, HookFn fn, unsigned flags, unsigned clobbers)
{
	int bank;
	uint16_t addr;
	if (!sym_lookup(name, &bank, &addr)) {
		fprintf(stderr, "hook: unknown symbol %s\n", name);
		return false;
	}
	if (addr >= 0x8000) {
		fprintf(stderr, "hook: %s is not in ROM\n", name);
		return false;
	}
	for (int i = 0; i < nhooks; i++) {
		if (hooks[i].addr == addr && hooks[i].bank == (addr < 0x4000 ? 0 : bank)) {
			fprintf(stderr, "hook: %s is ported twice (also as %s)\n", name, hooks[i].name);
			return false;
		}
	}
	if (nhooks == MAX_HOOKS) {
		fprintf(stderr, "hook: too many hooks\n");
		return false;
	}
	Hook *h = &hooks[nhooks++];
	h->name = name;
	h->bank = addr < 0x4000 ? 0 : bank;
	h->addr = addr;
	h->fn = fn;
	h->flags = flags;
	h->clobbers = clobbers;
	h->enabled = true;
	if (hook_bitmap) {
		size_t off = rom_offset(h->bank, addr);
		hook_bitmap[off / 8] |= (uint8_t)(1 << (off % 8));
	}
	return true;
}

Hook *hook_find(GB *gb, uint16_t pc)
{
	if (!all_enabled)
		return NULL;
	return hook_find_any(gb, pc);
}

Hook *hook_find_any(GB *gb, uint16_t pc)
{
	if (pc >= 0x8000 || !hook_bitmap)
		return NULL;
	int bank = pc < 0x4000 ? 0 : gb->rom_bank % gb->rom_banks;
	size_t off = rom_offset(bank, pc);
	if (!(hook_bitmap[off / 8] & (1 << (off % 8))))
		return NULL;
	if (hook_index && off < gb->rom_size && hook_index[off] >= 0 && hooks[hook_index[off]].enabled)
		return &hooks[hook_index[off]];
	// a disabled first hook: look for another at the same address
	for (int i = 0; i < nhooks; i++) {
		Hook *h = &hooks[i];
		if (h->addr == pc && h->bank == bank && h->enabled)
			return h;
	}
	return NULL;
}

Hook *hook_at(int bank, uint16_t addr)
{
	if (!all_enabled)
		return NULL;
	int b = addr < 0x4000 ? 0 : bank;
	for (int i = 0; i < nhooks; i++)
		if (hooks[i].addr == addr && hooks[i].bank == b && hooks[i].enabled)
			return &hooks[i];
	return NULL;
}

int hook_count(void)
{
	return nhooks;
}

Hook *hook_get(int i)
{
	return &hooks[i];
}

void hooks_set_enabled(bool enabled)
{
	all_enabled = enabled;
}

static RamHookFn ram_hooks[0x8000]; // by address - $8000

void ram_hook_register(uint16_t addr, RamHookFn fn)
{
	if (addr >= 0x8000)
		ram_hooks[addr - 0x8000] = fn;
}

RamHookFn ram_hook_find(uint16_t pc)
{
	return all_enabled && pc >= 0x8000 ? ram_hooks[pc - 0x8000] : NULL;
}
