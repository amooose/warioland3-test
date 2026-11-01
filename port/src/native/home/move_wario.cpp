// home/move_wario (src/home/move_wario.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeMoveWario : public AsmModule {
public:
	using AsmModule::AsmModule;

	// Adds (or subtracts) b to a 16-bit big-endian position: the low byte with
	// add/sub, then the carry into the high byte with adc/sbc 0.
	void add_to_pos(uint16_t pos, bool subtract)
	{
		uint8_t lo = rd8(pos + 1);
		lo = subtract ? alu_.sub(lo, cpu_.b) : alu_.add(lo, cpu_.b);
		wr8(pos + 1, lo);
		uint8_t hi = rd8(pos);
		hi = subtract ? alu_.sbc(hi, 0) : alu_.adc(hi, 0);
		wr8(pos, hi);
		cpu_.a = hi;
	}

	void move_camera(uint16_t delta, bool subtract)
	{
		uint8_t v = rd8(delta);
		v = subtract ? alu_.sub(v, cpu_.b) : alu_.add(v, cpu_.b);
		wr8(delta, v);
	}

	void port_MoveWarioRight()
	{
		move_camera(wCameraXDelta, false);
		add_to_pos(wWarioXPos, false);
	}

	void port_MoveWarioLeft()
	{
		move_camera(wCameraXDelta, true);
		add_to_pos(wWarioXPos, true);
	}

	// b = y offset
	void port_MoveWarioDown()
	{
		move_camera(wCameraYDelta, false);
		add_to_pos(wWarioYPos, false);
	}

	void port_MoveWarioDown_WithoutCameraOffset()
	{
		add_to_pos(wWarioYPos, false);
	}

	// b = y offset
	void port_MoveWarioUp()
	{
		move_camera(wCameraYDelta, true);
		add_to_pos(wWarioYPos, true);
	}

	void port_MoveWarioUp_WithoutCameraOffset()
	{
		add_to_pos(wWarioYPos, true);
	}

	void port_ClearParticles()
	{
		SET_HL(gb_, wParticles);
		cpu_.b = PARTICLE_STRUCT_LENGTH * MAX_NUM_PARTICLES + 2;
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		CALL(WriteAToHL_BTimes);
	}

	// b = ground shake counter
	void port_DoGroundShake()
	{
		cpu_.a = rd8(wGroundShakeCounter);
		if (alu_.and_(cpu_.a, cpu_.a))
			return;
		cpu_.a = cpu_.b;
		wr8(wGroundShakeCounter, cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wIsWarioGroundShaking, 0);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeMoveWario;

PORT_EX(MoveWarioRight, 0, 0) { HomeMoveWario(gb).port_MoveWarioRight(); }
PORT_EX(MoveWarioLeft, 0, 0) { HomeMoveWario(gb).port_MoveWarioLeft(); }
PORT_EX(MoveWarioDown, 0, 0) { HomeMoveWario(gb).port_MoveWarioDown(); }
PORT_EX(MoveWarioDown_WithoutCameraOffset, 0, 0) { HomeMoveWario(gb).port_MoveWarioDown_WithoutCameraOffset(); }
PORT_EX(MoveWarioUp, 0, 0) { HomeMoveWario(gb).port_MoveWarioUp(); }
PORT_EX(MoveWarioUp_WithoutCameraOffset, 0, 0) { HomeMoveWario(gb).port_MoveWarioUp_WithoutCameraOffset(); }
PORT_EX(ClearParticles, 0, 0) { HomeMoveWario(gb).port_ClearParticles(); }
PORT_EX(DoGroundShake, 0, 0) { HomeMoveWario(gb).port_DoGroundShake(); }
