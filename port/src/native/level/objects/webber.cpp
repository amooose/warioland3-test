// Webber (webber.asm): a spider hanging on a thread that spits webs at
// Wario every so often; stood on, it sinks under him, and climbs back to
// its start; shaken or pounded it drops. Its mode is the substate's low
// bits. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t UPDATE = ADDR_WebberFunc + 0x70; // .Update
constexpr uint16_t WEB_GONE = ADDR_WebberProjectileFunc + 0x54; // .Destroy

// the modes (the substate's low bits)
enum : uint8_t {
	IDLE = 1, // .Idle (0 too)
	FIRING = 2, // .FireProjectile
	RETURNING = 3, // .MoveBackToStartPosition
	SHAKING = 4, // .Shake
	SMASHED = 5, // .SmashAttacked
	CLIMBING = 6, // .Climb
	STEPPED_ON = 7, // .SteppedOn
	RESTING = 8, // .SpecialIdle
};

class Webber : public Object {
public:
	using Object::Object;

	void init(); // WebberFunc
	void update(); // .Update
	void run(uint8_t mode);
	void web(); // WebberProjectileFunc
	void web_gone(); // .Destroy

	void idle(); // .Idle
	void firing(); // .FireProjectile
	void returning(); // .MoveBackToStartPosition
	void shaking(); // .Shake
	void smashed(); // .SmashAttacked
	void climbing(); // .Climb
	void stepped_on(); // .SteppedOn
	void resting(); // .SpecialIdle

private:
	void set_mode(uint8_t mode) { substate() = (substate() & OBJSUBFLAGS_MASK) | mode; }
	void set_mode(uint8_t mode, uint16_t frameset);
	bool shaken(); // .set_shake if the ground shakes hard
	void set_shake();
	void set_fire(); // .set_fire_projectile
	void set_idle(); // .set_idle
	void set_return(); // .set_move_to_start_pos
	void knocked(uint16_t routine, uint16_t frameset); // .asm_40865
	void move_vertically(bool up); // .move_up, .move_down
	bool landed();
};

} // namespace

void Webber::set_mode(uint8_t mode, uint16_t frameset)
{
	set_mode(mode);
	set_frameset(frameset);
}

void Webber::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = static_cast<uint8_t>(-2);
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-16);
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-4);
	var1() = mem_[wCurObjYPos + 0]; // its start
	var2() = mem_[wCurObjYPos + 1];
	var3() = 4; // webs before a rest
	set_fire();
}

void Webber::set_fire()
{
	set_mode(FIRING, ADDR_Frameset_682b7);
	state_duration() = 64;
}

void Webber::set_idle() { set_mode(IDLE, ADDR_Frameset_682a4); }

void Webber::set_return()
{
	substate() = (substate() & (OBJSUBFLAGS_MASK ^ OBJSUBFLAG_HDIR)) | RETURNING;
	var3() = 5;
	set_frameset(ADDR_Frameset_682b2);
}

void Webber::set_shake()
{
	set_mode(SHAKING);
	var3() = 0;
	set_frameset(ADDR_Frameset_682a9);
	state_duration() = 48;
}

bool Webber::shaken()
{
	if (mem_[wGroundShakeCounter] < 0x10)
		return false;
	set_shake();
	return true;
}

void Webber::knocked(uint16_t routine, uint16_t frameset)
{
	set_update(routine);
	set_frameset(frameset);
	substate() &= OBJSUBFLAGS_MASK;
	var3() = 0;
	var2() = 0x02;
	no_actions();
}

void Webber::update()
{
	uint8_t s = state();
	if (s == OBJSTATE_ATTACKED_LEFT_START) {
		knocked(ADDR_Func_3326, ADDR_Frameset_682c2);
		return;
	}
	if (s == OBJSTATE_ATTACKED_RIGHT_START) {
		knocked(ADDR_Func_3317, ADDR_Frameset_682c5);
		return;
	}
	if (s == OBJSTATE_VANISH_TOUCH) { // .set_smash_attacked
		state() = 0;
		if (facing_right()) { // already down
			routines_.call(ADDR_VanishObject);
			return;
		}
		set_mode(SMASHED);
		var3() = 0;
		set_frameset(ADDR_Frameset_682a9);
		state_duration() = 48;
		return;
	}
	if (s) { // hit: climbs up its thread a bit
		state() = 0;
		set_mode(CLIMBING, ADDR_Frameset_682d1);
		state_duration() = 50;
		flags() &= ~OBJFLAG_NO_COLLISION;
	}
	uint8_t mode = substate() & ~OBJSUBFLAGS_MASK;
	if (mode > RESTING) {
		unrealistic();
		return;
	}
	run(mode);
}

void Webber::run(uint8_t mode)
{
	switch (mode) {
	case 0: case IDLE: idle(); return;
	case FIRING: firing(); return;
	case RETURNING: returning(); return;
	case SHAKING: shaking(); return;
	case SMASHED: smashed(); return;
	case CLIMBING: climbing(); return;
	case STEPPED_ON: stepped_on(); return;
	case RESTING: resting(); return;
	}
}

// back to its start, a pixel a frame (half that, carrying Wario)
void Webber::returning()
{
	if (shaken())
		return;
	uint16_t start = static_cast<uint16_t>(var2() << 8 | var1());
	if (y() == start) {
		set_idle();
		return;
	}
	move_vertically(y() > start);
}

void Webber::move_vertically(bool up)
{
	if (flags() & OBJFLAG_STEPPED) {
		flags() &= ~OBJFLAG_STEPPED;
		if (!(mem_[wGlobalCounter] & 1))
			return;
		gb_->cpu.b = 1;
		routines_.call(up ? ADDR_MoveWarioUp : ADDR_MoveWarioDown);
		routines_.call(up ? ADDR_HandleUpwardsFloorTransition : ADDR_HandleDownwardsFloorTransition);
	} else if (frame_duration()) { // the animation at double speed
		--frame_duration();
	}
	up ? move_up() : move_down();
}

bool Webber::landed()
{
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_352b);
	return gb_->cpu.a;
}

// bounced down to the floor (var3 15: done), then climbs back
void Webber::shaking()
{
	if (var3() != 15) {
		SET_BC(gb_, ADDR_Data_604a0);
		routines_.call(ADDR_ApplyObjYMovement);
		if (!landed())
			return;
		mem_[wCurObjYPos + 0] = mem_[hYPosLo];
		mem_[wCurObjYPos + 1] = mem_[hYPosHi];
		substate() |= OBJSUBFLAG_HDIR;
	}
	if (!--state_duration()) {
		set_return();
		return;
	}
	if (mem_[wGroundShakeCounter] >= 0x11)
		set_shake();
}

// a web every 128 frames, a rest after 4; stood on, it sinks
void Webber::idle()
{
	if (shaken())
		return;
	if (flags() & OBJFLAG_STEPPED) {
		set_mode(STEPPED_ON, ADDR_Frameset_682d6);
		return;
	}
	if (mem_[wGlobalCounter] & 0x7f)
		return;
	if (--var3()) {
		set_fire();
		return;
	}
	set_mode(RESTING, ADDR_Frameset_682e2);
	state_duration() = 162;
}

void Webber::stepped_on()
{
	if (!(flags() & OBJFLAG_STEPPED)) {
		set_return();
		return;
	}
	flags() &= ~OBJFLAG_STEPPED;
}

// pushed down to the floor (knocked out, with the super jump slam)
void Webber::smashed()
{
	if (shaken())
		return;
	if (!--state_duration()) {
		set_return();
		return;
	}
	if (facing_right())
		return;
	bool super_jump_slam = mem_[wEffectivePowerUp] >= SUPER_JUMP_SLAM_OVERALLS;
	SET_BC(gb_, super_jump_slam ? ADDR_Data_60490 : ADDR_Data_603f0);
	routines_.call(ADDR_ApplyObjYMovement);
	if (!landed())
		return;
	if (super_jump_slam) {
		routines_.call(ADDR_VanishObject);
		return;
	}
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	substate() |= OBJSUBFLAG_HDIR;
}

void Webber::firing()
{
	if (shaken())
		return;
	if (!--state_duration()) {
		set_idle();
		return;
	}
	if (state_duration() != 40)
		return;
	SET_BC(gb_, ADDR_ObjParams_WebberProjectile);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

void Webber::climbing()
{
	no_actions();
	if (shaken())
		return;
	if (!--state_duration()) {
		set_return();
		return;
	}
	if (state_duration() >= 20)
		move_up();
}

void Webber::resting()
{
	if (!--state_duration()) {
		var3() = 5;
		set_idle();
		return;
	}
	if (shaken())
		return;
	if (flags() & OBJFLAG_STEPPED)
		set_mode(STEPPED_ON, ADDR_Frameset_682d6);
}

// drifts down towards Wario until it lands or hits something
void Webber::web()
{
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	bool right = static_cast<uint8_t>(screen_x() + 0x2a) < wario;
	routines_.call(right ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
	move_down();
	flags() |= OBJFLAG_NO_COLLISION;
	if (!state()) {
		if (!landed())
			return;
		mem_[wCurObjYPos + 0] = mem_[hYPosLo];
		mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	}
	set_update(WEB_GONE);
	set_frameset(ADDR_Frameset_682db);
	state_duration() = 24;
}

void Webber::web_gone()
{
	no_actions();
	if (!--state_duration())
		flags() = 0;
}

} // namespace wl3::level

using Webber = wl3::level::Webber;

PORT_EX(WebberFunc, 0, ALL_REGISTERS) { Webber(gb).init(); }
PORT_LOCAL_EX(WebberFunc, Update, 0, ALL_REGISTERS) { Webber(gb).update(); }
PORT_LOCAL_EX(WebberFunc, MoveBackToStartPosition, 0, ALL_REGISTERS) { Webber(gb).returning(); }
PORT_LOCAL_EX(WebberFunc, Shake, 0, ALL_REGISTERS) { Webber(gb).shaking(); }
PORT_LOCAL_EX(WebberFunc, Idle, 0, ALL_REGISTERS) { Webber(gb).idle(); }
PORT_LOCAL_EX(WebberFunc, SteppedOn, 0, ALL_REGISTERS) { Webber(gb).stepped_on(); }
PORT_LOCAL_EX(WebberFunc, SmashAttacked, 0, ALL_REGISTERS) { Webber(gb).smashed(); }
PORT_LOCAL_EX(WebberFunc, FireProjectile, 0, ALL_REGISTERS) { Webber(gb).firing(); }
PORT_LOCAL_EX(WebberFunc, Climb, 0, ALL_REGISTERS) { Webber(gb).climbing(); }
PORT_LOCAL_EX(WebberFunc, SpecialIdle, 0, ALL_REGISTERS) { Webber(gb).resting(); }
PORT_EX(WebberProjectileFunc, 0, ALL_REGISTERS) { Webber(gb).web(); }
PORT_LOCAL_EX(WebberProjectileFunc, Destroy, 0, ALL_REGISTERS) { Webber(gb).web_gone(); }
