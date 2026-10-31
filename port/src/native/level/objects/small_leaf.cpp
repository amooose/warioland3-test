// The small leaf (small_leaf.asm): a one-way platform that wilts once
// Wario's stood on it, then comes back. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t REGROWN = ADDR_SmallLeafFunc + 0x25; // .Update4
constexpr uint16_t IDLE = ADDR_SmallLeafFunc + 0x31; // .Update1
constexpr uint16_t WILTING = ADDR_SmallLeafFunc + 0x60; // .Update2
constexpr uint16_t GONE = ADDR_SmallLeafFunc + 0x90; // .Update3

class SmallLeaf : public Object {
public:
	using Object::Object;

	void init(); // SmallLeafFunc
	// .Init: its update function, through h's page (wCurObj's from its callers)
	void reset(uint16_t page = wCurObjUpdateFunction & 0xff00);
	void regrown(); // .Update4
	void idle(); // .Update1
	void wilting(); // .Update2
	void gone(); // .Update3

private:
	bool attacked(); // vanishes (Func_3173) if so; else clears the state
};

} // namespace

void SmallLeaf::init()
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 0x06;
	mem_[wCurObjCollBoxLeft] = 0xf9;
	mem_[wCurObjCollBoxBottom] = 0xf3;
	reset();
}

void SmallLeaf::reset(uint16_t page)
{
	// high byte first, as the asm (h may point at the MBC's registers)
	mem_[static_cast<uint16_t>(page | (OBJ_UPDATE_FUNCTION + 1))] = IDLE >> 8;
	mem_[static_cast<uint16_t>(page | OBJ_UPDATE_FUNCTION)] = IDLE & 0xff;
	set_frameset(ADDR_Frameset_68a9f);
}

void SmallLeaf::regrown()
{
	no_actions();
	if (!--state_duration())
		reset();
}

bool SmallLeaf::attacked()
{
	uint8_t s = state();
	if (s == 0x0a || (s & 0xfe) == 0x04) {
		vanish();
		return true;
	}
	state() = 0;
	return false;
}

// until Wario steps on it
void SmallLeaf::idle()
{
	one_way_platform();
	if (attacked())
		return;
	if (!(flags() & OBJFLAG_STEPPED))
		return;
	flags() &= ~OBJFLAG_STEPPED;
	set_frameset(ADDR_Frameset_68aa8);
	state_duration() = 0x28;
	set_update(WILTING);
}

// no longer solid for its last frames
void SmallLeaf::wilting()
{
	if (attacked())
		return;
	if (state_duration() < 0x0f)
		no_actions();
	if (--state_duration())
		return;
	state_duration() = 0x9b;
	flags() = (flags() & ~OBJFLAG_STEPPED) | OBJFLAG_INVISIBLE;
	set_update(GONE);
}

void SmallLeaf::gone()
{
	no_actions();
	if (--state_duration())
		return;
	flags() &= ~OBJFLAG_INVISIBLE;
	set_frameset(ADDR_Frameset_68abb);
	state_duration() = 0x28;
	set_update(REGROWN);
}

} // namespace wl3::level

using SmallLeaf = wl3::level::SmallLeaf;

PORT_EX(SmallLeafFunc, 0, ALL_REGISTERS) { SmallLeaf(gb).init(); }
PORT_LOCAL_EX(SmallLeafFunc, Init, 0, ALL_REGISTERS) { SmallLeaf(gb).reset(gb->cpu.h << 8); }
PORT_LOCAL_EX(SmallLeafFunc, Update4, 0, ALL_REGISTERS) { SmallLeaf(gb).regrown(); }
PORT_LOCAL_EX(SmallLeafFunc, Update1, 0, ALL_REGISTERS) { SmallLeaf(gb).idle(); }
PORT_LOCAL_EX(SmallLeafFunc, Update2, 0, ALL_REGISTERS) { SmallLeaf(gb).wilting(); }
PORT_LOCAL_EX(SmallLeafFunc, Update3, 0, ALL_REGISTERS) { SmallLeaf(gb).gone(); }
