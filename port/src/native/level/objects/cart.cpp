// The carts (cart.asm): platforms that roll off once Wario's on them,
// carrying him, until they hit a wall. The variable ones only roll with
// the truck wheel collected. See object.hpp.
#include "../../home/ow.hpp"
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t WAITING = ADDR_CartFunc + 0x25; // .Waiting
constexpr uint16_t MOVING = ADDR_CartFunc + 0x3f; // .Moving
constexpr uint16_t VARIABLE_WAITING = ADDR_CartVariableFunc + 0x3d; // .Waiting
constexpr uint16_t VARIABLE_MOVING = ADDR_CartVariableFunc + 0x63; // .Moving

class Cart : public Object {
public:
	explicit Cart(GB *gb) : Object(gb, BANK_CartFunc) {}

	void init(); // CartFunc
	void waiting(); // .Waiting
	void moving(); // .Moving
	void variable(); // CartVariableFunc
	void variable_waiting(); // CartVariableFunc.Waiting
	void variable_moving(); // CartVariableFunc.Moving

private:
	void set_up(uint16_t update, uint16_t frameset);
	bool stepped_on();
	void roll(bool right, uint16_t stopped);
};

} // namespace

void Cart::set_up(uint16_t update, uint16_t frameset)
{
	start(update, frameset);
	mem_.cur_obj_coll_box_right() = 0x0e;
	mem_.cur_obj_coll_box_left() = 0xf1;
	mem_.cur_obj_coll_box_bottom() = 0xf4;
}

void Cart::init() { set_up(WAITING, ADDR_Frameset_69199); }

void Cart::variable()
{
	uint8_t wram_bank = mem_[rWBK];
	mem_[rWBK] = 0x02;
	bool collected = home::is_treasure_collected(gb_, TRUCK_WHEEL);
	mem_[rWBK] = wram_bank;
	if (collected)
		set_up(VARIABLE_WAITING, ADDR_Frameset_691bf);
	else
		set_up(ADDR_CartStoppedFunc, ADDR_Frameset_691a7);
}

bool Cart::stepped_on()
{
	one_way_platform();
	if (!(flags() & OBJFLAG_STEPPED))
		return false;
	flags() &= ~OBJFLAG_STEPPED;
	return true;
}

void Cart::waiting()
{
	if (!stepped_on())
		return;
	set_update(MOVING);
	set_frameset(ADDR_Frameset_6919e);
}

void Cart::variable_waiting()
{
	if (!stepped_on())
		return;
	set_update(VARIABLE_MOVING);
	set_frameset(mem_.cur_obj_id() == CART_VARIABLE_RIGHT ? ADDR_Frameset_691aa : ADDR_Frameset_691b3);
}

void Cart::moving() { roll(mem_.cur_obj_id() == CART_RIGHT, ADDR_Frameset_69199); }
void Cart::variable_moving() { roll(mem_.cur_obj_id() == CART_VARIABLE_RIGHT, ADDR_Frameset_691bf); }

// a pixel every other frame, and Wario with it (unless he's blocked);
// stops at a wall in front
void Cart::roll(bool right, uint16_t stopped)
{
	one_way_platform();
	uint8_t counter = mem_.global_counter();
	if (!(counter & 0x3f))
		play_sfx(SFX_06D);
	if (counter & 1)
		return;
	uint16_t y_pos = static_cast<uint16_t>(y() - 0x04);
	uint16_t x_pos = right ? static_cast<uint16_t>(x() + 0x0e) : static_cast<uint16_t>(x() - 0x0f);
	mem_.y_pos_lo() = static_cast<uint8_t>(y_pos);
	mem_.y_pos_hi() = static_cast<uint8_t>(y_pos >> 8);
	mem_.x_pos_lo() = static_cast<uint8_t>(x_pos);
	mem_.x_pos_hi() = static_cast<uint8_t>(x_pos >> 8);
	if (hpos_block_unk4()) {
		set_frameset(stopped);
		set_update(ADDR_CartStoppedFunc);
		return;
	}
	right ? move_right() : move_left();
	if (!(flags() & OBJFLAG_STEPPED))
		return;
	flags() &= ~OBJFLAG_STEPPED;
	if (wario_bottom_collision(right))
		return;
	move_wario(right ? ADDR_MoveWarioRight : ADDR_MoveWarioLeft, 0x01);
}

} // namespace wl3::level

using Cart = wl3::level::Cart;

PORT_EX(CartFunc, 0, ALL_REGISTERS) { Cart(gb).init(); }
PORT_LOCAL_EX(CartFunc, Waiting, 0, ALL_REGISTERS) { Cart(gb).waiting(); }
PORT_LOCAL_EX(CartFunc, Moving, 0, ALL_REGISTERS) { Cart(gb).moving(); }
PORT_EX(CartStoppedFunc, 0, ALL_REGISTERS) { gb_call(gb, ADDR_SetOneWayPlatformAction); }
PORT_EX(CartVariableFunc, 0, ALL_REGISTERS) { Cart(gb).variable(); }
PORT_LOCAL_EX(CartVariableFunc, Waiting, 0, ALL_REGISTERS) { Cart(gb).variable_waiting(); }
PORT_LOCAL_EX(CartVariableFunc, Moving, 0, ALL_REGISTERS) { Cart(gb).variable_moving(); }
