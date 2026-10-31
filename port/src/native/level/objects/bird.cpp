// The birds (bird.asm): perched, turning now and then; when Wario's level
// with one and in front of it, it swoops at him. Knocked out of the air
// it's stunned (with stars; the blue bird recovers and flies off). See
// object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t INIT = ADDR_BirdFunc + 0x0a; // .Init
constexpr uint16_t STUNNED = ADDR_BirdFunc + 0xdb; // .Func_43353
constexpr uint16_t UPDATE = ADDR_BirdFunc + 0xea; // .Update
constexpr uint16_t BLUE_STUNNED = ADDR_BirdFunc + 0x193; // .Func_4340b
constexpr uint16_t BLUE_RECOVER = ADDR_BirdFunc + 0x1af; // .Func_43427

constexpr uint8_t SWOOPING = 0x2f;

class Bird : public Object {
public:
	using Object::Object;

	void start() { set_update(INIT); } // BirdFunc
	void init(); // .Init
	void update(); // .Update
	// .Func_432c2 and .Func_43342 find its fields through h's page (wCurObj's from .Update)
	void perched(uint16_t page); // .Func_432c2
	void swoop(uint16_t page); // .Func_43342
	void stunned(); // .Func_43353
	void blue_stunned(); // .Func_4340b
	void blue_recover(); // .Func_43427

private:
	void fly(); // .asm_43358
	void knocked(uint16_t routine); // .asm_433e8
	void hit(uint16_t stars); // .asm_43397
};

} // namespace

// facing Wario
void Bird::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	mem_[wCurObjCollBoxRight] = 0x06;
	mem_[wCurObjCollBoxLeft] = 0xf9;
	mem_[wCurObjCollBoxBottom] = 0xfa;
	state_duration() = 0;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	if (static_cast<uint8_t>(screen_x() + 0x2a) < wario) {
		substate() |= OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_687e4);
	} else {
		substate() &= ~OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_687d3);
	}
}

void Bird::update()
{
	uint8_t s = state();
	uint16_t page = wCurObjState & 0xff00;
	switch (s) {
	case 0: perched(page); return;
	case SWOOPING: swoop(page); return;
	case 0x0b: hit(ADDR_ObjParams_BirdStars1); return;
	case 0x0c: hit(ADDR_ObjParams_BirdStars2); return;
	case 0x0a: vanish(); return;
	case 0x28: knocked(ADDR_Func_33f8); return;
	case 0x29: knocked(ADDR_Func_3407); return;
	case 0x04: knocked(ADDR_Func_3326); return;
	case 0x05: knocked(ADDR_Func_3317); return;
	default: knocked((s & 1) ? ADDR_Func_33f8 : ADDR_Func_3407); return;
	}
}

// on screen: turns when its time's up; swoops when Wario's within $18
// rows and less than $70 pixels in front (or turns to him, behind it)
void Bird::perched(uint16_t page)
{
	Byte flags_ = mem_[static_cast<uint16_t>(page | OBJ_FLAGS)];
	Byte duration = mem_[static_cast<uint16_t>(page | OBJ_STATE_DURATION)];
	Byte sub = mem_[static_cast<uint16_t>(page | OBJ_SUBSTATE)];
	if (!(flags_ & OBJFLAG_ON_SCREEN))
		return;
	bool turned = false;
	if (uint8_t time = duration) {
		// the decremented value as computed (page h may be ROM or VRAM)
		duration = static_cast<uint8_t>(time - 1);
		if (time != 1)
			return;
		turned = true; // then hl is wCurObj's, from SetObjectFramesetPtr
		if (sub & OBJSUBFLAG_HDIR) {
			sub &= ~OBJSUBFLAG_HDIR;
			set_frameset(ADDR_Frameset_687d3);
		} else {
			sub |= OBJSUBFLAG_HDIR;
			set_frameset(ADDR_Frameset_687e4);
		}
	}
	uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
	if (dy >= 0x18 && dy < 0xe8)
		return;
	uint8_t bird_x = static_cast<uint8_t>(screen_x() + 0x2a);
	uint8_t wario_x = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	uint8_t dx = static_cast<uint8_t>(bird_x - wario_x);
	bool wario_right = bird_x < wario_x;
	bool facing = (turned ? substate() : sub) & OBJSUBFLAG_HDIR;
	if (facing != wario_right) { // facing away: turns to him
		set_frameset(ADDR_Frameset_687ff);
		state_duration() = 0x0c;
		return;
	}
	if (wario_right ? dx < 0x90 : dx >= 0x70)
		return;
	set_frameset(wario_right ? ADDR_Frameset_687ce : ADDR_Frameset_687c9);
	state_duration() = 0x23;
	state() = SWOOPING;
	play_sfx(SFX_06F);
}

// after a wind-up, flies straight at speed
void Bird::swoop(uint16_t page)
{
	Byte duration = mem_[static_cast<uint16_t>(page | OBJ_STATE_DURATION)];
	if (uint8_t time = duration) {
		duration = static_cast<uint8_t>(time - 1);
		if (time == 1)
			play_sfx(SFX_070);
		return;
	}
	fly();
}

void Bird::fly()
{
	routines_.call(facing_right() ? ADDR_MoveObjectRight_Fast : ADDR_MoveObjectLeft_Fast);
}

void Bird::stunned()
{
	no_actions();
	fly();
}

// hit while flying: knocked away (the way it faces); else stunned
void Bird::hit(uint16_t stars)
{
	state() = 0;
	if (flags() & OBJFLAG_NO_COLLISION) {
		knocked(facing_right() ? ADDR_Func_33f8 : ADDR_Func_3407);
		return;
	}
	flags() |= OBJFLAG_NO_COLLISION;
	if (mem_[wCurObjId] == BLUE_BIRD) {
		set_update(BLUE_STUNNED);
		state_duration() = 0x14;
		return;
	}
	set_update(STUNNED);
	SET_BC(gb_, stars);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

void Bird::knocked(uint16_t routine)
{
	flags() |= OBJFLAG_NO_COLLISION;
	set_update(routine);
	var2() = 0x02;
	var3() = 0;
	no_actions();
	set_frameset(facing_right() ? ADDR_Frameset_687fa : ADDR_Frameset_687f5);
}

void Bird::blue_stunned()
{
	mem_[wCurObjAction] = 0x81;
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_687ff);
	state_duration() = 0x0c;
	set_update(BLUE_RECOVER);
}

// turns round and flies off the other way
void Bird::blue_recover()
{
	mem_[wCurObjAction] = 0x81;
	if (state()) {
		swoop(wCurObjState & 0xff00);
		return;
	}
	if (--state_duration())
		return;
	state() = 0x01;
	if (facing_right()) {
		substate() &= ~OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_687c9);
	} else {
		substate() |= OBJSUBFLAG_HDIR;
		set_frameset(ADDR_Frameset_687ce);
	}
}

} // namespace wl3::level

using Bird = wl3::level::Bird;

PORT_EX(BirdFunc, 0, ALL_REGISTERS) { Bird(gb).start(); }
PORT_LOCAL_EX(BirdFunc, Init, 0, ALL_REGISTERS) { Bird(gb).init(); }
PORT_LOCAL_EX(BirdFunc, Update, 0, ALL_REGISTERS) { Bird(gb).update(); }
PORT_LOCAL_EX(BirdFunc, Func_432c2, 0, ALL_REGISTERS) { Bird(gb).perched(gb->cpu.h << 8); }
PORT_LOCAL_EX(BirdFunc, Func_43342, 0, ALL_REGISTERS) { Bird(gb).swoop(gb->cpu.h << 8); }
PORT_LOCAL_EX(BirdFunc, Func_43353, 0, ALL_REGISTERS) { Bird(gb).stunned(); }
PORT_LOCAL_EX(BirdFunc, Func_4340b, 0, ALL_REGISTERS) { Bird(gb).blue_stunned(); }
PORT_LOCAL_EX(BirdFunc, Func_43427, 0, ALL_REGISTERS) { Bird(gb).blue_recover(); }
