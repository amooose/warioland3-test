// Home's object routines as plain functions (src/home/object_mechanics.asm),
// for C++ modules to call directly; their hooks (object_mechanics.cpp)
// wrap them.
#pragma once

#include "../native.hpp"

namespace wl3::home {

// UpdateObjSprite: the object's sprite (its OAM data in the Objects OAM
// bank its id at `id` names, the frame and position from the 5 bytes at
// pos: screen y, x, frame, OAM pointer) added to virtual OAM
void update_obj_sprite(GB *gb, uint16_t id, uint16_t pos);

} // namespace wl3::home
