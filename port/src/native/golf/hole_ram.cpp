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
	// ram_code.c), plain over memory. Each ends in ld a, HIGH(wVirtualOAM) /
	// jp hTransferVirtualOAM, which reads only a.

	Memory ram() { return Memory(gb_); }

	void transfer_oam()
	{
		cpu_.a = static_cast<uint8_t>(wVirtualOAM >> 8);
		JUMP_TO(hTransferVirtualOAM);
	}

	// ld a, BANK("Golf RAM") / ldh [rWBK], a (vbk: and to rVBK)
	void golf_wbk(bool vbk = false)
	{
		Memory mem(gb_);
		mem[rWBK] = BANK_wGolfHoleRAMStart;
		if (vbk)
			mem[rVBK] = BANK_wGolfHoleRAMStart;
	}

	void scx() { ram()[rSCX] = ram()[wSCX]; }

	void wx_lcdc()
	{
		Memory mem(gb_);
		mem[rWX] = mem[wGolfWX];
		mem[rLCDC] = mem[wGolfLCDConfig];
	}

	void hdma(uint16_t src, uint16_t dest, uint8_t len)
	{
		Memory mem(gb_);
		mem[rVDMA_SRC_HIGH] = static_cast<uint8_t>(src >> 8);
		mem[rVDMA_SRC_LOW] = static_cast<uint8_t>(src);
		mem[rVDMA_DEST_HIGH] = static_cast<uint8_t>(dest >> 8);
		mem[rVDMA_DEST_LOW] = static_cast<uint8_t>(dest);
		mem[rVDMA_LEN] = len;
	}

	// Golf Wario's tiles, from wGolfWarioTilesBank (written straight to the
	// MBC) at wGolfWarioTilesPtr (big-endian), to VRAM $8000
	void wario_tiles_dma(uint8_t len)
	{
		Memory mem(gb_);
		mem[rROMB0] = mem[wGolfWarioTilesBank];
		hdma(static_cast<uint16_t>(mem[wGolfWarioTilesPtr + 0] << 8 | mem[wGolfWarioTilesPtr + 1]), 0, len);
	}

	void port_VBlank_1c9d1d__Func()
	{
		golf_wbk(true);
		scx();
		wx_lcdc();
		hdma(wGolfPinFlagTiles, 0x1740, 0x03); // 4 dma_tiles
		ram()[rVBK] = 0;
		transfer_oam();
	}

	void port_VBlank_1c9e8d__Func()
	{
		Memory mem(gb_);
		golf_wbk(true);
		mem[v0BGMap0 + 10 * TILEMAP_WIDTH + 5] = 0x03; // ldcoord_a 5, 10
		mem[rLCDC] = mem[wGolfLCDConfig];
		mem[rVBK] = 0;
		mem[wGolfVBlankMode] = 0;
		transfer_oam();
	}

	void port_VBlank_1c9eb3__Func()
	{
		golf_wbk();
		ram()[wGolfVBlankMode] = 0;
		scx();
		wx_lcdc();
		wario_tiles_dma(0x7f);
		transfer_oam();
	}

	void port_VBlank_1c9ef1__Func()
	{
		golf_wbk(true);
		ram()[wGolfVBlankMode] = 0;
		scx();
		wx_lcdc();
		hdma(w1db90, 0x0f00, 0x03);
		ram()[rVBK] = 0;
		transfer_oam();
	}

	// add a / add $e0 / ldcoord_a x, 8 / inc a / ldcoord_a x, 9
	void coin_digit(uint8_t v, int x)
	{
		Memory mem(gb_);
		uint8_t tile = static_cast<uint8_t>(v * 2 + 0xe0);
		mem[static_cast<uint16_t>(v0BGMap0 + 8 * TILEMAP_WIDTH + x)] = tile;
		mem[static_cast<uint16_t>(v0BGMap0 + 9 * TILEMAP_WIDTH + x)] = static_cast<uint8_t>(tile + 1);
	}

	// update the coin number in the Golf Lobby
	void port_VBlank_1c9f36__Func()
	{
		Memory mem(gb_);
		golf_wbk();
		mem[rVBK] = 0;
		coin_digit(mem[wGolfNumCoins + 0], 6);
		coin_digit(mem[wGolfNumCoins + 1] >> 4, 7);
		coin_digit(mem[wGolfNumCoins + 1] & 0x0f, 8);
		transfer_oam();
	}

	// VBlank_1c9eb3.Func without the WX and LCDC writes. No recording reaches
	// it, and it switches the ROM bank, so every fuzz check of it at its ROM
	// address is inconclusive: it was checked against the ROM bytes by hand.
	void port_VBlank_1c9f7a__Func()
	{
		golf_wbk();
		ram()[wGolfVBlankMode] = 0;
		scx();
		wario_tiles_dma(0x7f);
		transfer_oam();
	}

	void port_VBlank_1c9fe1__Func()
	{
		golf_wbk();
		scx();
		transfer_oam();
	}

	// Shared by VBlank_1c9ffa.Func and VBlank_1ca056.Func: the map's
	// attributes into VRAM1 (Func_2c30 takes hl)
	void attrmap(uint16_t map)
	{
		uint16_t base = cpu_.pc;
		Memory mem(gb_);
		golf_wbk();
		mem[wGolfVBlankMode] = 0;
		scx();
		mem[rVBK] = 1; // BANK("VRAM1")
		SET_HL(gb_, map);
		gb_call_ret(gb_, ADDR_Func_2c30, static_cast<uint16_t>(base + 0x17));
		mem[rVBK] = 0;
		transfer_oam();
	}

	void port_VBlank_1c9ffa__Func() { attrmap(wGoldBuildingCourseAttrMap); }

	// Func_2c46 takes hl
	void port_VBlank_1ca033__Func()
	{
		uint16_t base = cpu_.pc;
		golf_wbk();
		ram()[wGolfVBlankMode] = 0;
		scx();
		SET_HL(gb_, w1db00);
		gb_call_ret(gb_, ADDR_Func_2c46, static_cast<uint16_t>(base + 0x10));
		transfer_oam();
	}

	void port_VBlank_1ca056__Func() { attrmap(wGolfPinFlagTiles); }
};

} // namespace

} // namespace wl3::golf

using wl3::golf::GolfHoleRam;

PORT_EX(HoldGolfWarioLastFrame, 0, 0) { GolfHoleRam(gb).port_HoldGolfWarioLastFrame(); }
PORT_LOCAL_EX(VBlank_1c9d1d, Func, HOOK_MANUAL_RET, ALL_REGISTERS & ~R_A) { GolfHoleRam(gb).port_VBlank_1c9d1d__Func(); }
PORT_LOCAL_EX(VBlank_1c9e8d, Func, HOOK_MANUAL_RET, ALL_REGISTERS & ~R_A) { GolfHoleRam(gb).port_VBlank_1c9e8d__Func(); }
PORT_LOCAL_EX(VBlank_1c9eb3, Func, HOOK_MANUAL_RET, ALL_REGISTERS & ~R_A) { GolfHoleRam(gb).port_VBlank_1c9eb3__Func(); }
PORT_LOCAL_EX(VBlank_1c9ef1, Func, HOOK_MANUAL_RET, ALL_REGISTERS & ~R_A) { GolfHoleRam(gb).port_VBlank_1c9ef1__Func(); }
PORT_LOCAL_EX(VBlank_1c9f36, Func, HOOK_MANUAL_RET, ALL_REGISTERS & ~R_A) { GolfHoleRam(gb).port_VBlank_1c9f36__Func(); }
PORT_LOCAL_EX(VBlank_1c9f7a, Func, HOOK_MANUAL_RET, ALL_REGISTERS & ~R_A) { GolfHoleRam(gb).port_VBlank_1c9f7a__Func(); }
PORT_LOCAL_EX(VBlank_1c9fe1, Func, HOOK_MANUAL_RET, ALL_REGISTERS & ~R_A) { GolfHoleRam(gb).port_VBlank_1c9fe1__Func(); }
PORT_LOCAL_EX(VBlank_1c9ffa, Func, HOOK_MANUAL_RET, ALL_REGISTERS & ~R_A) { GolfHoleRam(gb).port_VBlank_1c9ffa__Func(); }
PORT_LOCAL_EX(VBlank_1ca033, Func, HOOK_MANUAL_RET, ALL_REGISTERS & ~R_A) { GolfHoleRam(gb).port_VBlank_1ca033__Func(); }
PORT_LOCAL_EX(VBlank_1ca056, Func, HOOK_MANUAL_RET, ALL_REGISTERS & ~R_A) { GolfHoleRam(gb).port_VBlank_1ca056__Func(); }
