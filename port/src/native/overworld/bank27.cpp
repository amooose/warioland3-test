// overworld/bank27 (src/engine/overworld/bank27.asm): what stays
// register-level; the cutscenes are C++ (src/native/cutscenes), their
// loaders in cutscenes/loaders.cpp. Converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldBank27 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// the Prologue, still register-level (bank2b_1/2)
	void port_PrologueFunc()
	{
		FARCALL(Prologue);
	}

	// called by register-level code too (cutscene/init.cpp)
	void port_VBlank_Cutscene()
	{
		SET_HL(gb_, ADDR_VBlank_Cutscene + 0x0c); // .Func
		SET_DE(gb_, wVBlankFunc);
		cpu_.b = 0x51; // .end - .Func
		CALL(CopyHLToDE);
	}

	// VBlank_Cutscene.Func, run from wVBlankFunc:
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

	// VBlank_Cutscene.Func, as copied to wVBlankFunc (see ram_code.c). Its calls
	// return to base + n, the block's start wherever it runs from.
	void port_VBlank_Cutscene__Func()
	{
		uint16_t base = cpu_.pc;
		vbf_wram2_scroll();
		wr8(rWY, rd8(wCutsceneWY));
		cpu_.a = rd8(wCutsceneWX);
		wr8(rWX, cpu_.a);

		vbf_flip_lcdc();

		// .skip_lcd_flip
		cpu_.b = BANK_ApplyPalConfig1;
		vbf_push_bank_switch();
		cpu_.a = alu_.and_(rd8(wPalConfig1Register), rd8(wPalConfig1Register));
		if (cpu_.a)
			gb_call_ret(gb_, ADDR_ApplyPalConfig1, (uint16_t)(base + 0x3b));
		cpu_.a = alu_.and_(rd8(wPalConfig2), rd8(wPalConfig2));
		if (cpu_.a)
			gb_call_ret(gb_, ADDR_ApplyPalConfig2, (uint16_t)(base + 0x42));
		vbf_pop_bank();

		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		JUMP_TO(hTransferVirtualOAM);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldBank27;

PORT_EX(PrologueFunc, 0, 0) { OverworldBank27(gb).port_PrologueFunc(); }
PORT_EX(VBlank_Cutscene, 0, 0) { OverworldBank27(gb).port_VBlank_Cutscene(); }
PORT_LOCAL_EX(VBlank_Cutscene, Func, HOOK_MANUAL_RET, 0) { OverworldBank27(gb).port_VBlank_Cutscene__Func(); }
