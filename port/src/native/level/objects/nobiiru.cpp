// Nobiiru (nobiiru.asm): a plant in a wall that stretches out to sting,
// stays out a while, and shrinks back, over and over. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t UPDATE = ADDR_NobiiruFunc + 0x37; // .Func_4380e

// A step: once its duration's up, the plant moves by var2 (out, or back
// in), takes a frameset and collision box for the way it faces, and waits
struct Step {
	uint8_t var2; // set first, if not 0
	bool out; // moves the way it faces, or back
	uint16_t left_frameset; // facing left
	uint8_t left_box_left, left_box_right;
	uint16_t right_frameset; // facing right
	uint8_t right_box_left, right_box_right;
	bool box; // sets the collision box
	uint8_t duration;
};

constexpr Step STEPS[] = {
	{0x0c, true, ADDR_Frameset_698d8, 0xff, 0x02, ADDR_Frameset_698b1, 0xfd, 0x00, true, 0x02}, // .Func_4382e
	{0x04, true, ADDR_Frameset_698db, 0xfc, 0x07, ADDR_Frameset_698b4, 0xf8, 0x03, true, 0x02}, // .Func_43876
	{0, true, ADDR_Frameset_698de, 0xf8, 0x0b, ADDR_Frameset_698b7, 0xf4, 0x07, true, 0x02}, // .Func_438a0
	{0, true, ADDR_Frameset_698e1, 0xf4, 0x0f, ADDR_Frameset_698ba, 0xf0, 0x0b, true, 0x02}, // .Func_438c5
	{0, true, ADDR_Frameset_698e4, 0xf0, 0x13, ADDR_Frameset_698bd, 0xec, 0x0f, true, 0x02}, // .Func_438ea
	{0, true, ADDR_Frameset_698e7, 0xec, 0x17, ADDR_Frameset_698c0, 0xe8, 0x13, true, 0x14}, // .Func_43910
	{0, false, ADDR_Frameset_698ea, 0, 0, ADDR_Frameset_698c3, 0, 0, false, 0x08}, // .Func_43936: stays out
	{0, false, ADDR_Frameset_698ed, 0, 0, ADDR_Frameset_698c6, 0, 0, false, 0x14}, // .Func_43956
	{0, false, ADDR_Frameset_698f0, 0xf0, 0x13, ADDR_Frameset_698c9, 0xec, 0x0f, true, 0x02}, // .Func_4396e
	{0, false, ADDR_Frameset_698f3, 0xf4, 0x0f, ADDR_Frameset_698cc, 0xf0, 0x0b, true, 0x02}, // .Func_43994
	{0, false, ADDR_Frameset_698f6, 0xf8, 0x0b, ADDR_Frameset_698cf, 0xf4, 0x07, true, 0x02}, // .Func_439ba
	{0, false, ADDR_Frameset_698f9, 0xfc, 0x07, ADDR_Frameset_698d2, 0xf8, 0x03, true, 0x02}, // .Func_439e0
	{0, false, ADDR_Frameset_698fc, 0xff, 0x02, ADDR_Frameset_698d5, 0xfd, 0x00, true, 0x02}, // .Func_43a06
};
constexpr uint8_t NUM_STEPS = sizeof(STEPS) / sizeof(STEPS[0]);
constexpr uint8_t STING = 0; // the first step makes it sting
constexpr uint8_t OUT = 6; // the step it stops stinging

class Nobiiru : public Object {
public:
	using Object::Object;

	void init(); // NobiiruFunc
	void update(); // .Func_4380e

private:
	void reset(); // .asm_437f7: in, waiting 60 frames
	void step(const Step &s, uint8_t index);
};

} // namespace

void Nobiiru::init()
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	if (mem_[wCurObjId] == NOBIIRU_LEFT)
		substate() &= ~OBJSUBFLAG_HDIR;
	else
		substate() |= OBJSUBFLAG_HDIR;
	reset();
}

void Nobiiru::reset()
{
	mem_[wCurObjCollBoxBottom] = 0xfc;
	mem_[wCurObjCollBoxLeft] = 0xfd;
	mem_[wCurObjCollBoxRight] = 0x02;
	set_frameset(ADDR_Frameset_698ff);
	state_duration() = 60;
	var1() = 0;
}

void Nobiiru::update()
{
	uint8_t index = var1();
	if (index > NUM_STEPS) {
		unrealistic();
		return;
	}
	if (index == STING)
		no_actions();
	if (--state_duration())
		return;
	if (index == NUM_STEPS) { // .Func_43a2c: all the way back
		var2() = 0x0c;
		facing_right() ? move_left_by_var2() : move_right_by_var2();
		reset();
		return;
	}
	step(STEPS[index], index);
}

void Nobiiru::step(const Step &s, uint8_t index)
{
	if (index == STING)
		play_sfx(SFX_079);
	if (s.var2)
		var2() = s.var2;
	bool right = facing_right();
	if (s.box) {
		bool forwards = s.out ? right : !right;
		forwards ? move_right_by_var2() : move_left_by_var2();
	}
	if (index == STING)
		set_interaction(OBJ_INTERACTION_FULL_STING);
	else if (index == OUT)
		set_interaction(OBJ_INTERACTION_0B);
	if (s.box) {
		// the asm's sting step leaves the new interaction type in a, which
		// .SetCollisionAndFrameset then stores as the box's left
		mem_[wCurObjCollBoxLeft] = index == STING ? interaction() : (right ? s.right_box_left : s.left_box_left);
		mem_[wCurObjCollBoxRight] = right ? s.right_box_right : s.left_box_right;
	}
	set_frameset(right ? s.right_frameset : s.left_frameset);
	state_duration() = s.duration;
	++var1();
}

} // namespace wl3::level

using Nobiiru = wl3::level::Nobiiru;

PORT_EX(NobiiruFunc, 0, ALL_REGISTERS) { Nobiiru(gb).init(); }
PORT_LOCAL_EX(NobiiruFunc, Func_4380e, 0, ALL_REGISTERS) { Nobiiru(gb).update(); }
