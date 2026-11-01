// home/decompress (src/home/decompress.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeDecompress : public AsmModule {
public:
	using AsmModule::AsmModule;

	// decompresses data from hl and writes to bc
	// the game stores some data in compressed form, such as tile gfx and bg maps
	// this routine decompresses it by doing the following logic:
	// - if bit 7 is not set, repeat the following byte that amount of times
	// - if bit 7 is set, copy the next amount of bytes literally
	// - a 0 byte ends the data
	void port_Decompress()
	{
		uint16_t hl = REG_HL(gb_), bc = REG_BC(gb_);
		for (;;) {
			uint8_t cmd = rd8(hl++);
			if (!cmd)
				break;
			if (cmd & 0x80) {
				// copies literal x bytes; a count of 0 means 256
				int n = (cmd & 0x7f) ? (cmd & 0x7f) : 256;
				for (int i = 0; i < n; i++)
					wr8(bc++, rd8(hl++));
			} else {
				// copies byte x times
				cpu_.e = rd8(hl++);
				for (int i = 0; i < cmd; i++)
					wr8(bc++, cpu_.e);
			}
			cpu_.d = 0;
		}
		SET_HL(gb_, hl);
		SET_BC(gb_, bc);
		cpu_.a = 0;
		cpu_.f = FLAG_Z | FLAG_H; // and a
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeDecompress;

PORT_EX(Decompress, 0, 0) { HomeDecompress(gb).port_Decompress(); }
