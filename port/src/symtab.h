// The disassembly's labels, built into the executable (generated
// sym_table.c, see tools/gen.c), and the ROM they belong to.
#ifndef WL3_SYMTAB_H
#define WL3_SYMTAB_H

#include <stdint.h>

typedef struct {
	const char *name;
	uint8_t bank;
	uint16_t addr;
} SymEntry;

extern const SymEntry wl3_syms[];
extern const int wl3_nsyms;
// SHA-1 of the ROM the labels were taken from, as 40 lowercase hex digits.
extern const char wl3_rom_sha1[];

#endif
