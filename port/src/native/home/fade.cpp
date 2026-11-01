// home/fade (src/home/fade.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeFade : public AsmModule {
public:
	using AsmModule::AsmModule;

	// Raises one colour component (5 bits) by the fade step, capping it at $1f:
	// +1 for FADE_SPEED_SLOW, +2 for FADE_SPEED_NORMAL, +4 otherwise.
	static uint8_t lighten(uint8_t v, uint8_t speed)
	{
		int step = speed == FADE_SPEED_SLOW ? 1 : speed == FADE_SPEED_NORMAL ? 2 : 4;
		v = (uint8_t)(v + step);
		return v >= 0x1f ? 0x1f : v;
	}

	// The first two frames of a fade set up the VBlank function that applies
	// wTempBGPals: returns whether to go on with this frame's step.
	bool fade_begin()
	{
		uint8_t counter = rd8(wPalFadeCounter);
		cpu_.a = counter;
		alu_.cp(counter, 2);
		if (counter >= 2)
			return true;
		cpu_.a = alu_.dec(counter);
		if (counter == 1) {
			CALL(VBlank_8bf);
			return true;
		}
		cpu_.a = 1;
		wr8(wUnused_c186, 1);
		CALL(VBlank_88d);
		SET_HL(gb_, wPalFadeCounter);
		wr8(wPalFadeCounter, alu_.inc(rd8(wPalFadeCounter))); // set up to begin fade
		return false;
	}

	// Counts the frame; once the fade has taken 33 frames (or 18 / 10 for the
	// faster speeds, if by_speed), the previous VBlank function is restored and
	// wSubState advances.
	void fade_end(bool by_speed)
	{
		SET_HL(gb_, wPalFadeCounter);
		uint8_t counter = (uint8_t)(rd8(wPalFadeCounter) + 1);
		wr8(wPalFadeCounter, counter);
		uint8_t speed = rd8(wFadeSpeed);
		cpu_.b = 33;
		if (by_speed)
			cpu_.b = speed == FADE_SPEED_SLOW ? 33 : speed == FADE_SPEED_NORMAL ? 18 : 10;
		cpu_.a = counter;
		alu_.cp(counter, cpu_.b);
		if (cpu_.f & FLAG_C)
			return;

		// done: restore the VBlank function
		cpu_.ime = false; // di
		SET_HL(gb_, wBackupVBlankFunc);
		SET_DE(gb_, wVBlankFunc);
		cpu_.b = 3;
		CALL(CopyHLToDE);
		cpu_.ime = true; // ei
		cpu_.ime_delay = 0;

		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wFadeSpeed, 0);
		wr8(wPalFadeCounter, 0);
		wr8(wUnused_c186, 0);
		SET_HL(gb_, wSubState);
		wr8(wSubState, alu_.inc(rd8(wSubState)));
	}

	// One step of fading the BG palettes to white, per frame.
	void port_FadeBGToWhite()
	{
		if (!fade_begin())
			return;
		// .lighten: each colour is RGB555, little-endian, from wTempBGPals; it's
		// unpacked into wCurRed/Green/Blue, lightened and packed back.
		uint8_t speed = rd8(wFadeSpeed);
		cpu_.d = (uint8_t)(wTempBGPals >> 8);
		uint8_t e = (uint8_t)wTempBGPals; // inc e / dec e never carry into d
		for (int i = 0; i < 8 * PAL_SIZE; i++) { // b = 8 palettes: also covers wTempOBPals
			uint16_t de = (uint16_t)(cpu_.d << 8);
			uint8_t lo = rd8(de | e);
			uint8_t hi = rd8(de | (uint8_t)(e + 1));
			uint8_t red = lo & 0x1f;
			uint8_t green = (uint8_t)(((lo & 0xe0) >> 5) | ((hi & 0x03) << 3));
			uint8_t blue = (hi & 0x7c) >> 2;

			uint8_t b = lighten(blue, speed);
			uint8_t g = lighten(green, speed);
			uint8_t r = lighten(red, speed);
			wr8(wCurRed, r);
			wr8(wCurGreen, g);
			wr8(wCurBlue, b);

			// convert back to RGB555
			lo = (uint8_t)(r | (g & 0x07) << 5);
			wr8(de | e, lo);
			cpu_.c = (g & 0x18) >> 3;
			hi = (uint8_t)(b << 2 | cpu_.c);
			wr8(de | (uint8_t)(e + 1), hi);
			e = (uint8_t)(e + 2);
		}
		cpu_.e = e;
		cpu_.a = (uint8_t)((rd8(wCurBlue) << 2) | cpu_.c);

		fade_end(true);
	}

	void port_FadeBGToWhite_Normal()
	{
		cpu_.a = FADE_SPEED_NORMAL;
		wr8(wFadeSpeed, cpu_.a);
		JUMP(FadeBGToWhite); // jr
	}

	void port_SlowFadeBGToWhite()
	{
		cpu_.a = FADE_SPEED_FAST;
		wr8(wFadeSpeed, cpu_.a);
		JUMP(FadeBGToWhite); // jr
	}

	void port_FastFadeToWhite()
	{
		cpu_.a = FADE_SPEED_SLOW;
		cpu_.f = FLAG_Z; // xor a
		wr8(wFadeSpeed, cpu_.a);
		JUMP(FadeBGToWhite); // fallthrough
	}

	// The .Func of VBlank_88d and VBlank_8bf: jp to the VBlank function proper.
	#define VBLANK_88D_FUNC (ADDR_VBlank_88d + 25)
	#define VBLANK_8BF_FUNC (ADDR_VBlank_8bf + 14)

	// backs up wVBlankFunc and sets the one that applies the BG and OB pals
	// from wTempBGPals / wTempOBPals
	void port_VBlank_88d()
	{
		cpu_.ime = false; // di
		cpu_.ime_delay = 0;
		SET_HL(gb_, wVBlankFunc);
		SET_DE(gb_, wBackupVBlankFunc);
		cpu_.b = 3;
		CALL(CopyHLToDE);
		SET_HL(gb_, VBLANK_88D_FUNC);
		SET_DE(gb_, wVBlankFunc);
		cpu_.b = 3;
		CALL(CopyHLToDE);
		cpu_.ime = true; // ei
	}

	void port_VBlank_8bf()
	{
		cpu_.ime = false; // di
		cpu_.ime_delay = 0;
		SET_HL(gb_, VBLANK_8BF_FUNC);
		SET_DE(gb_, wVBlankFunc);
		cpu_.b = 3;
		CALL(CopyHLToDE);
		cpu_.ime = true; // ei
	}

	// RGB555, little-endian, at lo / hi, into red, green and blue at rgb.
	void unpack_rgb(uint16_t lo, uint16_t hi, uint16_t rgb)
	{
		wr8(rgb + 0, rd8(lo) & 0x1f);
		wr8(rgb + 1, (uint8_t)(((rd8(lo) & 0xe0) >> 5) | ((rd8(hi) & 0x03) << 3)));
		wr8(rgb + 2, (uint8_t)((rd8(hi) & 0x7c) >> 2));
	}

	// Packs wCurRGB back into lo / hi. Components aren't masked, so one that
	// wrapped below 0 (see darken) spills into the others, as in the asm.
	void pack_cur_rgb(uint16_t lo, uint16_t hi)
	{
		uint8_t r = rd8(wCurRed), g = rd8(wCurGreen), b = rd8(wCurBlue);
		wr8(lo, (uint8_t)(r | (g & 0x07) << 5));
		cpu_.c = (g & 0x18) >> 3;
		cpu_.a = (uint8_t)((b << 2 | b >> 6) | cpu_.c); // rlca / rlca / or c
		wr8(hi, cpu_.a);
	}

	// Steps every colour of wTempBGPals (and wTempOBPals) towards wTempPals1,
	// one component at a time (blue, green, red) with step(cur, target).
	void fade_to_pals1(uint8_t (HomeFade::*step)(uint8_t cur, uint8_t target))
	{
		uint8_t e = (uint8_t)wTempPals1; // inc e / dec e never carry into d
		for (int i = 0; i < 8 * PAL_SIZE; i++) { // b = 8 palettes
			uint16_t target = (uint16_t)((wTempPals1 & 0xff00) | e);
			uint16_t cur = (uint16_t)((wTempBGPals & 0xff00) | e);
			uint16_t target_hi = (uint16_t)((target & 0xff00) | (uint8_t)(e + 1));
			uint16_t cur_hi = (uint16_t)((cur & 0xff00) | (uint8_t)(e + 1));
			unpack_rgb(target, target_hi, wTargetRGB);
			unpack_rgb(cur, cur_hi, wCurRGB);
			for (int c = 2; c >= 0; c--) {
				cpu_.c = rd8(wTargetRGB + c);
				wr8(wCurRGB + c, (this->*step)(rd8(wCurRGB + c), cpu_.c));
			}
			pack_cur_rgb(cur, cur_hi);
			e = (uint8_t)(e + 2);
		}
		cpu_.b = 0;
		cpu_.d = (uint8_t)(wTempPals1 >> 8);
		cpu_.e = e;
		SET_HL(gb_, wTargetRGB);
	}

	// Lowers a component by 1, 2 or 4 by speed, stopping at 0, then raises it
	// to the target if it went below. A component that starts at 0 wraps to
	// $ff and down, and is kept (it's not below the target).
	uint8_t darken(uint8_t v, uint8_t target)
	{
		uint8_t speed = rd8(wFadeSpeed);
		int steps = speed == FADE_SPEED_SLOW ? 1 : speed == FADE_SPEED_NORMAL ? 2 : 4;
		for (int i = 0; i < steps; i++) {
			v = (uint8_t)(v - 1);
			if (v == 0)
				break;
		}
		return v < target ? target : v;
	}

	// fades BG palettes to wTempPals1
	// gradually darkens up to wTempPals1
	void port_DarkenBGToPal()
	{
		if (!fade_begin())
			return;
		fade_to_pals1(&HomeFade::darken);
		fade_end(true);
	}

	void port_DarkenBGToPal_Normal()
	{
		cpu_.a = FADE_SPEED_NORMAL;
		wr8(wFadeSpeed, cpu_.a);
		JUMP(DarkenBGToPal); // jr
	}

	void port_DarkenBGToPal_Fast()
	{
		cpu_.a = FADE_SPEED_FAST;
		wr8(wFadeSpeed, cpu_.a);
		JUMP(DarkenBGToPal); // jr
	}

	void port_SlowFadeFromWhite()
	{
		cpu_.a = FADE_SPEED_SLOW;
		cpu_.f = FLAG_Z; // xor a
		wr8(wFadeSpeed, cpu_.a);
		JUMP(DarkenBGToPal); // fallthrough
	}

	// Moves a component 1 towards the target.
	uint8_t move_towards(uint8_t v, uint8_t target)
	{
		return target == v ? v : target > v ? (uint8_t)(v + 1) : (uint8_t)(v - 1);
	}

	// fades BG palettes to wTempPals1, in 33 frames
	void port_SlowFadeInScreen()
	{
		if (!fade_begin())
			return;
		fade_to_pals1(&HomeFade::move_towards);
		fade_end(false);
	}

	// The end of Func_54e / Func_7cf: like fade_end(false), but wFadeSpeed
	// is left alone.
	void fade_end_unref()
	{
		SET_HL(gb_, wPalFadeCounter);
		uint8_t counter = alu_.inc(rd8(wPalFadeCounter));
		wr8(wPalFadeCounter, counter);
		cpu_.a = counter;
		alu_.cp(counter, 0x21);
		if (cpu_.f & FLAG_C)
			return;
		cpu_.ime = false; // di
		SET_HL(gb_, wBackupVBlankFunc);
		SET_DE(gb_, wVBlankFunc);
		cpu_.b = 3;
		CALL(CopyHLToDE);
		cpu_.ime = true; // ei
		cpu_.ime_delay = 0;
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wPalFadeCounter, 0);
		wr8(wUnused_c186, 0);
		SET_HL(gb_, wSubState);
		wr8(wSubState, alu_.inc(rd8(wSubState)));
	}

	// unreferenced: darkens wTempBGPals (and wTempOBPals) by 1 per component
	// and frame, down to 0, in 33 frames
	void port_Func_54e()
	{
		if (!fade_begin())
			return;
		uint8_t e = (uint8_t)wTempBGPals; // inc e / dec e never carry into d
		for (int i = 0; i < 8 * PAL_SIZE; i++) {
			uint16_t lo = (uint16_t)((wTempBGPals & 0xff00) | e);
			uint16_t hi = (uint16_t)((wTempBGPals & 0xff00) | (uint8_t)(e + 1));
			unpack_rgb(lo, hi, wCurRGB);
			for (int c = 2; c >= 0; c--) {
				uint8_t v = rd8(wCurRGB + c);
				cpu_.a = alu_.and_(v, v);
				if (v)
					wr8(wCurRGB + c, alu_.dec(v));
			}
			pack_cur_rgb(lo, hi);
			e = (uint8_t)(e + 2);
		}
		cpu_.b = 0;
		cpu_.d = (uint8_t)(wTempBGPals >> 8);
		cpu_.e = e;
		fade_end_unref();
	}

	// unreferenced: raises each component of wTempBGPals (and wTempOBPals) by 1
	// per frame until it equals wTempPals1's, in 33 frames
	void port_Func_7cf()
	{
		if (!fade_begin())
			return;
		uint8_t e = (uint8_t)wTempPals1; // inc e / dec e never carry into d
		for (int i = 0; i < 0x40; i++) {
			uint16_t target = (uint16_t)(0xc000 | e);
			uint16_t target_hi = (uint16_t)(0xc000 | (uint8_t)(e + 1));
			uint16_t cur = (uint16_t)(0xc100 | e);
			uint16_t cur_hi = (uint16_t)(0xc100 | (uint8_t)(e + 1));
			unpack_rgb(target, target_hi, wTargetRGB);
			unpack_rgb(cur, cur_hi, wCurRGB);
			for (int c = 2; c >= 0; c--) {
				cpu_.c = rd8(wTargetRGB + c);
				uint8_t v = rd8(wCurRGB + c);
				cpu_.a = v;
				alu_.cp(v, cpu_.c);
				if (!(cpu_.f & FLAG_Z))
					wr8(wCurRGB + c, alu_.inc(v));
			}
			pack_cur_rgb(cur, cur_hi);
			e = (uint8_t)(e + 2);
		}
		cpu_.b = 0;
		cpu_.d = 0xc0;
		cpu_.e = e;
		fade_end_unref();
	}

	// VBlank_88d.Func's jp target: apply the scroll, the pals in wTempBGPals /
	// wTempOBPals and the OAM
	void port_VBlank_88d__asm_8a9()
	{
		wr8(rSCY, rd8(wSCY));
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
		CALL(StoreBGPals);
		CALL(StoreOBPals);
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		gb_call(gb_, hTransferVirtualOAM);
	}

	// VBlank_8bf.Func's jp target: same, with the pals from WRAM
	void port_VBlank_8bf__asm_8d0()
	{
		wr8(rSCY, rd8(wSCY));
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);
		CALL(LoadBGPalettesFromWRAM);
		CALL(LoadOBPalettesFromWRAM);
		cpu_.a = (uint8_t)(wVirtualOAM >> 8);
		gb_call(gb_, hTransferVirtualOAM);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeFade;

PORT_EX(FadeBGToWhite, 0, 0) { HomeFade(gb).port_FadeBGToWhite(); }
PORT_EX(FadeBGToWhite_Normal, HOOK_MANUAL_RET, 0) { HomeFade(gb).port_FadeBGToWhite_Normal(); }
PORT_EX(SlowFadeBGToWhite, HOOK_MANUAL_RET, 0) { HomeFade(gb).port_SlowFadeBGToWhite(); }
PORT_EX(FastFadeToWhite, HOOK_MANUAL_RET, 0) { HomeFade(gb).port_FastFadeToWhite(); }
PORT_EX(VBlank_88d, 0, 0) { HomeFade(gb).port_VBlank_88d(); }
PORT_EX(VBlank_8bf, 0, 0) { HomeFade(gb).port_VBlank_8bf(); }
PORT_EX(DarkenBGToPal, 0, 0) { HomeFade(gb).port_DarkenBGToPal(); }
PORT_EX(DarkenBGToPal_Normal, HOOK_MANUAL_RET, 0) { HomeFade(gb).port_DarkenBGToPal_Normal(); }
PORT_EX(DarkenBGToPal_Fast, HOOK_MANUAL_RET, 0) { HomeFade(gb).port_DarkenBGToPal_Fast(); }
PORT_EX(SlowFadeFromWhite, HOOK_MANUAL_RET, 0) { HomeFade(gb).port_SlowFadeFromWhite(); }
PORT_EX(SlowFadeInScreen, 0, 0) { HomeFade(gb).port_SlowFadeInScreen(); }
PORT_EX(Func_54e, 0, 0) { HomeFade(gb).port_Func_54e(); }
PORT_EX(Func_7cf, 0, 0) { HomeFade(gb).port_Func_7cf(); }
PORT_LOCAL_EX(VBlank_88d, asm_8a9, 0, 0) { HomeFade(gb).port_VBlank_88d__asm_8a9(); }
PORT_LOCAL_EX(VBlank_8bf, asm_8d0, 0, 0) { HomeFade(gb).port_VBlank_8bf__asm_8d0(); }
