// Kushimushi (kushimushi.asm): a spiny bug crawling back and forth along
// a wall (vertical) or a ceiling (horizontal), its spikes out to the side
// it's turning towards. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t VERTICAL = ADDR_KushimushiVerticalFunc + 0x29; // .Update
constexpr uint16_t HORIZONTAL = ADDR_KushimushiVerticalFunc + 0x10b; // .Func_42acf

// a leg of the patrol, by var1: crawling (the state duration, a pixel
// every other frame) or turning around, then the next
struct Leg {
	bool crawl;
	uint16_t frameset; // the next leg's
	uint8_t interaction; // the next leg's
	uint8_t duration; // the next leg's
};

constexpr Leg VERTICAL_LEGS[] = {
	{true, ADDR_Frameset_6863f, OBJ_INTERACTION_01, 22}, // .move_down
	{false, ADDR_Frameset_68616, OBJ_INTERACTION_TOP_STING, 96}, // .turn_around_up
	{true, ADDR_Frameset_68634, OBJ_INTERACTION_01, 22}, // .move_up
	{false, ADDR_Frameset_6861b, OBJ_INTERACTION_BOTTOM_STING, 96}, // .turn_around_down
};

constexpr Leg HORIZONTAL_LEGS[] = {
	{true, ADDR_Frameset_68655, OBJ_INTERACTION_01, 22}, // .move_right
	{false, ADDR_Frameset_6860c, OBJ_INTERACTION_FRONT_STING, 96}, // .turn_around_left
	{true, ADDR_Frameset_6864a, OBJ_INTERACTION_01, 22}, // .move_left
	{false, ADDR_Frameset_68611, OBJ_INTERACTION_FRONT_STING, 96}, // .turn_around_right
};

class Kushimushi : public Object {
public:
	using Object::Object;

	void init_vertical(); // KushimushiVerticalFunc
	void init_horizontal(); // KushimushiHorizontalFunc
	void update(bool horizontal); // .Update, .Func_42acf

private:
	void knocked(uint16_t routine); // .asm_42a4b
	void patrol(bool horizontal);
};

} // namespace

void Kushimushi::init_vertical()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(VERTICAL);
	set_frameset(ADDR_Frameset_6861b);
	state_duration() = 0x60;
	var1() = 0;
	var2() = 0;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 3;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-4);
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-3);
}

void Kushimushi::init_horizontal()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(HORIZONTAL);
	set_frameset(ADDR_Frameset_68611);
	state_duration() = 0x60;
	var1() = 0;
	var2() = 0;
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | OBJSUBFLAG_HDIR;
	mem_[wCurObjCollBoxRight] = 4;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-5);
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-4);
}

// knocked off by Wario: routine (Func_33f8 and the like) takes over
void Kushimushi::knocked(uint16_t routine)
{
	flags() |= OBJFLAG_NO_COLLISION;
	set_update(routine);
	substate() &= 0xf0;
	var3() = 0;
	var2() = 0x02;
	no_actions();
	set_frameset(ADDR_Frameset_68698);
}

void Kushimushi::update(bool horizontal)
{
	uint8_t s = state();
	if (!s) {
		patrol(horizontal);
		return;
	}
	bool stunned = horizontal ? (s == 0x0b || s == 0x0c) : (s == 0x0e || s == 0x0d);
	if (stunned) { // curled up for 60 frames
		state() = 0;
		var2() = 0x3c;
		if (horizontal)
			set_frameset(var1() ? ADDR_Frameset_68620 : ADDR_Frameset_68625);
		else
			set_frameset(var1() ? ADDR_Frameset_6862a : ADDR_Frameset_6862f);
		return;
	}
	switch (s) {
	case 0x0a: routines_.call(ADDR_VanishObject2); return;
	case 0x28: knocked(ADDR_Func_33f8); return;
	case 0x29: knocked(ADDR_Func_3407); return;
	case 0x04: knocked(ADDR_Func_3326); return;
	case 0x05: knocked(ADDR_Func_3317); return;
	default: knocked((s & 1) ? ADDR_Func_33f8 : ADDR_Func_3407); return;
	}
}

void Kushimushi::patrol(bool horizontal)
{
	if (var2()) { // curled up
		--var2();
		if (!horizontal)
			frame_duration() = 2;
		return;
	}
	if (mem_[wGlobalCounter] & 1)
		return;
	uint8_t leg = var1();
	if (leg > 3)
		return;
	const Leg &l = (horizontal ? HORIZONTAL_LEGS : VERTICAL_LEGS)[leg];
	if (--state_duration()) {
		if (!l.crawl)
			return;
		if (horizontal)
			leg == 0 ? move_right() : move_left();
		else
			leg == 0 ? move_down() : move_up();
		return;
	}
	if (horizontal && !l.crawl)
		substate() ^= OBJSUBFLAG_HDIR;
	state_duration() = l.duration;
	var1() = static_cast<uint8_t>((leg + 1) & 3);
	set_interaction(l.interaction);
	set_frameset(l.frameset);
}

} // namespace wl3::level

using Kushimushi = wl3::level::Kushimushi;

PORT_EX(KushimushiVerticalFunc, 0, ALL_REGISTERS) { Kushimushi(gb).init_vertical(); }
PORT_LOCAL_EX(KushimushiVerticalFunc, Update, 0, ALL_REGISTERS) { Kushimushi(gb).update(false); }
PORT_LOCAL_EX(KushimushiVerticalFunc, Func_42acf, 0, ALL_REGISTERS) { Kushimushi(gb).update(true); }
PORT_EX(KushimushiHorizontalFunc, 0, ALL_REGISTERS) { Kushimushi(gb).init_horizontal(); }
