// The hammer bot (hammer_bot.asm): a robot walking the floor that swings
// its hammer when Wario's close in front, then charges along; it bounces
// Wario off (stars when it hits him). Knocked about like other enemies,
// through a table of 93 state handlers. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_HammerBotFunc + offset); }

constexpr uint16_t INIT = L(0x0a); // .Init
constexpr uint16_t UPDATE = L(0x1e); // .Update
constexpr uint16_t VANISH_TOUCH = L(0xdc); // .Func_44317
constexpr uint16_t RESET = L(0x114); // .Func_4434f
constexpr uint16_t STATE_41 = L(0x150); // .Func_4438b
constexpr uint16_t SWINGING = L(0x16c); // .Func_443a7
constexpr uint16_t CHARGING = L(0x184); // .Func_443bf
constexpr uint16_t SET_WALK = L(0x231); // .Func_4446c
constexpr uint16_t WALKING = L(0x251); // .Func_4448c
constexpr uint16_t BUMP_32 = L(0x386); // .Func_445c1
constexpr uint16_t BUMP_33 = L(0x38e); // .Func_445c9
constexpr uint16_t HIT_LEFT = L(0x3bc); // .Func_445f7
constexpr uint16_t HIT_LEFT_MOVE = L(0x3e1); // .Func_4461c
constexpr uint16_t HIT_RIGHT = L(0x3ef); // .Func_4462a
constexpr uint16_t HIT_RIGHT_MOVE = L(0x414); // .Func_4464f
constexpr uint16_t STATE_44 = L(0x422); // .Func_4465d
constexpr uint16_t STATE_45 = L(0x42a); // .Func_44665
constexpr uint16_t DROPPED = L(0x457); // .Func_44692
constexpr uint16_t DAZED = L(0x466); // .Func_446a1
constexpr uint16_t BOUNCE_START = L(0x480); // .Func_446bb
constexpr uint16_t BOUNCING = L(0x48e); // .Func_446c9
constexpr uint16_t SET_TURN = L(0x49b); // .Func_446d6
constexpr uint16_t TURNING = L(0x4c8); // .Func_44703
constexpr uint16_t STUNNED_38 = L(0x4f4); // .Func_4472f
constexpr uint16_t STUNNED_39 = L(0x4fe); // .Func_44739
constexpr uint16_t GIVE_STARS_LEFT = L(0x530); // .Func_4476b
constexpr uint16_t GIVE_STARS_RIGHT = L(0x53d); // .Func_44778
constexpr uint16_t RECOIL_3B = L(0x562); // .Func_4479d
constexpr uint16_t RECOIL_3C = L(0x574); // .Func_447af
constexpr uint16_t KNOCKED_59 = L(0x586); // .Func_447c1
constexpr uint16_t KNOCKED_58 = L(0x590); // .Func_447cb
constexpr uint16_t ATTACKED_LEFT = L(0x59a); // .Func_447d5
constexpr uint16_t ATTACKED_RIGHT = L(0x5be); // .Func_447f9
constexpr uint16_t STATE_36 = L(0x5fe); // .Func_44839
constexpr uint16_t STATE_37 = L(0x611); // .Func_4484c
constexpr uint16_t GRAB_49 = L(0x624); // .Func_4485f
constexpr uint16_t HELD_49 = L(0x645); // .Func_44880
constexpr uint16_t GRAB_48 = L(0x655); // .Func_44890
constexpr uint16_t HELD_48 = L(0x676); // .Func_448b1
constexpr uint16_t STATE_42 = L(0x687); // .Func_448c2

// .Update's jumptable, by state
constexpr uint16_t HANDLERS[] = {
	RESET, SET_WALK, BUMP_32, BUMP_33, ATTACKED_LEFT, ATTACKED_RIGHT, STATE_36, STATE_37, // $00
	STUNNED_38, STUNNED_39, VANISH_TOUCH, GIVE_STARS_LEFT, GIVE_STARS_RIGHT, RESET, RESET, DROPPED,
	BOUNCE_START, STATE_41, STATE_42, SET_TURN, STATE_44, STATE_45, HIT_LEFT, HIT_RIGHT, // $10
	GRAB_48, GRAB_49, ADDR_Func_3263, ADDR_Func_3272, ADDR_Func_3281, RESET, RESET, RESET,
	ADDR_Func_3191, ADDR_Func_31af, ADDR_Func_31cd, ADDR_Func_31eb, ADDR_Func_3209, ADDR_Func_3227, ADDR_Func_3245, ADDR_Func_3254, // $20
	KNOCKED_58, KNOCKED_59, RESET, RESET, RESET, SWINGING, CHARGING, RESET,
	ADDR_ObjState_Fall, WALKING, ADDR_ObjState_BumpLeft, ADDR_ObjState_BumpRight, ADDR_Func_3326, ADDR_Func_3317, ADDR_Func_3335, ADDR_Func_3344, // $30
	ADDR_Func_31fa, ADDR_Func_31dc, RESET, RECOIL_3B, RECOIL_3C, RESET, RESET, DAZED,
	BOUNCING, ADDR_ObjState_StandingFall, ADDR_Func_3371, TURNING, ADDR_Func_32cc, ADDR_Func_32db, HIT_LEFT_MOVE, HIT_RIGHT_MOVE, // $40
	HELD_48, HELD_49, ADDR_Func_3380, ADDR_Func_338f, ADDR_ObjState_FullThrowLeft, ADDR_ObjState_FullThrowRight, ADDR_Func_339e, ADDR_Func_33ad,
	ADDR_Func_31a0, ADDR_Func_31be, ADDR_Func_31dc, ADDR_Func_31fa, ADDR_Func_3218, ADDR_Func_3236, ADDR_Func_32cc, ADDR_Func_32db, // $50
	ADDR_Func_33f8, ADDR_Func_3407, RESET, RESET, RESET,
};
constexpr uint8_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

class HammerBot : public Object {
public:
	using Object::Object;

	void start() { set_update(INIT); } // HammerBotFunc
	void init(); // .Init
	void update(); // .Update
	void run(uint16_t handler);

private:
	uint16_t facing(uint16_t left, uint16_t right) { return facing_right() ? right : left; }
	bool wario_to_right() const;
	bool shaken(); // state $1a
	void box(uint8_t left = 0xf8, uint8_t right = 0x07);
	bool on_floor(); // Func_3543 (state $11, falling, if not)
	bool move_on(int edge, int ahead, uint16_t move); // .asm_44414 and the like
	void hit_move(uint16_t routine);
	void hit(uint8_t s);
	void stunned(uint8_t s);
	void stars(uint8_t s, uint16_t params, uint16_t frameset); // .asm_44783
	void recoil(uint16_t routine);
	void knocked(uint8_t s, uint8_t var2_); // .asm_4481b
	bool shrugs_off();
	void grab(uint8_t s);
	void set_state_then(uint8_t s, uint8_t var2_, uint16_t routine);
};

} // namespace

bool HammerBot::wario_to_right() const
{
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	return static_cast<uint8_t>(screen_x() + 0x2a) < wario;
}

void HammerBot::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	run(RESET);
}

void HammerBot::update()
{
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

bool HammerBot::shaken()
{
	if (mem_[wGroundShakeCounter] < 0x10)
		return false;
	state() = 0x1a;
	return true;
}

void HammerBot::box(uint8_t left, uint8_t right)
{
	mem_[wCurObjCollBoxRight] = right;
	mem_[wCurObjCollBoxLeft] = left;
}

bool HammerBot::on_floor()
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

// a step ahead (dx from its x), unless a block edge (its block's
// position past edge) or a wall stops it: then it turns (state $13)
bool HammerBot::move_on(int edge, int ahead, uint16_t move)
{
	uint8_t in_block = mem_[wCurObjXPos + 0] & 0x0f;
	bool right = ahead > 0;
	if (right ? in_block + edge >= 0x10 : in_block < edge) {
		routines_.call(right ? ADDR_ProcessBlockOnRight : ADDR_ProcessBlockOnLeft);
		if (!(gb_->cpu.a & 0x0f)) {
			state() = 0x13;
			return false;
		}
	}
	uint16_t y_pos = static_cast<uint16_t>(y() - 4);
	uint16_t x_pos = static_cast<uint16_t>(x() + ahead);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a) {
		state() = 0x13;
		return false;
	}
	routines_.call(move);
	return true;
}

void HammerBot::hit_move(uint16_t routine)
{
	if (var2())
		routines_.call(routine);
	else
		state() = 0x0f;
}

void HammerBot::hit(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_68b2e, ADDR_Frameset_68b37));
	box();
	set_interaction(OBJ_INTERACTION_00);
}

void HammerBot::stunned(uint8_t s)
{
	mem_[wCurObjAction] = 0x8f;
	hit(s);
	state_duration() = 4;
	var2() = 0x02;
	var3() = 0;
}

void HammerBot::stars(uint8_t s, uint16_t params, uint16_t frameset)
{
	state() = s;
	set_frameset(frameset);
	state_duration() = 20;
	var2() = 0x02;
	flags() &= ~OBJFLAG_PRIORITY;
	play_sfx(SFX_049);
	SET_BC(gb_, params);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

void HammerBot::recoil(uint16_t routine)
{
	if (var2()) {
		routines_.call(routine);
		return;
	}
	if (!--state_duration())
		state() = 0x13;
}

void HammerBot::knocked(uint8_t s, uint8_t var2_)
{
	state() = s;
	no_actions();
	uint8_t sub = substate() & 0xf0;
	substate() = sub;
	var3() = 0;
	var2() = var2_;
	set_frameset((sub & 0x80) ? ADDR_Frameset_68b45 : ADDR_Frameset_68b40);
}

bool HammerBot::shrugs_off()
{
	return !mem_[wTransformation] && mem_[wEffectivePowerUp] < GARLIC && (interaction() & INTERACTION_MASK);
}

void HammerBot::grab(uint8_t s)
{
	state() = s;
	set_frameset(facing(ADDR_Frameset_68b2e, ADDR_Frameset_68b37));
	box();
	flags() |= 1 << OBJFLAG_GRABBED_F;
}

void HammerBot::set_state_then(uint8_t s, uint8_t var2_, uint16_t routine)
{
	state() = s;
	var3() = 0x41;
	var2() = var2_;
	var1() = 0x01;
	routines_.call(routine);
}

void HammerBot::run(uint16_t handler)
{
	switch (handler) {
	case VANISH_TOUCH: // pounded: wobbles, unless Wario's strong enough
		if (!mem_[wCurObjAction] || mem_[wTransformation] || mem_[wEffectivePowerUp] >= SUPER_JUMP_SLAM_OVERALLS
			|| !(interaction() & INTERACTION_MASK)) {
			routines_.call(ADDR_VanishObject2);
			return;
		}
		play_sfx(SFX_014);
		run(wario_to_right() ? STUNNED_39 : STUNNED_38);
		return;
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
		set_frameset(right ? ADDR_Frameset_68b17 : ADDR_Frameset_68b12);
		return;
	}
	case STATE_41:
		state() = 0x41;
		var3() = 0;
		set_frameset(facing(ADDR_Frameset_68b12, ADDR_Frameset_68b17));
		return;
	case SWINGING:
		if (shaken())
			return;
		if (!--state_duration())
			state() = 0x2e;
		return;
	case CHARGING: { // fast, until a wall or an edge
		if (!(mem_[wGlobalCounter] & 0x0f))
			play_sfx(SFX_107);
		if (shaken() || !on_floor())
			return;
		bool right = facing_right();
		move_on(right ? 0x0f : 0x10, right ? 0x0f : -0x10, right ? ADDR_MoveObjectRight : ADDR_MoveObjectLeft);
		return;
	}
	case SET_WALK:
		set_interaction(OBJ_INTERACTION_01);
		state() = 0x31;
		set_frameset(facing(ADDR_Frameset_68b12, ADDR_Frameset_68b17));
		return;
	case WALKING: { // swings when Wario's level and close in front
		if ((flags() & OBJFLAG_ON_SCREEN) && !(mem_[wGlobalCounter] & 0x1f))
			play_sfx(SFX_107);
		if (shaken() || !on_floor())
			return;
		bool right = facing_right();
		uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
		if (dy < 0x18 || dy >= 0xe8) {
			uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
			if (right ? dx >= 0xc0 : dx < 0x40) {
				set_frameset(right ? ADDR_Frameset_68b25 : ADDR_Frameset_68b1c);
				state_duration() = 20;
				if (right)
					box(0xf8, 0x0f);
				else
					box(0xf0, 0x07);
				set_interaction(OBJ_INTERACTION_BOUNCY);
				state() = 0x2d;
				return;
			}
		}
		move_on(right ? 0x07 : 0x08, right ? 0x07 : -0x08, right ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
		return;
	}
	case BUMP_32:
	case BUMP_33:
		state() = handler == BUMP_32 ? 0x32 : 0x33;
		set_frameset(facing(ADDR_Frameset_68b58, ADDR_Frameset_68b5b));
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
		set_frameset(facing(ADDR_Frameset_68b4a, ADDR_Frameset_68b51));
		state_duration() = 26;
		box();
		set_interaction(OBJ_INTERACTION_01);
		flags() |= OBJFLAG_PRIORITY;
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
	case GIVE_STARS_LEFT: stars(0x3b, ADDR_ObjParams_StarsLeft, ADDR_Frameset_68b5e); return;
	case GIVE_STARS_RIGHT: stars(0x3c, ADDR_ObjParams_StarsRight, ADDR_Frameset_68b61); return;
	case RECOIL_3B: recoil(ADDR_Func_32bd); return;
	case RECOIL_3C: recoil(ADDR_Func_32ae); return;
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

using HammerBot = wl3::level::HammerBot;

PORT_EX(HammerBotFunc, 0, ALL_REGISTERS) { HammerBot(gb).start(); }
PORT_LOCAL_EX(HammerBotFunc, Init, 0, ALL_REGISTERS) { HammerBot(gb).init(); }
PORT_LOCAL_EX(HammerBotFunc, Update, 0, ALL_REGISTERS) { HammerBot(gb).update(); }
PORT_LOCAL_EX(HammerBotFunc, Func_44317, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::VANISH_TOUCH); }
PORT_LOCAL_EX(HammerBotFunc, Func_4434f, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::RESET); }
PORT_LOCAL_EX(HammerBotFunc, Func_4438b, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::STATE_41); }
PORT_LOCAL_EX(HammerBotFunc, Func_443a7, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::SWINGING); }
PORT_LOCAL_EX(HammerBotFunc, Func_443bf, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::CHARGING); }
PORT_LOCAL_EX(HammerBotFunc, Func_4446c, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::SET_WALK); }
PORT_LOCAL_EX(HammerBotFunc, Func_4448c, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::WALKING); }
PORT_LOCAL_EX(HammerBotFunc, Func_445c1, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::BUMP_32); }
PORT_LOCAL_EX(HammerBotFunc, Func_445c9, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::BUMP_33); }
PORT_LOCAL_EX(HammerBotFunc, Func_445f7, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::HIT_LEFT); }
PORT_LOCAL_EX(HammerBotFunc, Func_4461c, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::HIT_LEFT_MOVE); }
PORT_LOCAL_EX(HammerBotFunc, Func_4462a, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::HIT_RIGHT); }
PORT_LOCAL_EX(HammerBotFunc, Func_4464f, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::HIT_RIGHT_MOVE); }
PORT_LOCAL_EX(HammerBotFunc, Func_4465d, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::STATE_44); }
PORT_LOCAL_EX(HammerBotFunc, Func_44665, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::STATE_45); }
PORT_LOCAL_EX(HammerBotFunc, Func_44692, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::DROPPED); }
PORT_LOCAL_EX(HammerBotFunc, Func_446a1, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::DAZED); }
PORT_LOCAL_EX(HammerBotFunc, Func_446bb, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::BOUNCE_START); }
PORT_LOCAL_EX(HammerBotFunc, Func_446c9, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::BOUNCING); }
PORT_LOCAL_EX(HammerBotFunc, Func_446d6, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::SET_TURN); }
PORT_LOCAL_EX(HammerBotFunc, Func_44703, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::TURNING); }
PORT_LOCAL_EX(HammerBotFunc, Func_4472f, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::STUNNED_38); }
PORT_LOCAL_EX(HammerBotFunc, Func_44739, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::STUNNED_39); }
PORT_LOCAL_EX(HammerBotFunc, Func_4476b, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::GIVE_STARS_LEFT); }
PORT_LOCAL_EX(HammerBotFunc, Func_44778, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::GIVE_STARS_RIGHT); }
PORT_LOCAL_EX(HammerBotFunc, Func_4479d, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::RECOIL_3B); }
PORT_LOCAL_EX(HammerBotFunc, Func_447af, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::RECOIL_3C); }
PORT_LOCAL_EX(HammerBotFunc, Func_447c1, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::KNOCKED_59); }
PORT_LOCAL_EX(HammerBotFunc, Func_447cb, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::KNOCKED_58); }
PORT_LOCAL_EX(HammerBotFunc, Func_447d5, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::ATTACKED_LEFT); }
PORT_LOCAL_EX(HammerBotFunc, Func_447f9, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::ATTACKED_RIGHT); }
PORT_LOCAL_EX(HammerBotFunc, Func_44839, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::STATE_36); }
PORT_LOCAL_EX(HammerBotFunc, Func_4484c, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::STATE_37); }
PORT_LOCAL_EX(HammerBotFunc, Func_4485f, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::GRAB_49); }
PORT_LOCAL_EX(HammerBotFunc, Func_44880, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::HELD_49); }
PORT_LOCAL_EX(HammerBotFunc, Func_44890, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::GRAB_48); }
PORT_LOCAL_EX(HammerBotFunc, Func_448b1, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::HELD_48); }
PORT_LOCAL_EX(HammerBotFunc, Func_448c2, 0, ALL_REGISTERS) { HammerBot(gb).run(wl3::level::STATE_42); }
