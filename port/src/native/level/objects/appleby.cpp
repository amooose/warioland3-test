// Appleby (appleby.asm): pops out of the ground when on screen, and when
// Wario's in range throws an apple at him, then hides again; the apple
// rolls along and bounces off when hit. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t UPDATE = ADDR_ApplebyFunc + 0x49; // .Update
constexpr uint16_t ROLL_START_LEFT = ADDR_AppleFunc + 0x12; // .Update1_Left
constexpr uint16_t ROLL_START_RIGHT = ADDR_AppleFunc + 0x34; // .Update1_Right
constexpr uint16_t ROLL_LEFT = ADDR_AppleFunc + 0x56; // .Update2_Left
constexpr uint16_t ROLL_RIGHT = ADDR_AppleFunc + 0x6d; // .Update2_Right
constexpr uint16_t BOUNCE_LEFT = ADDR_AppleFunc + 0xba; // .Func_47bd3
constexpr uint16_t BOUNCE_RIGHT = ADDR_AppleFunc + 0xbf; // .Func_47bd8

class Appleby : public Object {
public:
	using Object::Object;

	void init(); // ApplebyFunc
	void update(); // .Update
	void hidden(uint16_t page); // .Func_479e0, through h's page
	void hit(uint8_t s, uint16_t state_addr); // .Func_47ab0: a = state, hl
	void apple(uint16_t update); // AppleFunc
	void roll_start(bool right); // .Update1_Left, .Update1_Right
	void roll(bool right, bool fast); // .Update2_Left, .Update2_Right
	void bounce(bool right); // .Func_47bd8, .Func_47bd3

private:
	void aim(); // .asm_47a6b
	void knocked(uint16_t page, uint16_t routine); // .asm_47af6
	void hide(uint16_t state_addr); // .asm_47ac4
};

} // namespace

void Appleby::init()
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	set_update(UPDATE);
	set_frameset(ADDR_Frameset_689e9);
	state_duration() = 5;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 7;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-8);
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-1);
}

void Appleby::update()
{
	uint8_t s = state();
	if (!s) {
		hidden(wCurObjState & 0xff00);
		return;
	}
	if (s < 0x2a) {
		hit(s, wCurObjState);
		return;
	}
	switch (s) {
	case 0x5a: // popping up
		no_actions();
		if (--state_duration())
			return;
		set_frameset(ADDR_Frameset_68a12);
		state() = 0x5b;
		return;
	case 0x5b:
		aim();
		return;
	case 0x5c: // throwing (the apple at $46 frames left)
		if (state_duration() == 0x46) {
			SET_BC(gb_, facing_right() ? ADDR_ObjParams_AppleRight : ADDR_ObjParams_AppleLeft);
			routines_.call(ADDR_CreateObjectAtRelativePos);
		}
		if (!--state_duration())
			hide(wCurObjState);
		return;
	case 0x2d: // hiding
		no_actions();
		if (--state_duration())
			return;
		set_frameset(ADDR_Frameset_689e9);
		state_duration() = 60;
		state() = 0;
		return;
	}
}

// pops up once its time's up on screen
void Appleby::hidden(uint16_t page)
{
	no_actions();
	if (!(mem_[static_cast<uint16_t>(page | OBJ_FLAGS)] & OBJFLAG_ON_SCREEN))
		return;
	// the decremented value as computed (page h may be ROM)
	Byte duration = mem_[static_cast<uint16_t>(page | OBJ_STATE_DURATION)];
	uint8_t left = static_cast<uint8_t>(duration - 1);
	duration = left;
	if (left)
		return;
	set_frameset(ADDR_Frameset_68a00);
	state_duration() = 29;
	state() = 0x5a;
	flags() &= ~OBJFLAG_NO_COLLISION;
}

// waits, then turns to Wario if he's below it and within $40 pixels
void Appleby::aim()
{
	if (state_duration()) {
		--state_duration();
		return;
	}
	uint8_t below = static_cast<uint8_t>(static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a) - static_cast<uint8_t>(screen_y() + 0x2a));
	if (below < 0x04 || below >= 0x46)
		return;
	uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
	if (dx < 0x40) {
		substate() &= ~OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_68a1f);
	} else if (dx >= 0xc0) {
		substate() |= OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_68a3a);
	} else {
		return;
	}
	state_duration() = 122;
	state() = 0x5c;
}

void Appleby::hit(uint8_t s, uint16_t state_addr)
{
	uint16_t page = state_addr & 0xff00;
	switch (s) {
	case 0x0a: routines_.call(ADDR_VanishObject); return;
	case 0x04: knocked(page, ADDR_Func_3326); return;
	case 0x05: knocked(page, ADDR_Func_3317); return;
	case 0x28: knocked(page, ADDR_Func_33f8); return;
	case 0x29: knocked(page, ADDR_Func_3407); return;
	default: hide(state_addr); return;
	}
}

void Appleby::hide(uint16_t state_addr)
{
	mem_[state_addr] = 0x2d;
	set_frameset(ADDR_Frameset_68a09);
	state_duration() = 29;
	flags() |= OBJFLAG_NO_COLLISION;
}

void Appleby::knocked(uint16_t page, uint16_t routine)
{
	// high byte first, as the asm (h may point at the MBC)
	mem_[static_cast<uint16_t>(page | (OBJ_UPDATE_FUNCTION + 1))] = routine >> 8;
	mem_[static_cast<uint16_t>(page | OBJ_UPDATE_FUNCTION)] = routine & 0xff;
	set_frameset(ADDR_Frameset_68a63);
	substate() &= OBJSUBFLAGS_MASK;
	var3() = 0;
	var2() = 0x02;
	no_actions();
}

// AppleFunc_Left / Right
void Appleby::apple(uint16_t update)
{
	set_update(update);
	flags() |= OBJFLAG_NO_COLLISION;
	play_sfx(SFX_067);
}

// rolls fast if Wario's at least $10 pixels ahead
void Appleby::roll_start(bool right)
{
	set_update(right ? ROLL_RIGHT : ROLL_LEFT);
	uint8_t apple_x = static_cast<uint8_t>(screen_x() + 0x2a);
	uint8_t wario_x = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	uint8_t from = right ? wario_x : apple_x, to = right ? apple_x : wario_x;
	if (from < to || static_cast<uint8_t>(from - to) < 0x10)
		return;
	var1() = 0x01;
	var2() = 0x02;
}

// var2 pixels a frame if fast, else every other frame; gone once
// collected, bounced off when hit
void Appleby::roll(bool right, bool fast)
{
	bool odd = mem_[wGlobalCounter] & 1;
	if (fast || (right ? odd : !odd)) {
		uint16_t step = var2();
		x() = static_cast<uint16_t>(right ? x() + step : x() - step);
	}
	uint8_t s = state();
	if (s == 0x18) {
		flags() = 0;
		return;
	}
	if (s != 0x19) {
		SET_BC(gb_, ADDR_Data_60670);
		routines_.call(ADDR_ApplyObjYMovement);
		return;
	}
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	set_update(static_cast<uint8_t>(screen_x() + 0x2a) < wario ? BOUNCE_LEFT : BOUNCE_RIGHT);
	var3() = 0;
}

void Appleby::bounce(bool right)
{
	right ? move_right() : move_left();
	no_actions();
	SET_BC(gb_, ADDR_Data_60040);
	routines_.call(ADDR_ApplyObjYMovement);
}

} // namespace wl3::level

using Appleby = wl3::level::Appleby;

PORT_EX(ApplebyFunc, 0, ALL_REGISTERS) { Appleby(gb).init(); }
PORT_LOCAL_EX(ApplebyFunc, Update, 0, ALL_REGISTERS) { Appleby(gb).update(); }
PORT_LOCAL_EX(ApplebyFunc, Func_479e0, 0, ALL_REGISTERS) { Appleby(gb).hidden(gb->cpu.h << 8); }
PORT_LOCAL_EX(ApplebyFunc, Func_47ab0, 0, ALL_REGISTERS) { Appleby(gb).hit(gb->cpu.a, REG_HL(gb)); }
PORT_EX(AppleFunc_Left, 0, ALL_REGISTERS) { Appleby(gb).apple(wl3::level::ROLL_START_LEFT); }
PORT_EX(AppleFunc_Right, 0, ALL_REGISTERS) { Appleby(gb).apple(wl3::level::ROLL_START_RIGHT); }
PORT_EX(AppleFunc, 0, ALL_REGISTERS) { Appleby(gb).apple(static_cast<uint16_t>(gb->cpu.a << 8 | gb->cpu.b)); }
PORT_LOCAL_EX(AppleFunc, Update1_Left, 0, ALL_REGISTERS) { Appleby(gb).roll_start(false); }
PORT_LOCAL_EX(AppleFunc, Update1_Right, 0, ALL_REGISTERS) { Appleby(gb).roll_start(true); }
// the asm tests the Z flag it's entered with (NZ from UpdateObject's
// `or BANK`) where it means var1
PORT_LOCAL_EX(AppleFunc, Update2_Left, 0, ALL_REGISTERS) { Appleby(gb).roll(false, !(gb->cpu.f & FLAG_Z)); }
PORT_LOCAL_EX(AppleFunc, Update2_Right, 0, ALL_REGISTERS) { Appleby(gb).roll(true, wl3::Memory(gb)[wCurObjVar1]); }
PORT_LOCAL_EX(AppleFunc, Func_47bd3, 0, ALL_REGISTERS) { Appleby(gb).bounce(false); }
PORT_LOCAL_EX(AppleFunc, Func_47bd8, 0, ALL_REGISTERS) { Appleby(gb).bounce(true); }
