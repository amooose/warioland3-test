// Prince Froggy (prince_froggy.asm): the red one is stuck in the ground
// (and squeaks when stood on) until it's knocked loose; then, like the
// grey one, it sits, blowing a bubble (the prince froggy interaction:
// he gives Wario the frog gloves' ride). See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t STUCK = ADDR_PrinceFroggyFunc + 0x30; // .Update_Stuck
constexpr uint16_t SITTING = ADDR_PrinceFroggyFunc + 0xe1; // .Update_Sitting
constexpr uint16_t VANISHING = ADDR_PrinceFroggyFunc + 0x15c; // .Func_44220

class PrinceFroggy : public Object {
public:
	using Object::Object;

	void grey(); // GreyPrinceFroggyFunc
	void red(); // RedPrinceFroggyFunc
	// .Func_440c4 and the like: routine (Func_33f8...) takes over; the
	// asm finds the fields through h's page (wCurObj's from its callers)
	void hit(uint16_t page, uint16_t routine);
	void stuck(); // .Update_Stuck
	void sitting(); // .Update_Sitting
	void sit(uint16_t state_addr); // .Func_441dc: writes $43 to [hl]
	void vanishing(); // .Func_44220

private:
	static constexpr uint16_t page() { return wCurObjState & 0xff00; }
	void set_up(uint16_t update);
	void shaken(); // .asm_44185
	void falling(); // .asm_44151
	void start_vanishing(); // .asm_4420a
	void settle(); // .asm_44166
};

} // namespace

void PrinceFroggy::set_up(uint16_t update)
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(update);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 0x03;
	mem_[wCurObjCollBoxLeft] = 0xec;
	mem_[wCurObjCollBoxBottom] = 0x00;
}

void PrinceFroggy::grey()
{
	set_up(SITTING);
	sit(wCurObjState);
}

void PrinceFroggy::red()
{
	set_up(STUCK);
	set_frameset(ADDR_Frameset_68ac6);
}

void PrinceFroggy::hit(uint16_t page_, uint16_t routine)
{
	// high byte first, as the asm (h may point at the MBC)
	mem_[static_cast<uint16_t>(page_ | (OBJ_UPDATE_FUNCTION + 1))] = routine >> 8;
	mem_[static_cast<uint16_t>(page_ | OBJ_UPDATE_FUNCTION)] = routine & 0xff;
	Byte sub = mem_[static_cast<uint16_t>(page_ | OBJ_SUBSTATE)];
	sub = sub & OBJSUBFLAGS_MASK;
	mem_[static_cast<uint16_t>(page_ | OBJ_VAR_3)] = 0;
	mem_[static_cast<uint16_t>(page_ | OBJ_VAR_2)] = 0x01;
	no_actions();
	set_frameset(ADDR_Frameset_68ae6);
}

// a strong ground shake knocks it over
void PrinceFroggy::shaken()
{
	if (mem_[wGroundShakeCounter] < 0x10)
		return;
	state() = 0x1b;
	var3() = 0;
	set_frameset(ADDR_Frameset_68ae6);
	state_duration() = 13;
	set_interaction(OBJ_INTERACTION_0B);
}

void PrinceFroggy::falling()
{
	state() = 0x41;
	var3() = 0;
}

void PrinceFroggy::start_vanishing()
{
	set_update(VANISHING);
	set_frameset(ADDR_Frameset_68ae6);
	state_duration() = 16;
	var3() = 0;
}

void PrinceFroggy::settle()
{
	state() = 0;
	set_frameset(ADDR_Frameset_68ac6);
	shaken();
}

void PrinceFroggy::stuck()
{
	uint8_t s = state();
	switch (s) {
	case 0: // squeaks when stood on
		if (!(flags() & OBJFLAG_STEPPED)) {
			shaken();
			return;
		}
		flags() &= ~OBJFLAG_STEPPED;
		state() = 0x5a;
		set_frameset(ADDR_Frameset_68add);
		state_duration() = 111;
		return;
	case 0x1b: routines_.call(ADDR_Func_3272); return;
	case 0x41: routines_.call(ADDR_ObjState_StandingFall); return;
	case 0x13: // knocked loose
		set_update(SITTING);
		sit(wCurObjState);
		return;
	case 0x11: falling(); return;
	case 0x5a: // until Wario steps off
		if (flags() & OBJFLAG_STEPPED) {
			flags() &= ~OBJFLAG_STEPPED;
			return;
		}
		settle();
		return;
	case 0x3b:
		if (--state_duration())
			shaken();
		else
			settle();
		return;
	case 0x28: hit(page(), ADDR_Func_33f8); return;
	case 0x29: hit(page(), ADDR_Func_3407); return;
	case 0x04: hit(page(), ADDR_Func_3326); return;
	case 0x05: hit(page(), ADDR_Func_3317); return;
	case 0x0a: start_vanishing(); return;
	default: // hit: wobbles
		state() = 0x3b;
		set_frameset(ADDR_Frameset_68af2);
		state_duration() = 40;
		flags() &= ~OBJFLAG_NO_COLLISION;
		return;
	}
}

void PrinceFroggy::sitting()
{
	uint8_t s = state();
	switch (s) {
	case 0: shaken(); return;
	case 0x11: falling(); return;
	case 0x1b: routines_.call(ADDR_Func_3272); return;
	case 0x41: routines_.call(ADDR_ObjState_StandingFall); return;
	case 0x28: hit(page(), ADDR_Func_33f8); return;
	case 0x29: hit(page(), ADDR_Func_3407); return;
	case 0x04: hit(page(), ADDR_Func_3326); return;
	case 0x05: hit(page(), ADDR_Func_3317); return;
	case 0x13: sit(wCurObjState); return;
	case 0x43: // blowing its bubble
		if (!--state_duration()) {
			state() = 0;
			set_frameset(ADDR_Frameset_68ae9);
		}
		shaken();
		return;
	case 0x0a: start_vanishing(); return;
	default:
		state() = 0;
		sit(wCurObjState);
		return;
	}
}

void PrinceFroggy::sit(uint16_t state_addr)
{
	mem_[state_addr] = 0x43;
	set_frameset(ADDR_Frameset_68b01);
	state_duration() = 73;
	flags() &= ~OBJFLAG_NO_COLLISION;
	set_interaction(OBJ_INTERACTION_PRINCE_FROGGY);
	mem_[wCurObjCollBoxTop] = 0xe6;
}

void PrinceFroggy::vanishing()
{
	no_actions();
	SET_BC(gb_, ADDR_Data_60040);
	routines_.call(ADDR_ApplyObjYMovement);
	if (--state_duration())
		return;
	var2() = 0x08;
	move_left_by_var2();
	routines_.call(ADDR_VanishObject2);
}

} // namespace wl3::level

using PrinceFroggy = wl3::level::PrinceFroggy;

PORT_EX(GreyPrinceFroggyFunc, 0, ALL_REGISTERS) { PrinceFroggy(gb).grey(); }
PORT_EX(RedPrinceFroggyFunc, 0, ALL_REGISTERS) { PrinceFroggy(gb).red(); }
PORT_EX(PrinceFroggyFunc, 0, ALL_REGISTERS) { PrinceFroggy(gb).hit(gb->cpu.h << 8, ADDR_Func_33f8); }
PORT_LOCAL_EX(PrinceFroggyFunc, Func_440ca, 0, ALL_REGISTERS) { PrinceFroggy(gb).hit(gb->cpu.h << 8, ADDR_Func_3407); }
PORT_LOCAL_EX(PrinceFroggyFunc, Func_440d0, 0, ALL_REGISTERS) { PrinceFroggy(gb).hit(gb->cpu.h << 8, ADDR_Func_3326); }
PORT_LOCAL_EX(PrinceFroggyFunc, Func_440d6, 0, ALL_REGISTERS) { PrinceFroggy(gb).hit(gb->cpu.h << 8, ADDR_Func_3317); }
PORT_LOCAL_EX(PrinceFroggyFunc, Update_Stuck, 0, ALL_REGISTERS) { PrinceFroggy(gb).stuck(); }
PORT_LOCAL_EX(PrinceFroggyFunc, Update_Sitting, 0, ALL_REGISTERS) { PrinceFroggy(gb).sitting(); }
PORT_LOCAL_EX(PrinceFroggyFunc, Func_441dc, 0, ALL_REGISTERS) { PrinceFroggy(gb).sit(REG_HL(gb)); }
PORT_LOCAL_EX(PrinceFroggyFunc, Func_44220, 0, ALL_REGISTERS) { PrinceFroggy(gb).vanishing(); }
