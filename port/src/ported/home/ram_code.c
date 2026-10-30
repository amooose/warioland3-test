// Native code for what the game runs from RAM: the HRAM call trampoline
// and OAM DMA routine (copied from ROM at start), and the V-Blank functions
// in WRAM (copied jumps, and the tile queue copy loops that
// SetTileQueueProcessingFunctions writes). Each first checks that the bytes
// there are the code it stands for, and otherwise leaves them to the
// interpreter. See ram_hook_register in hooks.h.
#include "ported.h"

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
	gb_call_ret(gb, func, (uint16_t)(hCallFunc + 0x0f));
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
	RET();
	return true;
}

// hTransferVirtualOAM (PushOAM): ldh [rDMA], a / ld a, 40 / .loop dec a /
// jr nz, .loop / ret, the DMA running while it waits
static const uint8_t push_oam_code[] = {0xe0, 0x46, 0x3e, 0x28, 0x3d, 0x20, 0xfd, 0xc9};

static bool transfer_virtual_oam(GB *gb)
{
	if (!ram_is(gb, hTransferVirtualOAM, push_oam_code, NULL, sizeof(push_oam_code)))
		return false;
	wr8(gb, rDMA, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = (uint8_t)(FLAG_Z | FLAG_N | (gb->cpu.f & FLAG_C)); // the last dec a
	idle(gb, 8 + 40 * 16 - 4); // ld a, n and the loop, as the asm takes
	RET();
	return true;
}

// The V-Blank functions in WRAM: a jp to the real one, a ret, or (from
// wVBlankFunc + $10 and wVBlankFuncExtended) SetTileQueueProcessingFunctions'
// copy loop, ld a, [hli] / ld d, a / ld a, [hli] / ld e, a / ld a, [bc] /
// ld [de], a / inc c once per queued tile, then a jp.
static const uint8_t tile_copy_code[] = {0x2a, 0x57, 0x2a, 0x5f, 0x0a, 0x12, 0x0c};

static bool vblank_code(GB *gb)
{
	uint16_t pc = gb->cpu.pc;
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
	if (*op == 0xc9) {
		RET();
		return true;
	}
	JUMP_TO(ram_byte(gb, (uint16_t)(end + 1)) | ram_byte(gb, (uint16_t)(end + 2)) << 8);
	return true;
}

void ram_hooks_register_all(void)
{
	ram_hook_register(hCallFunc, hcall_func);
	ram_hook_register(hTransferVirtualOAM, transfer_virtual_oam);
	ram_hook_register(wVBlankFunc, vblank_code);
	ram_hook_register(wVBlankFunc + 0x10, vblank_code);
	ram_hook_register(wVBlankFuncExtended, vblank_code);
}
