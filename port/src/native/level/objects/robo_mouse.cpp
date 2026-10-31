// The robo mouse (robo_mouse.asm): a wind-up mouse that walks along the
// floor, climbing small steps and breaking the robo mouse blocks in its
// way; hit, it's thrown (and breaks blocks where it lands), and turns to
// Wario before setting off again. Its state picks a handler from a table
// of 95, many of them the shared object states (home routines that load
// their own registers). See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_RoboMouseFunc + offset); }

constexpr uint16_t APPEAR = L(0x1b); // .Func_488f7
constexpr uint16_t UPDATE = L(0x4e); // .Func_4892a
constexpr uint16_t STATE_5A = L(0x10c); // .Func_489e8
constexpr uint16_t NOTHING = L(0x122); // .Func_489fe
constexpr uint16_t THROWN_54 = L(0x123); // .Func_489ff
constexpr uint16_t THROWN_3218 = L(0x129); // .Func_48a05
constexpr uint16_t THROWN_55 = L(0x12f); // .Func_48a0b
constexpr uint16_t THROWN_3236 = L(0x135); // .Func_48a11
constexpr uint16_t THROWN_52 = L(0x13b); // .Func_48a17
constexpr uint16_t THROWN_31DC = L(0x141); // .Func_48a1d
constexpr uint16_t THROWN_53 = L(0x147); // .Func_48a23
constexpr uint16_t THROWN_31FA = L(0x14d); // .Func_48a29
constexpr uint16_t THROWN_56 = L(0x153); // .Func_48a2f
constexpr uint16_t THROWN_32CC = L(0x159); // .Func_48a35
constexpr uint16_t THROWN_57 = L(0x15f); // .Func_48a3b
constexpr uint16_t THROWN_32DB = L(0x165); // .Func_48a41
constexpr uint16_t POUNDED = L(0x16b); // .Func_48a47
constexpr uint16_t SINKING = L(0x20d); // .Func_48ae9
constexpr uint16_t RESET = L(0x237); // .Func_48b13
constexpr uint16_t STATE_41 = L(0x245); // .Func_48b21
constexpr uint16_t START_WALK = L(0x24f); // .Func_48b2b
constexpr uint16_t WALK = L(0x254); // .Func_48b30
constexpr uint16_t STUNNED = L(0x371); // .Func_48c4d
constexpr uint16_t RECOVER = L(0x384); // .Func_48c60
constexpr uint16_t WAKE_RIGHT = L(0x3ea); // .Func_48cc6
constexpr uint16_t WAKE_LEFT = L(0x3fb); // .Func_48cd7

// .Func_4892a's jumptable, by state
constexpr uint16_t HANDLERS[] = {
	RESET, START_WALK, RESET, RESET, RESET, RESET, RESET, RESET, // $00
	RESET, RESET, POUNDED, RESET, RESET, RESET, RESET, RESET,
	RESET, STATE_41, RESET, STUNNED, ADDR_Func_3245, ADDR_Func_3254, STUNNED, STUNNED, // $10
	RESET, RESET, RESET, RESET, RESET, RESET, RESET, RESET,
	ADDR_Func_3191, ADDR_Func_31af, THROWN_52, THROWN_53, THROWN_54, THROWN_55, THROWN_56, THROWN_57, // $20
	RESET, RESET, STATE_5A, RESET, RESET, WAKE_RIGHT, WAKE_LEFT, RESET,
	ADDR_ObjState_Fall, WALK, RESET, RESET, RESET, RESET, RESET, RESET, // $30
	RESET, RESET, SINKING, RESET, RESET, RESET, RESET, RESET,
	RESET, ADDR_ObjState_StandingFall, RESET, RECOVER, RESET, RESET, RESET, RESET, // $40
	RESET, RESET, RESET, RESET, RESET, RESET, RESET, RESET,
	ADDR_Func_31a0, ADDR_Func_31be, THROWN_31DC, THROWN_31FA, THROWN_3218, THROWN_3236, THROWN_32CC, THROWN_32DB, // $50
	RESET, RESET, NOTHING, RESET, RESET,
};
constexpr uint8_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

class RoboMouse : public Object {
public:
	using Object::Object;

	void init(); // RoboMouseFunc
	void appear(); // .Func_488f7
	void update(); // .Func_4892a
	void run(uint16_t handler);
	void break_blocks(bool right, uint8_t dx); // .Func_48cec (left, $e) and the like

private:
	bool wario_to_right() const;
	void thrown(bool right, uint16_t routine);
	void pounded();
	void walk();
	bool walk_floor(bool right); // whether it went on
	void recover(); // .Func_48c60
	void set_interaction_and_priority(); // .asm_48ca5
	void hpos_offset(int16_t dy, int16_t dx);
};

} // namespace

void RoboMouse::init()
{
	set_update(APPEAR);
	// the asm sets OBJFLAG_INVISIBLE_F in the substate, not the flags
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJFLAG_INVISIBLE_F);
	mem_[wCurObjCollBoxRight] = 0x0b;
	mem_[wCurObjCollBoxLeft] = 0xf4;
	mem_[wCurObjCollBoxBottom] = 0xfc;
}

bool RoboMouse::wario_to_right() const
{
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	return static_cast<uint8_t>(screen_x() + 0x2a) < wario;
}

// facing Wario
void RoboMouse::appear()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	bool right = wario_to_right();
	if (right)
		substate() |= OBJSUBFLAG_HDIR;
	else
		substate() &= ~OBJSUBFLAG_HDIR;
	var3() = 0;
	set_frameset(right ? ADDR_Frameset_691de : ADDR_Frameset_69220);
}

void RoboMouse::update()
{
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

void RoboMouse::hpos_offset(int16_t dy, int16_t dx)
{
	uint16_t y_pos = static_cast<uint16_t>(y() + dy);
	uint16_t x_pos = static_cast<uint16_t>(x() + dx);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
}

// the robo mouse blocks dx to its side, at 3 heights
void RoboMouse::break_blocks(bool right, uint8_t dx)
{
	uint16_t x_pos = right ? static_cast<uint16_t>(x() + dx) : static_cast<uint16_t>(x() - dx);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	for (uint8_t dy : {0x04, 0x14, 0x24}) {
		uint16_t y_pos = static_cast<uint16_t>(y() - dy);
		mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
		mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
		mem_[wBlockFuncSpecialCollision] = COLLISION_ROBO_MOUSE_BREAK;
		routines_.call(ADDR_Func_3513);
	}
}

void RoboMouse::thrown(bool right, uint16_t routine)
{
	break_blocks(right, right ? 0x0d : 0x0e);
	routines_.call(routine);
}

void RoboMouse::run(uint16_t handler)
{
	switch (handler) {
	case STATE_5A:
		state() = 0x5a;
		set_frameset(facing_right() ? ADDR_Frameset_691fb : ADDR_Frameset_69200);
		return;
	case NOTHING: return;
	case THROWN_54: thrown(false, ADDR_Func_3209); return;
	case THROWN_3218: thrown(false, ADDR_Func_3218); return;
	case THROWN_55: thrown(true, ADDR_Func_3227); return;
	case THROWN_3236: thrown(true, ADDR_Func_3236); return;
	case THROWN_52: thrown(false, ADDR_Func_31cd); return;
	case THROWN_31DC: thrown(false, ADDR_Func_31dc); return;
	case THROWN_53: thrown(true, ADDR_Func_31eb); return;
	case THROWN_31FA: thrown(true, ADDR_Func_31fa); return;
	case THROWN_56: thrown(false, ADDR_Func_3245); return;
	case THROWN_32CC: thrown(false, ADDR_Func_32cc); return;
	case THROWN_57: thrown(true, ADDR_Func_3254); return;
	case THROWN_32DB: thrown(true, ADDR_Func_32db); return;
	case POUNDED: pounded(); return;
	case SINKING:
		routines_.call(ADDR_MoveObjectDown_Slow);
		hpos_offset(0, 0);
		routines_.call(ADDR_Func_352b);
		if (gb_->cpu.a)
			state() = (interaction() & 0x7f) == OBJ_INTERACTION_0B ? 0x13 : 0x31;
		return;
	case RESET:
		state() = 0x30;
		var3() = 0;
		flags() &= ~OBJFLAG_NO_COLLISION;
		return;
	case STATE_41:
		state() = 0x41;
		var3() = 0;
		return;
	case START_WALK:
		state() = 0x31;
		walk();
		return;
	case WALK: walk(); return;
	case STUNNED:
		state() = 0x43;
		state_duration() = (flags() & OBJFLAG_ON_SCREEN) ? 0x0a : 0x01;
		recover();
		return;
	case RECOVER: recover(); return;
	case WAKE_RIGHT:
	case WAKE_LEFT:
		if (--state_duration())
			return;
		set_frameset(handler == WAKE_RIGHT ? ADDR_Frameset_691de : ADDR_Frameset_69220);
		state() = 0x31;
		return;
	default: // a shared object state
		routines_.call(handler);
		return;
	}
}

// ground pounded: knocked over and away from Wario (with stars) the
// first time; then, already knocked over, it's knocked into water
void RoboMouse::pounded()
{
	if (flags() & OBJFLAG_NO_COLLISION) {
		flags() &= ~(OBJFLAG_NO_COLLISION | OBJFLAG_PRIORITY);
		set_frameset(facing_right() ? ADDR_Frameset_69205 : ADDR_Frameset_6920c);
		state_duration() = 0x3c;
		set_interaction(OBJ_INTERACTION_0B);
		play_sfx(SFX_VANISH);
		SET_BC(gb_, ADDR_ObjParams_StarsAbove);
		routines_.call(ADDR_CreateObjectFromCurObjPos);
		run(wario_to_right() ? THROWN_54 : THROWN_55);
		return;
	}
	state() = 0x3a;
	if ((interaction() & 0x7f) != OBJ_INTERACTION_0B)
		set_frameset(facing_right() ? ADDR_Frameset_691de : ADDR_Frameset_69220);
	if (!(flags() & OBJFLAG_ON_SCREEN))
		return;
	hpos_offset(0, 0);
	gb_->cpu.b = PARTICLE_SPLASH;
	routines_.farcall(BANK_CreateParticle, ADDR_CreateParticle);
	play_sfx(SFX_083);
}

void RoboMouse::walk()
{
	bool right = facing_right();
	if (!walk_floor(right))
		return;
	var2() = 0x02;
	uint16_t frameset = right ? ADDR_Frameset_691e5 : ADDR_Frameset_69213;
	if (mem_[wCurObjFramesetPtr] != (frameset & 0xff))
		set_frameset(frameset);
}

// a pixel along, on the floor: off an edge it falls (state $11); a step
// up it climbs (state $22 / $23); a wall it breaks (if it's a robo mouse
// block) and climbs onto (state $24 / $25)
bool RoboMouse::walk_floor(bool right)
{
	right ? move_right() : move_left();
	hpos_offset(0, 0);
	routines_.call(ADDR_Func_3543);
	uint8_t floor = gb_->cpu.a;
	if (!floor) {
		state() = 0x11;
		set_frameset(right ? ADDR_Frameset_691de : ADDR_Frameset_69220);
		return false;
	}
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	if (floor == 0x10)
		return false;
	uint16_t x_pos = right ? static_cast<uint16_t>(x() + 7) : static_cast<uint16_t>(x() - 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
	routines_.call(ADDR_Func_3543);
	if (!gb_->cpu.a) {
		state() = right ? 0x23 : 0x22;
		return true;
	}
	break_blocks(right, right ? 0x0b : 0x0c);
	hpos_offset(-4, right ? 0x13 : -0x14);
	mem_[wBlockFuncSpecialCollision] = COLLISION_ROBO_MOUSE_FREE;
	routines_.call(ADDR_Func_3513);
	if (!(gb_->cpu.a & 0x0f))
		return false;
	y() = static_cast<uint16_t>(y() - 6);
	state() = right ? 0x25 : 0x24;
	return true;
}

// once its time's up, sets off towards Wario
void RoboMouse::recover()
{
	if (--state_duration()) {
		routines_.call(ADDR_Func_3308);
		return;
	}
	uint16_t wario_x = static_cast<uint16_t>(mem_[wWarioXPos + 0] << 8 | mem_[wWarioXPos + 1]);
	bool wario_left = wario_x < x();
	if (wario_left == !facing_right()) {
		set_interaction_and_priority();
		return;
	}
	if (wario_left) {
		state() = 0x2e;
		substate() &= ~OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_691f2);
	} else {
		state() = 0x2d;
		substate() |= OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_69227);
	}
	state_duration() = 0x31;
	set_interaction(OBJ_INTERACTION_ROBO_MOUSE);
}

// already facing Wario: walks on (righted, if it was knocked over)
void RoboMouse::set_interaction_and_priority()
{
	state() = 0x31;
	if ((interaction() & 0x7f) == OBJ_INTERACTION_0B) {
		set_frameset(facing_right() ? ADDR_Frameset_691de : ADDR_Frameset_69220);
		flags() |= OBJFLAG_PRIORITY;
	}
	set_interaction(OBJ_INTERACTION_ROBO_MOUSE);
}

} // namespace wl3::level

using RoboMouse = wl3::level::RoboMouse;

PORT_EX(RoboMouseFunc, 0, ALL_REGISTERS) { RoboMouse(gb).init(); }
PORT_LOCAL_EX(RoboMouseFunc, Func_488f7, 0, ALL_REGISTERS) { RoboMouse(gb).appear(); }
PORT_LOCAL_EX(RoboMouseFunc, Func_4892a, 0, ALL_REGISTERS) { RoboMouse(gb).update(); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48cec, 0, ALL_REGISTERS) { RoboMouse(gb).break_blocks(false, 0x0e); }
PORT_LOCAL_EX(RoboMouseFunc, Func_489e8, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::STATE_5A); }
PORT_LOCAL_EX(RoboMouseFunc, Func_489fe, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::NOTHING); }
PORT_LOCAL_EX(RoboMouseFunc, Func_489ff, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::THROWN_54); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48a05, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::THROWN_3218); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48a0b, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::THROWN_55); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48a11, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::THROWN_3236); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48a17, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::THROWN_52); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48a1d, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::THROWN_31DC); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48a23, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::THROWN_53); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48a29, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::THROWN_31FA); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48a2f, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::THROWN_56); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48a35, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::THROWN_32CC); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48a3b, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::THROWN_57); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48a41, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::THROWN_32DB); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48a47, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::POUNDED); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48ae9, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::SINKING); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48b13, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::RESET); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48b21, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::STATE_41); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48b2b, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::START_WALK); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48b30, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::WALK); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48c4d, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::STUNNED); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48c60, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::RECOVER); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48cc6, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::WAKE_RIGHT); }
PORT_LOCAL_EX(RoboMouseFunc, Func_48cd7, 0, ALL_REGISTERS) { RoboMouse(gb).run(wl3::level::WAKE_LEFT); }
