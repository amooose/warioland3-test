// Port of src/engine/overworld/bank76.asm
#include "ported.h"

// Writes the five VDMA registers from rVDMA_SRC_HIGH on, as the asm does
// with ld [$ff00+c], a / inc c; the write to rVDMA_LEN starts the transfer.
static void vdma(GB *gb, const uint8_t regs[5])
{
	gb->cpu.c = (uint8_t)rVDMA_SRC_HIGH;
	for (int i = 0; i < 5; i++) {
		if (i)
			gb->cpu.c = alu_inc(gb, gb->cpu.c);
		gb->cpu.a = regs[i];
		wr8(gb, (uint16_t)(0xff00 | gb->cpu.c), gb->cpu.a);
	}
}

PORT(Func_1d8c2c)
{
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, 1);
	const uint16_t src = wTilemap + 0x24 * TILE_SIZE;
	const uint8_t tilemap[5] = {(uint8_t)(src >> 8), (uint8_t)src, 0x17, 0x00, 0x0b};
	vdma(gb, tilemap);

	gb->cpu.a = rd8(gb, w2d044 + 0);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		const uint8_t regs[5] = {gb->cpu.a, rd8(gb, w2d044 + 1), 0x0e, 0xc0, 0x13};
		vdma(gb, regs);
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

// rr r (the CB one, which sets z)
static uint8_t rr(GB *gb, uint8_t v)
{
	uint8_t r = (uint8_t)(v >> 1 | ((gb->cpu.f & FLAG_C) ? 0x80 : 0));
	gb->cpu.f = (uint8_t)((r ? 0 : FLAG_Z) | ((v & 1) ? FLAG_C : 0));
	return r;
}

// rrca: bit 0 into bit 7 and carry, z clear
static uint8_t rrca(GB *gb, uint8_t v)
{
	gb->cpu.f = (v & 1) ? FLAG_C : 0;
	return (uint8_t)(v >> 1 | v << 7);
}

// ld a, [hli] / and a / ret z / dec a / inc [hl] / cp [hl] / ret nc /
// xor a / ld [hli], a / ld a, [hl]: returns false on either ret, else leaves
// hl at the current frame and a = it
static bool anim_tick(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	SET_HL(gb, hl);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a)
		return false;
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	alu_cp(gb, gb->cpu.a, rd8(gb, hl));
	if (!(gb->cpu.f & FLAG_C))
		return false;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	return true;
}

// hl = gfx + de + c, copied (2 tiles) to wTilemap tile `tile` + c, then
// sets bit `bit` of w2d031
static void copy_anim_tiles(GB *gb, uint16_t gfx, uint16_t tile, int bit)
{
	uint16_t hl = alu_add16(gb, gfx, REG_DE(gb));
	gb->cpu.b = 0;
	hl = alu_add16(gb, hl, REG_BC(gb));
	SET_HL(gb, hl);
	const uint16_t dest = wTilemap + tile * TILE_SIZE;
	gb->cpu.d = (uint8_t)(dest >> 8);
	gb->cpu.a = alu_add(gb, (uint8_t)dest, gb->cpu.c);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.b = 2 * TILE_SIZE;
	CALL(CopyHLToDE);
	SET_HL(gb, w2d031);
	wr8(gb, w2d031, (uint8_t)(rd8(gb, w2d031) | (1 << bit)));
}

static void update_big_clouds(GB *gb)
{
	if (!anim_tick(gb))
		return;
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x03);
	wr8(gb, REG_HL(gb), gb->cpu.a);

	SET_HL(gb, wCloudType);
	gb->cpu.d = rd8(gb, wCloudType);
	gb->cpu.d = alu_sla(gb, gb->cpu.d);
	gb->cpu.e = 0;
	gb->cpu.a = alu_rra(gb, gb->cpu.a);
	gb->cpu.e = rr(gb, gb->cpu.e);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.d);
	gb->cpu.d = gb->cpu.a;
	copy_anim_tiles(gb, ADDR_OverworldClouds1Gfx, 0x24, 0);
}

static void update_small_clouds(GB *gb)
{
	if (!anim_tick(gb))
		return;
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x03);
	wr8(gb, REG_HL(gb), gb->cpu.a);

	SET_HL(gb, w2d049);
	gb->cpu.d = rd8(gb, w2d049);
	gb->cpu.a = alu_rlca(gb, gb->cpu.a);
	gb->cpu.a = alu_rlca(gb, gb->cpu.a);
	gb->cpu.a = alu_swap(gb, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	copy_anim_tiles(gb, ADDR_OverworldSeaGfx, 0x2c, 1);
}

static void update_sea(GB *gb)
{
	if (!anim_tick(gb))
		return;
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 5 + 1);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
	}
	wr8(gb, REG_HL(gb), gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 4);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = alu_rlca(gb, gb->cpu.a);
	} else {
		alu_cp(gb, gb->cpu.a, 5);
		if (!(gb->cpu.f & FLAG_Z)) {
			gb->cpu.a = alu_rlca(gb, gb->cpu.a);
			gb->cpu.a = alu_rlca(gb, gb->cpu.a);
		}
	}
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x0c);
	gb->cpu.a = alu_swap(gb, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	SET_HL(gb, w2d049);
	gb->cpu.d = rd8(gb, w2d049);
	copy_anim_tiles(gb, ADDR_OverworldSeaGfx, 0x2c, 1);
}

// updates animations for clouds, sea and cliffs
PORT(UpdateCommonOWAnimations)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d031, 0);

	static const uint16_t big[4] = { w2d032, w2d035, w2d038, w2d03b };
	for (int i = 0; i < 4; i++) {
		SET_HL(gb, big[i]);
		gb->cpu.c = (uint8_t)(i * 0x20);
		update_big_clouds(gb);
	}
	SET_HL(gb, w2d03e);
	gb->cpu.c = 0x00;
	update_sea(gb);
	SET_HL(gb, w2d041);
	gb->cpu.c = 0x20;
	update_small_clouds(gb);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d044, 0);
	SET_HL(gb, w2d046);
	if (!anim_tick(gb))
		return;
	uint16_t hl = REG_HL(gb); // w2d048
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 3 + 1);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
	}
	wr8(gb, hl, gb->cpu.a);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_add(gb, gb->cpu.a, (uint8_t)(ADDR_OverworldCliffs1Gfx >> 8));
	wr8(gb, w2d044 + 0, gb->cpu.a);
	gb->cpu.a = gb->cpu.b;
	gb->cpu.a = rrca(gb, gb->cpu.a);
	gb->cpu.a = rrca(gb, gb->cpu.a);
	wr8(gb, w2d044 + 1, gb->cpu.a);
}

PORT(_LoadCloudAndSeaGfx)
{
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, 1);
	gb->cpu.a = rd8(gb, wCloudType);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.e = 0;
	uint16_t hl = alu_add16(gb, ADDR_OverworldClouds1Gfx, REG_DE(gb));
	gb_push16(gb, hl);
	SET_HL(gb, hl);
	SET_DE(gb, v1Tiles2 + 0x70 * TILE_SIZE);
	gb->cpu.b = 8 * TILE_SIZE;
	CALL(CopyHLToDE);
	SET_HL(gb, ADDR_OverworldSeaGfx);
	gb->cpu.b = 4 * TILE_SIZE;
	CALL(CopyHLToDE);
	SET_HL(gb, gb_pop16(gb));
	SET_DE(gb, wTilemap + 0x24 * TILE_SIZE);
	gb->cpu.b = 8 * TILE_SIZE;
	CALL(CopyHLToDE);
	SET_HL(gb, ADDR_OverworldSeaGfx);
	gb->cpu.b = 4 * TILE_SIZE;
	CALL(CopyHLToDE);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}
