// home/sprite (src/home/sprite.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeSprite : public AsmModule {
public:
	using AsmModule::AsmModule;

	// hl = OAM data pointer
	// given an OAM pointer and a frame index in wCurSpriteFrame
	// goes through each tile element and adds them to virtual OAM
	void port_LoadSprite()
	{
		// look up the frame's tile list
		uint8_t frame2 = alu_.add(rd8(wCurSpriteFrame), rd8(wCurSpriteFrame));
		cpu_.d = 0;
		cpu_.e = frame2;
		uint16_t hl = alu_.add16(REG_HL(gb_), REG_DE(gb_));
		uint16_t de = rd16(hl);

		cpu_.c = rd8(wCurSpriteXCoord);
		cpu_.b = rd8(wCurSpriteYCoord);
		uint8_t l = rd8(wVirtualOAMByteSize);
		uint8_t h = (uint8_t)(wVirtualOAM >> 8);

		for (;;) {
			cpu_.a = l;
			alu_.cp(l, OAM_SIZE);
			if (!(cpu_.f & FLAG_C))
				break; // virtual OAM full
			cpu_.a = rd8(de);
			alu_.cp(cpu_.a, 0x80);
			if (cpu_.f & FLAG_Z)
				break; // finished

			// y, x, tile, and attributes flipped by wCurSpriteAttributes
			uint16_t oam = (uint16_t)(h << 8 | l);
			wr8(oam++, alu_.add(rd8(de++), cpu_.b));
			wr8(oam++, alu_.add(rd8(de++), cpu_.c));
			wr8(oam++, rd8(de++));
			cpu_.a = alu_.xor_(rd8(de), rd8(wCurSpriteAttributes));
			wr8(oam++, cpu_.a);
			h = (uint8_t)(oam >> 8);
			l = (uint8_t)oam;
			cpu_.a = l;
			wr8(wVirtualOAMByteSize, l);
			de++;
		}
		cpu_.h = h;
		cpu_.l = l;
		SET_DE(gb_, de);
	}

	// Advances Wario's animation by a frame. His frameset pointer is stored
	// big-endian in wWarioFramesetPtr; wAnimationFrame follows wFrameDuration.
	void port_UpdateWarioAnimation()
	{
		cpu_.d = rd8(wWarioFramesetPtr + 0);
		cpu_.e = rd8(wWarioFramesetPtr + 1);
		wr8(wWarioAnimationEnded, FALSE);

		uint16_t hl = wFrameDuration;
		cpu_.a = alu_.sub(rd8(hl), 1);
		wr8(hl++, cpu_.a);
		if (!(cpu_.f & FLAG_C)) {
			SET_HL(gb_, hl);
			return; // duration hasn't elapsed yet
		}

		uint8_t offset = rd8(hl); // wAnimationFrame
		cpu_.c = alu_.add(offset, cpu_.e);
		cpu_.b = alu_.adc(cpu_.d, 0);
		uint16_t entry = REG_BC(gb_);
		uint8_t frame = rd8(entry);
		alu_.cp(frame, 0xff);
		if (frame == 0xff) {
			// .finished
			wr8(hl--, 0); // wAnimationFrame
			wr8(hl, 0);   // wFrameDuration
			wr8(wWarioAnimationEnded, TRUE);
			cpu_.a = TRUE;
			cpu_.f = FLAG_Z; // xor a
			SET_HL(gb_, hl);
			return;
		}

		// next frame
		wr8(wWarioFrame, frame);
		cpu_.a = alu_.add(rd8(hl), 2);
		wr8(hl--, cpu_.a); // wAnimationFrame
		entry++;
		cpu_.a = rd8(entry);
		wr8(hl, cpu_.a); // wFrameDuration
		SET_BC(gb_, entry);
		SET_HL(gb_, hl);
	}

	// Queues the block at hl (in SRAM bank wBlockPtrBank) for a BG map update.
	void port_Func_e31()
	{
		// h + (wBlockPtrBank - 1) * $20
		uint8_t a = alu_.dec(rd8(wBlockPtrBank));
		for (int i = 0; i < 5; i++)
			a = alu_.add(a, a);
		cpu_.b = alu_.add(a, cpu_.h);
		cpu_.c = cpu_.l;

		cpu_.e = rd8(wc19e);
		cpu_.d = 0;
		uint16_t hl = alu_.add16(wc18e, cpu_.e);
		wr8(hl++, cpu_.b);
		wr8(hl, cpu_.c);
		SET_HL(gb_, hl);
		cpu_.a = alu_.and_(alu_.add(cpu_.e, 2), 0x0f);
		wr8(wc19e, cpu_.a);
	}

	void port_Func_e2b()
	{
		cpu_.a = rd8(wCurObjFlags);
		if (!alu_.bit(cpu_.a, OBJFLAG_ON_SCREEN_F)) {
			RET();
			return;
		}
		JUMP(Func_e31); // fallthrough
	}

	// draws Wario on screen unless he's invisible
	void port_DrawWario()
	{
		cpu_.a = rd8(wTransformation);
		alu_.cp(cpu_.a, TRANSFORMATION_INVISIBLE_WARIO);
		if (cpu_.f & FLAG_Z) {
			// show Wario if wInvisibleFrame has upper 4 bits set and bit 2 unset
			SET_HL(gb_, wInvisibleFrame);
			uint8_t frame = alu_.inc(rd8(wInvisibleFrame));
			wr8(wInvisibleFrame, frame);
			cpu_.a = alu_.and_(frame, 0xf2);
			alu_.cp(cpu_.a, 0xf0);
			if (!(cpu_.f & FLAG_Z))
				return;
		} else {
			// .invincible: show Wario if wInvisibleFrame is 0
			cpu_.a = rd8(wInvisibleFrame);
			alu_.and_(cpu_.a, cpu_.a);
			if (!(cpu_.f & FLAG_Z))
				return;
		}

		cpu_.a = rd8(wWarioOAMBank);
		wr8(wTempBank, cpu_.a);
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = rd8(wTempBank);
		bankswitch(cpu_.a);
		cpu_.h = rd8(wWarioOAMPtr + 0);
		cpu_.l = rd8(wWarioOAMPtr + 1);
		wr8(wCurSpriteYCoord, rd8(wWarioScreenYPos));
		wr8(wCurSpriteXCoord, rd8(wWarioScreenXPos));
		wr8(wCurSpriteFrame, rd8(wWarioFrame));
		cpu_.a = rd8(wWarioAttributes);
		wr8(wCurSpriteAttributes, cpu_.a);
		CALL(LoadSprite);
		pop_af();
		bankswitch(cpu_.a);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeSprite;

PORT_EX(LoadSprite, 0, 0) { HomeSprite(gb).port_LoadSprite(); }
PORT_EX(UpdateWarioAnimation, 0, 0) { HomeSprite(gb).port_UpdateWarioAnimation(); }
PORT_EX(Func_e31, 0, 0) { HomeSprite(gb).port_Func_e31(); }
PORT_EX(Func_e2b, HOOK_MANUAL_RET, 0) { HomeSprite(gb).port_Func_e2b(); }
PORT_EX(DrawWario, 0, 0) { HomeSprite(gb).port_DrawWario(); }
