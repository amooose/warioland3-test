// Port of src/engine/clear/vblank.asm: the V-Blank functions (the copiers
// are C++, src/native/clear)
#include "ported.h"

// ld hl, src / ld c, [hl] / ld b, TILE_SIZE - 1 / ld de, src - 2 /
// .loop: ld a, [de] / ld [hld], a / dec l / dec e / dec e / dec b /
// jr nz, .loop / ld [hl], c
static void rotate_tile_plane(GB *gb, uint16_t src)
{
	SET_HL(gb, src);
	gb->cpu.c = rd8(gb, src);
	gb->cpu.b = TILE_SIZE - 1;
	SET_DE(gb, (uint16_t)(src - 2));
	do {
		gb->cpu.a = rd8(gb, REG_DE(gb));
		uint16_t hl = REG_HL(gb);
		wr8(gb, hl, gb->cpu.a);
		SET_HL(gb, (uint16_t)(hl - 1));
		gb->cpu.l = alu_dec(gb, gb->cpu.l);
		gb->cpu.e = alu_dec(gb, gb->cpu.e);
		gb->cpu.e = alu_dec(gb, gb->cpu.e);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (!(gb->cpu.f & FLAG_Z));
	wr8(gb, REG_HL(gb), gb->cpu.c);
}

// VBlank_ClearScreen.Func, as copied to wVBlankFunc
PORT_LOCAL_JUMP(VBlank_ClearScreen, Func)
{
	uint16_t base = gb->cpu.pc;
	gb->cpu.a = rd8(gb, wSCY);
	wr8(gb, rSCY, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, gb->cpu.a);
	rotate_tile_plane(gb, v0Tiles2 + 0x71 * TILE_SIZE + (TILE_SIZE - 2));
	rotate_tile_plane(gb, v0Tiles2 + 0x71 * TILE_SIZE + (TILE_SIZE - 1));
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	gb_call_ret(gb, hTransferVirtualOAM, (uint16_t)(base + 0x36));
	RET();
}

// ld a, [hli] / ld d, a / ld a, [hli] / ld e, a: the queued address
static uint16_t queue_addr(GB *gb, uint16_t hl)
{
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.a = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.e = gb->cpu.a;
	return (uint16_t)(hl + 2);
}

// VBlank_MusicalCoinsScreen.Func, as copied to wVBlankFunc
PORT_LOCAL_JUMP(VBlank_MusicalCoinsScreen, Func)
{
	uint16_t base = gb->cpu.pc;
	gb->cpu.a = rd8(gb, wSCY);
	wr8(gb, rSCY, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
	gb->cpu.a = rd8(gb, wcee4);
	if (alu_bit(gb, gb->cpu.a, 7)) {
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x7f);
		uint16_t hl;
		if (!(gb->cpu.f & FLAG_Z)) {
			// .asm_d47d9
			gb->cpu.a = 1; // BANK("VRAM1")
			wr8(gb, rVBK, gb->cpu.a);
			hl = queue_addr(gb, wBGMapTileVRAM0Queue);
			gb->cpu.a = rd8(gb, hl);
			wr8(gb, REG_DE(gb), gb->cpu.a);
		} else {
			hl = queue_addr(gb, wBGMapTileVRAM0Queue);
			gb->cpu.a = rd8(gb, hl++);
			wr8(gb, REG_DE(gb), gb->cpu.a);
			hl = queue_addr(gb, hl);
			gb->cpu.a = rd8(gb, hl++);
			wr8(gb, REG_DE(gb), gb->cpu.a);
		}
		// .asm_d47e6
		SET_HL(gb, wcee4);
		wr8(gb, wcee4, (uint8_t)(rd8(gb, wcee4) & 0x7f));
	}
	// .dma_transfer
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	gb_call_ret(gb, hTransferVirtualOAM, (uint16_t)(base + 0x3d));
	RET();
}
