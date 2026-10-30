// Port of src/home/door_transition.asm
#include "ported.h"

PORT(VBlank_DoorTransition)
{
	gb->cpu.ime = false; // di
	// back up current VBlank function
	SET_HL(gb, wVBlankFunc);
	SET_DE(gb, wBackupVBlankFunc);
	gb->cpu.b = 3;
	CALL(CopyHLToDE);

	// then overwrite it
	SET_HL(gb, ADDR_VBlank_DoorTransition + 0x19); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 3; // .end - .Func
	CALL(CopyHLToDE);
	gb->cpu.ime = true; // ei
	gb->cpu.ime_delay = 0;
}

// DoorTransition.PrepareNextStepInTransitionEffect
static void prepare_next_step(GB *gb)
{
	gb->cpu.a = rd8(gb, wDoorTransitionCounter);
	alu_cp(gb, gb->cpu.a, 16);
	if (!(gb->cpu.f & FLAG_C))
		return; // >= 16
	uint16_t hl = wTransitionEffectStep;
	gb->cpu.a = alu_add(gb, rd8(gb, hl), 2);
	wr8(gb, hl++, gb->cpu.a);
	// wDoorTransitionEffectBGPtr: one row below
	gb->cpu.a = alu_add(gb, rd8(gb, hl), TILEMAP_WIDTH);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = alu_adc(gb, rd8(gb, hl), 0x00);
	alu_cp(gb, gb->cpu.a, (uint8_t)((v0BGMap0 + TILEMAP_WIDTH * TILEMAP_HEIGHT) >> 8));
	if (!(gb->cpu.f & FLAG_C))
		gb->cpu.a = alu_sub(gb, gb->cpu.a, (uint8_t)((TILEMAP_WIDTH * TILEMAP_HEIGHT) >> 8)); // wrap up
	// .no_wrap_up
	wr8(gb, hl--, gb->cpu.a);

	// one column left
	gb->cpu.a = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, gb->cpu.a);
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x1f);
	alu_cp(gb, gb->cpu.a, 0x1f);
	if (gb->cpu.f & FLAG_Z) {
		// wrap right
		gb->cpu.a = alu_add(gb, rd8(gb, hl), TILEMAP_WIDTH);
		wr8(gb, hl, gb->cpu.a);
	}
	SET_HL(gb, hl);
}

PORT(DoorTransition)
{
	// push_wram $03
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = 0x03;
	wr8(gb, rWBK, gb->cpu.a);
	gb->cpu.a = rd8(gb, wDoorTransitionOrientation);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_or(gb, rd8(gb, wDoorTransitionCounter), gb->cpu.c);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0x01;
		wr8(gb, wUnused_c186, 0x01);

		// set VBlank function
		CALL(VBlank_DoorTransition);

		// toggle wDoorTransitionOrientation
		gb->cpu.a = alu_xor(gb, rd8(gb, wDoorTransitionOrientation), 0x1);
		wr8(gb, wDoorTransitionOrientation, gb->cpu.a);

		gb->cpu.a = 1;
		wr8(gb, wTransitionEffectStep, 1);
		// start wDoorTransitionEffectBGPtr with Wario's current tile
		gb->cpu.a = rd8(gb, wWarioTileBGPtr + 0);
		wr8(gb, wDoorTransitionEffectBGPtr + 0, gb->cpu.a);
		gb->cpu.a = rd8(gb, wWarioTileBGPtr + 1);
		wr8(gb, wDoorTransitionEffectBGPtr + 1, gb->cpu.a);
	} else {
		// .already_started: toggle wDoorTransitionOrientation
		gb->cpu.a = alu_xor(gb, rd8(gb, wDoorTransitionOrientation), 0x1);
		wr8(gb, wDoorTransitionOrientation, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z)) {
			prepare_next_step(gb);
		} else {
			// .skip_incr_step
			SET_HL(gb, wDoorTransitionCounter);
			wr8(gb, wDoorTransitionCounter, alu_inc(gb, rd8(gb, wDoorTransitionCounter)));
			gb->cpu.a = rd8(gb, wDoorTransitionCounter);
			alu_cp(gb, gb->cpu.a, 17);
			if (!(gb->cpu.f & FLAG_C)) {
				// restore backed up VBlank function
				gb->cpu.ime = false; // di
				SET_HL(gb, wBackupVBlankFunc);
				SET_DE(gb, wVBlankFunc);
				gb->cpu.b = 3;
				CALL(CopyHLToDE);
				gb->cpu.ime = true; // ei
				gb->cpu.ime_delay = 0;

				// finish door transition and advance level state
				gb->cpu.a = 0;
				gb->cpu.f = FLAG_Z; // xor a
				wr8(gb, wDoorTransitionCounter, 0);
				wr8(gb, wUnused_c186, 0);
				wr8(gb, wDoorTransitionOrientation, 0);
				// stop_music
				wr8(gb, hMusicID + 0, 0xff);
				wr8(gb, hMusicID + 1, 0x00);
				// stop_sfx
				wr8(gb, hSFXID + 0, 0xff);
				gb->cpu.a = 0x00;
				wr8(gb, hSFXID + 1, 0x00);
				SET_HL(gb, wSubState);
				wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
			}
		}
	}
	// .done: pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

// one tile of the door transition effect: set the attribute's palette to 7
// (and $7f / or $0f, in VRAM1), then overwrite the tile with $7f
static void cover_tile(GB *gb, uint16_t hl)
{
	wr8(gb, rVBK, 1); // BANK("VRAM1")
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & 0x7f) | 0x0f));
	wr8(gb, rVBK, 0); // BANK("VRAM0")
	wr8(gb, hl, 0x7f);
}

// do b tiles up (add hl, -TILEMAP_WIDTH, wrapping down below v0BGMap0)
static uint16_t fill_upwards(GB *gb, uint16_t hl, uint8_t b)
{
	do {
		cover_tile(gb, hl);
		hl = (uint16_t)(hl - TILEMAP_WIDTH);
		if ((hl >> 8) < (v0BGMap0 >> 8))
			hl = (uint16_t)((((hl >> 8) + 0x04) & 0xff) << 8 | (hl & 0xff)); // wrap down
	} while (--b);
	return hl;
}

// do b tiles left (ld [hli], wrapping a row back when l reaches a multiple of 32)
static uint16_t fill_left(GB *gb, uint16_t hl, uint8_t b)
{
	do {
		cover_tile(gb, hl);
		hl++;
		if (!(hl & 0x1f))
			hl = (uint16_t)(hl - TILEMAP_WIDTH); // wrap left
	} while (--b);
	return hl;
}

// reached from wVBlankFunc's jp
PORT_LOCAL(VBlank_DoorTransition, DoTransitionEffect)
{
	// apply scroll
	wr8(gb, rSCY, rd8(gb, wSCY));
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);

	// push_wram $03
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	wr8(gb, rWBK, 0x03);
	SET_DE(gb, (uint16_t)-TILEMAP_WIDTH);
	uint8_t orientation = rd8(gb, wDoorTransitionOrientation);
	uint8_t step = rd8(gb, wTransitionEffectStep);
	uint16_t hl = (uint16_t)(rd8(gb, wDoorTransitionEffectBGPtr + 0) |
		rd8(gb, wDoorTransitionEffectBGPtr + 1) << 8);
	gb->cpu.c = step;
	if (orientation) {
		// up then left
		hl = fill_upwards(gb, hl, step);
		hl = fill_left(gb, hl, (uint8_t)(step + 1));
	} else {
		// .left_then_up
		hl = fill_left(gb, hl, step);
		hl = fill_upwards(gb, hl, step);
	}
	gb->cpu.b = 0;
	SET_HL(gb, hl);
	// .asm_a8e: pop_wram
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}
