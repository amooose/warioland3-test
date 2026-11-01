// Home's memory routines as plain functions (src/home/copy.asm,
// clear_memory.asm, decompress.asm, far_decompress.asm), for C++ modules
// to call directly. Their hooks (memory.cpp) wrap them for register-level
// callers, leaving the registers the asm leaves.
#pragma once

#include "../native.hpp"

namespace wl3::home {

void fill(GB *gb, uint16_t dest, uint8_t value, uint32_t count); // WriteAToHL_BCTimes
// CopyHLToDE_BC; returns the last byte copied
uint8_t copy(GB *gb, uint16_t src, uint16_t dest, uint32_t count);
// FarCopyHLToDE_BC(2): from the ROM bank in wTempBank
void far_copy(GB *gb, uint16_t src, uint16_t dest, uint32_t count);

// Decompress: commands up to a 0 one, each a run of a byte (under $80:
// the count, then the byte) or literal bytes (from $80: the count in the
// low 7 bits, 0 being 256). Returns where both ends stopped.
struct DecompressEnd {
	uint16_t src, dest;
	bool any;      // a command ran (the asm leaves d = 0)
	int last_fill; // the last run's byte (the asm leaves it in e), or -1
};
DecompressEnd decompress(GB *gb, uint16_t src, uint16_t dest);
DecompressEnd far_decompress(GB *gb, uint16_t src, uint16_t dest); // FarDecompress: wTempBank

void clear_virtual_oam(GB *gb); // ClearVirtualOAM
// ClearUnusedVirtualOAM; returns the sprites it cleared (b's count)
int clear_unused_virtual_oam(GB *gb);
void fill_bg_map0(GB *gb, uint8_t value); // FillBGMap0: VRAM0's with value, VRAM1's with 0
void clear_bg_map1(GB *gb); // ClearBGMap1

void clear_vram(GB *gb); // ClearVRAM (only on CGB)
void clear_wram(GB *gb); // ClearWRAM: banks 1 to 7
// DisableLCD: waits for V-Blank (once LY passes it); false if the LCD was
// already off
bool disable_lcd(GB *gb);

// The bankswitch macro, run as home code does it (a write to the MBC from
// ROMX code would be the code switching banks under itself).
void bankswitch(GB *gb, uint8_t bank, uint16_t home_routine);

// The hcall macro (farcall: hCallFuncBank set first): fn with the bank in
// hCallFuncBank mapped, leaving hCallFunc's pointer bytes as the
// original's.
template <class F>
void hcall(GB *gb, uint16_t addr, F fn)
{
	Memory mem(gb);
	mem[hCallFuncPointer + 0] = static_cast<uint8_t>(addr);
	mem[hCallFuncPointer + 1] = static_cast<uint8_t>(addr >> 8);
	uint8_t bank = mem[wROMBank];
	bankswitch(gb, mem[hCallFuncBank], hCallFunc);
	fn();
	bankswitch(gb, bank, hCallFunc);
}

template <class F>
void farcall(GB *gb, uint8_t bank, uint16_t addr, F fn)
{
	Memory(gb).byte(hCallFuncBank) = bank;
	hcall(gb, addr, fn);
}

// idles for the given cycles, if time flows
void idle(GB *gb, int cycles);

} // namespace wl3::home
