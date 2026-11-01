// overworld/bank76 (src/engine/overworld/bank76.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::overworld {

namespace {

class OverworldBank76 : public AsmModule {
public:
	using AsmModule::AsmModule;

	// calls (the animations are C++, src/native/overworld/animations.cpp)

	// Writes the five VDMA registers from rVDMA_SRC_HIGH on, as the asm does
	// with ld [$ff00+c], a / inc c; the write to rVDMA_LEN starts the transfer.
	void vdma(const uint8_t regs[5])
	{
		cpu_.c = (uint8_t)rVDMA_SRC_HIGH;
		for (int i = 0; i < 5; i++) {
			if (i)
				cpu_.c = alu_.inc(cpu_.c);
			cpu_.a = regs[i];
			wr8((uint16_t)(0xff00 | cpu_.c), cpu_.a);
		}
	}

	void port_Func_1d8c2c()
	{
		cpu_.a = 1; // BANK("VRAM1")
		wr8(rVBK, 1);
		const uint16_t src = wTilemap + 0x24 * TILE_SIZE;
		const uint8_t tilemap[5] = {(uint8_t)(src >> 8), (uint8_t)src, 0x17, 0x00, 0x0b};
		vdma(tilemap);

		cpu_.a = rd8(w2d044 + 0);
		alu_.and_(cpu_.a, cpu_.a);
		if (!(cpu_.f & FLAG_Z)) {
			const uint8_t regs[5] = {cpu_.a, rd8(w2d044 + 1), 0x0e, 0xc0, 0x13};
			vdma(regs);
		}
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
	}
};

} // namespace

} // namespace wl3::overworld

using wl3::overworld::OverworldBank76;

PORT_EX(Func_1d8c2c, 0, 0) { OverworldBank76(gb).port_Func_1d8c2c(); }
