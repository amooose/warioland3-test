// Build-time code generator for the port.
//
//   gen syms <warioland3.sym> <out.h>
//     Turns the linker's symbol file into C constants, so ported code can
//     refer to labels from the disassembly by name:
//       RAM labels:  #define wROMBank 0xc5ff / #define BANK_wROMBank 0x00
//       ROM labels:  #define ADDR_Foo 0x4a12 / #define BANK_Foo 0x05
//
//   gen consts <state.asm> <out.h>
//     Turns an rgbasm state dump (rgbasm -s equ:state.asm) of the
//     disassembly's includes into C constants: every DEF ... EQU and const
//     from src/constants, with the exact value the assembler computed.
//
//   gen symtab <warioland3.sym> <warioland3.gbc> <out.c>
//     Builds the whole symbol table (local labels too) into the executable,
//     with the SHA-1 of the ROM it belongs to, so the port needs neither the
//     .sym file nor the disassembly at runtime.
//
//   gen ram <warioland3.sym> <out.h>
//     Lists the RAM labels (wFoo, hFoo) with C++ names for the C++ modules
//     (src/native/ram.hpp): WRAM(foo_bar, wFooBar) / HRAM(...), one per
//     label, wFooBar -> foo_bar.
//
//   gen ports <out.c> <ported/*.c...>
//     Finds every PORT(Name) / PORT_JUMP(Name) / PORT_EX(Name, ...) and writes the
//     table that registers them as hooks at startup.
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/sha1.h"

// The file name without its directories, so generated files don't depend
// on where the repository is checked out.
static const char *base_name(const char *path)
{
	const char *b = path;
	for (const char *p = path; *p; p++)
		if (*p == '/' || *p == '\\')
			b = p + 1;
	return b;
}

static int valid_c_ident(const char *s)
{
	if (!isalpha((unsigned char)*s) && *s != '_')
		return 0;
	for (; *s; s++)
		if (!isalnum((unsigned char)*s) && *s != '_')
			return 0;
	return 1;
}

static int gen_syms(const char *in, const char *out)
{
	FILE *f = fopen(in, "r");
	if (!f) {
		fprintf(stderr, "gen: can't read %s\n", in);
		return 1;
	}
	FILE *o = fopen(out, "w");
	if (!o) {
		fprintf(stderr, "gen: can't write %s\n", out);
		return 1;
	}
	fprintf(o, "// Generated from %s by port/tools/gen.c. Do not edit.\n", base_name(in));
	fprintf(o, "#ifndef WL3_SYMS_H\n#define WL3_SYMS_H\n\n");
	char line[512];
	while (fgets(line, sizeof(line), f)) {
		unsigned bank, addr;
		char name[400];
		if (line[0] == ';' || sscanf(line, "%x:%x %399s", &bank, &addr, name) != 3)
			continue;
		if (!valid_c_ident(name))
			continue; // local labels (Foo.bar) and anonymous labels
		if (addr >= 0x8000) {
			fprintf(o, "#define %s 0x%04x\n#define BANK_%s 0x%02x\n", name, addr, name, bank);
		} else {
			fprintf(o, "#define ADDR_%s 0x%04x\n#define BANK_%s 0x%02x\n", name, addr, name, bank);
		}
	}
	fprintf(o, "\n#endif\n");
	fclose(f);
	return fclose(o) != 0;
}

// wFooBar -> foo_bar (VBlank counts as one word; an unnamed label such as
// w3d50d -> unk_3d50d). Returns 0 if the result isn't usable.
static int snake_name(const char *label, char *out, size_t size)
{
	char buf[400];
	snprintf(buf, sizeof(buf), "%s", label + 1); // drop the w / h
	for (char *p = strstr(buf, "VBlank"); p; p = strstr(p + 1, "VBlank"))
		p[1] = 'b';
	int hex = strlen(buf) >= 4;
	for (const char *p = buf; *p; p++)
		hex &= isxdigit((unsigned char)*p) && !isupper((unsigned char)*p);
	size_t n = 0;
	if (hex || isdigit((unsigned char)buf[0]))
		n += snprintf(out, size, "unk_");
	for (size_t i = 0; buf[i] && n + 2 < size; i++) {
		char c = buf[i];
		if (isupper((unsigned char)c) && i > 0) {
			char prev = buf[i - 1], next = buf[i + 1];
			if (islower((unsigned char)prev) || isdigit((unsigned char)prev) ||
			    (isupper((unsigned char)prev) && islower((unsigned char)next)))
				out[n++] = '_';
		}
		out[n++] = (char)tolower((unsigned char)c);
	}
	out[n] = 0;
	static const char *const keywords[] = {"new", "delete", "default", "switch", "case",
		"class", "this", "register", "char", "int", "long", "short", "auto", "break",
		"continue", "return", "for", "while", "do", "if", "else", "goto", "union", "struct"};
	for (size_t i = 0; i < sizeof(keywords) / sizeof(*keywords); i++)
		if (!strcmp(out, keywords[i]) && n + 1 < size) {
			out[n++] = '_';
			out[n] = 0;
		}
	return valid_c_ident(out);
}

static int gen_ram(const char *in, const char *out)
{
	FILE *f = fopen(in, "r");
	if (!f) {
		fprintf(stderr, "gen: can't read %s\n", in);
		return 1;
	}
	FILE *o = fopen(out, "w");
	if (!o) {
		fprintf(stderr, "gen: can't write %s\n", out);
		return 1;
	}
	fprintf(o, "// Generated from %s by port/tools/gen.c. Do not edit.\n", base_name(in));
	fprintf(o, "// The RAM labels' C++ names, for src/native/ram.hpp.\n");
	static char seen[8192][64];
	int nseen = 0;
	char line[512];
	while (fgets(line, sizeof(line), f)) {
		unsigned bank, addr;
		char label[400], name[400];
		if (line[0] == ';' || sscanf(line, "%x:%x %399s", &bank, &addr, label) != 3)
			continue;
		if (addr < 0xc000 || (label[0] != 'w' && label[0] != 'h') || !valid_c_ident(label))
			continue;
		if (!snake_name(label, name, sizeof(name) - 8) || strlen(name) >= 56)
			continue;
		int dup = 0;
		for (int i = 0; i < nseen && !dup; i++)
			dup = !strcmp(seen[i], name);
		if (dup) // two labels with one name: the second gets its address
			snprintf(name + strlen(name), 8, "_%04x", addr);
		if (nseen < 8192)
			snprintf(seen[nseen++], sizeof(seen[0]), "%s", name);
		fprintf(o, "%s(%s, %s)\n", label[0] == 'w' ? "WRAM" : "HRAM", name, label);
	}
	fclose(f);
	return fclose(o) != 0;
}

// asm constants whose names already mean something in C.
static const char *const skip_consts[] = {"NULL"};

static int gen_consts(const char *in, const char *out)
{
	FILE *f = fopen(in, "r");
	if (!f) {
		fprintf(stderr, "gen: can't read %s\n", in);
		return 1;
	}
	FILE *o = fopen(out, "w");
	if (!o) {
		fprintf(stderr, "gen: can't write %s\n", out);
		return 1;
	}
	fprintf(o, "// Generated from the disassembly's constants by port/tools/gen.c. Do not edit.\n");
	fprintf(o, "#ifndef WL3_CONSTANTS_H\n#define WL3_CONSTANTS_H\n\n");
	char line[512];
	while (fgets(line, sizeof(line), f)) {
		char name[400];
		unsigned long value;
		if (sscanf(line, "def %399s equ $%lx", name, &value) != 2 || !valid_c_ident(name))
			continue;
		int skip = 0;
		for (size_t i = 0; i < sizeof(skip_consts) / sizeof(*skip_consts); i++)
			skip |= !strcmp(name, skip_consts[i]);
		if (!skip)
			fprintf(o, "#define %s 0x%lx\n", name, value);
	}
	fprintf(o, "\n#endif\n");
	fclose(f);
	return fclose(o) != 0;
}

static int gen_symtab(const char *in, const char *rom, const char *out)
{
	FILE *r = fopen(rom, "rb");
	if (!r) {
		fprintf(stderr, "gen: can't read %s\n", rom);
		return 1;
	}
	fseek(r, 0, SEEK_END);
	long size = ftell(r);
	fseek(r, 0, SEEK_SET);
	uint8_t *data = malloc(size > 0 ? (size_t)size : 1);
	if (size <= 0 || fread(data, 1, (size_t)size, r) != (size_t)size) {
		fprintf(stderr, "gen: can't read %s\n", rom);
		return 1;
	}
	fclose(r);
	char sha1[41];
	sha1_hex(data, (size_t)size, sha1);
	free(data);

	FILE *f = fopen(in, "r");
	if (!f) {
		fprintf(stderr, "gen: can't read %s\n", in);
		return 1;
	}
	FILE *o = fopen(out, "w");
	if (!o) {
		fprintf(stderr, "gen: can't write %s\n", out);
		return 1;
	}
	fprintf(o, "// Generated from %s and %s by port/tools/gen.c. Do not edit.\n", base_name(in),
		base_name(rom));
	fprintf(o, "#include \"symtab.h\"\n\n");
	fprintf(o, "const char wl3_rom_sha1[] = \"%s\";\n\n", sha1);
	fprintf(o, "const SymEntry wl3_syms[] = {\n");
	int n = 0;
	char line[512];
	while (fgets(line, sizeof(line), f)) {
		unsigned bank, addr;
		char name[400];
		if (line[0] == ';' || sscanf(line, "%x:%x %399s", &bank, &addr, name) != 3)
			continue;
		fprintf(o, "\t{\"");
		for (const char *p = name; *p; p++) {
			if (*p == '"' || *p == '\\')
				fputc('\\', o);
			fputc(*p, o);
		}
		fprintf(o, "\", 0x%02x, 0x%04x},\n", bank, addr);
		n++;
	}
	fprintf(o, "};\n\nconst int wl3_nsyms = %d;\n", n);
	fclose(f);
	return fclose(o) != 0;
}

static int gen_ports(const char *out, int nfiles, char **files)
{
	char **names = NULL;
	int n = 0, cap = 0;
	for (int i = 0; i < nfiles; i++) {
		FILE *f = fopen(files[i], "r");
		if (!f) {
			fprintf(stderr, "gen: can't read %s\n", files[i]);
			return 1;
		}
		char line[512];
		while (fgets(line, sizeof(line), f)) {
			// Any PORT macro at the start of a line: PORT(Name), PORT_JUMP(Name),
			// PORT_EX(Name, ...)
			if (strncmp(line, "PORT", 4))
				continue;
			// PORT_RAM(Name): native as a RAM hook (ram_code.c), no ROM hook
			if (!strncmp(line, "PORT_RAM", 8))
				continue;
			// PORT_COVERED(Name): implemented inside a C++ module, no hook
			if (!strncmp(line, "PORT_COVERED", 12))
				continue;
			const char *p = line + 4;
			while (*p == '_' || (*p >= 'A' && *p <= 'Z'))
				p++;
			int local = !strncmp(line, "PORT_LOCAL", 10);
			if (*p++ != '(')
				continue;
			size_t len = strcspn(p, ",) \t");
			if (n == cap) {
				cap = cap ? cap * 2 : 256;
				names = realloc(names, (size_t)cap * sizeof(char *));
			}
			// PORT_LOCAL(Parent, local) defines portinfo_Parent__local
			const char *q = p + len;
			size_t len2 = 0;
			if (local) {
				q += strspn(q, ", \t");
				len2 = strcspn(q, ",) \t");
			}
			names[n] = malloc(len + 2 + len2 + 1);
			memcpy(names[n], p, len);
			names[n][len] = 0;
			if (local) {
				strcat(names[n], "__");
				strncat(names[n], q, len2);
			}
			n++;
		}
		fclose(f);
	}

	FILE *o = fopen(out, "w");
	if (!o) {
		fprintf(stderr, "gen: can't write %s\n", out);
		return 1;
	}
	fprintf(o, "// Generated by port/tools/gen.c. Do not edit.\n");
	fprintf(o, "#include \"ported.h\"\n\n");
	for (int i = 0; i < n; i++)
		fprintf(o, "extern const PortInfo portinfo_%s;\n", names[i]);
	fprintf(o, "\nvoid ported_register_all(void)\n{\n");
	for (int i = 0; i < n; i++)
		fprintf(o, "\tport_register(&portinfo_%s);\n", names[i]);
	fprintf(o, "}\n");
	return fclose(o) != 0;
}

int main(int argc, char **argv)
{
	if (argc == 4 && !strcmp(argv[1], "syms"))
		return gen_syms(argv[2], argv[3]);
	if (argc == 5 && !strcmp(argv[1], "symtab"))
		return gen_symtab(argv[2], argv[3], argv[4]);
	if (argc == 4 && !strcmp(argv[1], "ram"))
		return gen_ram(argv[2], argv[3]);
	if (argc == 4 && !strcmp(argv[1], "consts"))
		return gen_consts(argv[2], argv[3]);
	if (argc >= 3 && !strcmp(argv[1], "ports"))
		return gen_ports(argv[2], argc - 3, argv + 3);
	fprintf(stderr, "usage: gen syms <in.sym> <out.h> | gen symtab <in.sym> <rom> <out.c> |\n"
		"       gen ram <in.sym> <out.h> | gen consts <state.asm> <out.h> | gen ports <out.c> <files...>\n");
	return 1;
}
