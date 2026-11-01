// home/wario_mechanics2 (src/home/wario_mechanics2.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeWarioMechanics2 : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_OpenActionHelp()
	{
		cpu_.a = ST_ACTION_HELP;
		wr8(wState, ST_ACTION_HELP);
		wr8(wState + 1, 0); // wSubState
		SET_HL(gb_, wState + 1);
	}

	// Caps wWalkVelIndex: at $0c from $10 on the ground or while jumping up,
	// at $04 from $08 while falling.
	void port_Func_1762()
	{
		cpu_.a = rd8(wJumpVelIndex);
		alu_.cp(cpu_.a, FALLING_JUMP_VEL_INDEX);
		bool falling = !(cpu_.f & FLAG_C);
		cpu_.a = rd8(wWalkVelIndex);
		alu_.cp(cpu_.a, falling ? 0x08 : 0x10);
		if (cpu_.f & FLAG_C)
			return;
		cpu_.a = falling ? 0x04 : 0x0c;
		wr8(wWalkVelIndex, cpu_.a);
	}

	// Copies wWarioPos to hPos with x moved to the left or right edge of the
	// collision box, 3 pixels in, as the asm does it: from wWarioXPos + 1 down.
	void get_pos_at_box_edge(bool left)
	{
		if (left) {
			cpu_.a = (uint8_t)(~rd8(wCollisionBoxLeft) + 1);
			cpu_.c = alu_.sub(cpu_.a, 3);
			cpu_.a = alu_.sub(rd8(wWarioXPos + 1), cpu_.c);
			wr8(hXPosLo, cpu_.a);
			cpu_.a = alu_.sbc(rd8(wWarioXPos), 0x00);
		} else {
			cpu_.c = alu_.sub(rd8(wCollisionBoxRight), 3);
			cpu_.a = alu_.add(rd8(wWarioXPos + 1), cpu_.c);
			wr8(hXPosLo, cpu_.a);
			cpu_.a = alu_.adc(rd8(wWarioXPos), 0x00);
		}
		wr8(hXPosLo - 1, cpu_.a);
		wr8(hXPosLo - 2, rd8(wWarioYPos + 1));
		cpu_.a = rd8(wWarioYPos);
		wr8(hXPosLo - 3, cpu_.a);
		SET_HL(gb_, wWarioYPos);
		SET_DE(gb_, hXPosLo - 3);
	}

	// returns nz if standing on slippery ground
	// The farcalls push the asm's own return addresses: Func_19b51 runs
	// ProcessBlock, whose block functions may look at the stack.
	void port_IsOnSlipperyGround()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wIsOnSlipperyGround, 0);

		get_pos_at_box_edge(true);
		FARCALL_RET(Func_19b51, (uint16_t)(ADDR_IsOnSlipperyGround + 0x2f));
		cpu_.a = rd8(wIsOnSlipperyGround);
		alu_.and_(cpu_.a, cpu_.a);
		if (!(cpu_.f & FLAG_Z))
			return;

		get_pos_at_box_edge(false);
		FARCALL_RET(Func_19b51, (uint16_t)(ADDR_IsOnSlipperyGround + 0x5d));
		cpu_.a = rd8(wIsOnSlipperyGround);
		alu_.and_(cpu_.a, cpu_.a);
	}

	void port_ReleaseOwl()
	{
		// hPos = wWarioPos + (-$18, 0)
		cpu_.a = rd8(wWarioXPos + 1);
		wr8(hXPosLo, cpu_.a);
		cpu_.a = rd8(wWarioXPos + 0);
		wr8(hXPosLo - 1, cpu_.a);
		cpu_.a = alu_.sub(rd8(wWarioXPos - 1), 0x18);
		wr8(hXPosLo - 2, cpu_.a);
		cpu_.a = alu_.sbc(rd8(wWarioXPos - 2), 0x00);
		wr8(hXPosLo - 3, cpu_.a);
		SET_HL(gb_, wWarioXPos - 2);
		SET_DE(gb_, hXPosLo - 3);
		cpu_.b = PARTICLE_OWL;
		FARCALL(CreateParticle);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeWarioMechanics2;

PORT_EX(OpenActionHelp, 0, 0) { HomeWarioMechanics2(gb).port_OpenActionHelp(); }
PORT_EX(Func_1762, 0, 0) { HomeWarioMechanics2(gb).port_Func_1762(); }
PORT_EX(IsOnSlipperyGround, 0, 0) { HomeWarioMechanics2(gb).port_IsOnSlipperyGround(); }
PORT_EX(ReleaseOwl, 0, 0) { HomeWarioMechanics2(gb).port_ReleaseOwl(); }
