// The hand (hand.asm): a hand that rises out of the ground (not while
// Wario's in sand), holds still a while, and sinks back, pulling Wario
// down with it if it's caught him (state $2e). See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t UPDATE = ADDR_HandFunc + 0x24; // .Update

constexpr uint8_t HOLDING = 0x2e; // has Wario
constexpr uint8_t GRABBED = 0x2f; // just caught him

// a step of the hand's cycle, by var1: once its time's up, a frameset
// and the next step's time, a move (by var2) and where Wario's held
struct Step {
	uint16_t frameset;
	uint8_t duration;
	int8_t move; // -1 up, 1 down, by var2
	uint8_t var2; // set first, if not 0
	int8_t wario_dy; // Wario's offset above it if held, or -1
};

constexpr Step STEPS[] = {
	{ADDR_Frameset_690ab, 41, 0, 0, -1}, // .Func_477b2: out
	{ADDR_Frameset_690b4, 1, 0, 0, -1}, // .Func_477de
	{ADDR_Frameset_690b7, 1, -1, 4, -1}, // .Func_477f3: rising
	{ADDR_Frameset_690ba, 1, -1, 0, -1}, // .Func_4780e
	{ADDR_Frameset_690bd, 1, -1, 0, -1}, // .Func_47825
	{ADDR_Frameset_690c0, 1, -1, 0, -1}, // .Func_4783c
	{ADDR_Frameset_690c3, 1, -1, 0, -1}, // .Func_47853
	{ADDR_Frameset_690c6, 1, -1, 0, -1}, // .Func_4786a
	{ADDR_Frameset_690c9, 56, 0, 0, -1}, // .Func_47881: up
	{ADDR_Frameset_690dc, 2, 1, 6, 9}, // .Func_47896: sinking
	{ADDR_Frameset_690df, 2, 1, 0, 8}, // .Func_478e1
	{ADDR_Frameset_690e2, 2, 1, 0, 6}, // .Func_47903
	{ADDR_Frameset_690e5, 2, 1, 0, 8}, // .Func_47925
	{ADDR_Frameset_690e8, 2, 0, 0, 0}, // .Func_47947
	{ADDR_Frameset_690eb, 41, 0, 0, -1}, // .Func_47966: down; lets go
};
constexpr uint8_t RISE = 0;
constexpr uint8_t UP = 9; // .Func_47896
constexpr uint8_t DOWN = 14; // .Func_47966
constexpr uint8_t INVISIBLE = 15; // .Invisible

class Hand : public Object {
public:
	using Object::Object;

	void init(); // HandFunc
	void update(); // .Update
	void step(uint8_t index); // .Func_477b2 and the rest
	void apply_offset(uint16_t page, uint8_t dy); // .ApplyOffsetToWario

private:
	void next(const Step &s);
};

} // namespace

void Hand::init()
{
	flags() |= OBJFLAG_NO_COLLISION;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-12);
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-6);
	mem_[wCurObjCollBoxRight] = 5;
	state_duration() = 50;
	var1() = 0;
}

// Wario at the hand's position (in page's object), dy above it
void Hand::apply_offset(uint16_t page, uint8_t dy)
{
	uint16_t y_pos = static_cast<uint16_t>(mem_.word(static_cast<uint16_t>(page | OBJ_Y_POS)) - dy);
	mem_[wWarioYPos + 1] = static_cast<uint8_t>(y_pos);
	mem_[wWarioYPos + 0] = static_cast<uint8_t>(y_pos >> 8);
	mem_[wWarioXPos + 1] = mem_[static_cast<uint16_t>(page | OBJ_X_POS)];
	mem_[wWarioXPos + 0] = mem_[static_cast<uint16_t>(page | (OBJ_X_POS + 1))];
}

void Hand::update()
{
	uint8_t s = state();
	if (s == GRABBED) {
		state() = HOLDING;
		apply_offset(wCurObj & 0xff00, 11);
	} else if (s != HOLDING) {
		state() = 0;
	}
	uint8_t index = var1();
	if (index > INVISIBLE) {
		unrealistic();
		return;
	}
	step(index);
}

void Hand::next(const Step &s)
{
	set_frameset(s.frameset);
	state_duration() = s.duration;
	++var1();
	if (s.var2)
		var2() = s.var2;
	if (s.move < 0)
		move_up_by_var2();
	else if (s.move > 0)
		move_down_by_var2();
	if (s.wario_dy >= 0 && state() == HOLDING)
		apply_offset(wCurObj & 0xff00, static_cast<uint8_t>(s.wario_dy));
}

void Hand::step(uint8_t index)
{
	if (index == INVISIBLE) { // back under, waiting
		no_actions();
		if (--state_duration())
			return;
		state_duration() = 50;
		var1() = 0;
		flags() |= OBJFLAG_INVISIBLE;
		return;
	}
	if (index == UP) {
		// up: holds for a while once Wario's below it; then Wario's held
		// lower as it starts down
		uint8_t time = state_duration();
		if (time < 26) {
			if (state() == HOLDING)
				apply_offset(wCurObj & 0xff00, 7);
		} else if (time < 49) {
			uint8_t top = static_cast<uint8_t>(screen_y() - 4);
			if (mem_[wWarioScreenYPos] < top) {
				--state_duration();
				return;
			}
		}
	}
	no_actions();
	if (index == RISE && mem_[wIsInSand]) {
		state_duration() = 50;
		return;
	}
	if (--state_duration())
		return;
	if (index == RISE)
		flags() &= ~OBJFLAG_INVISIBLE;
	next(STEPS[index]);
	if (index == RISE)
		play_sfx(SFX_081);
	if (index == DOWN && state() == HOLDING) {
		mem_[wAutoMoveState] = 0;
		state() = 0;
	}
}

} // namespace wl3::level

using Hand = wl3::level::Hand;

PORT_EX(HandFunc, 0, ALL_REGISTERS) { Hand(gb).init(); }
PORT_LOCAL_EX(HandFunc, Update, 0, ALL_REGISTERS) { Hand(gb).update(); }
PORT_LOCAL_EX(HandFunc, ApplyOffsetToWario, 0, ALL_REGISTERS) { Hand(gb).apply_offset(gb->cpu.h << 8, gb->cpu.b); }
PORT_LOCAL_EX(HandFunc, Func_477b2, 0, ALL_REGISTERS) { Hand(gb).step(0); }
PORT_LOCAL_EX(HandFunc, Func_477de, 0, ALL_REGISTERS) { Hand(gb).step(1); }
PORT_LOCAL_EX(HandFunc, Func_477f3, 0, ALL_REGISTERS) { Hand(gb).step(2); }
PORT_LOCAL_EX(HandFunc, Func_4780e, 0, ALL_REGISTERS) { Hand(gb).step(3); }
PORT_LOCAL_EX(HandFunc, Func_47825, 0, ALL_REGISTERS) { Hand(gb).step(4); }
PORT_LOCAL_EX(HandFunc, Func_4783c, 0, ALL_REGISTERS) { Hand(gb).step(5); }
PORT_LOCAL_EX(HandFunc, Func_47853, 0, ALL_REGISTERS) { Hand(gb).step(6); }
PORT_LOCAL_EX(HandFunc, Func_4786a, 0, ALL_REGISTERS) { Hand(gb).step(7); }
PORT_LOCAL_EX(HandFunc, Func_47881, 0, ALL_REGISTERS) { Hand(gb).step(8); }
PORT_LOCAL_EX(HandFunc, Func_47896, 0, ALL_REGISTERS) { Hand(gb).step(9); }
PORT_LOCAL_EX(HandFunc, Func_478e1, 0, ALL_REGISTERS) { Hand(gb).step(10); }
PORT_LOCAL_EX(HandFunc, Func_47903, 0, ALL_REGISTERS) { Hand(gb).step(11); }
PORT_LOCAL_EX(HandFunc, Func_47925, 0, ALL_REGISTERS) { Hand(gb).step(12); }
PORT_LOCAL_EX(HandFunc, Func_47947, 0, ALL_REGISTERS) { Hand(gb).step(13); }
PORT_LOCAL_EX(HandFunc, Func_47966, 0, ALL_REGISTERS) { Hand(gb).step(14); }
PORT_LOCAL_EX(HandFunc, Invisible, 0, ALL_REGISTERS) { Hand(gb).step(15); }
