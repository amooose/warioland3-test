// overworld/bank2b_1 (src/engine/overworld/bank2b_1.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldBank2b1 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// the V-Blank functions copied to wVBlankFunc. The rest is C++
	// (src/native/overworld: SceneCommon, SceneWario, SceneSprites, the scenes).

	// ld hl, counter / xor a / ld [hld], a / inc [hl]
	void reset_and_advance(uint16_t counter)
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(counter, 0);
		SET_HL(gb_, counter - 1);
		wr8(counter - 1, alu_.inc(rd8(counter - 1)));
	}

	// c = frame to wait for
	void wait_and_advance(uint16_t counter)
	{
		cpu_.a = rd8(counter);
		alu_.cp(cpu_.a, cpu_.c);
		if (cpu_.f & FLAG_C)
			return;
		reset_and_advance(counter);
	}

	void port_Func_ad330() { wait_and_advance(w2d891); }
	void port_Func_ad335() { reset_and_advance(w2d891); }
	void port_Func_ad011() { wait_and_advance(w2d014); }
	void port_Func_ad016() { reset_and_advance(w2d014); }

	// unreferenced
	void port_Func_aca63()
	{
		SET_HL(gb_, wOWUIObj1XCoord);
		wr8(wOWUIObj1XCoord, alu_.dec(rd8(wOWUIObj1XCoord)));
	}

	void port_Func_aca68()
	{
		SET_HL(gb_, wOWUIObj1XCoord);
		wr8(wOWUIObj1XCoord, alu_.inc(rd8(wOWUIObj1XCoord)));
	}

	// unreferenced
	void port_DebugSceneWario()
	{
		cpu_.a = rd8(wJoypadPressed);
		cpu_.b = cpu_.a;
		cpu_.a = rd8(wDebugSceneWarioState);
		if (alu_.bit(cpu_.b, B_PAD_RIGHT)) {
			alu_.cp(cpu_.a, NUM_SCENEWARIO_STATES - 1);
			if (cpu_.f & FLAG_C)
				cpu_.a = alu_.inc(cpu_.a);
		} else {
			if (!alu_.bit(cpu_.b, B_PAD_LEFT)) {
				RET();
				return;
			}
			uint8_t a = alu_.and_(cpu_.a, cpu_.a);
			if (a)
				cpu_.a = alu_.dec(cpu_.a);
		}
		wr8(wDebugSceneWarioState, cpu_.a);
		JUMP_TO(ADDR_SetSceneWarioState); // fallthrough
	}

	// unreferenced
	void port_Func_ad329()
	{
		cpu_.a = rd8(wSceneWarioState);
		alu_.cp(cpu_.a, cpu_.b);
		if (!(cpu_.f & FLAG_Z)) {
			RET();
			return;
		}
		JUMP_TO(ADDR_Func_ad335);
	}

	void port_Func_ad33c()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(w2d890, 0);
		wr8(w2d891, 0);
		SET_HL(gb_, w2d891);
		JUMP_TO(ADDR_Func_ad016);
	}

	// ld [$ff00+c], a
	void vbf_ldh_c()
	{
		wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
	}

	// ld a, [hli]
	void vbf_ld_a_hli()
	{
		uint16_t hl = REG_HL(gb_);
		cpu_.a = rd8(hl);
		SET_HL(gb_, (uint16_t)(hl + 1));
	}

	// ld a, [wROMBank] / push af / ld a, b / bankswitch
	void vbf_push_bank_switch()
	{
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = cpu_.b;
		bankswitch(cpu_.a);
	}

	// pop af / bankswitch
	void vbf_pop_bank()
	{
		pop_af();
		bankswitch(cpu_.a);
	}

	// ld a, [wLCDCFlagsToFlip] / and a / jr z, .skip / ld hl, rLCDC / xor [hl] /
	// ld [hl], a / xor a / ld [wLCDCFlagsToFlip], a
	void vbf_flip_lcdc()
	{
		cpu_.a = rd8(wLCDCFlagsToFlip);
		cpu_.a = alu_.and_(cpu_.a, cpu_.a);
		if (!cpu_.a)
			return;
		SET_HL(gb_, rLCDC);
		cpu_.a = alu_.xor_(cpu_.a, rd8(rLCDC));
		wr8(rLCDC, cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wLCDCFlagsToFlip, 0);
	}

	// ld a, $02 / ldh [rWBK], a / apply wSCY and wSCX
	void vbf_wram2_scroll()
	{
		cpu_.a = 0x02; // BANK("WRAM2")
		wr8(rWBK, cpu_.a);
		wr8(rSCY, rd8(wSCY));
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
	}

	// VBlank_PrologueEpilogue.Func, as copied to wVBlankFunc (see ram_code.c).
	// Its calls return to base + n, the block's start wherever it runs from.
	void port_VBlank_PrologueEpilogue__Func()
	{
		uint16_t base = cpu_.pc;
		vbf_wram2_scroll();

		cpu_.a = alu_.and_(rd8(wHDMABank), rd8(wHDMABank));
		if (cpu_.a) {
			cpu_.b = cpu_.a;
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
			wr8(wHDMABank, 0);
			vbf_push_bank_switch();
			cpu_.a = rd8(wHDMADestVRAMBank);
			wr8(rVBK, cpu_.a);
			SET_HL(gb_, wHDMA);
			vbf_ld_a_hli();
			cpu_.c = (uint8_t)rVDMA_SRC_HIGH;
			vbf_ldh_c();
			for (int i = 0; i < 4; i++) {
				vbf_ld_a_hli();
				cpu_.c = alu_.inc(cpu_.c);
				vbf_ldh_c();
			}
			vbf_pop_bank();
		}

		// .skip_hdma_config
		SET_HL(gb_, wPendingCharDest);
		cpu_.a = alu_.and_(rd8(wPendingCharDest), rd8(wPendingCharDest));
		if (cpu_.a) {
			cpu_.e = cpu_.a;
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
			wr8(wPendingCharDest, 0);
			SET_HL(gb_, wPendingCharDest + 1);
			vbf_ld_a_hli();
			cpu_.d = cpu_.a;
			vbf_ld_a_hli();
			wr8(REG_DE(gb_), cpu_.a); // wPendingCharTile
			cpu_.a = 0x01; // BANK("VRAM1")
			wr8(rVBK, cpu_.a);
			cpu_.a = rd8(REG_HL(gb_)); // wPendingCharAttr
			wr8(REG_DE(gb_), cpu_.a);
		}

		// .skip_char
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
		SET_HL(gb_, wPalConfig1);
		cpu_.a = alu_.and_(rd8(wPalConfig1), rd8(wPalConfig1));
		if (cpu_.a)
			gb_call_ret(gb_, ADDR_ApplyPalConfig, (uint16_t)(base + 0x62));

		SET_HL(gb_, wPalConfig2);
		cpu_.a = alu_.and_(rd8(wPalConfig2), rd8(wPalConfig2));
		if (cpu_.a)
			gb_call_ret(gb_, ADDR_ApplyPalConfig, (uint16_t)(base + 0x6a));

		vbf_flip_lcdc();

		// .dma_transfer
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		JUMP_TO(hTransferVirtualOAM);
	}

	// _GolfBuilding.func, as copied to wVBlankFunc (see ram_code.c): starts the
	// HDMA transfer in wHDMA, if there's one
	void port__GolfBuilding__func()
	{
		cpu_.a = 2; // BANK("WRAM2")
		wr8(rWBK, cpu_.a);
		cpu_.a = rd8(wSCY);
		wr8(rSCY, cpu_.a);
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
		cpu_.a = rd8(wHDMABank);
		if (alu_.and_(cpu_.a, cpu_.a)) {
			cpu_.b = cpu_.a;
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
			wr8(wHDMABank, 0);
			cpu_.a = rd8(wROMBank);
			push_af();
			cpu_.a = cpu_.b;
			bankswitch(cpu_.a);
			uint16_t hl = wHDMA;
			cpu_.c = (uint8_t)rVDMA_SRC_HIGH;
			for (int i = 0; i < 5; i++) {
				if (i)
					cpu_.c = alu_.inc(cpu_.c);
				cpu_.a = rd8(hl++);
				wr8((uint16_t)(0xff00 | cpu_.c), cpu_.a);
			}
			SET_HL(gb_, hl);
			pop_af();
			bankswitch(cpu_.a);
		}
		// .done
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		JUMP_TO(hTransferVirtualOAM);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldBank2b1;

PORT_EX(Func_ad330, 0, 0) { OverworldBank2b1(gb).port_Func_ad330(); }
PORT_EX(Func_ad335, 0, 0) { OverworldBank2b1(gb).port_Func_ad335(); }
PORT_EX(Func_ad011, 0, 0) { OverworldBank2b1(gb).port_Func_ad011(); }
PORT_EX(Func_ad016, 0, 0) { OverworldBank2b1(gb).port_Func_ad016(); }
PORT_EX(Func_aca63, 0, 0) { OverworldBank2b1(gb).port_Func_aca63(); }
PORT_EX(Func_aca68, 0, 0) { OverworldBank2b1(gb).port_Func_aca68(); }
PORT_EX(DebugSceneWario, HOOK_MANUAL_RET, 0) { OverworldBank2b1(gb).port_DebugSceneWario(); }
PORT_EX(Func_ad329, HOOK_MANUAL_RET, 0) { OverworldBank2b1(gb).port_Func_ad329(); }
PORT_EX(Func_ad33c, HOOK_MANUAL_RET, 0) { OverworldBank2b1(gb).port_Func_ad33c(); }
PORT_LOCAL_EX(VBlank_PrologueEpilogue, Func, HOOK_MANUAL_RET, 0) { OverworldBank2b1(gb).port_VBlank_PrologueEpilogue__Func(); }
PORT_LOCAL_EX(_GolfBuilding, func, HOOK_MANUAL_RET, 0) { OverworldBank2b1(gb).port__GolfBuilding__func(); }
