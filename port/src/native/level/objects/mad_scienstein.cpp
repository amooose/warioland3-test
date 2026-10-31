// Mad Scienstein (mad_scienstein.asm): walks the floor and, when Wario's
// level and close in front, throws an invisibility potion, then runs off
// and turns back to throw again; bumping him drops potions too. Knocked
// about like other enemies, through a table of 93 state handlers. And the
// potions he throws (and drops): they break where they land.
// See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_MadSciensteinFunc + offset); }

constexpr uint16_t INIT = L(0x0a); // .Init
constexpr uint16_t UPDATE = L(0x1e); // .Update
constexpr uint16_t HIT = L(0xdc); // .Func_44e18
constexpr uint16_t THROWING = L(0x114); // .Func_44e50
constexpr uint16_t TURN_TO_RUN = L(0x16e); // .Func_44eaa
constexpr uint16_t RUNNING = L(0x19e); // .Func_44eda
constexpr uint16_t TURN_TO_THROW = L(0x24d); // .Func_44f89
constexpr uint16_t PAUSE = L(0x283); // .Func_44fbf
constexpr uint16_t DEFAULT = L(0x29d); // .Func_44fd9
constexpr uint16_t STATE_41 = L(0x2dc); // .Func_45018
constexpr uint16_t SET_WALK = L(0x2f8); // .Func_45034
constexpr uint16_t WALKING = L(0x318); // .Func_45054
constexpr uint16_t BUMP_LEFT_START = L(0x404); // .Func_45140
constexpr uint16_t BUMP_LEFT = L(0x421); // .Func_4515d
constexpr uint16_t BUMP_RIGHT_START = L(0x44a); // .Func_45186
constexpr uint16_t BUMP_RIGHT = L(0x480); // .Func_451bc
constexpr uint16_t STUN_LEFT_START = L(0x4a9); // .Func_451e5
constexpr uint16_t STUN_LEFT = L(0x4c6); // .Func_45202
constexpr uint16_t STUN_RIGHT_START = L(0x4d4); // .Func_45210
constexpr uint16_t STUN_RIGHT = L(0x4f1); // .Func_4522d
constexpr uint16_t STATE_44 = L(0x4ff); // .Func_4523b
constexpr uint16_t STATE_45 = L(0x507); // .Func_45243
constexpr uint16_t STATE_0F = L(0x52f); // .Func_4526b
constexpr uint16_t STATE_3F = L(0x53e); // .Func_4527a
constexpr uint16_t STATE_10 = L(0x558); // .Func_45294
constexpr uint16_t STATE_40 = L(0x566); // .Func_452a2
constexpr uint16_t TURN_START = L(0x573); // .Func_452af
constexpr uint16_t TURNING = L(0x58f); // .Func_452cb
constexpr uint16_t WOBBLE_LEFT = L(0x5bb); // .Func_452f7
constexpr uint16_t WOBBLE_RIGHT = L(0x5c5); // .Func_45301
constexpr uint16_t STATE_29 = L(0x5f2); // .Func_4532e
constexpr uint16_t STATE_28 = L(0x5fc); // .Func_45338
constexpr uint16_t ATTACKED_LEFT = L(0x606); // .Func_45342
constexpr uint16_t ATTACKED_RIGHT = L(0x62a); // .Func_45366
constexpr uint16_t PICKED_UP_LEFT = L(0x66a); // .Func_453a6
constexpr uint16_t PICKED_UP_RIGHT = L(0x67d); // .Func_453b9
constexpr uint16_t STATE_19 = L(0x690); // .Func_453cc
constexpr uint16_t STATE_49 = L(0x6ac); // .Func_453e8
constexpr uint16_t STATE_18 = L(0x6bc); // .Func_453f8
constexpr uint16_t STATE_48 = L(0x6d8); // .Func_45414
constexpr uint16_t STATE_12 = L(0x6e9); // .Func_45425

// .Update's jumptable, by state
constexpr uint16_t HANDLERS[] = {
	DEFAULT, SET_WALK, BUMP_LEFT_START, BUMP_RIGHT_START, ATTACKED_LEFT, ATTACKED_RIGHT, PICKED_UP_LEFT, PICKED_UP_RIGHT, // $00
	WOBBLE_LEFT, WOBBLE_RIGHT, HIT, DEFAULT, DEFAULT, DEFAULT, DEFAULT, STATE_0F,
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

constexpr uint16_t POTION_DROP_UPDATE = ADDR_InvisibilityPotionFunc_Drop + 0x11; // .Update

class MadScienstein : public Object {
public:
	using Object::Object;

	void start() { set_update(INIT); } // MadSciensteinFunc
	void init(); // .Init
	void update(); // .Update
	void run(uint16_t handler);

private:
	uint16_t facing(uint16_t left, uint16_t right) { return facing_right() ? right : left; }
	bool shaken(uint8_t s); // to state s, with the ground shaking
	bool turned(uint8_t turn); // turns at `turn`; true when the duration ran out
	int walk(uint16_t move_left, uint16_t move_right); // 0: moved (or fell), else blocked 1 left, 2 right
	void bump_start(uint8_t s, uint16_t params);
	void bumped(uint16_t bump, uint16_t left, uint8_t left_state, uint16_t right, uint8_t right_state);
	void var2_move(uint16_t routine);
	void dazed(uint8_t s);
	void wobble(uint8_t s);
	void stunned(uint8_t s, uint8_t var2_);
	void attacked(uint8_t attack, uint8_t s);
	void grab(uint8_t s);
};

// an invisibility potion
class Potion : public Object {
public:
	using Object::Object;

	void fly(uint16_t page); // InvisibilityPotionFunc
	void fly_after(uint16_t move) { routines_.call(move); fly(gb_->cpu.h << 8); } // InvisibilityPotionFunc_Left / _Right
	void destroy() { routines_.farcall(BANK_VanishObject_NoSFX, ADDR_VanishObject_NoSFX); } // DestroyInvisibilityPotion
	void land(); // Func_45453
	void drop(); // InvisibilityPotionFunc_Drop
	void dropping(); // InvisibilityPotionFunc_Drop.Update
};

} // namespace

void MadScienstein::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	run(DEFAULT);
}

void MadScienstein::update()
{
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

bool MadScienstein::shaken(uint8_t s)
{
	if (mem_[wGroundShakeCounter] < 0x10)
		return false;
	state() = s;
	return true;
}

bool MadScienstein::turned(uint8_t turn)
{
	if (state_duration() == turn) {
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

int MadScienstein::walk(uint16_t move_left, uint16_t move_right)
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
	if (right ? in_block + 6 >= 0x10 : in_block < 7) {
		routines_.call(right ? ADDR_ProcessBlockOnRight : ADDR_ProcessBlockOnLeft);
		if (!(gb_->cpu.a & 0x0f))
			return right ? 2 : 1;
	}
	uint16_t y_pos = static_cast<uint16_t>(y() - 4);
	uint16_t x_pos = static_cast<uint16_t>(right ? x() + 6 : x() - 7);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a)
		return right ? 2 : 1;
	routines_.call(right ? move_right : move_left);
	return 0;
}

// a potion dropped on some frames
void MadScienstein::bump_start(uint8_t s, uint16_t params)
{
	uint8_t f = frame();
	if (f < 0x19 && (f >= 0x14 || f < 0x06)) {
		SET_BC(gb_, params);
		routines_.call(ADDR_CreateObjectAtRelativePos);
	}
	state() = s;
	set_frameset(facing(ADDR_Frameset_68bc7, ADDR_Frameset_68bcc));
	state_duration() = 0x0c;
	var2() = 0x02;
}

void MadScienstein::bumped(uint16_t bump, uint16_t left, uint8_t left_state, uint16_t right, uint8_t right_state)
{
	if (var2()) {
		routines_.call(bump);
		return;
	}
	if (--state_duration())
		return;
	bool r = facing_right();
	uint8_t s = r ? right_state : left_state;
	state() = s;
	set_frameset(r ? right : left);
	if (s == 0x2b)
		state_duration() = 28;
}

void MadScienstein::var2_move(uint16_t routine)
{
	if (var2())
		routines_.call(routine);
	else
		state() = 0x0f;
}

void MadScienstein::dazed(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_68be3, ADDR_Frameset_68be8));
	var2() = 0x02;
	var3() = 0;
	set_interaction(OBJ_INTERACTION_00);
}

void MadScienstein::wobble(uint8_t s)
{
	mem_[wCurObjAction] = 0x8f;
	state() = s;
	set_frameset(facing(ADDR_Frameset_68be3, ADDR_Frameset_68be8));
	state_duration() = 0x04;
	var2() = 0x02;
	var3() = 0;
	set_interaction(OBJ_INTERACTION_00);
}

void MadScienstein::stunned(uint8_t s, uint8_t var2_)
{
	state() = s;
	no_actions();
	uint8_t sub = substate() & 0xf0;
	substate() = sub;
	var3() = 0;
	var2() = var2_;
	set_frameset((sub & 0x80) ? ADDR_Frameset_68c14 : ADDR_Frameset_68c11);
}

// attacked, unless Wario can only bump him
void MadScienstein::attacked(uint8_t attack, uint8_t s)
{
	if (!mem_[wTransformation] && mem_[wEffectivePowerUp] < GARLIC && (interaction() & INTERACTION_MASK)) {
		state() = attack;
		return;
	}
	stunned(s, 0x01);
}

void MadScienstein::grab(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_68be3, ADDR_Frameset_68be8));
	flags() |= OBJFLAG_GRABBED;
}

void MadScienstein::run(uint16_t handler)
{
	switch (handler) {
	case HIT: // vanishes, unless Wario can only bump him
		if (!mem_[wCurObjAction] || mem_[wTransformation] || mem_[wEffectivePowerUp] >= SUPER_JUMP_SLAM_OVERALLS
			|| !(interaction() & INTERACTION_MASK)) {
			routines_.call(ADDR_VanishObject2);
			return;
		}
		play_sfx(SFX_014);
		run(static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a) ? WOBBLE_RIGHT : WOBBLE_LEFT);
		return;
	case THROWING:
		if (shaken(0x1c))
			return;
		if (state_duration() == 0x18) { // the potion away
			--state_duration();
			if (flags() & (1 << OBJFLAG_ON_SCREEN_F))
				play_sfx(SFX_067);
			state() = 0x5a;
			SET_BC(gb_, facing_right() ? ADDR_ObjParams_InvisibilityPotionRight : ADDR_ObjParams_InvisibilityPotionLeft);
			routines_.call(ADDR_CreateObjectAtRelativePos);
			return;
		}
		if (--state_duration()) {
			routines_.call(ADDR_Func_3308);
			return;
		}
		state() = 0x2b;
		set_frameset(facing(ADDR_Frameset_68c25, ADDR_Frameset_68c2c));
		state_duration() = 28;
		return;
	case TURN_TO_RUN:
		if (!turned(0x08))
			return;
		state() = 0x5b;
		set_frameset(facing(ADDR_Frameset_68bd1, ADDR_Frameset_68bda));
		return;
	case RUNNING: {
		if (shaken(0x1c))
			return;
		int side = walk(ADDR_MoveObjectLeft, ADDR_MoveObjectRight);
		if (!side)
			return;
		state() = 0x2c;
		set_frameset(side == 2 ? ADDR_Frameset_68c2c : ADDR_Frameset_68c25);
		state_duration() = 28;
		return;
	}
	case TURN_TO_THROW:
		if (!turned(0x08))
			return;
		state() = 0x5c;
		set_frameset(facing(ADDR_Frameset_68bff, ADDR_Frameset_68c08));
		state_duration() = 63;
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
		mem_[wCurObjCollBoxRight] = 0x06;
		mem_[wCurObjCollBoxLeft] = 0xf9;
		mem_[wCurObjCollBoxBottom] = 0xff;
		state() = 0x30;
		bool right = static_cast<uint8_t>(screen_x() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
		if (right)
			substate() |= OBJSUBFLAG_HDIR;
		else
			substate() &= ~OBJSUBFLAG_HDIR;
		var3() = 0;
		set_frameset(right ? ADDR_Frameset_68bbe : ADDR_Frameset_68bb5);
		return;
	}
	case STATE_41:
		state() = 0x41;
		var3() = 0;
		set_frameset(facing(ADDR_Frameset_68bb5, ADDR_Frameset_68bbe));
		return;
	case SET_WALK:
		set_interaction(OBJ_INTERACTION_01);
		state() = 0x31;
		set_frameset(facing(ADDR_Frameset_68bb5, ADDR_Frameset_68bbe));
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
				set_frameset(wario_right ? ADDR_Frameset_68bf6 : ADDR_Frameset_68bed);
				state_duration() = 63;
				return;
			}
		}
		if (walk(ADDR_MoveObjectLeft_Slow, ADDR_MoveObjectRight_Slow))
			state() = 0x13;
		return;
	}
	case BUMP_LEFT_START:
		bump_start(0x32, ADDR_ObjParams_InvisibilityPotionDropLeft);
		return;
	case BUMP_RIGHT_START:
		bump_start(0x33, ADDR_ObjParams_InvisibilityPotionDropRight);
		return;
	case BUMP_LEFT:
		bumped(ADDR_ObjState_BumpLeft, ADDR_Frameset_68c25, 0x2b, ADDR_Frameset_68bda, 0x5b);
		return;
	case BUMP_RIGHT:
		bumped(ADDR_ObjState_BumpRight, ADDR_Frameset_68bd1, 0x5b, ADDR_Frameset_68c2c, 0x2b);
		return;
	case STUN_LEFT_START:
	case STUN_RIGHT_START:
		state() = handler == STUN_LEFT_START ? 0x46 : 0x47;
		set_frameset(facing(ADDR_Frameset_68be3, ADDR_Frameset_68be8));
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
		set_frameset(facing(ADDR_Frameset_68c17, ADDR_Frameset_68c1e));
		state_duration() = 0x1c;
		return;
	case TURNING:
		if (shaken(0x1a))
			return;
		if (turned(0x08))
			state() = 0x01;
		return;
	case WOBBLE_LEFT:
		wobble(0x38);
		return;
	case WOBBLE_RIGHT:
		wobble(0x39);
		return;
	case STATE_29:
		stunned(0x59, 0x02);
		return;
	case STATE_28:
		stunned(0x58, 0x02);
		return;
	case ATTACKED_LEFT:
		attacked(0x08, 0x34);
		return;
	case ATTACKED_RIGHT:
		attacked(0x09, 0x35);
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
		substate() ^= OBJSUBFLAG_HDIR;
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
		substate() ^= OBJSUBFLAG_HDIR;
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

void Potion::fly(uint16_t page)
{
	mem_[static_cast<uint16_t>(page | OBJ_FLAGS)] |= OBJFLAG_NO_COLLISION;
	if (state() == 0x18) {
		destroy();
		return;
	}
	SET_BC(gb_, ADDR_Data_60510);
	routines_.call(ADDR_ApplyObjYMovement);
}

void Potion::land()
{
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a)
		return;
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	destroy();
}

void Potion::drop()
{
	set_update(POTION_DROP_UPDATE);
	mem_[wCurObjAction] = 0x8f;
	flags() |= OBJFLAG_NO_COLLISION;
	dropping();
}

void Potion::dropping()
{
	if (state() == 0x18) {
		destroy();
		return;
	}
	SET_BC(gb_, ADDR_Data_60540);
	routines_.call(ADDR_ApplyObjYMovement);
	land();
}

} // namespace wl3::level

using wl3::level::MadScienstein;
using wl3::level::Potion;

PORT_EX(MadSciensteinFunc, 0, ALL_REGISTERS) { MadScienstein(gb).start(); }
PORT_LOCAL_EX(MadSciensteinFunc, Init, 0, ALL_REGISTERS) { MadScienstein(gb).init(); }
PORT_LOCAL_EX(MadSciensteinFunc, Update, 0, ALL_REGISTERS) { MadScienstein(gb).update(); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_44e18, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::HIT); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_44e50, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::THROWING); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_44eaa, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::TURN_TO_RUN); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_44eda, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::RUNNING); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_44f89, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::TURN_TO_THROW); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_44fbf, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::PAUSE); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_44fd9, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::DEFAULT); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_45018, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STATE_41); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_45034, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::SET_WALK); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_45054, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::WALKING); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_45140, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::BUMP_LEFT_START); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_4515d, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::BUMP_LEFT); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_45186, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::BUMP_RIGHT_START); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_451bc, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::BUMP_RIGHT); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_451e5, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STUN_LEFT_START); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_45202, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STUN_LEFT); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_45210, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STUN_RIGHT_START); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_4522d, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STUN_RIGHT); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_4523b, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STATE_44); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_45243, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STATE_45); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_4526b, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STATE_0F); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_4527a, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STATE_3F); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_45294, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STATE_10); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_452a2, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STATE_40); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_452af, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::TURN_START); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_452cb, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::TURNING); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_452f7, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::WOBBLE_LEFT); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_45301, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::WOBBLE_RIGHT); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_4532e, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STATE_29); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_45338, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STATE_28); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_45342, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::ATTACKED_LEFT); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_45366, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::ATTACKED_RIGHT); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_453a6, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::PICKED_UP_LEFT); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_453b9, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::PICKED_UP_RIGHT); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_453cc, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STATE_19); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_453e8, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STATE_49); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_453f8, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STATE_18); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_45414, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STATE_48); }
PORT_LOCAL_EX(MadSciensteinFunc, Func_45425, 0, ALL_REGISTERS) { MadScienstein(gb).run(wl3::level::STATE_12); }

PORT_EX(InvisibilityPotionFunc_Left, 0, ALL_REGISTERS) { Potion(gb).fly_after(ADDR_MoveObjectLeft_Fast); }
PORT_EX(InvisibilityPotionFunc_Right, 0, ALL_REGISTERS) { Potion(gb).fly_after(ADDR_MoveObjectRight_Fast); }
PORT_EX(InvisibilityPotionFunc, 0, ALL_REGISTERS) { Potion(gb).fly(gb->cpu.h << 8); }
PORT_EX(DestroyInvisibilityPotion, 0, ALL_REGISTERS) { Potion(gb).destroy(); }
PORT_EX(Func_45453, 0, ALL_REGISTERS) { Potion(gb).land(); }
PORT_EX(InvisibilityPotionFunc_Drop, 0, ALL_REGISTERS) { Potion(gb).drop(); }
PORT_LOCAL_EX(InvisibilityPotionFunc_Drop, Update, 0, ALL_REGISTERS) { Potion(gb).dropping(); }
