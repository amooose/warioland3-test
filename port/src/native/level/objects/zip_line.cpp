// The zip lines (zip_line.asm): handles Wario grabs from below and rides
// along a line (level, or sloping up or down by the line's id) to a wall,
// where the first three kinds turn back and the others stop and drop
// him. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t RESET = ADDR_ZipLineFunc + 0x30; // .Func_49356
constexpr uint16_t IDLE = ADDR_ZipLineFunc + 0x43; // .Func_49369
constexpr uint16_t RIDING = ADDR_ZipLineFunc + 0xb6; // .Func_493dc
constexpr uint16_t LET_GO = ADDR_ZipLineFunc + 0xdf; // .Func_49405
constexpr uint16_t STOPPED = ADDR_ZipLineFunc + 0xec; // .Func_49412

// var1
constexpr uint8_t WAITING = 0;
constexpr uint8_t CARRYING = 1;
constexpr uint8_t AT_END = 3;

class ZipLine : public Object {
public:
	using Object::Object;

	void init(); // ZipLineFunc
	void reset(); // .Func_49356
	void idle(); // .Func_49369
	void riding(); // .Func_493dc
	void let_go(); // .Func_49405
	void move(); // .Func_49418

private:
	void stop(); // .asm_493cd
	void at_wall(); // .asm_4947b
	void along(bool right); // .Func_494cb, .Func_494af
	void slope(bool up); // .asm_494e7, .asm_494f5
	bool carrying() { return var1() == CARRYING; }
};

} // namespace

void ZipLine::init()
{
	uint8_t id = mem_[wCurObjId];
	var2() = id < ZIP_LINE4 ? 1 : 3;
	if (id == ZIP_LINE4)
		substate() &= ~OBJSUBFLAG_HDIR;
	else
		substate() |= OBJSUBFLAG_HDIR;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	mem_[wCurObjCollBoxRight] = 0x03;
	mem_[wCurObjCollBoxLeft] = 0xfc;
	mem_[wCurObjCollBoxBottom] = 0xfe;
	var1() = WAITING;
	reset();
}

void ZipLine::reset()
{
	set_frameset(ADDR_Frameset_691c4);
	set_update(IDLE);
	state() = 0;
}

void ZipLine::stop()
{
	set_frameset(ADDR_Frameset_691db);
	set_update(STOPPED);
}

// grabbable from just below; once grabbed (its state set), Wario hangs
// from it
void ZipLine::idle()
{
	no_actions();
	move();
	if (var1() == AT_END) {
		stop();
		return;
	}
	if (!state()) {
		// the asm adds c to both, which cancels out
		uint8_t below = static_cast<uint8_t>(mem_[wWarioScreenYPos] - screen_y());
		if (below >= 0x10 && below < 0x18)
			mem_[wCurObjAction] = 0;
		return;
	}
	set_update(RIDING);
	var1() = CARRYING;
	uint16_t wario_y = static_cast<uint16_t>(y() + 0x1b);
	uint16_t wario_x = static_cast<uint16_t>(x() + 0x04);
	mem_[wWarioYPos + 1] = static_cast<uint8_t>(wario_y);
	mem_[wWarioYPos + 0] = static_cast<uint8_t>(wario_y >> 8);
	mem_[wWarioXPos + 1] = static_cast<uint8_t>(wario_x);
	mem_[wWarioXPos + 0] = static_cast<uint8_t>(wario_x >> 8);
	set_frameset(facing_right() ? ADDR_Frameset_691d2 : ADDR_Frameset_691c9);
}

// until Wario lets go
void ZipLine::riding()
{
	no_actions();
	move();
	if (var1() == AT_END) {
		stop();
		return;
	}
	if (mem_[wTransformation] == TRANSFORMATION_RAIL)
		return;
	set_update(LET_GO);
	state_duration() = 0x0a;
	var1() = 2;
}

void ZipLine::let_go()
{
	no_actions();
	if (!--state_duration())
		reset();
}

// var2 pixels along the line (and Wario with it), unless at a wall
void ZipLine::move()
{
	if (!var1())
		return;
	bool right = facing_right();
	uint16_t y_pos = static_cast<uint16_t>(y() - 0x10);
	uint16_t x_pos = right ? static_cast<uint16_t>(x() + 7) : static_cast<uint16_t>(x() - 8);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a) {
		at_wall();
		return;
	}
	along(right);
	uint8_t id = mem_[wCurObjId];
	if (id == ZIP_LINE1)
		return;
	bool even = !(id & 1);
	slope(right ? !even : even); // up or down by the line's id
}

void ZipLine::at_wall()
{
	if (mem_[wCurObjId] < ZIP_LINE4) { // turns back
		if (facing_right()) {
			substate() &= ~OBJSUBFLAG_HDIR;
			set_frameset(ADDR_Frameset_691c9);
		} else {
			substate() |= OBJSUBFLAG_HDIR;
			set_frameset(ADDR_Frameset_691d2);
		}
		return;
	}
	if (carrying()) {
		mem_[wAutoMoveState] = 0;
		play_sfx(SFX_129);
	}
	var1() = AT_END;
}

// MoveObject*ByVar2 leave var2 in b, which MoveWario* moves him by
void ZipLine::along(bool right)
{
	right ? move_right_by_var2() : move_left_by_var2();
	if (!carrying())
		return;
	routines_.call(right ? ADDR_MoveWarioRight : ADDR_MoveWarioLeft);
	if (right)
		routines_.farcall(BANK_Func_19706, ADDR_Func_19706);
	else
		routines_.farcall(BANK_Func_19713, ADDR_Func_19713);
	if (mem_[wCurObjId] >= ZIP_LINE4)
		return;
	if (!(mem_[wGlobalCounter] & 0x1f))
		play_sfx(SFX_08C);
}

void ZipLine::slope(bool up)
{
	up ? move_up_by_var2() : move_down_by_var2();
	if (!carrying())
		return;
	if (up) {
		routines_.call(ADDR_MoveWarioUp);
		routines_.call(ADDR_HandleUpwardsFloorTransition);
		return;
	}
	routines_.call(ADDR_MoveWarioDown);
	routines_.call(ADDR_HandleDownwardsFloorTransition);
	if (mem_[wCurObjId] < ZIP_LINE4)
		return;
	if (!(mem_[wGlobalCounter] & 0b111))
		play_sfx(SFX_ZIP_LINE_SLIDE);
}

} // namespace wl3::level

using ZipLine = wl3::level::ZipLine;

PORT_EX(ZipLineFunc, 0, ALL_REGISTERS) { ZipLine(gb).init(); }
PORT_LOCAL_EX(ZipLineFunc, Func_49356, 0, ALL_REGISTERS) { ZipLine(gb).reset(); }
PORT_LOCAL_EX(ZipLineFunc, Func_49369, 0, ALL_REGISTERS) { ZipLine(gb).idle(); }
PORT_LOCAL_EX(ZipLineFunc, Func_493dc, 0, ALL_REGISTERS) { ZipLine(gb).riding(); }
PORT_LOCAL_EX(ZipLineFunc, Func_49405, 0, ALL_REGISTERS) { ZipLine(gb).let_go(); }
PORT_LOCAL_EX(ZipLineFunc, Func_49412, 0, ALL_REGISTERS) { wl3::Memory mem(gb); mem[wCurObjAction] = (1 << NO_ACTIONS_F) | 1; }
PORT_LOCAL_EX(ZipLineFunc, Func_49418, 0, ALL_REGISTERS) { ZipLine(gb).move(); }
