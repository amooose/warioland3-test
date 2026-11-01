// Calling home routines from C++. Those with plain C++ versions (home/
// memory.hpp...) are called directly; for the rest, each wrapper loads the
// registers the routine takes, calls it as the original's call would, and
// returns what it returns.
#pragma once

#include "home/memory.hpp"
#include "home/pals.hpp"
#include "native.hpp"
#include "ram.hpp"

extern "C" void port_farcall(GB *gb, int bank, uint16_t addr);
extern "C" void port_farcall_ret(GB *gb, int bank, uint16_t addr, uint16_t ret);

namespace wl3 {

class Routines {
public:
	explicit Routines(GB *gb) : gb_(gb) {}

	// any routine, by address (one in a ROMX bank runs from the bank mapped)
	void call(uint16_t addr) { gb_call(gb_, addr); }

	// the farcall macro: any routine, with the given ROM bank mapped
	// (through hCallFunc)
	void farcall(uint8_t bank, uint16_t addr) { port_farcall(gb_, bank, addr); }

	// call and farcall pushing the original's return address (ret: the
	// address after its call) rather than a sentinel, for code that reads
	// the stack (the room drawing's queues, block functions)
	void call_ret(uint16_t addr, uint16_t ret) { gb_call_ret(gb_, addr, ret); }
	void farcall_ret(uint8_t bank, uint16_t addr, uint16_t ret) { port_farcall_ret(gb_, bank, addr, ret); }

	// The farcall macro, for a routine of a C++ module: fn runs with bank
	// mapped, leaving hCallFunc's bank and pointer (addr, the original
	// routine) as the original's farcall does.
	template <class F>
	void in_bank(uint8_t bank, uint16_t addr, F fn)
	{
		home::farcall(gb_, bank, addr, fn);
	}

	// the bankswitch macro
	void switch_bank(uint8_t bank)
	{
		Memory mem(gb_);
		mem[wROMBank] = bank;
		mem[rROMB0 + 0x100] = bank;
	}

	void disable_lcd() { home::disable_lcd(gb_); }
	void clear_virtual_oam() { home::clear_virtual_oam(gb_); }
	void clear_unused_virtual_oam() { home::clear_unused_virtual_oam(gb_); }
	void save_backup_vram() { call(ADDR_SaveBackupVRAM); }

	// decompresses src from the given ROM bank (wTempBank) to dest
	void far_decompress(uint8_t bank, uint16_t src, uint16_t dest)
	{
		Memory(gb_).byte(wTempBank) = bank;
		home::far_decompress(gb_, src, dest);
	}

	// decompresses src (from the ROM bank mapped) to dest
	void decompress(uint16_t src, uint16_t dest)
	{
		home::decompress(gb_, src, dest);
	}

	// copies count bytes from src, in the ROM bank wTempBank, to dest
	void far_copy(uint16_t src, uint16_t dest, uint16_t count)
	{
		home::far_copy(gb_, src, dest, count ? count : 0x10000);
	}

	void copy(uint16_t src, uint16_t dest, uint8_t count)
	{
		home::copy(gb_, src, dest, count ? count : 0x100);
	}

	// CopyHLToDE_BC and CopyHLToDE_Short, with bank mapped (farcall) or
	// from the bank mapped (bank 0)
	void copy_bc(uint16_t src, uint16_t dest, uint16_t count, uint8_t bank = 0)
	{
		auto copy = [&] { home::copy(gb_, src, dest, count ? count : 0x10000); };
		if (bank)
			home::farcall(gb_, bank, ADDR_CopyHLToDE_BC, copy);
		else
			copy();
	}

	void copy_short(uint16_t src, uint16_t dest, uint8_t count, uint8_t bank = 0)
	{
		auto copy = [&] { home::copy_short(gb_, src, dest, count); };
		if (bank)
			home::farcall(gb_, bank, ADDR_CopyHLToDE_Short, copy);
		else
			copy();
	}

	void load_pals_to_temp_pals1(uint16_t pals)
	{
		home::load_pals_to_temp_pals1(gb_, pals);
	}

	void load_pals_to_temp_pals2(uint16_t pals)
	{
		home::load_pals_to_temp_pals2(gb_, pals);
	}

	void play_new_music(uint16_t music)
	{
		SET_BC(gb_, music);
		call(ADDR_PlayNewMusic);
	}

	// a = mod, b = value, d = SFX channels, e = music channels
	void execute_audio_mod(uint8_t mod, uint8_t value, uint8_t sfx_channels, uint8_t music_channels)
	{
		gb_->cpu.a = mod;
		gb_->cpu.b = value;
		gb_->cpu.d = sfx_channels;
		gb_->cpu.e = music_channels;
		call(ADDR_ExecuteAudioMod);
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

// The play_sfx, stop_sfx and play_music2 / stop_music2 macros
inline void play_sfx(Memory mem, uint16_t sfx)
{
	mem[hSFXID + 0] = static_cast<uint8_t>(sfx >> 8);
	mem[hSFXID + 1] = static_cast<uint8_t>(sfx);
}

inline void play_music(Memory mem, uint16_t music)
{
	mem[hMusicID + 0] = static_cast<uint8_t>(music >> 8);
	mem[hMusicID + 1] = static_cast<uint8_t>(music);
}

inline void stop_music(Memory mem)
{
	mem[hMusicID + 0] = 0xff;
	mem[hMusicID + 1] = 0x00;
}

inline void play_music2(Memory mem, uint16_t music)
{
	mem[hMusicIDBackup + 0] = static_cast<uint8_t>(music >> 8);
	mem[hMusicID + 0] = static_cast<uint8_t>(music >> 8);
	mem[hMusicIDBackup + 1] = static_cast<uint8_t>(music);
	mem[hMusicID + 1] = static_cast<uint8_t>(music);
}

inline void stop_sfx(Memory mem)
{
	mem[hSFXID + 0] = 0xff;
	mem[hSFXID + 1] = 0x00;
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
