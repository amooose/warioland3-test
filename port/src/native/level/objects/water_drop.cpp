// The water drops (water_drop.asm): each falls from where it started
// until it lands (or meets water), splashes, and rises back unseen to
// fall again. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t UPDATE = ADDR_WaterDropFunc + 0x1f; // .Func_49d83

constexpr uint8_t FALLING = 0x00;
constexpr uint8_t SPLASHING = 0x5a;
constexpr uint8_t RISING = 0x5b;

class WaterDrop : public Object {
public:
	using Object::Object;

	void init(); // WaterDropFunc
	void update(); // .Func_49d83

private:
	void reset(); // .asm_49ddd: a new drop, its start in var1 and var2
	void fall(); // .asm_49df7
	void splash(); // .asm_49d92
	void splashing(); // .asm_49da6
	void rising(); // .asm_49dbd
};

} // namespace

void WaterDrop::init()
{
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 0x04;
	mem_[wCurObjCollBoxLeft] = 0xfa;
	mem_[wCurObjCollBoxBottom] = 0xff;
	flags() |= OBJFLAG_NO_COLLISION;
	reset();
}

void WaterDrop::reset()
{
	set_frameset(ADDR_Frameset_69761);
	state_duration() = 0x41;
	var1() = mem_[wCurObjYPos + 0];
	var2() = mem_[wCurObjYPos + 1];
	var3() = 0;
	state() = FALLING;
	flags() &= ~OBJFLAG_INVISIBLE;
}

void WaterDrop::update()
{
	switch (state()) {
	case FALLING: fall(); return;
	case SPLASHING: splashing(); return;
	case RISING: rising(); return;
	default: splash(); return; // hit by something
	}
}

// forming for a while, then down until it lands
void WaterDrop::fall()
{
	if (state_duration()) {
		if (!--state_duration())
			set_frameset(ADDR_Frameset_69772);
		return;
	}
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_352b);
	if (!gb_->cpu.a && !mem_[wIsInWaterOrSand]) {
		SET_BC(gb_, ADDR_FallingYVel_Light);
		routines_.call(ADDR_ApplyObjYMovement);
		return;
	}
	mem_[wCurObjYPos + 0] = mem_[hYPosLo];
	mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	splash();
}

void WaterDrop::splash()
{
	state() = SPLASHING;
	set_frameset(ADDR_Frameset_69775);
	state_duration() = 0x18;
	play_sfx(SFX_0A1);
	splashing();
}

void WaterDrop::splashing()
{
	no_actions();
	if (--state_duration())
		return;
	flags() |= OBJFLAG_INVISIBLE;
	state() = RISING;
	state_duration() = 0x3c;
	rising();
}

// back up to its start (and a wait there)
void WaterDrop::rising()
{
	no_actions();
	uint8_t wait = state_duration();
	if (wait)
		--state_duration();
	if (mem_[wCurObjYPos + 0] != var1() || mem_[wCurObjYPos + 1] != var2()) {
		move_up();
		return;
	}
	if (!wait)
		reset();
}

} // namespace wl3::level

using WaterDrop = wl3::level::WaterDrop;

PORT_EX(WaterDropFunc, 0, ALL_REGISTERS) { WaterDrop(gb).init(); }
PORT_LOCAL_EX(WaterDropFunc, Func_49d83, 0, ALL_REGISTERS) { WaterDrop(gb).update(); }
