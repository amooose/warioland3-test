// Port of src/home/sprite.asm
#include "ported.h"

// hl = OAM data pointer
// given an OAM pointer and a frame index in wCurSpriteFrame
// goes through each tile element and adds them to virtual OAM
PORT(LoadSprite)
{
	// look up the frame's tile list
	uint8_t frame2 = alu_add(gb, rd8(gb, wCurSpriteFrame), rd8(gb, wCurSpriteFrame));
	gb->cpu.d = 0;
	gb->cpu.e = frame2;
	uint16_t hl = alu_add16(gb, REG_HL(gb), REG_DE(gb));
	uint16_t de = rd16(gb, hl);

	gb->cpu.c = rd8(gb, wCurSpriteXCoord);
	gb->cpu.b = rd8(gb, wCurSpriteYCoord);
	uint8_t l = rd8(gb, wVirtualOAMByteSize);
	uint8_t h = (uint8_t)(wVirtualOAM >> 8);

	for (;;) {
		gb->cpu.a = l;
		alu_cp(gb, l, OAM_SIZE);
		if (!(gb->cpu.f & FLAG_C))
			break; // virtual OAM full
		gb->cpu.a = rd8(gb, de);
		alu_cp(gb, gb->cpu.a, 0x80);
		if (gb->cpu.f & FLAG_Z)
			break; // finished

		// y, x, tile, and attributes flipped by wCurSpriteAttributes
		uint16_t oam = (uint16_t)(h << 8 | l);
		wr8(gb, oam++, alu_add(gb, rd8(gb, de++), gb->cpu.b));
		wr8(gb, oam++, alu_add(gb, rd8(gb, de++), gb->cpu.c));
		wr8(gb, oam++, rd8(gb, de++));
		gb->cpu.a = alu_xor(gb, rd8(gb, de), rd8(gb, wCurSpriteAttributes));
		wr8(gb, oam++, gb->cpu.a);
		h = (uint8_t)(oam >> 8);
		l = (uint8_t)oam;
		gb->cpu.a = l;
		wr8(gb, wVirtualOAMByteSize, l);
		de++;
	}
	gb->cpu.h = h;
	gb->cpu.l = l;
	SET_DE(gb, de);
}

// Advances Wario's animation by a frame. His frameset pointer is stored
// big-endian in wWarioFramesetPtr; wAnimationFrame follows wFrameDuration.
PORT(UpdateWarioAnimation)
{
	gb->cpu.d = rd8(gb, wWarioFramesetPtr + 0);
	gb->cpu.e = rd8(gb, wWarioFramesetPtr + 1);
	wr8(gb, wWarioAnimationEnded, FALSE);

	uint16_t hl = wFrameDuration;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl), 1);
	wr8(gb, hl++, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_C)) {
		SET_HL(gb, hl);
		return; // duration hasn't elapsed yet
	}

	uint8_t offset = rd8(gb, hl); // wAnimationFrame
	gb->cpu.c = alu_add(gb, offset, gb->cpu.e);
	gb->cpu.b = alu_adc(gb, gb->cpu.d, 0);
	uint16_t entry = REG_BC(gb);
	uint8_t frame = rd8(gb, entry);
	alu_cp(gb, frame, 0xff);
	if (frame == 0xff) {
		// .finished
		wr8(gb, hl--, 0); // wAnimationFrame
		wr8(gb, hl, 0);   // wFrameDuration
		wr8(gb, wWarioAnimationEnded, TRUE);
		gb->cpu.a = TRUE;
		gb->cpu.f = FLAG_Z; // xor a
		SET_HL(gb, hl);
		return;
	}

	// next frame
	wr8(gb, wWarioFrame, frame);
	gb->cpu.a = alu_add(gb, rd8(gb, hl), 2);
	wr8(gb, hl--, gb->cpu.a); // wAnimationFrame
	entry++;
	gb->cpu.a = rd8(gb, entry);
	wr8(gb, hl, gb->cpu.a); // wFrameDuration
	SET_BC(gb, entry);
	SET_HL(gb, hl);
}

// Queues the block at hl (in SRAM bank wBlockPtrBank) for a BG map update.
PORT(Func_e31)
{
	// h + (wBlockPtrBank - 1) * $20
	uint8_t a = alu_dec(gb, rd8(gb, wBlockPtrBank));
	for (int i = 0; i < 5; i++)
		a = alu_add(gb, a, a);
	gb->cpu.b = alu_add(gb, a, gb->cpu.h);
	gb->cpu.c = gb->cpu.l;

	gb->cpu.e = rd8(gb, wc19e);
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, wc18e, gb->cpu.e);
	wr8(gb, hl++, gb->cpu.b);
	wr8(gb, hl, gb->cpu.c);
	SET_HL(gb, hl);
	gb->cpu.a = alu_and(gb, alu_add(gb, gb->cpu.e, 2), 0x0f);
	wr8(gb, wc19e, gb->cpu.a);
}

PORT_JUMP(Func_e2b)
{
	gb->cpu.a = rd8(gb, wCurObjFlags);
	if (!alu_bit(gb, gb->cpu.a, OBJFLAG_ON_SCREEN_F)) {
		RET();
		return;
	}
	JUMP(Func_e31); // fallthrough
}

// draws Wario on screen unless he's invisible
PORT(DrawWario)
{
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_INVISIBLE_WARIO);
	if (gb->cpu.f & FLAG_Z) {
		// show Wario if wInvisibleFrame has upper 4 bits set and bit 2 unset
		SET_HL(gb, wInvisibleFrame);
		uint8_t frame = alu_inc(gb, rd8(gb, wInvisibleFrame));
		wr8(gb, wInvisibleFrame, frame);
		gb->cpu.a = alu_and(gb, frame, 0xf2);
		alu_cp(gb, gb->cpu.a, 0xf0);
		if (!(gb->cpu.f & FLAG_Z))
			return;
	} else {
		// .invincible: show Wario if wInvisibleFrame is 0
		gb->cpu.a = rd8(gb, wInvisibleFrame);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z))
			return;
	}

	gb->cpu.a = rd8(gb, wWarioOAMBank);
	wr8(gb, wTempBank, gb->cpu.a);
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = rd8(gb, wTempBank);
	bankswitch(gb, gb->cpu.a);
	gb->cpu.h = rd8(gb, wWarioOAMPtr + 0);
	gb->cpu.l = rd8(gb, wWarioOAMPtr + 1);
	wr8(gb, wCurSpriteYCoord, rd8(gb, wWarioScreenYPos));
	wr8(gb, wCurSpriteXCoord, rd8(gb, wWarioScreenXPos));
	wr8(gb, wCurSpriteFrame, rd8(gb, wWarioFrame));
	gb->cpu.a = rd8(gb, wWarioAttributes);
	wr8(gb, wCurSpriteAttributes, gb->cpu.a);
	CALL(LoadSprite);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}
