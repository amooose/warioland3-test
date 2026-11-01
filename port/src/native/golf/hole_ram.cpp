// golf/hole_ram (src/engine/golf/hole_ram.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::golf {

namespace {

class GolfHoleRam : public AsmModule {
public:
	using AsmModule::AsmModule;

	// The parts of src/engine/golf/hole.asm that stay register-level C: the
	// V-Blank functions the holes copy to wVBlankFunc (run from there, see
	// ram_code.c) and HoldGolfWarioLastFrame (called from home code). The rest
	// of the golf minigame is C++ (src/native/golf).

	void port_HoldGolfWarioLastFrame()
	{
		cpu_.a = rd8(wGolfWarioState);
		static const uint8_t states[] = {
			GOLF_WARIO_CHARGING, GOLF_WARIO_ATTACKING_1, GOLF_WARIO_MISSING, GOLF_WARIO_TURNING,
		};
		bool ok = false;
		for (int i = 0; i < 4 && !ok; i++) {
			alu_.cp(cpu_.a, states[i]);
			ok = cpu_.f & FLAG_Z;
		}
		if (!ok)
			return;
		SET_HL(gb_, wGolfCounter);
		wr8(wGolfCounter, alu_.inc(rd8(wGolfCounter)));
		cpu_.a = rd8(wGolfAnimLastFrame);
		wr8(wGolfWarioCurrentFrame, cpu_.a);
	}

	// The .Func blocks below are copied to wVBlankFunc and run from there (see
	// ram_code.c). Each ends in ld a, HIGH(wVirtualOAM) / jp hTransferVirtualOAM.

	void vb_xor_a()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z;
	}

	void vb_transfer_oam()
	{
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		JUMP_TO(hTransferVirtualOAM);
	}

	// ld a, BANK("Golf RAM") / ldh [rWBK], a
	void vb_golf_wbk()
	{
		cpu_.a = BANK_wGolfHoleRAMStart;
		wr8(rWBK, cpu_.a);
	}

	// ld a, [wSCX] / ldh [rSCX], a, and likewise wGolfWX and wGolfLCDConfig
	void vb_scx()
	{
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
	}

	void vb_wx_lcdc()
	{
		cpu_.a = rd8(wGolfWX);
		wr8(rWX, cpu_.a);
		cpu_.a = rd8(wGolfLCDConfig);
		wr8(rLCDC, cpu_.a);
	}

	// ld a, [wGolfWarioTilesBank] / ld [rROMB0], a / ld hl, rVDMA_SRC_HIGH /
	// the tiles pointer / xor a / ld [hli], a x2 / ld a, len / ld [hl], a
	void vb_wario_tiles_dma(uint8_t len)
	{
		cpu_.a = rd8(wGolfWarioTilesBank);
		wr8(rROMB0, cpu_.a);
		cpu_.a = rd8(wGolfWarioTilesPtr + 0);
		wr8(rVDMA_SRC_HIGH, cpu_.a);
		cpu_.a = rd8(wGolfWarioTilesPtr + 1);
		wr8(rVDMA_SRC_LOW, cpu_.a);
		vb_xor_a();
		wr8(rVDMA_DEST_HIGH, cpu_.a);
		wr8(rVDMA_DEST_LOW, cpu_.a);
		cpu_.a = len;
		wr8(rVDMA_LEN, cpu_.a);
		SET_HL(gb_, rVDMA_LEN);
	}

	void port_VBlank_1c9d1d__Func()
	{
		vb_golf_wbk();
		wr8(rVBK, cpu_.a);
		vb_scx();
		vb_wx_lcdc();
		wr8(rVDMA_SRC_HIGH, cpu_.a = (uint8_t)(wGolfPinFlagTiles >> 8));
		wr8(rVDMA_SRC_LOW, cpu_.a = (uint8_t)wGolfPinFlagTiles);
		wr8(rVDMA_DEST_HIGH, cpu_.a = 0x17);
		wr8(rVDMA_DEST_LOW, cpu_.a = 0x40);
		wr8(rVDMA_LEN, cpu_.a = 0x03); // 4 dma_tiles
		SET_HL(gb_, rVDMA_LEN);
		vb_xor_a();
		wr8(rVBK, cpu_.a);
		vb_transfer_oam();
	}

	void port_VBlank_1c9e8d__Func()
	{
		vb_golf_wbk();
		wr8(rVBK, cpu_.a);
		cpu_.a = 0x03;
		wr8(v0BGMap0 + 10 * TILEMAP_WIDTH + 5, cpu_.a); // ldcoord_a 5, 10
		cpu_.a = rd8(wGolfLCDConfig);
		wr8(rLCDC, cpu_.a);
		vb_xor_a();
		wr8(rVBK, cpu_.a);
		wr8(wGolfVBlankMode, cpu_.a);
		vb_transfer_oam();
	}

	void port_VBlank_1c9eb3__Func()
	{
		vb_golf_wbk();
		vb_xor_a();
		wr8(wGolfVBlankMode, cpu_.a);
		vb_scx();
		vb_wx_lcdc();
		vb_wario_tiles_dma(0x7f);
		vb_transfer_oam();
	}

	void port_VBlank_1c9ef1__Func()
	{
		vb_golf_wbk();
		wr8(rVBK, cpu_.a);
		vb_xor_a();
		wr8(wGolfVBlankMode, cpu_.a);
		vb_scx();
		vb_wx_lcdc();
		wr8(rVDMA_SRC_HIGH, cpu_.a = (uint8_t)(w1db90 >> 8));
		wr8(rVDMA_SRC_LOW, cpu_.a = (uint8_t)w1db90);
		wr8(rVDMA_DEST_HIGH, cpu_.a = 0x0f);
		vb_xor_a();
		wr8(rVDMA_DEST_LOW, cpu_.a);
		wr8(rVDMA_LEN, cpu_.a = 0x03);
		SET_HL(gb_, rVDMA_LEN);
		vb_xor_a();
		wr8(rVBK, cpu_.a);
		vb_transfer_oam();
	}

	// add a / add $e0 / ldcoord_a x, 8 / inc a / ldcoord_a x, 9
	void vb_coin_digit(uint8_t v, int x)
	{
		cpu_.a = alu_.add(v, v);
		cpu_.a = alu_.add(cpu_.a, 0xe0);
		wr8((uint16_t)(v0BGMap0 + 8 * TILEMAP_WIDTH + x), cpu_.a);
		cpu_.a = alu_.inc(cpu_.a);
		wr8((uint16_t)(v0BGMap0 + 9 * TILEMAP_WIDTH + x), cpu_.a);
	}

	// update the coin number in the Golf Lobby
	void port_VBlank_1c9f36__Func()
	{
		vb_golf_wbk();
		vb_xor_a();
		wr8(rVBK, cpu_.a);
		vb_coin_digit(rd8(wGolfNumCoins + 0), 6);
		cpu_.a = alu_.and_(rd8(wGolfNumCoins + 1), 0xf0);
		cpu_.a = alu_.swap(cpu_.a);
		vb_coin_digit(cpu_.a, 7);
		cpu_.a = alu_.and_(rd8(wGolfNumCoins + 1), 0x0f);
		vb_coin_digit(cpu_.a, 8);
		vb_transfer_oam();
	}

	// VBlank_1c9eb3.Func without the WX and LCDC writes. No recording reaches
	// it, and it switches the ROM bank, so every fuzz check of it at its ROM
	// address is inconclusive: it was checked against the ROM bytes by hand.
	void port_VBlank_1c9f7a__Func()
	{
		vb_golf_wbk();
		vb_xor_a();
		wr8(wGolfVBlankMode, cpu_.a);
		vb_scx();
		vb_wario_tiles_dma(0x7f);
		vb_transfer_oam();
	}

	void port_VBlank_1c9fe1__Func()
	{
		vb_golf_wbk();
		vb_scx();
		vb_transfer_oam();
	}

	// Shared by VBlank_1c9ffa.Func and VBlank_1ca056.Func: ld a, BANK("VRAM1") /
	// ldh [rVBK], a / ld hl, map / call Func_2c30 / xor a / ldh [rVBK], a
	void vb_attrmap(uint16_t map)
	{
		uint16_t base = cpu_.pc;
		vb_golf_wbk();
		vb_xor_a();
		wr8(wGolfVBlankMode, cpu_.a);
		vb_scx();
		cpu_.a = 1; // BANK("VRAM1")
		wr8(rVBK, cpu_.a);
		SET_HL(gb_, map);
		gb_call_ret(gb_, ADDR_Func_2c30, (uint16_t)(base + 0x17));
		vb_xor_a();
		wr8(rVBK, cpu_.a);
		vb_transfer_oam();
	}


	void port_VBlank_1c9ffa__Func() { vb_attrmap(wGoldBuildingCourseAttrMap); }

	void port_VBlank_1ca033__Func()
	{
		uint16_t base = cpu_.pc;
		vb_golf_wbk();
		vb_xor_a();
		wr8(wGolfVBlankMode, cpu_.a);
		vb_scx();
		SET_HL(gb_, w1db00);
		gb_call_ret(gb_, ADDR_Func_2c46, (uint16_t)(base + 0x10));
		vb_transfer_oam();
	}

	void port_VBlank_1ca056__Func() { vb_attrmap(wGolfPinFlagTiles); }
};

} // namespace

} // namespace wl3::golf

using wl3::golf::GolfHoleRam;

PORT_EX(HoldGolfWarioLastFrame, 0, 0) { GolfHoleRam(gb).port_HoldGolfWarioLastFrame(); }
PORT_LOCAL_EX(VBlank_1c9d1d, Func, HOOK_MANUAL_RET, 0) { GolfHoleRam(gb).port_VBlank_1c9d1d__Func(); }
PORT_LOCAL_EX(VBlank_1c9e8d, Func, HOOK_MANUAL_RET, 0) { GolfHoleRam(gb).port_VBlank_1c9e8d__Func(); }
PORT_LOCAL_EX(VBlank_1c9eb3, Func, HOOK_MANUAL_RET, 0) { GolfHoleRam(gb).port_VBlank_1c9eb3__Func(); }
PORT_LOCAL_EX(VBlank_1c9ef1, Func, HOOK_MANUAL_RET, 0) { GolfHoleRam(gb).port_VBlank_1c9ef1__Func(); }
PORT_LOCAL_EX(VBlank_1c9f36, Func, HOOK_MANUAL_RET, 0) { GolfHoleRam(gb).port_VBlank_1c9f36__Func(); }
PORT_LOCAL_EX(VBlank_1c9f7a, Func, HOOK_MANUAL_RET, 0) { GolfHoleRam(gb).port_VBlank_1c9f7a__Func(); }
PORT_LOCAL_EX(VBlank_1c9fe1, Func, HOOK_MANUAL_RET, 0) { GolfHoleRam(gb).port_VBlank_1c9fe1__Func(); }
PORT_LOCAL_EX(VBlank_1c9ffa, Func, HOOK_MANUAL_RET, 0) { GolfHoleRam(gb).port_VBlank_1c9ffa__Func(); }
PORT_LOCAL_EX(VBlank_1ca033, Func, HOOK_MANUAL_RET, 0) { GolfHoleRam(gb).port_VBlank_1ca033__Func(); }
PORT_LOCAL_EX(VBlank_1ca056, Func, HOOK_MANUAL_RET, 0) { GolfHoleRam(gb).port_VBlank_1ca056__Func(); }
