// Home's palette fades; see fades.hpp.
#include "fades.hpp"

#include "../flags.hpp"
#include "memory.hpp"

namespace wl3::home {

namespace {

// The .Func of VBlank_88d and VBlank_8bf: a jp to the V-Blank function
// proper, 3 bytes for wVBlankFunc.
constexpr uint16_t VBLANK_88D_FUNC = ADDR_VBlank_88d + 25;
constexpr uint16_t VBLANK_8BF_FUNC = ADDR_VBlank_8bf + 14;
constexpr int COLOURS = 8 * PAL_SIZE; // 64: through wTempBGPals and wTempOBPals

// The steps, each over every colour from wTempBGPals on; a colour is
// RGB555, little-endian. They walk with inc e / dec e, never carrying
// into d.
struct Colour {
	uint16_t lo, hi;
};
Colour colour(uint16_t page, uint8_t e)
{
	return {static_cast<uint16_t>((page & 0xff00) | e), static_cast<uint16_t>((page & 0xff00) | static_cast<uint8_t>(e + 1))};
}

void unpack(Memory mem, Colour at, uint16_t rgb)
{
	mem[rgb + 0] = mem[at.lo] & 0x1f;
	mem[rgb + 1] = static_cast<uint8_t>(((mem[at.lo] & 0xe0) >> 5) | ((mem[at.hi] & 0x03) << 3));
	mem[rgb + 2] = static_cast<uint8_t>((mem[at.hi] & 0x7c) >> 2);
}

// wCurRGB back into the colour. Components aren't masked, so one that
// wrapped below 0 spills into the others, as in the asm.
void pack(Memory mem, Colour at, FadeFrame &f)
{
	uint8_t r = mem[wCurRed], g = mem[wCurGreen], b = mem[wCurBlue];
	mem[at.lo] = static_cast<uint8_t>(r | (g & 0x07) << 5);
	f.c = static_cast<uint8_t>((g & 0x18) >> 3);
	f.a = static_cast<uint8_t>((b << 2 | b >> 6) | f.c); // rlca / rlca / or c
	mem[at.hi] = f.a;
}

int speed_steps(Memory mem)
{
	uint8_t speed = mem[wFadeSpeed];
	return speed == FADE_SPEED_SLOW ? 1 : speed == FADE_SPEED_NORMAL ? 2 : 4;
}

// to white: each component up by the speed's step, up to $1f
void lighten(GB *gb, FadeFrame &f)
{
	Memory mem(gb);
	int step = speed_steps(mem);
	uint8_t e = static_cast<uint8_t>(wTempBGPals);
	for (int i = 0; i < COLOURS; i++, e += 2) {
		Colour at = colour(wTempBGPals, e);
		unpack(mem, at, wCurRGB);
		for (int c = 2; c >= 0; c--) {
			uint8_t v = static_cast<uint8_t>(mem[wCurRGB + c] + step);
			mem[wCurRGB + c] = v >= 0x1f ? 0x1f : v;
		}
		uint8_t r = mem[wCurRed], g = mem[wCurGreen], b = mem[wCurBlue];
		mem[at.lo] = static_cast<uint8_t>(r | (g & 0x07) << 5);
		f.c = static_cast<uint8_t>((g & 0x18) >> 3);
		f.a = static_cast<uint8_t>(b << 2 | f.c);
		mem[at.hi] = f.a;
	}
	f.e = e;
}

// towards wTempPals1 (on page target), each component by towards(cur,
// target)
template <class Towards>
void towards_pals1(GB *gb, FadeFrame &f, uint16_t target_page, uint16_t cur_page, Towards towards)
{
	Memory mem(gb);
	uint8_t e = static_cast<uint8_t>(wTempPals1);
	for (int i = 0; i < COLOURS; i++, e += 2) {
		Colour cur = colour(cur_page, e);
		unpack(mem, colour(target_page, e), wTargetRGB);
		unpack(mem, cur, wCurRGB);
		for (int c = 2; c >= 0; c--)
			mem[wCurRGB + c] = towards(mem[wCurRGB + c], mem[wTargetRGB + c]);
		pack(mem, cur, f);
	}
	f.e = e;
}

// Down by the speed's step, stopping at 0, then up to the target if below
// it. A component at 0 wraps to $ff and down, and stays (it's not below).
uint8_t darken(Memory mem, uint8_t v, uint8_t target)
{
	int steps = speed_steps(mem);
	for (int i = 0; i < steps; i++)
		if (!--v)
			break;
	return v < target ? target : v;
}

// One frame: the set-up on the first two, then the step and the count.
template <class Step>
FadeFrame frame(GB *gb, uint8_t limit, bool unreferenced, Step step)
{
	Memory mem(gb);
	FadeFrame f{};
	f.counter = mem[wPalFadeCounter];
	if (f.counter < 2) {
		if (f.counter == 0) {
			mem[wUnused_c186] = 1;
			vblank_88d(gb);
			++mem[wPalFadeCounter];
			return f;
		}
		vblank_8bf(gb);
	}
	f.stepped = true;
	step(f);
	f.limit = limit;
	if (++mem[wPalFadeCounter] < limit)
		return f;
	gb->cpu.ime = false; // di
	copy(gb, wBackupVBlankFunc, wVBlankFunc, 3);
	gb->cpu.ime = true; // ei
	gb->cpu.ime_delay = 0;
	if (!unreferenced)
		mem[wFadeSpeed] = 0;
	mem[wPalFadeCounter] = 0;
	mem[wUnused_c186] = 0;
	f.sub_state = mem[wSubState];
	++mem[wSubState];
	f.done = true;
	return f;
}

// 33 frames, or 18 / 10 at the faster speeds
uint8_t frames_by_speed(Memory mem)
{
	uint8_t speed = mem[wFadeSpeed];
	return speed == FADE_SPEED_SLOW ? 33 : speed == FADE_SPEED_NORMAL ? 18 : 10;
}

void set_speed(GB *gb, uint8_t speed)
{
	if (speed != 0xff)
		Memory(gb).byte(wFadeSpeed) = speed;
}

} // namespace

void vblank_88d(GB *gb)
{
	gb->cpu.ime = false; // di
	gb->cpu.ime_delay = 0;
	copy(gb, wVBlankFunc, wBackupVBlankFunc, 3);
	copy(gb, VBLANK_88D_FUNC, wVBlankFunc, 3);
	gb->cpu.ime = true; // ei
}

void vblank_8bf(GB *gb)
{
	gb->cpu.ime = false; // di
	gb->cpu.ime_delay = 0;
	copy(gb, VBLANK_8BF_FUNC, wVBlankFunc, 3);
	gb->cpu.ime = true; // ei
}

FadeFrame fade_bg_to_white(GB *gb, uint8_t speed)
{
	set_speed(gb, speed);
	return frame(gb, frames_by_speed(Memory(gb)), false, [&](FadeFrame &f) { lighten(gb, f); });
}

FadeFrame darken_bg_to_pal(GB *gb, uint8_t speed)
{
	set_speed(gb, speed);
	Memory mem(gb);
	return frame(gb, frames_by_speed(mem), false, [&](FadeFrame &f) {
		towards_pals1(gb, f, wTempPals1, wTempBGPals, [&](uint8_t v, uint8_t t) { return darken(mem, v, t); });
	});
}

FadeFrame slow_fade_in_screen(GB *gb)
{
	return frame(gb, 33, false, [&](FadeFrame &f) {
		towards_pals1(gb, f, wTempPals1, wTempBGPals, [](uint8_t v, uint8_t t) {
			return t == v ? v : t > v ? static_cast<uint8_t>(v + 1) : static_cast<uint8_t>(v - 1);
		});
	});
}

namespace {

// unreferenced: wTempBGPals (and wTempOBPals) darkened by 1 per component
// and frame, down to 0, in 33 frames
FadeFrame func_54e(GB *gb)
{
	return frame(gb, 0x21, true, [&](FadeFrame &f) {
		Memory mem(gb);
		uint8_t e = static_cast<uint8_t>(wTempBGPals);
		for (int i = 0; i < COLOURS; i++, e += 2) {
			Colour at = colour(wTempBGPals, e);
			unpack(mem, at, wCurRGB);
			for (int c = 2; c >= 0; c--)
				if (uint8_t v = mem[wCurRGB + c])
					mem[wCurRGB + c] = static_cast<uint8_t>(v - 1);
			pack(mem, at, f);
		}
		f.e = e;
	});
}

// unreferenced: up by 1 per component and frame until equal to
// wTempPals1's, in 33 frames (the pages fixed at $c0 and $c1)
FadeFrame func_7cf(GB *gb)
{
	return frame(gb, 0x21, true, [&](FadeFrame &f) {
		towards_pals1(gb, f, 0xc000, 0xc100, [](uint8_t v, uint8_t t) { return v == t ? v : static_cast<uint8_t>(v + 1); });
	});
}

} // namespace

} // namespace wl3::home

using namespace wl3::home;
namespace flags = wl3::flags;

namespace {

// The registers a fade frame leaves: d is the page its loop walked; b the
// frame count it compared with (or, for the unreferenced two, 0).
void fade_regs(GB *gb, const FadeFrame &f, uint8_t d, bool limit_in_b = true)
{
	wl3::Memory mem(gb);
	if (!f.stepped) { // the first frame: VBlank_88d, then inc [hl]
		gb->cpu.a = mem[VBLANK_88D_FUNC + 2];
		gb->cpu.b = 0;
		SET_DE(gb, wVBlankFunc + 3);
		SET_HL(gb, wPalFadeCounter);
		gb->cpu.f = FLAG_C; // cp 2's carry, kept since
		return;
	}
	gb->cpu.c = f.c;
	if (!f.done) {
		gb->cpu.a = static_cast<uint8_t>(f.counter + 1);
		gb->cpu.b = limit_in_b ? f.limit : 0;
		gb->cpu.d = d;
		gb->cpu.e = f.e;
		SET_HL(gb, wPalFadeCounter);
		gb->cpu.f = flags::cp(gb->cpu.a, f.limit);
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.b = 0;
	SET_DE(gb, wVBlankFunc + 3);
	SET_HL(gb, wSubState);
	gb->cpu.f = flags::inc(0, f.sub_state); // after xor a
}

} // namespace

PORT_EX(FadeBGToWhite, 0, 0)
{
	fade_regs(gb, fade_bg_to_white(gb), wTempBGPals >> 8);
}

PORT_EX(FadeBGToWhite_Normal, 0, 0)
{
	fade_regs(gb, fade_bg_to_white(gb, FADE_SPEED_NORMAL), wTempBGPals >> 8);
}

PORT_EX(SlowFadeBGToWhite, 0, 0)
{
	fade_regs(gb, fade_bg_to_white(gb, FADE_SPEED_FAST), wTempBGPals >> 8);
}

PORT_EX(FastFadeToWhite, 0, 0)
{
	fade_regs(gb, fade_bg_to_white(gb, FADE_SPEED_SLOW), wTempBGPals >> 8);
}

PORT_EX(DarkenBGToPal, 0, 0)
{
	fade_regs(gb, darken_bg_to_pal(gb), wTempPals1 >> 8);
}

PORT_EX(DarkenBGToPal_Normal, 0, 0)
{
	fade_regs(gb, darken_bg_to_pal(gb, FADE_SPEED_NORMAL), wTempPals1 >> 8);
}

PORT_EX(DarkenBGToPal_Fast, 0, 0)
{
	fade_regs(gb, darken_bg_to_pal(gb, FADE_SPEED_FAST), wTempPals1 >> 8);
}

PORT_EX(SlowFadeFromWhite, 0, 0)
{
	fade_regs(gb, darken_bg_to_pal(gb, FADE_SPEED_SLOW), wTempPals1 >> 8);
}

PORT_EX(SlowFadeInScreen, 0, 0)
{
	fade_regs(gb, slow_fade_in_screen(gb), wTempPals1 >> 8);
}

PORT_EX(Func_54e, 0, 0)
{
	fade_regs(gb, func_54e(gb), wTempBGPals >> 8, false);
}

PORT_EX(Func_7cf, 0, 0)
{
	fade_regs(gb, func_7cf(gb), 0xc0, false);
}

// the last CopyHLToDE's registers
static void vblank_func_regs(GB *gb, uint16_t func)
{
	SET_HL(gb, func + 3);
	SET_DE(gb, wVBlankFunc + 3);
	gb->cpu.a = wl3::Memory(gb)[func + 2];
	gb->cpu.b = 0;
	gb->cpu.f = static_cast<uint8_t>((gb->cpu.f & FLAG_C) | FLAG_Z | FLAG_N);
}

PORT_EX(VBlank_88d, 0, 0)
{
	vblank_88d(gb);
	vblank_func_regs(gb, VBLANK_88D_FUNC);
}

PORT_EX(VBlank_8bf, 0, 0)
{
	vblank_8bf(gb);
	vblank_func_regs(gb, VBLANK_8BF_FUNC);
}

// VBlank_88d.Func's and VBlank_8bf.Func's jp targets, run from wVBlankFunc:
// the scroll, the palettes and the OAM (through hTransferVirtualOAM, in
// HRAM)
static void apply_vblank(GB *gb, uint16_t bg, uint16_t ob)
{
	wl3::Memory mem(gb);
	mem[rSCY] = mem[wSCY];
	mem[rSCX] = mem[wSCX];
	gb_call(gb, bg);
	gb_call(gb, ob);
	gb->cpu.a = static_cast<uint8_t>(wVirtualOAM >> 8);
	gb_call(gb, hTransferVirtualOAM);
}

PORT_LOCAL_EX(VBlank_88d, asm_8a9, 0, 0)
{
	gb->cpu.a = wl3::Memory(gb)[wSCX];
	apply_vblank(gb, ADDR_StoreBGPals, ADDR_StoreOBPals);
}

PORT_LOCAL_EX(VBlank_8bf, asm_8d0, 0, 0)
{
	gb->cpu.a = wl3::Memory(gb)[wSCX];
	apply_vblank(gb, ADDR_LoadBGPalettesFromWRAM, ADDR_LoadOBPalettesFromWRAM);
}
