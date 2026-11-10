// The levels' objects: the object being updated. See object.hpp.
#include "object.hpp"
#include "../home/wario.hpp"
#include "../home/floor_transition.hpp"

#include <cstdlib>

namespace wl3::level {

void Object::set_coll_box(uint8_t top, uint8_t bottom, uint8_t left, uint8_t right)
{
	mem_.cur_obj_coll_box_top() = top;
	mem_.cur_obj_coll_box_bottom() = bottom;
	mem_.cur_obj_coll_box_left() = left;
	mem_.cur_obj_coll_box_right() = right;
}

void Object::set_frameset(uint16_t frameset)
{
	home::set_object_frameset_ptr(gb_, frameset);
}

void Object::call_home(uint16_t routine)
{
	if (object_state(gb_, routine))
		return;
	bool odd = mem_.global_counter() & 1;
	switch (routine) {
	case ADDR_MoveObjectLeft: move_left(); break;
	case ADDR_MoveObjectRight: move_right(); break;
	case ADDR_MoveObjectUp: move_up(); break;
	case ADDR_MoveObjectDown: move_down(); break;
	case ADDR_MoveObjectLeft_Fast: home::move_object_x(gb_, -2); break;
	case ADDR_MoveObjectRight_Fast: home::move_object_x(gb_, 2); break;
	case ADDR_MoveObjectUp_Fast: home::move_object_y(gb_, -2); break;
	case ADDR_MoveObjectDown_Fast: home::move_object_y(gb_, 2); break;
	case ADDR_MoveObjectLeft_Slow: if (odd) move_left(); break;
	case ADDR_MoveObjectRight_Slow: if (!odd) move_right(); break;
	case ADDR_MoveObjectUp_Slow: if (odd) move_up(); break;
	case ADDR_MoveObjectDown_Slow: if (!odd) move_down(); break;
	case ADDR_MoveObjectLeftByVar2: move_left_by_var2(); break;
	case ADDR_MoveObjectRightByVar2: move_right_by_var2(); break;
	case ADDR_MoveObjectUpByVar2: move_up_by_var2(); break;
	case ADDR_MoveObjectDownByVar2: move_down_by_var2(); break;
	case ADDR_DecrementRemainingBossHits: decrement_boss_hits(); break;
	case ADDR_HandleUpwardsFloorTransition: home::handle_upwards_floor_transition(gb_); break;
	case ADDR_HandleDownwardsFloorTransition: home::handle_downwards_floor_transition(gb_); break;
	default: routines_.call(routine); break;
	}
}

void Object::move_wario(uint16_t routine, uint8_t n)
{
	switch (routine) {
	case ADDR_MoveWarioRight: home::move_wario_x(gb_, n); break;
	case ADDR_MoveWarioLeft: home::move_wario_x(gb_, -n); break;
	case ADDR_MoveWarioDown: home::move_wario_y(gb_, n); break;
	case ADDR_MoveWarioUp: home::move_wario_y(gb_, -n); break;
	default: std::abort(); // the callers pass only the four above
	}
}

void Object::despawn()
{
	routines_.in_bank(BANK_DespawnObject, ADDR_DespawnObject, [&] { despawn_object(gb_, wCurObjUnk02); });
}

void Object::set_block_object(uint8_t nybble)
{
	routines_.in_bank(BANK_Func_ba42, ADDR_Func_ba42, [&] { set_object_block(gb_, wCurObjUnk02, nybble); });
}

void Object::collect()
{
	routines_.in_bank(BANK_Func_bb2d, ADDR_Func_bb2d, [&] { clear_object_block(gb_, wCurObjUnk02); });
}

void Object::start(uint16_t update, uint16_t frameset)
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	set_update(update);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	set_frameset(frameset);
}

} // namespace wl3::level
