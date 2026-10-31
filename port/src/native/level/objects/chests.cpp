// The treasure chests and their treasures (chests.asm). See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t CLOSED = ADDR_ChestFunc + 0x24; // .Update
constexpr uint16_t DONE = ADDR_ChestFunc + 0x42; // .Ret
constexpr uint16_t OPENING = ADDR_ChestFunc + 0x43; // .Open

class Chest : public Object {
public:
	using Object::Object;

	void chest(uint8_t treasure); // GreyChestFunc and the like
	void init(); // ChestFunc
	void closed(); // .Update
	void opening(); // .Open
	void treasure(uint16_t frameset); // TreasureFunc
};

} // namespace

// none once its treasure's taken
void Chest::chest(uint8_t treasure)
{
	if (mem_[wKeyAndTreasureFlags] & treasure)
		flags() = 0; // DeinitChest
	else
		init();
}

void Chest::init()
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	set_update(CLOSED);
	substate() = (substate() & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F);
	set_frameset(ADDR_Frameset_68000);
	mem_[wCurObjCollBoxRight] = 13;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-14);
}

// until Wario opens it with the key
void Chest::closed()
{
	if (state() != OBJSTATE_18 || mem_[wAutoMoveState])
		return;
	set_frameset(ADDR_Frameset_68003);
	state_duration() = 86;
	set_update(OPENING);
}

// then the treasure comes out, to a fanfare
void Chest::opening()
{
	if (state_duration() == 73)
		play_sfx(SFX_035);
	if (--state_duration())
		return;
	set_frameset(ADDR_Frameset_68060);
	set_update(DONE);
	uint16_t params;
	switch (interaction() & INTERACTION_MASK) {
	case OBJ_INTERACTION_GREY_TREASURE: params = ADDR_ObjParams_GreyTreasure; break;
	case OBJ_INTERACTION_RED_TREASURE: params = ADDR_ObjParams_RedTreasure; break;
	case OBJ_INTERACTION_GREEN_TREASURE: params = ADDR_ObjParams_GreenTreasure; break;
	default: params = ADDR_ObjParams_BlueTreasure; break;
	}
	play_music2(mem_, MUSIC_TREASURE_FANFARE);
	SET_BC(gb_, params);
	routines_.call(ADDR_CreateObjectAtRelativePos);
}

// held still; its frameset once its time's up (if it has one)
void Chest::treasure(uint16_t frameset)
{
	no_actions();
	if (!state_duration())
		return;
	if (!--state_duration())
		set_frameset(frameset);
}

} // namespace wl3::level

using Chest = wl3::level::Chest;

PORT_EX(DeinitChest, 0, ALL_REGISTERS) { wl3::Memory mem(gb); mem[wCurObjFlags] = 0; }
PORT_EX(GreyChestFunc, 0, ALL_REGISTERS) { Chest(gb).chest(GREY_TREASURE); }
PORT_EX(RedChestFunc, 0, ALL_REGISTERS) { Chest(gb).chest(RED_TREASURE); }
PORT_EX(GreenChestFunc, 0, ALL_REGISTERS) { Chest(gb).chest(GREEN_TREASURE); }
PORT_EX(BlueChestFunc, 0, ALL_REGISTERS) { Chest(gb).chest(BLUE_TREASURE); }
PORT_EX(ChestFunc, 0, ALL_REGISTERS) { Chest(gb).init(); }
PORT_LOCAL_EX(ChestFunc, Update, 0, ALL_REGISTERS) { Chest(gb).closed(); }
PORT_LOCAL_EX(ChestFunc, Ret, 0, ALL_REGISTERS) {}
PORT_LOCAL_EX(ChestFunc, Open, 0, ALL_REGISTERS) { Chest(gb).opening(); }
PORT_EX(GreyTreasureFunc, 0, ALL_REGISTERS) { Chest(gb).treasure(ADDR_Frameset_681be); }
PORT_EX(RedTreasureFunc, 0, ALL_REGISTERS) { Chest(gb).treasure(ADDR_Frameset_681c3); }
PORT_EX(GreenTreasureFunc, 0, ALL_REGISTERS) { Chest(gb).treasure(ADDR_Frameset_681c8); }
PORT_EX(BlueTreasureFunc, 0, ALL_REGISTERS) { Chest(gb).treasure(ADDR_Frameset_681cd); }
PORT_EX(TreasureFunc, 0, ALL_REGISTERS) { Chest(gb).treasure(REG_DE(gb)); }
