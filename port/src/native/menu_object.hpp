// A menu_obj_struct (src/macros/wram.asm), as the title screen, game over
// and other menus keep their sprites: y, x, frame and attributes, animated
// by UpdateObjAnim from a frameset; and the menu_obj_ext_struct fields that
// follow it in some.
#pragma once

#include "native.hpp"

extern "C" void port_farcall(GB *gb, int bank, uint16_t addr);

namespace wl3 {

class MenuObject {
public:
	MenuObject(GB *gb, uint16_t base) : gb_(gb), base_(base) {}

	Byte y() const { return byte(0); }
	Byte x() const { return byte(1); }
	Byte frame() const { return byte(2); }
	Byte attributes() const { return byte(3); }
	Byte frameset_offset() const { return byte(4); }
	Byte duration() const { return byte(5); }
	// menu_obj_ext_struct
	Byte var() const { return byte(8); }
	Byte animation_ended() const { return byte(11); }
	uint16_t addr() const { return base_; }

	// at (y, x), with a new frame, attributes and animation
	void init(uint8_t y_, uint8_t x_, uint16_t frameset)
	{
		y() = y_;
		x() = x_;
		frame() = 0;
		attributes() = 0;
		set_frameset(frameset);
	}

	// restarts the animation with frameset (stored high byte first)
	void set_frameset(uint16_t frameset)
	{
		frameset_offset() = 0;
		duration() = 0;
		byte(6) = static_cast<uint8_t>(frameset >> 8);
		byte(7) = static_cast<uint8_t>(frameset);
	}

	// UpdateObjAnim, its frameset in the bank mapped or (farcall) in bank;
	// whether the animation restarted (wObjAnimWasReset)
	bool animate(uint8_t bank = 0)
	{
		SET_HL(gb_, static_cast<uint16_t>(base_ + 7));
		if (bank)
			port_farcall(gb_, bank, ADDR_UpdateObjAnim);
		else
			gb_call(gb_, ADDR_UpdateObjAnim);
		return Byte(gb_, wObjAnimWasReset);
	}

	// LoadSprite with the OAM data at oam (in the bank mapped), the y
	// relative to the background (wSCY)
	void load_sprite(uint16_t oam)
	{
		Memory mem(gb_);
		mem[wCurSpriteYCoord] = static_cast<uint8_t>(y() + OAM_Y_OFS - mem[wSCY]);
		mem[wCurSpriteXCoord] = static_cast<uint8_t>(x() + OAM_X_OFS);
		mem[wCurSpriteFrame] = frame();
		mem[wCurSpriteAttributes] = attributes();
		SET_HL(gb_, oam);
		gb_call(gb_, ADDR_LoadSprite);
	}

private:
	Byte byte(int offset) const { return Byte(gb_, static_cast<uint16_t>(base_ + offset)); }

	GB *gb_;
	uint16_t base_;
};

} // namespace wl3
