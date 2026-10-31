// The big leaves (big_leaf.asm): a spawner drops leaves while Wario's to
// its left; each drifts down as a platform (pushed aside when hit, taking
// Wario down with it) until it reaches water. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t SPAWN_SET_1 = ADDR_BigLeafSpawnerFunc + 0x17; // .SpawnSet1
constexpr uint16_t SPAWN_SET_2 = ADDR_BigLeafSpawnerFunc + 0x5b; // .SpawnSet2
constexpr uint16_t FALLING = ADDR_BigLeafFunc + 0x22; // .Update
constexpr uint16_t SINKING = ADDR_BigLeafFunc + 0xc2; // .Func_42d0e

class BigLeaf : public Object {
public:
	using Object::Object;

	void spawner(); // BigLeafSpawnerFunc
	void spawn_set_1(); // .SpawnSet1
	void spawn_set_2(); // .SpawnSet2
	void leaf_1(); // BigLeaf1Func
	void leaf(); // BigLeafFunc, BigLeaf2Func
	void falling(); // .Update
	void sinking(); // .Func_42d0e

private:
	void spawn(uint16_t params);
	bool pos_in_water(); // .asm_42ccd
};

} // namespace

void BigLeaf::spawner()
{
	set_update(SPAWN_SET_1);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	state_duration() = 20;
	flags() |= OBJFLAG_NO_COLLISION;
}

void BigLeaf::spawn(uint16_t params)
{
	SET_BC(gb_, params);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

// 2 leaves, once Wario's left of x + 22 and the time's up
void BigLeaf::spawn_set_1()
{
	no_actions();
	frame_duration() = 2;
	uint16_t limit = static_cast<uint16_t>(x() + 22);
	uint16_t wario_x = static_cast<uint16_t>(mem_[wWarioXPos + 0] << 8 | mem_[wWarioXPos + 1]);
	if (wario_x >= limit)
		return;
	if (state_duration()) {
		--state_duration();
		return;
	}
	set_update(SPAWN_SET_2);
	state_duration() = 160;
	spawn(ADDR_ObjParams_BigLeaf1);
	spawn(ADDR_ObjParams_BigLeaf3);
}

// then 1
void BigLeaf::spawn_set_2()
{
	no_actions();
	frame_duration() = 2;
	if (--state_duration())
		return;
	set_update(SPAWN_SET_1);
	state_duration() = 80;
	spawn(ADDR_ObjParams_BigLeaf2);
}

// unseen until its time comes
void BigLeaf::leaf_1()
{
	no_actions();
	flags() |= OBJFLAG_INVISIBLE | OBJFLAG_NO_COLLISION;
	if (--state_duration())
		return;
	state_duration() = 18;
	set_update(ADDR_BigLeafFunc);
	flags() &= ~OBJFLAG_INVISIBLE;
	set_frameset(ADDR_Frameset_68732);
}

void BigLeaf::leaf()
{
	no_actions();
	flags() |= OBJFLAG_NO_COLLISION;
	if (--state_duration())
		return;
	set_update(FALLING);
	set_frameset(ADDR_Frameset_68729);
}

// a pixel down every other frame
void BigLeaf::falling()
{
	one_way_platform();
	uint8_t s = state();
	if (s == 0x0a) {
		routines_.call(ADDR_VanishObject2);
		return;
	}
	if (s && s < 0x0b) { // .destroy
		flags() = 0;
		return;
	}
	if (s) { // hit: drifts away from Wario from now on
		uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
		state_duration() = static_cast<uint8_t>(screen_x() + 0x2a) < wario ? 2 : 1;
		state() = 0;
	}
	uint8_t drift = state_duration();
	if (drift == 1)
		move_right();
	else if (drift)
		move_left();
	bool odd = mem_[wGlobalCounter] & 1;
	if (flags() & OBJFLAG_STEPPED) {
		flags() &= ~OBJFLAG_STEPPED;
		if (!odd)
			return;
		gb_->cpu.b = 0x01;
		routines_.call(ADDR_MoveWarioDown);
		routines_.call(ADDR_HandleDownwardsFloorTransition);
	} else if (!odd) {
		return;
	}
	move_down();
	if (!pos_in_water())
		return;
	no_actions();
	set_update(SINKING);
	set_frameset(ADDR_Frameset_6873b);
	state_duration() = 0x14;
	if (screen_y() >= 0xb8 || screen_x() >= 0xb8)
		return;
	play_sfx(SFX_08F);
}

bool BigLeaf::pos_in_water()
{
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_352b);
	return mem_[wIsInWaterOrSand];
}

void BigLeaf::sinking()
{
	no_actions();
	if (!--state_duration())
		flags() = 0;
}

} // namespace wl3::level

using BigLeaf = wl3::level::BigLeaf;

PORT_EX(BigLeafSpawnerFunc, 0, ALL_REGISTERS) { BigLeaf(gb).spawner(); }
PORT_LOCAL_EX(BigLeafSpawnerFunc, SpawnSet1, 0, ALL_REGISTERS) { BigLeaf(gb).spawn_set_1(); }
PORT_LOCAL_EX(BigLeafSpawnerFunc, SpawnSet2, 0, ALL_REGISTERS) { BigLeaf(gb).spawn_set_2(); }
PORT_EX(BigLeaf1Func, 0, ALL_REGISTERS) { BigLeaf(gb).leaf_1(); }
PORT_EX(BigLeaf2Func, 0, ALL_REGISTERS) { BigLeaf(gb).leaf(); }
PORT_LOCAL_EX(BigLeaf2Func, Update, 0, ALL_REGISTERS) { BigLeaf(gb).falling(); }
PORT_LOCAL_EX(BigLeaf2Func, Func_42d0e, 0, ALL_REGISTERS) { BigLeaf(gb).sinking(); }
