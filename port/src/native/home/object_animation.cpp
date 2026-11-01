// home/object_animation (src/home/object_animation.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeObjectAnimation : public AsmModule {
public:
	using AsmModule::AsmModule;

	// Advances an object's animation by a frame.
	// hl points at the high byte of the object's frameset pointer; below it in
	// memory are the low byte, the frame duration, the frameset offset, and two
	// bytes further the current frame.
	void port_UpdateObjAnim()
	{
		wr8(wObjAnimWasReset, 0);
		uint16_t hl = REG_HL(gb_);
		cpu_.e = rd8(hl--);
		cpu_.d = rd8(hl--);
		uint8_t duration = alu_.sub(rd8(hl), 1);
		cpu_.a = duration;
		wr8(hl--, duration);
		if (!(cpu_.f & FLAG_C)) {
			// current frame still showing
			SET_HL(gb_, hl);
			return;
		}

		// new frame
		uint8_t offset = rd8(hl);
		uint8_t c = alu_.add(offset, cpu_.e);
		uint8_t b = alu_.adc(cpu_.d, 0);
		uint16_t entry = (uint16_t)(b << 8 | c);
		uint8_t frame = rd8(entry);
		if (frame == 0xff) {
			// .reset_anim
			wr8(hl++, 0); // frameset offset
			wr8(hl, 0);   // duration
			wr8(wObjAnimWasReset, TRUE);
			cpu_.a = TRUE;
			cpu_.f = FLAG_Z; // xor a
			SET_BC(gb_, entry);
			SET_HL(gb_, hl);
			return;
		}

		// The asm steps l with dec l / inc l, which never carry into h.
		uint8_t h = (uint8_t)(hl >> 8), l = (uint8_t)hl;
		l = (uint8_t)(l - 2);
		wr8((uint16_t)(h << 8 | l), frame); // frame
		hl = (uint16_t)((h << 8 | l) + 1);      // ld [hli]
		h = (uint8_t)(hl >> 8);
		l = (uint8_t)(hl + 1);                  // inc l
		hl = (uint16_t)(h << 8 | l);
		cpu_.a = alu_.add(rd8(hl), 2);
		wr8(hl++, cpu_.a); // frameset offset
		entry++;
		cpu_.a = rd8(entry);
		wr8(hl, cpu_.a); // duration
		SET_BC(gb_, entry);
		SET_HL(gb_, hl);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeObjectAnimation;

PORT_EX(UpdateObjAnim, 0, 0) { HomeObjectAnimation(gb).port_UpdateObjAnim(); }
