// Home's Wario primitives; see wario.hpp.
#include "wario.hpp"

#include "../flags.hpp"
#include "memory.hpp"

#include <initializer_list>

namespace wl3::home {

namespace {

// a big-endian 16-bit position, moved by delta
void move_pos(Memory mem, uint16_t pos, int delta)
{
	uint16_t v = static_cast<uint16_t>(mem[pos] << 8 | mem[static_cast<uint16_t>(pos + 1)]);
	v = static_cast<uint16_t>(v + delta);
	mem[static_cast<uint16_t>(pos + 1)] = static_cast<uint8_t>(v);
	mem[pos] = static_cast<uint8_t>(v >> 8);
}

} // namespace

void move_wario_x(GB *gb, int delta)
{
	Memory mem(gb);
	mem[wCameraXDelta] = static_cast<uint8_t>(mem[wCameraXDelta] + delta);
	move_pos(mem, wWarioXPos, delta);
}

void move_wario_y(GB *gb, int delta, bool with_camera)
{
	Memory mem(gb);
	if (with_camera)
		mem[wCameraYDelta] = static_cast<uint8_t>(mem[wCameraYDelta] + delta);
	move_pos(mem, wWarioYPos, delta);
}

void clear_particles(GB *gb)
{
	fill(gb, wParticles, 0, PARTICLE_STRUCT_LENGTH * MAX_NUM_PARTICLES + 2);
}

void do_ground_shake(GB *gb, uint8_t frames)
{
	Memory mem(gb);
	if (mem[wGroundShakeCounter])
		return;
	mem[wGroundShakeCounter] = frames;
	mem[wIsWarioGroundShaking] = 0;
}

void open_action_help(GB *gb)
{
	Memory mem(gb);
	mem[wState] = ST_ACTION_HELP;
	mem[wSubState] = 0;
}

namespace {

bool falling(Memory mem)
{
	return mem[wJumpVelIndex] >= FALLING_JUMP_VEL_INDEX;
}

} // namespace

void cap_walk_vel(GB *gb)
{
	Memory mem(gb);
	bool fall = falling(mem);
	if (mem[wWalkVelIndex] >= (fall ? 0x08 : 0x10))
		mem[wWalkVelIndex] = fall ? 0x04 : 0x0c;
}

namespace {

// count palettes from src to the palette data register, one per H-Blank
void apply_pals_in_hblank(GB *gb, uint16_t src, uint16_t data_reg, int count)
{
	Memory mem(gb);
	while (count--) {
		while (!(mem[rSTAT] & STAT_MODE))
			idle(gb, 32);
		while (mem[rSTAT] & STAT_MODE)
			idle(gb, 32);
		for (int i = 0; i < PAL_SIZE; i++)
			mem[data_reg] = mem[src++];
	}
}

} // namespace

void clear_transformation_values(GB *gb)
{
	Memory mem(gb);
	if (mem[wTransformation] == TRANSFORMATION_BLIND) { // .RestoreBlindPalettes
		mem[rBGPI] = BGPI_AUTOINC | 0;
		apply_pals_in_hblank(gb, wTempPals1, rBGPD, 8);
		mem[rOBPI] = OBPI_AUTOINC | (3 * PAL_SIZE);
		apply_pals_in_hblank(gb, wTempPals2 + 3 * PAL_SIZE, rOBPD, 4);
	}
	for (uint16_t at : std::initializer_list<uint16_t>{wInvincibleCounter, wTransformation, wTransformationProgress, wStingTouchState, wTouchState,
			wca94, wIsIntangible, static_cast<uint16_t>(wTransformationDuration + 0),
			static_cast<uint16_t>(wTransformationDuration + 1), wAutoMoveState, wInvisibleFrame,
			wSwimmingDirectionInput})
		mem[at] = 0;
}

} // namespace wl3::home

using namespace wl3::home;
namespace flags = wl3::flags;

namespace {

// a = the position's new high byte, f from its adc / sbc 0
void moved_regs(GB *gb, uint16_t pos, uint8_t hi, uint8_t lo, bool down)
{
	uint8_t b = gb->cpu.b;
	if (down) {
		bool carry = lo + b > 0xff;
		gb->cpu.a = static_cast<uint8_t>(hi + carry);
		gb->cpu.f = flags::add(hi, 0, carry);
	} else {
		bool borrow = lo < b;
		gb->cpu.a = static_cast<uint8_t>(hi - borrow);
		gb->cpu.f = flags::sub(hi, 0, borrow);
	}
	(void)pos;
}

// b = how far; right and down add, left and up subtract
void move_hook(GB *gb, uint16_t pos, bool add, bool camera)
{
	wl3::Memory mem(gb);
	uint8_t hi = mem[pos], lo = mem[static_cast<uint16_t>(pos + 1)];
	int delta = add ? gb->cpu.b : -gb->cpu.b;
	if (pos == wWarioXPos)
		move_wario_x(gb, delta);
	else
		move_wario_y(gb, delta, camera);
	moved_regs(gb, pos, hi, lo, add);
}

} // namespace

PORT_EX(MoveWarioRight, 0, 0) { move_hook(gb, wWarioXPos, true, true); }
PORT_EX(MoveWarioLeft, 0, 0) { move_hook(gb, wWarioXPos, false, true); }
PORT_EX(MoveWarioDown, 0, 0) { move_hook(gb, wWarioYPos, true, true); }
PORT_EX(MoveWarioUp, 0, 0) { move_hook(gb, wWarioYPos, false, true); }
PORT_EX(MoveWarioDown_WithoutCameraOffset, 0, 0) { move_hook(gb, wWarioYPos, true, false); }
PORT_EX(MoveWarioUp_WithoutCameraOffset, 0, 0) { move_hook(gb, wWarioYPos, false, false); }

PORT_EX(ClearParticles, 0, 0)
{
	clear_particles(gb);
	SET_HL(gb, wParticles + PARTICLE_STRUCT_LENGTH * MAX_NUM_PARTICLES + 2);
	gb->cpu.b = 0;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z | FLAG_N; // the fill's dec b, after xor a
}

// b = the frames
PORT_EX(DoGroundShake, 0, 0)
{
	uint8_t counter = wl3::Memory(gb)[wGroundShakeCounter];
	do_ground_shake(gb, gb->cpu.b);
	gb->cpu.a = counter;
	gb->cpu.f = flags::and_(counter);
	if (!counter) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
	}
}

PORT_EX(OpenActionHelp, 0, 0)
{
	open_action_help(gb);
	gb->cpu.a = ST_ACTION_HELP;
	SET_HL(gb, wState + 1); // wSubState
}

// a and f from the cap's cp (then the cap, if it applied)
PORT_EX(Func_1762, 0, 0)
{
	wl3::Memory mem(gb);
	uint8_t walk = mem[wWalkVelIndex];
	bool fall = mem[wJumpVelIndex] >= FALLING_JUMP_VEL_INDEX;
	cap_walk_vel(gb);
	gb->cpu.f = flags::cp(walk, fall ? 0x08 : 0x10);
	gb->cpu.a = mem[wWalkVelIndex];
}

// after Blind, the palette copies' hl, b and c
PORT_EX(ClearTransformationValues, 0, 0)
{
	bool blind = wl3::Memory(gb)[wTransformation] == TRANSFORMATION_BLIND;
	clear_transformation_values(gb);
	if (blind) {
		SET_HL(gb, wTempPals2 + 3 * PAL_SIZE + 4 * PAL_SIZE);
		gb->cpu.b = 0;
		gb->cpu.c = static_cast<uint8_t>(rOBPD);
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}
