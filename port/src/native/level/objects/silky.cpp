// Silky (silky.asm): a silkworm that crawls along the floor, turns to
// face Wario when he's level with it and spits string at him (turning
// him into Ball o' String Wario); hit, it's knocked about and can be
// carried. Its state picks a handler from a table of 96, many of them
// the shared object states. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_SilkyFunc + offset); }

constexpr uint16_t INIT = L(0x0a); // .Init
constexpr uint16_t UPDATE = L(0x1e); // .Update
constexpr uint16_t THROW_LEFT = L(0xe2); // .Func_42dff
constexpr uint16_t THROW_RIGHT = L(0xfe); // .Func_42e1b
constexpr uint16_t RESET = L(0x11a); // .Default
constexpr uint16_t STATE_41 = L(0x15e); // .Func_42e7b
constexpr uint16_t SPITTING = L(0x178); // .Func_42e95
constexpr uint16_t SET_CRAWL = L(0x1ad); // .Func_42eca
constexpr uint16_t CRAWLING = L(0x1db); // .Func_42ef8
constexpr uint16_t HIT_LEFT = L(0x2c8); // .Func_42fe5
constexpr uint16_t HIT_LEFT_MOVE = L(0x2ed); // .Func_4300a
constexpr uint16_t HIT_RIGHT = L(0x2fb); // .Func_43018
constexpr uint16_t HIT_RIGHT_MOVE = L(0x320); // .Func_4303d
constexpr uint16_t STATE_44 = L(0x32e); // .Func_4304b
constexpr uint16_t STATE_45 = L(0x336); // .Func_43053
constexpr uint16_t DROPPED = L(0x366); // .Func_43083
constexpr uint16_t DAZED = L(0x383); // .Func_430a0
constexpr uint16_t BOUNCE_START = L(0x39d); // .Func_430ba
constexpr uint16_t BOUNCING = L(0x3ab); // .Func_430c8
constexpr uint16_t SET_TURN = L(0x3b8); // .Func_430d5
constexpr uint16_t TURNING = L(0x3d9); // .Func_430f6
constexpr uint16_t STUNNED_38 = L(0x409); // .Func_43126
constexpr uint16_t STUNNED_39 = L(0x413); // .Func_43130
constexpr uint16_t STATE_3B = L(0x445); // .Func_43162
constexpr uint16_t STATE_3C = L(0x44c); // .Func_43169
constexpr uint16_t ROLLING_3B = L(0x45d); // .Func_4317a
constexpr uint16_t ROLLING_3C = L(0x470); // .Func_4318d
constexpr uint16_t KNOCKED_59 = L(0x483); // .Func_431a0
constexpr uint16_t KNOCKED_58 = L(0x48d); // .Func_431aa
constexpr uint16_t KNOCKED_34 = L(0x497); // .Func_431b4
constexpr uint16_t KNOCKED_35 = L(0x4a1); // .Func_431be
constexpr uint16_t STATE_36 = L(0x4bd); // .Func_431da
constexpr uint16_t STATE_37 = L(0x4d0); // .Func_431ed
constexpr uint16_t GRAB_49 = L(0x4e3); // .Func_43200
constexpr uint16_t HELD_49 = L(0x504); // .Func_43221
constexpr uint16_t GRAB_48 = L(0x514); // .Func_43231
constexpr uint16_t HELD_48 = L(0x535); // .Func_43252
constexpr uint16_t STATE_42 = L(0x546); // .Func_43263

// .Update's jumptable, by state
constexpr uint16_t HANDLERS[] = {
	RESET, SET_CRAWL, STUNNED_38, STUNNED_39, KNOCKED_34, KNOCKED_35, STATE_36, STATE_37, // $00
	STUNNED_38, STUNNED_39, ADDR_VanishObject2, STATE_3B, STATE_3C, RESET, RESET, DROPPED,
	BOUNCE_START, STATE_41, STATE_42, SET_TURN, STATE_44, STATE_45, HIT_LEFT, HIT_RIGHT, // $10
	GRAB_48, GRAB_49, ADDR_Func_3263, ADDR_Func_3272, ADDR_Func_3281, RESET, RESET, RESET,
	ADDR_Func_3191, ADDR_Func_31af, ADDR_Func_31cd, ADDR_Func_31eb, ADDR_Func_3209, ADDR_Func_3227, ADDR_Func_3245, ADDR_Func_3254, // $20
	KNOCKED_58, KNOCKED_59, RESET, RESET, RESET, SPITTING, RESET, RESET,
	ADDR_ObjState_Fall, CRAWLING, ADDR_ObjState_BumpLeft, ADDR_ObjState_BumpRight, ADDR_Func_3326, ADDR_Func_3317, ADDR_Func_3335, ADDR_Func_3344, // $30
	ADDR_Func_31fa, ADDR_Func_31dc, RESET, ROLLING_3B, ROLLING_3C, RESET, RESET, DAZED,
	BOUNCING, ADDR_ObjState_StandingFall, ADDR_Func_3371, TURNING, ADDR_Func_32cc, ADDR_Func_32db, HIT_LEFT_MOVE, HIT_RIGHT_MOVE, // $40
	HELD_48, HELD_49, ADDR_Func_3380, ADDR_Func_338f, THROW_LEFT, THROW_RIGHT, ADDR_Func_339e, ADDR_Func_33ad,
	ADDR_Func_31a0, ADDR_Func_31be, ADDR_Func_31dc, ADDR_Func_31fa, ADDR_Func_3218, ADDR_Func_3236, ADDR_Func_32cc, ADDR_Func_32db, // $50
	ADDR_Func_33f8, ADDR_Func_3407, RESET, RESET, RESET, RESET, ADDR_ObjState_FullThrowLeft, ADDR_ObjState_FullThrowRight,
};
constexpr uint8_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

class Silky : public Object {
public:
	using Object::Object;

	void start() { set_update(INIT); } // SilkyFunc
	void init(); // .Init
	void update(); // .Update
	void run(uint16_t handler);

private:
	uint16_t facing(uint16_t left, uint16_t right) { return facing_right() ? right : left; }
	bool shaken(); // a strong shake knocks it down (state 8 or 9, at random)
	void crawl(); // .asm_42f44
	void small_box(); // the collision box when hit
	void hit(uint8_t s, uint16_t move);
	void hit_move(uint16_t routine);
	void stunned(uint8_t s);
	void knocked(uint8_t s);
	void rolling(uint16_t routine);
	void grab(uint8_t s);
	void set_state_then(uint8_t s, uint8_t var2_, uint16_t routine);
	void throw_(uint8_t s, uint16_t routine);
};

} // namespace

void Silky::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	run(RESET);
}

void Silky::update()
{
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

bool Silky::shaken()
{
	if (mem_[wGroundShakeCounter] < 0x10)
		return false;
	state() = static_cast<uint8_t>(0x08 | (mem_[rDIV] & 1));
	return true;
}

void Silky::small_box()
{
	mem_[wCurObjCollBoxRight] = 0x05;
	mem_[wCurObjCollBoxLeft] = 0xfa;
}

// a pixel every other frame, unless at a wall or an edge (it turns then)
void Silky::crawl()
{
	if (shaken())
		return;
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_3543);
	if (!gb_->cpu.a) {
		state() = 0x11; // falls
		return;
	}
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	bool right = facing_right();
	uint8_t in_block = mem_[wCurObjXPos + 0] & 0x0f;
	// the block ahead, once it's at the edge of this one
	bool edge = right ? in_block + 7 >= 0x10 : in_block < 8;
	if (edge) {
		routines_.call(right ? ADDR_ProcessBlockOnRight : ADDR_ProcessBlockOnLeft);
		if (!(gb_->cpu.a & 0x0f)) {
			state() = 0x13;
			return;
		}
	}
	uint16_t y_pos = static_cast<uint16_t>(y() - 4);
	uint16_t x_pos = right ? static_cast<uint16_t>(x() + 7) : static_cast<uint16_t>(x() - 8);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a) {
		state() = 0x13;
		return;
	}
	routines_.call(right ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
}

void Silky::hit_move(uint16_t routine)
{
	if (var2())
		routines_.call(routine);
	else
		state() = 0x0f;
}

void Silky::hit(uint8_t s, uint16_t move)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_6877a, ADDR_Frameset_68783));
	small_box();
	set_interaction(OBJ_INTERACTION_00);
	hit_move(move);
}

void Silky::stunned(uint8_t s)
{
	mem_[wCurObjAction] = 0x8f;
	state() = s;
	set_frameset(facing(ADDR_Frameset_6878c, ADDR_Frameset_68795));
	state_duration() = 0x04;
	var2() = 0x02;
	var3() = 0;
	small_box();
	set_interaction(OBJ_INTERACTION_00);
}

void Silky::knocked(uint8_t s)
{
	state() = s;
	substate() &= 0xf0;
	var3() = 0;
	var2() = 0x02;
	set_frameset(ADDR_Frameset_687c6);
	no_actions();
}

void Silky::rolling(uint16_t routine)
{
	frame_duration() = 0;
	mem_[wCurObjFramesetOffset] = 0;
	if (var2())
		routines_.call(routine);
	else
		state() = 0x13;
}

void Silky::grab(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_6877a, ADDR_Frameset_68783));
	small_box();
	flags() |= 1 << OBJFLAG_GRABBED_F;
}

void Silky::set_state_then(uint8_t s, uint8_t var2_, uint16_t routine)
{
	state() = s;
	var3() = 0x41;
	var2() = var2_;
	var1() = 0x01;
	routines_.call(routine);
}

void Silky::throw_(uint8_t s, uint16_t routine)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_6879e, ADDR_Frameset_687a7));
	routines_.call(routine);
}

void Silky::run(uint16_t handler)
{
	switch (handler) {
	case THROW_LEFT: throw_(0x5e, ADDR_ObjState_FullThrowLeft); return;
	case THROW_RIGHT: throw_(0x5f, ADDR_ObjState_FullThrowRight); return;
	case RESET: // facing Wario
		set_interaction(OBJ_INTERACTION_01);
		state() = 0x30;
		{
			uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
			bool right = static_cast<uint8_t>(screen_x() + 0x2a) < wario;
			if (right)
				substate() |= OBJSUBFLAG_HDIR;
			else
				substate() &= ~OBJSUBFLAG_HDIR;
			var3() = 0;
			set_frameset(right ? ADDR_Frameset_6874d : ADDR_Frameset_68744);
			mem_[wCurObjCollBoxRight] = right ? 0x07 : 0x06;
			mem_[wCurObjCollBoxLeft] = right ? 0xf9 : 0xf8;
		}
		return;
	case STATE_41:
		state() = 0x41;
		var3() = 0;
		set_frameset(facing(ADDR_Frameset_68744, ADDR_Frameset_6874d));
		return;
	case SPITTING: // the string, after a wind-up
		if (!state_duration()) {
			crawl();
			return;
		}
		if (shaken())
			return;
		if (--state_duration())
			return;
		set_interaction(OBJ_INTERACTION_SILKY);
		set_frameset(facing(ADDR_Frameset_68756, ADDR_Frameset_6875f));
		return;
	case SET_CRAWL: {
		set_interaction(OBJ_INTERACTION_01);
		state() = 0x31;
		bool right = facing_right();
		mem_[wCurObjCollBoxRight] = right ? 0x07 : 0x06;
		mem_[wCurObjCollBoxLeft] = right ? 0xf9 : 0xf8;
		set_frameset(right ? ADDR_Frameset_6874d : ADDR_Frameset_68744);
		return;
	}
	case CRAWLING: // spits when Wario's level and ahead; turns when he's behind
		if (mem_[wTransformation] != TRANSFORMATION_BALL_O_STRING_WARIO) {
			uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
			if (dy < 0x0f || dy >= 0xf1) {
				uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
				bool wario_right = static_cast<uint8_t>(screen_x() + 0x2a) < wario;
				if (wario_right != facing_right()) {
					state() = 0x13;
					return;
				}
				set_frameset(wario_right ? ADDR_Frameset_687bb : ADDR_Frameset_687b0);
				state_duration() = 0x23;
				state() = 0x2d;
				return;
			}
		}
		crawl();
		return;
	case HIT_LEFT: hit(0x46, ADDR_Func_32ae); return;
	case HIT_LEFT_MOVE: hit_move(ADDR_Func_32ae); return;
	case HIT_RIGHT: hit(0x47, ADDR_Func_32bd); return;
	case HIT_RIGHT_MOVE: hit_move(ADDR_Func_32bd); return;
	case STATE_44:
	case STATE_45:
		state() = handler == STATE_44 ? 0x44 : 0x45;
		set_frameset(facing(ADDR_Frameset_6877a, ADDR_Frameset_68783));
		var2() = 0x02;
		var3() = 0;
		small_box();
		set_interaction(OBJ_INTERACTION_00);
		return;
	case DROPPED:
		state() = 0x3f;
		set_frameset(facing(ADDR_Frameset_6877a, ADDR_Frameset_68783));
		state_duration() = 0x64;
		flags() &= ~(1 << OBJFLAG_GRABBED_F);
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
	case BOUNCE_START:
		state() = 0x40;
		var3() = 0;
		state_duration() = 0x07;
		run(BOUNCING);
		return;
	case BOUNCING:
		if (--state_duration())
			routines_.call(ADDR_Func_32f9);
		else
			state() = 0x00;
		return;
	case SET_TURN:
		state() = 0x43;
		set_frameset(facing(ADDR_Frameset_68768, ADDR_Frameset_68771));
		state_duration() = 0x23;
		set_interaction(OBJ_INTERACTION_01);
		return;
	case TURNING:
		if (shaken())
			return;
		if (--state_duration()) {
			routines_.call(ADDR_Func_3308);
			return;
		}
		state() = 0x01;
		substate() ^= OBJSUBFLAG_HDIR;
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
	case ROLLING_3B: rolling(ADDR_Func_32bd); return;
	case ROLLING_3C: rolling(ADDR_Func_32ae); return;
	case KNOCKED_59: knocked(0x59); return;
	case KNOCKED_58: knocked(0x58); return;
	case KNOCKED_34: knocked(0x34); return;
	case KNOCKED_35: knocked(0x35); return;
	case STATE_36: set_state_then(0x36, 0xc0, ADDR_Func_3335); return;
	case STATE_37: set_state_then(0x37, 0xe0, ADDR_Func_3344); return;
	case GRAB_49:
		grab(0x49);
		run(HELD_49);
		return;
	case HELD_49: // carried facing the way Wario does
		if (mem_[wDirection]) {
			routines_.call(ADDR_Func_3353);
			return;
		}
		substate() ^= 0x80;
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
		substate() ^= 0x80;
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

using Silky = wl3::level::Silky;

PORT_EX(SilkyFunc, 0, ALL_REGISTERS) { Silky(gb).start(); }
PORT_LOCAL_EX(SilkyFunc, Init, 0, ALL_REGISTERS) { Silky(gb).init(); }
PORT_LOCAL_EX(SilkyFunc, Update, 0, ALL_REGISTERS) { Silky(gb).update(); }
PORT_LOCAL_EX(SilkyFunc, Default, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::RESET); }
PORT_LOCAL_EX(SilkyFunc, Func_42dff, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::THROW_LEFT); }
PORT_LOCAL_EX(SilkyFunc, Func_42e1b, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::THROW_RIGHT); }
PORT_LOCAL_EX(SilkyFunc, Func_42e7b, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::STATE_41); }
PORT_LOCAL_EX(SilkyFunc, Func_42e95, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::SPITTING); }
PORT_LOCAL_EX(SilkyFunc, Func_42eca, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::SET_CRAWL); }
PORT_LOCAL_EX(SilkyFunc, Func_42ef8, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::CRAWLING); }
PORT_LOCAL_EX(SilkyFunc, Func_42fe5, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::HIT_LEFT); }
PORT_LOCAL_EX(SilkyFunc, Func_4300a, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::HIT_LEFT_MOVE); }
PORT_LOCAL_EX(SilkyFunc, Func_43018, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::HIT_RIGHT); }
PORT_LOCAL_EX(SilkyFunc, Func_4303d, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::HIT_RIGHT_MOVE); }
PORT_LOCAL_EX(SilkyFunc, Func_4304b, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::STATE_44); }
PORT_LOCAL_EX(SilkyFunc, Func_43053, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::STATE_45); }
PORT_LOCAL_EX(SilkyFunc, Func_43083, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::DROPPED); }
PORT_LOCAL_EX(SilkyFunc, Func_430a0, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::DAZED); }
PORT_LOCAL_EX(SilkyFunc, Func_430ba, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::BOUNCE_START); }
PORT_LOCAL_EX(SilkyFunc, Func_430c8, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::BOUNCING); }
PORT_LOCAL_EX(SilkyFunc, Func_430d5, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::SET_TURN); }
PORT_LOCAL_EX(SilkyFunc, Func_430f6, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::TURNING); }
PORT_LOCAL_EX(SilkyFunc, Func_43126, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::STUNNED_38); }
PORT_LOCAL_EX(SilkyFunc, Func_43130, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::STUNNED_39); }
PORT_LOCAL_EX(SilkyFunc, Func_43162, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::STATE_3B); }
PORT_LOCAL_EX(SilkyFunc, Func_43169, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::STATE_3C); }
PORT_LOCAL_EX(SilkyFunc, Func_4317a, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::ROLLING_3B); }
PORT_LOCAL_EX(SilkyFunc, Func_4318d, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::ROLLING_3C); }
PORT_LOCAL_EX(SilkyFunc, Func_431a0, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::KNOCKED_59); }
PORT_LOCAL_EX(SilkyFunc, Func_431aa, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::KNOCKED_58); }
PORT_LOCAL_EX(SilkyFunc, Func_431b4, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::KNOCKED_34); }
PORT_LOCAL_EX(SilkyFunc, Func_431be, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::KNOCKED_35); }
PORT_LOCAL_EX(SilkyFunc, Func_431da, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::STATE_36); }
PORT_LOCAL_EX(SilkyFunc, Func_431ed, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::STATE_37); }
PORT_LOCAL_EX(SilkyFunc, Func_43200, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::GRAB_49); }
PORT_LOCAL_EX(SilkyFunc, Func_43221, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::HELD_49); }
PORT_LOCAL_EX(SilkyFunc, Func_43231, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::GRAB_48); }
PORT_LOCAL_EX(SilkyFunc, Func_43252, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::HELD_48); }
PORT_LOCAL_EX(SilkyFunc, Func_43263, 0, ALL_REGISTERS) { Silky(gb).run(wl3::level::STATE_42); }
