// Port of src/engine/level/particles.asm
#include "ported.h"

// b = PARTICLE_* constant
// Creates a particle at hYPos/hXPos, in the next of the 4 particle slots.
PORT(CreateParticle)
{
	uint8_t index = rd8(gb, wCurParticleIndex);
	uint8_t offset = index;
	for (int i = 0; i < 4; i++)
		offset = alu_add(gb, offset, offset); // *16
	gb->cpu.e = offset;
	gb->cpu.d = 0;
	wr8(gb, wCurParticleIndex, alu_and(gb, (uint8_t)(index + 1), 0x03));

	uint16_t hl = alu_add16(gb, wParticles, offset);
	if (!alu_and(gb, rd8(gb, hl), 0xff)) {
		// a free slot: one more particle
		wr8(gb, wNumParticles, alu_inc(gb, rd8(gb, wNumParticles)));
	}
	wr8(gb, hl, gb->cpu.b); // PARTICLE_STRUCT_ID
	hl = (uint16_t)((hl & 0xff00) | alu_inc(gb, (uint8_t)hl)); // inc l
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, FALSE); // PARTICLE_STRUCT_INITIALISED
	wr8(gb, hl++, rd8(gb, hYPosHi));
	wr8(gb, hl++, rd8(gb, hYPosLo));
	wr8(gb, hl++, rd8(gb, hXPosHi));
	gb->cpu.a = rd8(gb, hXPosLo);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
}

// hl = OAM data
// Adds the current particle's sprite to virtual OAM, relative to the camera.
PORT(LoadParticleSprite)
{
	uint16_t scy = wSCY, scx = wSCX;
	alu_cp(gb, rd8(gb, wCameraConfigFlags),
		CAM_XSCROLL2 | CAM_TRANSITIONS | CAM_BORDER_RIGHT | CAM_BORDER_LEFT);
	if (gb->cpu.f & FLAG_Z) {
		scy = wCameraSCY + 1;
		scx = wCameraSCX + 1;
	}
	gb->cpu.b = rd8(gb, scy);
	wr8(gb, wCurSpriteYCoord,
		alu_sub(gb, alu_add(gb, rd8(gb, wCurParticleYCoord), OAM_Y_OFS), gb->cpu.b));
	gb->cpu.b = rd8(gb, scx);
	wr8(gb, wCurSpriteXCoord,
		alu_sub(gb, alu_add(gb, rd8(gb, wCurParticleXCoord), OAM_X_OFS), gb->cpu.b));
	wr8(gb, wCurSpriteFrame, rd8(gb, wCurParticleFrame));
	gb->cpu.a = rd8(gb, wCurParticleAttributes);
	wr8(gb, wCurSpriteAttributes, gb->cpu.a);
	CALL(LoadSprite);
}

// hl = particle's frame duration, followed by its frameset offset and frame
// de = frameset
PORT(UpdateParticleAnimation)
{
	wr8(gb, wCurParticleAnimEnded, FALSE);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = alu_sub(gb, rd8(gb, hl), 1);
	wr8(gb, hl, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_C)) {
		SET_HL(gb, hl + 2);
		return; // .no_new_frame
	}

	uint8_t offset = rd8(gb, hl + 1);
	gb->cpu.c = alu_add(gb, offset, gb->cpu.e);
	gb->cpu.b = alu_adc(gb, gb->cpu.d, 0);
	uint16_t entry = REG_BC(gb);
	uint8_t frame = rd8(gb, entry);
	alu_cp(gb, frame, 0xff);
	if (frame == 0xff) {
		// .reset_anim
		wr8(gb, hl + 1, 0); // frameset offset
		wr8(gb, hl, 0);     // duration
		wr8(gb, wCurParticleAnimEnded, TRUE);
		gb->cpu.a = TRUE;
		gb->cpu.f = FLAG_Z; // xor a
		SET_HL(gb, hl + 2);
		return;
	}

	wr8(gb, hl + 2, frame);
	// The asm reads the offset again after writing the frame (they can
	// alias, e.g. through I/O registers that auto-increment).
	gb->cpu.a = alu_add(gb, rd8(gb, hl + 1), 2);
	wr8(gb, hl + 1, gb->cpu.a); // frameset offset
	entry++;
	gb->cpu.a = rd8(gb, entry);
	wr8(gb, hl, gb->cpu.a); // duration
	SET_BC(gb, entry);
	SET_HL(gb, hl + 2);
}
