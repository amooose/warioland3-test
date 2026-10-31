// Pneumo (pneumo.asm): a bubble creature hopping in place; when Wario's
// above it, it hops towards him and floats up. Touching it (unless he's
// already puffy) inflates it, and it carries him up. Its mode is the
// substate's low nibble. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t UPDATE = ADDR_PneumoFunc + 0x41; // .Update

enum : uint8_t {
	HOPPING = 1, // .Func_4570f (0 too)
	INFLATED = 2, // .Func_458c7
	WALKING = 3, // .Func_457bd
	FLOATING = 4, // .Func_4585c
	DRIFTING = 5, // .Func_45893
	RISING = 6, // .Func_45934
};

class Pneumo : public Object {
public:
	using Object::Object;

	void init(); // PneumoFunc
	void update(); // .Update
	void run(uint8_t mode);
	void reset(); // .Func_456fe
	void hopping(); // .Func_4570f
	void walking(); // .Func_457bd
	void floating(); // .Func_4585c
	void drifting(); // .Func_45893
	void inflate(); // .Func_4589d
	void inflated(); // .Func_458c7
	void deflate(); // .Func_4592e
	void rising(); // .Func_45934

private:
	bool puffy() const { return mem_[wTransformation] == TRANSFORMATION_PUFFY_WARIO; }
	void set_mode(uint8_t m) { substate() = (substate() & OBJSUBFLAGS_MASK) | m; }
	void hpos(int dy, int dx);
	void knocked(uint16_t routine); // .asm_456e8
	void walk_on(); // .asm_4581d
	void set_floating(); // .asm_45801
};

} // namespace

void Pneumo::init()
{
	flags() &= ~OBJFLAG_INVISIBLE;
	set_update(UPDATE);
	set_frameset(ADDR_Frameset_68c5c);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	var3() = 0;
	var1() = 0;
	mem_[wCurObjCollBoxRight] = 0x07;
	mem_[wCurObjCollBoxLeft] = 0xf8;
	mem_[wCurObjCollBoxBottom] = 0x02;
}

void Pneumo::hpos(int dy, int dx)
{
	uint16_t y_pos = static_cast<uint16_t>(y() + dy);
	uint16_t x_pos = static_cast<uint16_t>(x() + dx);
	mem_[hYPosLo] = static_cast<uint8_t>(y_pos);
	mem_[hYPosHi] = static_cast<uint8_t>(y_pos >> 8);
	mem_[hXPosLo] = static_cast<uint8_t>(x_pos);
	mem_[hXPosHi] = static_cast<uint8_t>(x_pos >> 8);
}

void Pneumo::knocked(uint16_t routine)
{
	set_update(routine);
	var3() = 0;
	var2() = 0x02;
	set_frameset(ADDR_Frameset_68c89);
	no_actions();
}

void Pneumo::update()
{
	uint8_t s = state();
	if (s) {
		if (s == 0x0e) {
			state() = 0;
			inflate();
			return;
		}
		if (s == 0x0a) {
			vanish();
			return;
		}
		if (s < 0x04) { // touched
			state() = 0;
			if (puffy() || (substate() & 0x0f) == 1) {
				inflate();
				return;
			}
		} else {
			switch (s) {
			case 0x28: knocked(ADDR_Func_33f8); return;
			case 0x29: knocked(ADDR_Func_3407); return;
			case 0x04: knocked(ADDR_Func_3326); return;
			case 0x05: knocked(ADDR_Func_3317); return;
			default: knocked((s & 1) ? ADDR_Func_33f8 : ADDR_Func_3407); return;
			}
		}
	}
	uint8_t m = substate() & 0x0f;
	if (m > RISING) {
		unrealistic();
		return;
	}
	run(m);
}

void Pneumo::run(uint8_t m)
{
	switch (m) {
	case 0: case HOPPING: hopping(); return;
	case INFLATED: inflated(); return;
	case WALKING: walking(); return;
	case FLOATING: floating(); return;
	case DRIFTING: drifting(); return;
	case RISING: rising(); return;
	}
}

void Pneumo::reset()
{
	set_frameset(ADDR_Frameset_68c5c);
	set_mode(HOPPING);
	var3() = 0;
}

// hops in place by Data_44000 (var3 its step); each hop's end, it hops
// towards Wario if he's above it, close enough; now and then, with him
// below it, it rises
void Pneumo::hopping()
{
	uint8_t below = static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a);
	if (below >= static_cast<uint8_t>(screen_y() + 0x2a) && below < 0x61 && !puffy()
		&& !(mem_[wGlobalCounter] & 0x3f)) {
		set_mode(RISING);
		return;
	}
	for (;;) {
		uint16_t step = static_cast<uint16_t>((ADDR_Data_44000 & 0xff00) | static_cast<uint8_t>((ADDR_Data_44000 & 0xff) + var3()));
		uint8_t v = mem_[step];
		if (v != 0x80) {
			++var3();
			y() = static_cast<uint16_t>(y() + static_cast<int8_t>(v));
			return;
		}
		if (!puffy()) {
			uint8_t dy = static_cast<uint8_t>(static_cast<uint8_t>(screen_y() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a));
			if (dy >= 0xb0 && dy < 0xec) {
				uint8_t dx = static_cast<uint8_t>(static_cast<uint8_t>(screen_x() + 0x2a) - static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a));
				if (dx < 0x60) {
					var2() = dx >= 0x10 ? 1 : 0;
					substate() &= ~OBJSUBFLAG_HDIR;
					set_mode(WALKING);
					set_frameset(ADDR_Frameset_68c7f);
					state_duration() = 1;
					return;
				}
				if (dx >= SCREEN_WIDTH_PX) {
					var2() = dx < 0xf0 ? 1 : 0;
					substate() |= OBJSUBFLAG_HDIR;
					set_mode(WALKING);
					set_frameset(ADDR_Frameset_68c84);
					state_duration() = 1;
					return;
				}
			}
		}
		var3() = 0;
	}
}

void Pneumo::set_floating()
{
	set_mode(FLOATING);
	state_duration() = 255;
}

// a step towards Wario; floats up once in water or he's not below
void Pneumo::walking()
{
	if (state_duration()) {
		--state_duration();
		return;
	}
	mem_[wc1ca] = 0;
	hpos(0, 0);
	routines_.call(ADDR_Func_352b);
	bool on_floor = gb_->cpu.a ? mem_[wc1ca] : !mem_[wIsInWaterOrSand];
	if (!on_floor) {
		set_floating();
		return;
	}
	uint8_t below = static_cast<uint8_t>(static_cast<uint8_t>(mem_[wWarioScreenYPos] + 0x2a) - static_cast<uint8_t>(screen_y() + 0x2a));
	if (below < 0x14 || below >= 0x80) {
		set_floating();
		return;
	}
	++var1();
	y() = static_cast<uint16_t>(y() + 2);
	walk_on();
}

// var2 pixels along, turning at walls
void Pneumo::walk_on()
{
	bool right = facing_right();
	hpos(-4, right ? 7 : -8);
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a) {
		substate() ^= OBJSUBFLAG_HDIR;
		return;
	}
	right ? move_right_by_var2() : move_left_by_var2();
}

// back up by var1 steps (unless something's above)
void Pneumo::floating()
{
	if (!--state_duration()) {
		reset();
		return;
	}
	hpos(-16, 0);
	routines_.call(ADDR_Func_358b);
	if (!gb_->cpu.a) {
		if (!var1()) {
			reset();
			return;
		}
		--var1();
		y() = static_cast<uint16_t>(y() - 2);
	}
	walk_on();
}

void Pneumo::drifting()
{
	if (--state_duration())
		walk_on();
	else
		reset();
}

// blown up (facing away from Wario)
void Pneumo::inflate()
{
	set_mode(INFLATED);
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	if (static_cast<uint8_t>(screen_x() + 0x2a) < wario)
		substate() &= ~OBJSUBFLAG_HDIR;
	else
		substate() |= OBJSUBFLAG_HDIR;
	var3() = 0;
	set_frameset(ADDR_Frameset_68c8e);
	state_duration() = 27;
}

// rising (unless blocked) and darting along, turning at walls
void Pneumo::inflated()
{
	if (!--state_duration()) {
		deflate();
		return;
	}
	hpos(-16, 0);
	routines_.call(ADDR_Func_358b);
	if (!gb_->cpu.a) {
		SET_BC(gb_, ADDR_Data_60600);
		routines_.call(ADDR_ApplyObjYMovement);
	}
	bool right = facing_right();
	hpos(-4, right ? 7 : -8);
	routines_.call(ADDR_Func_3513);
	if (!gb_->cpu.a) {
		routines_.call(right ? ADDR_MoveObjectRight_Fast : ADDR_MoveObjectLeft_Fast);
		return;
	}
	if (right)
		substate() &= ~OBJSUBFLAG_HDIR;
	else
		substate() |= OBJSUBFLAG_HDIR;
}

void Pneumo::deflate()
{
	var1() = 0;
	reset();
}

// to Wario's height (by 4 pixels, plus hXPosLoBackup's address: the
// asm adds the label), inflating at a ceiling
void Pneumo::rising()
{
	if (puffy()) {
		reset();
		return;
	}
	uint16_t wario_y = static_cast<uint16_t>(mem_[wWarioYPos + 0] << 8 | (mem_[wWarioYPos + 1] & 0xfc));
	uint16_t target = static_cast<uint16_t>(wario_y + hXPosLoBackup);
	uint8_t hi = mem_[wCurObjYPos + 1];
	bool up;
	if (hi == (target >> 8)) {
		uint8_t lo = mem_[wCurObjYPos + 0] & 0xfc;
		if (lo == (target & 0xff)) {
			reset();
			return;
		}
		up = lo > (target & 0xff);
	} else {
		up = hi > (target >> 8);
	}
	if (up) {
		hpos(-16, 0);
		routines_.call(ADDR_Func_358b);
		if (gb_->cpu.a)
			inflate();
		else
			move_up();
		return;
	}
	mem_[wc1ca] = 0;
	hpos(0, 0);
	routines_.call(ADDR_Func_352b);
	if (gb_->cpu.a) {
		if (mem_[wc1ca])
			routines_.call(ADDR_MoveObjectDown_Slow);
		else
			reset();
		return;
	}
	if (mem_[wIsInWaterOrSand])
		reset();
	else
		move_down();
}

} // namespace wl3::level

using Pneumo = wl3::level::Pneumo;

PORT_EX(PneumoFunc, 0, ALL_REGISTERS) { Pneumo(gb).init(); }
PORT_LOCAL_EX(PneumoFunc, Update, 0, ALL_REGISTERS) { Pneumo(gb).update(); }
PORT_LOCAL_EX(PneumoFunc, Func_456fe, 0, ALL_REGISTERS) { Pneumo(gb).reset(); }
PORT_LOCAL_EX(PneumoFunc, Func_4589d, 0, ALL_REGISTERS) { Pneumo(gb).inflate(); }
PORT_LOCAL_EX(PneumoFunc, Func_4570f, 0, ALL_REGISTERS) { Pneumo(gb).hopping(); }
PORT_LOCAL_EX(PneumoFunc, Func_457bd, 0, ALL_REGISTERS) { Pneumo(gb).walking(); }
PORT_LOCAL_EX(PneumoFunc, Func_4585c, 0, ALL_REGISTERS) { Pneumo(gb).floating(); }
PORT_LOCAL_EX(PneumoFunc, Func_45893, 0, ALL_REGISTERS) { Pneumo(gb).drifting(); }
PORT_LOCAL_EX(PneumoFunc, Func_458c7, 0, ALL_REGISTERS) { Pneumo(gb).inflated(); }
PORT_LOCAL_EX(PneumoFunc, Func_45934, 0, ALL_REGISTERS) { Pneumo(gb).rising(); }
