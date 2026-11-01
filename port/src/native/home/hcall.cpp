// home/hcall (src/home/hcall.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeHcall : public AsmModule {
public:
	using AsmModule::AsmModule;

	#define FUNC (ADDR_InitHRAMCallFunc + 0x0c) // .Func
	#define FUNC_SIZE 0x17                      // .end - .Func

	// copies hCallFunc's code to HRAM
	void port_InitHRAMCallFunc()
	{
		SET_HL(gb_, FUNC);
		SET_DE(gb_, hCallFunc);
		cpu_.b = FUNC_SIZE;
		CALL(CopyHLToDE);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeHcall;

PORT_EX(InitHRAMCallFunc, 0, 0) { HomeHcall(gb).port_InitHRAMCallFunc(); }
