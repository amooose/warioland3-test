// The parts of src/engine/golf/hole.asm that stay register-level C: the
// V-Blank functions the holes copy to wVBlankFunc (run from there, see
// ram_code.c) and HoldGolfWarioLastFrame (called from home code). The rest
// of the golf minigame is C++ (src/native/golf).
#include "ported.h"

PORT(HoldGolfWarioLastFrame)
{
	gb->cpu.a = rd8(gb, wGolfWarioState);
	static const uint8_t states[] = {
		GOLF_WARIO_CHARGING, GOLF_WARIO_ATTACKING_1, GOLF_WARIO_MISSING, GOLF_WARIO_TURNING,
	};
	bool ok = false;
	for (int i = 0; i < 4 && !ok; i++) {
		alu_cp(gb, gb->cpu.a, states[i]);
		ok = gb->cpu.f & FLAG_Z;
	}
	if (!ok)
		return;
	SET_HL(gb, wGolfCounter);
	wr8(gb, wGolfCounter, alu_inc(gb, rd8(gb, wGolfCounter)));
	gb->cpu.a = rd8(gb, wGolfAnimLastFrame);
	wr8(gb, wGolfWarioCurrentFrame, gb->cpu.a);
}

// The .Func blocks below are copied to wVBlankFunc and run from there (see
// ram_code.c). Each ends in ld a, HIGH(wVirtualOAM) / jp hTransferVirtualOAM.

static void vb_xor_a(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
}

static void vb_transfer_oam(GB *gb)
{
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	JUMP_TO(hTransferVirtualOAM);
}

// ld a, BANK("Golf RAM") / ldh [rWBK], a
static void vb_golf_wbk(GB *gb)
{
	gb->cpu.a = BANK_wGolfHoleRAMStart;
	wr8(gb, rWBK, gb->cpu.a);
}

// ld a, [wSCX] / ldh [rSCX], a, and likewise wGolfWX and wGolfLCDConfig
static void vb_scx(GB *gb)
{
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
}

static void vb_wx_lcdc(GB *gb)
{
	gb->cpu.a = rd8(gb, wGolfWX);
	wr8(gb, rWX, gb->cpu.a);
	gb->cpu.a = rd8(gb, wGolfLCDConfig);
	wr8(gb, rLCDC, gb->cpu.a);
}

// ld a, [wGolfWarioTilesBank] / ld [rROMB0], a / ld hl, rVDMA_SRC_HIGH /
// the tiles pointer / xor a / ld [hli], a x2 / ld a, len / ld [hl], a
static void vb_wario_tiles_dma(GB *gb, uint8_t len)
{
	gb->cpu.a = rd8(gb, wGolfWarioTilesBank);
	wr8(gb, rROMB0, gb->cpu.a);
	gb->cpu.a = rd8(gb, wGolfWarioTilesPtr + 0);
	wr8(gb, rVDMA_SRC_HIGH, gb->cpu.a);
	gb->cpu.a = rd8(gb, wGolfWarioTilesPtr + 1);
	wr8(gb, rVDMA_SRC_LOW, gb->cpu.a);
	vb_xor_a(gb);
	wr8(gb, rVDMA_DEST_HIGH, gb->cpu.a);
	wr8(gb, rVDMA_DEST_LOW, gb->cpu.a);
	gb->cpu.a = len;
	wr8(gb, rVDMA_LEN, gb->cpu.a);
	SET_HL(gb, rVDMA_LEN);
}

PORT_LOCAL_JUMP(VBlank_1c9d1d, Func)
{
	vb_golf_wbk(gb);
	wr8(gb, rVBK, gb->cpu.a);
	vb_scx(gb);
	vb_wx_lcdc(gb);
	wr8(gb, rVDMA_SRC_HIGH, gb->cpu.a = (uint8_t)(wGolfPinFlagTiles >> 8));
	wr8(gb, rVDMA_SRC_LOW, gb->cpu.a = (uint8_t)wGolfPinFlagTiles);
	wr8(gb, rVDMA_DEST_HIGH, gb->cpu.a = 0x17);
	wr8(gb, rVDMA_DEST_LOW, gb->cpu.a = 0x40);
	wr8(gb, rVDMA_LEN, gb->cpu.a = 0x03); // 4 dma_tiles
	SET_HL(gb, rVDMA_LEN);
	vb_xor_a(gb);
	wr8(gb, rVBK, gb->cpu.a);
	vb_transfer_oam(gb);
}

PORT_LOCAL_JUMP(VBlank_1c9e8d, Func)
{
	vb_golf_wbk(gb);
	wr8(gb, rVBK, gb->cpu.a);
	gb->cpu.a = 0x03;
	wr8(gb, v0BGMap0 + 10 * TILEMAP_WIDTH + 5, gb->cpu.a); // ldcoord_a 5, 10
	gb->cpu.a = rd8(gb, wGolfLCDConfig);
	wr8(gb, rLCDC, gb->cpu.a);
	vb_xor_a(gb);
	wr8(gb, rVBK, gb->cpu.a);
	wr8(gb, wGolfVBlankMode, gb->cpu.a);
	vb_transfer_oam(gb);
}

PORT_LOCAL_JUMP(VBlank_1c9eb3, Func)
{
	vb_golf_wbk(gb);
	vb_xor_a(gb);
	wr8(gb, wGolfVBlankMode, gb->cpu.a);
	vb_scx(gb);
	vb_wx_lcdc(gb);
	vb_wario_tiles_dma(gb, 0x7f);
	vb_transfer_oam(gb);
}

PORT_LOCAL_JUMP(VBlank_1c9ef1, Func)
{
	vb_golf_wbk(gb);
	wr8(gb, rVBK, gb->cpu.a);
	vb_xor_a(gb);
	wr8(gb, wGolfVBlankMode, gb->cpu.a);
	vb_scx(gb);
	vb_wx_lcdc(gb);
	wr8(gb, rVDMA_SRC_HIGH, gb->cpu.a = (uint8_t)(w1db90 >> 8));
	wr8(gb, rVDMA_SRC_LOW, gb->cpu.a = (uint8_t)w1db90);
	wr8(gb, rVDMA_DEST_HIGH, gb->cpu.a = 0x0f);
	vb_xor_a(gb);
	wr8(gb, rVDMA_DEST_LOW, gb->cpu.a);
	wr8(gb, rVDMA_LEN, gb->cpu.a = 0x03);
	SET_HL(gb, rVDMA_LEN);
	vb_xor_a(gb);
	wr8(gb, rVBK, gb->cpu.a);
	vb_transfer_oam(gb);
}

// add a / add $e0 / ldcoord_a x, 8 / inc a / ldcoord_a x, 9
static void vb_coin_digit(GB *gb, uint8_t v, int x)
{
	gb->cpu.a = alu_add(gb, v, v);
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0xe0);
	wr8(gb, (uint16_t)(v0BGMap0 + 8 * TILEMAP_WIDTH + x), gb->cpu.a);
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	wr8(gb, (uint16_t)(v0BGMap0 + 9 * TILEMAP_WIDTH + x), gb->cpu.a);
}

// update the coin number in the Golf Lobby
PORT_LOCAL_JUMP(VBlank_1c9f36, Func)
{
	vb_golf_wbk(gb);
	vb_xor_a(gb);
	wr8(gb, rVBK, gb->cpu.a);
	vb_coin_digit(gb, rd8(gb, wGolfNumCoins + 0), 6);
	gb->cpu.a = alu_and(gb, rd8(gb, wGolfNumCoins + 1), 0xf0);
	gb->cpu.a = alu_swap(gb, gb->cpu.a);
	vb_coin_digit(gb, gb->cpu.a, 7);
	gb->cpu.a = alu_and(gb, rd8(gb, wGolfNumCoins + 1), 0x0f);
	vb_coin_digit(gb, gb->cpu.a, 8);
	vb_transfer_oam(gb);
}

// VBlank_1c9eb3.Func without the WX and LCDC writes. No recording reaches
// it, and it switches the ROM bank, so every fuzz check of it at its ROM
// address is inconclusive: it was checked against the ROM bytes by hand.
PORT_LOCAL_JUMP(VBlank_1c9f7a, Func)
{
	vb_golf_wbk(gb);
	vb_xor_a(gb);
	wr8(gb, wGolfVBlankMode, gb->cpu.a);
	vb_scx(gb);
	vb_wario_tiles_dma(gb, 0x7f);
	vb_transfer_oam(gb);
}

PORT_LOCAL_JUMP(VBlank_1c9fe1, Func)
{
	vb_golf_wbk(gb);
	vb_scx(gb);
	vb_transfer_oam(gb);
}

// Shared by VBlank_1c9ffa.Func and VBlank_1ca056.Func: ld a, BANK("VRAM1") /
// ldh [rVBK], a / ld hl, map / call Func_2c30 / xor a / ldh [rVBK], a
static void vb_attrmap(GB *gb, uint16_t map)
{
	uint16_t base = gb->cpu.pc;
	vb_golf_wbk(gb);
	vb_xor_a(gb);
	wr8(gb, wGolfVBlankMode, gb->cpu.a);
	vb_scx(gb);
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, map);
	gb_call_ret(gb, ADDR_Func_2c30, (uint16_t)(base + 0x17));
	vb_xor_a(gb);
	wr8(gb, rVBK, gb->cpu.a);
	vb_transfer_oam(gb);
}


PORT_LOCAL_JUMP(VBlank_1c9ffa, Func) { vb_attrmap(gb, wGoldBuildingCourseAttrMap); }

PORT_LOCAL_JUMP(VBlank_1ca033, Func)
{
	uint16_t base = gb->cpu.pc;
	vb_golf_wbk(gb);
	vb_xor_a(gb);
	wr8(gb, wGolfVBlankMode, gb->cpu.a);
	vb_scx(gb);
	SET_HL(gb, w1db00);
	gb_call_ret(gb, ADDR_Func_2c46, (uint16_t)(base + 0x10));
	vb_transfer_oam(gb);
}

PORT_LOCAL_JUMP(VBlank_1ca056, Func) { vb_attrmap(gb, wGolfPinFlagTiles); }
