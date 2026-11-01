// home/clear_ram (src/home/clear_ram.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeClearRam : public AsmModule {
public:
	using AsmModule::AsmModule;

	// clear VRAM if hCGB != 0
	void port_ClearVRAM()
	{
		cpu_.a = rd8(hCGB);
		cpu_.a = alu_.and_(cpu_.a, cpu_.a);
		if (!cpu_.a) {
			RET();
			return;
		}
		cpu_.a = BANK_v1Tiles0;
		wr8(rVBK, cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z;
		SET_HL(gb_, 0x8000);
		SET_BC(gb_, 0x2000);
		CALL(WriteAToHL_BCTimes);

		cpu_.a = 0;
		cpu_.f = FLAG_Z;
		wr8(rVBK, 0);
		SET_HL(gb_, 0x8000);
		SET_BC(gb_, 0x2000);
		JUMP(WriteAToHL_BCTimes);
	}

	void port_ClearWRAM()
	{
		cpu_.e = 1; // WRAM1
		do {
			cpu_.a = cpu_.e;
			wr8(rWBK, cpu_.a);
			cpu_.a = 0;
			cpu_.f = FLAG_Z;
			SET_HL(gb_, 0xd000);
			SET_BC(gb_, 0x1000);
			CALL(WriteAToHL_BCTimes);
			cpu_.e = alu_.inc(cpu_.e);
		} while (!alu_.bit(cpu_.e, 3));
		cpu_.a = 0;
		cpu_.f = FLAG_Z;
		wr8(rWBK, 0);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeClearRam;

PORT_EX(ClearVRAM, HOOK_MANUAL_RET, 0) { HomeClearRam(gb).port_ClearVRAM(); }
PORT_EX(ClearWRAM, 0, 0) { HomeClearRam(gb).port_ClearWRAM(); }
