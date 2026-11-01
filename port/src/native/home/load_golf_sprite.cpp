// home/load_golf_sprite (src/home/load_golf_sprite.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeLoadGolfSprite : public AsmModule {
public:
	using AsmModule::AsmModule;

	// hl = source, wdc11 = destination, w1dc13 = length - 1
	void port_Func_2c30()
	{
		cpu_.a = cpu_.h;
		wr8(rVDMA_SRC_HIGH, cpu_.a);
		cpu_.a = cpu_.l;
		wr8(rVDMA_SRC_LOW, cpu_.a);
		cpu_.a = rd8(wdc11 + 0);
		wr8(rVDMA_DEST_HIGH, cpu_.a);
		cpu_.a = rd8(wdc11 + 1);
		wr8(rVDMA_DEST_LOW, cpu_.a);
		cpu_.a = rd8(w1dc13);
		wr8(rVDMA_LEN, cpu_.a);
	}

	// copies 4 BG palettes from hl to rBGPD
	void port_Func_2c46()
	{
		SET_DE(gb_, rBGPI);
		cpu_.c = 4;
		cpu_.a = 0x80;
		wr8(REG_DE(gb_), cpu_.a);
		cpu_.e++; // inc e
		uint16_t hl = REG_HL(gb_);
		uint16_t de = REG_DE(gb_);
		do {
			for (int i = 0; i < 0x10; i++) {
				cpu_.a = rd8(hl++);
				wr8(de, cpu_.a);
			}
			cpu_.c = alu_.dec(cpu_.c);
		} while (cpu_.c);
		SET_HL(gb_, hl);
	}

	// updates Golf Wario sprite, given its state
	// and which direction it is facing
	void port_UpdateGolfWarioAnimation()
	{
		cpu_.a = rd8(wGolfWarioState);
		cpu_.b = 0x00;
		cpu_.c = cpu_.a;
		uint16_t hl = alu_.add16(ADDR_UpdateGolfWarioAnimation + 0x75, REG_BC(gb_)); // .OAMBanks
		cpu_.a = rd8(hl);
		wr8(wTempBank, cpu_.a);

		cpu_.a = rd8(wGolfWarioDir);
		alu_.and_(cpu_.a, cpu_.a);
		if (!(cpu_.f & FLAG_Z)) {
			// dir left
			cpu_.a = alu_.add(cpu_.c, NUM_GOLF_WARIO_STATES);
			cpu_.c = cpu_.a;
		}
		// .dir_right
		cpu_.b = 0x00;
		cpu_.c = alu_.sla(cpu_.c);
		hl = alu_.add16(ADDR_UpdateGolfWarioAnimation + 0x81, REG_BC(gb_)); // .OAMTable
		cpu_.a = rd8(hl++);
		wr8(wGolfOAMPtr + 1, cpu_.a);
		cpu_.a = rd8(hl);
		wr8(wGolfOAMPtr + 0, cpu_.a);

		hl = alu_.add16(ADDR_UpdateGolfWarioAnimation + 0xb1, REG_BC(gb_)); // .Framesets
		cpu_.a = rd8(hl++);
		cpu_.d = rd8(hl);
		cpu_.e = cpu_.a;
		SET_HL(gb_, hl);
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = rd8(wTempBank);
		bankswitch(cpu_.a);
		SET_HL(gb_, wGolfWarioDuration);
		CALL(UpdateGolfObjectAnimation);
		cpu_.a = cpu_.c;
		wr8(wHasGolfWarioAnimationFinished, cpu_.a);
		pop_af();
		bankswitch(cpu_.a);

		cpu_.a = rd8(wHasGolfWarioAnimationFinished);
		alu_.and_(cpu_.a, cpu_.a);
		if (!(cpu_.f & FLAG_Z))
			gb_call(gb_, ADDR_HoldGolfWarioLastFrame); // whatever bank is mapped
		cpu_.a = rd8(wGolfWarioCurrentFrame);
		wr8(wGolfWarioFrame, cpu_.a);
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = rd8(wTempBank);
		bankswitch(cpu_.a);
		SET_HL(gb_, wGolfWarioSprite);
		CALL(LoadGolfSprite);
		pop_af();
		bankswitch(cpu_.a);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeLoadGolfSprite;

PORT_EX(Func_2c30, 0, 0) { HomeLoadGolfSprite(gb).port_Func_2c30(); }
PORT_EX(Func_2c46, 0, 0) { HomeLoadGolfSprite(gb).port_Func_2c46(); }
PORT_EX(UpdateGolfWarioAnimation, 0, 0) { HomeLoadGolfSprite(gb).port_UpdateGolfWarioAnimation(); }
