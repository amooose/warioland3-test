// The levels' objects: the object being updated. See object.hpp.
#include "object.hpp"

namespace wl3::level {

void Object::set_coll_box(uint8_t top, uint8_t bottom, uint8_t left, uint8_t right)
{
	mem_[wCurObjCollBoxTop] = top;
	mem_[wCurObjCollBoxBottom] = bottom;
	mem_[wCurObjCollBoxLeft] = left;
	mem_[wCurObjCollBoxRight] = right;
}

void Object::set_frameset(uint16_t frameset)
{
	SET_DE(gb_, frameset);
	routines_.call(ADDR_SetObjectFramesetPtr);
}

void Object::despawn()
{
	SET_HL(gb_, wCurObjUnk02);
	routines_.farcall(BANK_DespawnObject, ADDR_DespawnObject);
}

void Object::collect()
{
	SET_HL(gb_, wCurObjUnk02);
	routines_.farcall(BANK_Func_bb2d, ADDR_Func_bb2d);
}

void Object::start(uint16_t update, uint16_t frameset)
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	set_update(update);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	set_frameset(frameset);
}

} // namespace wl3::level
