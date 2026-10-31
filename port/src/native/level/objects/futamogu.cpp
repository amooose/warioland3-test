// Futamogu (futamogu.asm): a mole that climbs back to where it started
// (carrying Wario up if he's on it), shakes when hit or the ground
// shakes, and is knocked down by a ground pound (or out, with the super
// jump slam overalls). See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t UPDATE = ADDR_FutamoguFunc + 0x7f; // .Update

class Futamogu : public Object {
public:
	using Object::Object;

	void init(); // FutamoguFunc
	void update(); // .Update

private:
	bool at_start();
	void idle(); // .Idle
	void climb(); // .Climb
	void angry(); // .Angry
	void set_idle(); // .set_idle
	void set_shake(); // .set_shake
	void pounded(); // .asm_407bf
	void fall(); // .Fall
	void knocked_out(); // .vanish
};

} // namespace

void Futamogu::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	set_frameset(ADDR_Frameset_68319);
	var1() = mem_[wCurObjYPos + 0]; // its start
	var2() = mem_[wCurObjYPos + 1];
	mem_[wCurObjCollBoxRight] = 5;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-21);
	mem_[wCurObjCollBoxBottom] = 4;
}

void Futamogu::update()
{
	uint8_t s = state();
	if (!s)
		idle();
	else if (s == OBJSTATE_SPECIAL_3)
		climb();
	else if (s == OBJSTATE_SPECIAL_2)
		angry();
	else if (s == OBJSTATE_ATTACKED_LEFT) { // .Shake
		if (!--state_duration())
			set_idle();
	} else if (s == OBJSTATE_VANISH_TOUCH)
		pounded();
	else if (s == OBJSTATE_3A)
		fall();
	else if ((s & 0xfe) == OBJSTATE_ATTACKED_LEFT_START) { // .Attacked
		state() = OBJSTATE_ATTACKED_LEFT;
		set_frameset(ADDR_Frameset_68351);
		state_duration() = 16;
	} else
		set_idle();
}

bool Futamogu::at_start()
{
	return mem_[wCurObjYPos + 0] == var1() && mem_[wCurObjYPos + 1] == var2();
}

void Futamogu::set_shake()
{
	state() = OBJSTATE_ATTACKED_LEFT;
	set_frameset(ADDR_Frameset_68348);
	state_duration() = 32;
}

void Futamogu::set_idle()
{
	set_frameset(ADDR_Frameset_68319);
	flags() &= ~OBJFLAG_NO_COLLISION;
	state() = 0;
}

// at its start, annoyed when stood on; else climbs back
void Futamogu::idle()
{
	if (mem_[wGroundShakeCounter] >= 0x10) {
		set_shake();
		return;
	}
	if (!at_start()) {
		state() = OBJSTATE_SPECIAL_3;
		substate() &= ~OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_68336);
		return;
	}
	if (!(flags() & OBJFLAG_STEPPED))
		return;
	state() = OBJSTATE_SPECIAL_2;
	set_frameset(ADDR_Frameset_68343);
}

// a pixel a frame (half that, with Wario on it)
void Futamogu::climb()
{
	if (mem_[wGroundShakeCounter] >= 0x10) {
		set_shake();
		return;
	}
	if (at_start()) {
		set_idle();
		return;
	}
	if (flags() & OBJFLAG_STEPPED) {
		flags() &= ~OBJFLAG_STEPPED;
		if (!(mem_[wGlobalCounter] & 1))
			return;
		gb_->cpu.b = 1;
		routines_.call(ADDR_MoveWarioUp);
		routines_.call(ADDR_HandleUpwardsFloorTransition);
		move_up();
		return;
	}
	if (frame_duration()) // the animation at double speed
		--frame_duration();
	move_up();
}

void Futamogu::angry()
{
	if (flags() & OBJFLAG_STEPPED)
		flags() &= ~OBJFLAG_STEPPED;
	else
		set_idle();
}

void Futamogu::pounded()
{
	if (facing_right()) { // already as low as it goes
		knocked_out();
		return;
	}
	state() = OBJSTATE_3A;
	var3() = 0;
	set_frameset(ADDR_Frameset_68348);
	state_duration() = 48;
}

// pushed down until it lands (as low as it goes from then on)
void Futamogu::fall()
{
	if (!--state_duration()) {
		set_idle();
		return;
	}
	if (facing_right())
		return;
	bool super_jump_slam = mem_[wEffectivePowerUp] >= SUPER_JUMP_SLAM_OVERALLS;
	SET_BC(gb_, super_jump_slam ? ADDR_Data_60490 : ADDR_Data_603f0);
	routines_.call(ADDR_ApplyObjYMovement);
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a)
		return;
	if (super_jump_slam) {
		knocked_out();
		return;
	}
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	substate() |= OBJSUBFLAG_HDIR;
}

void Futamogu::knocked_out()
{
	var2() = 0x08;
	move_left_by_var2();
	routines_.call(ADDR_VanishObject);
}

} // namespace wl3::level

using Futamogu = wl3::level::Futamogu;

PORT_EX(FutamoguFunc, 0, ALL_REGISTERS) { Futamogu(gb).init(); }
PORT_LOCAL_EX(FutamoguFunc, Update, 0, ALL_REGISTERS) { Futamogu(gb).update(); }
