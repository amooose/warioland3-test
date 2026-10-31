// The falling snow (falling_snow.asm): a spawner on a ledge that every so
// often shows a flurry and drops a clump of snow, which lands and breaks
// up (or melts away once hit). See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t SPAWNER_WAIT = ADDR_FallingSnowSpawnerFunc + 0x17; // .Func_49544
constexpr uint16_t SPAWNER_DROPPED = ADDR_FallingSnowSpawnerFunc + 0x39; // .Func_49566
constexpr uint16_t SPAWNER_FLURRY = ADDR_FallingSnowSpawnerFunc + 0x70; // .Func_4959d
constexpr uint16_t SNOW_FORMING = ADDR_FallingSnowFunc + 0x2d; // .Func_495ef
constexpr uint16_t SNOW_FALLING = ADDR_FallingSnowFunc + 0x4e; // .Func_49610
constexpr uint16_t SNOW_HIT = ADDR_FallingSnowFunc + 0xb3; // .Func_49675
constexpr uint16_t SNOW_GONE = ADDR_FallingSnowFunc + 0xec; // .Func_496ae

class FallingSnow : public Object {
public:
	using Object::Object;

	void spawner(); // FallingSnowSpawnerFunc
	void spawner_wait(); // .Func_49544
	void spawner_dropped(); // .Func_49566
	void spawner_flurry(); // .Func_4959d
	void snow(); // FallingSnowFunc
	void forming(); // .Func_495ef
	void falling(); // .Func_49610
	void hit(); // .Func_49675
	void gone(); // .Func_496ae

private:
	bool landed(); // Func_352b at its position; else it falls on
	void sfx_on_screen(uint16_t sfx);
};

} // namespace

void FallingSnow::spawner()
{
	set_update(SPAWNER_WAIT);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	flags() |= OBJFLAG_NO_COLLISION;
	state_duration() = 0x1e;
}

void FallingSnow::spawner_wait()
{
	no_actions();
	frame_duration() = 2;
	if (--state_duration())
		return;
	state_duration() = 0x56;
	var1() = 0;
	set_update(SPAWNER_DROPPED);
	SET_BC(gb_, ADDR_ObjParams_FallingSnow);
	routines_.call(ADDR_CreateObjectFromCurObjPos);
}

// a flurry above, randomly to the left
void FallingSnow::spawner_dropped()
{
	no_actions();
	frame_duration() = 2;
	if (--state_duration())
		return;
	var2() = 0x0a;
	move_up_by_var2();
	set_update(SPAWNER_FLURRY);
	flags() &= ~OBJFLAG_INVISIBLE;
	set_frameset(ADDR_Frameset_695c0);
	state_duration() = 0x1d;
	if (!(mem_[rDIV] & 1))
		return;
	var1() = 0x01;
	move_left_by_var2();
}

// then back where it was, unseen
void FallingSnow::spawner_flurry()
{
	no_actions();
	if (--state_duration())
		return;
	state_duration() = 0x3c;
	if (var1())
		move_right_by_var2();
	move_down_by_var2();
	flags() |= OBJFLAG_INVISIBLE;
	set_update(SPAWNER_WAIT);
}

void FallingSnow::sfx_on_screen(uint16_t sfx)
{
	if (flags() & OBJFLAG_ON_SCREEN)
		play_sfx(sfx);
}

void FallingSnow::snow()
{
	no_actions();
	flags() |= OBJFLAG_NO_COLLISION;
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_6958f);
	state_duration() = 0x20;
	set_update(SNOW_FORMING);
	sfx_on_screen(SFX_080);
}

void FallingSnow::forming()
{
	no_actions();
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_695a6);
	set_update(SNOW_FALLING);
	y() = static_cast<uint16_t>(y() + 8);
}

bool FallingSnow::landed()
{
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a) {
		SET_BC(gb_, ADDR_Data_603e0);
		routines_.call(ADDR_ApplyObjYMovement);
		return false;
	}
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	return true;
}

// until it lands and breaks; hit by Wario (state $18) it falls on
// harmlessly, other hits break it at once
void FallingSnow::falling()
{
	uint8_t s = state();
	if (s == 0x18) {
		set_update(SNOW_HIT);
		set_frameset(ADDR_Frameset_695d6);
		return;
	}
	if (!s) {
		if (!landed())
			return;
		set_frameset(ADDR_Frameset_695af);
		state_duration() = 0x2d;
	} else {
		set_frameset(ADDR_Frameset_695cb);
		state_duration() = 0x11;
	}
	set_update(SNOW_GONE);
	sfx_on_screen(SFX_0A1);
}

void FallingSnow::hit()
{
	no_actions();
	if (!landed())
		return;
	set_frameset(ADDR_Frameset_695d9);
	state_duration() = 0x07;
	set_update(SNOW_GONE);
}

void FallingSnow::gone()
{
	no_actions();
	if (!--state_duration())
		flags() = 0;
}

} // namespace wl3::level

using FallingSnow = wl3::level::FallingSnow;

PORT_EX(FallingSnowSpawnerFunc, 0, ALL_REGISTERS) { FallingSnow(gb).spawner(); }
PORT_LOCAL_EX(FallingSnowSpawnerFunc, Func_49544, 0, ALL_REGISTERS) { FallingSnow(gb).spawner_wait(); }
PORT_LOCAL_EX(FallingSnowSpawnerFunc, Func_49566, 0, ALL_REGISTERS) { FallingSnow(gb).spawner_dropped(); }
PORT_LOCAL_EX(FallingSnowSpawnerFunc, Func_4959d, 0, ALL_REGISTERS) { FallingSnow(gb).spawner_flurry(); }
PORT_EX(FallingSnowFunc, 0, ALL_REGISTERS) { FallingSnow(gb).snow(); }
PORT_LOCAL_EX(FallingSnowFunc, Func_495ef, 0, ALL_REGISTERS) { FallingSnow(gb).forming(); }
PORT_LOCAL_EX(FallingSnowFunc, Func_49610, 0, ALL_REGISTERS) { FallingSnow(gb).falling(); }
PORT_LOCAL_EX(FallingSnowFunc, Func_49675, 0, ALL_REGISTERS) { FallingSnow(gb).hit(); }
PORT_LOCAL_EX(FallingSnowFunc, Func_496ae, 0, ALL_REGISTERS) { FallingSnow(gb).gone(); }
