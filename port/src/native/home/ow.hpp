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
// the object UpdateOWAnimation worked on (its hl, l & $f8), from the result
uint16_t ow_anim_object(uint16_t duration, int frame);

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

// overworld.cpp:
// GetOWCoordInTilemap: the wTilemap tile of the (y, x) pixel position at
// pos, also in wOWTilemapPtr (low byte first)
uint16_t ow_coord_in_tilemap(GB *gb, uint16_t pos);
// LoadLevelName: the HDMA parameters (w2d0b5) for a level name's tiles to
// v0Tiles2 tile $50; returns their source
uint16_t load_level_name(GB *gb, uint8_t side, uint8_t index);
void start_hdma(GB *gb); // Func_3c03: the HDMA in w2d0b5, to VRAM1
void load_far_bg_map(GB *gb, uint8_t bank, uint16_t src); // LoadFarBGMap: 18 rows to v0BGMap0

// ApplyPalConfig: a pal config (its register, cleared after reading; the
// index; 8-byte blocks, 0 being 256; the colours, big-endian) into the
// palette registers
struct PalConfigEnd {
	uint16_t src;     // after the colours
	uint8_t data_reg; // the data register's low byte
	uint8_t last;     // the last byte written
};
PalConfigEnd apply_pal_config(GB *gb, uint16_t config);

// ApplyOWMovement(_Mirrored): the OW object's next (y, x) offsets from the
// table (x subtracted when mirrored), by the step counter at obj | 7; at
// the table's end ($80) the counter starts over
struct OWMovement {
	uint16_t x;       // the x's address (its | 7 at the end)
	uint16_t entry;   // the next entry
	uint8_t dy, dx;
	uint8_t next;     // the next entry's y offset
	bool finished;
};
OWMovement apply_ow_movement(GB *gb, uint16_t obj, uint16_t table, bool mirrored);
void play_overworld_music(GB *gb); // PlayOverworldMusic: by day or night

} // namespace wl3::home
