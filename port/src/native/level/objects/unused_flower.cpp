// The unused flower (unused_flower.asm): grows, waits for its moment of
// the 256-frame cycle and spits a seed towards Wario, which falls, rolls
// along the floor and hops off its edges. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t GROW = ADDR_UnusedFlowerFunc + 0x21; // .Grow
constexpr uint16_t IDLE = ADDR_UnusedFlowerFunc + 0x4b; // .Idle
constexpr uint16_t PREPARING = ADDR_UnusedFlowerFunc + 0x7a; // .PreparingAttack
constexpr uint16_t ATTACK = ADDR_UnusedFlowerFunc + 0xaf; // .Attack
constexpr uint16_t DESTROY = ADDR_UnusedFlowerProjectileCommonFunc + 0x85; // .Destroy

class UnusedFlower : public Object {
public:
	using Object::Object;

	void init(); // UnusedFlowerFunc
	void grow(); // .Grow
	void idle(); // .Idle
	void preparing(); // .PreparingAttack
	void attack(); // .Attack
	// UnusedFlowerProjectileCommonFunc; page = h, where .Roll finds the position
	void projectile(uint16_t page);
	void destroy(); // .Destroy

private:
	bool waited(); // the state duration down, and the cycle back at var1
	void set_destroy();
	void pos_to_hram(uint16_t pos, uint8_t dy);
};

} // namespace

void UnusedFlower::init()
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	set_update(GROW);
	set_frameset(ADDR_Frameset_6837a);
	state_duration() = 30;
	var1() = mem_[wGlobalCounter];
}

// once on screen
void UnusedFlower::grow()
{
	no_actions();
	frame_duration() = 2;
	if ((flags() & (OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN)) != (OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN))
		return;
	if (--state_duration())
		return;
	set_update(IDLE);
	set_frameset(ADDR_Frameset_6837a);
	state_duration() = 49;
}

bool UnusedFlower::waited()
{
	no_actions();
	if (state_duration() && --state_duration())
		return false;
	frame_duration() = 2;
	return var1() == mem_[wGlobalCounter];
}

void UnusedFlower::idle()
{
	if (!waited())
		return;
	set_update(PREPARING);
	set_frameset(ADDR_Frameset_683ac);
	state_duration() = 68;
}

void UnusedFlower::preparing()
{
	no_actions();
	if (--state_duration())
		return;
	set_update(ATTACK);
	set_frameset(ADDR_Frameset_683b1);
	state_duration() = 35;
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	bool right = static_cast<uint8_t>(screen_x() + 0x2a) < wario;
	SET_BC(gb_, right ? ADDR_ObjParams_UnusedFlowerProjectileRight : ADDR_ObjParams_UnusedFlowerProjectileLeft);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

void UnusedFlower::attack()
{
	if (waited())
		set_update(ADDR_UnusedFlowerFunc); // .Init
}

void UnusedFlower::set_destroy()
{
	set_update(DESTROY);
	set_frameset(ADDR_Frameset_68389);
	state_duration() = 8;
}

void UnusedFlower::pos_to_hram(uint16_t pos, uint8_t dy)
{
	uint16_t y_pos = static_cast<uint16_t>(mem_.word(pos) - dy);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = mem_[static_cast<uint16_t>(pos + 2)];
	mem_[hXPosHi] = mem_[static_cast<uint16_t>(pos + 3)];
}

// thrown (state 0) and falling, rolling (1) until it reaches a wall
// (destroyed) or an edge, falling again (2) from there; any other state
// (hit) destroys it
void UnusedFlower::projectile(uint16_t page)
{
	uint8_t s = state();
	if (s == OBJSTATE_SPECIAL_1) { // .Roll
		pos_to_hram(static_cast<uint16_t>(page | OBJ_Y_POS), 4);
		routines_.call(ADDR_Func_3513);
		if (gb_->cpu.a) {
			set_destroy();
			return;
		}
		pos_to_hram(wCurObjYPos, 0);
		routines_.call(ADDR_Func_3543);
		if (gb_->cpu.a)
			return;
		state() = OBJSTATE_SPECIAL_2;
		var3() = 0;
		return;
	}
	if (s && s != OBJSTATE_SPECIAL_2) {
		set_destroy();
		return;
	}
	SET_BC(gb_, s ? ADDR_Data_60320 + 8 : ADDR_Data_60320); // .Jump, .Thrown
	routines_.call(ADDR_ApplyObjYMovement);
	pos_to_hram(wCurObjYPos, 0);
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a)
		return;
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	state() = OBJSTATE_SPECIAL_1;
}

void UnusedFlower::destroy()
{
	no_actions();
	if (!--state_duration())
		flags() = 0;
}

} // namespace wl3::level

using UnusedFlower = wl3::level::UnusedFlower;

PORT_EX(UnusedFlowerFunc, 0, ALL_REGISTERS) { UnusedFlower(gb).init(); }
PORT_LOCAL_EX(UnusedFlowerFunc, Grow, 0, ALL_REGISTERS) { UnusedFlower(gb).grow(); }
PORT_LOCAL_EX(UnusedFlowerFunc, Idle, 0, ALL_REGISTERS) { UnusedFlower(gb).idle(); }
PORT_LOCAL_EX(UnusedFlowerFunc, PreparingAttack, 0, ALL_REGISTERS) { UnusedFlower(gb).preparing(); }
PORT_LOCAL_EX(UnusedFlowerFunc, Attack, 0, ALL_REGISTERS) { UnusedFlower(gb).attack(); }
// MoveObjectRight / Left leave h at wCurObj's page
PORT_EX(UnusedFlowerProjectileRightFunc, 0, ALL_REGISTERS)
{
	gb_call(gb, ADDR_MoveObjectRight);
	UnusedFlower(gb).projectile(wCurObjXPos & 0xff00);
}
PORT_EX(UnusedFlowerProjectileLeftFunc, 0, ALL_REGISTERS)
{
	gb_call(gb, ADDR_MoveObjectLeft);
	UnusedFlower(gb).projectile(wCurObjXPos & 0xff00);
}
PORT_EX(UnusedFlowerProjectileCommonFunc, 0, ALL_REGISTERS) { UnusedFlower(gb).projectile(gb->cpu.h << 8); }
PORT_LOCAL_EX(UnusedFlowerProjectileCommonFunc, Destroy, 0, ALL_REGISTERS) { UnusedFlower(gb).destroy(); }
