// Omodonmeka (omodonmeka.asm): a flying machine that hovers, opens its
// mouth when Wario's close and above, swallows him and carries him up
// (flat Wario rides it instead). And the one carrying an Omodon, which
// swallows Wario and hands him to it, and the Omodon itself, which drops
// onto the ground as a block. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t OM(uint16_t offset) { return static_cast<uint16_t>(ADDR_OmodonmekaFunc + offset); }
constexpr uint16_t OWO(uint16_t offset) { return static_cast<uint16_t>(ADDR_OmodonmekaWithOmodonFunc + offset); }

constexpr uint16_t GOT_ON = OM(0x29); // .Func_42199
constexpr uint16_t UPDATE = OM(0xa8); // .Func_42218
constexpr uint16_t CARRY_WAIT = OM(0xf1); // .Func_42261
constexpr uint16_t CARRY_UP = OM(0x121); // .Func_42291
constexpr uint16_t CARRY_HOVER = OM(0x18c); // .Func_422fc
constexpr uint16_t HOVER = OM(0x1b1); // .Func_42321
constexpr uint16_t MOUTH_OPEN = OM(0x1f9); // .Func_42369
constexpr uint16_t FALL_BACK = OM(0x271); // .Func_423e1
constexpr uint16_t RISE_BACK = OM(0x2b0); // .Func_42420
constexpr uint16_t START_HOVER = OM(0x2c2); // .Func_42432
constexpr uint16_t KNOCKED = OM(0x2dc); // .Func_4244c

constexpr uint16_t WAIT_STEP = OWO(0x4b); // .Func_42535
constexpr uint16_t STEPPED = OWO(0x6a); // .Func_42554
constexpr uint16_t SWALLOW = OWO(0x6f); // .Func_42559
constexpr uint16_t HAND_OVER = OWO(0x82); // .Func_4256c
constexpr uint16_t RISE = OWO(0x204); // .Func_426ee
constexpr uint16_t OWO_KNOCKED = OWO(0x22a); // .Func_42714

constexpr uint16_t OMODON_UPDATE = ADDR_Func_4279c + 0xd; // .Func_427a9

// .asm_42249's jumptable, by the substate's low nybble
constexpr uint16_t HANDLERS[] = {
	HOVER, HOVER, FALL_BACK, RISE_BACK, KNOCKED, CARRY_UP, CARRY_HOVER, CARRY_WAIT,
	MOUTH_OPEN,
};
constexpr uint8_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

// .asm_42525's jumptable, by the substate's low nybble
constexpr uint16_t OWO_HANDLERS[] = {
	WAIT_STEP, WAIT_STEP, STEPPED, HAND_OVER, RISE,
};
constexpr uint8_t NUM_OWO_HANDLERS = sizeof(OWO_HANDLERS) / sizeof(OWO_HANDLERS[0]);

class Omodonmeka : public Object {
public:
	using Object::Object;

	void start(); // OmodonmekaFunc
	void got_on(uint16_t at); // .Func_42199, with hl = at
	void run(uint16_t handler);
	void with_omodon_start(uint16_t omodon); // OmodonmekaWithOmodon1Func, ...2Func
	void with_omodon(); // OmodonmekaWithOmodonFunc
	void run_owo(uint16_t handler);
	void omodon_start(); // Func_4279c
	void omodon(); // .Func_427a9
	void omodon_idle(); // Func_42767
	void omodon_fall(); // Func_42767.asm_4277f
	void owner_stepped(); // .Func_42894
	void find_owner_state(); // .Func_42935
	void bob(); // Func_42725
	void block(); // Func_42754

private:
	void set_substate(uint8_t mask, uint8_t v) { substate() = (substate() & mask) | v; }
	void dispatch(); // .asm_42249
	void knocked(uint16_t routine); // .asm_421fa
	bool hit(uint8_t s);
	bool auto_move_over();
	void wario_above(uint8_t offset);
	bool at_start() const;
	bool wario_near(uint8_t &dx);
	void swallow(); // .Func_42559
	void omodon_land(); // .asm_4283c
	void above_wario();
	int find(uint8_t mask, uint8_t want) const;
};

} // namespace

void Omodonmeka::start()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 0xfd;
	mem_[wCurObjCollBoxLeft] = 0xf3;
	mem_[wCurObjCollBoxBottom] = 0xf9;
	var1() = mem_[wCurObjYPos + 0];
	var2() = mem_[wCurObjYPos + 1];
	run(START_HOVER);
}

void Omodonmeka::dispatch()
{
	uint8_t i = substate() & 0x0f;
	if (i >= NUM_HANDLERS) {
		unrealistic();
		return;
	}
	run(HANDLERS[i]);
}

// Wario got on (swallowed, or flat on top)
void Omodonmeka::got_on(uint16_t at)
{
	mem_[at] = 0;
	set_frameset(ADDR_Frameset_68f14);
	uint8_t e, d;
	if (mem_[wTransformation] == TRANSFORMATION_FLAT_WARIO) {
		state_duration() = 0x18;
		e = 0x07;
		d = 0x07;
	} else {
		e = 0x05;
		d = 0x1a;
	}
	uint16_t wario_y = static_cast<uint16_t>(mem_[wWarioYPos + 0] << 8 | mem_[wWarioYPos + 1]);
	y() = static_cast<uint16_t>(wario_y - d);
	uint16_t wario_x = static_cast<uint16_t>(x() - 8);
	mem_[wWarioXPos + 1] = static_cast<uint8_t>(wario_x);
	mem_[wWarioXPos + 0] = static_cast<uint8_t>(wario_x >> 8);
	set_substate(0xf0, e);
	mem_[wCurObjCollBoxBottom] = 0xf9;
	dispatch();
}

void Omodonmeka::knocked(uint16_t routine)
{
	set_update(routine);
	var3() = 0;
	var2() = 0x02;
	set_frameset(ADDR_Frameset_68f3d);
	no_actions();
	state() = 0x3a;
}

// the hits both handle: true if it's one
bool Omodonmeka::hit(uint8_t s)
{
	switch (s) {
	case 0x04:
		knocked(ADDR_Func_3326);
		return true;
	case 0x05:
		knocked(ADDR_Func_3317);
		return true;
	case 0x28:
		knocked(ADDR_Func_33f8);
		return true;
	case 0x29:
		knocked(ADDR_Func_3407);
		return true;
	case 0x0a:
		vanish();
		return true;
	default:
		return false;
	}
}

// once the auto-move is over, back to state 1
bool Omodonmeka::auto_move_over()
{
	no_actions();
	if (mem_[wAutoMoveState])
		return false;
	state() = 1;
	return true;
}

// its y position at the given offset above Wario's
void Omodonmeka::wario_above(uint8_t offset)
{
	uint16_t wario_y = static_cast<uint16_t>(mem_[wWarioYPos + 0] << 8 | mem_[wWarioYPos + 1]);
	y() = static_cast<uint16_t>(wario_y - offset);
}

// back at its starting position (var1/var2)
bool Omodonmeka::at_start() const
{
	return mem_[wCurObjYPos + 1] == var2() && mem_[wCurObjYPos + 0] == var1();
}

// Wario's within $28 pixels across (dx: its distance)
bool Omodonmeka::wario_near(uint8_t &dx)
{
	dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x22) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
	return dx < 0x28 || dx >= 0xd8;
}

void Omodonmeka::run(uint16_t handler)
{
	switch (handler) {
	case UPDATE: {
		uint8_t s = state();
		if (!s) {
			dispatch();
			return;
		}
		if (s == 0x2a) {
			got_on(wCurObjState);
			return;
		}
		if (hit(s))
			return;
		state() = 0;
		set_substate(0xf0, 0x04);
		var3() = 0;
		var2() = 0;
		set_frameset(ADDR_Frameset_68f34);
		dispatch();
		return;
	}
	case CARRY_WAIT: { // lines up with Wario
		if (auto_move_over())
			return;
		uint8_t d = --state_duration();
		if (d == 0x10)
			wario_above(0x0e);
		else if (d == 0x07)
			wario_above(0x16);
		return;
	}
	case CARRY_UP: { // rises with Wario until the ceiling or its top position
		if (auto_move_over())
			return;
		uint16_t y_pos = static_cast<uint16_t>(y() - 0x1d);
		mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
		mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		bool stop = hpos_block_unk3() & 0x0f;
		if (!stop)
			stop = y() == static_cast<uint16_t>((var1() | var2() << 8) - 0x40);
		if (stop) {
			set_substate(0xf0, 0x06);
			var3() = 0;
			set_frameset(ADDR_Frameset_68f14);
			return;
		}
		if (!(mem_[wGlobalCounter] & 0x07))
			play_sfx(SFX_087);
		move_wario(ADDR_MoveWarioUp, 0x01);
		home::handle_upwards_floor_transition(gb_);
		move_up();
		return;
	}
	case CARRY_HOVER: {
		if (auto_move_over())
			return;
		apply_y_movement_loop(ADDR_Data_604c0);
		uint16_t wario_y = static_cast<uint16_t>(y() + 0x1a);
		mem_[wWarioYPos + 1] = static_cast<uint8_t>(wario_y);
		mem_[wWarioYPos + 0] = static_cast<uint8_t>(wario_y >> 8);
		return;
	}
	case HOVER: { // opens its mouth when Wario's close and above
		apply_y_movement_loop(ADDR_Data_604c0);
		if (!(flags() & (1 << OBJFLAG_ON_SCREEN_F)))
			return;
		if (mem_[wTransformation] == TRANSFORMATION_PUFFY_WARIO)
			return;
		uint8_t dx;
		if (!wario_near(dx))
			return;
		if (static_cast<uint8_t>(screen_y() + 0x2a) >= static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a))
			return;
		set_frameset(ADDR_Frameset_68f1d);
		set_substate(0xf0, 0x08);
		play_sfx(SFX_08C);
		return;
	}
	case MOUTH_OPEN: { // closes it when Wario moves away, or swallows him
		if (!(mem_[wGlobalCounter] & 0x1f))
			play_sfx(SFX_08C);
		apply_y_movement_loop(ADDR_Data_604c0);
		uint8_t dx;
		if (wario_near(dx)) {
			bool above = static_cast<uint8_t>(screen_y() + 0x2a) < static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a);
			if (dx < 0x10 || dx >= 0xf0) {
				if (!above)
					return;
				set_frameset(ADDR_Frameset_68f2b);
				set_substate(0xf0, 0x02);
				var3() = 0;
				mem_[wCurObjCollBoxBottom] = 0x00;
				play_sfx(SFX_088);
				return;
			}
			if (above)
				return;
		}
		set_substate(0xf0, 0x01);
		set_frameset(ADDR_Frameset_68f0b);
		return;
	}
	case FALL_BACK: // falls back to the ground
		apply_y_movement(ADDR_Data_60500);
		mem_[hYPosLo] = mem_[wCurObjYPos + 0];
		mem_[hYPosHi] = mem_[wCurObjYPos + 1];
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		if (!hpos_block_unk1())
			return;
		mem_[wCurObjYPos + 0] = mem_[hYPosLo];
		mem_[wCurObjYPos + 1] = mem_[hYPosHi];
		set_interaction(OBJ_INTERACTION_01);
		mem_[wCurObjCollBoxBottom] = 0xf9;
		set_frameset(ADDR_Frameset_68f14);
		set_substate(0xf0, 0x03);
		return;
	case RISE_BACK: // rises back to its starting position
		if (!at_start()) {
			move_up();
			return;
		}
		run(START_HOVER);
		return;
	case START_HOVER:
		set_substate(OBJSUBFLAGS_MASK, 0x1);
		var3() = 0;
		set_frameset(ADDR_Frameset_68f0b);
		set_interaction(OBJ_INTERACTION_OMODONMEKA);
		return;
	case KNOCKED:
		no_actions();
		apply_y_movement(ADDR_Data_604e0);
		apply_x_movement_loop(ADDR_Data_60650);
		if (mem_[wTransformation] != TRANSFORMATION_FLAT_WARIO)
			return;
		if (!at_start())
			return;
		mem_[wCurObjCollBoxBottom] = 0xf9;
		run(START_HOVER);
		return;
	default:
		unrealistic();
		return;
	}
}

// spawns the Omodon riding it (unless it's already about)
void Omodonmeka::with_omodon_start(uint16_t omodon)
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(ADDR_OmodonmekaWithOmodonFunc);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 0xfd;
	mem_[wCurObjCollBoxLeft] = 0xf3;
	mem_[wCurObjCollBoxBottom] = 0xf9;
	if (same_transient_objects()) {
		swallow();
		return;
	}
	create_object_at_offset(omodon);
	set_frameset(ADDR_Frameset_68f14);
	state_duration() = 0;
	var3() = 0;
	run_owo(WAIT_STEP);
}

void Omodonmeka::with_omodon()
{
	uint8_t s = state();
	if (!s) {
		uint8_t i = substate() & 0x0f;
		if (i >= NUM_OWO_HANDLERS) {
			unrealistic();
			return;
		}
		run_owo(OWO_HANDLERS[i]);
		return;
	}
	if (hit(s))
		return;
	state() = 0x3a;
	var3() = 0;
	var2() = 0;
	set_frameset(ADDR_Frameset_68f34);
	state_duration() = 0x02;
	set_update(OWO_KNOCKED);
	run_owo(OWO_KNOCKED);
}

void Omodonmeka::swallow()
{
	set_substate(OBJSUBFLAGS_MASK, 0x3);
	set_frameset(ADDR_Frameset_68f2b);
	state_duration() = 0x28;
}

// the active object (flags & mask == want) whose OBJ_UNK_01/02 match its
// own (the one that spawned it); the slot's offset, or -1
int Omodonmeka::find(uint8_t mask, uint8_t want) const
{
	uint8_t lo = mem_[wCurObjUnk01], hi = mem_[wCurObjUnk02];
	for (int base = 0; base < 0x100; base += 0x20) {
		uint16_t at = static_cast<uint16_t>((wObj1 & 0xff00) | base);
		if ((mem_[at] & mask) == want && mem_[at + 1] == lo && mem_[at + 2] == hi)
			return base;
	}
	return -1;
}

void Omodonmeka::run_owo(uint16_t handler)
{
	switch (handler) {
	case WAIT_STEP: // waits for Wario to step on it
		if (mem_[wTransformation] == TRANSFORMATION_FLAT_WARIO) {
			bob();
			return;
		}
		set_substate(OBJSUBFLAGS_MASK, 0x2);
		set_frameset(ADDR_Frameset_68f26);
		state_duration() = 0x3c;
		flags() &= ~OBJFLAG_STEPPED;
		return;
	case STEPPED:
		if (--state_duration())
			return;
		swallow();
		return;
	case SWALLOW:
		swallow();
		return;
	case HAND_OVER: { // lines up with its Omodon, then hands Wario to it
		if (state_duration() == 0x28) {
			int base = find(0x41, 0x41);
			if (base < 0)
				return;
			uint8_t sy = mem_[(wObj1 & 0xff00) | (base + OBJ_SCREEN_Y_POS)];
			if (static_cast<uint8_t>(sy - 0x10) != screen_y() && static_cast<uint8_t>(sy - 0x0f) != screen_y()) {
				move_down();
				return;
			}
		}
		if (--state_duration())
			return;
		set_substate(0xf0, 0x04);
		set_frameset(ADDR_Frameset_68f14);
		state_duration() = 0x32;
		int base = find(0x41, 0x41);
		if (base < 0)
			return;
		uint16_t omodon = static_cast<uint16_t>((wObj1 & 0xff00) | base);
		mem_[omodon + OBJ_STATE] = 0x2d;
		mem_[omodon + OBJ_STATE_DURATION] = 0x33;
		mem_[omodon + OBJ_FRAMESET_OFFSET] = 0;
		mem_[omodon + OBJ_FRAME_DURATION] = 0;
		mem_[omodon + OBJ_FRAMESET_PTR + 1] = 0x4f;
		mem_[omodon + OBJ_FRAMESET_PTR + 0] = 0x42;
		mem_[omodon + OBJ_COLLBOX_BOTTOM] = 0x10;
		--mem_[omodon + OBJ_COLLBOX_LEFT];
		++mem_[omodon + OBJ_COLLBOX_RIGHT];
		return;
	}
	case RISE: { // rises, carrying Wario if he's on it
		if (flags() & OBJFLAG_STEPPED) {
			if (mem_[wGlobalCounter] & 1)
				return;
			move_wario(ADDR_MoveWarioUp, 0x01);
			home::handle_upwards_floor_transition(gb_);
		}
		if (dec_at(wCurObjStateDuration)) {
			move_up();
			return;
		}
		set_substate(0xf0, 0x01);
		var3() = 0;
		return;
	}
	case OWO_KNOCKED:
		no_actions();
		apply_y_movement(ADDR_Data_604e0);
		apply_x_movement_loop(ADDR_Data_60650);
		return;
	default:
		unrealistic();
		return;
	}
}

// bobs up and down along Data_40000 (flat Wario on it)
void Omodonmeka::bob()
{
	uint8_t i = var3();
	uint8_t a = mem_[static_cast<uint16_t>((ADDR_Data_40000 & 0xff00) | static_cast<uint8_t>(i + ADDR_Data_40000))];
	if (a == 0x80) {
		uint8_t c = static_cast<uint8_t>(static_cast<uint8_t>(i + ADDR_Data_40000) - 0x3f);
		a = mem_[static_cast<uint16_t>((ADDR_Data_40000 & 0xff00) | c)];
		var3() = 0x01;
		--state_duration();
	} else {
		++var3();
	}
	if (!a)
		return;
	y() = static_cast<uint16_t>(y() + static_cast<int8_t>(a));
}

// a block for a while, then stands again
void Omodonmeka::block()
{
	flags() &= ~OBJFLAG_STEPPED;
	if (!state_duration())
		return;
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_68f50);
}

void Omodonmeka::omodon_start()
{
	flags() |= OBJFLAG_NO_COLLISION;
	set_update(OMODON_UPDATE);
	omodon();
}

void Omodonmeka::above_wario()
{
	wario_above(0x18);
}

// a block on the ground
void Omodonmeka::omodon_land()
{
	state_duration() = 0x1c;
	set_interaction(OBJ_INTERACTION_SOLID);
	state() = 0x2f;
}

void Omodonmeka::omodon()
{
	find_owner_state();
	switch (state()) {
	case 0x00:
		omodon_idle();
		return;
	case 0x2d: // rises with Wario in it
		owner_stepped();
		if (state_duration() == 0x33) {
			--state_duration();
			return;
		}
		if ((flags() & OBJFLAG_STEPPED) && (mem_[wGlobalCounter] & 1))
			return;
		if (--state_duration()) {
			move_up();
			return;
		}
		set_frameset(ADDR_Frameset_68f50);
		state() = 0;
		var3() = 0;
		mem_[wCurObjCollBoxBottom] = 0x0b;
		return;
	case 0x2e: { // falls, and shakes the ground when it lands
		apply_y_movement(ADDR_Data_605e0);
		uint16_t y_pos = static_cast<uint16_t>(y() + 0x0f);
		mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
		mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		if (!hpos_block_unk1())
			return;
		y() = static_cast<uint16_t>((mem_[hYPosLo] | mem_[hYPosHi] << 8) - 0x0f);
		play_sfx(SFX_063);
		ground_shake(0x18);
		set_frameset(ADDR_Frameset_68f45);
		omodon_land();
		return;
	}
	case 0x2f:
		block();
		return;
	case 0x2a: // .Func_4284d
		set_frameset(ADDR_Frameset_68f45);
		state_duration() = 0x14;
		state() = 0x5a;
		above_wario();
		return;
	case 0x5a: // .Func_4286b
		if (state_duration() == 0x14) {
			if (mem_[wAutoMoveState]) {
				call_home(ADDR_MoveObjectDown_Fast);
				return;
			}
			above_wario();
		}
		if (--state_duration())
			return;
		mem_[wCurObjAction] = (1 << NO_ACTIONS_F) | 31; // no_actions_for 31
		omodon_land();
		return;
	default:
		state() = 0;
		omodon_idle();
		return;
	}
}

void Omodonmeka::omodon_idle()
{
	flags() |= OBJFLAG_NO_COLLISION;
	if (mem_[wTransformation] == TRANSFORMATION_FLAT_WARIO) {
		bob();
		owner_stepped();
		flags() &= ~OBJFLAG_STEPPED;
		return;
	}
	omodon_fall();
}

void Omodonmeka::omodon_fall()
{
	state() = 0x2e;
	var3() = 0;
	set_frameset(ADDR_Frameset_68f42);
	set_interaction(OBJ_INTERACTION_OMODON);
	++mem_[wCurObjCollBoxLeft];
	--mem_[wCurObjCollBoxRight];
}

// passes being stepped on to the Omodonmeka it rides; falls if that's
// knocked
void Omodonmeka::owner_stepped()
{
	int base = find(0x43, 0x03);
	if (base < 0)
		return;
	uint16_t owner = static_cast<uint16_t>((wObj1 & 0xff00) | base);
	mem_[owner] = (mem_[owner] & ~OBJFLAG_STEPPED) | (flags() & OBJFLAG_STEPPED);
	if (mem_[owner + OBJ_STATE] != 0x3a)
		return;
	omodon_fall();
}

// substate $10 while the Omodonmeka it rides is about, else 0
void Omodonmeka::find_owner_state()
{
	substate() = find(0x41, 0x01) < 0 ? 0 : 0x10;
}

} // namespace wl3::level

using wl3::level::Omodonmeka;

PORT_EX(OmodonmekaFunc, 0, ALL_REGISTERS) { Omodonmeka(gb).start(); }
PORT_LOCAL_EX(OmodonmekaFunc, Func_42199, 0, ALL_REGISTERS) { Omodonmeka(gb).got_on(REG_HL(gb)); }
PORT_LOCAL_EX(OmodonmekaFunc, Func_42218, 0, ALL_REGISTERS) { Omodonmeka(gb).run(wl3::level::UPDATE); }
PORT_LOCAL_EX(OmodonmekaFunc, Func_42261, 0, ALL_REGISTERS) { Omodonmeka(gb).run(wl3::level::CARRY_WAIT); }
PORT_LOCAL_EX(OmodonmekaFunc, Func_42291, 0, ALL_REGISTERS) { Omodonmeka(gb).run(wl3::level::CARRY_UP); }
PORT_LOCAL_EX(OmodonmekaFunc, Func_422fc, 0, ALL_REGISTERS) { Omodonmeka(gb).run(wl3::level::CARRY_HOVER); }
PORT_LOCAL_EX(OmodonmekaFunc, Func_42321, 0, ALL_REGISTERS) { Omodonmeka(gb).run(wl3::level::HOVER); }
PORT_LOCAL_EX(OmodonmekaFunc, Func_42369, 0, ALL_REGISTERS) { Omodonmeka(gb).run(wl3::level::MOUTH_OPEN); }
PORT_LOCAL_EX(OmodonmekaFunc, Func_423e1, 0, ALL_REGISTERS) { Omodonmeka(gb).run(wl3::level::FALL_BACK); }
PORT_LOCAL_EX(OmodonmekaFunc, Func_42420, 0, ALL_REGISTERS) { Omodonmeka(gb).run(wl3::level::RISE_BACK); }
PORT_LOCAL_EX(OmodonmekaFunc, Func_42432, 0, ALL_REGISTERS) { Omodonmeka(gb).run(wl3::level::START_HOVER); }
PORT_LOCAL_EX(OmodonmekaFunc, Func_4244c, 0, ALL_REGISTERS) { Omodonmeka(gb).run(wl3::level::KNOCKED); }
PORT_EX(OmodonmekaWithOmodon1Func, 0, ALL_REGISTERS) { Omodonmeka(gb).with_omodon_start(ADDR_ObjParams_Omodon1); }
PORT_EX(OmodonmekaWithOmodon2Func, 0, ALL_REGISTERS) { Omodonmeka(gb).with_omodon_start(ADDR_ObjParams_Omodon2); }
PORT_EX(OmodonmekaWithOmodonFunc, 0, ALL_REGISTERS) { Omodonmeka(gb).with_omodon(); }
PORT_LOCAL_EX(OmodonmekaWithOmodonFunc, Func_42535, 0, ALL_REGISTERS) { Omodonmeka(gb).run_owo(wl3::level::WAIT_STEP); }
PORT_LOCAL_EX(OmodonmekaWithOmodonFunc, Func_42554, 0, ALL_REGISTERS) { Omodonmeka(gb).run_owo(wl3::level::STEPPED); }
PORT_LOCAL_EX(OmodonmekaWithOmodonFunc, Func_42559, 0, ALL_REGISTERS) { Omodonmeka(gb).run_owo(wl3::level::SWALLOW); }
PORT_LOCAL_EX(OmodonmekaWithOmodonFunc, Func_4256c, 0, ALL_REGISTERS) { Omodonmeka(gb).run_owo(wl3::level::HAND_OVER); }
PORT_LOCAL_EX(OmodonmekaWithOmodonFunc, Func_426ee, 0, ALL_REGISTERS) { Omodonmeka(gb).run_owo(wl3::level::RISE); }
PORT_LOCAL_EX(OmodonmekaWithOmodonFunc, Func_42714, 0, ALL_REGISTERS) { Omodonmeka(gb).run_owo(wl3::level::OWO_KNOCKED); }
PORT_EX(Func_42725, 0, ALL_REGISTERS) { Omodonmeka(gb).bob(); }
PORT_EX(Func_42754, 0, ALL_REGISTERS) { Omodonmeka(gb).block(); }
PORT_EX(Func_42767, 0, ALL_REGISTERS) { Omodonmeka(gb).omodon_idle(); }
PORT_LOCAL_EX(Func_42767, asm_4277f, 0, ALL_REGISTERS) { Omodonmeka(gb).omodon_fall(); }
PORT_EX(Func_4279c, 0, ALL_REGISTERS) { Omodonmeka(gb).omodon_start(); }
PORT_LOCAL_EX(Func_4279c, Func_427a9, 0, ALL_REGISTERS) { Omodonmeka(gb).omodon(); }
PORT_LOCAL_EX(Func_4279c, Func_42894, 0, ALL_REGISTERS) { Omodonmeka(gb).owner_stepped(); }
PORT_LOCAL_EX(Func_4279c, Func_42935, 0, ALL_REGISTERS) { Omodonmeka(gb).find_owner_state(); }
