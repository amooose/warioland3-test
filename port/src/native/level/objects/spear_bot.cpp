// The spear bot (spear_bot.asm): a robot walking the floor that, when
// Wario's level and close, levels its spear and charges until it meets a
// wall or an edge (turning back once, var1). Knocked about like other
// enemies, through a table of 93 state handlers. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_SpearBotFunc + offset); }

constexpr uint16_t INIT = L(0x0a); // .Init
constexpr uint16_t UPDATE = L(0x1e); // .Update
constexpr uint16_t RESET = L(0xdc); // .Func_45fbf
constexpr uint16_t STATE_41_START = L(0x118); // .Func_45ffb
constexpr uint16_t STATE_41 = L(0x160); // .Func_46043
constexpr uint16_t AIMING = L(0x17c); // .Func_4605f
constexpr uint16_t CHARGING = L(0x1c3); // .Func_460a6
constexpr uint16_t STOPPED = L(0x28d); // .Func_46170
constexpr uint16_t TURN_BACK = L(0x2a9); // .Func_4618c
constexpr uint16_t SET_WALK = L(0x2f8); // .Func_461db
constexpr uint16_t WALKING = L(0x318); // .Func_461fb
constexpr uint16_t BUMP_32 = L(0x400); // .Func_462e3
constexpr uint16_t BUMP_33 = L(0x408); // .Func_462eb
constexpr uint16_t HIT_LEFT = L(0x436); // .Func_46319
constexpr uint16_t HIT_LEFT_MOVE = L(0x45b); // .Func_4633e
constexpr uint16_t HIT_RIGHT = L(0x469); // .Func_4634c
constexpr uint16_t HIT_RIGHT_MOVE = L(0x48e); // .Func_46371
constexpr uint16_t STATE_44 = L(0x49c); // .Func_4637f
constexpr uint16_t STATE_45 = L(0x4a4); // .Func_46387
constexpr uint16_t DROPPED = L(0x4d1); // .Func_463b4
constexpr uint16_t DAZED = L(0x4e0); // .Func_463c3
constexpr uint16_t BOUNCE_START = L(0x4fa); // .Func_463dd
constexpr uint16_t BOUNCING = L(0x508); // .Func_463eb
constexpr uint16_t SET_TURN = L(0x515); // .Func_463f8
constexpr uint16_t TURNING = L(0x53e); // .Func_46421
constexpr uint16_t STUNNED_38 = L(0x56e); // .Func_46451
constexpr uint16_t STUNNED_39 = L(0x578); // .Func_4645b
constexpr uint16_t STATE_3B = L(0x5aa); // .Func_4648d
constexpr uint16_t STATE_3C = L(0x5b1); // .Func_46494
constexpr uint16_t ROLLING_3B = L(0x5c2); // .Func_464a5
constexpr uint16_t ROLLING_3C = L(0x5d4); // .Func_464b7
constexpr uint16_t KNOCKED_59 = L(0x5f0); // .Func_464d3
constexpr uint16_t KNOCKED_58 = L(0x5fa); // .Func_464dd
constexpr uint16_t KNOCKED_34 = L(0x604); // .Func_464e7
constexpr uint16_t KNOCKED_35 = L(0x60e); // .Func_464f1
constexpr uint16_t STATE_36 = L(0x634); // .Func_46517
constexpr uint16_t STATE_37 = L(0x647); // .Func_4652a
constexpr uint16_t GRAB_49 = L(0x65a); // .Func_4653d
constexpr uint16_t HELD_49 = L(0x67b); // .Func_4655e
constexpr uint16_t GRAB_48 = L(0x68b); // .Func_4656e
constexpr uint16_t HELD_48 = L(0x6ac); // .Func_4658f
constexpr uint16_t STATE_42 = L(0x6bd); // .Func_465a0

// .Update's jumptable, by state
constexpr uint16_t HANDLERS[] = {
	RESET, SET_WALK, BUMP_32, BUMP_33, KNOCKED_34, KNOCKED_35, STATE_36, STATE_37, // $00
	STUNNED_38, STUNNED_39, ADDR_VanishObject2, STATE_3B, STATE_3C, RESET, RESET, DROPPED,
	BOUNCE_START, STATE_41_START, STATE_42, SET_TURN, STATE_44, STATE_45, HIT_LEFT, HIT_RIGHT, // $10
	GRAB_48, GRAB_49, ADDR_Func_3263, ADDR_Func_3272, ADDR_Func_3281, RESET, RESET, RESET,
	ADDR_Func_3191, ADDR_Func_31af, ADDR_Func_31cd, ADDR_Func_31eb, ADDR_Func_3209, ADDR_Func_3227, ADDR_Func_3245, ADDR_Func_3254, // $20
	KNOCKED_58, KNOCKED_59, RESET, RESET, RESET, AIMING, CHARGING, RESET,
	ADDR_ObjState_Fall, WALKING, ADDR_ObjState_BumpLeft, ADDR_ObjState_BumpRight, ADDR_Func_3326, ADDR_Func_3317, ADDR_Func_3335, ADDR_Func_3344, // $30
	ADDR_Func_31fa, ADDR_Func_31dc, RESET, ROLLING_3B, ROLLING_3C, RESET, RESET, DAZED,
	BOUNCING, STATE_41, ADDR_Func_3371, TURNING, ADDR_Func_32cc, ADDR_Func_32db, HIT_LEFT_MOVE, HIT_RIGHT_MOVE, // $40
	HELD_48, HELD_49, ADDR_Func_3380, ADDR_Func_338f, ADDR_ObjState_FullThrowLeft, ADDR_ObjState_FullThrowRight, ADDR_Func_339e, ADDR_Func_33ad,
	ADDR_Func_31a0, ADDR_Func_31be, ADDR_Func_31dc, ADDR_Func_31fa, ADDR_Func_3218, ADDR_Func_3236, ADDR_Func_32cc, ADDR_Func_32db, // $50
	ADDR_Func_33f8, ADDR_Func_3407, TURN_BACK, STOPPED, RESET,
};
constexpr uint8_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

class SpearBot : public Object {
public:
	using Object::Object;

	void start() { set_update(INIT); } // SpearBotFunc
	void init(); // .Init
	void update(); // .Update
	void run(uint16_t handler);

private:
	uint16_t facing(uint16_t left, uint16_t right) { return facing_right() ? right : left; }
	bool wario_to_right() const;
	bool shaken();
	void box(uint8_t left = 0xfa, uint8_t right = 0x05);
	bool on_floor();
	bool move_on(int edge, int ahead, uint16_t move); // false: a block edge or a wall
	void hit_move(uint16_t routine);
	void hit(uint8_t s);
	void stunned(uint8_t s);
	void stop_charge(); // .asm_4610d / .asm_4614b
	void rolling(uint16_t routine, uint16_t frameset);
	void knocked(uint8_t s); // .asm_464f9
	void grab(uint8_t s);
	void set_state_then(uint8_t s, uint8_t var2_, uint16_t routine);
};

} // namespace

bool SpearBot::wario_to_right() const
{
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	return static_cast<uint8_t>(screen_x() + 0x2a) < wario;
}

void SpearBot::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	run(RESET);
}

void SpearBot::update()
{
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

bool SpearBot::shaken()
{
	if (mem_[wGroundShakeCounter] < 0x10)
		return false;
	state() = static_cast<uint8_t>(0x08 | (mem_[rDIV] & 1));
	return true;
}

void SpearBot::box(uint8_t left, uint8_t right)
{
	mem_[wCurObjCollBoxRight] = right;
	mem_[wCurObjCollBoxLeft] = left;
}

bool SpearBot::on_floor()
{
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_3543);
	if (!gb_->cpu.a) {
		state() = 0x11;
		return false;
	}
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	return true;
}

bool SpearBot::move_on(int edge, int ahead, uint16_t move)
{
	uint8_t in_block = mem_[wCurObjXPos + 0] & 0x0f;
	bool right = ahead > 0;
	if (right ? in_block + edge >= 0x10 : in_block < edge) {
		routines_.call(right ? ADDR_ProcessBlockOnRight : ADDR_ProcessBlockOnLeft);
		if (!(gb_->cpu.a & 0x0f))
			return false;
	}
	uint16_t y_pos = static_cast<uint16_t>(y() - 4);
	uint16_t x_pos = static_cast<uint16_t>(x() + ahead);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a)
		return false;
	routines_.call(move);
	return true;
}

void SpearBot::hit_move(uint16_t routine)
{
	if (var2())
		routines_.call(routine);
	else
		state() = 0x0f;
}

void SpearBot::hit(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_68e17, ADDR_Frameset_68e20));
	box();
	set_interaction(OBJ_INTERACTION_00);
}

void SpearBot::stunned(uint8_t s)
{
	mem_[wCurObjAction] = 0x8f;
	hit(s);
	state_duration() = 4;
	var2() = 0x02;
	var3() = 0;
}

// at a wall or an edge: turns back (once), else stops
void SpearBot::stop_charge()
{
	bool right = facing_right();
	if (var1()) {
		set_frameset(right ? ADDR_Frameset_68e4c : ADDR_Frameset_68e41);
		state_duration() = 36;
		--var1();
		state() = 0x5a;
		return;
	}
	set_frameset(right ? ADDR_Frameset_68e08 : ADDR_Frameset_68dff);
	state_duration() = 38;
	state() = 0x5b;
}

void SpearBot::rolling(uint16_t routine, uint16_t frameset)
{
	frame_duration() = 0;
	mem_[wCurObjFramesetOffset] = 0;
	if (var2()) {
		routines_.call(routine);
		return;
	}
	set_frameset(frameset);
	state_duration() = 38;
	state() = 0x5b;
}

void SpearBot::knocked(uint8_t s)
{
	state() = s;
	no_actions();
	uint8_t sub = substate() & 0xf0;
	substate() = sub;
	var3() = 0;
	var2() = 0x02;
	set_frameset((sub & 0x80) ? ADDR_Frameset_68e2e : ADDR_Frameset_68e29);
}

void SpearBot::grab(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_68e17, ADDR_Frameset_68e20));
	box();
	flags() |= 1 << OBJFLAG_GRABBED_F;
}

void SpearBot::set_state_then(uint8_t s, uint8_t var2_, uint16_t routine)
{
	state() = s;
	var3() = 0x41;
	var2() = var2_;
	var1() = 0x01;
	routines_.call(routine);
}

void SpearBot::run(uint16_t handler)
{
	switch (handler) {
	case RESET: { // facing Wario
		set_interaction(OBJ_INTERACTION_01);
		box();
		state() = 0x30;
		bool right = wario_to_right();
		if (right)
			substate() |= OBJSUBFLAG_HDIR;
		else
			substate() &= ~OBJSUBFLAG_HDIR;
		var3() = 0;
		set_frameset(right ? ADDR_Frameset_68de8 : ADDR_Frameset_68ddf);
		return;
	}
	case STATE_41_START: { // the spear put away, if it was out
		state() = 0x41;
		var3() = 0;
		bool right = facing_right();
		Byte edge = mem_[right ? wCurObjCollBoxRight : wCurObjCollBoxLeft];
		if (edge == (right ? 0x09 : 0xf6)) {
			edge = right ? 0x05 : 0xfa;
			set_frameset(right ? ADDR_Frameset_68e70 : ADDR_Frameset_68e69);
			state_duration() = 14;
			return;
		}
		set_frameset(right ? ADDR_Frameset_68de8 : ADDR_Frameset_68ddf);
		state_duration() = 0;
		return;
	}
	case STATE_41:
		if (!state_duration()) {
			routines_.call(ADDR_ObjState_StandingFall);
			return;
		}
		if (!--state_duration())
			set_frameset(facing(ADDR_Frameset_68ddf, ADDR_Frameset_68de8));
		return;
	case AIMING: { // then charges, its spear out (and stinging)
		if (shaken())
			return;
		if (--state_duration())
			return;
		state() = 0x2e;
		bool right = facing_right();
		if (right)
			box(0xfa, 0x09);
		else
			box(0xf6, 0x05);
		set_frameset(right ? ADDR_Frameset_68e60 : ADDR_Frameset_68e57);
		var1() = 0x02;
		set_interaction(OBJ_INTERACTION_FRONT_STING);
		return;
	}
	case CHARGING: {
		if (shaken() || !on_floor())
			return;
		bool right = facing_right();
		if (!move_on(right ? 0x05 : 0x06, right ? 0x05 : -0x06, right ? ADDR_MoveObjectRight : ADDR_MoveObjectLeft))
			stop_charge();
		return;
	}
	case STOPPED:
		if (shaken())
			return;
		if (!--state_duration())
			state() = 0x13;
		return;
	case TURN_BACK: // turns halfway, then charges the other way
		if (shaken())
			return;
		if (state_duration() == 0x0f) {
			--state_duration();
			if (facing_right()) {
				substate() &= ~OBJSUBFLAG_HDIR;
				box(0xf6, 0x05);
			} else {
				substate() |= OBJSUBFLAG_HDIR;
				box(0xfa, 0x09);
			}
			return;
		}
		if (--state_duration()) {
			routines_.call(ADDR_Func_3308);
			return;
		}
		state() = 0x2e;
		set_frameset(facing(ADDR_Frameset_68e57, ADDR_Frameset_68e60));
		return;
	case SET_WALK:
		set_interaction(OBJ_INTERACTION_01);
		state() = 0x31;
		set_frameset(facing(ADDR_Frameset_68ddf, ADDR_Frameset_68de8));
		return;
	case WALKING: { // aims when Wario's level and close; turns if he's behind
		if (shaken())
			return;
		uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
		if (dy < 0x18 || dy >= 0xe8) {
			uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
			bool near_right = dx >= 0xc0, near_left = dx < 0x40;
			if (near_right || near_left) {
				if (facing_right() != near_right) {
					state() = 0x13;
					return;
				}
				state() = 0x2d;
				set_frameset(near_right ? ADDR_Frameset_68df8 : ADDR_Frameset_68df1);
				state_duration() = 20;
				return;
			}
		}
		if (!on_floor())
			return;
		bool right = facing_right();
		if (!move_on(right ? 0x05 : 0x06, right ? 0x05 : -0x06, right ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow))
			state() = 0x13;
		return;
	}
	case BUMP_32:
	case BUMP_33:
		state() = handler == BUMP_32 ? 0x32 : 0x33;
		set_frameset(facing(ADDR_Frameset_68e11, ADDR_Frameset_68e14));
		state_duration() = 12;
		var2() = 0x02;
		box();
		set_interaction(OBJ_INTERACTION_01);
		return;
	case HIT_LEFT:
		hit(0x46);
		[[fallthrough]];
	case HIT_LEFT_MOVE:
		hit_move(ADDR_Func_32ae);
		return;
	case HIT_RIGHT:
		hit(0x47);
		[[fallthrough]];
	case HIT_RIGHT_MOVE:
		hit_move(ADDR_Func_32bd);
		return;
	case STATE_44:
	case STATE_45:
		hit(handler == STATE_44 ? 0x44 : 0x45);
		var2() = 0x02;
		var3() = 0;
		return;
	case DROPPED:
		state() = 0x3f;
		state_duration() = 0x64;
		flags() &= ~(1 << OBJFLAG_GRABBED_F);
		[[fallthrough]];
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
	case BOUNCE_START:
		state() = 0x40;
		var3() = 0;
		state_duration() = 0x07;
		[[fallthrough]];
	case BOUNCING:
		if (--state_duration())
			routines_.call(ADDR_Func_32f9);
		else
			state() = 0x00;
		return;
	case SET_TURN:
		state() = 0x43;
		set_frameset(facing(ADDR_Frameset_68e33, ADDR_Frameset_68e3a));
		state_duration() = 26;
		box();
		set_interaction(OBJ_INTERACTION_01);
		return;
	case TURNING:
		if (shaken())
			return;
		if (state_duration() == 0x09) {
			--state_duration();
			substate() ^= OBJSUBFLAG_HDIR;
			return;
		}
		if (--state_duration())
			routines_.call(ADDR_Func_3308);
		else
			state() = 0x01;
		return;
	case STUNNED_38: stunned(0x38); return;
	case STUNNED_39: stunned(0x39); return;
	case STATE_3B:
	case STATE_3C:
		state() = handler == STATE_3B ? 0x3b : 0x3c;
		frame_duration() = 0;
		mem_[wCurObjFramesetOffset] = 0;
		var2() = 0x02;
		return;
	case ROLLING_3B: rolling(ADDR_Func_32bd, ADDR_Frameset_68dff); return;
	case ROLLING_3C: rolling(ADDR_Func_32ae, ADDR_Frameset_68e08); return;
	case KNOCKED_59: knocked(0x59); return;
	case KNOCKED_58: knocked(0x58); return;
	case KNOCKED_34: knocked(0x34); return;
	case KNOCKED_35: knocked(0x35); return;
	case STATE_36: set_state_then(0x36, 0xc0, ADDR_Func_3335); return;
	case STATE_37: set_state_then(0x37, 0xe0, ADDR_Func_3344); return;
	case GRAB_49:
		grab(0x49);
		[[fallthrough]];
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
		[[fallthrough]];
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

using SpearBot = wl3::level::SpearBot;

PORT_EX(SpearBotFunc, 0, ALL_REGISTERS) { SpearBot(gb).start(); }
PORT_LOCAL_EX(SpearBotFunc, Init, 0, ALL_REGISTERS) { SpearBot(gb).init(); }
PORT_LOCAL_EX(SpearBotFunc, Update, 0, ALL_REGISTERS) { SpearBot(gb).update(); }
PORT_LOCAL_EX(SpearBotFunc, Func_45fbf, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::RESET); }
PORT_LOCAL_EX(SpearBotFunc, Func_45ffb, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::STATE_41_START); }
PORT_LOCAL_EX(SpearBotFunc, Func_46043, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::STATE_41); }
PORT_LOCAL_EX(SpearBotFunc, Func_4605f, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::AIMING); }
PORT_LOCAL_EX(SpearBotFunc, Func_460a6, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::CHARGING); }
PORT_LOCAL_EX(SpearBotFunc, Func_46170, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::STOPPED); }
PORT_LOCAL_EX(SpearBotFunc, Func_4618c, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::TURN_BACK); }
PORT_LOCAL_EX(SpearBotFunc, Func_461db, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::SET_WALK); }
PORT_LOCAL_EX(SpearBotFunc, Func_461fb, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::WALKING); }
PORT_LOCAL_EX(SpearBotFunc, Func_462e3, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::BUMP_32); }
PORT_LOCAL_EX(SpearBotFunc, Func_462eb, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::BUMP_33); }
PORT_LOCAL_EX(SpearBotFunc, Func_46319, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::HIT_LEFT); }
PORT_LOCAL_EX(SpearBotFunc, Func_4633e, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::HIT_LEFT_MOVE); }
PORT_LOCAL_EX(SpearBotFunc, Func_4634c, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::HIT_RIGHT); }
PORT_LOCAL_EX(SpearBotFunc, Func_46371, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::HIT_RIGHT_MOVE); }
PORT_LOCAL_EX(SpearBotFunc, Func_4637f, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::STATE_44); }
PORT_LOCAL_EX(SpearBotFunc, Func_46387, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::STATE_45); }
PORT_LOCAL_EX(SpearBotFunc, Func_463b4, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::DROPPED); }
PORT_LOCAL_EX(SpearBotFunc, Func_463c3, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::DAZED); }
PORT_LOCAL_EX(SpearBotFunc, Func_463dd, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::BOUNCE_START); }
PORT_LOCAL_EX(SpearBotFunc, Func_463eb, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::BOUNCING); }
PORT_LOCAL_EX(SpearBotFunc, Func_463f8, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::SET_TURN); }
PORT_LOCAL_EX(SpearBotFunc, Func_46421, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::TURNING); }
PORT_LOCAL_EX(SpearBotFunc, Func_46451, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::STUNNED_38); }
PORT_LOCAL_EX(SpearBotFunc, Func_4645b, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::STUNNED_39); }
PORT_LOCAL_EX(SpearBotFunc, Func_4648d, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::STATE_3B); }
PORT_LOCAL_EX(SpearBotFunc, Func_46494, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::STATE_3C); }
PORT_LOCAL_EX(SpearBotFunc, Func_464a5, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::ROLLING_3B); }
PORT_LOCAL_EX(SpearBotFunc, Func_464b7, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::ROLLING_3C); }
PORT_LOCAL_EX(SpearBotFunc, Func_464d3, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::KNOCKED_59); }
PORT_LOCAL_EX(SpearBotFunc, Func_464dd, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::KNOCKED_58); }
PORT_LOCAL_EX(SpearBotFunc, Func_464e7, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::KNOCKED_34); }
PORT_LOCAL_EX(SpearBotFunc, Func_464f1, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::KNOCKED_35); }
PORT_LOCAL_EX(SpearBotFunc, Func_46517, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::STATE_36); }
PORT_LOCAL_EX(SpearBotFunc, Func_4652a, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::STATE_37); }
PORT_LOCAL_EX(SpearBotFunc, Func_4653d, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::GRAB_49); }
PORT_LOCAL_EX(SpearBotFunc, Func_4655e, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::HELD_49); }
PORT_LOCAL_EX(SpearBotFunc, Func_4656e, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::GRAB_48); }
PORT_LOCAL_EX(SpearBotFunc, Func_4658f, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::HELD_48); }
PORT_LOCAL_EX(SpearBotFunc, Func_465a0, 0, ALL_REGISTERS) { SpearBot(gb).run(wl3::level::STATE_42); }
