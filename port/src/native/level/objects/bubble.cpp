// The bubbles (bubble.asm): a hole releases one now and then (while few
// are about), which rises swaying from side to side until it leaves the
// water or bumps the ceiling, and bursts. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t HOLE_UPDATE = ADDR_BubbleHoleFunc + 0x13; // .Update
constexpr uint16_t RISING = ADDR_BubbleFunc + 0x1f; // .Update
constexpr uint16_t BURST = ADDR_BubbleFunc + 0x43; // .Burst

// the sway's speeds, one a frame until a $80 ends the list, read from
// wherever's mapped at $4020 + var2 (left) or $4050 + var2 (right)
constexpr uint16_t SWAY_LEFT = 0x4000;
constexpr uint8_t SWAY_LEFT_START = 0x20;
constexpr uint8_t SWAY_RIGHT_START = 0x50;

class Bubble : public Object {
public:
	using Object::Object;

	void hole(); // BubbleHoleFunc
	void hole_update(); // .Update
	void bubble(); // BubbleFunc
	void rising(); // .Update
	void burst(); // .Burst
	void rise(); // .Func_47c88

private:
	void spawn(); // .asm_47c08
	void set_burst(); // .set_burst
	bool in_water_at(uint8_t dy);
	void sway(); // .asm_47ce0
	bool blocked(bool right); // a wall ahead
	void turn(bool right);
};

} // namespace

void Bubble::hole()
{
	set_update(HOLE_UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	flags() |= OBJFLAG_NO_COLLISION;
	spawn();
}

void Bubble::hole_update()
{
	no_actions();
	frame_duration() = 2;
	if (!--state_duration())
		spawn();
}

// every 208 frames, unless there's already 2 objects about (1, with
// Wario in a bubble); else tries again next frame
void Bubble::spawn()
{
	routines_.call(ADDR_CountActiveTransientObjects);
	uint8_t limit = mem_[wTransformation] == TRANSFORMATION_BUBBLE ? 1 : 2;
	if (gb_->cpu.e >= limit) {
		state_duration() = 1;
		return;
	}
	state_duration() = 208;
	play_sfx(SFX_069);
	SET_BC(gb_, ADDR_ObjParams_Bubble);
	routines_.call(ADDR_CreateObjectFromCurObjPos);
}

void Bubble::bubble()
{
	no_actions();
	flags() |= OBJFLAG_NO_COLLISION;
	if (--state_duration()) {
		rise();
		return;
	}
	set_frameset(ADDR_Frameset_690f4);
	set_update(RISING);
}

// popped when touched
void Bubble::rising()
{
	if (state())
		flags() = 0;
	else
		rise();
}

void Bubble::set_burst()
{
	set_frameset(ADDR_Frameset_6910c);
	state_duration() = 16;
	set_update(BURST);
	play_sfx(SFX_072);
}

void Bubble::burst()
{
	no_actions();
	if (!--state_duration())
		flags() = 0;
}

bool Bubble::in_water_at(uint8_t dy)
{
	uint16_t y_pos = static_cast<uint16_t>(y() - dy);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_358b);
	return gb_->cpu.a;
}

// up a pixel (on screen, every other frame; the others sway)
void Bubble::rise()
{
	if ((flags() & OBJFLAG_ON_SCREEN) && (mem_[wGlobalCounter] & 1)) {
		sway();
		return;
	}
	y() = static_cast<uint16_t>(y() - 1);
	if (in_water_at(0x18)) { // the ceiling
		set_burst();
		return;
	}
	mem_[hIsInWaterOrSandBackup] = mem_[wIsInWaterOrSand];
	in_water_at(0x0c);
	if (mem_[wIsInWaterOrSand] || mem_[hIsInWaterOrSandBackup])
		return; // still underwater
	set_burst();
}

void Bubble::turn(bool right)
{
	if (right)
		substate() |= OBJSUBFLAG_HDIR;
	else
		substate() &= ~OBJSUBFLAG_HDIR;
	var1() = 0x30;
	var2() = 0;
}

bool Bubble::blocked(bool right)
{
	uint16_t y_pos = static_cast<uint16_t>(y() - 0x18);
	uint16_t x_pos = right ? static_cast<uint16_t>(x() + 0x0b) : static_cast<uint16_t>(x() - 0x0c);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a & 0x0f)
		return true;
	y_pos = static_cast<uint16_t>(y() - 7);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	routines_.call(ADDR_Func_3513);
	return gb_->cpu.a & 0x0f;
}

// 48 frames each way, easing along the speed list; turns at walls
void Bubble::sway()
{
	if (!--var1()) {
		substate() ^= OBJSUBFLAG_HDIR;
		var1() = 0x30;
		var2() = 0;
		return;
	}
	bool right = facing_right();
	if (blocked(right)) {
		turn(!right);
		return;
	}
	uint8_t start = right ? SWAY_RIGHT_START : SWAY_LEFT_START;
	uint8_t c = static_cast<uint8_t>(var2() + start);
	uint8_t speed = mem_[static_cast<uint16_t>(SWAY_LEFT | c)];
	if (speed == 0x80)
		speed = mem_[static_cast<uint16_t>(SWAY_LEFT | static_cast<uint8_t>(c - 1))];
	else
		++var2();
	if (!speed)
		return;
	unsigned sum = mem_[wCurObjXPos + 0] + speed;
	mem_[wCurObjXPos + 0] = static_cast<uint8_t>(sum);
	if (right) {
		if (sum > 0xff)
			++mem_[wCurObjXPos + 1];
	} else if (sum <= 0xff) {
		--mem_[wCurObjXPos + 1];
	}
}

} // namespace wl3::level

using Bubble = wl3::level::Bubble;

PORT_EX(BubbleHoleFunc, 0, ALL_REGISTERS) { Bubble(gb).hole(); }
PORT_LOCAL_EX(BubbleHoleFunc, Update, 0, ALL_REGISTERS) { Bubble(gb).hole_update(); }
PORT_EX(BubbleFunc, 0, ALL_REGISTERS) { Bubble(gb).bubble(); }
PORT_LOCAL_EX(BubbleFunc, Update, 0, ALL_REGISTERS) { Bubble(gb).rising(); }
PORT_LOCAL_EX(BubbleFunc, Burst, 0, ALL_REGISTERS) { Bubble(gb).burst(); }
PORT_LOCAL_EX(BubbleFunc, Func_47c88, 0, ALL_REGISTERS) { Bubble(gb).rise(); }
