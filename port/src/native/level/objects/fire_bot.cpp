// The fire bot (fire_bot.asm): the spear bot's twin (spear_bot.cpp, the
// same code over its own framesets and timings), charging with a flame
// that sets Wario on fire (it doesn't aim at him once he's burning).
// See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_FireBotFunc + offset); }

constexpr uint16_t INIT = L(0x0a); // .Init
constexpr uint16_t UPDATE = L(0x1e); // .Update
constexpr uint16_t RESET = L(0xdc); // .Func_46c42
constexpr uint16_t STATE_41_START = L(0x118); // .Func_46c7e
constexpr uint16_t STATE_41 = L(0x160); // .Func_46cc6
constexpr uint16_t AIMING = L(0x17e); // .Func_46ce4
constexpr uint16_t CHARGING = L(0x1d2); // .Func_46d38
constexpr uint16_t STOPPED = L(0x29c); // .Func_46e02
constexpr uint16_t TURN_BACK = L(0x2b8); // .Func_46e1e
constexpr uint16_t SET_WALK = L(0x309); // .Func_46e6f
constexpr uint16_t WALKING = L(0x327); // .Func_46e8d
constexpr uint16_t BUMP_32 = L(0x416); // .Func_46f7c
constexpr uint16_t BUMP_33 = L(0x41e); // .Func_46f84
constexpr uint16_t HIT_LEFT = L(0x44c); // .Func_46fb2
constexpr uint16_t HIT_LEFT_MOVE = L(0x471); // .Func_46fd7
constexpr uint16_t HIT_RIGHT = L(0x47f); // .Func_46fe5
constexpr uint16_t HIT_RIGHT_MOVE = L(0x4a4); // .Func_4700a
constexpr uint16_t STATE_44 = L(0x4b2); // .Func_47018
constexpr uint16_t STATE_45 = L(0x4ba); // .Func_47020
constexpr uint16_t DROPPED = L(0x4e7); // .Func_4704d
constexpr uint16_t DAZED = L(0x4f6); // .Func_4705c
constexpr uint16_t BOUNCE_START = L(0x510); // .Func_47076
constexpr uint16_t BOUNCING = L(0x51e); // .Func_47084
constexpr uint16_t SET_TURN = L(0x52b); // .Func_47091
constexpr uint16_t TURNING = L(0x554); // .Func_470ba
constexpr uint16_t STUNNED_38 = L(0x584); // .Func_470ea
constexpr uint16_t STUNNED_39 = L(0x58e); // .Func_470f4
constexpr uint16_t STATE_3B = L(0x5c0); // .Func_47126
constexpr uint16_t STATE_3C = L(0x5c7); // .Func_4712d
constexpr uint16_t ROLLING_3B = L(0x5d8); // .Func_4713e
constexpr uint16_t ROLLING_3C = L(0x5ea); // .Func_47150
constexpr uint16_t KNOCKED_59 = L(0x606); // .Func_4716c
constexpr uint16_t KNOCKED_58 = L(0x610); // .Func_47176
constexpr uint16_t KNOCKED_34 = L(0x61a); // .Func_47180
constexpr uint16_t KNOCKED_35 = L(0x624); // .Func_4718a
constexpr uint16_t STATE_36 = L(0x64a); // .Func_471b0
constexpr uint16_t STATE_37 = L(0x65d); // .Func_471c3
constexpr uint16_t GRAB_49 = L(0x670); // .Func_471d6
constexpr uint16_t HELD_49 = L(0x691); // .Func_471f7
constexpr uint16_t GRAB_48 = L(0x6a1); // .Func_47207
constexpr uint16_t HELD_48 = L(0x6c2); // .Func_47228
constexpr uint16_t STATE_42 = L(0x6d3); // .Func_47239

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

class FireBot : public Object {
public:
	using Object::Object;

	void start() { set_update(INIT); } // FireBotFunc
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

bool FireBot::wario_to_right() const
{
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	return static_cast<uint8_t>(screen_x() + 0x2a) < wario;
}

void FireBot::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	run(RESET);
}

void FireBot::update()
{
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

bool FireBot::shaken()
{
	if (mem_[wGroundShakeCounter] < 0x10)
		return false;
	state() = static_cast<uint8_t>(0x08 | (mem_[rDIV] & 1));
	return true;
}

void FireBot::box(uint8_t left, uint8_t right)
{
	mem_[wCurObjCollBoxRight] = right;
	mem_[wCurObjCollBoxLeft] = left;
}

bool FireBot::on_floor()
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

bool FireBot::move_on(int edge, int ahead, uint16_t move)
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

void FireBot::hit_move(uint16_t routine)
{
	if (var2())
		routines_.call(routine);
	else
		state() = 0x0f;
}

void FireBot::hit(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_68989, ADDR_Frameset_68992));
	box();
	set_interaction(OBJ_INTERACTION_00);
}

void FireBot::stunned(uint8_t s)
{
	mem_[wCurObjAction] = 0x8f;
	hit(s);
	state_duration() = 4;
	var2() = 0x02;
	var3() = 0;
}

// at a wall or an edge: turns back (once), else stops
void FireBot::stop_charge()
{
	bool right = facing_right();
	if (var1()) {
		set_frameset(right ? ADDR_Frameset_689be : ADDR_Frameset_689b3);
		state_duration() = 36;
		--var1();
		state() = 0x5a;
		return;
	}
	set_frameset(right ? ADDR_Frameset_68976 : ADDR_Frameset_68969);
	state_duration() = 61;
	state() = 0x5b;
}

void FireBot::rolling(uint16_t routine, uint16_t frameset)
{
	frame_duration() = 0;
	mem_[wCurObjFramesetOffset] = 0;
	if (var2()) {
		routines_.call(routine);
		return;
	}
	set_frameset(frameset);
	state_duration() = 61;
	state() = 0x5b;
}

void FireBot::knocked(uint8_t s)
{
	state() = s;
	no_actions();
	uint8_t sub = substate() & 0xf0;
	substate() = sub;
	var3() = 0;
	var2() = 0x02;
	set_frameset((sub & 0x80) ? ADDR_Frameset_689a0 : ADDR_Frameset_6899b);
}

void FireBot::grab(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_68989, ADDR_Frameset_68992));
	box();
	flags() |= 1 << OBJFLAG_GRABBED_F;
}

void FireBot::set_state_then(uint8_t s, uint8_t var2_, uint16_t routine)
{
	state() = s;
	var3() = 0x41;
	var2() = var2_;
	var1() = 0x01;
	routines_.call(routine);
}

void FireBot::run(uint16_t handler)
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
		set_frameset(right ? ADDR_Frameset_68946 : ADDR_Frameset_6893d);
		return;
	}
	case STATE_41_START: { // the spear put away, if it was out
		state() = 0x41;
		var3() = 0;
		bool right = facing_right();
		Byte edge = mem_[right ? wCurObjCollBoxRight : wCurObjCollBoxLeft];
		if (edge == (right ? 0x09 : 0xf6)) {
			edge = right ? 0x05 : 0xfa;
			set_frameset(right ? ADDR_Frameset_689e2 : ADDR_Frameset_689db);
			state_duration() = 14;
			return;
		}
		set_frameset(right ? ADDR_Frameset_68946 : ADDR_Frameset_6893d);
		state_duration() = 0;
		return;
	}
	case STATE_41:
		if (!state_duration()) {
			routines_.call(ADDR_ObjState_StandingFall);
			return;
		}
		if (!--state_duration())
			set_frameset(facing(ADDR_Frameset_6893d, ADDR_Frameset_68946));
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
		set_frameset(right ? ADDR_Frameset_689d2 : ADDR_Frameset_689c9);
		var1() = 0x02;
		set_interaction(OBJ_INTERACTION_FIRE_BOT);
		if (flags() & OBJFLAG_ON_SCREEN)
			play_sfx(SFX_073);
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
		set_frameset(facing(ADDR_Frameset_689c9, ADDR_Frameset_689d2));
		return;
	case SET_WALK:
		set_interaction(OBJ_INTERACTION_01);
		state() = 0x31;
		set_frameset(facing(ADDR_Frameset_6893d, ADDR_Frameset_68946));
		return;
	case WALKING: { // aims when Wario's level and close; turns if he's behind
		if (shaken())
			return;
		uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
		if (mem_[wTransformation] != TRANSFORMATION_HOT_WARIO && (dy < 0x18 || dy >= 0xe8)) {
			uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
			bool near_right = dx >= 0xc0, near_left = dx < 0x40;
			if (near_right || near_left) {
				if (facing_right() != near_right) {
					state() = 0x13;
					return;
				}
				state() = 0x2d;
				set_frameset(near_right ? ADDR_Frameset_6895c : ADDR_Frameset_6894f);
				state_duration() = 23;
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
		set_frameset(facing(ADDR_Frameset_68983, ADDR_Frameset_68986));
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
		set_frameset(facing(ADDR_Frameset_689a5, ADDR_Frameset_689ac));
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
	case ROLLING_3B: rolling(ADDR_Func_32bd, ADDR_Frameset_68969); return;
	case ROLLING_3C: rolling(ADDR_Func_32ae, ADDR_Frameset_68976); return;
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

using FireBot = wl3::level::FireBot;

PORT_EX(FireBotFunc, 0, ALL_REGISTERS) { FireBot(gb).start(); }
PORT_LOCAL_EX(FireBotFunc, Init, 0, ALL_REGISTERS) { FireBot(gb).init(); }
PORT_LOCAL_EX(FireBotFunc, Update, 0, ALL_REGISTERS) { FireBot(gb).update(); }
PORT_LOCAL_EX(FireBotFunc, Func_46c42, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::RESET); }
PORT_LOCAL_EX(FireBotFunc, Func_46c7e, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::STATE_41_START); }
PORT_LOCAL_EX(FireBotFunc, Func_46cc6, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::STATE_41); }
PORT_LOCAL_EX(FireBotFunc, Func_46ce4, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::AIMING); }
PORT_LOCAL_EX(FireBotFunc, Func_46d38, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::CHARGING); }
PORT_LOCAL_EX(FireBotFunc, Func_46e02, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::STOPPED); }
PORT_LOCAL_EX(FireBotFunc, Func_46e1e, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::TURN_BACK); }
PORT_LOCAL_EX(FireBotFunc, Func_46e6f, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::SET_WALK); }
PORT_LOCAL_EX(FireBotFunc, Func_46e8d, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::WALKING); }
PORT_LOCAL_EX(FireBotFunc, Func_46f7c, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::BUMP_32); }
PORT_LOCAL_EX(FireBotFunc, Func_46f84, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::BUMP_33); }
PORT_LOCAL_EX(FireBotFunc, Func_46fb2, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::HIT_LEFT); }
PORT_LOCAL_EX(FireBotFunc, Func_46fd7, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::HIT_LEFT_MOVE); }
PORT_LOCAL_EX(FireBotFunc, Func_46fe5, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::HIT_RIGHT); }
PORT_LOCAL_EX(FireBotFunc, Func_4700a, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::HIT_RIGHT_MOVE); }
PORT_LOCAL_EX(FireBotFunc, Func_47018, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::STATE_44); }
PORT_LOCAL_EX(FireBotFunc, Func_47020, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::STATE_45); }
PORT_LOCAL_EX(FireBotFunc, Func_4704d, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::DROPPED); }
PORT_LOCAL_EX(FireBotFunc, Func_4705c, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::DAZED); }
PORT_LOCAL_EX(FireBotFunc, Func_47076, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::BOUNCE_START); }
PORT_LOCAL_EX(FireBotFunc, Func_47084, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::BOUNCING); }
PORT_LOCAL_EX(FireBotFunc, Func_47091, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::SET_TURN); }
PORT_LOCAL_EX(FireBotFunc, Func_470ba, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::TURNING); }
PORT_LOCAL_EX(FireBotFunc, Func_470ea, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::STUNNED_38); }
PORT_LOCAL_EX(FireBotFunc, Func_470f4, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::STUNNED_39); }
PORT_LOCAL_EX(FireBotFunc, Func_47126, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::STATE_3B); }
PORT_LOCAL_EX(FireBotFunc, Func_4712d, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::STATE_3C); }
PORT_LOCAL_EX(FireBotFunc, Func_4713e, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::ROLLING_3B); }
PORT_LOCAL_EX(FireBotFunc, Func_47150, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::ROLLING_3C); }
PORT_LOCAL_EX(FireBotFunc, Func_4716c, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::KNOCKED_59); }
PORT_LOCAL_EX(FireBotFunc, Func_47176, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::KNOCKED_58); }
PORT_LOCAL_EX(FireBotFunc, Func_47180, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::KNOCKED_34); }
PORT_LOCAL_EX(FireBotFunc, Func_4718a, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::KNOCKED_35); }
PORT_LOCAL_EX(FireBotFunc, Func_471b0, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::STATE_36); }
PORT_LOCAL_EX(FireBotFunc, Func_471c3, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::STATE_37); }
PORT_LOCAL_EX(FireBotFunc, Func_471d6, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::GRAB_49); }
PORT_LOCAL_EX(FireBotFunc, Func_471f7, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::HELD_49); }
PORT_LOCAL_EX(FireBotFunc, Func_47207, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::GRAB_48); }
PORT_LOCAL_EX(FireBotFunc, Func_47228, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::HELD_48); }
PORT_LOCAL_EX(FireBotFunc, Func_47239, 0, ALL_REGISTERS) { FireBot(gb).run(wl3::level::STATE_42); }
