// credits/credits (src/engine/credits/credits.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::credits {

namespace {

class CreditsCredits : public AsmModule {
public:
	using AsmModule::AsmModule;

	// C++, src/native/screens/credits.cpp)

	// VBlank_160439.Func, as copied to wVBlankFunc (see ram_code.c). Its calls
	// return to base + n, the block's start wherever it runs from.
	void port_VBlank_160439__Func()
	{
		uint16_t base = cpu_.pc;
		cpu_.a = rd8(wSCY);
		wr8(rSCY, cpu_.a);
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
		cpu_.a = rd8(wcee4);
		if (!alu_.bit(cpu_.a, 7))
			goto dma_transfer;
		cpu_.a = alu_.and_(cpu_.a, 0x7f);
		cpu_.a = alu_.dec(cpu_.a);
		if (cpu_.f & FLAG_Z) {
			// .write_text
			cpu_.a = rd8(wCreditsBGMapPtr);
			cpu_.e = rd8(wCreditsBGMapPtr + 1);
			cpu_.d = cpu_.a;
			uint16_t hl = wCreditsTextBuffer;
			for (;;) {
				cpu_.a = rd8(hl++);
				alu_.cp(cpu_.a, 0x7f); // '\n'
				if (cpu_.f & FLAG_Z)
					break;
				wr8(REG_DE(gb_), cpu_.a);
				cpu_.e = alu_.inc(cpu_.e);
			}
			SET_HL(gb_, hl);
			goto clear_pending;
		}
		cpu_.a = alu_.dec(cpu_.a);
		if (cpu_.f & FLAG_Z) {
			// .asm_160499
			gb_call_ret(gb_, ADDR_ApplyTempPals1ToBGPals, (uint16_t)(base + 0x57));
			gb_call_ret(gb_, ADDR_ApplyTempPals2ToOBPals, (uint16_t)(base + 0x5a));
			SET_HL(gb_, rLCDC);
			cpu_.a = alu_.or_(rd8(rLCDC), LCDC_WIN_ON | LCDC_WIN_9C00);
			wr8(rLCDC, cpu_.a);
			goto clear_pending;
		}
		cpu_.a = 1; // BANK("VRAM1")
		wr8(rVBK, cpu_.a);
		cpu_.a = rd8(wCreditsBGMapPtr);
		cpu_.e = rd8(wCreditsBGMapPtr + 1);
		cpu_.d = cpu_.a;
		SET_HL(gb_, wCreditsTextPals);
		cpu_.b = CREDITS_TEXT_LENGTH;
		gb_call_ret(gb_, ADDR_CopyHLToDE, (uint16_t)(base + 0x2b));
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, cpu_.a);
		cpu_.a = rd8(wCreditsBGMapPtr);
		cpu_.l = rd8(wCreditsBGMapPtr + 1);
		cpu_.h = cpu_.a;
		cpu_.a = 0x7e;
		cpu_.b = CREDITS_TEXT_LENGTH;
		gb_call_ret(gb_, ADDR_WriteAToHL_BTimes, (uint16_t)(base + 0x3b));
		cpu_.a = 0x01 | (1 << 7);
		wr8(wcee4, cpu_.a);
		goto dma_transfer;
	clear_pending:
		// .asm_1604a6
		SET_HL(gb_, wcee4);
		wr8(wcee4, (uint8_t)(rd8(wcee4) & 0x7f));
	dma_transfer:
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		gb_call_ret(gb_, hTransferVirtualOAM, (uint16_t)(base + 0x6b));
		RET();
	}
};

} // namespace

} // namespace wl3::credits

using wl3::credits::CreditsCredits;

PORT_LOCAL_EX(VBlank_160439, Func, HOOK_MANUAL_RET, 0) { CreditsCredits(gb).port_VBlank_160439__Func(); }
