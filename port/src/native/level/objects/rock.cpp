// The rock (rock.asm): a throwable rock, by its state (what Wario's done
// to it) through a table of 96 handlers, most of them the shared object
// states (ObjState_* and the like, home routines that load their own
// registers). See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t L(uint16_t offset) { return static_cast<uint16_t>(ADDR_RockFunc + offset); }

constexpr uint16_t UPDATE = L(0x28); // .Update
constexpr uint16_t DESTROY = L(0xec); // .Destroy
constexpr uint16_t DESPAWN = L(0x108); // .Func_4aa0a
constexpr uint16_t NOTHING = L(0x124); // .Func_4aa26
constexpr uint16_t BUMP_32 = L(0x125); // .Func_4aa27
constexpr uint16_t BUMP_33 = L(0x12d); // .Func_4aa2f
constexpr uint16_t STATE_44 = L(0x13e); // .Func_4aa40
constexpr uint16_t STATE_45 = L(0x142); // .Func_4aa44
constexpr uint16_t STATE_36 = L(0x151); // .Func_4aa53
constexpr uint16_t STATE_37 = L(0x164); // .Func_4aa66
constexpr uint16_t GRAB_49 = L(0x177); // .Func_4aa79
constexpr uint16_t HELD_49 = L(0x181); // .Func_4aa83
constexpr uint16_t GRAB_48 = L(0x191); // .Func_4aa93
constexpr uint16_t HELD_48 = L(0x19b); // .Func_4aa9d
constexpr uint16_t STATE_42 = L(0x1ac); // .Func_4aaae
constexpr uint16_t THROW_LEFT = L(0x1b9); // .Func_4aabb
constexpr uint16_t THROW_RIGHT = L(0x1c1); // .Func_4aac3
constexpr uint16_t HIT_33F8 = L(0x1c9); // .Func_4aacb
constexpr uint16_t HIT_3407 = L(0x1d1); // .Func_4aad3
constexpr uint16_t HIT_3317 = L(0x1d9); // .Func_4aadb
constexpr uint16_t HIT_3326 = L(0x1e1); // .Func_4aae3

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

class Rock : public Object {
public:
	using Object::Object;

	void init(); // RockFunc
	void update(); // .Update
	void run(uint16_t handler);

private:
	void set_state_then(uint8_t s, uint8_t var1_, uint8_t var2_, uint8_t var3_, uint16_t routine);
	void hit(uint16_t routine); // .asm_4aae9
};

} // namespace

void Rock::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	set_frameset(ADDR_Frameset_69bfe);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 0x06;
	mem_[wCurObjCollBoxLeft] = 0xf9;
	mem_[wCurObjCollBoxBottom] = 0x00;
	state() = 0x31;
}

void Rock::update()
{
	uint8_t s = state();
	if (s >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[s]);
}

void Rock::set_state_then(uint8_t s, uint8_t var1_, uint8_t var2_, uint8_t var3_, uint16_t routine)
{
	state() = s;
	var3() = var3_;
	var2() = var2_;
	var1() = var1_;
	routines_.call(routine);
}

// knocked away: routine (Func_33f8 and the like) takes over
void Rock::hit(uint16_t routine)
{
	set_update(routine);
	var3() = 0;
	var2() = 0x02;
	no_actions();
}

void Rock::run(uint16_t handler)
{
	switch (handler) {
	case DESTROY: // crumbles
		state() = 0x2f;
		set_frameset(ADDR_Frameset_69c01);
		state_duration() = 0x19;
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
		var2() = 0x01;
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
		state() = 0x49;
		flags() |= 1 << OBJFLAG_GRABBED_F;
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
		state() = 0x48;
		flags() |= 1 << OBJFLAG_GRABBED_F;
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
		routines_.call(ADDR_ObjState_FullThrowLeft);
		return;
	case THROW_RIGHT:
		state() = 0x5f;
		routines_.call(ADDR_ObjState_FullThrowRight);
		return;
	case HIT_33F8: hit(ADDR_Func_33f8); return;
	case HIT_3407: hit(ADDR_Func_3407); return;
	case HIT_3317: hit(ADDR_Func_3317); return;
	case HIT_3326: hit(ADDR_Func_3326); return;
	default: // a shared object state
		routines_.call(handler);
		return;
	}
}

} // namespace wl3::level

using Rock = wl3::level::Rock;

PORT_EX(RockFunc, 0, ALL_REGISTERS) { Rock(gb).init(); }
PORT_LOCAL_EX(RockFunc, Update, 0, ALL_REGISTERS) { Rock(gb).update(); }
PORT_LOCAL_EX(RockFunc, Destroy, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::DESTROY); }
PORT_LOCAL_EX(RockFunc, Func_4aa0a, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::DESPAWN); }
PORT_LOCAL_EX(RockFunc, Func_4aa26, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::NOTHING); }
PORT_LOCAL_EX(RockFunc, Func_4aa27, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::BUMP_32); }
PORT_LOCAL_EX(RockFunc, Func_4aa2f, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::BUMP_33); }
PORT_LOCAL_EX(RockFunc, Func_4aa40, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::STATE_44); }
PORT_LOCAL_EX(RockFunc, Func_4aa44, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::STATE_45); }
PORT_LOCAL_EX(RockFunc, Func_4aa53, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::STATE_36); }
PORT_LOCAL_EX(RockFunc, Func_4aa66, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::STATE_37); }
PORT_LOCAL_EX(RockFunc, Func_4aa79, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::GRAB_49); }
PORT_LOCAL_EX(RockFunc, Func_4aa83, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::HELD_49); }
PORT_LOCAL_EX(RockFunc, Func_4aa93, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::GRAB_48); }
PORT_LOCAL_EX(RockFunc, Func_4aa9d, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::HELD_48); }
PORT_LOCAL_EX(RockFunc, Func_4aaae, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::STATE_42); }
PORT_LOCAL_EX(RockFunc, Func_4aabb, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::THROW_LEFT); }
PORT_LOCAL_EX(RockFunc, Func_4aac3, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::THROW_RIGHT); }
PORT_LOCAL_EX(RockFunc, Func_4aacb, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::HIT_33F8); }
PORT_LOCAL_EX(RockFunc, Func_4aad3, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::HIT_3407); }
PORT_LOCAL_EX(RockFunc, Func_4aadb, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::HIT_3317); }
PORT_LOCAL_EX(RockFunc, Func_4aae3, 0, ALL_REGISTERS) { Rock(gb).run(wl3::level::HIT_3326); }
