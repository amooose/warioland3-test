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
SpriteEnd load_sprite(GB *gb, RomPtr table);
// the table in the bank mapped now
inline SpriteEnd load_sprite(GB *gb, uint16_t table) { return load_sprite(gb, RomPtr::mapped(gb, table)); }
// for the hooks: the registers LoadSprite leaves (b and c the position,
// de the data, hl virtual OAM, a and f from its last check)
void set_sprite_regs(GB *gb, const SpriteEnd &end);

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

// LoadGameOverSprite / LoadCreditsMusicBoxSprite: a sprite from its 4
// bytes at obj (y, x, frame, attributes; the position on screen), from
// the game over screen's OAM data or the music box's (wCreditsMusicBox*)
SpriteEnd load_game_over_sprite(GB *gb, uint16_t obj);
SpriteEnd load_credits_music_box_sprite(GB *gb, uint16_t obj);
// LoadWarioGfx: queues the DMA of Wario's graphics (wDMASource*) to
// v0Tiles0
void load_wario_gfx(GB *gb);

// UpdateGolfObjectAnimation: a golf object's animation step (duration,
// then the offset and the frame after it; frameset entries are (frame,
// duration), ended by $ff, which keeps the last frame in
// wGolfAnimLastFrame and starts over); true when it started over.
// new_duration: the duration it read for the next frame, if any
bool update_golf_object_animation(GB *gb, uint16_t frameset, uint16_t duration, uint8_t *new_duration = nullptr);
// LoadGolfSprite: the golf object's 4 bytes at obj (y, x, frame,
// attributes, as OAM takes them), from the OAM data at wGolfOAMPtr
SpriteEnd load_golf_sprite(GB *gb, uint16_t obj, uint8_t bank); // wGolfOAMPtr's table, in bank
// Func_2c30: starts the VDMA from src to wdc11, of w1dc13's length
void start_golf_vdma(GB *gb, uint16_t src);
// Func_2c46: 4 BG palettes from src to rBGPD
void copy_golf_bg_pals(GB *gb, uint16_t src);
// UpdateGolfWarioAnimation: Golf Wario's OAM and frameset for his state
// and direction, his animation step and his sprite
void update_golf_wario_animation(GB *gb);

void save_backup_vram(GB *gb); // SaveBackupVRAM: scroll, palettes and VRAM to WRAM 3-7
void load_backup_vram(GB *gb); // LoadBackupVRAM: and back

} // namespace wl3::home
