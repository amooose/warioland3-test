// The Doughnuteer (doughnuteer.asm): walks the floor and, when Wario's
// level and close in front, throws a doughnut, then runs off and turns back
// to throw again; bumping him throws one up. Knocked about like other
// enemies, through a table of 93 state handlers. And the doughnuts: they
// fly level, or up and fall, then break. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_DoughnuteerFunc + offset); }

constexpr uint16_t INIT = L(0x0a); // .Init
constexpr uint16_t UPDATE = L(0x1e); // .Update
constexpr uint16_t THROWING = L(0xdc); // .Func_41b58
constexpr uint16_t TURN_TO_RUN = L(0x132); // .Func_41bae
constexpr uint16_t RUNNING = L(0x160); // .Func_41bdc
constexpr uint16_t TURN_TO_THROW = L(0x20f); // .Func_41c8b
constexpr uint16_t PAUSE = L(0x245); // .Func_41cc1
constexpr uint16_t DEFAULT = L(0x25f); // .Func_41cdb
constexpr uint16_t STATE_41 = L(0x29e); // .Func_41d1a
constexpr uint16_t SET_WALK = L(0x2ba); // .Func_41d36
constexpr uint16_t WALKING = L(0x2da); // .Func_41d56
constexpr uint16_t BUMP_LEFT_START = L(0x3be); // .Func_41e3a
constexpr uint16_t BUMP_LEFT = L(0x3d7); // .Func_41e53
constexpr uint16_t BUMP_RIGHT_START = L(0x400); // .Func_41e7c
constexpr uint16_t BUMP_RIGHT = L(0x432); // .Func_41eae
constexpr uint16_t STUN_LEFT_START = L(0x45b); // .Func_41ed7
constexpr uint16_t STUN_LEFT = L(0x478); // .Func_41ef4
constexpr uint16_t STUN_RIGHT_START = L(0x486); // .Func_41f02
constexpr uint16_t STUN_RIGHT = L(0x4a3); // .Func_41f1f
constexpr uint16_t STATE_44 = L(0x4b1); // .Func_41f2d
constexpr uint16_t STATE_45 = L(0x4b9); // .Func_41f35
constexpr uint16_t STATE_0F = L(0x4e1); // .Func_41f5d
constexpr uint16_t STATE_3F = L(0x4f0); // .Func_41f6c
constexpr uint16_t STATE_10 = L(0x50a); // .Func_41f86
constexpr uint16_t STATE_40 = L(0x518); // .Func_41f94
constexpr uint16_t TURN_START = L(0x525); // .Func_41fa1
constexpr uint16_t TURNING = L(0x541); // .Func_41fbd
constexpr uint16_t WOBBLE_LEFT = L(0x56d); // .Func_41fe9
constexpr uint16_t WOBBLE_RIGHT = L(0x577); // .Func_41ff3
constexpr uint16_t STATE_29 = L(0x5a4); // .Func_42020
constexpr uint16_t STATE_28 = L(0x5ae); // .Func_4202a
constexpr uint16_t ATTACKED_LEFT = L(0x5b8); // .Func_42034
constexpr uint16_t ATTACKED_RIGHT = L(0x5c2); // .Func_4203e
constexpr uint16_t PICKED_UP_LEFT = L(0x5e8); // .Func_42064
constexpr uint16_t PICKED_UP_RIGHT = L(0x5fb); // .Func_42077
constexpr uint16_t STATE_19 = L(0x60e); // .Func_4208a
constexpr uint16_t STATE_49 = L(0x62a); // .Func_420a6
constexpr uint16_t STATE_18 = L(0x63a); // .Func_420b6
constexpr uint16_t STATE_48 = L(0x656); // .Func_420d2
constexpr uint16_t STATE_12 = L(0x667); // .Func_420e3

// .Update's jumptable, by state
constexpr uint16_t HANDLERS[] = {
	DEFAULT, SET_WALK, BUMP_LEFT_START, BUMP_RIGHT_START, ATTACKED_LEFT, ATTACKED_RIGHT, PICKED_UP_LEFT, PICKED_UP_RIGHT, // $00
	WOBBLE_LEFT, WOBBLE_RIGHT, ADDR_VanishObject2, DEFAULT, DEFAULT, DEFAULT, DEFAULT, STATE_0F,
	STATE_10, STATE_41, STATE_12, TURN_START, STATE_44, STATE_45, STUN_LEFT_START, STUN_RIGHT_START, // $10
	STATE_18, STATE_19, ADDR_Func_3263, ADDR_Func_3272, ADDR_Func_3281, DEFAULT, DEFAULT, DEFAULT,
	ADDR_Func_3191, ADDR_Func_31af, ADDR_Func_31cd, ADDR_Func_31eb, ADDR_Func_3209, ADDR_Func_3227, ADDR_Func_3245, ADDR_Func_3254, // $20
	STATE_28, STATE_29, DEFAULT, TURN_TO_RUN, TURN_TO_THROW, DEFAULT, DEFAULT, DEFAULT,
	ADDR_ObjState_Fall, WALKING, BUMP_LEFT, BUMP_RIGHT, ADDR_Func_3326, ADDR_Func_3317, ADDR_Func_3335, ADDR_Func_3344, // $30
	ADDR_Func_31fa, ADDR_Func_31dc, DEFAULT, DEFAULT, DEFAULT, DEFAULT, DEFAULT, STATE_3F,
	STATE_40, ADDR_ObjState_StandingFall, ADDR_Func_3371, TURNING, ADDR_Func_32cc, ADDR_Func_32db, STUN_LEFT, STUN_RIGHT, // $40
	STATE_48, STATE_49, ADDR_Func_3380, ADDR_Func_338f, ADDR_ObjState_FullThrowLeft, ADDR_ObjState_FullThrowRight, ADDR_Func_339e, ADDR_Func_33ad,
	ADDR_Func_31a0, ADDR_Func_31be, ADDR_Func_31dc, ADDR_Func_31fa, ADDR_Func_3218, ADDR_Func_3236, ADDR_Func_32cc, ADDR_Func_32db, // $50
	ADDR_Func_33f8, ADDR_Func_3407, THROWING, RUNNING, PAUSE,
};
constexpr uint8_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

constexpr uint16_t DOUGHNUT_UPDATE_LEFT = ADDR_DoughnutFunc + 0x21; // .Update_Left
constexpr uint16_t DOUGHNUT_UPDATE_RIGHT = ADDR_DoughnutFunc + 0x26; // .Update_Right
constexpr uint16_t DOUGHNUT_UP_UPDATE = ADDR_DoughnutFunc_Up + 0x11; // .Update

class Doughnuteer : public Object {
public:
	using Object::Object;

	void start() { set_update(INIT); } // DoughnuteerFunc
	void init(); // .Init
	void update(); // .Update
	void run(uint16_t handler);

private:
	uint16_t facing(uint16_t left, uint16_t right) { return facing_right() ? right : left; }
	bool shaken(uint8_t s); // to state s, with the ground shaking
	bool turned(); // turns at 8; true when the duration ran out
	int walk(bool slow); // 0: moved (or fell), else blocked 1 left, 2 right
	void bump_start(uint8_t s, uint16_t params);
	void bumped(bool right);
	void var2_move(uint16_t routine);
	void dazed(uint8_t s);
	void wobble(uint8_t s);
	void knocked(uint8_t s);
	void grab(uint8_t s);
};

class Doughnut : public Object {
public:
	using Object::Object;

	void aim(); // DoughnutFunc
	void destroy() { flags() = 0; } // DoughnutFunc_Destroy
	void fly(uint16_t page); // DoughnutFunc_LeftRight
	void fly_after(uint16_t move) { routines_.call(move); fly(gb_->cpu.h << 8); } // DoughnutFunc_Left / _Right
	void up(); // DoughnutFunc_Up
	void moving(uint16_t movement); // DoughnutFunc_Up.Update and the rest of DoughnutFunc_LeftRight
	void at_wario(uint16_t move); // DoughnutFunc.Update_Left / _Right
};

} // namespace

void Doughnuteer::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	run(DEFAULT);
}

void Doughnuteer::update()
{
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

bool Doughnuteer::shaken(uint8_t s)
{
	if (mem_[wGroundShakeCounter] < 0x10)
		return false;
	state() = s;
	return true;
}

bool Doughnuteer::turned()
{
	if (state_duration() == 0x08) {
		--state_duration();
		substate() ^= OBJSUBFLAG_HDIR;
		return false;
	}
	if (--state_duration()) {
		routines_.call(ADDR_Func_3308);
		return false;
	}
	return true;
}

int Doughnuteer::walk(bool slow)
{
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_3543);
	if (!gb_->cpu.a) {
		state() = 0x11;
		return 0;
	}
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	bool right = facing_right();
	uint8_t in_block = mem_[wCurObjXPos + 0] & 0x0f;
	if (right ? in_block + 5 >= 0x10 : in_block < 6) {
		routines_.call(right ? ADDR_ProcessBlockOnRight : ADDR_ProcessBlockOnLeft);
		if (!(gb_->cpu.a & 0x0f))
			return right ? 2 : 1;
	}
	uint16_t y_pos = static_cast<uint16_t>(y() - 4);
	uint16_t x_pos = static_cast<uint16_t>(right ? x() + 5 : x() - 6);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a)
		return right ? 2 : 1;
	if (slow)
		routines_.call(right ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
	else
		routines_.call(right ? ADDR_MoveObjectRight : ADDR_MoveObjectLeft);
	return 0;
}

// a doughnut thrown up, unless the frame's in [6, $24)
void Doughnuteer::bump_start(uint8_t s, uint16_t params)
{
	uint8_t f = frame();
	if (f >= 0x24 || f < 0x06) {
		SET_BC(gb_, params);
		routines_.call(ADDR_CreateObjectAtRelativePos);
	}
	state() = s;
	set_frameset(facing(ADDR_Frameset_685a8, ADDR_Frameset_685ad));
	state_duration() = 0x0c;
	var2() = 0x02;
}

// then runs on away from the bump, or turns to throw again
void Doughnuteer::bumped(bool right)
{
	if (var2()) {
		routines_.call(right ? ADDR_ObjState_BumpRight : ADDR_ObjState_BumpLeft);
		return;
	}
	if (--state_duration())
		return;
	bool r = facing_right();
	if (r != right) {
		state() = 0x5b;
		set_frameset(r ? ADDR_Frameset_6857d : ADDR_Frameset_68574);
		return;
	}
	state() = 0x2b;
	set_frameset(r ? ADDR_Frameset_68605 : ADDR_Frameset_685fe);
	state_duration() = 0x1a;
}

void Doughnuteer::var2_move(uint16_t routine)
{
	if (var2())
		routines_.call(routine);
	else
		state() = 0x0f;
}

void Doughnuteer::dazed(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_685b2, ADDR_Frameset_685bb));
	var2() = 0x02;
	var3() = 0;
	set_interaction(OBJ_INTERACTION_00);
}

void Doughnuteer::wobble(uint8_t s)
{
	mem_[wCurObjAction] = 0x8f;
	state() = s;
	set_frameset(facing(ADDR_Frameset_685b2, ADDR_Frameset_685bb));
	state_duration() = 0x04;
	var2() = 0x02;
	var3() = 0;
	set_interaction(OBJ_INTERACTION_00);
}

void Doughnuteer::knocked(uint8_t s)
{
	state() = s;
	no_actions();
	uint8_t sub = substate() & 0xf0;
	substate() = sub;
	var3() = 0;
	var2() = 0x02;
	set_frameset((sub & 0x80) ? ADDR_Frameset_685bb : ADDR_Frameset_685b2);
}

void Doughnuteer::grab(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_685b2, ADDR_Frameset_685bb));
	flags() |= OBJFLAG_GRABBED;
}

void Doughnuteer::run(uint16_t handler)
{
	switch (handler) {
	case THROWING:
		if (shaken(0x1a))
			return;
		if (state_duration() == 0x1a) { // the doughnut away
			--state_duration();
			if (flags() & (1 << OBJFLAG_ON_SCREEN_F))
				play_sfx(SFX_067);
			SET_BC(gb_, facing_right() ? ADDR_ObjParams_DoughnutRight : ADDR_ObjParams_DoughnutLeft);
			routines_.call(ADDR_CreateObjectAtRelativePos);
			return;
		}
		if (--state_duration()) {
			routines_.call(ADDR_Func_3308);
			return;
		}
		state() = 0x2b;
		set_frameset(facing(ADDR_Frameset_685fe, ADDR_Frameset_68605));
		state_duration() = 0x1a;
		return;
	case TURN_TO_RUN:
		if (!turned())
			return;
		state() = 0x5b;
		set_frameset(facing(ADDR_Frameset_68574, ADDR_Frameset_6857d));
		return;
	case RUNNING: {
		if (shaken(0x1c))
			return;
		int side = walk(false);
		if (!side)
			return;
		state() = 0x2c;
		set_frameset(side == 2 ? ADDR_Frameset_68605 : ADDR_Frameset_685fe);
		state_duration() = 0x1a;
		return;
	}
	case TURN_TO_THROW:
		if (!turned())
			return;
		state() = 0x5c;
		set_frameset(facing(ADDR_Frameset_685c4, ADDR_Frameset_685d7));
		state_duration() = 0x37;
		return;
	case PAUSE:
		if (shaken(0x1c))
			return;
		if (--state_duration()) {
			routines_.call(ADDR_Func_3308);
			return;
		}
		state() = 0x01;
		return;
	case DEFAULT: { // standing, facing Wario
		set_interaction(OBJ_INTERACTION_01);
		mem_[wCurObjCollBoxRight] = 0x05;
		mem_[wCurObjCollBoxLeft] = 0xfa;
		mem_[wCurObjCollBoxBottom] = 0xff;
		state() = 0x30;
		bool right = static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
		if (right)
			substate() |= OBJSUBFLAG_HDIR;
		else
			substate() &= ~OBJSUBFLAG_HDIR;
		var3() = 0;
		set_frameset(right ? ADDR_Frameset_6856b : ADDR_Frameset_68562);
		return;
	}
	case STATE_41:
		state() = 0x41;
		var3() = 0;
		set_frameset(facing(ADDR_Frameset_68562, ADDR_Frameset_6856b));
		return;
	case SET_WALK:
		set_interaction(OBJ_INTERACTION_01);
		state() = 0x31;
		set_frameset(facing(ADDR_Frameset_68562, ADDR_Frameset_6856b));
		return;
	case WALKING: { // throws at Wario level and close in front; turns if he's behind
		if (shaken(0x1a))
			return;
		uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
		if (dy < 0x20 || dy >= 0xe0) {
			uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
			bool wario_right = dx >= 0xc0;
			if (wario_right || dx < 0x40) {
				if (facing_right() != wario_right) {
					state() = 0x13;
					return;
				}
				state() = 0x5a;
				set_frameset(wario_right ? ADDR_Frameset_68597 : ADDR_Frameset_68586);
				state_duration() = 0x33;
				return;
			}
		}
		if (walk(true))
			state() = 0x13;
		return;
	}
	case BUMP_LEFT_START:
		bump_start(0x32, ADDR_ObjParams_DoughnutUpLeft);
		return;
	case BUMP_RIGHT_START:
		bump_start(0x33, ADDR_ObjParams_DoughnutUpRight);
		return;
	case BUMP_LEFT:
		bumped(false);
		return;
	case BUMP_RIGHT:
		bumped(true);
		return;
	case STUN_LEFT_START:
	case STUN_RIGHT_START:
		state() = handler == STUN_LEFT_START ? 0x46 : 0x47;
		set_frameset(facing(ADDR_Frameset_685b2, ADDR_Frameset_685bb));
		set_interaction(OBJ_INTERACTION_00);
		var2_move(handler == STUN_LEFT_START ? ADDR_Func_32ae : ADDR_Func_32bd);
		return;
	case STUN_LEFT:
		var2_move(ADDR_Func_32ae);
		return;
	case STUN_RIGHT:
		var2_move(ADDR_Func_32bd);
		return;
	case STATE_44:
		dazed(0x44);
		return;
	case STATE_45:
		dazed(0x45);
		return;
	case STATE_0F:
		state() = 0x3f;
		state_duration() = 0x64;
		flags() &= ~OBJFLAG_GRABBED;
		[[fallthrough]];
	case STATE_3F:
		if (shaken(0x1c))
			return;
		if (--state_duration())
			routines_.call(ADDR_Func_32ea);
		else
			state() = 0x10;
		return;
	case STATE_10:
		state() = 0x40;
		var3() = 0;
		state_duration() = 0x07;
		[[fallthrough]];
	case STATE_40:
		if (--state_duration())
			routines_.call(ADDR_Func_32f9);
		else
			state() = 0x00;
		return;
	case TURN_START:
		state() = 0x43;
		set_frameset(facing(ADDR_Frameset_685f0, ADDR_Frameset_685f7));
		state_duration() = 0x1a;
		return;
	case TURNING:
		if (shaken(0x1a))
			return;
		if (turned())
			state() = 0x01;
		return;
	case WOBBLE_LEFT:
		wobble(0x38);
		return;
	case WOBBLE_RIGHT:
		wobble(0x39);
		return;
	case STATE_29:
		knocked(0x59);
		return;
	case STATE_28:
		knocked(0x58);
		return;
	case ATTACKED_LEFT:
		knocked(0x34);
		return;
	case ATTACKED_RIGHT:
		knocked(0x35);
		return;
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
		set_interaction(OBJ_INTERACTION_00);
		return;
	default: // a shared object state
		routines_.call(handler);
		return;
	}
}

void Doughnut::aim()
{
	bool left = static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	set_update(left ? DOUGHNUT_UPDATE_LEFT : DOUGHNUT_UPDATE_RIGHT);
	var3() = 0;
}

void Doughnut::fly(uint16_t page)
{
	mem_[static_cast<uint16_t>(page | OBJ_FLAGS)] |= OBJFLAG_NO_COLLISION;
	moving(ADDR_Data_60510);
}

void Doughnut::up()
{
	set_update(DOUGHNUT_UP_UPDATE);
	mem_[wCurObjAction] = 0x8f;
	flags() |= OBJFLAG_NO_COLLISION;
	moving(ADDR_Data_60540);
}

void Doughnut::moving(uint16_t movement)
{
	uint8_t s = state();
	if (s == 0x18) {
		destroy();
		return;
	}
	if (s == 0x19) { // knocked back at Wario
		aim();
		return;
	}
	SET_BC(gb_, movement);
	routines_.call(ADDR_ApplyObjYMovement);
}

void Doughnut::at_wario(uint16_t move)
{
	routines_.call(move);
	no_actions();
	SET_BC(gb_, ADDR_Data_60040);
	routines_.call(ADDR_ApplyObjYMovement);
}

} // namespace wl3::level

using wl3::level::Doughnuteer;
using wl3::level::Doughnut;

PORT_EX(DoughnuteerFunc, 0, ALL_REGISTERS) { Doughnuteer(gb).start(); }
PORT_LOCAL_EX(DoughnuteerFunc, Init, 0, ALL_REGISTERS) { Doughnuteer(gb).init(); }
PORT_LOCAL_EX(DoughnuteerFunc, Update, 0, ALL_REGISTERS) { Doughnuteer(gb).update(); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41b58, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::THROWING); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41bae, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::TURN_TO_RUN); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41bdc, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::RUNNING); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41c8b, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::TURN_TO_THROW); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41cc1, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::PAUSE); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41cdb, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::DEFAULT); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41d1a, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STATE_41); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41d36, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::SET_WALK); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41d56, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::WALKING); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41e3a, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::BUMP_LEFT_START); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41e53, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::BUMP_LEFT); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41e7c, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::BUMP_RIGHT_START); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41eae, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::BUMP_RIGHT); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41ed7, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STUN_LEFT_START); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41ef4, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STUN_LEFT); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41f02, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STUN_RIGHT_START); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41f1f, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STUN_RIGHT); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41f2d, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STATE_44); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41f35, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STATE_45); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41f5d, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STATE_0F); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41f6c, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STATE_3F); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41f86, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STATE_10); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41f94, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STATE_40); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41fa1, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::TURN_START); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41fbd, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::TURNING); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41fe9, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::WOBBLE_LEFT); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_41ff3, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::WOBBLE_RIGHT); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_42020, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STATE_29); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_4202a, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STATE_28); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_42034, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::ATTACKED_LEFT); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_4203e, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::ATTACKED_RIGHT); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_42064, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::PICKED_UP_LEFT); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_42077, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::PICKED_UP_RIGHT); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_4208a, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STATE_19); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_420a6, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STATE_49); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_420b6, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STATE_18); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_420d2, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STATE_48); }
PORT_LOCAL_EX(DoughnuteerFunc, Func_420e3, 0, ALL_REGISTERS) { Doughnuteer(gb).run(wl3::level::STATE_12); }

PORT_EX(DoughnutFunc_Destroy, 0, ALL_REGISTERS) { Doughnut(gb).destroy(); }
PORT_EX(DoughnutFunc, 0, ALL_REGISTERS) { Doughnut(gb).aim(); }
PORT_LOCAL_EX(DoughnutFunc, Update_Left, 0, ALL_REGISTERS) { Doughnut(gb).at_wario(ADDR_MoveObjectLeft); }
PORT_LOCAL_EX(DoughnutFunc, Update_Right, 0, ALL_REGISTERS) { Doughnut(gb).at_wario(ADDR_MoveObjectRight); }
PORT_EX(DoughnutFunc_Left, 0, ALL_REGISTERS) { Doughnut(gb).fly_after(ADDR_MoveObjectLeft_Fast); }
PORT_EX(DoughnutFunc_Right, 0, ALL_REGISTERS) { Doughnut(gb).fly_after(ADDR_MoveObjectRight_Fast); }
PORT_EX(DoughnutFunc_LeftRight, 0, ALL_REGISTERS) { Doughnut(gb).fly(gb->cpu.h << 8); }
PORT_EX(DoughnutFunc_Up, 0, ALL_REGISTERS) { Doughnut(gb).up(); }
PORT_LOCAL_EX(DoughnutFunc_Up, Update, 0, ALL_REGISTERS) { Doughnut(gb).moving(ADDR_Data_60540); }
