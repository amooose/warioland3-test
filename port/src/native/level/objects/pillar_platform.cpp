// The pillar platforms (pillar_platform.asm): one-way platforms in
// columns of 3 heights. Hit from below (state $2b) one rises a step;
// pounded (state $0a) it sinks a step and its neighbours rise. See
// object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t RISING = ADDR_PillarPlatformFunc + 0x34; // .Func_49cc2
constexpr uint16_t SINKING = ADDR_PillarPlatformFunc + 0x40; // .Func_49cce
constexpr uint16_t IDLE = ADDR_PillarPlatformFunc + 0x57; // .Func_49ce5

constexpr uint8_t HIT_FROM_BELOW = 0x2b;
constexpr uint8_t POUNDED = 0x0a;
constexpr uint8_t PILLAR_ID = 0x20; // the neighbours' OBJ_ID
constexpr uint8_t TOP = 2; // var1: the step it's on

class PillarPlatform : public Object {
public:
	using Object::Object;

	void init(uint8_t height); // PillarPlatformFunc
	void init_offset(uint8_t height, int8_t dy); // PillarPlatform2Func, PillarPlatform3Func
	void moving(bool up); // .Func_49cc2, .Func_49cce
	void idle(); // .Func_49ce5
	void push_neighbours(uint8_t start, uint8_t b, uint8_t state, uint8_t unk01); // .Func_49d43
};

} // namespace

void PillarPlatform::init_offset(uint8_t height, int8_t dy)
{
	y() = static_cast<uint16_t>(y() + dy);
	init(height);
}

void PillarPlatform::init(uint8_t height)
{
	var1() = height;
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	set_update(IDLE);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 0x00;
	mem_[wCurObjCollBoxLeft] = 0xf9;
	mem_[wCurObjCollBoxBottom] = 0xf4;
	set_frameset((wram_.day_night() & 1) ? ADDR_Frameset_6975e : ADDR_Frameset_6975b);
}

// 9 frames of moving fast, then idle
void PillarPlatform::moving(bool up)
{
	one_way_platform();
	if (--state_duration()) {
		routines_.call(up ? ADDR_MoveObjectUp_Fast : ADDR_MoveObjectDown_Fast);
		return;
	}
	state() = 0;
	set_update(IDLE);
}

void PillarPlatform::idle()
{
	one_way_platform();
	uint8_t s = state();
	if (!s)
		return;
	if (s == HIT_FROM_BELOW) {
		if (var1() >= TOP) {
			state() = 0;
			return;
		}
		++var1();
		state_duration() = 0x09;
		set_update(RISING);
		return;
	}
	if (s != POUNDED) {
		state() = 0;
		return;
	}
	if (!var1()) {
		state() = 0;
		return;
	}
	--var1();
	state_duration() = 0x09;
	set_update(SINKING);
	uint8_t unk01 = mem_[wCurObjUnk01];
	push_neighbours(0x00, 0x00, HIT_FROM_BELOW, static_cast<uint8_t>(unk01 + 3));
	push_neighbours(0x00, 0x00, HIT_FROM_BELOW, static_cast<uint8_t>(unk01 - 3));
}

// .Func_49d43: gives state to each pillar platform among the objects
// whose OBJ_UNK_01 is unk01 (the slot after start, then each $20 from b
// up to $e0)
void PillarPlatform::push_neighbours(uint8_t start, uint8_t b, uint8_t state, uint8_t unk01)
{
	if (b & 0x1f) { // never reaches $e0: the asm loops forever
		unrealistic();
		return;
	}
	uint16_t page = wObjects & 0xff00;
	uint8_t slot = start;
	for (;;) {
		uint8_t l = static_cast<uint8_t>(OBJ_UNK_01 + slot);
		if (mem_[static_cast<uint16_t>(page | l)] == unk01) {
			l = static_cast<uint8_t>(l + 6);
			if (mem_[static_cast<uint16_t>(page | l)] == PILLAR_ID)
				mem_[static_cast<uint16_t>(page | static_cast<uint8_t>(l + 0x14))] = state;
		}
		if (b == 0xe0)
			return;
		b = static_cast<uint8_t>(b + 0x20);
		slot = b;
	}
}

} // namespace wl3::level

using PillarPlatform = wl3::level::PillarPlatform;

PORT_EX(PillarPlatform1Func, 0, ALL_REGISTERS) { PillarPlatform(gb).init(1); }
PORT_EX(PillarPlatform2Func, 0, ALL_REGISTERS) { PillarPlatform(gb).init_offset(2, -0x10); }
PORT_EX(PillarPlatform3Func, 0, ALL_REGISTERS) { PillarPlatform(gb).init_offset(0, 0x10); }
PORT_EX(PillarPlatformFunc, 0, ALL_REGISTERS) { PillarPlatform(gb).init(gb->cpu.b); }
PORT_LOCAL_EX(PillarPlatformFunc, Func_49cc2, 0, ALL_REGISTERS) { PillarPlatform(gb).moving(true); }
PORT_LOCAL_EX(PillarPlatformFunc, Func_49cce, 0, ALL_REGISTERS) { PillarPlatform(gb).moving(false); }
PORT_LOCAL_EX(PillarPlatformFunc, Func_49ce5, 0, ALL_REGISTERS) { PillarPlatform(gb).idle(); }
PORT_LOCAL_EX(PillarPlatformFunc, Func_49d43, 0, ALL_REGISTERS)
{
	PillarPlatform(gb).push_neighbours(gb->cpu.a, gb->cpu.b, gb->cpu.c, gb->cpu.e);
}
