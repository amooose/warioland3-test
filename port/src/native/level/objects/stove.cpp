// The stove (stove.asm): a heavy block Wario can push along the floor;
// it falls off edges (shaking the ground as it lands) and lights Wario
// on fire unless he's already transformed. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t UPDATE = ADDR_StoveFunc + 0x6f; // .Update

class Stove : public Object {
public:
	using Object::Object;

	void init(); // StoveFunc
	void update(); // .Update

private:
	void set_fall(); // .set_fall
	void fall(); // .Fall
	void set_land(); // .set_land
	void land(); // .Land
	void pushed(bool right); // .State34, .AttackedRightStart
	void start_push(uint8_t state); // .AttackedLeftStart, .State05
	bool ground_at(uint16_t x_pos); // Func_35a3 at x, at its height
};

} // namespace

void Stove::init()
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	set_update(UPDATE);
	mem_[wCurObjCollBoxRight] = 10;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-9);
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-9);
	set_frameset(ADDR_Frameset_69615);
	var1() = 0;
}

void Stove::update()
{
	switch (state()) {
	case OBJSTATE_FALL: fall(); return;
	case 0: set_fall(); return;
	case OBJSTATE_LAND: land(); return;
	case OBJSTATE_ATTACKED_LEFT: pushed(true); return;
	case OBJSTATE_ATTACKED_LEFT_START: start_push(OBJSTATE_ATTACKED_LEFT); return;
	case OBJSTATE_ATTACKED_RIGHT: pushed(false); return;
	case OBJSTATE_ATTACKED_RIGHT_START: start_push(OBJSTATE_ATTACKED_RIGHT); return;
	case OBJSTATE_VANISH_TOUCH: // .VanishTouch: Wario's set on fire
		if (mem_[wTransformation]) {
			vanish();
			return;
		}
		state() = OBJSTATE_3A;
		set_frameset(ADDR_Frameset_69625);
		state_duration() = 44;
		return;
	case OBJSTATE_3A:
		if (!--state_duration())
			set_land();
		return;
	default: set_fall(); return;
	}
}

void Stove::set_fall()
{
	state() = OBJSTATE_FALL;
	var3() = 0;
	set_frameset(ADDR_Frameset_69615);
}

// shaking the ground on landing, unless it's just been placed
void Stove::fall()
{
	SET_BC(gb_, ADDR_Data_605e0);
	routines_.call(ADDR_ApplyObjYMovement);
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a)
		return;
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	state() = OBJSTATE_LAND;
	if (!var1())
		return;
	play_sfx(SFX_063);
	gb_->cpu.b = 24;
	routines_.call(ADDR_DoGroundShake);
}

void Stove::set_land()
{
	state() = OBJSTATE_LAND;
	set_frameset(ADDR_Frameset_69615);
}

void Stove::start_push(uint8_t s)
{
	state() = s;
	var2() = 0x02;
	set_frameset(ADDR_Frameset_69618);
}

// pushed var2 pixels a frame, slowing every 8 frames, until it stops or
// meets a wall
void Stove::pushed(bool right)
{
	if (!(mem_[wGlobalCounter] & 0b111)) {
		play_sfx(SFX_078);
		if (!--var2()) {
			set_land();
			return;
		}
	}
	uint16_t y_pos = static_cast<uint16_t>(y() - 4);
	uint16_t x_pos = right ? static_cast<uint16_t>(x() + 10) : static_cast<uint16_t>(x() - 9);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a & 0x0f) {
		set_land();
		return;
	}
	right ? move_right_by_var2() : move_left_by_var2();
}

bool Stove::ground_at(uint16_t x_pos)
{
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_35a3);
	return gb_->cpu.a;
}

// on the floor; with nothing under either side, it'll fall (and shake
// the ground when it lands)
void Stove::land()
{
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_3543);
	bool on_floor = gb_->cpu.a == 0x10;
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	if (on_floor)
		return;
	if (ground_at(static_cast<uint16_t>(x() - 7)))
		return;
	if (ground_at(static_cast<uint16_t>(x() + 8)))
		return;
	state() = OBJSTATE_00;
	var1() = 1;
}

} // namespace wl3::level

using Stove = wl3::level::Stove;

PORT_EX(StoveFunc, 0, ALL_REGISTERS) { Stove(gb).init(); }
PORT_LOCAL_EX(StoveFunc, Update, 0, ALL_REGISTERS) { Stove(gb).update(); }
