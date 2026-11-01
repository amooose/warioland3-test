// overworld/vblank (src/engine/overworld/vblank.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldVblank : public AsmModule {
public:
	using AsmModule::AsmModule;

	// Both copy their .Func, the VBlank handler that runs from WRAM, to
	// wVBlankFunc. The handlers themselves stay asm.

	void port_VBlank_Overworld()
	{
		SET_HL(gb_, ADDR_VBlank_Overworld + 0x0c); // .Func
		SET_DE(gb_, wVBlankFunc);
		cpu_.b = (uint8_t)(ADDR_VBlank_OWScene - (ADDR_VBlank_Overworld + 0x0c)); // .end - .Func
		CALL(CopyHLToDE);
	}

	void port_VBlank_OWScene()
	{
		SET_HL(gb_, ADDR_VBlank_OWScene + 0x0c); // .Func
		SET_DE(gb_, wVBlankFunc);
		cpu_.b = (uint8_t)(ADDR_UnreferencedOpenTreasureCollection - (ADDR_VBlank_OWScene + 0x0c)); // .end - .Func
		CALL(CopyHLToDE);
	}

	// ld [$ff00+c], a
	void ldh_c()
	{
		wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
	}

	// ld a, [hli]
	void ld_a_hli()
	{
		uint16_t hl = REG_HL(gb_);
		cpu_.a = rd8(hl);
		SET_HL(gb_, (uint16_t)(hl + 1));
	}

	// ld b, a / ld a, BANK(x) or [wHDMABank] / ...: push the current bank and
	// map b (ld a, [wROMBank] / push af / ld a, b / bankswitch)
	void push_bank_switch()
	{
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = cpu_.b;
		bankswitch(cpu_.a);
	}

	// pop af / bankswitch
	void pop_bank()
	{
		pop_af();
		bankswitch(cpu_.a);
	}

	// [hl] is nonzero: xor a / ld [hli], a, then the 5 HDMA registers from
	// b and the 4 bytes after it
	void start_hdma()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		uint16_t hl = REG_HL(gb_);
		wr8(hl, 0);
		SET_HL(gb_, (uint16_t)(hl + 1));
		cpu_.a = cpu_.b;
		cpu_.c = (uint8_t)rVDMA_SRC_HIGH;
		ldh_c();
		for (int i = 0; i < 4; i++) {
			ld_a_hli();
			cpu_.c = alu_.inc(cpu_.c);
			ldh_c();
		}
	}

	// VBlank_Overworld.Func, as copied to wVBlankFunc (see ram_code.c). Its calls
	// return to base + n, the block's start wherever it runs from.
	void port_VBlank_Overworld__Func()
	{
		uint16_t base = cpu_.pc;
		cpu_.a = 0x02; // BANK("WRAM2")
		wr8(rWBK, cpu_.a);
		wr8(rSCY, rd8(wSCY));
		wr8(rSCX, rd8(wSCX));
		wr8(rWY, rd8(wWY));
		cpu_.a = alu_.add(rd8(wWX), WX_OFS);
		wr8(rWX, cpu_.a);

		cpu_.b = BANK_Func_1d8c2c;
		push_bank_switch();
		gb_call_ret(gb_, ADDR_Func_1d8c2c, (uint16_t)(base + 0x2a));
		pop_bank();

		SET_HL(gb_, wPalConfig1);
		cpu_.a = alu_.and_(rd8(wPalConfig1), rd8(wPalConfig1));
		if (cpu_.a) {
			cpu_.c = cpu_.a;
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
			wr8(wPalConfig1, 0);
			SET_HL(gb_, wPalConfig1 + 1);
			ld_a_hli();
			ldh_c();
			cpu_.c = alu_.inc(cpu_.c);
			ld_a_hli();
			cpu_.b = cpu_.a;
			ld_a_hli();
			cpu_.l = rd8(REG_HL(gb_));
			cpu_.h = cpu_.a;
			do {
				// .asm_80c48
				for (int i = 0; i < 8; i++) {
					ld_a_hli();
					ldh_c();
				}
				cpu_.b = alu_.dec(cpu_.b);
			} while (cpu_.b);
		}

		// .asm_80c5b
		cpu_.a = rd8(wHDMABank);
		cpu_.b = cpu_.a;
		push_bank_switch();
		SET_HL(gb_, wHDMA);
		cpu_.a = alu_.and_(rd8(wHDMA), rd8(wHDMA));
		if (cpu_.a) {
			cpu_.b = rd8(wHDMA);
			start_hdma();
		}

		// .asm_80c84
		SET_HL(gb_, w2d0b5);
		cpu_.a = alu_.and_(rd8(w2d0b5), rd8(w2d0b5));
		if (cpu_.a) {
			cpu_.b = cpu_.a;
			cpu_.a = 0x01; // BANK("VRAM1")
			wr8(rVBK, cpu_.a);
			start_hdma();
		}

		// .asm_80ca2
		pop_bank();
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		JUMP_TO(hTransferVirtualOAM);
	}

	// ld c, LOW(rVDMA_SRC_HIGH), then a 15-row HDMA from src to dest
	void hdma_15_rows(uint16_t src, uint16_t dest)
	{
		cpu_.c = (uint8_t)rVDMA_SRC_HIGH;
		cpu_.a = (uint8_t)(src >> 8);
		ldh_c();
		cpu_.c = alu_.inc(cpu_.c);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		ldh_c();
		cpu_.c = alu_.inc(cpu_.c);
		cpu_.a = (uint8_t)((dest >> 8) - 0x80);
		ldh_c();
		cpu_.c = alu_.inc(cpu_.c);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		ldh_c();
		cpu_.c = alu_.inc(cpu_.c);
		cpu_.a = 15 * 2 - 1; // 15 dma_rows
		ldh_c();
	}

	// VBlank_OWScene.Func, as copied to wVBlankFunc
	void port_VBlank_OWScene__Func()
	{
		uint16_t base = cpu_.pc;
		cpu_.a = 0x02;
		wr8(rWBK, cpu_.a);
		wr8(rSCY, rd8(wSCY));
		wr8(rSCX, rd8(wSCX));

		cpu_.a = alu_.and_(rd8(wOWPendingTileUpdate), rd8(wOWPendingTileUpdate));
		if (cpu_.a) {
			hdma_15_rows(wTilemap, v0BGMap0);
			cpu_.a = 0x01; // BANK("VRAM1")
			wr8(rVBK, cpu_.a);
			hdma_15_rows(wAttrmap, v1BGMap0);
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
			wr8(wOWPendingTileUpdate, 0);
		}

		// .skip_update_tiles
		cpu_.a = alu_.and_(rd8(wHDMABank), rd8(wHDMABank));
		if (cpu_.a) {
			cpu_.b = cpu_.a;
			push_bank_switch();
			gb_call_ret(gb_, ADDR_Func_3c03, (uint16_t)(base + 0x57));
			pop_bank();
		}

		// .asm_80d1b
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
		cpu_.b = BANK_Func_1d8c2c;
		push_bank_switch();
		gb_call_ret(gb_, ADDR_Func_1d8c2c, (uint16_t)(base + 0x71));
		pop_bank();

		cpu_.b = BANK_ApplyPalConfig1;
		push_bank_switch();
		cpu_.a = alu_.and_(rd8(wPalConfig1Register), rd8(wPalConfig1Register));
		if (cpu_.a)
			gb_call_ret(gb_, ADDR_ApplyPalConfig1, (uint16_t)(base + 0x8c));
		cpu_.a = alu_.and_(rd8(wPalConfig2), rd8(wPalConfig2));
		if (cpu_.a)
			gb_call_ret(gb_, ADDR_ApplyPalConfig2, (uint16_t)(base + 0x93));
		pop_bank();

		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		JUMP_TO(hTransferVirtualOAM);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldVblank;

PORT_EX(VBlank_Overworld, 0, 0) { OverworldVblank(gb).port_VBlank_Overworld(); }
PORT_EX(VBlank_OWScene, 0, 0) { OverworldVblank(gb).port_VBlank_OWScene(); }
PORT_LOCAL_EX(VBlank_Overworld, Func, HOOK_MANUAL_RET, 0) { OverworldVblank(gb).port_VBlank_Overworld__Func(); }
PORT_LOCAL_EX(VBlank_OWScene, Func, HOOK_MANUAL_RET, 0) { OverworldVblank(gb).port_VBlank_OWScene__Func(); }
