// Native code for what the game runs from RAM: the HRAM call trampoline
// and OAM DMA routine (copied from ROM at start), and the V-Blank functions
// in WRAM (copied jumps, and the tile queue copy loops that
// SetTileQueueProcessingFunctions writes). Each first checks that the bytes
// there are the code it stands for, and otherwise leaves them to the
// interpreter. See ram_hook_register in hooks.h.
#include "ported.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Does RAM at addr hold `code` (len bytes), ignoring the bytes where `wild`
// is set?
static bool ram_is(GB *gb, uint16_t addr, const uint8_t *code, const uint8_t *wild, int len)
{
	for (int i = 0; i < len; i++) {
		uint8_t *p = gb_ram_byte(gb, (uint16_t)(addr + i));
		if (!p || (!(wild && wild[i]) && *p != code[i]))
			return false;
	}
	return true;
}

static uint8_t ram_byte(GB *gb, uint16_t addr)
{
	return *gb_ram_byte(gb, addr);
}

// hCallFunc (InitHRAMCallFunc.Func): ld a, [wROMBank] / push af /
// ld a, BANK / bankswitch / call POINTER / pop af / bankswitch / ret
static const uint8_t hcall_code[] = {
	0xfa, 0xff, 0xc5, 0xf5, 0x3e, 0x00, 0xea, 0xff, 0xc5, 0xea, 0x00, 0x21,
	0xcd, 0x00, 0x40, 0xf1, 0xea, 0xff, 0xc5, 0xea, 0x00, 0x21, 0xc9,
};
static const uint8_t hcall_wild[sizeof(hcall_code)] = {[5] = 1, [13] = 1, [14] = 1};

static bool hcall_func(GB *gb)
{
	if (!ram_is(gb, hCallFunc, hcall_code, hcall_wild, sizeof(hcall_code)))
		return false;
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = ram_byte(gb, hCallFuncBank);
	bankswitch(gb, gb->cpu.a);
	uint16_t func = (uint16_t)(ram_byte(gb, hCallFuncPointer) | ram_byte(gb, hCallFuncPointer + 1) << 8);
	// Every farcall and object update the game makes calls a label: one
	// into the middle of a routine comes from a fuzzed pointer (an
	// object's update function), as for jp hl in the interpreter.
	if (gb->test.active && func < 0x8000 && !sym_is_rom_label(gb->rom_bank, func))
		gb->test.unrealistic = true;
	if (!gb_call_native(gb, func))
		gb_call_ret(gb, func, (uint16_t)(hCallFunc + 0x0f));
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
	RET();
	return true;
}

// hTransferVirtualOAM (PushOAM): ldh [rDMA], a / ld a, 40 / .loop dec a /
// jr nz, .loop / ret, the DMA running while it waits
static const uint8_t push_oam_code[] = {0xe0, 0x46, 0x3e, 0x28, 0x3d, 0x20, 0xfd, 0xc9};

// The DMA and the time the asm's ld a, n and loop take, less the 4 cycles
// cpu_step charges a RAM hook it dispatches
static bool oam_dma(GB *gb)
{
	if (!ram_is(gb, hTransferVirtualOAM, push_oam_code, NULL, sizeof(push_oam_code)))
		return false;
	wr8(gb, rDMA, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = (uint8_t)(FLAG_Z | FLAG_N | (gb->cpu.f & FLAG_C)); // the last dec a
	idle(gb, 8 + 40 * 16 - 4);
	return true;
}

bool ram_transfer_virtual_oam(GB *gb)
{
	if (!oam_dma(gb))
		return false;
	idle(gb, 4); // no dispatch charges them here
	return true;
}

static bool transfer_virtual_oam(GB *gb)
{
	if (!oam_dma(gb))
		return false;
	RET();
	return true;
}

// The V-Blank functions in WRAM: a jp to the real one, a ret, or (from
// wVBlankFunc + $10 and wVBlankFuncExtended) SetTileQueueProcessingFunctions'
// copy loop, ld a, [hli] / ld d, a / ld a, [hli] / ld e, a / ld a, [bc] /
// ld [de], a / inc c once per queued tile, then a jp.
static const uint8_t tile_copy_code[] = {0x2a, 0x57, 0x2a, 0x5f, 0x0a, 0x12, 0x0c};

// The V-Blank functions the screens copy into wVBlankFunc: each is an
// X.Func ... X.end (or X.func ... X.func_end) block in ROM (found in the
// .sym file), and runs from RAM
// byte for byte as it would from ROM. One whose ROM block has a native
// routine (PORT_LOCAL(X, Func)) runs that instead.
typedef struct {
	int bank;
	uint16_t addr;
	size_t off; // into the ROM
	int len;
} CopiedFunc;

static CopiedFunc *copied;
static int ncopied = -1;

static void find_copied_funcs(void)
{
	ncopied = 0;
	copied = malloc((size_t)(sym_count() ? sym_count() : 1) * sizeof(CopiedFunc));
	for (int i = 0; i < sym_count(); i++) {
		const char *name;
		int bank, end_bank;
		uint16_t addr, end;
		if (!sym_get(i, &name, &bank, &addr))
			continue;
		size_t len = strlen(name);
		if (len < 6 || (strcmp(name + len - 5, ".Func") && strcmp(name + len - 5, ".func")) || addr >= 0x8000)
			continue;
		// X.Func ... X.end, or X.func ... X.func_end
		char end_name[128];
		snprintf(end_name, sizeof(end_name), "%.*s.end", (int)(len - 5), name);
		if (!sym_lookup(end_name, &end_bank, &end)) {
			snprintf(end_name, sizeof(end_name), "%s_end", name);
			if (!sym_lookup(end_name, &end_bank, &end))
				continue;
		}
		if (end_bank != bank || end <= addr || end - addr > 0x200)
			continue;
		size_t off = addr < 0x4000 ? addr : (size_t)bank * 0x4000 + (addr - 0x4000);
		copied[ncopied++] = (CopiedFunc){bank, addr, off, end - addr};
	}
}

// the native routine of the ROM block copied to pc, or NULL
static Hook *copied_hook(GB *gb, uint16_t pc)
{
	if (ncopied < 0)
		find_copied_funcs();
	for (int i = 0; i < ncopied; i++) {
		const CopiedFunc *c = &copied[i];
		if (c->off + (size_t)c->len > gb->rom_size || !ram_is(gb, pc, gb->rom + c->off, NULL, c->len))
			continue;
		Hook *h = hook_at(c->bank, c->addr);
		if (h)
			return h;
	}
	return NULL;
}

static bool copied_func(GB *gb)
{
	Hook *h = copied_hook(gb, gb->cpu.pc);
	if (!h)
		return false;
	h->calls++;
	if (gb->test.active)
		gb->test.nested_clobbers |= h->clobbers;
	h->fn(gb);
	if (!(h->flags & HOOK_MANUAL_RET))
		RET();
	return true;
}

// The V-Blank function copied to wVBlankFunc, when its ROM block has a
// native routine that returns, run as cpu_step would dispatch it
static bool run_vblank_func(GB *gb)
{
	Hook *h = copied_hook(gb, wVBlankFunc);
	if (!h || (h->flags & HOOK_MANUAL_RET))
		return false;
	// As cpu_step would dispatch it: the same guards, and the token 4
	// cycles a native routine is charged (the hardware moves on: an
	// H-Blank DMA it started copies its next chunk on time)
	if (gb->test.active && gb->call_depth >= 256) {
		gb->test.unrealistic = true;
		gb->test.crashed = true;
		return true;
	}
	if (gb->test.budget && --gb->test.budget == 0)
		gb->test.out_of_budget(gb);
	// it runs at wVBlankFunc (the blocks find their own addresses from pc)
	uint16_t pc = gb->cpu.pc;
	gb->cpu.pc = wVBlankFunc;
	if (gb->pc_cb)
		gb->pc_cb(gb);
	h->by_direct++;
	gb->call_depth++;
	h->fn(gb);
	gb->call_depth--;
	gb->cpu.pc = pc;
	if (!gb->no_tick)
		gb_tick(gb, 4);
	return true;
}

// The tile copy loop at pc, if it's there: run, and the address of the jp
// or ret that ends it in *end. False, doing nothing, if pc holds other code.
static bool tile_copy(GB *gb, uint16_t pc, uint16_t *end_out)
{
	int n = 0;
	while (n < 0x100 && ram_is(gb, (uint16_t)(pc + n * 7), tile_copy_code, NULL, sizeof(tile_copy_code)))
		n++;
	uint16_t end = (uint16_t)(pc + n * 7);
	uint8_t *op = gb_ram_byte(gb, end);
	if (!op || (*op != 0xc3 && (*op != 0xc9 || n)))
		return false;
	uint16_t hl = REG_HL(gb);
	for (int i = 0; i < n; i++) {
		gb->cpu.a = rd8(gb, hl++);
		gb->cpu.d = gb->cpu.a;
		gb->cpu.a = rd8(gb, hl++);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = rd8(gb, REG_BC(gb));
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.c = alu_inc(gb, gb->cpu.c);
	}
	SET_HL(gb, hl);
	*end_out = end;
	return true;
}

// the jp at end's target, or -1 for a ret
static int tile_copy_exit(GB *gb, uint16_t end)
{
	if (ram_byte(gb, end) == 0xc9)
		return -1;
	return ram_byte(gb, (uint16_t)(end + 1)) | ram_byte(gb, (uint16_t)(end + 2)) << 8;
}

static bool vblank_code(GB *gb)
{
	uint16_t pc = gb->cpu.pc, end;
	if (pc == wVBlankFunc && copied_func(gb))
		return true;
	if (!tile_copy(gb, pc, &end))
		return false;
	int to = tile_copy_exit(gb, end);
	if (to < 0)
		RET();
	else
		JUMP_TO(to);
	return true;
}

bool ram_tail_jump(GB *gb, uint16_t addr)
{
	if (addr == hTransferVirtualOAM)
		return ram_transfer_virtual_oam(gb);
	if (addr == wVBlankFunc && run_vblank_func(gb))
		return true;
	uint16_t end;
	if ((addr != wVBlankFunc && addr != wVBlankFunc + 0x10 && addr != wVBlankFuncExtended) ||
		!tile_copy(gb, addr, &end))
		return false;
	if (!gb->no_tick)
		gb_tick(gb, 4); // the dispatch's token cycles, as cpu_step charges
	int to = tile_copy_exit(gb, end);
	if (to >= 0)
		gb_tail_jump(gb, (uint16_t)to);
	return true;
}

// The ROM labels whose code the hooks above stand in for: hCallFunc and
// its operand labels (InitHRAMCallFunc copies them to HRAM), and PushOAM
// (copied to hTransferVirtualOAM).
PORT_RAM(hCallFunc);
PORT_RAM(hCallFuncBank);
PORT_RAM(hCallFuncPointer);
PORT_RAM(PushOAM);

void ram_hooks_register_all(void)
{
	ram_hook_register(hCallFunc, hcall_func);
	ram_hook_register(hTransferVirtualOAM, transfer_virtual_oam);
	ram_hook_register(wVBlankFunc, vblank_code);
	ram_hook_register(wVBlankFunc + 0x10, vblank_code);
	ram_hook_register(wVBlankFuncExtended, vblank_code);
}
