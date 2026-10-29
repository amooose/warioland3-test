// Port of src/engine/level/particles.asm
#include "wario_states.h"

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

// --- particle functions (bc = particle) ---

static void inc_c(GB *gb, int n)
{
	while (n--)
		gb->cpu.c = alu_inc(gb, gb->cpu.c);
}

// xor a / ld [bc], a / ld hl, wNumParticles / dec [hl]
static void particle_deinit(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, REG_BC(gb), 0);
	SET_HL(gb, wNumParticles);
	wr8(gb, wNumParticles, alu_dec(gb, rd8(gb, wNumParticles)));
}

// inc c / ldh a, [hPos + i] / ld [bc], a for the 4 bytes of hPos, inc c
static void hpos_to_particle(GB *gb)
{
	for (int i = 0; i < 4; i++) {
		inc_c(gb, 1);
		gb->cpu.a = rd8(gb, hYPosHi + i);
		wr8(gb, REG_BC(gb), gb->cpu.a);
	}
	inc_c(gb, 1);
}

enum { PRE_NONE, PRE_BOUNCY, PRE_MAGIC };
enum { INIT_NONE, INIT_WARIO_POS, INIT_SLIDE_RIGHT, INIT_SLIDE_LEFT, INIT_OWL };
enum { END_ANIM, END_NONE, END_SLIDING, END_OWL };

static void particle_func(GB *gb, int pre, int init, uint16_t anim, int end, uint16_t sfx)
{
	if (pre != PRE_NONE) {
		ld_cp(gb, wWarioState, pre == PRE_BOUNCY ? WST_BOUNCY_AIRBORNE : WST_MAGIC_STOPPING);
		if (pre == PRE_BOUNCY ? !ZF : ZF) {
			particle_deinit(gb);
			return;
		}
	}
	gb_push16(gb, REG_BC(gb));
	inc_c(gb, 1);
	gb->cpu.a = rd8(gb, REG_BC(gb)); // PARTICLE_STRUCT_INITIALISED
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
		wr8(gb, REG_BC(gb), gb->cpu.a);
		inc_c(gb, 5);
		xor_a(gb);
		wr8(gb, REG_BC(gb), 0); // PARTICLE_STRUCT_DURATION
		inc_c(gb, 1);
		wr8(gb, REG_BC(gb), 0); // PARTICLE_STRUCT_FRAMESET_OFFSET
		if (sfx)
			play_sfx(gb, sfx);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
	} else {
		uint16_t hl, de;
		switch (init) {
		case INIT_NONE:
			inc_c(gb, 5);
			break;
		case INIT_WARIO_POS:
			hl = wWarioPos;
			de = hYPosHi;
			for (int i = 0; i < 4; i++) {
				gb->cpu.a = rd8(gb, hl);
				wr8(gb, de, gb->cpu.a);
				if (i < 3) {
					hl++;
					de++;
				}
			}
			SET_HL(gb, hl);
			SET_DE(gb, de);
			hpos_to_particle(gb);
			break;
		case INIT_SLIDE_RIGHT:
		case INIT_SLIDE_LEFT:
			hl = wWarioXPos + 1;
			de = hXPosLo;
			if (init == INIT_SLIDE_RIGHT) {
				gb->cpu.a = alu_sub(gb, rd8(gb, hl--), 0x0c);
				wr8(gb, de--, gb->cpu.a);
				gb->cpu.a = alu_sbc(gb, rd8(gb, hl--), 0x00);
			} else {
				gb->cpu.a = alu_add(gb, rd8(gb, hl--), 0x0c);
				wr8(gb, de--, gb->cpu.a);
				gb->cpu.a = alu_adc(gb, rd8(gb, hl--), 0x00);
			}
			wr8(gb, de--, gb->cpu.a);
			gb->cpu.a = alu_sub(gb, rd8(gb, hl--), 0x0c);
			wr8(gb, de--, gb->cpu.a);
			gb->cpu.a = alu_sbc(gb, rd8(gb, hl), 0x00);
			wr8(gb, de, gb->cpu.a);
			SET_HL(gb, hl);
			SET_DE(gb, de);
			hpos_to_particle(gb);
			break;
		case INIT_OWL:
			inc_c(gb, 2);
			gb->cpu.a = alu_sub(gb, rd8(gb, REG_BC(gb)), 0x02);
			wr8(gb, REG_BC(gb), gb->cpu.a);
			gb->cpu.c = alu_dec(gb, gb->cpu.c);
			gb->cpu.a = alu_sbc(gb, rd8(gb, REG_BC(gb)), 0x00);
			wr8(gb, REG_BC(gb), gb->cpu.a);
			inc_c(gb, 4);
			break;
		}
	}
	gb_call(gb, anim);
	SET_BC(gb, gb_pop16(gb));
	switch (end) {
	case END_NONE:
		return;
	case END_ANIM:
		if (!ld_and(gb, wCurParticleAnimEnded))
			return;
		break;
	case END_SLIDING:
		ld_cp(gb, wWarioState, WST_SLIDING);
		if (ZF)
			return;
		break;
	case END_OWL:
		ld_cp(gb, wCurSpriteYCoord, 0xc0);
		if (CF)
			return;
		break;
	}
	particle_deinit(gb);
}

PORT(ParticleFunc_None) {}
PORT(ParticleFunc_Debris) { particle_func(gb, PRE_NONE, INIT_NONE, ADDR_ParticleAnim_Debris, END_ANIM, SFX_BLOCK_BREAK); }
PORT(ParticleFunc_Stars) { particle_func(gb, PRE_NONE, INIT_NONE, ADDR_ParticleAnim_Stars, END_ANIM, 0); }
PORT(ParticleFunc_Splash) { particle_func(gb, PRE_NONE, INIT_NONE, ADDR_ParticleAnim_Splash, END_ANIM, 0); }
PORT(ParticleFunc_Dust) { particle_func(gb, PRE_NONE, INIT_NONE, ADDR_ParticleAnim_Dust, END_ANIM, 0); }
PORT(ParticleFunc_SlideDustRight) { particle_func(gb, PRE_NONE, INIT_SLIDE_RIGHT, ADDR_ParticleAnim_Dust, END_SLIDING, 0); }
PORT(ParticleFunc_SlideDustLeft) { particle_func(gb, PRE_NONE, INIT_SLIDE_LEFT, ADDR_ParticleAnim_Dust, END_SLIDING, 0); }
PORT(ParticleFunc_SmokeLarge) { particle_func(gb, PRE_NONE, INIT_NONE, ADDR_ParticleAnim_SmokeLarge, END_ANIM, 0); }
PORT(ParticleFunc_SmokeSmall) { particle_func(gb, PRE_NONE, INIT_NONE, ADDR_ParticleAnim_SmokeSmall, END_ANIM, 0); }
PORT(ParticleFunc_Bubbles) { particle_func(gb, PRE_NONE, INIT_NONE, ADDR_ParticleAnim_Bubbles, END_ANIM, 0); }
PORT(ParticleFunc_BouncyJump) { particle_func(gb, PRE_BOUNCY, INIT_WARIO_POS, ADDR_ParticleAnim_BouncyJump, END_ANIM, 0); }
PORT(ParticleFunc_BubbleBurst) { particle_func(gb, PRE_NONE, INIT_NONE, ADDR_ParticleAnim_BubbleBurst, END_ANIM, 0); }
PORT(ParticleFunc_Ice) { particle_func(gb, PRE_NONE, INIT_NONE, ADDR_ParticleAnim_Ice, END_ANIM, 0); }
PORT(ParticleFunc_TreasureGlow) { particle_func(gb, PRE_NONE, INIT_NONE, ADDR_ParticleAnim_TreasureGlow, END_NONE, 0); }
PORT(ParticleFunc_Owl) { particle_func(gb, PRE_NONE, INIT_OWL, ADDR_ParticleAnim_Owl, END_OWL, 0); }
PORT(ParticleFunc_Snow) { particle_func(gb, PRE_NONE, INIT_NONE, ADDR_ParticleAnim_Snow, END_ANIM, 0); }
PORT(ParticleFunc_Magic1) { particle_func(gb, PRE_MAGIC, INIT_WARIO_POS, ADDR_ParticleAnim_Magic1, END_NONE, 0); }
PORT(ParticleFunc_Magic2) { particle_func(gb, PRE_MAGIC, INIT_WARIO_POS, ADDR_ParticleAnim_Magic2, END_NONE, 0); }
PORT(ParticleFunc_MagicStop) { particle_func(gb, PRE_NONE, INIT_WARIO_POS, ADDR_ParticleAnim_MagicStop, END_ANIM, 0); }

// hl = block pointer: hPos = the block's position
PORT(Func_ce3e)
{
	gb->cpu.a = alu_sub(gb, gb->cpu.h, 0xa0);
	gb->cpu.h = gb->cpu.a;
	gb->cpu.a = alu_swap(gb, alu_and(gb, gb->cpu.a, 0xf0));
	gb->cpu.d = gb->cpu.a;
	gb->cpu.a = alu_dec(gb, rd8(gb, wBlockPtrBank));
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.d);
	wr8(gb, hYPosHi, gb->cpu.a);
	gb->cpu.a = alu_swap(gb, alu_and(gb, gb->cpu.h, 0x0f));
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0x10);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_adc(gb, rd8(gb, hYPosHi), 0x00);
	wr8(gb, hYPosHi, gb->cpu.a);
	gb->cpu.a = alu_swap(gb, alu_and(gb, gb->cpu.l, 0xf0));
	wr8(gb, hXPosHi, gb->cpu.a);
	gb->cpu.a = alu_swap(gb, alu_and(gb, gb->cpu.l, 0x0f));
	gb->cpu.a = alu_add(gb, gb->cpu.a, 0x08);
	wr8(gb, hXPosLo, gb->cpu.a);
	gb->cpu.a = alu_adc(gb, rd8(gb, hXPosHi), 0x00);
	wr8(gb, hXPosHi, gb->cpu.a);
}

// b = PARTICLE_* constant, hl = block
PORT_JUMP(CreateParticleInBlock)
{
	gb_call(gb, ADDR_Func_ce3e); // same bank: whatever is mapped
	JUMP_TO(ADDR_CreateParticle); // fallthrough
}

// Runs the function of every particle slot that's in use.
PORT(UpdateParticles)
{
	gb->cpu.a = rd8(gb, wNumParticles);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	static const uint16_t rets[MAX_NUM_PARTICLES] = {
		ADDR_UpdateParticles + 0x0f, ADDR_UpdateParticles + 0x19,
		ADDR_UpdateParticles + 0x23, ADDR_UpdateParticles + 0x2d,
	};
	for (int n = 0; n < MAX_NUM_PARTICLES; n++) {
		uint16_t hl = (uint16_t)(wParticle0 + n * (wParticle1 - wParticle0));
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z)
			continue;
		// call .DoParticleFunc: ld b, h / ld c, l / ld a, [hl] / jumptable
		gb->cpu.b = gb->cpu.h;
		gb->cpu.c = gb->cpu.l;
		gb->cpu.a = rd8(gb, hl);
		gb_push16(gb, rets[n]); // call .DoParticleFunc
		gb_push16(gb, ADDR_UpdateParticles + 0x32); // rst JumpTable: .table (after the $ef)
		uint16_t t = gb_pop16(gb); // pop hl
		gb_pop16(gb); // gb_call_ret pushes the return address again
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.d = 0;
		t = alu_add16(gb, t, gb->cpu.e);
		gb->cpu.a = rd8(gb, t);
		gb->cpu.h = rd8(gb, (uint16_t)(t + 1));
		gb->cpu.l = gb->cpu.a;
		gb_call_ret(gb, REG_HL(gb), rets[n]);
	}
}
