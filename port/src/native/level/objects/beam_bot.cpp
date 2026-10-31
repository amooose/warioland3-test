// The beam bot (beam_bot.asm): a robot walking the floor that, when
// Wario's level and ahead, fires a beam at him (splitting him in two).
// Knocked about like other enemies, through a table of 93 state
// handlers. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_BeamBotFunc + offset); }

constexpr uint16_t INIT = L(0x0a); // .Init
constexpr uint16_t UPDATE = L(0x1e); // .Update
constexpr uint16_t RESET = L(0xdc); // .Func_46691
constexpr uint16_t STATE_41 = L(0x116); // .Func_466cb
constexpr uint16_t FIRING = L(0x132); // .Func_466e7
constexpr uint16_t FIRED = L(0x177); // .Func_4672c
constexpr uint16_t SET_WALK = L(0x193); // .Func_46748
constexpr uint16_t WALKING = L(0x1b4); // .Func_46769
constexpr uint16_t BUMP_32 = L(0x2bf); // .Func_46874
constexpr uint16_t BUMP_33 = L(0x2c7); // .Func_4687c
constexpr uint16_t HIT_LEFT = L(0x2f5); // .Func_468aa
constexpr uint16_t HIT_LEFT_MOVE = L(0x31a); // .Func_468cf
constexpr uint16_t HIT_RIGHT = L(0x328); // .Func_468dd
constexpr uint16_t HIT_RIGHT_MOVE = L(0x34d); // .Func_46902
constexpr uint16_t STATE_44 = L(0x35b); // .Func_46910
constexpr uint16_t STATE_45 = L(0x363); // .Func_46918
constexpr uint16_t DROPPED = L(0x390); // .Func_46945
constexpr uint16_t DAZED = L(0x39f); // .Func_46954
constexpr uint16_t BOUNCE_START = L(0x3b9); // .Func_4696e
constexpr uint16_t BOUNCING = L(0x3c7); // .Func_4697c
constexpr uint16_t SET_TURN = L(0x3d4); // .Func_46989
constexpr uint16_t TURNING = L(0x3fd); // .Func_469b2
constexpr uint16_t STUNNED_38 = L(0x42d); // .Func_469e2
constexpr uint16_t STUNNED_39 = L(0x437); // .Func_469ec
constexpr uint16_t KNOCKED_59 = L(0x469); // .Func_46a1e
constexpr uint16_t KNOCKED_58 = L(0x473); // .Func_46a28
constexpr uint16_t KNOCKED_34 = L(0x47d); // .Func_46a32
constexpr uint16_t KNOCKED_35 = L(0x487); // .Func_46a3c
constexpr uint16_t STATE_36 = L(0x4ad); // .Func_46a62
constexpr uint16_t STATE_37 = L(0x4c0); // .Func_46a75
constexpr uint16_t GRAB_49 = L(0x4d3); // .Func_46a88
constexpr uint16_t HELD_49 = L(0x4f4); // .Func_46aa9
constexpr uint16_t GRAB_48 = L(0x504); // .Func_46ab9
constexpr uint16_t HELD_48 = L(0x525); // .Func_46ada
constexpr uint16_t STATE_42 = L(0x536); // .Func_46aeb

constexpr uint16_t BEAM_UPDATE_LEFT = ADDR_BeamFunc_Left + 0x15; // .Update
constexpr uint16_t BEAM_UPDATE_RIGHT = ADDR_BeamFunc_Right + 0x15; // .Update

// .Update's jumptable, by state
constexpr uint16_t HANDLERS[] = {
	RESET, SET_WALK, BUMP_32, BUMP_33, KNOCKED_34, KNOCKED_35, STATE_36, STATE_37, // $00
	STUNNED_38, STUNNED_39, ADDR_VanishObject2, RESET, RESET, RESET, RESET, DROPPED,
	BOUNCE_START, STATE_41, STATE_42, SET_TURN, STATE_44, STATE_45, HIT_LEFT, HIT_RIGHT, // $10
	GRAB_48, GRAB_49, ADDR_Func_3263, ADDR_Func_3272, ADDR_Func_3281, RESET, RESET, RESET,
	ADDR_Func_3191, ADDR_Func_31af, ADDR_Func_31cd, ADDR_Func_31eb, ADDR_Func_3209, ADDR_Func_3227, ADDR_Func_3245, ADDR_Func_3254, // $20
	KNOCKED_58, KNOCKED_59, RESET, RESET, RESET, FIRING, FIRED, RESET,
	ADDR_ObjState_Fall, WALKING, ADDR_ObjState_BumpLeft, ADDR_ObjState_BumpRight, ADDR_Func_3326, ADDR_Func_3317, ADDR_Func_3335, ADDR_Func_3344, // $30
	ADDR_Func_31fa, ADDR_Func_31dc, RESET, RESET, RESET, RESET, RESET, DAZED,
	BOUNCING, ADDR_ObjState_StandingFall, ADDR_Func_3371, TURNING, ADDR_Func_32cc, ADDR_Func_32db, HIT_LEFT_MOVE, HIT_RIGHT_MOVE, // $40
	HELD_48, HELD_49, ADDR_Func_3380, ADDR_Func_338f, ADDR_ObjState_FullThrowLeft, ADDR_ObjState_FullThrowRight, ADDR_Func_339e, ADDR_Func_33ad,
	ADDR_Func_31a0, ADDR_Func_31be, ADDR_Func_31dc, ADDR_Func_31fa, ADDR_Func_3218, ADDR_Func_3236, ADDR_Func_32cc, ADDR_Func_32db, // $50
	ADDR_Func_33f8, ADDR_Func_3407, RESET, RESET, RESET,
};
constexpr uint8_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

class BeamBot : public Object {
public:
	using Object::Object;

	void start() { set_update(INIT); } // BeamBotFunc
	void init(); // .Init
	void update(); // .Update
	void run(uint16_t handler);
	void beam_start(bool right); // BeamFunc_Left, BeamFunc_Right
	void beam(bool right); // .Update
	void beam_move(uint16_t movement); // BeamFunc

private:
	uint16_t facing(uint16_t left, uint16_t right) { return facing_right() ? right : left; }
	bool wario_to_right() const;
	bool shaken();
	void small_box();
	void walk(); // .asm_467cf
	void hit_move(uint16_t routine);
	void hit(uint8_t s);
	void stunned(uint8_t s);
	void knocked(uint8_t s); // .asm_46a44
	void grab(uint8_t s);
	void set_state_then(uint8_t s, uint8_t var2_, uint16_t routine);
};

} // namespace

bool BeamBot::wario_to_right() const
{
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	return static_cast<uint8_t>(screen_x() + 0x2a) < wario;
}

void BeamBot::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	run(RESET);
}

void BeamBot::update()
{
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

bool BeamBot::shaken()
{
	if (mem_[wGroundShakeCounter] < 0x10)
		return false;
	state() = static_cast<uint8_t>(0x08 | (mem_[rDIV] & 1));
	return true;
}

void BeamBot::small_box()
{
	mem_[wCurObjCollBoxRight] = 0x05;
	mem_[wCurObjCollBoxLeft] = 0xfa;
}

// slowly along the floor; at a block's edge it marks time (half speed
// animation) until its time's up; at a wall it turns
void BeamBot::walk()
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
	if (right ? in_block + 5 >= 0x10 : in_block < 6) {
		routines_.call(right ? ADDR_ProcessBlockOnRight : ADDR_ProcessBlockOnLeft);
		if (!(gb_->cpu.a & 0x0f)) { // .asm_4685f
			if (!(mem_[wGlobalCounter] & 1))
				++frame_duration();
			if (!--state_duration())
				state() = 0x13;
			return;
		}
	}
	uint16_t y_pos = static_cast<uint16_t>(y() - 4);
	uint16_t x_pos = right ? static_cast<uint16_t>(x() + 5) : static_cast<uint16_t>(x() - 6);
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

void BeamBot::hit_move(uint16_t routine)
{
	if (var2())
		routines_.call(routine);
	else
		state() = 0x0f;
}

void BeamBot::hit(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_68eb7, ADDR_Frameset_68ec0));
	small_box();
	set_interaction(OBJ_INTERACTION_00);
}

void BeamBot::stunned(uint8_t s)
{
	mem_[wCurObjAction] = 0x8f;
	hit(s);
	state_duration() = 4;
	var2() = 0x02;
	var3() = 0;
}

void BeamBot::knocked(uint8_t s)
{
	state() = s;
	no_actions();
	uint8_t sub = substate() & 0xf0;
	substate() = sub;
	var3() = 0;
	var2() = 0x02;
	set_frameset((sub & 0x80) ? ADDR_Frameset_68ece : ADDR_Frameset_68ec9);
}

void BeamBot::grab(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_68eb7, ADDR_Frameset_68ec0));
	small_box();
	flags() |= 1 << OBJFLAG_GRABBED_F;
}

void BeamBot::set_state_then(uint8_t s, uint8_t var2_, uint16_t routine)
{
	state() = s;
	var3() = 0x41;
	var2() = var2_;
	var1() = 0x01;
	routines_.call(routine);
}

void BeamBot::run(uint16_t handler)
{
	switch (handler) {
	case RESET: { // facing Wario
		set_interaction(OBJ_INTERACTION_01);
		small_box();
		state() = 0x30;
		bool right = wario_to_right();
		if (right)
			substate() |= OBJSUBFLAG_HDIR;
		else
			substate() &= ~OBJSUBFLAG_HDIR;
		var3() = 0;
		set_frameset(right ? ADDR_Frameset_68e80 : ADDR_Frameset_68e77);
		return;
	}
	case STATE_41:
		state() = 0x41;
		var3() = 0;
		set_frameset(facing(ADDR_Frameset_68e77, ADDR_Frameset_68e80));
		return;
	case FIRING: // the beam at $19 frames left
		if (shaken())
			return;
		if (!--state_duration()) {
			state() = 0x2e;
			set_frameset(facing(ADDR_Frameset_68ea3, ADDR_Frameset_68eaa));
			state_duration() = 50;
			return;
		}
		if (state_duration() != 0x19)
			return;
		SET_BC(gb_, facing(ADDR_ObjParams_BeamLeft, ADDR_ObjParams_BeamRight));
		routines_.call(ADDR_CreateObjectAtRelativePos);
		return;
	case FIRED:
		if (shaken())
			return;
		if (!--state_duration())
			state() = 0x13;
		return;
	case SET_WALK:
		set_interaction(OBJ_INTERACTION_01);
		state() = 0x31;
		set_frameset(facing(ADDR_Frameset_68e77, ADDR_Frameset_68e80));
		state_duration() = 100;
		return;
	case WALKING: // fires when Wario's level and ahead; turns if he's behind
		if (shaken())
			return;
		if (mem_[wTransformation] != TRANSFORMATION_SPLIT && (flags() & OBJFLAG_ON_SCREEN)) {
			uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
			if (dy < 0x1c || dy >= 0xe4) {
				uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
				bool near_right = dx >= 0xb8, near_left = dx < 0x48;
				if (near_right || near_left) {
					if (facing_right() != near_right) {
						state() = 0x13;
						return;
					}
					state() = 0x2d;
					set_frameset(near_right ? ADDR_Frameset_68e96 : ADDR_Frameset_68e89);
					state_duration() = 41;
					return;
				}
			}
		}
		walk();
		return;
	case BUMP_32:
	case BUMP_33:
		state() = handler == BUMP_32 ? 0x32 : 0x33;
		set_frameset(facing(ADDR_Frameset_68eb1, ADDR_Frameset_68eb4));
		state_duration() = 12;
		var2() = 0x02;
		small_box();
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
		set_frameset(facing(ADDR_Frameset_68ed3, ADDR_Frameset_68eda));
		state_duration() = 17;
		small_box();
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

void BeamBot::beam_start(bool right)
{
	set_update(right ? BEAM_UPDATE_RIGHT : BEAM_UPDATE_LEFT);
	flags() |= OBJFLAG_NO_COLLISION;
	play_sfx(SFX_07C);
	beam(right);
}

// its frameset changes once its time's up
void BeamBot::beam(bool right)
{
	if (state_duration() && !--state_duration())
		set_frameset(right ? ADDR_Frameset_68ef8 : ADDR_Frameset_68ef3);
	beam_move(right ? ADDR_Data_60630 : ADDR_Data_60610);
}

void BeamBot::beam_move(uint16_t movement)
{
	if (state()) { // hit: a sound
		state() = 0;
		play_sfx(SFX_07D);
	}
	SET_BC(gb_, movement);
	routines_.call(ADDR_ApplyObjXMovement);
}

} // namespace wl3::level

using BeamBot = wl3::level::BeamBot;

PORT_EX(BeamBotFunc, 0, ALL_REGISTERS) { BeamBot(gb).start(); }
PORT_LOCAL_EX(BeamBotFunc, Init, 0, ALL_REGISTERS) { BeamBot(gb).init(); }
PORT_LOCAL_EX(BeamBotFunc, Update, 0, ALL_REGISTERS) { BeamBot(gb).update(); }
PORT_LOCAL_EX(BeamBotFunc, Func_46691, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::RESET); }
PORT_LOCAL_EX(BeamBotFunc, Func_466cb, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::STATE_41); }
PORT_LOCAL_EX(BeamBotFunc, Func_466e7, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::FIRING); }
PORT_LOCAL_EX(BeamBotFunc, Func_4672c, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::FIRED); }
PORT_LOCAL_EX(BeamBotFunc, Func_46748, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::SET_WALK); }
PORT_LOCAL_EX(BeamBotFunc, Func_46769, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::WALKING); }
PORT_LOCAL_EX(BeamBotFunc, Func_46874, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::BUMP_32); }
PORT_LOCAL_EX(BeamBotFunc, Func_4687c, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::BUMP_33); }
PORT_LOCAL_EX(BeamBotFunc, Func_468aa, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::HIT_LEFT); }
PORT_LOCAL_EX(BeamBotFunc, Func_468cf, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::HIT_LEFT_MOVE); }
PORT_LOCAL_EX(BeamBotFunc, Func_468dd, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::HIT_RIGHT); }
PORT_LOCAL_EX(BeamBotFunc, Func_46902, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::HIT_RIGHT_MOVE); }
PORT_LOCAL_EX(BeamBotFunc, Func_46910, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::STATE_44); }
PORT_LOCAL_EX(BeamBotFunc, Func_46918, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::STATE_45); }
PORT_LOCAL_EX(BeamBotFunc, Func_46945, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::DROPPED); }
PORT_LOCAL_EX(BeamBotFunc, Func_46954, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::DAZED); }
PORT_LOCAL_EX(BeamBotFunc, Func_4696e, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::BOUNCE_START); }
PORT_LOCAL_EX(BeamBotFunc, Func_4697c, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::BOUNCING); }
PORT_LOCAL_EX(BeamBotFunc, Func_46989, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::SET_TURN); }
PORT_LOCAL_EX(BeamBotFunc, Func_469b2, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::TURNING); }
PORT_LOCAL_EX(BeamBotFunc, Func_469e2, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::STUNNED_38); }
PORT_LOCAL_EX(BeamBotFunc, Func_469ec, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::STUNNED_39); }
PORT_LOCAL_EX(BeamBotFunc, Func_46a1e, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::KNOCKED_59); }
PORT_LOCAL_EX(BeamBotFunc, Func_46a28, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::KNOCKED_58); }
PORT_LOCAL_EX(BeamBotFunc, Func_46a32, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::KNOCKED_34); }
PORT_LOCAL_EX(BeamBotFunc, Func_46a3c, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::KNOCKED_35); }
PORT_LOCAL_EX(BeamBotFunc, Func_46a62, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::STATE_36); }
PORT_LOCAL_EX(BeamBotFunc, Func_46a75, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::STATE_37); }
PORT_LOCAL_EX(BeamBotFunc, Func_46a88, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::GRAB_49); }
PORT_LOCAL_EX(BeamBotFunc, Func_46aa9, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::HELD_49); }
PORT_LOCAL_EX(BeamBotFunc, Func_46ab9, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::GRAB_48); }
PORT_LOCAL_EX(BeamBotFunc, Func_46ada, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::HELD_48); }
PORT_LOCAL_EX(BeamBotFunc, Func_46aeb, 0, ALL_REGISTERS) { BeamBot(gb).run(wl3::level::STATE_42); }
PORT_EX(BeamFunc_Left, 0, ALL_REGISTERS) { BeamBot(gb).beam_start(false); }
PORT_EX(BeamFunc_Right, 0, ALL_REGISTERS) { BeamBot(gb).beam_start(true); }
PORT_LOCAL_EX(BeamFunc_Left, Update, 0, ALL_REGISTERS) { BeamBot(gb).beam(false); }
PORT_LOCAL_EX(BeamFunc_Right, Update, 0, ALL_REGISTERS) { BeamBot(gb).beam(true); }
PORT_EX(BeamFunc, 0, ALL_REGISTERS) { BeamBot(gb).beam_move(REG_BC(gb)); }
