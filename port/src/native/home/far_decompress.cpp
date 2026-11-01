// home/far_decompress (src/home/far_decompress.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeFarDecompress : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_FarCopyHLToDE_BC2()
	{
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = rd8(wTempBank);
		bankswitch(cpu_.a);
		CALL(CopyHLToDE_BC);
		pop_af();
		bankswitch(cpu_.a);
	}

	void port_FarDecompress()
	{
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = rd8(wTempBank);
		bankswitch(cpu_.a);
		CALL(Decompress);
		pop_af();
		bankswitch(cpu_.a);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeFarDecompress;

PORT_EX(FarCopyHLToDE_BC2, 0, 0) { HomeFarDecompress(gb).port_FarCopyHLToDE_BC2(); }
PORT_EX(FarDecompress, 0, 0) { HomeFarDecompress(gb).port_FarDecompress(); }
