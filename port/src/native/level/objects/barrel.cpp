// The barrels (barrel.asm): one that sinks a little when stood on, then
// floats off along Data_60a50 (Barrel1), and a throwable one, by its
// state through a table of 96 handlers like the rock's (Barrel2). See
// object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t FLOAT_IDLE = ADDR_Barrel1Func + 0x28; // .Update1
constexpr uint16_t FLOAT_SINKING = ADDR_Barrel1Func + 0x51; // .Update2

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_Barrel2Func + offset); }

constexpr uint16_t UPDATE = L(0x28); // .Update
constexpr uint16_t DESTROY = L(0xec); // .Func_435a6
constexpr uint16_t DESPAWN = L(0x108); // .Func_435c2
constexpr uint16_t NOTHING = L(0x124); // .Func_435de
constexpr uint16_t BUMP_32 = L(0x125); // .Func_435df
constexpr uint16_t BUMP_33 = L(0x12d); // .Func_435e7
constexpr uint16_t STATE_44 = L(0x13e); // .Func_435f8
constexpr uint16_t STATE_45 = L(0x142); // .Func_435fc
constexpr uint16_t STATE_36 = L(0x151); // .Func_4360b
constexpr uint16_t STATE_37 = L(0x16a); // .Func_43624
constexpr uint16_t GRAB_49 = L(0x183); // .Func_4363d
constexpr uint16_t HELD_49 = L(0x192); // .Func_4364c
constexpr uint16_t GRAB_48 = L(0x1a2); // .Func_4365c
constexpr uint16_t HELD_48 = L(0x1b1); // .Func_4366b
constexpr uint16_t STATE_42 = L(0x1c2); // .Func_4367c
constexpr uint16_t THROW_LEFT = L(0x1cf); // .Func_43689
constexpr uint16_t THROW_RIGHT = L(0x1dd); // .Func_43697
constexpr uint16_t HIT_33F8 = L(0x1eb); // .Func_436a5
constexpr uint16_t HIT_3407 = L(0x1f9); // .Func_436b3
constexpr uint16_t HIT_3317 = L(0x207); // .Func_436c1
constexpr uint16_t HIT_3326 = L(0x215); // .Func_436cf

// .Update's jumptable, by state
constexpr uint16_t HANDLERS[] = {
	DESTROY, DESTROY, BUMP_32, BUMP_33, HIT_3326, HIT_3317, STATE_36, STATE_37, // $00
	BUMP_32, BUMP_33, ADDR_Func_3173, DESTROY, DESTROY, DESTROY, DESTROY, DESTROY,
	DESTROY, DESTROY, STATE_42, NOTHING, STATE_44, STATE_45, DESTROY, DESTROY, // $10
	GRAB_48, GRAB_49, DESTROY, DESTROY, DESTROY, DESTROY, DESTROY, DESTROY,
	DESTROY, DESTROY, DESTROY, DESTROY, DESTROY, DESTROY, DESTROY, DESTROY, // $20
	HIT_33F8, HIT_3407, DESTROY, DESTROY, DESTROY, DESTROY, DESTROY, DESPAWN,
	DESTROY, NOTHING, ADDR_ObjState_BumpLeft, ADDR_ObjState_BumpRight, DESTROY, DESTROY, ADDR_Func_3335, ADDR_Func_3344, // $30
	DESTROY, DESTROY, DESTROY, DESTROY, DESTROY, DESTROY, DESTROY, DESTROY,
	DESTROY, DESTROY, ADDR_Func_3371, DESTROY, ADDR_Func_32cc, ADDR_Func_32db, DESTROY, DESTROY, // $40
	HELD_48, HELD_49, ADDR_Func_3380, ADDR_Func_338f, THROW_LEFT, THROW_RIGHT, ADDR_Func_339e, ADDR_Func_33ad,
	DESTROY, DESTROY, DESTROY, DESTROY, DESTROY, DESTROY, DESTROY, DESTROY, // $50
	DESTROY, DESTROY, DESTROY, DESTROY, DESTROY, DESTROY, ADDR_ObjState_FullThrowLeft, ADDR_ObjState_FullThrowRight,
};
constexpr uint8_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

class Barrel : public Object {
public:
	using Object::Object;

	void floating(); // Barrel1Func
	void float_idle(); // .Update1
	void float_sinking(); // .Update2
	void init(); // Barrel2Func
	void update(); // .Update
	void run(uint16_t handler);

private:
	void set_state_then(uint8_t s, uint8_t var1_, uint8_t var2_, uint8_t var3_, uint16_t routine);
	void hit(uint16_t frameset, uint16_t routine); // .asm_436db
	void grab(uint8_t s);
};

} // namespace

void Barrel::floating()
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	set_update(FLOAT_IDLE);
	set_frameset(ADDR_Frameset_68a7b);
	state_duration() = 0x0a;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 0x06;
	mem_[wCurObjCollBoxLeft] = 0xf9;
	mem_[wCurObjCollBoxBottom] = 0xf7;
}

// stood on for 10 frames in a row
void Barrel::float_idle()
{
	if (!(flags() & OBJFLAG_STEPPED)) {
		state_duration() = 0x0a;
		return;
	}
	flags() &= ~OBJFLAG_STEPPED;
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_68a7e);
	state_duration() = 0x11;
	set_update(FLOAT_SINKING);
	var3() = 0;
}

void Barrel::float_sinking()
{
	if (state_duration()) {
		if (!--state_duration())
			play_sfx(SFX_086);
		return;
	}
	SET_BC(gb_, ADDR_Data_60a50);
	routines_.call(ADDR_Func_34fc);
}

void Barrel::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	set_frameset(ADDR_Frameset_68a78);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 0x06;
	mem_[wCurObjCollBoxLeft] = 0xf9;
	mem_[wCurObjCollBoxBottom] = 0x00;
	state() = 0x31;
}

void Barrel::update()
{
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

void Barrel::set_state_then(uint8_t s, uint8_t var1_, uint8_t var2_, uint8_t var3_, uint16_t routine)
{
	state() = s;
	var3() = var3_;
	var2() = var2_;
	var1() = var1_;
	set_frameset(ADDR_Frameset_68a78);
	routines_.call(routine);
}

// knocked away: routine (Func_33f8 and the like) takes over
void Barrel::hit(uint16_t frameset, uint16_t routine)
{
	set_frameset(frameset);
	set_update(routine);
	var3() = 0;
	var2() = 0x02;
	no_actions();
}

void Barrel::grab(uint8_t s)
{
	state() = s;
	set_frameset(ADDR_Frameset_68a78);
	flags() |= 1 << OBJFLAG_GRABBED_F;
}

void Barrel::run(uint16_t handler)
{
	switch (handler) {
	case DESTROY: // breaks
		state() = 0x2f;
		set_frameset(ADDR_Frameset_68a8b);
		state_duration() = 0x0e;
		flags() = (flags() & ~(1 << OBJFLAG_GRABBED_F)) | OBJFLAG_NO_COLLISION;
		play_sfx(SFX_TEMPLE_ROCK);
		run(DESPAWN);
		return;
	case DESPAWN:
		no_actions();
		if (!--state_duration())
			despawn();
		return;
	case NOTHING:
		return;
	case BUMP_32:
	case BUMP_33:
		state() = handler == BUMP_32 ? 0x32 : 0x33;
		state_duration() = 0x0c;
		var2() = 0x02;
		return;
	case STATE_44:
	case STATE_45:
		state() = handler == STATE_44 ? 0x44 : 0x45;
		var3() = 0;
		var2() = 0x02;
		return;
	case STATE_36: set_state_then(0x36, 0x01, 0xc0, 0x41, ADDR_Func_3335); return;
	case STATE_37: set_state_then(0x37, 0x01, 0xe0, 0x41, ADDR_Func_3344); return;
	case GRAB_49:
		grab(0x49);
		run(HELD_49);
		return;
	case HELD_49: // carried facing the way Wario does
		if (mem_[wDirection]) {
			routines_.call(ADDR_Func_3353);
			return;
		}
		substate() ^= 0x80;
		run(GRAB_48);
		return;
	case GRAB_48:
		grab(0x48);
		run(HELD_48);
		return;
	case HELD_48:
		if (!mem_[wDirection]) {
			routines_.call(ADDR_Func_3362);
			return;
		}
		substate() ^= 0x80;
		run(GRAB_49);
		return;
	case STATE_42:
		mem_[wCurObjAction] = 0x9f;
		state() = 0x42;
		var3() = 0;
		return;
	case THROW_LEFT:
		state() = 0x5e;
		set_frameset(ADDR_Frameset_68a6f);
		routines_.call(ADDR_ObjState_FullThrowLeft);
		return;
	case THROW_RIGHT:
		state() = 0x5f;
		set_frameset(ADDR_Frameset_68a66);
		routines_.call(ADDR_ObjState_FullThrowRight);
		return;
	case HIT_33F8: hit(ADDR_Frameset_68a6f, ADDR_Func_33f8); return;
	case HIT_3407: hit(ADDR_Frameset_68a66, ADDR_Func_3407); return;
	case HIT_3317: hit(ADDR_Frameset_68a6f, ADDR_Func_3317); return;
	case HIT_3326: hit(ADDR_Frameset_68a66, ADDR_Func_3326); return;
	default: // a shared object state
		routines_.call(handler);
		return;
	}
}

} // namespace wl3::level

using Barrel = wl3::level::Barrel;

PORT_EX(Barrel1Func, 0, ALL_REGISTERS) { Barrel(gb).floating(); }
PORT_LOCAL_EX(Barrel1Func, Update1, 0, ALL_REGISTERS) { Barrel(gb).float_idle(); }
PORT_LOCAL_EX(Barrel1Func, Update2, 0, ALL_REGISTERS) { Barrel(gb).float_sinking(); }
PORT_EX(Barrel2Func, 0, ALL_REGISTERS) { Barrel(gb).init(); }
PORT_LOCAL_EX(Barrel2Func, Update, 0, ALL_REGISTERS) { Barrel(gb).update(); }
PORT_LOCAL_EX(Barrel2Func, Func_435a6, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::DESTROY); }
PORT_LOCAL_EX(Barrel2Func, Func_435c2, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::DESPAWN); }
PORT_LOCAL_EX(Barrel2Func, Func_435de, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::NOTHING); }
PORT_LOCAL_EX(Barrel2Func, Func_435df, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::BUMP_32); }
PORT_LOCAL_EX(Barrel2Func, Func_435e7, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::BUMP_33); }
PORT_LOCAL_EX(Barrel2Func, Func_435f8, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::STATE_44); }
PORT_LOCAL_EX(Barrel2Func, Func_435fc, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::STATE_45); }
PORT_LOCAL_EX(Barrel2Func, Func_4360b, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::STATE_36); }
PORT_LOCAL_EX(Barrel2Func, Func_43624, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::STATE_37); }
PORT_LOCAL_EX(Barrel2Func, Func_4363d, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::GRAB_49); }
PORT_LOCAL_EX(Barrel2Func, Func_4364c, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::HELD_49); }
PORT_LOCAL_EX(Barrel2Func, Func_4365c, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::GRAB_48); }
PORT_LOCAL_EX(Barrel2Func, Func_4366b, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::HELD_48); }
PORT_LOCAL_EX(Barrel2Func, Func_4367c, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::STATE_42); }
PORT_LOCAL_EX(Barrel2Func, Func_43689, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::THROW_LEFT); }
PORT_LOCAL_EX(Barrel2Func, Func_43697, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::THROW_RIGHT); }
PORT_LOCAL_EX(Barrel2Func, Func_436a5, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::HIT_33F8); }
PORT_LOCAL_EX(Barrel2Func, Func_436b3, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::HIT_3407); }
PORT_LOCAL_EX(Barrel2Func, Func_436c1, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::HIT_3317); }
PORT_LOCAL_EX(Barrel2Func, Func_436cf, 0, ALL_REGISTERS) { Barrel(gb).run(wl3::level::HIT_3326); }
