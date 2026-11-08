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

// MoveObject*: the current object (wCurObj) moved by dx / dy pixels
void move_object_x(GB *gb, int dx);
void move_object_y(GB *gb, int dy);

// SetObjectFramesetPtr: the current object's frameset, from its start
// (duration and offset 0)
void set_object_frameset_ptr(GB *gb, uint16_t frameset);

// UpdateObjectAnimation: the current object's frame duration down one; at
// its end the frameset's next (frame, duration) entry ($ff starts over);
// UpdateObjectFrame: just the frame at the offset, unless the duration
// lasts (the floor transitions')
void update_object_animation(GB *gb);
void update_object_frame(GB *gb);

// SetOneWayPlatformAction: no actions for the current object for a frame
// while Wario's under it (unless he's stepped on it)
void set_one_way_platform_action(GB *gb);

} // namespace wl3::home
