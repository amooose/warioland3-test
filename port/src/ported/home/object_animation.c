// Port of src/home/object_animation.asm
#include "ported.h"

// Advances an object's animation by a frame.
// hl points at the high byte of the object's frameset pointer; below it in
// memory are the low byte, the frame duration, the frameset offset, and two
// bytes further the current frame.
PORT(UpdateObjAnim)
{
	wr8(gb, wObjAnimWasReset, 0);
	uint16_t hl = REG_HL(gb);
	gb->cpu.e = rd8(gb, hl--);
	gb->cpu.d = rd8(gb, hl--);
	uint8_t duration = alu_sub(gb, rd8(gb, hl), 1);
	gb->cpu.a = duration;
	wr8(gb, hl--, duration);
	if (!(gb->cpu.f & FLAG_C)) {
		// current frame still showing
		SET_HL(gb, hl);
		return;
	}

	// new frame
	uint8_t offset = rd8(gb, hl);
	uint8_t c = alu_add(gb, offset, gb->cpu.e);
	uint8_t b = alu_adc(gb, gb->cpu.d, 0);
	uint16_t entry = (uint16_t)(b << 8 | c);
	uint8_t frame = rd8(gb, entry);
	if (frame == 0xff) {
		// .reset_anim
		wr8(gb, hl++, 0); // frameset offset
		wr8(gb, hl, 0);   // duration
		wr8(gb, wObjAnimWasReset, TRUE);
		gb->cpu.a = TRUE;
		gb->cpu.f = FLAG_Z; // xor a
		SET_BC(gb, entry);
		SET_HL(gb, hl);
		return;
	}

	// The asm steps l with dec l / inc l, which never carry into h.
	uint8_t h = (uint8_t)(hl >> 8), l = (uint8_t)hl;
	l = (uint8_t)(l - 2);
	wr8(gb, (uint16_t)(h << 8 | l), frame); // frame
	hl = (uint16_t)((h << 8 | l) + 1);      // ld [hli]
	h = (uint8_t)(hl >> 8);
	l = (uint8_t)(hl + 1);                  // inc l
	hl = (uint16_t)(h << 8 | l);
	gb->cpu.a = alu_add(gb, rd8(gb, hl), 2);
	wr8(gb, hl++, gb->cpu.a); // frameset offset
	entry++;
	gb->cpu.a = rd8(gb, entry);
	wr8(gb, hl, gb->cpu.a); // duration
	SET_BC(gb, entry);
	SET_HL(gb, hl);
}
