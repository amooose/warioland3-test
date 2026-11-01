// Home's overworld helpers as plain functions (src/home/overworld.asm),
// for C++ modules to call directly; their hooks (ow.cpp) wrap them for
// register-level callers, leaving the registers the asm leaves. The rest
// of overworld.asm (pointer tables, movement) is still register-level
// (overworld.cpp).
#pragma once

#include "../native.hpp"
#include "sprites.hpp"

namespace wl3::home {

// UpdateOWAnimation: a scene object's animation step, its frameset in
// bank at frameset; duration: the object's duration (the offset follows,
// the frame is two bytes before). Frameset entries are (frame, duration)
// pairs, ended by $ff, which sets wOWAnimationFinished and starts over.
// Returns the new frame, or -1 while the duration lasts.
int update_ow_animation(GB *gb, uint8_t bank, uint16_t duration, uint16_t frameset);

// CopyFarBytes: count bytes (0 is 256) from bank:src; CopyFarBytes_Long:
// count * $100 bytes (0 is $10000)
void copy_far_bytes(GB *gb, uint8_t bank, uint16_t src, uint16_t dest, uint8_t count);
void copy_far_bytes_long(GB *gb, uint8_t bank, uint16_t src, uint16_t dest, uint8_t pages);
void load_far_tiles(GB *gb, uint8_t bank, uint16_t src); // LoadFarTiles: $80 tiles to v0Tiles2
void load_far_pals_to_temp_pals1(GB *gb, uint8_t bank, uint16_t pals); // LoadFarPalsToTempPals1
void load_far_pals_to_temp_pals2(GB *gb, uint8_t bank, uint16_t pals); // LoadFarPalsToTempPals2

// SetSceneObjState: state at its byte, the animation's two bytes before
// it cleared
void set_scene_obj_state(GB *gb, uint16_t state_addr, uint8_t state);

// The overworld sprite loaders: a sprite's 4 bytes at obj (y, x, frame,
// attributes), its frame from the OAM data at bank:table. LoadOWSprite
// takes a screen position, LoadOWSpriteWithScroll one on the BG map.
SpriteEnd load_ow_sprite(GB *gb, uint8_t bank, uint16_t table, uint16_t obj);
SpriteEnd load_ow_sprite_with_scroll(GB *gb, uint8_t bank, uint16_t table, uint16_t obj);
// LoadOWSpriteWithScroll_GotParams: wCurSprite* already set
SpriteEnd load_ow_sprite_got_params(GB *gb, uint8_t bank, uint16_t table);

bool is_treasure_collected(GB *gb, uint8_t treasure); // IsTreasureCollected
void play_overworld_music(GB *gb); // PlayOverworldMusic: by day or night

} // namespace wl3::home
