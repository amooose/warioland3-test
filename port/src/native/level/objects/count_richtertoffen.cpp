// Count Richtertoffen (count_richtertoffen.asm): a vampire that walks,
// then hops along, landing with a ground shake; jumping on him flattens
// Wario. He's knocked about like other enemies, through a table of 93
// state handlers. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_CountRichtertoffenFunc + offset); }

constexpr uint16_t UPDATE = L(0x1f); // .Update
constexpr uint16_t VANISH_TOUCH = L(0xdd); // .VanishTouch
constexpr uint16_t STATE_0B = L(0x115); // .State0b
constexpr uint16_t STATE_0C = L(0x11d); // .State0c
constexpr uint16_t FLATTEN = L(0x125); // .Flatten
constexpr uint16_t STATE_5A = L(0x142); // .State5a
constexpr uint16_t RESET = L(0x16c); // .Default
constexpr uint16_t STANDING_FALL_START = L(0x184); // .StandingFallStart
constexpr uint16_t LAND = L(0x194); // .Land
constexpr uint16_t WALK = L(0x1ac); // .Walk
constexpr uint16_t BUMP = L(0x218); // .Bump
constexpr uint16_t JUMP_RISE = L(0x22c); // .JumpRise
constexpr uint16_t JUMP_FALL = L(0x2bb); // .JumpFall
constexpr uint16_t FALL_BOUNCE = L(0x351); // .FallBounce
constexpr uint16_t STUN_LEFT_START = L(0x375); // .StunLeftStart
constexpr uint16_t STUN_LEFT = L(0x389); // .StunLeft
constexpr uint16_t STUN_RIGHT_START = L(0x397); // .StunRightStart
constexpr uint16_t STUN_RIGHT = L(0x3ab); // .StunRight
constexpr uint16_t STATE_14 = L(0x3b9); // .State14
constexpr uint16_t STATE_15 = L(0x3c1); // .State15
constexpr uint16_t STATE_0F = L(0x3dd); // .State0f
constexpr uint16_t STATE_3F = L(0x3ec); // .State3f
constexpr uint16_t STATE_10 = L(0x3f9); // .State10
constexpr uint16_t STATE_40 = L(0x407); // .State40
constexpr uint16_t TURN_AROUND_START = L(0x414); // .TurnAroundStart
constexpr uint16_t TURN_AROUND = L(0x424); // .TurnAround
constexpr uint16_t WOBBLE_LEFT = L(0x431); // .WobbleLeftStart
constexpr uint16_t WOBBLE_RIGHT = L(0x43b); // .WobbleRightStart
constexpr uint16_t STATE_29 = L(0x45c); // .State29
constexpr uint16_t STATE_28 = L(0x466); // .State28
constexpr uint16_t ATTACKED_LEFT = L(0x470); // .AttackedLeftStart
constexpr uint16_t ATTACKED_RIGHT = L(0x4a5); // .AttackedRightStart
constexpr uint16_t PICKED_UP_LEFT = L(0x4c9); // .PickedUpLeftStart
constexpr uint16_t PICKED_UP_RIGHT = L(0x4dc); // .PickedUpRightStart
constexpr uint16_t STATE_19 = L(0x4ef); // .State19
constexpr uint16_t STATE_49 = L(0x4ff); // .State49
constexpr uint16_t STATE_18 = L(0x50f); // .State18
constexpr uint16_t STATE_48 = L(0x51f); // .State48
constexpr uint16_t STATE_12 = L(0x530); // .State12

// .Update's jumptable, by state
constexpr uint16_t HANDLERS[] = {
	RESET, LAND, BUMP, BUMP, ATTACKED_LEFT, ATTACKED_RIGHT, PICKED_UP_LEFT, PICKED_UP_RIGHT, // $00
	WOBBLE_LEFT, WOBBLE_RIGHT, VANISH_TOUCH, STATE_0B, STATE_0C, RESET, RESET, STATE_0F,
	STATE_10, STANDING_FALL_START, STATE_12, TURN_AROUND_START, STATE_14, STATE_15, STUN_LEFT_START, STUN_RIGHT_START, // $10
	STATE_18, STATE_19, ADDR_Func_3263, ADDR_Func_3272, ADDR_Func_3281, RESET, RESET, RESET,
	ADDR_Func_3191, ADDR_Func_31af, ADDR_Func_31cd, ADDR_Func_31eb, ADDR_Func_3209, ADDR_Func_3227, ADDR_Func_3245, ADDR_Func_3254, // $20
	STATE_28, STATE_29, FLATTEN, RESET, RESET, JUMP_RISE, JUMP_FALL, FALL_BOUNCE,
	ADDR_ObjState_Fall, WALK, ADDR_ObjState_BumpLeft, ADDR_ObjState_BumpRight, ADDR_Func_3326, ADDR_Func_3317, ADDR_Func_3335, ADDR_Func_3344, // $30
	ADDR_Func_31fa, ADDR_Func_31dc, RESET, RESET, RESET, RESET, RESET, STATE_3F,
	STATE_40, ADDR_ObjState_StandingFall, ADDR_Func_3371, TURN_AROUND, ADDR_Func_32cc, ADDR_Func_32db, STUN_LEFT, STUN_RIGHT, // $40
	STATE_48, STATE_49, ADDR_Func_3380, ADDR_Func_338f, ADDR_ObjState_FullThrowLeft, ADDR_ObjState_FullThrowRight, ADDR_Func_339e, ADDR_Func_33ad,
	ADDR_Func_31a0, ADDR_Func_31be, ADDR_Func_31dc, ADDR_Func_31fa, ADDR_Func_3218, ADDR_Func_3236, ADDR_Func_32cc, ADDR_Func_32db, // $50
	ADDR_Func_33f8, ADDR_Func_3407, STATE_5A, RESET, RESET,
};
constexpr uint8_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

class Count : public Object {
public:
	using Object::Object;

	void init(); // CountRichtertoffenFunc
	void update(); // .Update
	void run(uint16_t handler);

private:
	void jump(bool right); // .asm_41006: a hop the given way
	void start_jump(); // .asm_40fe9
	void jump_top(); // .asm_41044
	void hop_along(); // the hop's sideways step, turning at a wall
	void wario_above(); // his position 8 pixels above Wario
	void stun(uint16_t routine);
	void knocked(uint8_t s, uint8_t var2_); // .asm_412a4
	bool shrugs_off(); // Wario's hit can't knock him
	void wobble(uint8_t s);
	void picked_up(uint8_t s, uint8_t var2_, uint16_t routine);
	void grab(uint8_t s);
};

} // namespace

void Count::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	set_update(UPDATE);
	mem_[wCurObjCollBoxRight] = 7;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-8);
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-4);
	run(RESET);
}

void Count::update()
{
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

void Count::jump(bool right)
{
	if (right) {
		substate() |= OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_683c4);
	} else {
		substate() &= ~OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_683c1);
	}
	state_duration() = 22;
	var3() = 0;
	play_sfx(SFX_066);
}

// towards Wario (away from him, flat Wario: the other way)
void Count::start_jump()
{
	state() = OBJSTATE_SPECIAL_1;
	bool right;
	if (mem_[wTransformation] == TRANSFORMATION_FLAT_WARIO) {
		right = !facing_right();
	} else {
		uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
		right = static_cast<uint8_t>(screen_x() + 0x2a) < wario;
	}
	jump(right);
}

void Count::jump_top()
{
	set_frameset(ADDR_Frameset_683c7);
	state() = OBJSTATE_SPECIAL_2;
	var3() = 0;
}

void Count::hop_along()
{
	bool right = facing_right();
	uint16_t y_pos = static_cast<uint16_t>(y() - 10);
	uint16_t x_pos = right ? static_cast<uint16_t>(x() + 7) : static_cast<uint16_t>(x() - 8);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (!(gb_->cpu.a & 0x0f)) {
		right ? move_right() : move_left();
		return;
	}
	if (right) {
		set_frameset(ADDR_Frameset_683c1);
		substate() &= ~OBJSUBFLAG_HDIR;
	} else {
		set_frameset(ADDR_Frameset_683c4);
		substate() |= OBJSUBFLAG_HDIR;
	}
}

void Count::wario_above()
{
	uint16_t wario_y = static_cast<uint16_t>((mem_[wWarioYPos + 0] << 8 | mem_[wWarioYPos + 1]) - 8);
	y() = wario_y;
}

void Count::stun(uint16_t routine)
{
	if (var2())
		routines_.call(routine);
	else
		state() = OBJ_INTERACTION_RICHTERTOFFEN; // the asm sets the state to this
}

void Count::knocked(uint8_t s, uint8_t var2_)
{
	state() = s;
	no_actions();
	substate() &= OBJSUBFLAGS_MASK;
	var3() = 0;
	var2() = var2_;
	set_frameset(ADDR_Frameset_683ca);
}

bool Count::shrugs_off()
{
	return !mem_[wTransformation] && mem_[wEffectivePowerUp] < GARLIC && (interaction() & INTERACTION_MASK);
}

void Count::wobble(uint8_t s)
{
	mem_[wCurObjAction] = 15 | (1 << 7);
	state() = s;
	set_frameset(ADDR_Frameset_683d9);
	state_duration() = 4;
	var2() = 0x02;
	var3() = 0;
	set_interaction(OBJ_INTERACTION_00);
}

void Count::picked_up(uint8_t s, uint8_t var2_, uint16_t routine)
{
	state() = s;
	var3() = 0x41;
	var2() = var2_;
	var1() = 0x01;
	routines_.call(routine);
}

void Count::grab(uint8_t s)
{
	state() = s;
	set_frameset(ADDR_Frameset_683d9);
	flags() |= 1 << OBJFLAG_GRABBED_F;
}

void Count::run(uint16_t handler)
{
	switch (handler) {
	case VANISH_TOUCH: // pounded: wobbles, unless Wario's strong enough
		if (!mem_[wCurObjAction] || mem_[wTransformation] || mem_[wEffectivePowerUp] >= SUPER_JUMP_SLAM_OVERALLS
			|| !(interaction() & INTERACTION_MASK)) {
			routines_.call(ADDR_VanishObject2);
			return;
		}
		play_sfx(SFX_014);
		{
			uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
			run(static_cast<uint8_t>(screen_x() + 0x2a) >= wario ? WOBBLE_LEFT : WOBBLE_RIGHT);
		}
		return;
	case STATE_0B:
		substate() |= OBJSUBFLAG_HDIR;
		jump_top();
		return;
	case STATE_0C:
		substate() &= ~OBJSUBFLAG_HDIR;
		jump_top();
		return;
	case FLATTEN: // landed on Wario: on him, flattening him
		set_frameset(ADDR_Frameset_683d4);
		state_duration() = 20;
		state() = OBJSTATE_5A;
		wario_above();
		return;
	case STATE_5A:
		if (state_duration() == 20) {
			if (mem_[wAutoMoveState]) {
				routines_.call(ADDR_MoveObjectDown_Fast);
				return;
			}
			wario_above();
		}
		if (--state_duration())
			return;
		mem_[wCurObjAction] = 31 | (1 << 7);
		start_jump();
		return;
	case RESET:
		set_interaction(OBJ_INTERACTION_RICHTERTOFFEN);
		state() = OBJSTATE_FALL;
		var3() = 0;
		set_frameset(ADDR_Frameset_683e1);
		return;
	case STANDING_FALL_START:
		state() = OBJSTATE_STANDING_FALL;
		var3() = 0;
		set_frameset(ADDR_Frameset_683de);
		return;
	case LAND: // once on screen
		if ((flags() & (OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN)) != (OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN))
			return;
		state() = OBJSTATE_WALK;
		set_frameset(ADDR_Frameset_683bc);
		state_duration() = 44;
		return;
	case WALK: // a while, then a hop
		if (!--state_duration()) {
			start_jump();
			return;
		}
		mem_[hYPosLo] = mem_[wCurObjYPos + 0];
		mem_[hYPosHi] = mem_[wCurObjYPos + 1];
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		routines_.call(ADDR_Func_3543);
		if (!gb_->cpu.a) {
			state() = OBJSTATE_STANDING_FALL_START;
			return;
		}
		mem_[wCurObjYPos + 0] = mem_[hYPosLo];
		mem_[wCurObjYPos + 1] = mem_[hYPosHi];
		return;
	case BUMP: { // hops away from Wario
		state() = OBJSTATE_SPECIAL_1;
		uint8_t count = static_cast<uint8_t>(screen_x() + 0x2a);
		jump(static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a) < count);
		return;
	}
	case JUMP_RISE:
		if (!--state_duration()) {
			jump_top();
			return;
		}
		{
			uint16_t y_pos = static_cast<uint16_t>(y() - 18);
			mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
			mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
			mem_[hXPosLo] = mem_[wCurObjXPos + 0];
			mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		}
		routines_.call(ADDR_Func_358b);
		if (gb_->cpu.a & 0x0f) { // the ceiling: turns and falls
			var2() = 0x01;
			substate() ^= OBJSUBFLAG_HDIR;
			jump_top();
			return;
		}
		SET_BC(gb_, ADDR_Data_603d0);
		routines_.call(ADDR_ApplyObjYMovement);
		hop_along();
		return;
	case JUMP_FALL: // lands with a ground shake (lost in water)
		mem_[hYPosLo] = mem_[wCurObjYPos + 0];
		mem_[hYPosHi] = mem_[wCurObjYPos + 1];
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		routines_.call(ADDR_Func_352b);
		if (gb_->cpu.a) {
			mem_[wCurObjYPos + 0] = mem_[hYPosLo];
			mem_[wCurObjYPos + 1] = mem_[hYPosHi];
			state() = OBJSTATE_SPECIAL_3;
			var3() = 0;
			set_frameset(ADDR_Frameset_683d4);
			state_duration() = 24;
			play_sfx(SFX_075);
			gb_->cpu.b = 24;
			routines_.call(ADDR_DoGroundShake);
			return;
		}
		if (mem_[wIsInWaterOrSand]) {
			routines_.call(ADDR_VanishObject2);
			return;
		}
		SET_BC(gb_, ADDR_Data_603e0);
		routines_.call(ADDR_ApplyObjYMovement);
		hop_along();
		return;
	case FALL_BOUNCE:
		if (var3() == 15) {
			if (!--state_duration())
				state() = OBJSTATE_LAND;
			return;
		}
		if (var3() == 14)
			set_frameset(ADDR_Frameset_683e1);
		SET_BC(gb_, ADDR_Data_604b0);
		routines_.call(ADDR_ApplyObjYMovement);
		return;
	case STUN_LEFT_START:
		state() = OBJSTATE_STUN_LEFT;
		set_frameset(ADDR_Frameset_683d9);
		set_interaction(OBJ_INTERACTION_00);
		[[fallthrough]];
	case STUN_LEFT:
		stun(ADDR_Func_32ae);
		return;
	case STUN_RIGHT_START:
		state() = OBJSTATE_STUN_RIGHT;
		set_frameset(ADDR_Frameset_683d9);
		set_interaction(OBJ_INTERACTION_00);
		[[fallthrough]];
	case STUN_RIGHT:
		stun(ADDR_Func_32bd);
		return;
	case STATE_14:
	case STATE_15:
		state() = handler == STATE_14 ? OBJSTATE_44 : OBJSTATE_45;
		set_frameset(ADDR_Frameset_683d9);
		var2() = 0x02;
		var3() = 0;
		set_interaction(OBJ_INTERACTION_00);
		return;
	case STATE_0F:
		state() = OBJSTATE_3F;
		state_duration() = 100;
		flags() &= ~(1 << OBJFLAG_GRABBED_F);
		[[fallthrough]];
	case STATE_3F:
		if (--state_duration())
			routines_.call(ADDR_Func_32ea);
		else
			state() = OBJSTATE_10;
		return;
	case STATE_10:
		state() = OBJSTATE_40;
		var3() = 0;
		state_duration() = 7;
		[[fallthrough]];
	case STATE_40:
		if (--state_duration())
			routines_.call(ADDR_Func_32f9);
		else
			state() = OBJSTATE_00;
		return;
	case TURN_AROUND_START:
		state() = OBJSTATE_TURN_AROUND;
		set_frameset(ADDR_Frameset_683e1);
		state_duration() = 48;
		return;
	case TURN_AROUND:
		if (--state_duration())
			routines_.call(ADDR_Func_3308);
		else
			state() = OBJ_INTERACTION_01; // the asm sets the state to this
		return;
	case WOBBLE_LEFT: wobble(OBJSTATE_WOBBLE_LEFT); return;
	case WOBBLE_RIGHT: wobble(OBJSTATE_WOBBLE_RIGHT); return;
	case STATE_29: knocked(OBJSTATE_59, 0x02); return;
	case STATE_28: knocked(OBJSTATE_58, 0x02); return;
	case ATTACKED_LEFT:
		if (shrugs_off())
			state() = OBJSTATE_WOBBLE_LEFT_START;
		else
			knocked(OBJSTATE_ATTACKED_LEFT, 0x01);
		return;
	case ATTACKED_RIGHT:
		if (shrugs_off())
			state() = OBJSTATE_WOBBLE_RIGHT_START;
		else
			knocked(OBJSTATE_ATTACKED_RIGHT, 0x01);
		return;
	case PICKED_UP_LEFT: picked_up(OBJSTATE_PICKED_UP_LEFT, 0xd0, ADDR_Func_3335); return;
	case PICKED_UP_RIGHT: picked_up(OBJSTATE_PICKED_UP_RIGHT, 0xf0, ADDR_Func_3344); return;
	case STATE_19:
		grab(OBJSTATE_49);
		[[fallthrough]];
	case STATE_49: // carried facing the way Wario does
		if (mem_[wDirection]) {
			routines_.call(ADDR_Func_3353);
			return;
		}
		substate() ^= OBJSUBFLAG_HDIR;
		run(STATE_18);
		return;
	case STATE_18:
		grab(OBJSTATE_48);
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

using Count = wl3::level::Count;

PORT_EX(CountRichtertoffenFunc, 0, ALL_REGISTERS) { Count(gb).init(); }
PORT_LOCAL_EX(CountRichtertoffenFunc, Update, 0, ALL_REGISTERS) { Count(gb).update(); }
PORT_LOCAL_EX(CountRichtertoffenFunc, Default, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::RESET); }
PORT_LOCAL_EX(CountRichtertoffenFunc, VanishTouch, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::VANISH_TOUCH); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State0b, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_0B); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State0c, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_0C); }
PORT_LOCAL_EX(CountRichtertoffenFunc, Flatten, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::FLATTEN); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State5a, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_5A); }
PORT_LOCAL_EX(CountRichtertoffenFunc, StandingFallStart, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STANDING_FALL_START); }
PORT_LOCAL_EX(CountRichtertoffenFunc, Land, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::LAND); }
PORT_LOCAL_EX(CountRichtertoffenFunc, Walk, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::WALK); }
PORT_LOCAL_EX(CountRichtertoffenFunc, Bump, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::BUMP); }
PORT_LOCAL_EX(CountRichtertoffenFunc, JumpRise, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::JUMP_RISE); }
PORT_LOCAL_EX(CountRichtertoffenFunc, JumpFall, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::JUMP_FALL); }
PORT_LOCAL_EX(CountRichtertoffenFunc, FallBounce, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::FALL_BOUNCE); }
PORT_LOCAL_EX(CountRichtertoffenFunc, StunLeftStart, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STUN_LEFT_START); }
PORT_LOCAL_EX(CountRichtertoffenFunc, StunLeft, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STUN_LEFT); }
PORT_LOCAL_EX(CountRichtertoffenFunc, StunRightStart, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STUN_RIGHT_START); }
PORT_LOCAL_EX(CountRichtertoffenFunc, StunRight, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STUN_RIGHT); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State14, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_14); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State15, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_15); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State0f, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_0F); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State3f, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_3F); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State10, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_10); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State40, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_40); }
PORT_LOCAL_EX(CountRichtertoffenFunc, TurnAroundStart, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::TURN_AROUND_START); }
PORT_LOCAL_EX(CountRichtertoffenFunc, TurnAround, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::TURN_AROUND); }
PORT_LOCAL_EX(CountRichtertoffenFunc, WobbleLeftStart, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::WOBBLE_LEFT); }
PORT_LOCAL_EX(CountRichtertoffenFunc, WobbleRightStart, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::WOBBLE_RIGHT); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State29, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_29); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State28, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_28); }
PORT_LOCAL_EX(CountRichtertoffenFunc, AttackedLeftStart, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::ATTACKED_LEFT); }
PORT_LOCAL_EX(CountRichtertoffenFunc, AttackedRightStart, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::ATTACKED_RIGHT); }
PORT_LOCAL_EX(CountRichtertoffenFunc, PickedUpLeftStart, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::PICKED_UP_LEFT); }
PORT_LOCAL_EX(CountRichtertoffenFunc, PickedUpRightStart, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::PICKED_UP_RIGHT); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State19, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_19); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State49, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_49); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State18, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_18); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State48, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_48); }
PORT_LOCAL_EX(CountRichtertoffenFunc, State12, 0, ALL_REGISTERS) { Count(gb).run(wl3::level::STATE_12); }
