// Calling the routines that are still register-level C (src/ported) from
// C++: each wrapper loads the registers the routine takes, calls it as the
// original's call would, and returns what it returns.
#pragma once

#include "native.hpp"

namespace wl3 {

class Routines {
public:
	explicit Routines(GB *gb) : gb_(gb) {}

	// any routine, by address (one in a ROMX bank runs from the bank mapped)
	void call(uint16_t addr) { gb_call(gb_, addr); }

	void disable_lcd() { call(ADDR_DisableLCD); }
	void clear_virtual_oam() { call(ADDR_ClearVirtualOAM); }
	void clear_unused_virtual_oam() { call(ADDR_ClearUnusedVirtualOAM); }
	void save_backup_vram() { call(ADDR_SaveBackupVRAM); }

	// decompresses src from the given ROM bank (wTempBank) to dest
	void far_decompress(uint8_t bank, uint16_t src, uint16_t dest)
	{
		Memory(gb_).byte(wTempBank) = bank;
		SET_HL(gb_, src);
		SET_BC(gb_, dest);
		call(ADDR_FarDecompress);
	}

	// copies count bytes from src, in the ROM bank wTempBank, to dest
	void far_copy(uint16_t src, uint16_t dest, uint16_t count)
	{
		SET_HL(gb_, src);
		SET_DE(gb_, dest);
		SET_BC(gb_, count);
		call(ADDR_FarCopyHLToDE_BC2);
	}

	void copy(uint16_t src, uint16_t dest, uint8_t count)
	{
		SET_HL(gb_, src);
		SET_DE(gb_, dest);
		gb_->cpu.b = count;
		call(ADDR_CopyHLToDE);
	}

	void load_pals_to_temp_pals1(uint16_t pals)
	{
		SET_HL(gb_, pals);
		call(ADDR_LoadPalsToTempPals1);
	}

	void load_pals_to_temp_pals2(uint16_t pals)
	{
		SET_HL(gb_, pals);
		call(ADDR_LoadPalsToTempPals2);
	}

	void play_new_music(uint16_t music)
	{
		SET_BC(gb_, music);
		call(ADDR_PlayNewMusic);
	}

	bool check_has_all_musical_coin_flags()
	{
		call(ADDR_CheckHasAllMusicalCoinFlags);
		return gb_->cpu.a;
	}

	// Golf objects (src/home/load_golf_sprite.asm). True when the
	// animation finished.
	bool update_golf_object_animation(uint16_t frameset, uint16_t duration)
	{
		SET_DE(gb_, frameset);
		SET_HL(gb_, duration);
		call(ADDR_UpdateGolfObjectAnimation);
		return gb_->cpu.c;
	}

	void load_golf_sprite(uint16_t sprite)
	{
		SET_HL(gb_, sprite);
		call(ADDR_LoadGolfSprite);
	}

	void update_golf_wario_animation() { call(ADDR_UpdateGolfWarioAnimation); }

private:
	GB *gb_;
};

// The play_sfx and play_music2 / stop_music2 macros
inline void play_sfx(Memory mem, uint16_t sfx)
{
	mem[hSFXID + 0] = static_cast<uint8_t>(sfx >> 8);
	mem[hSFXID + 1] = static_cast<uint8_t>(sfx);
}

inline void play_music2(Memory mem, uint16_t music)
{
	mem[hMusicIDBackup + 0] = static_cast<uint8_t>(music >> 8);
	mem[hMusicID + 0] = static_cast<uint8_t>(music >> 8);
	mem[hMusicIDBackup + 1] = static_cast<uint8_t>(music);
	mem[hMusicID + 1] = static_cast<uint8_t>(music);
}

inline void stop_music2(Memory mem)
{
	mem[hMusicIDBackup + 0] = 0xff;
	mem[hMusicID + 0] = 0xff;
	mem[hMusicIDBackup + 1] = 0x00;
	mem[hMusicID + 1] = 0x00;
}

// sub n / daa on a BCD byte: the result, and whether it borrowed (the carry
// the following sbc uses)
inline uint8_t bcd_sub(uint8_t value, uint8_t n, bool &borrow)
{
	borrow = value < n;
	uint8_t result = static_cast<uint8_t>(value - n);
	if ((value & 0x0f) < (n & 0x0f))
		result = static_cast<uint8_t>(result - 0x06);
	if (borrow)
		result = static_cast<uint8_t>(result - 0x60);
	return result;
}

} // namespace wl3
