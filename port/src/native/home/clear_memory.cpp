// home/clear_memory (src/home/clear_memory.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeClearMemory : public AsmModule {
public:
	using AsmModule::AsmModule;

	// clears whole wVirtualOAM
	void port_ClearVirtualOAM()
	{
		SET_HL(gb_, wVirtualOAM);
		cpu_.b = OAM_SIZE;
		cpu_.a = 0;
		CALL(WriteAToHL_BTimes);
		wr8(wVirtualOAMByteSize, 0);
		cpu_.f = FLAG_Z; // xor a
	}

	// fills v0BGMap0 and v1BGMap0 with byte in a
	void port_FillBGMap0()
	{
		cpu_.d = cpu_.a;
		wr8(rVBK, BANK_v1BGMap0);
		SET_HL(gb_, v1BGMap0);
		SET_BC(gb_, v1BGMap1 - v1BGMap0);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		CALL(WriteAToHL_BCTimes);

		wr8(rVBK, cpu_.a); // a is still 0: VRAM0
		cpu_.a = cpu_.d;
		SET_HL(gb_, v0BGMap0);
		SET_BC(gb_, v0BGMap1 - v0BGMap0);
		CALL(WriteAToHL_BCTimes);
	}

	// clears unused sprites that are in wVirtualOAM
	void port_ClearUnusedVirtualOAM()
	{
		uint8_t used = rd8(wVirtualOAMByteSize);
		cpu_.l = used;
		cpu_.h = (uint8_t)(wVirtualOAM >> 8);
		if (used < OAM_SIZE) {
			// one 4-byte OAM struct per iteration. As in the asm, a remainder
			// under 4 would make b = 0 and loop 256 times; it never happens
			// because sprites are always added 4 bytes at a time.
			cpu_.b = (uint8_t)((OAM_SIZE - used) >> 2);
			int n = cpu_.b ? cpu_.b : 256;
			uint16_t hl = REG_HL(gb_);
			for (int i = 0; i < n * 4; i++)
				wr8(hl++, 0);
			SET_HL(gb_, hl);
			cpu_.b = 0;
		}
		cpu_.a = 0;
		wr8(wVirtualOAMByteSize, 0);
		cpu_.f = FLAG_Z; // xor a
	}

	// fills v0BGMap0 and v1BGMap0 with $7f
	void port_FillBGMap0_With7f()
	{
		cpu_.a = 0x7f;
		JUMP(FillBGMap0); // fallthrough
	}

	void port_ClearBGMap1()
	{
		cpu_.a = 0xff;
		SET_HL(gb_, v0BGMap1);
		SET_BC(gb_, v0End - v0BGMap1);
		CALL(WriteAToHL_BCTimes);
	}

	void port_ClearBGMap0()
	{
		cpu_.a = 0xff;
		JUMP(FillBGMap0); // jr
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeClearMemory;

PORT_EX(ClearVirtualOAM, 0, 0) { HomeClearMemory(gb).port_ClearVirtualOAM(); }
PORT_EX(FillBGMap0, 0, 0) { HomeClearMemory(gb).port_FillBGMap0(); }
PORT_EX(ClearUnusedVirtualOAM, 0, 0) { HomeClearMemory(gb).port_ClearUnusedVirtualOAM(); }
PORT_EX(FillBGMap0_With7f, HOOK_MANUAL_RET, 0) { HomeClearMemory(gb).port_FillBGMap0_With7f(); }
PORT_EX(ClearBGMap1, 0, 0) { HomeClearMemory(gb).port_ClearBGMap1(); }
PORT_EX(ClearBGMap0, HOOK_MANUAL_RET, 0) { HomeClearMemory(gb).port_ClearBGMap0(); }
