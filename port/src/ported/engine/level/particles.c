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

// bc = particle
// Updates the particle's animation with the frameset in de and loads its
// sprite: hl is left at the particle's frame by UpdateParticleAnimation.
static void particle_anim(GB *gb, uint16_t frameset, uint16_t y_dest, uint16_t oam)
{
	gb->cpu.h = gb->cpu.b;
	gb->cpu.l = gb->cpu.c;
	SET_DE(gb, frameset);
	gb_call(gb, ADDR_UpdateParticleAnimation); // same bank: whatever is mapped
	uint16_t hl = REG_HL(gb);
	wr8(gb, wCurParticleFrame, rd8(gb, hl--));
	hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 2)); // dec l / dec l
	wr8(gb, wCurParticleXCoord, rd8(gb, hl--));
	hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 1)); // dec l
	gb->cpu.a = rd8(gb, hl--);
	wr8(gb, y_dest, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a (after the dec l)
	wr8(gb, wCurParticleAttributes, 0);
	SET_HL(gb, oam);
	JUMP_TO(ADDR_LoadParticleSprite);
}

// The frameset for Wario's direction: and a sets the flags the call sees.
static uint16_t frameset_by_direction(GB *gb, uint16_t left, uint16_t right)
{
	gb->cpu.a = rd8(gb, wDirection);
	return alu_and(gb, gb->cpu.a, gb->cpu.a) ? right : left;
}

PORT_JUMP(ParticleAnim_Bubbles) { particle_anim(gb, frameset_by_direction(gb, ADDR_Frameset_d502, ADDR_Frameset_d50b), wCurParticleYCoord, ADDR_OAM_d20e); }
PORT_JUMP(ParticleAnim_Magic1) { particle_anim(gb, ADDR_Frameset_d8e7, wCurParticle, ADDR_OAM_d80d); }
PORT_JUMP(ParticleAnim_Magic2) { particle_anim(gb, ADDR_Frameset_d8f4, wCurParticle, ADDR_OAM_d80d); }
PORT_JUMP(ParticleAnim_MagicStop) { particle_anim(gb, ADDR_Frameset_d901, wCurParticle, ADDR_OAM_d80d); }
PORT_JUMP(ParticleAnim_Owl) { particle_anim(gb, ADDR_Frameset_d7b4, wCurParticleYCoord, ADDR_OAM_d763); }
PORT_JUMP(ParticleAnim_TreasureGlow) { particle_anim(gb, ADDR_Frameset_d75a, wCurParticleYCoord, ADDR_OAM_d716); }
PORT_JUMP(ParticleAnim_BubbleBurst) { particle_anim(gb, ADDR_Frameset_d622, wCurParticleYCoord, ADDR_OAM_d5ae); }
PORT_JUMP(ParticleAnim_BouncyJump) { particle_anim(gb, ADDR_Frameset_d591, wCurParticleYCoord, ADDR_OAM_d532); }
PORT_JUMP(ParticleAnim_Snow) { particle_anim(gb, ADDR_Frameset_d806, wCurParticle, ADDR_OAM_d7bd); }
PORT_JUMP(ParticleAnim_Ice) { particle_anim(gb, frameset_by_direction(gb, ADDR_Frameset_d6fd, ADDR_Frameset_d6e4), wCurParticleYCoord, ADDR_OAM_d62b); }
PORT_JUMP(ParticleAnim_Dust) { particle_anim(gb, ADDR_Frameset_d4c5, wCurParticleYCoord, ADDR_OAM_d20e); }
PORT_JUMP(ParticleAnim_Debris) { particle_anim(gb, ADDR_Frameset_d49a, wCurParticleYCoord, ADDR_OAM_d20e); }
PORT_JUMP(ParticleAnim_Stars) { particle_anim(gb, ADDR_Frameset_d525, wCurParticleYCoord, ADDR_OAM_d20e); }
PORT_JUMP(ParticleAnim_SmokeLarge) { particle_anim(gb, ADDR_Frameset_d4f3, wCurParticleYCoord, ADDR_OAM_d20e); }
PORT_JUMP(ParticleAnim_SmokeSmall) { particle_anim(gb, ADDR_Frameset_d4ea, wCurParticleYCoord, ADDR_OAM_d20e); }
PORT_JUMP(ParticleAnim_Splash) { particle_anim(gb, ADDR_Frameset_d4ba, wCurParticleYCoord, ADDR_OAM_d20e); }
