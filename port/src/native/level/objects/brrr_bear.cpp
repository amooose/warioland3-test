// Brrr Bear (brrr_bear.asm): a polar bear walking the floor that, when
// Wario's level and ahead, breathes snowflakes at him (freezing him into
// Snowman Wario). Knocked about like other enemies, through a table of
// 93 state handlers. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_BrrrBearFunc + offset); }

constexpr uint16_t INIT = L(0x0a); // .Init
constexpr uint16_t UPDATE = L(0x29); // .Update
constexpr uint16_t VANISH_TOUCH = L(0xe7); // .Func_480e7
constexpr uint16_t RESET = L(0x11f); // .Func_4811f
constexpr uint16_t STATE_41 = L(0x153); // .Func_48153
constexpr uint16_t BREATHING = L(0x16f); // .Func_4816f
constexpr uint16_t SET_WALK = L(0x1aa); // .Func_481aa
constexpr uint16_t WALKING = L(0x1ca); // .Func_481ca
constexpr uint16_t BUMP_32 = L(0x2b2); // .Func_482b2
constexpr uint16_t BUMP_33 = L(0x2ba); // .Func_482ba
constexpr uint16_t HIT_LEFT = L(0x2e0); // .Func_482e0
constexpr uint16_t HIT_LEFT_MOVE = L(0x2f3); // .Func_482f3
constexpr uint16_t HIT_RIGHT = L(0x301); // .Func_48301
constexpr uint16_t HIT_RIGHT_MOVE = L(0x314); // .Func_48314
constexpr uint16_t STATE_44 = L(0x322); // .Func_48322
constexpr uint16_t STATE_45 = L(0x326); // .Func_48326
constexpr uint16_t DROPPED = L(0x341); // .Func_48341
constexpr uint16_t DAZED = L(0x350); // .Func_48350
constexpr uint16_t GET_UP = L(0x36a); // .Func_4836a
constexpr uint16_t GETTING_UP = L(0x399); // .Func_48399
constexpr uint16_t SET_TURN = L(0x3a4); // .Func_483a4
constexpr uint16_t TURNING = L(0x3c5); // .Func_483c5
constexpr uint16_t STUNNED_38 = L(0x3f5); // .Func_483f5
constexpr uint16_t STUNNED_39 = L(0x3ff); // .Func_483ff
constexpr uint16_t KNOCKED_59 = L(0x420); // .Func_48420
constexpr uint16_t KNOCKED_58 = L(0x42a); // .Func_4842a
constexpr uint16_t ATTACKED_LEFT = L(0x434); // .Func_48434
constexpr uint16_t ATTACKED_RIGHT = L(0x458); // .Func_48458
constexpr uint16_t STATE_36 = L(0x48e); // .Func_4848e
constexpr uint16_t STATE_37 = L(0x4a1); // .Func_484a1
constexpr uint16_t GRAB_49 = L(0x4b4); // .Func_484b4
constexpr uint16_t HELD_49 = L(0x4c3); // .Func_484c3
constexpr uint16_t GRAB_48 = L(0x4d3); // .Func_484d3
constexpr uint16_t HELD_48 = L(0x4e2); // .Func_484e2
constexpr uint16_t STATE_42 = L(0x4f3); // .Func_484f3

constexpr uint16_t FLAKE_GONE = ADDR_SnowflakeFunc + 0x1c; // .Update

// .Update's jumptable, by state
constexpr uint16_t HANDLERS[] = {
	RESET, SET_WALK, BUMP_32, BUMP_33, ATTACKED_LEFT, ATTACKED_RIGHT, STATE_36, STATE_37, // $00
	STUNNED_38, STUNNED_39, VANISH_TOUCH, RESET, RESET, RESET, RESET, DROPPED,
	GET_UP, STATE_41, STATE_42, SET_TURN, STATE_44, STATE_45, HIT_LEFT, HIT_RIGHT, // $10
	GRAB_48, GRAB_49, ADDR_Func_3263, ADDR_Func_3272, ADDR_Func_3281, RESET, RESET, RESET,
	ADDR_Func_3191, ADDR_Func_31af, ADDR_Func_31cd, ADDR_Func_31eb, ADDR_Func_3209, ADDR_Func_3227, ADDR_Func_3245, ADDR_Func_3254, // $20
	KNOCKED_58, KNOCKED_59, RESET, RESET, RESET, BREATHING, RESET, RESET,
	ADDR_ObjState_Fall, WALKING, ADDR_ObjState_BumpLeft, ADDR_ObjState_BumpRight, ADDR_Func_3326, ADDR_Func_3317, ADDR_Func_3335, ADDR_Func_3344, // $30
	ADDR_Func_31fa, ADDR_Func_31dc, RESET, RESET, RESET, RESET, RESET, DAZED,
	GETTING_UP, ADDR_ObjState_StandingFall, ADDR_Func_3371, TURNING, ADDR_Func_32cc, ADDR_Func_32db, HIT_LEFT_MOVE, HIT_RIGHT_MOVE, // $40
	HELD_48, HELD_49, ADDR_Func_3380, ADDR_Func_338f, ADDR_ObjState_FullThrowLeft, ADDR_ObjState_FullThrowRight, ADDR_Func_339e, ADDR_Func_33ad,
	ADDR_Func_31a0, ADDR_Func_31be, ADDR_Func_31dc, ADDR_Func_31fa, ADDR_Func_3218, ADDR_Func_3236, ADDR_Func_32cc, ADDR_Func_32db, // $50
	ADDR_Func_33f8, ADDR_Func_3407, RESET, RESET, RESET,
};
constexpr uint8_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

class BrrrBear : public Object {
public:
	using Object::Object;

	void start() { set_update(INIT); } // BrrrBearFunc
	void init(); // .Init
	void update(); // .Update
	void run(uint16_t handler);
	void snowflake(); // SnowflakeFunc
	void snowflake_gone(); // .Update

private:
	uint16_t facing(uint16_t left, uint16_t right) { return facing_right() ? right : left; }
	bool wario_to_right() const;
	bool shaken();
	void walk(); // .asm_48222
	void hit_move(uint16_t routine);
	void stunned(uint8_t s);
	void knocked(uint8_t s, uint8_t var2_); // .asm_4847a
	bool shrugs_off();
	void grab(uint8_t s);
	void set_state_then(uint8_t s, uint8_t var2_, uint16_t routine);
};

} // namespace

bool BrrrBear::wario_to_right() const
{
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	return static_cast<uint8_t>(screen_x() + 0x2a) < wario;
}

void BrrrBear::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 0x08;
	mem_[wCurObjCollBoxLeft] = 0xf7;
	mem_[wCurObjCollBoxBottom] = 0xf9;
	run(RESET);
}

void BrrrBear::update()
{
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

bool BrrrBear::shaken()
{
	if (mem_[wGroundShakeCounter] < 0x10)
		return false;
	state() = static_cast<uint8_t>(0x08 | (mem_[rDIV] & 1));
	return true;
}

// slowly along the floor, turning at walls and edges
void BrrrBear::walk()
{
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
	if (right ? in_block + 7 >= 0x10 : in_block < 8) {
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

void BrrrBear::hit_move(uint16_t routine)
{
	if (var2())
		routines_.call(routine);
	else
		state() = 0x0f;
}

void BrrrBear::stunned(uint8_t s)
{
	mem_[wCurObjAction] = 0x8f;
	state() = s;
	set_frameset(ADDR_Frameset_6915b);
	state_duration() = 0x04;
	var2() = 0x02;
	var3() = 0;
	set_interaction(OBJ_INTERACTION_00);
}

void BrrrBear::knocked(uint8_t s, uint8_t var2_)
{
	state() = s;
	substate() &= 0xf0;
	var3() = 0;
	var2() = var2_;
	set_frameset(ADDR_Frameset_69164);
	no_actions();
}

bool BrrrBear::shrugs_off()
{
	return !mem_[wTransformation] && mem_[wEffectivePowerUp] < 0x05 && (interaction() & 0x7f);
}

void BrrrBear::grab(uint8_t s)
{
	state() = s;
	set_frameset(ADDR_Frameset_6915b);
	flags() |= 1 << OBJFLAG_GRABBED_F;
}

void BrrrBear::set_state_then(uint8_t s, uint8_t var2_, uint16_t routine)
{
	state() = s;
	var3() = 0x41;
	var2() = var2_;
	var1() = 0x01;
	routines_.call(routine);
}

void BrrrBear::run(uint16_t handler)
{
	switch (handler) {
	case VANISH_TOUCH: // pounded: wobbles, unless Wario's strong enough
		if (!mem_[wCurObjAction] || mem_[wTransformation] || mem_[wEffectivePowerUp] >= 0x06
			|| !(interaction() & 0x7f)) {
			routines_.call(ADDR_VanishObject2);
			return;
		}
		play_sfx(SFX_014);
		run(wario_to_right() ? STUNNED_39 : STUNNED_38);
		return;
	case RESET: { // facing Wario
		set_interaction(OBJ_INTERACTION_01);
		state() = 0x30;
		bool right = wario_to_right();
		if (right)
			substate() |= OBJSUBFLAG_HDIR;
		else
			substate() &= ~OBJSUBFLAG_HDIR;
		var3() = 0;
		set_frameset(right ? ADDR_Frameset_69126 : ADDR_Frameset_69115);
		return;
	}
	case STATE_41:
		state() = 0x41;
		var3() = 0;
		set_frameset(facing(ADDR_Frameset_69115, ADDR_Frameset_69126));
		return;
	case BREATHING: // a snowflake at $22 frames left
		if (shaken())
			return;
		if (!--state_duration()) {
			state() = 0x13;
			return;
		}
		if (state_duration() != 0x22)
			return;
		play_sfx(SFX_07E);
		SET_BC(gb_, facing(ADDR_ObjParams_SnowflakeLeft, ADDR_ObjParams_SnowflakeRight));
		routines_.call(ADDR_CreateObjectAtRelativePos);
		return;
	case SET_WALK:
		set_interaction(OBJ_INTERACTION_01);
		state() = 0x31;
		set_frameset(facing(ADDR_Frameset_69115, ADDR_Frameset_69126));
		return;
	case WALKING: // breathes when Wario's level and ahead; turns if he's behind
		if (shaken())
			return;
		{
			uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
			if (dy < 0x18 || dy >= 0xe8) {
				uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
				bool near_right = dx >= 0xb8, near_left = dx < 0x48;
				if (near_right || near_left) {
					if (facing_right() != near_right) {
						state() = 0x13;
						return;
					}
					state() = 0x2d;
					set_frameset(near_right ? ADDR_Frameset_6914c : ADDR_Frameset_6913d);
					state_duration() = 0x39;
					return;
				}
			}
		}
		walk();
		return;
	case BUMP_32:
	case BUMP_33:
		state() = handler == BUMP_32 ? 0x32 : 0x33;
		set_frameset(facing(ADDR_Frameset_69137, ADDR_Frameset_6913a));
		state_duration() = 0x0c;
		var2() = 0x02;
		set_interaction(OBJ_INTERACTION_01);
		return;
	case HIT_LEFT:
		state() = 0x46;
		set_frameset(ADDR_Frameset_6915b);
		set_interaction(OBJ_INTERACTION_00);
		[[fallthrough]];
	case HIT_LEFT_MOVE:
		hit_move(ADDR_Func_32ae);
		return;
	case HIT_RIGHT:
		state() = 0x47;
		set_frameset(ADDR_Frameset_6915b);
		set_interaction(OBJ_INTERACTION_00);
		[[fallthrough]];
	case HIT_RIGHT_MOVE:
		hit_move(ADDR_Func_32bd);
		return;
	case STATE_44:
	case STATE_45:
		state() = handler == STATE_44 ? 0x44 : 0x45;
		set_frameset(ADDR_Frameset_6915b);
		var2() = 0x02;
		var3() = 0;
		set_interaction(OBJ_INTERACTION_00);
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
	case GET_UP: { // facing Wario
		state() = 0x40;
		bool right = wario_to_right();
		if (right)
			substate() |= OBJSUBFLAG_HDIR;
		else
			substate() &= ~OBJSUBFLAG_HDIR;
		set_frameset(right ? ADDR_Frameset_6918e : ADDR_Frameset_69183);
		state_duration() = 0x28;
		set_interaction(OBJ_INTERACTION_01);
		return;
	}
	case GETTING_UP:
		if (!--state_duration())
			state() = 0x01;
		return;
	case SET_TURN:
		state() = 0x43;
		set_frameset(facing(ADDR_Frameset_69175, ADDR_Frameset_6917c));
		state_duration() = 0x16;
		set_interaction(OBJ_INTERACTION_01);
		return;
	case TURNING:
		if (shaken())
			return;
		if (state_duration() == 0x06) {
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
	case KNOCKED_59: knocked(0x59, 0x02); return;
	case KNOCKED_58: knocked(0x58, 0x02); return;
	case ATTACKED_LEFT:
		if (shrugs_off())
			state() = 0x08;
		else
			knocked(0x34, 0x01);
		return;
	case ATTACKED_RIGHT:
		if (shrugs_off())
			state() = 0x09;
		else
			knocked(0x35, 0x01);
		return;
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
		substate() ^= 0x80;
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

// flies straight until it hits something
void BrrrBear::snowflake()
{
	flags() |= OBJFLAG_NO_COLLISION;
	if (!state())
		return;
	set_frameset(ADDR_Frameset_6916e);
	state_duration() = 0x0c;
	set_update(FLAKE_GONE);
}

void BrrrBear::snowflake_gone()
{
	no_actions();
	if (!--state_duration())
		flags() = 0;
}

} // namespace wl3::level

using BrrrBear = wl3::level::BrrrBear;

PORT_EX(BrrrBearFunc, 0, ALL_REGISTERS) { BrrrBear(gb).start(); }
PORT_LOCAL_EX(BrrrBearFunc, Init, 0, ALL_REGISTERS) { BrrrBear(gb).init(); }
PORT_LOCAL_EX(BrrrBearFunc, Update, 0, ALL_REGISTERS) { BrrrBear(gb).update(); }
PORT_LOCAL_EX(BrrrBearFunc, Func_480e7, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::VANISH_TOUCH); }
PORT_LOCAL_EX(BrrrBearFunc, Func_4811f, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::RESET); }
PORT_LOCAL_EX(BrrrBearFunc, Func_48153, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::STATE_41); }
PORT_LOCAL_EX(BrrrBearFunc, Func_4816f, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::BREATHING); }
PORT_LOCAL_EX(BrrrBearFunc, Func_481aa, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::SET_WALK); }
PORT_LOCAL_EX(BrrrBearFunc, Func_481ca, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::WALKING); }
PORT_LOCAL_EX(BrrrBearFunc, Func_482b2, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::BUMP_32); }
PORT_LOCAL_EX(BrrrBearFunc, Func_482ba, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::BUMP_33); }
PORT_LOCAL_EX(BrrrBearFunc, Func_482e0, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::HIT_LEFT); }
PORT_LOCAL_EX(BrrrBearFunc, Func_482f3, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::HIT_LEFT_MOVE); }
PORT_LOCAL_EX(BrrrBearFunc, Func_48301, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::HIT_RIGHT); }
PORT_LOCAL_EX(BrrrBearFunc, Func_48314, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::HIT_RIGHT_MOVE); }
PORT_LOCAL_EX(BrrrBearFunc, Func_48322, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::STATE_44); }
PORT_LOCAL_EX(BrrrBearFunc, Func_48326, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::STATE_45); }
PORT_LOCAL_EX(BrrrBearFunc, Func_48341, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::DROPPED); }
PORT_LOCAL_EX(BrrrBearFunc, Func_48350, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::DAZED); }
PORT_LOCAL_EX(BrrrBearFunc, Func_4836a, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::GET_UP); }
PORT_LOCAL_EX(BrrrBearFunc, Func_48399, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::GETTING_UP); }
PORT_LOCAL_EX(BrrrBearFunc, Func_483a4, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::SET_TURN); }
PORT_LOCAL_EX(BrrrBearFunc, Func_483c5, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::TURNING); }
PORT_LOCAL_EX(BrrrBearFunc, Func_483f5, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::STUNNED_38); }
PORT_LOCAL_EX(BrrrBearFunc, Func_483ff, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::STUNNED_39); }
PORT_LOCAL_EX(BrrrBearFunc, Func_48420, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::KNOCKED_59); }
PORT_LOCAL_EX(BrrrBearFunc, Func_4842a, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::KNOCKED_58); }
PORT_LOCAL_EX(BrrrBearFunc, Func_48434, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::ATTACKED_LEFT); }
PORT_LOCAL_EX(BrrrBearFunc, Func_48458, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::ATTACKED_RIGHT); }
PORT_LOCAL_EX(BrrrBearFunc, Func_4848e, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::STATE_36); }
PORT_LOCAL_EX(BrrrBearFunc, Func_484a1, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::STATE_37); }
PORT_LOCAL_EX(BrrrBearFunc, Func_484b4, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::GRAB_49); }
PORT_LOCAL_EX(BrrrBearFunc, Func_484c3, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::HELD_49); }
PORT_LOCAL_EX(BrrrBearFunc, Func_484d3, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::GRAB_48); }
PORT_LOCAL_EX(BrrrBearFunc, Func_484e2, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::HELD_48); }
PORT_LOCAL_EX(BrrrBearFunc, Func_484f3, 0, ALL_REGISTERS) { BrrrBear(gb).run(wl3::level::STATE_42); }
PORT_EX(SnowflakeFunc_Left, 0, ALL_REGISTERS) { gb_call(gb, ADDR_MoveObjectLeft_Fast); BrrrBear(gb).snowflake(); }
PORT_EX(SnowflakeFunc_Right, 0, ALL_REGISTERS) { gb_call(gb, ADDR_MoveObjectRight_Fast); BrrrBear(gb).snowflake(); }
PORT_EX(SnowflakeFunc, 0, ALL_REGISTERS) { BrrrBear(gb).snowflake(); }
PORT_LOCAL_EX(SnowflakeFunc, Update, 0, ALL_REGISTERS) { BrrrBear(gb).snowflake_gone(); }
