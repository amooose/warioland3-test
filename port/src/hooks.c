// Symbol table and hook registry.
#include "hooks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char *name;
	int bank;
	uint16_t addr;
} Sym;

static Sym *syms;
static int nsyms;

#define MAX_HOOKS 8192
static Hook hooks[MAX_HOOKS];
static int nhooks;

// One bit per ROM byte: is there a hook at this offset?
static uint8_t *hook_bitmap;
static size_t hook_bitmap_bytes;
static bool all_enabled = true;

static size_t rom_offset(int bank, uint16_t addr)
{
	if (addr < 0x4000)
		return addr;
	return (size_t)bank * 0x4000 + (addr - 0x4000);
}

bool sym_load(const char *path)
{
	FILE *f = fopen(path, "r");
	if (!f)
		return false;
	char line[512];
	int cap = 0;
	while (fgets(line, sizeof(line), f)) {
		unsigned bank, addr;
		char name[400];
		if (line[0] == ';' || sscanf(line, "%x:%x %399s", &bank, &addr, name) != 3)
			continue;
		if (nsyms == cap) {
			cap = cap ? cap * 2 : 4096;
			syms = realloc(syms, (size_t)cap * sizeof(Sym));
		}
		syms[nsyms].name = strdup(name);
		syms[nsyms].bank = (int)bank;
		syms[nsyms].addr = (uint16_t)addr;
		nsyms++;
	}
	fclose(f);
	return nsyms > 0;
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

void hooks_init(GB *gb)
{
	hook_bitmap_bytes = gb->rom_size / 8 + 1;
	hook_bitmap = calloc(hook_bitmap_bytes, 1);
	for (int i = 0; i < nhooks; i++) {
		size_t off = rom_offset(hooks[i].bank, hooks[i].addr);
		hook_bitmap[off / 8] |= (uint8_t)(1 << (off % 8));
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
	for (int i = 0; i < nhooks; i++) {
		Hook *h = &hooks[i];
		if (h->addr == pc && h->bank == bank && h->enabled)
			return h;
	}
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
