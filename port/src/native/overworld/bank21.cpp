// overworld/bank21 (src/engine/overworld/bank21.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldBank21 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// xor a / ld hl, start / ld bc, size / call WriteAToHL_BCTimes
	void clear_bytes(uint16_t start, uint16_t size)
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		SET_HL(gb_, start);
		SET_BC(gb_, size);
		CALL(WriteAToHL_BCTimes);
	}

	void port_Func_84fd5()
	{
		clear_bytes(wTempBGPals, 8 * PAL_SIZE);
		clear_bytes(wTempOBPals, 8 * PAL_SIZE);
	}

	// hl = palette config: applies its palettes and clears its register
	void port_ApplyPalConfig_Bank21()
	{
		uint16_t hl = REG_HL(gb_);
		cpu_.c = rd8(hl);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(hl++, 0);
		cpu_.a = rd8(hl++);
		wr8((uint16_t)(0xff00 | cpu_.c), cpu_.a);
		cpu_.c = alu_.inc(cpu_.c);
		cpu_.a = rd8(hl++);
		cpu_.b = cpu_.a;
		cpu_.a = rd8(hl++);
		cpu_.l = rd8(hl);
		cpu_.h = cpu_.a;
		hl = REG_HL(gb_);
		do {
			for (int i = 0; i < 8; i++) {
				cpu_.a = rd8(hl++);
				wr8((uint16_t)(0xff00 | cpu_.c), cpu_.a);
			}
			cpu_.b = alu_.dec(cpu_.b);
		} while (cpu_.b);
		SET_HL(gb_, hl);
	}

	void port_ApplyPalConfig1()
	{
		SET_HL(gb_, wPalConfig1);
		JUMP_TO(ADDR_ApplyPalConfig_Bank21); // fallthrough
	}

	void port_ApplyPalConfig2()
	{
		SET_HL(gb_, wPalConfig2);
		JUMP_TO(ADDR_ApplyPalConfig_Bank21);
	}

	#define DNT_LOAD_BASE_AND_TARGET_PALS (ADDR_DayNightTransition + 0x2e)
	#define DNT_ADVANCE_TABLE (ADDR_DayNightTransition + 0x47)
	#define DNT_FADE_COLOURS (ADDR_DayNightTransition + 0x57)
	#define DNT_LOAD_PAL1 (ADDR_DayNightTransition + 0xdc)
	#define DNT_LOAD_PAL2 (ADDR_DayNightTransition + 0xf2)


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


	// ld a, $02 / ldh [rWBK], a / apply wSCY and wSCX
	void vbf_wram2_scroll()
	{
		cpu_.a = 0x02; // BANK("WRAM2")
		wr8(rWBK, cpu_.a);
		wr8(rSCY, rd8(wSCY));
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
	}

	// VBlank_OWPalTransition.Func, as copied to wVBlankFunc (see ram_code.c).
	// Its calls return to base + n, the block's start wherever it runs from.
	void port_VBlank_OWPalTransition__Func()
	{
		uint16_t base = cpu_.pc;
		vbf_wram2_scroll();
		wr8(rWY, rd8(wWY));
		cpu_.a = alu_.add(rd8(wWX), WX_OFS);
		wr8(rWX, cpu_.a);

		cpu_.b = BANK_Func_1d8c2c;
		vbf_push_bank_switch();
		gb_call_ret(gb_, ADDR_Func_1d8c2c, (uint16_t)(base + 0x2a));
		vbf_pop_bank();

		SET_HL(gb_, wTempBGPals);
		cpu_.c = (uint8_t)rBGPI;
		cpu_.a = BGPI_AUTOINC | 0; // palette 0
		vbf_ldh_c();
		cpu_.c = alu_.inc(cpu_.c);
		cpu_.b = 8; // num pals
		do {
			// .loop_pals
			for (int i = 0; i < 8; i++) {
				vbf_ld_a_hli();
				vbf_ldh_c();
			}
			cpu_.b = alu_.dec(cpu_.b);
		} while (cpu_.b);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		JUMP_TO(hTransferVirtualOAM);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldBank21;

PORT_EX(Func_84fd5, 0, 0) { OverworldBank21(gb).port_Func_84fd5(); }
PORT_EX(ApplyPalConfig_Bank21, 0, 0) { OverworldBank21(gb).port_ApplyPalConfig_Bank21(); }
PORT_EX(ApplyPalConfig1, HOOK_MANUAL_RET, 0) { OverworldBank21(gb).port_ApplyPalConfig1(); }
PORT_EX(ApplyPalConfig2, HOOK_MANUAL_RET, 0) { OverworldBank21(gb).port_ApplyPalConfig2(); }
PORT_LOCAL_EX(VBlank_OWPalTransition, Func, HOOK_MANUAL_RET, 0) { OverworldBank21(gb).port_VBlank_OWPalTransition__Func(); }
