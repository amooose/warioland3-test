// Porting progress report.
//
//   progress <repo root> <asm file list> <out.md> [--coverage file]... <ported/*.c...>
//
// Reads the disassembly's asm sources and builds the routine call graph
// (call, jp, farcall and friends, jump table entries, and fallthrough into
// the next label), then reports how much has been ported and which routines
// are ready to port next: those whose callees are all native already.
// Coverage files (from wl3 --coverage) mark which routines the test inputs
// exercise, so the difftest can check them once ported.
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char *name;
	char *file; // relative to the repo root
	int line;
	int instructions;
	int *callees;
	int ncallees, capcallees;
	int callers;
	int ported;
	unsigned long long covered; // times entered during the test inputs
} Routine;

static Routine *routines;
static int nroutines, caproutines;

// Simple open-addressing name -> routine index table.
#define HASH_SIZE (1 << 16)
static int hash_table[HASH_SIZE];

static unsigned hash(const char *s)
{
	unsigned h = 2166136261u;
	for (; *s; s++)
		h = (h ^ (unsigned char)*s) * 16777619u;
	return h;
}

static int find(const char *name)
{
	for (unsigned h = hash(name) & (HASH_SIZE - 1);; h = (h + 1) & (HASH_SIZE - 1)) {
		int i = hash_table[h] - 1;
		if (i < 0)
			return -1;
		if (!strcmp(routines[i].name, name))
			return i;
	}
}

static int add(const char *name, const char *file, int line)
{
	int i = find(name);
	if (i >= 0)
		return i;
	if (nroutines == caproutines) {
		caproutines = caproutines ? caproutines * 2 : 4096;
		routines = realloc(routines, (size_t)caproutines * sizeof(Routine));
	}
	i = nroutines++;
	memset(&routines[i], 0, sizeof(Routine));
	routines[i].name = strdup(name);
	routines[i].file = strdup(file);
	routines[i].line = line;
	unsigned h = hash(name) & (HASH_SIZE - 1);
	while (hash_table[h])
		h = (h + 1) & (HASH_SIZE - 1);
	hash_table[h] = i + 1;
	return i;
}

// Edges are recorded by name and resolved after every file is read.
typedef struct {
	int from;
	char *to;
} Edge;
static Edge *edges;
static int nedges, capedges;

static void add_edge(int from, const char *to)
{
	if (from < 0)
		return;
	if (nedges == capedges) {
		capedges = capedges ? capedges * 2 : 8192;
		edges = realloc(edges, (size_t)capedges * sizeof(Edge));
	}
	edges[nedges].from = from;
	edges[nedges].to = strdup(to);
	nedges++;
}

static const char *const instructions[] = {
	"adc", "add", "and", "bit", "call", "ccf", "cp", "cpl", "daa", "dec", "di", "ei", "halt",
	"inc", "jp", "jr", "ld", "ldh", "ldi", "ldd", "nop", "or", "pop", "push", "res", "ret",
	"reti", "rl", "rla", "rlc", "rlca", "rr", "rra", "rrc", "rrca", "rst", "sbc", "scf", "set",
	"sla", "sra", "srl", "stop", "sub", "swap", "xor",
	// macros that expand to code (src/macros/code.asm, pos.asm, audio.asm)
	"lb", "bankswitch", "sramswitch", "push_wram", "pop_wram", "jumptable", "hcall",
	"farcall", "homecall", "homejp", "debug_nop", "debug_assert_not", "wait_not_hblank",
	"wait_hblank", "ld_obj_frameset", "update_pos_y", "get_pos_y", "update_pos_x",
	"get_pos_x", "update_pos", "get_pos", "play_sfx", "play_sfx_rept",
};

static int is_instruction(const char *w)
{
	for (size_t i = 0; i < sizeof(instructions) / sizeof(*instructions); i++)
		if (!strcmp(w, instructions[i]))
			return 1;
	return 0;
}

static int is_ident_char(int c)
{
	return isalnum(c) || c == '_' || c == '.' || c == '@' || c == '#';
}

// Last operand of an instruction ("call nz, Foo" -> "Foo"), if it's a
// global label name.
static int last_operand(const char *args, char *out, size_t outlen)
{
	const char *comma = strrchr(args, ',');
	const char *p = comma ? comma + 1 : args;
	while (*p == ' ' || *p == '\t')
		p++;
	size_t n = 0;
	while (is_ident_char((unsigned char)p[n]) && n + 1 < outlen)
		n++;
	if (!n || !(isalpha((unsigned char)p[0]) || p[0] == '_') || memchr(p, '.', n))
		return 0;
	memcpy(out, p, n);
	out[n] = 0;
	return 1;
}

static int is_condition(const char *s)
{
	while (*s == ' ' || *s == '\t')
		s++;
	return (!strncmp(s, "nz", 2) || !strncmp(s, "nc", 2) || s[0] == 'z' || s[0] == 'c') &&
		strchr(s, ',');
}

static void read_asm(const char *path, const char *rel)
{
	FILE *f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, "progress: can't read %s\n", path);
		return;
	}
	char line[1024];
	int lineno = 0;
	int current = -1;
	int falls_through = 0; // last code line of `current` doesn't end control flow
	int after_jumptable = 0;
	while (fgets(line, sizeof(line), f)) {
		lineno++;
		char *semi = strchr(line, ';');
		if (semi)
			*semi = 0;

		// Global label at column 0: Foo: / Foo:: (not .local or Foo.local)
		if (isalpha((unsigned char)line[0]) || line[0] == '_') {
			size_t n = 0;
			while (is_ident_char((unsigned char)line[n]))
				n++;
			if (line[n] == ':' && !memchr(line, '.', n)) {
				line[n] = 0;
				int next = add(line, rel, lineno);
				if (current >= 0 && falls_through && current != next)
					add_edge(current, line);
				current = next;
				falls_through = 0;
				after_jumptable = 0;
				memmove(line, line + n + 1 + (line[n + 1] == ':'), strlen(line + n + 1) + 1);
			} else if (!strncmp(line, "SECTION", 7)) {
				current = -1;
				continue;
			} else {
				continue;
			}
		}

		char *p = line;
		while (*p == ' ' || *p == '\t')
			p++;
		char word[64];
		size_t n = 0;
		while ((isalnum((unsigned char)p[n]) || p[n] == '_') && n + 1 < sizeof(word)) {
			word[n] = (char)tolower((unsigned char)p[n]);
			n++;
		}
		word[n] = 0;
		if (!n || current < 0)
			continue;
		const char *args = p + n;
		char target[256];

		if (is_instruction(word)) {
			routines[current].instructions++;
			falls_through = 1;
			after_jumptable = !strcmp(word, "jumptable");
			if (!strcmp(word, "call") || !strcmp(word, "farcall") || !strcmp(word, "homecall") ||
				!strcmp(word, "hcall") || !strcmp(word, "jp") || !strcmp(word, "homejp") ||
				!strcmp(word, "jr")) {
				if (last_operand(args, target, sizeof(target)) && strcmp(target, "hl"))
					add_edge(current, target);
			}
			int conditional = is_condition(args);
			if ((!strcmp(word, "jp") || !strcmp(word, "jr") || !strcmp(word, "homejp")) && !conditional)
				falls_through = 0;
			if ((!strcmp(word, "ret") && !conditional) || !strcmp(word, "reti"))
				falls_through = 0;
			if (after_jumptable)
				falls_through = 0;
		} else if (!strcmp(word, "dw") && after_jumptable) {
			// jump table entries are dispatch targets of this routine
			char *list = strdup(args);
			for (char *t = strtok(list, ", \t\r\n"); t; t = strtok(NULL, ", \t\r\n"))
				if ((isalpha((unsigned char)t[0]) || t[0] == '_') && !strchr(t, '.'))
					add_edge(current, t);
			free(list);
		} else if (strcmp(word, "db") && strcmp(word, "dw") && strcmp(word, "ds")) {
			// directives, data macros: not code
			if (!strcmp(word, "incbin"))
				falls_through = 0;
		}
	}
	fclose(f);
}

static void read_ported(const char *path)
{
	FILE *f = fopen(path, "r");
	if (!f)
		return;
	char line[512];
	while (fgets(line, sizeof(line), f)) {
		// Any PORT macro at the start of a line: PORT(Name), PORT_JUMP(Name),
		// PORT_EX(Name, ...)
		if (strncmp(line, "PORT", 4))
			continue;
		// PORT_LOCAL(Parent, local) ports part of Parent, which counts as
		// ported once PORT(Parent) exists.
		if (!strncmp(line, "PORT_LOCAL", 10))
			continue;
		const char *p = line + 4;
		while (*p == '_' || (*p >= 'A' && *p <= 'Z'))
			p++;
		if (*p++ != '(')
			continue;
		char name[256];
		size_t n = strcspn(p, ",) \t");
		if (n >= sizeof(name))
			continue;
		memcpy(name, p, n);
		name[n] = 0;
		int i = find(name);
		if (i >= 0)
			routines[i].ported = 1;
		else
			fprintf(stderr, "progress: ported routine %s not found in the asm\n", name);
	}
	fclose(f);
}

static void read_coverage(const char *path)
{
	FILE *f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, "progress: can't read %s\n", path);
		return;
	}
	char name[256];
	unsigned long long count;
	while (fscanf(f, "%255s %llu", name, &count) == 2) {
		int i = find(name);
		if (i >= 0)
			routines[i].covered += count;
	}
	fclose(f);
}

static int is_code(const Routine *r)
{
	return r->instructions > 0;
}

// Directory of a routine's file, as a grouping key ("src/engine/level").
static void group_of(const Routine *r, char *out, size_t len)
{
	snprintf(out, len, "%s", r->file);
	char *slash = strrchr(out, '/');
	if (slash)
		*slash = 0;
}

static int ready(const Routine *r)
{
	if (r->ported || !is_code(r))
		return 0;
	for (int k = 0; k < r->ncallees; k++) {
		const Routine *c = &routines[r->callees[k]];
		if (c != r && is_code(c) && !c->ported)
			return 0;
	}
	return 1;
}

static int by_ready_priority(const void *a, const void *b)
{
	const Routine *x = &routines[*(const int *)a], *y = &routines[*(const int *)b];
	// tested first, then most-called, then smallest
	if (!x->covered != !y->covered)
		return x->covered ? -1 : 1;
	if (x->callers != y->callers)
		return y->callers - x->callers;
	return x->instructions - y->instructions;
}

int main(int argc, char **argv)
{
	if (argc < 4) {
		fprintf(stderr, "usage: progress <repo root> <asm file list> <out.md> <ported files...>\n");
		return 1;
	}
	const char *root = argv[1];
	FILE *list = fopen(argv[2], "r");
	if (!list) {
		fprintf(stderr, "progress: can't read %s\n", argv[2]);
		return 1;
	}
	char path[4096];
	size_t rootlen = strlen(root);
	while (fgets(path, sizeof(path), list)) {
		path[strcspn(path, "\r\n")] = 0;
		if (!*path)
			continue;
		const char *rel = path;
		if (!strncmp(path, root, rootlen)) {
			rel = path + rootlen;
			while (*rel == '/')
				rel++;
		}
		read_asm(path, rel);
	}
	fclose(list);
	for (int i = 4; i < argc; i++) {
		if (!strcmp(argv[i], "--coverage") && i + 1 < argc)
			read_coverage(argv[++i]);
		else
			read_ported(argv[i]);
	}

	for (int e = 0; e < nedges; e++) {
		int to = find(edges[e].to);
		if (to < 0)
			continue; // RAM labels, constants, or local labels
		Routine *r = &routines[edges[e].from];
		int dup = 0;
		for (int k = 0; k < r->ncallees; k++)
			dup |= r->callees[k] == to;
		if (dup)
			continue;
		if (r->ncallees == r->capcallees) {
			r->capcallees = r->capcallees ? r->capcallees * 2 : 8;
			r->callees = realloc(r->callees, (size_t)r->capcallees * sizeof(int));
		}
		r->callees[r->ncallees++] = to;
		routines[to].callers++;
	}

	// Totals, overall and per directory.
	typedef struct {
		char name[256];
		int routines, ported, instructions, ported_instructions;
	} Group;
	Group groups[128];
	int ngroups = 0;
	Group total = {"total", 0, 0, 0, 0};
	for (int i = 0; i < nroutines; i++) {
		Routine *r = &routines[i];
		if (!is_code(r))
			continue;
		char g[256];
		group_of(r, g, sizeof(g));
		int k;
		for (k = 0; k < ngroups && strcmp(groups[k].name, g); k++)
			;
		if (k == ngroups && ngroups < 128) {
			memset(&groups[k], 0, sizeof(Group));
			snprintf(groups[k].name, sizeof(groups[k].name), "%s", g);
			ngroups++;
		}
		Group *gr[2] = {k < 128 ? &groups[k] : NULL, &total};
		for (int j = 0; j < 2; j++) {
			if (!gr[j])
				continue;
			gr[j]->routines++;
			gr[j]->instructions += r->instructions;
			if (r->ported) {
				gr[j]->ported++;
				gr[j]->ported_instructions += r->instructions;
			}
		}
	}

	FILE *out = fopen(argv[3], "w");
	if (!out) {
		fprintf(stderr, "progress: can't write %s\n", argv[3]);
		return 1;
	}
	FILE *streams[2] = {stdout, out};
	for (int s = 0; s < 2; s++) {
		FILE *o = streams[s];
		fprintf(o, "# Porting progress\n\n");
		fprintf(o, "**%d / %d routines ported (%.1f%%), %d / %d instructions (%.1f%%)**\n\n",
			total.ported, total.routines, 100.0 * total.ported / (total.routines ? total.routines : 1),
			total.ported_instructions, total.instructions,
			100.0 * total.ported_instructions / (total.instructions ? total.instructions : 1));
		fprintf(o, "| Directory | Routines | Ported | Instructions | Ported |\n");
		fprintf(o, "| --- | ---: | ---: | ---: | ---: |\n");
		for (int k = 0; k < ngroups; k++) {
			Group *g = &groups[k];
			fprintf(o, "| %s | %d | %d | %d | %.1f%% |\n", g->name, g->routines, g->ported,
				g->instructions, 100.0 * g->ported_instructions / (g->instructions ? g->instructions : 1));
		}
	}

	int ported_total = 0, ported_tested = 0;
	for (int i = 0; i < nroutines; i++) {
		if (routines[i].ported) {
			ported_total++;
			ported_tested += routines[i].covered > 0;
		}
	}
	for (int s = 0; s < 2; s++)
		fprintf(streams[s], "\n%d of %d ported routines are exercised by the test inputs.\n",
			ported_tested, ported_total);

	// Ready to port: every callee is already native.
	int *ready_list = malloc((size_t)nroutines * sizeof(int));
	int nready = 0;
	for (int i = 0; i < nroutines; i++)
		if (ready(&routines[i]))
			ready_list[nready++] = i;
	qsort(ready_list, (size_t)nready, sizeof(int), by_ready_priority);

	int nready_tested = 0;
	for (int k = 0; k < nready; k++)
		nready_tested += routines[ready_list[k]].covered > 0;
	fprintf(stdout, "\n%d routines are ready to port (all their callees are native), %d of them "
		"exercised by the test inputs; the full list is in %s\n", nready, nready_tested, argv[3]);
	fprintf(out, "\n## Ready to port\n\n");
	fprintf(out, "Routines whose callees are all native already: the ones the test inputs\n"
		"exercise first (the difftest can check those), then most-called first.\n\n");
	fprintf(out, "| Routine | Tested calls | Callers | Instructions | Location |\n"
		"| --- | ---: | ---: | ---: | --- |\n");
	for (int k = 0; k < nready; k++) {
		Routine *r = &routines[ready_list[k]];
		fprintf(out, "| %s | %llu | %d | %d | %s:%d |\n", r->name, r->covered, r->callers,
			r->instructions, r->file, r->line);
		if (k < 15)
			fprintf(stdout, "  %-36s %8llu tested %4d callers %5d instr  %s:%d\n", r->name,
				r->covered, r->callers, r->instructions, r->file, r->line);
	}

	// Everything else still in asm, with the unported callees that hold
	// it back.
	fprintf(out, "\n## Not ported\n\n");
	fprintf(out, "Every routine with code that has no native version yet, and which of its\n"
		"callees are still asm (a dash: none, so it's in \"Ready to port\").\n\n");
	fprintf(out, "| Routine | Instructions | Waiting on | Location |\n| --- | ---: | --- | --- |\n");
	for (int i = 0; i < nroutines; i++) {
		Routine *r = &routines[i];
		if (r->ported || !is_code(r))
			continue;
		fprintf(out, "| %s | %d | ", r->name, r->instructions);
		int n = 0;
		for (int k = 0; k < r->ncallees; k++) {
			const Routine *c = &routines[r->callees[k]];
			if (c != r && is_code(c) && !c->ported)
				fprintf(out, "%s%s", n++ ? ", " : "", c->name);
		}
		fprintf(out, "%s | %s:%d |\n", n ? "" : "-", r->file, r->line);
	}
	return fclose(out) != 0;
}
