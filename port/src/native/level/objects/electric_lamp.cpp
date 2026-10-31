// The electric lamps (electric_lamp.asm): every 256 frames one charges up
// and drops a spark, which runs along the floor towards Wario. See
// object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t DISCHARGED = ADDR_ElectricLampFunc + 0x1a; // .Discharged
constexpr uint16_t CHARGING = ADDR_ElectricLampFunc + 0x36; // .Charging
constexpr uint16_t GO_LEFT = ADDR_ElectricLampSparkFunc + 0x5a; // .GoLeft
constexpr uint16_t GO_RIGHT = ADDR_ElectricLampSparkFunc + 0x66; // .GoRight

class ElectricLamp : public Object {
public:
	using Object::Object;

	void init(); // ElectricLampFunc
	void discharged(); // .Discharged
	void charging(); // .Charging
	void spark(); // ElectricLampSparkFunc: falling
	void spark_run(bool right); // .GoLeft, .GoRight

private:
	void pos_to_hram();
};

} // namespace

void ElectricLamp::init()
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	set_update(DISCHARGED);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	set_frameset(ADDR_Frameset_68cc4);
}

void ElectricLamp::discharged()
{
	no_actions();
	if (mem_[wGlobalCounter])
		return;
	set_frameset(ADDR_Frameset_68ccd);
	state_duration() = 30;
	set_update(CHARGING);
}

void ElectricLamp::charging()
{
	no_actions();
	if (!(state_duration() & 0b11))
		play_sfx(SFX_06A);
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_68cc4);
	set_update(DISCHARGED);
	play_sfx(SFX_06B);
	SET_BC(gb_, ADDR_ObjParams_ElectricLampSpark);
	routines_.call(ADDR_CreateObjectFromCurObjPos);
}

void ElectricLamp::pos_to_hram()
{
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
}

// down to the floor, then towards Wario
void ElectricLamp::spark()
{
	flags() |= OBJFLAG_NO_COLLISION;
	SET_BC(gb_, ADDR_Data_60040);
	routines_.call(ADDR_ApplyObjYMovement);
	pos_to_hram();
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a)
		return;
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	mem_[wCurObjCollBoxTop] = static_cast<uint8_t>(-6);
	mem_[wCurObjCollBoxBottom] = 5;
	set_frameset(ADDR_Frameset_68ca0);
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	set_update(static_cast<uint8_t>(screen_x() + 0x2a) < wario ? GO_RIGHT : GO_LEFT);
	play_sfx(SFX_06C);
}

// 2 pixels a frame, gone when hit, off the floor's edge or at a wall
void ElectricLamp::spark_run(bool right)
{
	x() = static_cast<uint16_t>(x() + (right ? 2 : -2));
	if (!state()) {
		pos_to_hram();
		routines_.call(ADDR_Func_3543);
		if (gb_->cpu.a) {
			uint16_t y_pos = static_cast<uint16_t>(y() - 4);
			mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
			mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
			routines_.call(ADDR_Func_3513);
			if (!gb_->cpu.a)
				return;
		}
	}
	flags() = 0;
}

} // namespace wl3::level

using ElectricLamp = wl3::level::ElectricLamp;

PORT_EX(ElectricLampFunc, 0, ALL_REGISTERS) { ElectricLamp(gb).init(); }
PORT_LOCAL_EX(ElectricLampFunc, Discharged, 0, ALL_REGISTERS) { ElectricLamp(gb).discharged(); }
PORT_LOCAL_EX(ElectricLampFunc, Charging, 0, ALL_REGISTERS) { ElectricLamp(gb).charging(); }
PORT_EX(ElectricLampSparkFunc, 0, ALL_REGISTERS) { ElectricLamp(gb).spark(); }
PORT_LOCAL_EX(ElectricLampSparkFunc, GoLeft, 0, ALL_REGISTERS) { ElectricLamp(gb).spark_run(false); }
PORT_LOCAL_EX(ElectricLampSparkFunc, GoRight, 0, ALL_REGISTERS) { ElectricLamp(gb).spark_run(true); }
