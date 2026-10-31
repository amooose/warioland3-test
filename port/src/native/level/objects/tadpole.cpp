// The tadpoles (tadpole.asm): a spawner releases one when Wario's to its
// left and a bit below; it shows itself, leaps out to the left and falls
// until it splashes into water. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t SPAWNER_WAIT = ADDR_TadpoleSpawnerFunc + 0x17; // .Func_49e4b
constexpr uint16_t SPAWNER_DONE = ADDR_TadpoleSpawnerFunc + 0x4b; // .Func_49e7f
constexpr uint16_t HIDDEN = ADDR_TadpoleFunc + 0x13; // .Func_49eb3
constexpr uint16_t READY = ADDR_TadpoleFunc + 0x2e; // .Func_49ece
constexpr uint16_t LEAPING = ADDR_TadpoleFunc + 0x94; // .Func_49f34
constexpr uint16_t SPLASHED = ADDR_TadpoleFunc + 0x115; // .Func_49fb5

class Tadpole : public Object {
public:
	using Object::Object;

	void spawner(); // TadpoleSpawnerFunc
	void spawner_wait(); // .Func_49e4b
	void spawner_done(); // .Func_49e7f
	void init(); // TadpoleFunc
	void hidden(); // .Func_49eb3
	void ready(); // .Func_49ece
	void leaping(); // .Func_49f34
	void splashed(); // .Func_49fb5

private:
	void knocked(uint16_t routine); // .asm_49f1c
	void hold() { mem_[wCurObjAction] = 0x81; }
};

} // namespace

void Tadpole::spawner()
{
	set_update(SPAWNER_WAIT);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	state_duration() = 180;
	flags() |= OBJFLAG_NO_COLLISION;
}

void Tadpole::spawner_wait()
{
	hold();
	frame_duration() = 2;
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	if (static_cast<uint8_t>(screen_x() + 0x2a) < wario)
		return;
	uint8_t below = static_cast<uint8_t>(screen_y() - mem_[wWarioScreenYPos]);
	if (below >= 0x50 || below < 0x28)
		return;
	set_update(SPAWNER_DONE);
	SET_BC(gb_, ADDR_ObjParams_Tadpole);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

void Tadpole::spawner_done()
{
	hold();
	frame_duration() = 2;
	if (!--state_duration())
		despawn();
}

void Tadpole::init()
{
	hold();
	set_update(HIDDEN);
	flags() |= OBJFLAG_INVISIBLE;
}

void Tadpole::hidden()
{
	hold();
	if (!(flags() & OBJFLAG_ON_SCREEN))
		return;
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(READY);
	set_frameset(ADDR_Frameset_69a33);
}

void Tadpole::ready()
{
	hold();
	if (state_duration()) {
		if (!--state_duration())
			set_frameset(ADDR_Frameset_69a3d);
		return;
	}
	set_frameset(ADDR_Frameset_69a1a);
	state_duration() = 0x20;
	var1() = 0;
	set_update(LEAPING);
	play_sfx(SFX_07F);
}

void Tadpole::knocked(uint16_t routine)
{
	flags() |= OBJFLAG_NO_COLLISION;
	set_update(routine);
	var3() = 0;
	var2() = 0x02;
	hold();
	set_frameset(ADDR_Frameset_69a30);
}

// up and left for 32 frames, then falling and left, until in water
void Tadpole::leaping()
{
	uint8_t s = state();
	switch (s) {
	case 0: break;
	case 0x0b: // .asm_49efe
		set_update(ADDR_Func_3407);
		var3() = 0;
		var2() = 0x01;
		return;
	case 0x08: case 0x29: case 0x04: knocked(ADDR_Func_3407); return;
	case 0x09: case 0x28: case 0x05: knocked(ADDR_Func_33f8); return;
	case 0x0a: vanish(); return;
	default: state() = 0; break;
	}
	if (!var1()) {
		if (state_duration()) {
			--state_duration();
			SET_BC(gb_, ADDR_Data_60a20);
			routines_.call(ADDR_ApplyObjYMovement);
			routines_.call(ADDR_MoveObjectLeft_Fast);
			return;
		}
		var3() = 0;
		var1() = 1;
		set_frameset(ADDR_Frameset_69a1d);
		return;
	}
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_352b);
	if (!mem_[wIsInWaterOrSand]) {
		SET_BC(gb_, ADDR_Data_60a40);
		routines_.call(ADDR_ApplyObjYMovement);
		routines_.call(ADDR_MoveObjectLeft_Fast);
		return;
	}
	set_frameset(ADDR_Frameset_69a26);
	state_duration() = 0x09;
	set_update(SPLASHED);
}

void Tadpole::splashed()
{
	hold();
	if (!--state_duration())
		flags() = 0;
}

} // namespace wl3::level

using Tadpole = wl3::level::Tadpole;

PORT_EX(TadpoleSpawnerFunc, 0, ALL_REGISTERS) { Tadpole(gb).spawner(); }
PORT_LOCAL_EX(TadpoleSpawnerFunc, Func_49e4b, 0, ALL_REGISTERS) { Tadpole(gb).spawner_wait(); }
PORT_LOCAL_EX(TadpoleSpawnerFunc, Func_49e7f, 0, ALL_REGISTERS) { Tadpole(gb).spawner_done(); }
PORT_EX(TadpoleFunc, 0, ALL_REGISTERS) { Tadpole(gb).init(); }
PORT_LOCAL_EX(TadpoleFunc, Func_49eb3, 0, ALL_REGISTERS) { Tadpole(gb).hidden(); }
PORT_LOCAL_EX(TadpoleFunc, Func_49ece, 0, ALL_REGISTERS) { Tadpole(gb).ready(); }
PORT_LOCAL_EX(TadpoleFunc, Func_49f34, 0, ALL_REGISTERS) { Tadpole(gb).leaping(); }
PORT_LOCAL_EX(TadpoleFunc, Func_49fb5, 0, ALL_REGISTERS) { Tadpole(gb).splashed(); }
