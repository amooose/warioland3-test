// Home's sprite, animation and VRAM backup routines as plain functions
// (src/home/sprite.asm, object_animation.asm, backup_vram.asm), for C++
// modules to call directly; their hooks (sprites.cpp) wrap them for
// register-level callers, leaving the registers the asm leaves.
#pragma once

#include "../native.hpp"

namespace wl3::home {

// LoadSprite: the frame wCurSpriteFrame of the OAM data at table, at
// wCurSpriteY/XCoord with wCurSpriteAttributes, added to virtual OAM until
// its $80 end (or virtual OAM fills)
struct SpriteEnd {
	uint16_t data; // where the frame's data stopped
	uint8_t oam;   // wVirtualOAMByteSize
	bool full;     // stopped by virtual OAM filling up
};
SpriteEnd load_sprite(GB *gb, uint16_t table);

// DrawWario: Wario's sprite from wWarioOAMBank/Ptr, unless he's invisible
// (or blinking); whether he was drawn, and where the sprite ended
bool draw_wario(GB *gb, SpriteEnd *end = nullptr);

// An animation step (UpdateObjAnim, UpdateWarioAnimation): the frame
// duration counts down, and when it runs out the next frame and duration
// come from the frameset, or at its $ff end it starts over.
struct AnimStep {
	bool new_frame; // the duration ran out
	bool reset;     // the frameset ended
	uint16_t entry; // the frameset entry read last
	uint8_t value;  // the duration left, or the offset before its + 2
};
// frameset: the object's frameset pointer's high byte; below it the low
// byte, the duration, the offset and, two bytes on, the frame
AnimStep update_obj_anim(GB *gb, uint16_t frameset);
AnimStep update_wario_animation(GB *gb);

// Func_e31: the block at block (in SRAM bank wBlockPtrBank) queued for a
// BG map update; returns the queue entry's offset (wc19e before)
uint8_t queue_block_update(GB *gb, uint16_t block);

void save_backup_vram(GB *gb); // SaveBackupVRAM: scroll, palettes and VRAM to WRAM 3-7
void load_backup_vram(GB *gb); // LoadBackupVRAM: and back

} // namespace wl3::home
