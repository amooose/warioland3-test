// Spearhead (spearhead.asm): a spiked shell walking the floor (asleep at
// night until Wario's close), its spikes out ahead after a moment, turning
// round at walls and edges; knocked over, it lands spikes out and angry.
// Knocked about like other enemies, through a table of 96 state handlers.
// See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_SpearheadFunc + offset); }

constexpr uint16_t INIT = L(0x0e); // .Init
constexpr uint16_t UPDATE = L(0x29); // .Update
constexpr uint16_t H_FULL_THROW_LEFT_START = L(0xed); // .FullThrowLeftStart
constexpr uint16_t H_FULL_THROW_RIGHT_START = L(0x109); // .FullThrowRightStart
constexpr uint16_t H_SLEEP_START = L(0x125); // .SleepStart
constexpr uint16_t H_SLEEP = L(0x142); // .Sleep
constexpr uint16_t H_WAKE_UP = L(0x1d3); // .WakeUp
constexpr uint16_t H_DEFAULT = L(0x1db); // .Default
constexpr uint16_t H_STANDING_FALL_START = L(0x21d); // .StandingFallStart
constexpr uint16_t H_LAND_WITH_STING = L(0x239); // .LandWithSting
constexpr uint16_t H_ANGRY = L(0x26a); // .Angry
constexpr uint16_t H_LAND = L(0x298); // .Land
constexpr uint16_t H_WALK = L(0x2cf); // .Walk
constexpr uint16_t H_BUMP_LEFT_START = L(0x382); // .BumpLeftStart
constexpr uint16_t H_BUMP_RIGHT_START = L(0x396); // .BumpRightStart
constexpr uint16_t H_STUN_LEFT_START = L(0x3c4); // .StunLeftStart
constexpr uint16_t H_STUN_LEFT = L(0x3e9); // .StunLeft
constexpr uint16_t H_STUN_RIGHT_START = L(0x3f7); // .StunRightStart
constexpr uint16_t H_STUN_RIGHT = L(0x41c); // .StunRight
constexpr uint16_t H_STATE14 = L(0x42a); // .State14
constexpr uint16_t H_STATE15 = L(0x432); // .State15
constexpr uint16_t H_FUNC_40478 = L(0x438); // .Func_40478
constexpr uint16_t H_STATE0F = L(0x462); // .State0f
constexpr uint16_t H_STATE3F = L(0x471); // .State3f
constexpr uint16_t H_STATE10 = L(0x48b); // .State10
constexpr uint16_t H_STATE40 = L(0x499); // .State40
constexpr uint16_t H_TURN_AROUND_START = L(0x4ab); // .TurnAroundStart
constexpr uint16_t H_TURN_AROUND = L(0x4ca); // .TurnAround
constexpr uint16_t H_WOBBLE_LEFT_START = L(0x52b); // .WobbleLeftStart
constexpr uint16_t H_WOBBLE_RIGHT_START = L(0x535); // .WobbleRightStart
constexpr uint16_t H_FUNC_4057D = L(0x53d); // .Func_4057d
constexpr uint16_t H_STATE0B = L(0x56a); // .State0b
constexpr uint16_t H_STATE0C = L(0x571); // .State0c
constexpr uint16_t H_FUNC_405B6 = L(0x576); // .Func_405b6
constexpr uint16_t H_STATE3B = L(0x582); // .State3b
constexpr uint16_t H_STATE3C = L(0x595); // .State3c
constexpr uint16_t H_STATE29 = L(0x5a8); // .State29
constexpr uint16_t H_STATE28 = L(0x5b2); // .State28
constexpr uint16_t H_ATTACKED_LEFT_START = L(0x5bc); // .AttackedLeftStart
constexpr uint16_t H_ATTACKED_RIGHT_START = L(0x5c6); // .AttackedRightStart
constexpr uint16_t H_PICKED_UP_LEFT_START = L(0x5e0); // .PickedUpLeftStart
constexpr uint16_t H_PICKED_UP_RIGHT_START = L(0x5f3); // .PickedUpRightStart
constexpr uint16_t H_STATE19 = L(0x606); // .State19
constexpr uint16_t H_STATE49 = L(0x62a); // .State49
constexpr uint16_t H_STATE18 = L(0x63a); // .State18
constexpr uint16_t H_STATE48 = L(0x65e); // .State48
constexpr uint16_t H_STATE12 = L(0x66f); // .State12

// .Update's jumptable, by state
constexpr uint16_t HANDLERS[] = {
	H_DEFAULT, H_LAND, H_BUMP_LEFT_START, H_BUMP_RIGHT_START, H_ATTACKED_LEFT_START, H_ATTACKED_RIGHT_START, H_PICKED_UP_LEFT_START, H_PICKED_UP_RIGHT_START, // $00
	H_WOBBLE_LEFT_START, H_WOBBLE_RIGHT_START, ADDR_VanishObject2, H_STATE0B, H_STATE0C, H_DEFAULT, H_DEFAULT, H_STATE0F,
	H_STATE10, H_STANDING_FALL_START, H_STATE12, H_TURN_AROUND_START, H_STATE14, H_STATE15, H_STUN_LEFT_START, H_STUN_RIGHT_START, // $10
	H_STATE18, H_STATE19, ADDR_Func_3263, ADDR_Func_3272, ADDR_Func_3281, H_DEFAULT, H_DEFAULT, H_DEFAULT,
	ADDR_Func_3191, ADDR_Func_31af, ADDR_Func_31cd, ADDR_Func_31eb, ADDR_Func_3209, ADDR_Func_3227, ADDR_Func_3245, ADDR_Func_3254, // $20
	H_STATE28, H_STATE29, H_DEFAULT, H_DEFAULT, H_DEFAULT, H_SLEEP, H_ANGRY, H_WAKE_UP,
	ADDR_ObjState_Fall, H_WALK, ADDR_ObjState_BumpLeft, ADDR_ObjState_BumpRight, ADDR_Func_3326, ADDR_Func_3317, ADDR_Func_3335, ADDR_Func_3344, // $30
	ADDR_Func_31fa, ADDR_Func_31dc, H_DEFAULT, H_STATE3B, H_STATE3C, H_DEFAULT, H_DEFAULT, H_STATE3F,
	H_STATE40, ADDR_ObjState_StandingFall, ADDR_Func_3371, H_TURN_AROUND, ADDR_Func_32cc, ADDR_Func_32db, H_STUN_LEFT, H_STUN_RIGHT, // $40
	H_STATE48, H_STATE49, ADDR_Func_3380, ADDR_Func_338f, H_FULL_THROW_LEFT_START, H_FULL_THROW_RIGHT_START, ADDR_Func_339e, ADDR_Func_33ad,
	ADDR_Func_31a0, ADDR_Func_31be, ADDR_Func_31dc, ADDR_Func_31fa, ADDR_Func_3218, ADDR_Func_3236, ADDR_Func_32cc, ADDR_Func_32db, // $50
	ADDR_Func_33f8, ADDR_Func_3407, H_DEFAULT, H_DEFAULT, H_DEFAULT, H_DEFAULT, ADDR_ObjState_FullThrowLeft, ADDR_ObjState_FullThrowRight,
};
constexpr uint8_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

class Spearhead : public Object {
public:
	using Object::Object;

	void start(); // SpearheadFunc
	void init(); // .Init
	void update(); // .Update
	void run(uint16_t handler);
	void dazed(bool right); // .Func_40478
	void wobble(uint16_t at, uint8_t s); // .Func_4057d: s written at `at`
	void rolled(); // .Func_405b6

private:
	uint16_t facing(uint16_t left, uint16_t right) { return facing_right() ? right : left; }
	bool wario_to_right() const;
	bool shaken();
	bool on_floor(); // else state OBJSTATE_STANDING_FALL_START
	void box(bool right); // spikes out on that side
	void small_box();
	void land(uint8_t interaction, uint8_t s, uint16_t left, uint16_t right, uint8_t duration);
	void var2_move(uint16_t routine, uint8_t s);
	void stun_start(uint8_t s);
	void grab(uint8_t s);
};

} // namespace

bool Spearhead::wario_to_right() const
{
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	return static_cast<uint8_t>(screen_x() + 0x2a) < wario;
}

void Spearhead::start()
{
	set_update(INIT);
	var1() = 0;
}

void Spearhead::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	run((mem_[wDayNight] & 1) ? H_SLEEP_START : H_DEFAULT);
}

void Spearhead::update()
{
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

bool Spearhead::shaken()
{
	if (mem_[wGroundShakeCounter] < 0x10)
		return false;
	state() = static_cast<uint8_t>((OBJSTATE_WOBBLE_LEFT_START & ~1) | (mem_[rDIV] & 1));
	return true;
}

bool Spearhead::on_floor()
{
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_3543);
	if (!gb_->cpu.a) {
		state() = OBJSTATE_STANDING_FALL_START;
		return false;
	}
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	return true;
}

void Spearhead::box(bool right)
{
	mem_[wCurObjCollBoxRight] = right ? 9 : 6;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(right ? -7 : -10);
}

void Spearhead::small_box()
{
	mem_[wCurObjCollBoxRight] = 5;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-6);
}

void Spearhead::land(uint8_t interaction, uint8_t s, uint16_t left, uint16_t right, uint8_t duration)
{
	set_interaction(interaction);
	state() = s;
	bool r = facing_right();
	box(r);
	set_frameset(r ? right : left);
	state_duration() = duration;
}

void Spearhead::var2_move(uint16_t routine, uint8_t s)
{
	if (var2())
		routines_.call(routine);
	else
		state() = s;
}

void Spearhead::stun_start(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_681e4, ADDR_Frameset_681ed));
	small_box();
	set_interaction(OBJ_INTERACTION_00);
}

void Spearhead::dazed(bool right)
{
	set_frameset(right ? ADDR_Frameset_681ed : ADDR_Frameset_681e4);
	var2() = 0x02;
	var3() = 0;
	small_box();
	set_interaction(OBJ_INTERACTION_00);
}

void Spearhead::wobble(uint16_t at, uint8_t s)
{
	mem_[at] = s;
	bool right = mem_[static_cast<uint16_t>(at - 1)] & 0x80;
	set_frameset(right ? ADDR_Frameset_681ed : ADDR_Frameset_681e4);
	state_duration() = 4;
	var2() = 0x02;
	var3() = 0;
	small_box();
	set_interaction(OBJ_INTERACTION_00);
}

void Spearhead::rolled()
{
	frame_duration() = 0;
	mem_[wCurObjFramesetOffset] = 0;
	var2() = 0x02;
}

void Spearhead::grab(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_681e4, ADDR_Frameset_681ed));
	small_box();
	flags() |= 1 << OBJFLAG_GRABBED_F;
}

void Spearhead::run(uint16_t handler)
{
	switch (handler) {
	case H_FULL_THROW_LEFT_START:
	case H_FULL_THROW_RIGHT_START: {
		bool left = handler == H_FULL_THROW_LEFT_START;
		state() = left ? OBJSTATE_FULL_THROW_LEFT : OBJSTATE_FULL_THROW_RIGHT;
		set_frameset(facing(ADDR_Frameset_68292, ADDR_Frameset_6829b));
		routines_.call(left ? ADDR_ObjState_FullThrowLeft : ADDR_ObjState_FullThrowRight);
		return;
	}
	case H_SLEEP_START:
		set_interaction(OBJ_INTERACTION_01);
		state() = OBJSTATE_SPECIAL_1;
		set_frameset(ADDR_Frameset_68234);
		small_box();
		return;
	case H_SLEEP: { // wakes when Wario's close
		if (wario_to_right())
			substate() &= ~OBJSUBFLAG_HDIR;
		else
			substate() |= OBJSUBFLAG_HDIR;
		uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
		if (dy < 32 || dy >= 0xe0) {
			uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
			if (dx >= 0xe0 || dx < 32) {
				state() = OBJSTATE_SPECIAL_3;
				if (facing_right()) {
					box(true);
					set_frameset(ADDR_Frameset_68287);
				} else {
					set_frameset(ADDR_Frameset_6827c);
				}
				state_duration() = 32;
				return;
			}
		}
		if (shaken())
			return;
		on_floor();
		return;
	}
	case H_WAKE_UP:
		if (!--state_duration())
			run(H_TURN_AROUND_START);
		return;
	case H_DEFAULT: { // facing Wario
		set_interaction(OBJ_INTERACTION_01);
		state() = OBJSTATE_FALL;
		bool right = wario_to_right();
		if (right)
			substate() |= OBJSUBFLAG_HDIR;
		else
			substate() &= ~OBJSUBFLAG_HDIR;
		var3() = 0;
		box(right);
		set_frameset(right ? ADDR_Frameset_681db : ADDR_Frameset_681d2);
		return;
	}
	case H_STANDING_FALL_START:
		state() = OBJSTATE_STANDING_FALL;
		var3() = 0;
		set_frameset(facing(ADDR_Frameset_681d2, ADDR_Frameset_681db));
		return;
	case H_LAND_WITH_STING: // knocked over: lands spikes out, angry a while
		land(OBJ_INTERACTION_FRONT_STING, OBJSTATE_SPECIAL_2, ADDR_Frameset_68208, ADDR_Frameset_68213, 32);
		return;
	case H_ANGRY:
		if (shaken())
			return;
		if (--state_duration())
			return;
		var1() = 0;
		state() = OBJSTATE_WALK;
		set_frameset(facing(ADDR_Frameset_681d2, ADDR_Frameset_681db));
		return;
	case H_LAND:
		if (var1() == 0xff)
			land(OBJ_INTERACTION_FRONT_STING, OBJSTATE_SPECIAL_2, ADDR_Frameset_68208, ADDR_Frameset_68213, 32);
		else
			land(OBJ_INTERACTION_01, OBJSTATE_WALK, ADDR_Frameset_681d2, ADDR_Frameset_681db, 10);
		return;
	case H_WALK: { // the spikes come out after a moment
		if (shaken())
			return;
		if (state_duration()) {
			if (!--state_duration())
				set_interaction(OBJ_INTERACTION_FRONT_STING);
			return;
		}
		if (!on_floor())
			return;
		bool right = facing_right();
		uint8_t in_block = mem_[wCurObjXPos + 0] & 0x0f;
		if (right ? in_block + 9 >= 0x10 : in_block < 10) {
			routines_.call(right ? ADDR_ProcessBlockOnRight : ADDR_ProcessBlockOnLeft);
			if (!(gb_->cpu.a & 0x0f)) {
				state() = OBJSTATE_TURN_AROUND_START;
				return;
			}
		}
		uint16_t y_pos = static_cast<uint16_t>(y() - 4);
		uint16_t x_pos = static_cast<uint16_t>(right ? x() + 9 : x() - 10);
		mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
		mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
		mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
		mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
		routines_.call(ADDR_Func_3513);
		if (gb_->cpu.a) {
			state() = OBJSTATE_TURN_AROUND_START;
			return;
		}
		routines_.call(right ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
		return;
	}
	case H_BUMP_LEFT_START:
	case H_BUMP_RIGHT_START:
		if (handler == H_BUMP_LEFT_START) {
			state() = OBJSTATE_BUMP_LEFT;
			set_frameset(facing(ADDR_Frameset_6826e, ADDR_Frameset_681f6));
		} else {
			state() = OBJSTATE_BUMP_RIGHT;
			set_frameset(facing(ADDR_Frameset_681ff, ADDR_Frameset_68265));
		}
		state_duration() = 12;
		var2() = 0x02;
		small_box();
		set_interaction(OBJ_INTERACTION_01);
		return;
	case H_STUN_LEFT_START:
		stun_start(OBJSTATE_STUN_LEFT);
		[[fallthrough]];
	case H_STUN_LEFT:
		var2_move(ADDR_Func_32ae, OBJSTATE_0F);
		return;
	case H_STUN_RIGHT_START:
		stun_start(OBJSTATE_STUN_RIGHT);
		[[fallthrough]];
	case H_STUN_RIGHT:
		var2_move(ADDR_Func_32bd, OBJSTATE_0F);
		return;
	case H_STATE14:
	case H_STATE15:
		state() = handler == H_STATE14 ? OBJSTATE_44 : OBJSTATE_45;
		dazed(facing_right());
		return;
	case H_STATE0F:
		state() = OBJSTATE_3F;
		state_duration() = 100;
		flags() &= ~(1 << OBJFLAG_GRABBED_F);
		[[fallthrough]];
	case H_STATE3F:
		if (mem_[wGroundShakeCounter] >= 0x10) {
			state() = OBJSTATE_1C;
			return;
		}
		if (--state_duration())
			routines_.call(ADDR_Func_32ea);
		else
			state() = OBJSTATE_10;
		return;
	case H_STATE10:
		state() = OBJSTATE_40;
		var3() = 0;
		state_duration() = 7;
		[[fallthrough]];
	case H_STATE40:
		if (--state_duration()) {
			routines_.call(ADDR_Func_32f9);
			return;
		}
		state() = OBJSTATE_00;
		var1() = 0xff; // lands spikes out
		return;
	case H_TURN_AROUND_START:
		state() = OBJSTATE_TURN_AROUND;
		set_frameset(facing(ADDR_Frameset_6824f, ADDR_Frameset_6825a));
		state_duration() = 48;
		var1() = 0x1c;
		return;
	case H_TURN_AROUND: // spikes in, then round
		if (shaken())
			return;
		if (state_duration()) {
			if (!--state_duration()) {
				small_box();
				set_interaction(OBJ_INTERACTION_01);
				set_frameset(facing(ADDR_Frameset_6821e, ADDR_Frameset_68229));
			}
			routines_.call(ADDR_Func_3308);
			return;
		}
		if (--var1()) {
			routines_.call(ADDR_Func_3308);
			return;
		}
		state() = OBJSTATE_LAND;
		substate() ^= OBJSUBFLAG_HDIR;
		return;
	case H_WOBBLE_LEFT_START:
	case H_WOBBLE_RIGHT_START:
		mem_[wCurObjAction] = 15 | (1 << 7);
		wobble(wCurObjState, handler == H_WOBBLE_LEFT_START ? OBJSTATE_WOBBLE_LEFT : OBJSTATE_WOBBLE_RIGHT);
		return;
	case H_STATE0B:
		state() = OBJSTATE_3B;
		rolled();
		return;
	case H_STATE0C:
		state() = OBJSTATE_3C;
		rolled();
		return;
	case H_STATE3B:
	case H_STATE3C:
		frame_duration() = 0;
		mem_[wCurObjFramesetOffset] = 0;
		var2_move(handler == H_STATE3B ? ADDR_Func_32bd : ADDR_Func_32ae, OBJSTATE_TURN_AROUND_START);
		return;
	case H_STATE29:
	case H_STATE28:
	case H_ATTACKED_LEFT_START:
	case H_ATTACKED_RIGHT_START:
		state() = handler == H_STATE29 ? OBJSTATE_59
			: handler == H_STATE28 ? OBJSTATE_58
			: handler == H_ATTACKED_LEFT_START ? OBJSTATE_ATTACKED_LEFT
			: OBJSTATE_ATTACKED_RIGHT;
		substate() &= OBJSUBFLAGS_MASK;
		var3() = 0;
		var2() = 0x02;
		no_actions();
		set_frameset(ADDR_Frameset_68277);
		return;
	case H_PICKED_UP_LEFT_START:
	case H_PICKED_UP_RIGHT_START: {
		bool left = handler == H_PICKED_UP_LEFT_START;
		uint16_t data = left ? ADDR_Data_601c0 : ADDR_Data_601e0;
		state() = left ? OBJSTATE_PICKED_UP_LEFT : OBJSTATE_PICKED_UP_RIGHT;
		var3() = static_cast<uint8_t>(data >> 8);
		var2() = static_cast<uint8_t>(data);
		var1() = 0x01;
		routines_.call(left ? ADDR_Func_3335 : ADDR_Func_3344);
		return;
	}
	case H_STATE19:
		grab(OBJSTATE_49);
		[[fallthrough]];
	case H_STATE49: // carried, turning with Wario
		if (mem_[wDirection]) {
			routines_.call(ADDR_Func_3353);
			return;
		}
		substate() ^= OBJSUBFLAG_HDIR;
		run(H_STATE18);
		return;
	case H_STATE18:
		grab(OBJSTATE_48);
		[[fallthrough]];
	case H_STATE48:
		if (!mem_[wDirection]) {
			routines_.call(ADDR_Func_3362);
			return;
		}
		substate() ^= OBJSUBFLAG_HDIR;
		run(H_STATE19);
		return;
	case H_STATE12:
		mem_[wCurObjAction] = 31 | (1 << 7);
		state() = OBJSTATE_42;
		var3() = 0;
		set_interaction(OBJ_INTERACTION_00);
		return;
	default: // a shared object state
		routines_.call(handler);
		return;
	}
}

} // namespace wl3::level

using wl3::level::Spearhead;

PORT_EX(SpearheadFunc, 0, ALL_REGISTERS) { Spearhead(gb).start(); }
PORT_LOCAL_EX(SpearheadFunc, Init, 0, ALL_REGISTERS) { Spearhead(gb).init(); }
PORT_LOCAL_EX(SpearheadFunc, Update, 0, ALL_REGISTERS) { Spearhead(gb).update(); }
PORT_LOCAL_EX(SpearheadFunc, Func_40478, 0, ALL_REGISTERS) { Spearhead(gb).dazed(wl3::Memory(gb)[REG_HL(gb)] & 0x80); }
PORT_LOCAL_EX(SpearheadFunc, Func_4057d, 0, ALL_REGISTERS) { Spearhead(gb).wobble(REG_HL(gb), gb->cpu.a); }
PORT_LOCAL_EX(SpearheadFunc, Func_405b6, 0, ALL_REGISTERS) { Spearhead(gb).rolled(); }
PORT_LOCAL_EX(SpearheadFunc, FullThrowLeftStart, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_FULL_THROW_LEFT_START); }
PORT_LOCAL_EX(SpearheadFunc, FullThrowRightStart, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_FULL_THROW_RIGHT_START); }
PORT_LOCAL_EX(SpearheadFunc, SleepStart, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_SLEEP_START); }
PORT_LOCAL_EX(SpearheadFunc, Sleep, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_SLEEP); }
PORT_LOCAL_EX(SpearheadFunc, WakeUp, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_WAKE_UP); }
PORT_LOCAL_EX(SpearheadFunc, Default, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_DEFAULT); }
PORT_LOCAL_EX(SpearheadFunc, StandingFallStart, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STANDING_FALL_START); }
PORT_LOCAL_EX(SpearheadFunc, LandWithSting, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_LAND_WITH_STING); }
PORT_LOCAL_EX(SpearheadFunc, Angry, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_ANGRY); }
PORT_LOCAL_EX(SpearheadFunc, Land, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_LAND); }
PORT_LOCAL_EX(SpearheadFunc, Walk, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_WALK); }
PORT_LOCAL_EX(SpearheadFunc, BumpLeftStart, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_BUMP_LEFT_START); }
PORT_LOCAL_EX(SpearheadFunc, BumpRightStart, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_BUMP_RIGHT_START); }
PORT_LOCAL_EX(SpearheadFunc, StunLeftStart, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STUN_LEFT_START); }
PORT_LOCAL_EX(SpearheadFunc, StunLeft, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STUN_LEFT); }
PORT_LOCAL_EX(SpearheadFunc, StunRightStart, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STUN_RIGHT_START); }
PORT_LOCAL_EX(SpearheadFunc, StunRight, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STUN_RIGHT); }
PORT_LOCAL_EX(SpearheadFunc, State14, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE14); }
PORT_LOCAL_EX(SpearheadFunc, State15, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE15); }
PORT_LOCAL_EX(SpearheadFunc, State0f, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE0F); }
PORT_LOCAL_EX(SpearheadFunc, State3f, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE3F); }
PORT_LOCAL_EX(SpearheadFunc, State10, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE10); }
PORT_LOCAL_EX(SpearheadFunc, State40, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE40); }
PORT_LOCAL_EX(SpearheadFunc, TurnAroundStart, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_TURN_AROUND_START); }
PORT_LOCAL_EX(SpearheadFunc, TurnAround, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_TURN_AROUND); }
PORT_LOCAL_EX(SpearheadFunc, WobbleLeftStart, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_WOBBLE_LEFT_START); }
PORT_LOCAL_EX(SpearheadFunc, WobbleRightStart, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_WOBBLE_RIGHT_START); }
PORT_LOCAL_EX(SpearheadFunc, State0b, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE0B); }
PORT_LOCAL_EX(SpearheadFunc, State0c, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE0C); }
PORT_LOCAL_EX(SpearheadFunc, State3b, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE3B); }
PORT_LOCAL_EX(SpearheadFunc, State3c, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE3C); }
PORT_LOCAL_EX(SpearheadFunc, State29, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE29); }
PORT_LOCAL_EX(SpearheadFunc, State28, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE28); }
PORT_LOCAL_EX(SpearheadFunc, AttackedLeftStart, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_ATTACKED_LEFT_START); }
PORT_LOCAL_EX(SpearheadFunc, AttackedRightStart, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_ATTACKED_RIGHT_START); }
PORT_LOCAL_EX(SpearheadFunc, PickedUpLeftStart, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_PICKED_UP_LEFT_START); }
PORT_LOCAL_EX(SpearheadFunc, PickedUpRightStart, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_PICKED_UP_RIGHT_START); }
PORT_LOCAL_EX(SpearheadFunc, State19, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE19); }
PORT_LOCAL_EX(SpearheadFunc, State49, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE49); }
PORT_LOCAL_EX(SpearheadFunc, State18, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE18); }
PORT_LOCAL_EX(SpearheadFunc, State48, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE48); }
PORT_LOCAL_EX(SpearheadFunc, State12, 0, ALL_REGISTERS) { Spearhead(gb).run(wl3::level::H_STATE12); }
