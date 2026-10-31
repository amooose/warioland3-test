// The owl (owl.asm): sleeps by day until Wario disturbs it, then flies up
// and back and forth, carrying him if he grabs on. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t SLEEP = ADDR_OwlFunc + 0x3c; // .Sleep
constexpr uint16_t WAKE_UP = ADDR_OwlFunc + 0x66; // .WakeUp
constexpr uint16_t FLY_UPWARDS = ADDR_OwlFunc + 0x92; // .FlyUpwards
constexpr uint16_t FLYING = ADDR_OwlFunc + 0xe1; // .Func_492b4

constexpr uint8_t GRABBED = 0x18; // the state once Wario's let go of it

class Owl : public Object {
public:
	using Object::Object;

	void init(); // OwlFunc
	void sleep(); // .Sleep
	void wake_up(); // .WakeUp
	void fly_upwards(); // .FlyUpwards
	void fly(); // .Func_492b4

private:
	void flap(uint8_t f);
	void start_flying_upwards(); // .set_fly_upwards
	void face(bool right);
};

} // namespace

// the flap's sound, on the last tick of the wings' frame
void Owl::flap(uint8_t f)
{
	if (frame_ending(f))
		play_sfx(SFX_FLAP);
}

// OwlFunc: asleep by day; at night it flies off straight away
void Owl::init()
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 0x06;
	mem_[wCurObjCollBoxLeft] = 0xf9;
	mem_[wCurObjCollBoxBottom] = 0xfe;
	if (!(wram_.day_night() & 1)) {
		set_update(SLEEP);
		set_frameset(ADDR_Frameset_692e7);
		return;
	}
	play_sfx(SFX_OWL_HOOT);
	set_interaction(OBJ_INTERACTION_0B);
	start_flying_upwards();
}

void Owl::start_flying_upwards()
{
	set_update(FLY_UPWARDS);
	set_frameset(ADDR_Frameset_69335);
	state_duration() = 54;
}

// until something sets its state
void Owl::sleep()
{
	if (!state())
		return;
	state() = 0;
	set_update(WAKE_UP);
	set_frameset(ADDR_Frameset_69318);
	state_duration() = 0x28;
	set_interaction(OBJ_INTERACTION_0B);
	play_sfx(SFX_OWL_HOOT);
}

void Owl::wake_up()
{
	flap(0x0c);
	if (--state_duration())
		return;
	start_flying_upwards();
}

// then turns towards Wario
void Owl::fly_upwards()
{
	flap(0x0c);
	move_up();
	if (--state_duration())
		return;
	state_duration() = 30;
	set_update(FLYING);
	set_interaction(OBJ_INTERACTION_OWL);
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	face(static_cast<uint8_t>(screen_x() + 0x2a) < wario);
}

void Owl::face(bool right)
{
	if (right)
		substate() |= OBJSUBFLAG_HDIR;
	else
		substate() &= ~OBJSUBFLAG_HDIR;
	set_frameset(right ? ADDR_Frameset_69377 : ADDR_Frameset_6936c);
}

// back and forth, turning every 60 frames; gone once Wario's let go
void Owl::fly()
{
	if (state() == GRABBED) {
		flags() = 0;
		despawn();
		return;
	}
	if (!--state_duration()) {
		state_duration() = 60;
		substate() ^= OBJSUBFLAG_HDIR;
		set_frameset(facing_right() ? ADDR_Frameset_69377 : ADDR_Frameset_6936c);
		return;
	}
	if (facing_right()) {
		flap(0x23);
		move_right();
	} else {
		flap(0x1e);
		move_left();
	}
}

} // namespace wl3::level

using Owl = wl3::level::Owl;

PORT_EX(OwlFunc, 0, ALL_REGISTERS) { Owl(gb).init(); }
PORT_LOCAL_EX(OwlFunc, Sleep, 0, ALL_REGISTERS) { Owl(gb).sleep(); }
PORT_LOCAL_EX(OwlFunc, WakeUp, 0, ALL_REGISTERS) { Owl(gb).wake_up(); }
PORT_LOCAL_EX(OwlFunc, FlyUpwards, 0, ALL_REGISTERS) { Owl(gb).fly_upwards(); }
PORT_LOCAL_EX(OwlFunc, Func_492b4, 0, ALL_REGISTERS) { Owl(gb).fly(); }
