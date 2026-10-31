// The seeing eye door (seeing_eye_door.asm): an eye that follows Wario
// along its row, shut (and passable) only while it looks straight ahead;
// it looks around for him when he's out of its sight. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t CHECK_DISTANCE = ADDR_SeeingEyeDoorFunc + 0x2b; // .CheckWarioDistance
constexpr uint16_t WAIT_FOR_WARIO = ADDR_SeeingEyeDoorFunc + 0x56; // .WaitForWarioInRange
constexpr uint16_t LOOK_AROUND = ADDR_SeeingEyeDoorFunc + 0xa0; // .LookAround
constexpr uint16_t WATCHING = ADDR_SeeingEyeDoorFunc + 0xb4; // .Default

// where it looks (var1)
constexpr uint8_t FAR_LEFT = 0x5a;
constexpr uint8_t LEFT = 0x5b;
constexpr uint8_t AHEAD = 0x5c;
constexpr uint8_t RIGHT = 0x2d;
constexpr uint8_t FAR_RIGHT = 0x2e;

class SeeingEyeDoor : public Object {
public:
	using Object::Object;

	void init(); // SeeingEyeDoorFunc
	void check_distance(); // .CheckWarioDistance
	void wait_for_wario(); // .WaitForWarioInRange
	void look_around(); // .LookAround
	void watching(); // .Default

private:
	bool wario_in_sight();
	bool wario_to_right() const;
	void set_look_around(); // .set_look_around
	void wait(); // .asm_45545
	void look(uint8_t where, uint16_t frameset, uint16_t sfx);
};

} // namespace

void SeeingEyeDoor::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(CHECK_DISTANCE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	var2() = 24;
	move_down_by_var2();
	set_frameset(ADDR_Frameset_68c47);
	mem_[wCurObjCollBoxRight] = 0x02;
	mem_[wCurObjCollBoxLeft] = 0xfd;
	mem_[wCurObjCollBoxBottom] = 0x17;
}

bool SeeingEyeDoor::wario_to_right() const
{
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	return static_cast<uint8_t>(screen_x() + 0x2a) < wario;
}

// looking far to Wario's side
void SeeingEyeDoor::check_distance()
{
	set_update(WATCHING);
	if (wario_to_right()) {
		var1() = FAR_RIGHT;
		set_frameset(ADDR_Frameset_68c59);
	} else {
		var1() = FAR_LEFT;
		set_frameset(ADDR_Frameset_68c47);
	}
	watching();
}

// visible and in the eye's rows
bool SeeingEyeDoor::wario_in_sight()
{
	if (mem_[wTransformation] == TRANSFORMATION_INVISIBLE_WARIO)
		return false;
	uint8_t top = static_cast<uint8_t>(screen_y() - 0x19);
	return static_cast<uint8_t>(mem_[wWarioScreenYPos] - top) < 0x36;
}

void SeeingEyeDoor::wait_for_wario()
{
	no_actions();
	if (wario_in_sight())
		check_distance();
}

void SeeingEyeDoor::set_look_around()
{
	uint8_t where = var1();
	if (where == FAR_RIGHT || where == FAR_LEFT) {
		wait();
		return;
	}
	set_frameset(wario_to_right() ? ADDR_Frameset_68c56 : ADDR_Frameset_68c4a);
	state_duration() = 8;
	set_update(LOOK_AROUND);
}

void SeeingEyeDoor::look_around()
{
	if (!--state_duration())
		wait();
}

void SeeingEyeDoor::wait()
{
	set_frameset(ADDR_Frameset_68c36);
	set_update(WAIT_FOR_WARIO);
}

void SeeingEyeDoor::look(uint8_t where, uint16_t frameset, uint16_t sfx)
{
	play_sfx(sfx);
	var1() = where;
	set_frameset(frameset);
}

// following Wario a step at a time; solid unless looking ahead (closed
// to him only while he's right in front)
void SeeingEyeDoor::watching()
{
	if (!wario_in_sight()) {
		set_look_around();
		return;
	}
	uint8_t wario = mem_[wWarioScreenXPos];
	uint8_t eye = screen_x();
	if (state()) {
		state() = 0;
		flags() &= ~OBJFLAG_NO_COLLISION;
	}
	auto left_of = [&](int offset) { return static_cast<uint8_t>(eye + offset) < wario; };
	switch (var1()) {
	case AHEAD:
		if (left_of(0x17))
			look(RIGHT, ADDR_Frameset_68c56, SFX_07A);
		else if (!left_of(-0x18))
			look(LEFT, ADDR_Frameset_68c4a, SFX_07A);
		return;
	case RIGHT:
		if (left_of(0x46))
			look(FAR_RIGHT, ADDR_Frameset_68c59, SFX_07A);
		else if (!left_of(0x17))
			look(AHEAD, ADDR_Frameset_68c50, SFX_07B);
		return;
	case FAR_RIGHT:
		no_actions();
		if (!left_of(0x46))
			look(RIGHT, ADDR_Frameset_68c56, SFX_07A);
		return;
	case LEFT:
		if (left_of(-0x18))
			look(AHEAD, ADDR_Frameset_68c50, SFX_07B);
		else if (!left_of(-0x47))
			look(FAR_LEFT, ADDR_Frameset_68c47, SFX_07A);
		return;
	case FAR_LEFT:
		no_actions();
		if (left_of(-0x47))
			look(LEFT, ADDR_Frameset_68c4a, SFX_07A);
		return;
	}
}

} // namespace wl3::level

using SeeingEyeDoor = wl3::level::SeeingEyeDoor;

PORT_EX(SeeingEyeDoorFunc, 0, ALL_REGISTERS) { SeeingEyeDoor(gb).init(); }
PORT_LOCAL_EX(SeeingEyeDoorFunc, CheckWarioDistance, 0, ALL_REGISTERS) { SeeingEyeDoor(gb).check_distance(); }
PORT_LOCAL_EX(SeeingEyeDoorFunc, WaitForWarioInRange, 0, ALL_REGISTERS) { SeeingEyeDoor(gb).wait_for_wario(); }
PORT_LOCAL_EX(SeeingEyeDoorFunc, LookAround, 0, ALL_REGISTERS) { SeeingEyeDoor(gb).look_around(); }
PORT_LOCAL_EX(SeeingEyeDoorFunc, Default, 0, ALL_REGISTERS) { SeeingEyeDoor(gb).watching(); }
