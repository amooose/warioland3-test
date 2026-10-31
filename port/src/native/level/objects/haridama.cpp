// Haridama (haridama.asm): a spiny ball crawling back and forth that
// curls up and rolls when Wario's near ahead of it; knocked into water
// it floats up and drifts about. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t INIT = ADDR_HaridamaFunc + 0x0a; // .Init
constexpr uint16_t UPDATE = ADDR_HaridamaFunc + 0xed; // .Update
constexpr uint16_t IN_WATER = ADDR_HaridamaFunc + 0x206; // .Func_490e8

constexpr uint8_t TURNING = 0x5a;
constexpr uint8_t CURLING = 0x5b;
constexpr uint8_t ROLLING = 0x32;
constexpr uint8_t FLOATING = 0x38;

class Haridama : public Object {
public:
	using Object::Object;

	void start() { set_update(INIT); } // HaridamaFunc
	void init(); // .Init
	void update(); // .Update
	void crawl(); // .Func_48f28
	void floating(); // .Func_49094
	void in_water(); // .Func_490e8
	void drift_right(); // .Func_49184
	bool wall(bool right); // .Func_49199, .Func_491b6

private:
	void turn(uint16_t frameset); // .asm_48f8e
	void curl(uint16_t frameset); // .asm_48fa2
	void uncurl(); // .Func_48fae
	void reset(); // .asm_49054
	void roll_on(bool right); // .asm_48ff6
	void hit_from(bool right_side); // .asm_49013 / .asm_49006
	void drift_left(); // .asm_4916a
};

} // namespace

void Haridama::init()
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	set_update(UPDATE);
	mem_[wCurObjCollBoxRight] = 0x07;
	mem_[wCurObjCollBoxLeft] = 0xf8;
	mem_[wCurObjCollBoxBottom] = 0xfd;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	if (static_cast<uint8_t>(screen_x() + 0x2a) < wario) {
		substate() |= OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_69290);
	} else {
		substate() &= ~OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_6927f);
	}
}

// a wall 8 pixels up and 9 to the left, or 8 to the right
bool Haridama::wall(bool right)
{
	uint16_t y_pos = static_cast<uint16_t>(y() - 8);
	uint16_t x_pos = right ? static_cast<uint16_t>(x() + 8) : static_cast<uint16_t>(x() - 9);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	return gb_->cpu.a & 0x0f;
}

void Haridama::turn(uint16_t frameset)
{
	set_frameset(frameset);
	state_duration() = 0x1b;
	state() = TURNING;
	set_interaction(OBJ_INTERACTION_06);
}

void Haridama::curl(uint16_t frameset)
{
	set_frameset(frameset);
	state_duration() = 0x12;
	state() = CURLING;
}

// crawls slowly, turning at walls; curls up when Wario's level with it
// and close ahead
void Haridama::crawl()
{
	bool right = facing_right();
	uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
	if (dy < 0x08 || dy >= 0xe8) {
		uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
		if (right ? dx >= 0xc0 : dx < 0x40) {
			curl(right ? ADDR_Frameset_692ce : ADDR_Frameset_692c7);
			return;
		}
	}
	if (!wall(right)) {
		routines_.call(right ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
		return;
	}
	turn(right ? ADDR_Frameset_692aa : ADDR_Frameset_692a1);
}

void Haridama::uncurl()
{
	if (--state_duration())
		return;
	set_interaction(OBJ_INTERACTION_HARIDAMA);
	state() = ROLLING;
	set_frameset(facing_right() ? ADDR_Frameset_692c2 : ADDR_Frameset_692bd);
}

void Haridama::reset()
{
	state() = 0;
	set_frameset(facing_right() ? ADDR_Frameset_69290 : ADDR_Frameset_6927f);
}

void Haridama::roll_on(bool right)
{
	turn(right ? ADDR_Frameset_692aa : ADDR_Frameset_692a1);
}

// hit from the right (state 2) or left (3): rolls away, or turns if it
// was rolling that way
void Haridama::hit_from(bool right_side)
{
	state() = ROLLING;
	bool right = facing_right();
	if (right != right_side) {
		roll_on(right);
		return;
	}
	set_frameset(right_side ? ADDR_Frameset_692b8 : ADDR_Frameset_692b3);
}

void Haridama::update()
{
	uint8_t s = state();
	if (!s) {
		crawl();
		return;
	}
	if (s == TURNING) { // turns round halfway through
		if (state_duration() == 0x0d) {
			--state_duration();
			substate() ^= OBJSUBFLAG_HDIR;
			return;
		}
		if (!--state_duration())
			reset();
		return;
	}
	if (s == CURLING) {
		uncurl();
		return;
	}
	if (s == 0x02) {
		hit_from(true);
		return;
	}
	if (s == 0x03) {
		hit_from(false);
		return;
	}
	switch (s & 0xfe) {
	case 0x08: // pounded
		if (!(mem_[wCurObjUnk1d] & 0x80)) {
			hit_from(facing_right());
			return;
		}
		set_interaction(OBJ_INTERACTION_06);
		state() = FLOATING;
		set_frameset(facing_right() ? ADDR_Frameset_692de : ADDR_Frameset_692d5);
		return;
	case ROLLING: {
		bool right = facing_right();
		if (!wall(right)) {
			routines_.call(right ? ADDR_MoveObjectRight_Fast : ADDR_MoveObjectLeft_Fast);
			return;
		}
		turn(right ? ADDR_Frameset_692aa : ADDR_Frameset_692a1);
		return;
	}
	case FLOATING:
		floating();
		return;
	default:
		roll_on(facing_right());
		return;
	}
}

// up through water until its top's out of it
void Haridama::floating()
{
	uint16_t y_pos = static_cast<uint16_t>(y() - 0x11);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	uint16_t x_pos = static_cast<uint16_t>(x() - 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_358b);
	if (!gb_->cpu.a && mem_[wIsInWaterOrSand]) {
		mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
		mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
		x_pos = static_cast<uint16_t>(x() + 7);
		mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
		mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
		routines_.call(ADDR_Func_358b);
		if (!gb_->cpu.a) {
			routines_.call(ADDR_MoveObjectUp_Slow);
			return;
		}
	}
	set_update(IN_WATER);
	state_duration() = 0x64;
	state() = 0;
}

// drifting for a while (a pixel every 4 frames), pushed by hits
void Haridama::in_water()
{
	uint8_t s = state();
	if (!s) {
		if (!--state_duration()) {
			set_update(UPDATE);
			bool right = facing_right();
			uint8_t f = frame();
			if (right ? f >= 0x1a : f < 0x1a) {
				reset();
				return;
			}
			if (right)
				substate() &= ~OBJSUBFLAG_HDIR;
			else
				substate() |= OBJSUBFLAG_HDIR;
			turn(right ? ADDR_Frameset_692a1 : ADDR_Frameset_692aa);
			return;
		}
		if (mem_[wGlobalCounter] & 0b11)
			return;
		bool right = facing_right();
		if (!wall(right)) {
			right ? move_right() : move_left();
			return;
		}
		substate() ^= 0x80;
		return;
	}
	if (s == 0x32) {
		drift_left();
		return;
	}
	if (s == 0x33) {
		drift_right();
		return;
	}
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	if (static_cast<uint8_t>(screen_x() + 0x2a) < wario) {
		state() = 0x32;
		substate() &= ~OBJSUBFLAG_HDIR;
	} else {
		state() = 0x33;
		substate() |= OBJSUBFLAG_HDIR;
	}
	var1() = 0x3c;
}

void Haridama::drift_left()
{
	if (!--var1()) {
		state() = 0;
		return;
	}
	if (!wall(false)) {
		move_left();
		return;
	}
	state() = 0x33;
	substate() |= OBJSUBFLAG_HDIR;
}

void Haridama::drift_right()
{
	if (!--var1()) {
		state() = 0;
		return;
	}
	if (!wall(true)) {
		move_right();
		return;
	}
	state() = 0x32;
	substate() &= ~OBJSUBFLAG_HDIR;
}

} // namespace wl3::level

using Haridama = wl3::level::Haridama;

PORT_EX(HaridamaFunc, 0, ALL_REGISTERS) { Haridama(gb).start(); }
PORT_LOCAL_EX(HaridamaFunc, Init, 0, ALL_REGISTERS) { Haridama(gb).init(); }
PORT_LOCAL_EX(HaridamaFunc, Func_48f28, 0, ALL_REGISTERS) { Haridama(gb).crawl(); }
PORT_LOCAL_EX(HaridamaFunc, Update, 0, ALL_REGISTERS) { Haridama(gb).update(); }
PORT_LOCAL_EX(HaridamaFunc, Func_49094, 0, ALL_REGISTERS) { Haridama(gb).floating(); }
PORT_LOCAL_EX(HaridamaFunc, Func_490e8, 0, ALL_REGISTERS) { Haridama(gb).in_water(); }
PORT_LOCAL_EX(HaridamaFunc, Func_49184, 0, ALL_REGISTERS) { Haridama(gb).drift_right(); }
PORT_LOCAL_EX(HaridamaFunc, Func_49199, 0, ALL_REGISTERS) { Haridama(gb).wall(false); }
PORT_LOCAL_EX(HaridamaFunc, Func_491b6, 0, ALL_REGISTERS) { Haridama(gb).wall(true); }
