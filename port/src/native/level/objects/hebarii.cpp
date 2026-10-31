// Hebarii (hebarii.asm): a creature crawling along the ceiling that drops
// spikes when it stops; a strong ground shake (or a hit) brings it down,
// and it then walks the floor for a while and is knocked about like other
// enemies, through a table of 96 state handlers. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_HebariiFunc + offset); }

constexpr uint16_t INIT = L(0x0a); // .Init
constexpr uint16_t ATTACHED = L(0x73); // .AttachedUpdate
constexpr uint16_t DETACHED = L(0x157); // .DetachedUpdate
constexpr uint16_t VANISH = L(0x21b); // .Vanish
constexpr uint16_t FALL = L(0x21e); // .FallFromCeiling
constexpr uint16_t WALKING = L(0x260); // .Func_4a224
constexpr uint16_t STATE_0F = L(0x276); // .State0f
constexpr uint16_t STATE_3F = L(0x28d); // .State3f
constexpr uint16_t WOBBLE_LEFT = L(0x2a9); // .Func_4a26d
constexpr uint16_t WOBBLE_RIGHT = L(0x2b3); // .Func_4a277
constexpr uint16_t STUN_LEFT_START = L(0x2d4); // .StunLeftStart
constexpr uint16_t STUN_LEFT = L(0x2e0); // .StunLeft
constexpr uint16_t STUN_RIGHT_START = L(0x2ee); // .StunRightStart
constexpr uint16_t STUN_RIGHT = L(0x2fa); // .StunRight
constexpr uint16_t STATE_14 = L(0x308); // .State14
constexpr uint16_t STATE_15 = L(0x30c); // .State15
constexpr uint16_t PICKED_UP_LEFT = L(0x31b); // .PickedUpLeftStart
constexpr uint16_t PICKED_UP_RIGHT = L(0x334); // .PickedUpRightStart
constexpr uint16_t STATE_19 = L(0x34d); // .State19
constexpr uint16_t STATE_49 = L(0x35c); // .State49
constexpr uint16_t STATE_18 = L(0x36c); // .State18
constexpr uint16_t STATE_48 = L(0x37b); // .State48
constexpr uint16_t STATE_12 = L(0x38c); // .State12
constexpr uint16_t THROW_LEFT = L(0x399); // .FullThrowLeftStart
constexpr uint16_t THROW_RIGHT = L(0x3a7); // .FullThrowRightStart
constexpr uint16_t STATE_28 = L(0x3b5); // .State28
constexpr uint16_t STATE_29 = L(0x3c3); // .State29
constexpr uint16_t ATTACKED_RIGHT = L(0x3d1); // .AttackedRightStart
constexpr uint16_t ATTACKED_LEFT = L(0x3df); // .AttackedLeftStart

constexpr uint16_t SPIKE_LATERAL = ADDR_HebariiProjectileFunc + 0x3f; // .MoveWithLateral
constexpr uint16_t SPIKE_DOWN = ADDR_HebariiProjectileFunc + 0x4d; // .MoveWithoutLateral
constexpr uint16_t SPIKE_GONE = ADDR_HebariiProjectileFunc + 0x8c; // .Destroy

// .DetachedUpdate's jumptable, by state
constexpr uint16_t HANDLERS[] = {
	VANISH, VANISH, WOBBLE_LEFT, WOBBLE_RIGHT, ATTACKED_LEFT, ATTACKED_RIGHT, PICKED_UP_LEFT, PICKED_UP_RIGHT, // $00
	WOBBLE_LEFT, WOBBLE_RIGHT, VANISH, VANISH, VANISH, VANISH, VANISH, STATE_0F,
	VANISH, VANISH, STATE_12, WALKING, STATE_14, STATE_15, STUN_LEFT_START, STUN_RIGHT_START, // $10
	STATE_18, STATE_19, VANISH, VANISH, VANISH, VANISH, VANISH, VANISH,
	ADDR_Func_3191, ADDR_Func_31af, ADDR_Func_31cd, ADDR_Func_31eb, ADDR_Func_3209, ADDR_Func_3227, ADDR_Func_3245, ADDR_Func_3254, // $20
	STATE_28, STATE_29, VANISH, VANISH, VANISH, FALL, VANISH, VANISH,
	VANISH, WALKING, ADDR_ObjState_BumpLeft, ADDR_ObjState_BumpRight, VANISH, VANISH, ADDR_Func_3335, ADDR_Func_3344, // $30
	ADDR_Func_31fa, ADDR_Func_31dc, VANISH, VANISH, VANISH, VANISH, VANISH, STATE_3F,
	VANISH, VANISH, ADDR_Func_3371, VANISH, ADDR_Func_32cc, ADDR_Func_32db, STUN_LEFT, STUN_RIGHT, // $40
	STATE_48, STATE_49, ADDR_Func_3380, ADDR_Func_338f, THROW_LEFT, THROW_RIGHT, ADDR_Func_339e, ADDR_Func_33ad,
	ADDR_Func_31a0, ADDR_Func_31be, ADDR_Func_31dc, ADDR_Func_31fa, ADDR_Func_3218, ADDR_Func_3236, ADDR_Func_32cc, ADDR_Func_32db, // $50
	VANISH, VANISH, VANISH, VANISH, VANISH, VANISH, ADDR_ObjState_FullThrowLeft, ADDR_ObjState_FullThrowRight,
};
constexpr uint8_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

class Hebarii : public Object {
public:
	using Object::Object;

	void start() { set_update(INIT); } // HebariiFunc
	void init(); // .Init
	void attached(); // .AttachedUpdate
	void detached(); // .DetachedUpdate
	void run(uint16_t handler);
	void spike(); // HebariiProjectileFunc
	void spike_fall(bool lateral); // .MoveWithLateral, .MoveWithoutLateral
	void spike_gone(); // .Destroy

private:
	void crawl(); // .asm_4a08d
	void knocked(uint16_t frameset, uint16_t routine); // .asm_4a3af
	void wobble(uint8_t s);
	void stun(uint16_t routine);
	void picked_up(uint8_t s, uint8_t var2_, uint16_t routine);
	void grab(uint8_t s);
	void throw_(uint8_t s, uint16_t frameset, uint16_t routine);
};

} // namespace

void Hebarii::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 5;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-7);
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-12);
	set_update(ATTACHED);
	set_frameset(ADDR_Frameset_683e4);
	state_duration() = 20;
	var1() = 0;
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	if (static_cast<uint8_t>(screen_x() + 0x2a) < wario)
		substate() |= OBJSUBFLAG_HDIR;
	else
		substate() &= ~OBJSUBFLAG_HDIR;
}

// on the ceiling, until a strong ground shake or a hit
void Hebarii::attached()
{
	if (mem_[wGroundShakeCounter] >= 0x10) { // drops
		set_update(DETACHED);
		set_frameset(ADDR_Frameset_683f8);
		mem_[wCurObjCollBoxRight] = 5;
		mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-6);
		mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-1);
		mem_[wCurObjCollBoxTop] = static_cast<uint8_t>(-14);
		set_interaction(OBJ_INTERACTION_01);
		state() = OBJSTATE_SPECIAL_1;
		var3() = 0;
		return;
	}
	uint8_t s = state();
	if (s) {
		switch (s) {
		case OBJSTATE_ATTACKED_LEFT_START:
			set_update(DETACHED);
			run(ATTACKED_LEFT);
			return;
		case OBJSTATE_ATTACKED_RIGHT_START:
			set_update(DETACHED);
			run(ATTACKED_RIGHT);
			return;
		case OBJSTATE_28:
			set_update(DETACHED);
			run(STATE_28);
			return;
		case OBJSTATE_29:
			set_update(DETACHED);
			run(STATE_29);
			return;
		case OBJSTATE_VANISH_TOUCH:
			routines_.call(ADDR_VanishObject2);
			return;
		}
		state() = 0;
		flags() &= ~OBJFLAG_NO_COLLISION;
	}
	crawl();
}

// crawls a pixel every 4 frames, turning at the ceiling's end or a wall;
// stops every 20 frames to drop a spike
void Hebarii::crawl()
{
	if (var1() == 1) { // stopped
		if (state_duration() == 30) {
			--state_duration();
			if (!(flags() & OBJFLAG_ON_SCREEN))
				return;
			SET_BC(gb_, ADDR_ObjParams_HebariiProjectile);
			routines_.call(ADDR_CreateObjectAtRelativePos);
			return;
		}
		if (--state_duration())
			return;
		state_duration() = 126;
		var1() = 0;
		set_frameset(ADDR_Frameset_683e4);
		return;
	}
	if (!--state_duration()) {
		set_frameset(ADDR_Frameset_683ed);
		state_duration() = 41;
		var1() = 0x01;
		return;
	}
	if (mem_[wGlobalCounter] & 0b11)
		return;
	bool right = facing_right();
	uint16_t y_pos = static_cast<uint16_t>(y() - 18);
	uint16_t x_pos = right ? static_cast<uint16_t>(x() + 9) : static_cast<uint16_t>(x() - 11);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3513);
	bool turn = !gb_->cpu.a; // no ceiling ahead
	if (!turn) {
		y_pos = static_cast<uint16_t>(y() - 8);
		mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
		mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
		routines_.call(ADDR_Func_3513);
		turn = gb_->cpu.a; // a wall
	}
	if (turn)
		substate() ^= OBJSUBFLAG_HDIR;
	else
		right ? move_right() : move_left();
}

void Hebarii::detached()
{
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

void Hebarii::knocked(uint16_t frameset, uint16_t routine)
{
	set_frameset(frameset);
	set_update(routine);
	substate() &= OBJSUBFLAGS_MASK;
	var3() = 0;
	var2() = 0x02;
	no_actions();
}

void Hebarii::wobble(uint8_t s)
{
	mem_[wCurObjAction] = 15 | (1 << 7);
	state() = s;
	set_frameset(ADDR_Frameset_683fb);
	state_duration() = 4;
	var2() = 0x02;
	var3() = 0;
	set_interaction(OBJ_INTERACTION_00);
}

void Hebarii::stun(uint16_t routine)
{
	if (var2())
		routines_.call(routine);
	else
		state() = OBJSTATE_0F;
}

void Hebarii::picked_up(uint8_t s, uint8_t var2_, uint16_t routine)
{
	state() = s;
	var3() = 65;
	var2() = var2_;
	var1() = 0x01;
	set_frameset(ADDR_Frameset_683fb);
	routines_.call(routine);
}

void Hebarii::grab(uint8_t s)
{
	state() = s;
	set_frameset(ADDR_Frameset_683fb);
	flags() |= 1 << OBJFLAG_GRABBED_F;
}

void Hebarii::throw_(uint8_t s, uint16_t frameset, uint16_t routine)
{
	state() = s;
	set_frameset(frameset);
	routines_.call(routine);
}

void Hebarii::run(uint16_t handler)
{
	switch (handler) {
	case VANISH: routines_.call(ADDR_VanishObject2); return;
	case FALL: // until it lands (lost in water)
		mem_[hYPosLo] = mem_[wCurObjYPos + 0];
		mem_[hYPosHi] = mem_[wCurObjYPos + 1];
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		routines_.call(ADDR_Func_352b);
		if (!gb_->cpu.a) {
			if (mem_[wIsInWaterOrSand]) {
				routines_.call(ADDR_VanishObject2);
				return;
			}
			SET_BC(gb_, ADDR_FallingYVel_Light);
			routines_.call(ADDR_ApplyObjYMovement);
			return;
		}
		mem_[wCurObjYPos + 0] = mem_[hYPosLo];
		mem_[wCurObjYPos + 1] = mem_[hYPosHi];
		state() = OBJSTATE_WALK;
		set_frameset(ADDR_Frameset_683fb);
		state_duration() = 160;
		set_interaction(OBJ_INTERACTION_00);
		return;
	case WALKING: // a while on the floor, then gone
		if (mem_[wGroundShakeCounter] >= 0x10) {
			state() = static_cast<uint8_t>((OBJSTATE_WOBBLE_LEFT_START & ~1) | (mem_[rDIV] & 1));
			return;
		}
		if (--state_duration())
			return;
		[[fallthrough]];
	case STATE_0F:
		no_actions();
		state() = OBJSTATE_3F;
		set_frameset(ADDR_Frameset_68430);
		state_duration() = 35;
		flags() &= ~(1 << OBJFLAG_GRABBED_F);
		[[fallthrough]];
	case STATE_3F:
		no_actions();
		if (!--state_duration())
			despawn();
		return;
	case WOBBLE_LEFT: wobble(OBJSTATE_WOBBLE_LEFT); return;
	case WOBBLE_RIGHT: wobble(OBJSTATE_WOBBLE_RIGHT); return;
	case STUN_LEFT_START:
		state() = OBJSTATE_STUN_LEFT;
		set_frameset(ADDR_Frameset_683fb);
		[[fallthrough]];
	case STUN_LEFT:
		stun(ADDR_Func_32ae);
		return;
	case STUN_RIGHT_START:
		state() = OBJSTATE_STUN_RIGHT;
		set_frameset(ADDR_Frameset_683fb);
		[[fallthrough]];
	case STUN_RIGHT:
		stun(ADDR_Func_32bd);
		return;
	case STATE_14:
	case STATE_15:
		state() = handler == STATE_14 ? OBJSTATE_44 : OBJSTATE_45;
		var3() = 0;
		var2() = 0x02;
		return;
	case PICKED_UP_LEFT: picked_up(OBJSTATE_PICKED_UP_LEFT, 0xc0, ADDR_Func_3335); return;
	case PICKED_UP_RIGHT: picked_up(OBJSTATE_PICKED_UP_RIGHT, 0xe0, ADDR_Func_3344); return;
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
		return;
	case THROW_LEFT: throw_(OBJSTATE_FULL_THROW_LEFT, ADDR_Frameset_68421, ADDR_ObjState_FullThrowLeft); return;
	case THROW_RIGHT: throw_(OBJSTATE_FULL_THROW_RIGHT, ADDR_Frameset_68418, ADDR_ObjState_FullThrowRight); return;
	case STATE_28: knocked(ADDR_Frameset_6842d, ADDR_Func_33f8); return;
	case STATE_29: knocked(ADDR_Frameset_6842a, ADDR_Func_3407); return;
	case ATTACKED_RIGHT: knocked(ADDR_Frameset_6842d, ADDR_Func_3317); return;
	case ATTACKED_LEFT: knocked(ADDR_Frameset_6842a, ADDR_Func_3326); return;
	default: // a shared object state
		routines_.call(handler);
		return;
	}
}

// waits, then falls: towards Wario if he's off to a side
void Hebarii::spike()
{
	flags() |= OBJFLAG_NO_COLLISION;
	if (--state_duration())
		return;
	state_duration() = 100;
	uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
	if (dx < 0x18 || dx >= 0xe8) {
		set_update(SPIKE_DOWN);
		return;
	}
	set_update(SPIKE_LATERAL);
	if (dx >= 0x80)
		substate() |= OBJSUBFLAG_HDIR;
	else
		substate() &= ~OBJSUBFLAG_HDIR;
}

void Hebarii::spike_fall(bool lateral)
{
	if (lateral)
		routines_.call(facing_right() ? ADDR_MoveObjectRight_Slow : ADDR_MoveObjectLeft_Slow);
	routines_.call(ADDR_MoveObjectDown_Slow);
	if (--state_duration() && !state()) {
		mem_[hYPosLo] = mem_[wCurObjYPos + 0];
		mem_[hYPosHi] = mem_[wCurObjYPos + 1];
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		routines_.call(ADDR_Func_352b);
		if (!gb_->cpu.a)
			return;
		mem_[wCurObjYPos + 0] = mem_[hYPosLo];
		mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	}
	set_update(SPIKE_GONE);
	set_frameset(ADDR_Frameset_6840d);
	state_duration() = 0x18;
}

void Hebarii::spike_gone()
{
	no_actions();
	if (!--state_duration())
		flags() = 0;
}

} // namespace wl3::level

using Hebarii = wl3::level::Hebarii;

PORT_EX(HebariiFunc, 0, ALL_REGISTERS) { Hebarii(gb).start(); }
PORT_LOCAL_EX(HebariiFunc, Init, 0, ALL_REGISTERS) { Hebarii(gb).init(); }
PORT_LOCAL_EX(HebariiFunc, AttachedUpdate, 0, ALL_REGISTERS) { Hebarii(gb).attached(); }
PORT_LOCAL_EX(HebariiFunc, DetachedUpdate, 0, ALL_REGISTERS) { Hebarii(gb).detached(); }
PORT_LOCAL_EX(HebariiFunc, Vanish, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::VANISH); }
PORT_LOCAL_EX(HebariiFunc, FallFromCeiling, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::FALL); }
PORT_LOCAL_EX(HebariiFunc, Func_4a224, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::WALKING); }
PORT_LOCAL_EX(HebariiFunc, State0f, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::STATE_0F); }
PORT_LOCAL_EX(HebariiFunc, State3f, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::STATE_3F); }
PORT_LOCAL_EX(HebariiFunc, Func_4a26d, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::WOBBLE_LEFT); }
PORT_LOCAL_EX(HebariiFunc, Func_4a277, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::WOBBLE_RIGHT); }
PORT_LOCAL_EX(HebariiFunc, StunLeftStart, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::STUN_LEFT_START); }
PORT_LOCAL_EX(HebariiFunc, StunLeft, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::STUN_LEFT); }
PORT_LOCAL_EX(HebariiFunc, StunRightStart, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::STUN_RIGHT_START); }
PORT_LOCAL_EX(HebariiFunc, StunRight, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::STUN_RIGHT); }
PORT_LOCAL_EX(HebariiFunc, State14, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::STATE_14); }
PORT_LOCAL_EX(HebariiFunc, State15, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::STATE_15); }
PORT_LOCAL_EX(HebariiFunc, PickedUpLeftStart, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::PICKED_UP_LEFT); }
PORT_LOCAL_EX(HebariiFunc, PickedUpRightStart, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::PICKED_UP_RIGHT); }
PORT_LOCAL_EX(HebariiFunc, State19, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::STATE_19); }
PORT_LOCAL_EX(HebariiFunc, State49, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::STATE_49); }
PORT_LOCAL_EX(HebariiFunc, State18, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::STATE_18); }
PORT_LOCAL_EX(HebariiFunc, State48, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::STATE_48); }
PORT_LOCAL_EX(HebariiFunc, State12, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::STATE_12); }
PORT_LOCAL_EX(HebariiFunc, FullThrowLeftStart, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::THROW_LEFT); }
PORT_LOCAL_EX(HebariiFunc, FullThrowRightStart, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::THROW_RIGHT); }
PORT_LOCAL_EX(HebariiFunc, State28, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::STATE_28); }
PORT_LOCAL_EX(HebariiFunc, State29, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::STATE_29); }
PORT_LOCAL_EX(HebariiFunc, AttackedRightStart, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::ATTACKED_RIGHT); }
PORT_LOCAL_EX(HebariiFunc, AttackedLeftStart, 0, ALL_REGISTERS) { Hebarii(gb).run(wl3::level::ATTACKED_LEFT); }
PORT_EX(HebariiProjectileFunc, 0, ALL_REGISTERS) { Hebarii(gb).spike(); }
PORT_LOCAL_EX(HebariiProjectileFunc, MoveWithLateral, 0, ALL_REGISTERS) { Hebarii(gb).spike_fall(true); }
PORT_LOCAL_EX(HebariiProjectileFunc, MoveWithoutLateral, 0, ALL_REGISTERS) { Hebarii(gb).spike_fall(false); }
PORT_LOCAL_EX(HebariiProjectileFunc, Destroy, 0, ALL_REGISTERS) { Hebarii(gb).spike_gone(); }
