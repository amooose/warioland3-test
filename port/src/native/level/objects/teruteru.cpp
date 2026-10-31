// Teruteru (teruteru.asm): a ghost that hovers back and forth, swoops
// down on Wario when he's just below it, and turns him blind; it shows
// only while he isn't. Hit, it's knocked about and carried like other
// enemies. Its state picks a handler from a table of 95, many of them the
// shared object states (home routines that load their own registers).
// See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_TeruteruFunc + offset); }

constexpr uint16_t INIT = L(0x0a); // .Init
constexpr uint16_t UPDATE = L(0x2c); // .Update
constexpr uint16_t STATE_5B = L(0xfa); // .Func_449d1
constexpr uint16_t ON_WARIO = L(0x100); // .Func_449d7
constexpr uint16_t SWOOP_START = L(0x132); // .Func_44a09
constexpr uint16_t SWOOPING = L(0x14e); // .Func_44a25
constexpr uint16_t RISING = L(0x186); // .Func_44a5d
constexpr uint16_t RESET = L(0x19b); // .Func_44a72
constexpr uint16_t HOVER = L(0x1c0); // .Func_44a97
constexpr uint16_t HOVERING = L(0x1ea); // .Func_44ac1
constexpr uint16_t TURNING = L(0x256); // .Func_44b2d
constexpr uint16_t HIT_LEFT = L(0x28a); // .Func_44b61
constexpr uint16_t HIT_LEFT_MOVE = L(0x2a8); // .Func_44b7f
constexpr uint16_t HIT_RIGHT = L(0x2b6); // .Func_44b8d
constexpr uint16_t HIT_RIGHT_MOVE = L(0x2d4); // .Func_44bab
constexpr uint16_t STATE_44 = L(0x2e2); // .Func_44bb9
constexpr uint16_t STATE_45 = L(0x2e9); // .Func_44bc0
constexpr uint16_t DROPPED = L(0x30a); // .Func_44be1
constexpr uint16_t DAZED = L(0x321); // .Func_44bf8
constexpr uint16_t FLOAT_UP_START = L(0x33b); // .Func_44c12
constexpr uint16_t FLOAT_UP = L(0x35b); // .Func_44c32
constexpr uint16_t STUNNED_38 = L(0x368); // .Func_44c3f
constexpr uint16_t STUNNED_39 = L(0x372); // .Func_44c49
constexpr uint16_t KNOCKED_59 = L(0x399); // .Func_44c70
constexpr uint16_t KNOCKED_58 = L(0x3a3); // .Func_44c7a
constexpr uint16_t KNOCKED_34 = L(0x3ad); // .Func_44c84
constexpr uint16_t KNOCKED_35 = L(0x3b7); // .Func_44c8e
constexpr uint16_t STATE_36 = L(0x3d3); // .Func_44caa
constexpr uint16_t STATE_37 = L(0x3e6); // .Func_44cbd
constexpr uint16_t GRAB_49 = L(0x3f9); // .Func_44cd0
constexpr uint16_t HELD_49 = L(0x418); // .Func_44cef
constexpr uint16_t GRAB_48 = L(0x428); // .Func_44cff
constexpr uint16_t HELD_48 = L(0x43f); // .Func_44d16
constexpr uint16_t STATE_42 = L(0x450); // .Func_44d27

// .Update's jumptable, by state
constexpr uint16_t HANDLERS[] = {
	RESET, HOVER, STUNNED_38, STUNNED_39, KNOCKED_34, KNOCKED_35, STATE_36, STATE_37, // $00
	STUNNED_38, STUNNED_39, ADDR_VanishObject2, RESET, RESET, RESET, STATE_5B, DROPPED,
	FLOAT_UP_START, RESET, STATE_42, RESET, STATE_44, STATE_45, HIT_LEFT, HIT_RIGHT, // $10
	GRAB_48, GRAB_49, ADDR_Func_3263, ADDR_Func_3272, ADDR_Func_3281, RESET, RESET, RESET,
	ADDR_Func_3191, ADDR_Func_31af, ADDR_Func_31cd, ADDR_Func_31eb, ADDR_Func_3209, ADDR_Func_3227, ADDR_Func_3245, ADDR_Func_3254, // $20
	KNOCKED_58, KNOCKED_59, RESET, RESET, RESET, SWOOP_START, SWOOPING, RISING,
	ADDR_ObjState_Fall, HOVERING, ADDR_ObjState_BumpLeft, ADDR_ObjState_BumpRight, ADDR_Func_3326, ADDR_Func_3317, ADDR_Func_3335, ADDR_Func_3344, // $30
	ADDR_Func_31fa, ADDR_Func_31dc, RESET, RESET, RESET, RESET, RESET, DAZED,
	FLOAT_UP, ADDR_ObjState_StandingFall, ADDR_Func_3371, RESET, ADDR_Func_32cc, ADDR_Func_32db, HIT_LEFT_MOVE, HIT_RIGHT_MOVE, // $40
	HELD_48, HELD_49, ADDR_Func_3380, ADDR_Func_338f, ADDR_ObjState_FullThrowLeft, ADDR_ObjState_FullThrowRight, ADDR_Func_339e, ADDR_Func_33ad,
	ADDR_Func_31a0, ADDR_Func_31be, ADDR_Func_31dc, ADDR_Func_31fa, ADDR_Func_3218, ADDR_Func_3236, ADDR_Func_32cc, ADDR_Func_32db, // $50
	ADDR_Func_33f8, ADDR_Func_3407, TURNING, ON_WARIO, RESET,
};
constexpr uint8_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

class Teruteru : public Object {
public:
	using Object::Object;

	void start() { set_update(INIT); } // TeruteruFunc
	void init(); // .Init
	void update(); // .Update
	void run(uint16_t handler);

private:
	bool blind() const { return mem_[wTransformation] == TRANSFORMATION_BLIND; }
	void hover(); // .Func_44a97
	void hovering(); // .Func_44ac1
	void settle(); // .asm_44b4c
	void stunned(uint8_t s); // .asm_44c51
	void knocked(uint8_t s); // .asm_44c96
	void hit(uint8_t s, uint16_t move); // .Func_44b61, .Func_44b8d
	void hit_move(uint16_t routine); // .Func_44b7f, .Func_44bab
	void grab(uint8_t s);
	void set_box(uint8_t top, uint8_t bottom);
	void set_state_then(uint8_t s, uint8_t var2_, uint16_t routine);
};

} // namespace

void Teruteru::init()
{
	if (!blind())
		flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED & 0xf0) | 0x08;
	run(RESET);
}

// unseen while Wario's blind
void Teruteru::update()
{
	if (blind())
		flags() |= OBJFLAG_INVISIBLE;
	else
		flags() &= ~OBJFLAG_INVISIBLE;
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

void Teruteru::set_box(uint8_t top, uint8_t bottom)
{
	mem_[wCurObjCollBoxTop] = top;
	mem_[wCurObjCollBoxBottom] = bottom;
}

void Teruteru::hover()
{
	state() = 0x31;
	uint16_t frameset = facing_right() ? ADDR_Frameset_68b6d : ADDR_Frameset_68b64;
	var3() = 0;
	set_frameset(frameset);
	var1() = 0x32;
	set_interaction(OBJ_INTERACTION_01);
	set_box(0xec, 0xf1);
}

// back and forth, bobbing; swoops when Wario's just below it
void Teruteru::hovering()
{
	if (!blind()) {
		uint8_t below = static_cast<uint8_t>(static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a) - static_cast<uint8_t>(screen_y() + 0x2a));
		if (below >= 0x18 && below < 0x80) {
			uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
			if (dx >= 0xe8 || dx < 0x18) {
				state() = 0x2d;
				set_frameset(ADDR_Frameset_68b84);
				state_duration() = 20;
				return;
			}
		}
	}
	SET_BC(gb_, ADDR_Data_604c0);
	routines_.call(ADDR_ApplyObjYMovement_Loop);
	uint16_t frameset;
	if (facing_right()) {
		if (++var1() != 0x64) {
			move_right();
			return;
		}
		frameset = ADDR_Frameset_68b7d;
	} else {
		if (--var1()) {
			move_left();
			return;
		}
		frameset = ADDR_Frameset_68b76;
	}
	set_frameset(frameset);
	state_duration() = 26;
	state() = 0x5a;
}

void Teruteru::settle()
{
	state() = 0x31;
	set_frameset(facing_right() ? ADDR_Frameset_68b6d : ADDR_Frameset_68b64);
}

void Teruteru::stunned(uint8_t s)
{
	mem_[wCurObjAction] = 0x8f;
	state() = s;
	set_frameset(ADDR_Frameset_68ba5);
	state_duration() = 4;
	var2() = 0x02;
	var3() = 0;
	set_interaction(OBJ_INTERACTION_00);
	set_box(0xf9, 0xff);
}

void Teruteru::knocked(uint8_t s)
{
	state() = s;
	substate() &= 0xf0;
	var3() = 0;
	var2() = 0x02;
	set_frameset(ADDR_Frameset_68ba2);
	no_actions();
}

void Teruteru::hit_move(uint16_t routine)
{
	if (var2())
		routines_.call(routine);
	else
		state() = 0x0f;
}

// a hit takes one of its hits left (its last, and it's gone)
void Teruteru::hit(uint8_t s, uint16_t move)
{
	state() = s;
	SET_HL(gb_, wCurObjSubState); // it takes h (and sets l)
	routines_.call(ADDR_DecrementRemainingBossHits);
	if ((substate() & 0x0f) == 1) {
		routines_.call(ADDR_VanishObject2);
		return;
	}
	set_frameset(ADDR_Frameset_68ba5);
	set_interaction(OBJ_INTERACTION_00);
	hit_move(move);
}

void Teruteru::grab(uint8_t s)
{
	state() = s;
	set_frameset(ADDR_Frameset_68ba5);
	set_box(0xf9, 0xff);
	flags() |= 1 << OBJFLAG_GRABBED_F;
}

void Teruteru::set_state_then(uint8_t s, uint8_t var2_, uint16_t routine)
{
	state() = s;
	var3() = 0x41;
	var2() = var2_;
	var1() = 0x01;
	routines_.call(routine);
}

void Teruteru::run(uint16_t handler)
{
	switch (handler) {
	case STATE_5B:
		state() = 0x5b;
		return;
	case ON_WARIO: // on his head while he's blind
		no_actions();
		if (!blind()) {
			run(mem_[wDirection] ? STUNNED_39 : STUNNED_38);
			return;
		}
		flags() &= ~OBJFLAG_INVISIBLE;
		{
			uint16_t wario_y = static_cast<uint16_t>((mem_[wWarioYPos + 0] << 8 | mem_[wWarioYPos + 1]) - 0x0c);
			y() = wario_y;
			x() = static_cast<uint16_t>(mem_[wWarioXPos + 0] << 8 | mem_[wWarioXPos + 1]);
		}
		return;
	case SWOOP_START:
		if (--state_duration())
			return;
		state() = 0x2e;
		set_frameset(ADDR_Frameset_68b8b);
		state_duration() = 40;
		set_interaction(OBJ_INTERACTION_TERUTERU);
		return;
	case SWOOPING: // down, after Wario
		if (!--state_duration()) {
			state() = 0x2f;
			set_frameset(ADDR_Frameset_68bae);
			state_duration() = 80;
			set_interaction(OBJ_INTERACTION_01);
			return;
		}
		{
			uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
			uint8_t ghost = static_cast<uint8_t>(screen_x() + 0x2a);
			if (ghost < wario)
				move_right();
			else if (ghost != wario)
				move_left();
		}
		routines_.call(ADDR_MoveObjectDown_Fast);
		return;
	case RISING:
		if (state_duration() == 0x3c)
			set_frameset(ADDR_Frameset_68b90);
		if (--state_duration())
			move_up();
		else
			settle();
		return;
	case RESET: // facing Wario
		mem_[wCurObjCollBoxRight] = 0x06;
		mem_[wCurObjCollBoxLeft] = 0xf9;
		set_box(0xec, 0xf1);
		{
			uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
			if (static_cast<uint8_t>(screen_x() + 0x2a) < wario)
				substate() |= OBJSUBFLAG_HDIR;
			else
				substate() &= ~OBJSUBFLAG_HDIR;
		}
		hover();
		return;
	case HOVER: hover(); return;
	case HOVERING: hovering(); return;
	case TURNING:
		SET_BC(gb_, ADDR_Data_604c0);
		routines_.call(ADDR_ApplyObjYMovement_Loop);
		if (state_duration() == 9) {
			--state_duration();
			substate() ^= OBJSUBFLAG_HDIR;
			return;
		}
		if (!--state_duration())
			settle();
		return;
	case HIT_LEFT: hit(0x46, ADDR_Func_32ae); return;
	case HIT_LEFT_MOVE: hit_move(ADDR_Func_32ae); return;
	case HIT_RIGHT: hit(0x47, ADDR_Func_32bd); return;
	case HIT_RIGHT_MOVE: hit_move(ADDR_Func_32bd); return;
	case STATE_44:
	case STATE_45:
		state() = handler == STATE_44 ? 0x44 : 0x45;
		set_frameset(ADDR_Frameset_68ba5);
		var2() = 0x02;
		var3() = 0;
		set_interaction(OBJ_INTERACTION_00);
		set_box(0xf9, 0xff);
		return;
	case DROPPED:
		state() = 0x3f;
		state_duration() = 0x64;
		flags() &= ~(1 << OBJFLAG_GRABBED_F);
		substate() = (substate() & OBJSUBFLAGS_MASK) | 0x08;
		run(DAZED);
		return;
	case DAZED:
		if (mem_[wGroundShakeCounter] >= 0x10) {
			state() = 0x1c;
			return;
		}
		if (--state_duration())
			routines_.call(ADDR_Func_32ea);
		else
			state() = 0x10;
		return;
	case FLOAT_UP_START:
		state() = 0x40;
		var3() = 0;
		set_frameset(ADDR_Frameset_68b99);
		state_duration() = 80;
		set_interaction(OBJ_INTERACTION_01);
		set_box(0xec, 0xf1);
		run(FLOAT_UP);
		return;
	case FLOAT_UP:
		if (--state_duration())
			move_up();
		else
			state() = 0;
		return;
	case STUNNED_38: stunned(0x38); return;
	case STUNNED_39: stunned(0x39); return;
	case KNOCKED_59: knocked(0x59); return;
	case KNOCKED_58: knocked(0x58); return;
	case KNOCKED_34: knocked(0x34); return;
	case KNOCKED_35: knocked(0x35); return;
	case STATE_36: set_state_then(0x36, 0xc0, ADDR_Func_3335); return;
	case STATE_37: set_state_then(0x37, 0xe0, ADDR_Func_3344); return;
	case GRAB_49:
		grab(0x49);
		substate() = (substate() & OBJSUBFLAGS_MASK) | 0x08;
		run(HELD_49);
		return;
	case HELD_49: // carried facing the way Wario does
		if (mem_[wDirection]) {
			routines_.call(ADDR_Func_3353);
			return;
		}
		substate() ^= OBJSUBFLAG_HDIR;
		run(GRAB_48);
		return;
	case GRAB_48:
		grab(0x48);
		run(HELD_48);
		return;
	case HELD_48:
		if (!mem_[wDirection]) {
			routines_.call(ADDR_Func_3362);
			return;
		}
		substate() ^= OBJSUBFLAG_HDIR;
		run(GRAB_49);
		return;
	case STATE_42:
		mem_[wCurObjAction] = 0x9f;
		state() = 0x42;
		var3() = 0;
		set_interaction(OBJ_INTERACTION_00);
		return;
	default: // a shared object state
		routines_.call(handler);
		return;
	}
}

} // namespace wl3::level

using Teruteru = wl3::level::Teruteru;

PORT_EX(TeruteruFunc, 0, ALL_REGISTERS) { Teruteru(gb).start(); }
PORT_LOCAL_EX(TeruteruFunc, Init, 0, ALL_REGISTERS) { Teruteru(gb).init(); }
PORT_LOCAL_EX(TeruteruFunc, Update, 0, ALL_REGISTERS) { Teruteru(gb).update(); }
PORT_LOCAL_EX(TeruteruFunc, Func_449d1, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::STATE_5B); }
PORT_LOCAL_EX(TeruteruFunc, Func_449d7, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::ON_WARIO); }
PORT_LOCAL_EX(TeruteruFunc, Func_44a09, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::SWOOP_START); }
PORT_LOCAL_EX(TeruteruFunc, Func_44a25, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::SWOOPING); }
PORT_LOCAL_EX(TeruteruFunc, Func_44a5d, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::RISING); }
PORT_LOCAL_EX(TeruteruFunc, Func_44a72, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::RESET); }
PORT_LOCAL_EX(TeruteruFunc, Func_44a97, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::HOVER); }
PORT_LOCAL_EX(TeruteruFunc, Func_44ac1, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::HOVERING); }
PORT_LOCAL_EX(TeruteruFunc, Func_44b2d, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::TURNING); }
PORT_LOCAL_EX(TeruteruFunc, Func_44b61, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::HIT_LEFT); }
PORT_LOCAL_EX(TeruteruFunc, Func_44b7f, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::HIT_LEFT_MOVE); }
PORT_LOCAL_EX(TeruteruFunc, Func_44b8d, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::HIT_RIGHT); }
PORT_LOCAL_EX(TeruteruFunc, Func_44bab, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::HIT_RIGHT_MOVE); }
PORT_LOCAL_EX(TeruteruFunc, Func_44bb9, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::STATE_44); }
PORT_LOCAL_EX(TeruteruFunc, Func_44bc0, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::STATE_45); }
PORT_LOCAL_EX(TeruteruFunc, Func_44be1, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::DROPPED); }
PORT_LOCAL_EX(TeruteruFunc, Func_44bf8, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::DAZED); }
PORT_LOCAL_EX(TeruteruFunc, Func_44c12, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::FLOAT_UP_START); }
PORT_LOCAL_EX(TeruteruFunc, Func_44c32, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::FLOAT_UP); }
PORT_LOCAL_EX(TeruteruFunc, Func_44c3f, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::STUNNED_38); }
PORT_LOCAL_EX(TeruteruFunc, Func_44c49, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::STUNNED_39); }
PORT_LOCAL_EX(TeruteruFunc, Func_44c70, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::KNOCKED_59); }
PORT_LOCAL_EX(TeruteruFunc, Func_44c7a, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::KNOCKED_58); }
PORT_LOCAL_EX(TeruteruFunc, Func_44c84, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::KNOCKED_34); }
PORT_LOCAL_EX(TeruteruFunc, Func_44c8e, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::KNOCKED_35); }
PORT_LOCAL_EX(TeruteruFunc, Func_44caa, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::STATE_36); }
PORT_LOCAL_EX(TeruteruFunc, Func_44cbd, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::STATE_37); }
PORT_LOCAL_EX(TeruteruFunc, Func_44cd0, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::GRAB_49); }
PORT_LOCAL_EX(TeruteruFunc, Func_44cef, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::HELD_49); }
PORT_LOCAL_EX(TeruteruFunc, Func_44cff, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::GRAB_48); }
PORT_LOCAL_EX(TeruteruFunc, Func_44d16, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::HELD_48); }
PORT_LOCAL_EX(TeruteruFunc, Func_44d27, 0, ALL_REGISTERS) { Teruteru(gb).run(wl3::level::STATE_42); }
