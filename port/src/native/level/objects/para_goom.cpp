// The Para-Goom (para_goom.asm): a goom drifting down under a spiked
// parachute that, landed, walks the floor, its spear out ahead after a
// moment (and the parachute, spiked side up, kept a while: collision box
// top $ef). Knocked about like other enemies, through a table of 96 state
// handlers. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_ParaGoomFunc + offset); }

constexpr uint16_t INIT = L(0x0a); // .Init
constexpr uint16_t UPDATE = L(0x1e); // .Update
constexpr uint16_t FULL_THROW_LEFT_START = L(0xe2); // .Func_41439
constexpr uint16_t FULL_THROW_RIGHT_START = L(0xf0); // .Func_41447
constexpr uint16_t DEFAULT = L(0xfe); // .Default
constexpr uint16_t SETTLE_START = L(0x169); // .Func_414c0
constexpr uint16_t SETTLING = L(0x1a4); // .Func_414fb
constexpr uint16_t WAIT_THEN_WALK = L(0x1c0); // .Func_41517
constexpr uint16_t WAIT_THEN_STING = L(0x1d8); // .Func_4152f
constexpr uint16_t SET_WALK = L(0x210); // .Func_41567
constexpr uint16_t WALKING = L(0x241); // .Func_41598
constexpr uint16_t BUMP_LEFT = L(0x359); // .Func_416b0
constexpr uint16_t BUMP_RIGHT = L(0x361); // .Func_416b8
constexpr uint16_t STUN_LEFT_START = L(0x3a4); // .Func_416fb
constexpr uint16_t STUN_LEFT = L(0x3cc); // .Func_41723
constexpr uint16_t STUN_RIGHT_START = L(0x3da); // .Func_41731
constexpr uint16_t STUN_RIGHT = L(0x402); // .Func_41759
constexpr uint16_t STATE_44 = L(0x410); // .Func_41767
constexpr uint16_t STATE_45 = L(0x418); // .Func_4176f
constexpr uint16_t STATE_0F = L(0x44b); // .Func_417a2
constexpr uint16_t STATE_3F = L(0x45a); // .Func_417b1
constexpr uint16_t STATE_10 = L(0x474); // .Func_417cb
constexpr uint16_t STATE_40 = L(0x491); // .Func_417e8
constexpr uint16_t STATE_5A = L(0x4c5); // .Func_4181c
constexpr uint16_t DROPPING = L(0x4d9); // .Func_41830
constexpr uint16_t LANDED = L(0x518); // .Func_4186f
constexpr uint16_t TURN_START = L(0x534); // .Func_4188b
constexpr uint16_t TURNING = L(0x56d); // .Func_418c4
constexpr uint16_t WOBBLE_START = L(0x5ad); // .Func_41904
constexpr uint16_t WOBBLE_LEFT = L(0x5bb); // .Func_41912
constexpr uint16_t WOBBLE_RIGHT = L(0x5c5); // .Func_4191c
constexpr uint16_t STATE_0B = L(0x5fd); // .Func_41954
constexpr uint16_t STATE_0C = L(0x604); // .Func_4195b
constexpr uint16_t STATE_3B = L(0x615); // .Func_4196c
constexpr uint16_t STATE_3C = L(0x628); // .Func_4197f
constexpr uint16_t STATE_29 = L(0x63b); // .Func_41992
constexpr uint16_t STATE_28 = L(0x645); // .Func_4199c
constexpr uint16_t ATTACKED_LEFT = L(0x64f); // .Func_419a6
constexpr uint16_t ATTACKED_RIGHT = L(0x659); // .Func_419b0
constexpr uint16_t PICKED_UP_LEFT = L(0x67e); // .Func_419d5
constexpr uint16_t PICKED_UP_RIGHT = L(0x691); // .Func_419e8
constexpr uint16_t STATE_19 = L(0x6a4); // .Func_419fb
constexpr uint16_t STATE_49 = L(0x6c8); // .Func_41a1f
constexpr uint16_t STATE_18 = L(0x6d8); // .Func_41a2f
constexpr uint16_t STATE_48 = L(0x6fc); // .Func_41a53
constexpr uint16_t STATE_12 = L(0x70d); // .Func_41a64

// .Update's jumptable, by state
constexpr uint16_t HANDLERS[] = {
	DEFAULT, SET_WALK, BUMP_LEFT, BUMP_RIGHT, ATTACKED_LEFT, ATTACKED_RIGHT, PICKED_UP_LEFT, PICKED_UP_RIGHT, // $00
	WOBBLE_LEFT, WOBBLE_RIGHT, ADDR_VanishObject2, STATE_0B, STATE_0C, WOBBLE_START, DEFAULT, STATE_0F,
	STATE_10, SETTLE_START, STATE_12, TURN_START, STATE_44, STATE_45, STUN_LEFT_START, STUN_RIGHT_START, // $10
	STATE_18, STATE_19, ADDR_Func_3263, ADDR_Func_3272, ADDR_Func_3281, DEFAULT, DEFAULT, DEFAULT,
	ADDR_Func_3191, ADDR_Func_31af, ADDR_Func_31cd, ADDR_Func_31eb, ADDR_Func_3209, ADDR_Func_3227, ADDR_Func_3245, ADDR_Func_3254, // $20
	STATE_28, STATE_29, DEFAULT, DEFAULT, DEFAULT, WAIT_THEN_STING, WAIT_THEN_WALK, DEFAULT,
	ADDR_ObjState_Fall, WALKING, ADDR_ObjState_BumpLeft, ADDR_ObjState_BumpRight, ADDR_Func_3326, ADDR_Func_3317, ADDR_Func_3335, ADDR_Func_3344, // $30
	ADDR_Func_31fa, ADDR_Func_31dc, DEFAULT, STATE_3B, STATE_3C, DEFAULT, DEFAULT, STATE_3F,
	STATE_40, SETTLING, ADDR_Func_3371, TURNING, ADDR_Func_32cc, ADDR_Func_32db, STUN_LEFT, STUN_RIGHT, // $40
	STATE_48, STATE_49, ADDR_Func_3380, ADDR_Func_338f, FULL_THROW_LEFT_START, FULL_THROW_RIGHT_START, ADDR_Func_339e, ADDR_Func_33ad,
	ADDR_Func_31a0, ADDR_Func_31be, ADDR_Func_31dc, ADDR_Func_31fa, ADDR_Func_3218, ADDR_Func_3236, ADDR_Func_32cc, ADDR_Func_32db, // $50
	ADDR_Func_33f8, ADDR_Func_3407, STATE_5A, DROPPING, LANDED, DEFAULT, ADDR_ObjState_FullThrowLeft, ADDR_ObjState_FullThrowRight,
};
constexpr uint8_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

constexpr uint8_t STING_TOP = 0xef; // the collision box top, parachute up

class ParaGoom : public Object {
public:
	using Object::Object;

	void start() { set_update(INIT); } // ParaGoomFunc
	void init(); // .Init
	void update(); // .Update
	void run(uint16_t handler);

private:
	uint16_t facing(uint16_t left, uint16_t right) { return facing_right() ? right : left; }
	bool sting_up() const { return mem_[wCurObjCollBoxTop] == STING_TOP; }
	void set_top(uint8_t interaction, uint8_t top);
	void box(uint8_t right, uint8_t left);
	bool shaken();
	void parachute(); // .asm_41505
	void sting_walk(); // .asm_41545
	void walk(); // .asm_41570
	void bump(uint8_t s); // .asm_416be
	void stun_start(uint8_t s);
	void var2_move(uint16_t routine, uint8_t s);
	void dazed(); // .asm_41775
	void wobble(uint8_t s); // .asm_41924
	void rolled(uint8_t s);
	void attacked(uint8_t s); // .asm_419b8
	void grab(uint8_t s);
};

} // namespace

void ParaGoom::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	run(DEFAULT);
}

void ParaGoom::update()
{
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

void ParaGoom::set_top(uint8_t interaction, uint8_t top)
{
	set_interaction(interaction);
	mem_[wCurObjCollBoxTop] = top;
}

void ParaGoom::box(uint8_t right, uint8_t left)
{
	mem_[wCurObjCollBoxRight] = right;
	mem_[wCurObjCollBoxLeft] = left;
}

bool ParaGoom::shaken()
{
	if (mem_[wGroundShakeCounter] < 0x10)
		return false;
	state() = static_cast<uint8_t>(0x08 | (mem_[rDIV] & 1));
	return true;
}

void ParaGoom::parachute()
{
	set_frameset(ADDR_Frameset_684ba);
	set_top(OBJ_INTERACTION_TOP_STING, STING_TOP);
}

void ParaGoom::sting_walk()
{
	state() = 0x31;
	set_frameset(facing(ADDR_Frameset_6844b, ADDR_Frameset_68454));
	state_duration() = 0;
	set_top(OBJ_INTERACTION_TOP_STING, STING_TOP);
}

void ParaGoom::walk()
{
	state() = 0x31;
	if (facing_right()) {
		box(0x09, 0xf9);
		set_frameset(ADDR_Frameset_68442);
	} else {
		box(0x06, 0xf6);
		set_frameset(ADDR_Frameset_68439);
	}
	state_duration() = 0x0a;
}

void ParaGoom::bump(uint8_t s)
{
	state() = s;
	if (sting_up())
		set_frameset(facing(ADDR_Frameset_6844b, ADDR_Frameset_68454));
	else
		set_frameset(facing(ADDR_Frameset_68439, ADDR_Frameset_68442));
	state_duration() = 0x0c;
	var2() = 0x02;
	box(0x05, 0xfa);
	set_interaction(OBJ_INTERACTION_01);
}

void ParaGoom::stun_start(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_684aa, ADDR_Frameset_684af));
	box(0x05, 0xfa);
	set_top(OBJ_INTERACTION_00, 0xf4);
}

void ParaGoom::var2_move(uint16_t routine, uint8_t s)
{
	if (var2())
		routines_.call(routine);
	else
		state() = s;
}

void ParaGoom::dazed()
{
	set_frameset(facing(ADDR_Frameset_684aa, ADDR_Frameset_684af));
	var2() = 0x02;
	var3() = 0;
	box(0x05, 0xfa);
	set_top(OBJ_INTERACTION_00, 0xf4);
}

void ParaGoom::wobble(uint8_t s)
{
	mem_[wCurObjAction] = 0x8f;
	state() = s;
	set_frameset(facing(ADDR_Frameset_684aa, ADDR_Frameset_684af));
	state_duration() = 0x04;
	var2() = 0x02;
	var3() = 0;
	box(0x05, 0xfa);
	set_top(OBJ_INTERACTION_00, 0xf4);
}

void ParaGoom::rolled(uint8_t s)
{
	state() = s;
	frame_duration() = 0;
	mem_[wCurObjFramesetOffset] = 0;
	var2() = 0x02;
}

void ParaGoom::attacked(uint8_t s)
{
	state() = s;
	no_actions();
	uint8_t sub = substate() & 0xf0;
	substate() = sub;
	var3() = 0;
	var2() = 0x02;
	set_frameset((sub & 0x80) ? ADDR_Frameset_684af : ADDR_Frameset_684aa);
}

void ParaGoom::grab(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_684aa, ADDR_Frameset_684af));
	box(0x05, 0xfa);
	flags() |= 1 << OBJFLAG_GRABBED_F;
}

void ParaGoom::run(uint16_t handler)
{
	switch (handler) {
	case DEFAULT: { // facing Wario; on a floor, else drifting down
		var1() = 0;
		set_top(OBJ_INTERACTION_01, 0xf4);
		state() = 0x30;
		uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
		if (static_cast<uint8_t>(screen_x() + 0x2a) >= wario) {
			substate() &= ~(1 << OBJFLAG_PRIORITY_F);
			var3() = 0;
			set_frameset(ADDR_Frameset_68439);
			box(0x06, 0xf6);
		} else {
			substate() |= 1 << OBJFLAG_PRIORITY_F;
			var3() = 0;
			set_frameset(ADDR_Frameset_68442);
			box(0x09, 0xf9);
		}
		mem_[hYPosLo] = mem_[wCurObjYPos + 0];
		mem_[hYPosHi] = mem_[wCurObjYPos + 1];
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		routines_.call(ADDR_Func_352b);
		if (gb_->cpu.a)
			return;
		state() = 0x41;
		state_duration() = 0;
		parachute();
		return;
	}
	case FULL_THROW_LEFT_START:
	case FULL_THROW_RIGHT_START: {
		bool left = handler == FULL_THROW_LEFT_START;
		state() = left ? 0x5e : 0x5f;
		set_frameset(ADDR_Frameset_684a1);
		routines_.call(left ? ADDR_ObjState_FullThrowLeft : ADDR_ObjState_FullThrowRight);
		return;
	}
	case SETTLE_START: { // the parachute folding
		state() = 0x41;
		bool top = sting_up();
		bool right = facing_right();
		var3() = 0;
		if (!top)
			set_frameset(right ? ADDR_Frameset_6850a : ADDR_Frameset_684e7);
		else
			set_frameset(right ? ADDR_Frameset_68546 : ADDR_Frameset_6852d);
		state_duration() = top ? 0x29 : 0x38;
		return;
	}
	case SETTLING:
		if (!state_duration()) {
			run(DROPPING);
			return;
		}
		if (!--state_duration())
			parachute();
		return;
	case WAIT_THEN_WALK:
	case WAIT_THEN_STING:
		if (shaken())
			return;
		if (--state_duration())
			return;
		if (handler == WAIT_THEN_WALK)
			walk();
		else
			sting_walk();
		return;
	case SET_WALK:
		set_interaction(OBJ_INTERACTION_01);
		walk();
		return;
	case WALKING: { // the spear out after a moment; the parachute up a while
		if (shaken())
			return;
		if (state_duration()) {
			if (!--state_duration())
				set_interaction(OBJ_INTERACTION_FRONT_STING);
			return;
		}
		if (!++var1()) {
			if (sting_up()) {
				state() = 0x2e;
				set_frameset(facing(ADDR_Frameset_6846e, ADDR_Frameset_68490));
				state_duration() = 0xa3;
				set_top(OBJ_INTERACTION_01, 0xf4);
				return;
			}
			state() = 0x2d;
			set_frameset(facing(ADDR_Frameset_6845d, ADDR_Frameset_6847f));
			state_duration() = 0xa3;
			set_interaction(OBJ_INTERACTION_01);
			box(0x05, 0xfa);
			return;
		}
		mem_[hYPosLo] = mem_[wCurObjYPos + 0];
		mem_[hYPosHi] = mem_[wCurObjYPos + 1];
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		routines_.call(ADDR_Func_3543);
		if (!gb_->cpu.a) {
			state() = 0x11;
			return;
		}
		mem_[wCurObjYPos + 0] = mem_[hYPosLo];
		mem_[wCurObjYPos + 1] = mem_[hYPosHi];
		bool right = facing_right();
		uint8_t in_block = mem_[wCurObjXPos + 0] & 0x0f;
		uint8_t edge = right ? static_cast<uint8_t>(mem_[wCurObjCollBoxRight])
			: static_cast<uint8_t>(-static_cast<uint8_t>(mem_[wCurObjCollBoxLeft]));
		if (right ? static_cast<uint8_t>(in_block + edge) >= 0x10 : in_block < edge) {
			routines_.call(right ? ADDR_ProcessBlockOnRight : ADDR_ProcessBlockOnLeft);
			if (!(gb_->cpu.a & 0x0f)) {
				state() = 0x13;
				return;
			}
		}
		uint16_t y_pos = static_cast<uint16_t>(y() - 4);
		edge = right ? static_cast<uint8_t>(mem_[wCurObjCollBoxRight]) : static_cast<uint8_t>(-static_cast<uint8_t>(mem_[wCurObjCollBoxLeft]));
		uint16_t x_pos = static_cast<uint16_t>(right ? x() + edge : x() - edge);
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
		return;
	}
	case BUMP_LEFT:
		bump(0x32);
		return;
	case BUMP_RIGHT:
		bump(0x33);
		return;
	case STUN_LEFT_START:
		stun_start(0x46);
		[[fallthrough]];
	case STUN_LEFT:
		var2_move(ADDR_Func_32ae, 0x0f);
		return;
	case STUN_RIGHT_START:
		stun_start(0x47);
		[[fallthrough]];
	case STUN_RIGHT:
		var2_move(ADDR_Func_32bd, 0x0f);
		return;
	case STATE_44:
	case STATE_45:
		state() = handler == STATE_44 ? 0x44 : 0x45;
		dazed();
		return;
	case STATE_0F:
		state() = 0x3f;
		state_duration() = 0x64;
		flags() &= ~(1 << OBJFLAG_GRABBED_F);
		[[fallthrough]];
	case STATE_3F:
		if (mem_[wGroundShakeCounter] >= 0x10) {
			state() = 0x1c;
			return;
		}
		if (--state_duration())
			routines_.call(ADDR_Func_32ea);
		else
			state() = 0x10;
		return;
	case STATE_10: // up, the parachute out again
		state() = 0x40;
		var3() = 0;
		set_frameset(ADDR_Frameset_684b4);
		state_duration() = 0x08;
		set_top(OBJ_INTERACTION_TOP_STING, STING_TOP);
		[[fallthrough]];
	case STATE_40:
		if (--state_duration()) {
			uint16_t y_pos = static_cast<uint16_t>(y() - 8);
			mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
			mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
			mem_[hXPosLo] = mem_[wCurObjXPos + 0];
			mem_[hXPosHi] = mem_[wCurObjXPos + 1];
			routines_.call(ADDR_Func_358b);
			if (!(gb_->cpu.a & 0x0f)) {
				SET_BC(gb_, ADDR_Data_60530);
				routines_.call(ADDR_ApplyObjYMovement);
				return;
			}
		}
		state() = 0x5a;
		set_frameset(ADDR_Frameset_684b7);
		state_duration() = 0x04;
		return;
	case STATE_5A:
		if (--state_duration())
			return;
		state() = 0x5b;
		var3() = 0;
		set_frameset(ADDR_Frameset_684ba);
		return;
	case DROPPING: // drifting down to a floor
		routines_.call(ADDR_MoveObjectDown_Slow);
		mem_[hYPosLo] = mem_[wCurObjYPos + 0];
		mem_[hYPosHi] = mem_[wCurObjYPos + 1];
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		mem_[wObjSlopeInteraction] = 0;
		routines_.call(ADDR_Func_352b);
		if (!gb_->cpu.a) {
			if (mem_[wIsInWaterOrSand])
				state() = 0x0a;
			return;
		}
		mem_[wCurObjYPos + 0] = mem_[hYPosLo];
		mem_[wCurObjYPos + 1] = mem_[hYPosHi];
		state() = 0x5c;
		set_frameset(ADDR_Frameset_6855f);
		state_duration() = 0x0a;
		return;
	case LANDED:
		if (shaken())
			return;
		if (!--state_duration())
			state() = 0x00;
		return;
	case TURN_START:
		state() = 0x43;
		if (sting_up())
			set_frameset(facing(ADDR_Frameset_684d9, ADDR_Frameset_684e0));
		else
			set_frameset(facing(ADDR_Frameset_684cb, ADDR_Frameset_684d2));
		state_duration() = 0x44;
		return;
	case TURNING:
		if (shaken())
			return;
		if (state_duration() == 0x27) { // halfway round
			--state_duration();
			set_interaction(OBJ_INTERACTION_01);
			substate() ^= OBJSUBFLAG_HDIR;
			return;
		}
		if (--state_duration()) {
			routines_.call(ADDR_Func_3308);
			return;
		}
		if (sting_up())
			sting_walk();
		else
			state() = 0x01;
		return;
	case WOBBLE_START: {
		uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
		wobble(static_cast<uint8_t>(screen_x() + 0x2a) < wario ? 0x39 : 0x38);
		return;
	}
	case WOBBLE_LEFT:
		wobble(0x38);
		return;
	case WOBBLE_RIGHT:
		wobble(0x39);
		return;
	case STATE_0B:
		rolled(0x3b);
		return;
	case STATE_0C:
		rolled(0x3c);
		return;
	case STATE_3B:
	case STATE_3C:
		frame_duration() = 0;
		mem_[wCurObjFramesetOffset] = 0;
		var2_move(handler == STATE_3B ? ADDR_Func_32bd : ADDR_Func_32ae, 0x13);
		return;
	case STATE_29: attacked(0x59); return;
	case STATE_28: attacked(0x58); return;
	case ATTACKED_LEFT: attacked(0x34); return;
	case ATTACKED_RIGHT: attacked(0x35); return;
	case PICKED_UP_LEFT:
	case PICKED_UP_RIGHT: {
		bool left = handler == PICKED_UP_LEFT;
		state() = left ? 0x36 : 0x37;
		var3() = 0x41;
		var2() = left ? 0xc0 : 0xe0;
		var1() = 0x01;
		routines_.call(left ? ADDR_Func_3335 : ADDR_Func_3344);
		return;
	}
	case STATE_19:
		grab(0x49);
		[[fallthrough]];
	case STATE_49: // carried, turning with Wario
		if (mem_[wDirection]) {
			routines_.call(ADDR_Func_3353);
			return;
		}
		substate() ^= 0x80;
		run(STATE_18);
		return;
	case STATE_18:
		grab(0x48);
		[[fallthrough]];
	case STATE_48:
		if (!mem_[wDirection]) {
			routines_.call(ADDR_Func_3362);
			return;
		}
		substate() ^= 0x80;
		run(STATE_19);
		return;
	case STATE_12:
		mem_[wCurObjAction] = 0x9f;
		state() = 0x42;
		var3() = 0;
		set_top(OBJ_INTERACTION_00, 0xf4);
		return;
	default: // a shared object state
		routines_.call(handler);
		return;
	}
}

} // namespace wl3::level

using wl3::level::ParaGoom;

PORT_EX(ParaGoomFunc, 0, ALL_REGISTERS) { ParaGoom(gb).start(); }
PORT_LOCAL_EX(ParaGoomFunc, Init, 0, ALL_REGISTERS) { ParaGoom(gb).init(); }
PORT_LOCAL_EX(ParaGoomFunc, Update, 0, ALL_REGISTERS) { ParaGoom(gb).update(); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41439, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::FULL_THROW_LEFT_START); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41447, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::FULL_THROW_RIGHT_START); }
PORT_LOCAL_EX(ParaGoomFunc, Default, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::DEFAULT); }
PORT_LOCAL_EX(ParaGoomFunc, Func_414c0, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::SETTLE_START); }
PORT_LOCAL_EX(ParaGoomFunc, Func_414fb, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::SETTLING); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41517, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::WAIT_THEN_WALK); }
PORT_LOCAL_EX(ParaGoomFunc, Func_4152f, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::WAIT_THEN_STING); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41567, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::SET_WALK); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41598, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::WALKING); }
PORT_LOCAL_EX(ParaGoomFunc, Func_416b0, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::BUMP_LEFT); }
PORT_LOCAL_EX(ParaGoomFunc, Func_416b8, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::BUMP_RIGHT); }
PORT_LOCAL_EX(ParaGoomFunc, Func_416fb, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STUN_LEFT_START); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41723, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STUN_LEFT); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41731, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STUN_RIGHT_START); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41759, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STUN_RIGHT); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41767, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_44); }
PORT_LOCAL_EX(ParaGoomFunc, Func_4176f, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_45); }
PORT_LOCAL_EX(ParaGoomFunc, Func_417a2, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_0F); }
PORT_LOCAL_EX(ParaGoomFunc, Func_417b1, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_3F); }
PORT_LOCAL_EX(ParaGoomFunc, Func_417cb, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_10); }
PORT_LOCAL_EX(ParaGoomFunc, Func_417e8, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_40); }
PORT_LOCAL_EX(ParaGoomFunc, Func_4181c, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_5A); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41830, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::DROPPING); }
PORT_LOCAL_EX(ParaGoomFunc, Func_4186f, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::LANDED); }
PORT_LOCAL_EX(ParaGoomFunc, Func_4188b, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::TURN_START); }
PORT_LOCAL_EX(ParaGoomFunc, Func_418c4, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::TURNING); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41904, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::WOBBLE_START); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41912, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::WOBBLE_LEFT); }
PORT_LOCAL_EX(ParaGoomFunc, Func_4191c, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::WOBBLE_RIGHT); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41954, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_0B); }
PORT_LOCAL_EX(ParaGoomFunc, Func_4195b, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_0C); }
PORT_LOCAL_EX(ParaGoomFunc, Func_4196c, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_3B); }
PORT_LOCAL_EX(ParaGoomFunc, Func_4197f, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_3C); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41992, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_29); }
PORT_LOCAL_EX(ParaGoomFunc, Func_4199c, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_28); }
PORT_LOCAL_EX(ParaGoomFunc, Func_419a6, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::ATTACKED_LEFT); }
PORT_LOCAL_EX(ParaGoomFunc, Func_419b0, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::ATTACKED_RIGHT); }
PORT_LOCAL_EX(ParaGoomFunc, Func_419d5, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::PICKED_UP_LEFT); }
PORT_LOCAL_EX(ParaGoomFunc, Func_419e8, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::PICKED_UP_RIGHT); }
PORT_LOCAL_EX(ParaGoomFunc, Func_419fb, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_19); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41a1f, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_49); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41a2f, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_18); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41a53, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_48); }
PORT_LOCAL_EX(ParaGoomFunc, Func_41a64, 0, ALL_REGISTERS) { ParaGoom(gb).run(wl3::level::STATE_12); }
