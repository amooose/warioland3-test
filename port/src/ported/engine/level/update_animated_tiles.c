// Port of src/engine/level/update_animated_tiles.asm
#include "ported.h"

// Copies the next frame of the room's animated tiles to v1Tiles2 tile $70,
// one row of 8 bytes per H-Blank, when the frame duration has passed.
PORT(UpdateRoomAnimatedTiles)
{
	gb->cpu.a = rd8(gb, rLY);
	alu_cp(gb, gb->cpu.a, LY_VBLANK - 20);
	if (!(gb->cpu.f & FLAG_C))
		return;
	gb->cpu.a = rd8(gb, wAnimatedTilesFrameDuration);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_inc(gb, rd8(gb, wAnimatedTilesFrameCount));
	wr8(gb, wAnimatedTilesFrameCount, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C)
		return;

	gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
	wr8(gb, wAnimatedTilesFrameCount, gb->cpu.a);
	gb->cpu.a = rd8(gb, wAnimatedTilesGfx);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, (uint8_t)(ADDR_AnimatedTilesGfx >> 8));
	uint16_t hl = (uint16_t)(gb->cpu.a << 8 | (uint8_t)ADDR_AnimatedTilesGfx);
	gb->cpu.a = alu_inc(gb, rd8(gb, wAnimatedTilesFrame));
	alu_cp(gb, gb->cpu.a, 0x04);
	if (gb->cpu.f & FLAG_Z)
		gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
	wr8(gb, wAnimatedTilesFrame, gb->cpu.a);
	gb->cpu.a = alu_swap(gb, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a); // *$80
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = alu_rl(gb, 0x00);
	hl = alu_add16(gb, hl, REG_DE(gb));

	uint16_t de = v1Tiles2 + TILE_SIZE * 0x70;
	gb->cpu.b = TILE_SIZE;
	gb->cpu.a = BANK_v1Tiles2;
	wr8(gb, rVBK, gb->cpu.a);
	do {
		// wait_not_hblank / wait_hblank
		while (!(gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
			idle(gb, 32);
		while ((gb->cpu.a = alu_and(gb, rd8(gb, rSTAT), STAT_MODE)))
			idle(gb, 32);
		for (int i = 0; i < 8; i++) {
			gb->cpu.a = rd8(gb, hl++);
			wr8(gb, de, gb->cpu.a);
			de = (uint16_t)((de & 0xff00) | alu_inc(gb, (uint8_t)de)); // inc e
		}
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (gb->cpu.b);
	SET_DE(gb, de);
	SET_HL(gb, hl);
	gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a); // VRAM0
	wr8(gb, rVBK, gb->cpu.a);
}
