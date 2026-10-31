// Port of src/engine/overworld/bank2b_1.asm
#include "ported.h"

// moves scene Wario 1.5 pixels to right
// if d-right is pressed, moves 2 pixels
PORT(ApplySceneWarioMovementRight)
{
	gb->cpu.b = alu_swap(gb, rd8(gb, wJoypadDown));
	// bit 0 is set if d-right is down or wGlobalCounter is odd
	uint8_t a = alu_or(gb, rd8(gb, wGlobalCounter), gb->cpu.b);
	SET_HL(gb, wSceneWarioXCoord);
	wr8(gb, wSceneWarioXCoord, alu_inc(gb, rd8(gb, wSceneWarioXCoord)));
	if (alu_bit(gb, a, 0))
		wr8(gb, wSceneWarioXCoord, alu_inc(gb, rd8(gb, wSceneWarioXCoord)));
	gb->cpu.a = alu_and(gb, rd8(gb, wSceneWarioXCoord), 0xfe);
}

PORT(Func_aca6d)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d014, 0);
	SET_HL(gb, w2d014 - 1);
	wr8(gb, w2d014 - 1, alu_inc(gb, rd8(gb, w2d014 - 1))); // w2d013
}

// Moves scene Wario 1 pixel, and another if bit 0 of a (the d-pad
// direction, or'ed with wGlobalCounter) is set.
static void apply_scene_wario_movement(GB *gb, uint8_t dpad, int dir)
{
	gb->cpu.b = dpad;
	gb->cpu.a = alu_or(gb, rd8(gb, wGlobalCounter), gb->cpu.b);
	SET_HL(gb, wSceneWarioXCoord);
	uint8_t x = rd8(gb, wSceneWarioXCoord);
	x = dir < 0 ? alu_dec(gb, x) : alu_inc(gb, x);
	wr8(gb, wSceneWarioXCoord, x);
	if (alu_bit(gb, gb->cpu.a, 0)) {
		x = dir < 0 ? alu_dec(gb, x) : alu_inc(gb, x);
		wr8(gb, wSceneWarioXCoord, x);
	}
	gb->cpu.a = alu_and(gb, x, 0xfe);
}

// moves scene Wario 1.5 pixels to left
// if d-left is pressed, moves 2 pixels
PORT(ApplySceneWarioMovementLeft)
{
	uint8_t a = alu_swap(gb, rd8(gb, wJoypadDown));
	// rra (carry is clear after swap)
	gb->cpu.f = (a & 1) ? FLAG_C : 0;
	a >>= 1;
	apply_scene_wario_movement(gb, a, -1);
}

// copies 5 bytes from de to hl
PORT(Func_ac9ec)
{
	uint16_t de = REG_DE(gb), hl = REG_HL(gb);
	gb->cpu.c = 0x5;
	do {
		gb->cpu.a = rd8(gb, de++);
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
	} while (gb->cpu.c);
	SET_DE(gb, de);
	SET_HL(gb, hl);
}

// hl = wTextBuffer + a * $40, with the asm's flags (and b and c)
static uint16_t text_buffer_page(GB *gb, uint8_t a)
{
	gb->cpu.b = 0x00;
	a = alu_swap(gb, a);
	a = alu_add(gb, a, a);
	gb->cpu.b = alu_rl(gb, gb->cpu.b);
	a = alu_add(gb, a, a);
	gb->cpu.b = alu_rl(gb, gb->cpu.b);
	gb->cpu.a = gb->cpu.c = a; // *$40
	return alu_add16(gb, wTextBuffer, REG_BC(gb));
}

PORT_JUMP(LoadCurTextPageBufferPage)
{
	SET_HL(gb, text_buffer_page(gb, rd8(gb, wCurTextBufferPage)));
	SET_DE(gb, wTextLine2);
	gb->cpu.b = 3 * TEXT_LINE_LENGTH;
	CALL(CopyHLToDE);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurTextLine, 0);
	JUMP_TO(ADDR_Func_aca6d); // jr
}

// Goes to the next page of the text buffer. Returns carry if it starts
// with '▼'.
PORT(Func_acaf3)
{
	SET_HL(gb, wCurTextBufferPage);
	uint8_t page = alu_inc(gb, rd8(gb, wCurTextBufferPage));
	wr8(gb, wCurTextBufferPage, page);
	uint16_t hl = text_buffer_page(gb, rd8(gb, wCurTextBufferPage));
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x5d); // '▼'
	if (gb->cpu.f & FLAG_Z) {
		gb_call(gb, ADDR_Func_aca6d);
		gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | FLAG_C); // scf
		return;
	}
	SET_HL(gb, wSceneObj1State);
	gb->cpu.a = 0x01;
	CALL(SetSceneObjState);
	SET_HL(gb, w2d013);
	for (int i = 0; i < 3; i++)
		wr8(gb, w2d013, alu_dec(gb, rd8(gb, w2d013)));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d014, 0);
}

// ld hl, start / ld bc, size / xor a / call WriteAToHL_BCTimes
static void clear_ram(GB *gb, uint16_t start, uint16_t size)
{
	SET_HL(gb, start);
	SET_BC(gb, size);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	CALL(WriteAToHL_BCTimes);
}

PORT(ClearGeneralSceneRAM) { clear_ram(gb, 0xd800, 0x0800); } // "WRAM2 2"
PORT(ClearSceneObjsRAM) { clear_ram(gb, wSceneWario, wSceneObjsEnd - wSceneWario); }

PORT(FillClearedTextBuffer)
{
	SET_HL(gb, wClearedTextBuffer);
	gb->cpu.a = 0x7f; // '@'
	SET_BC(gb, 8 * TEXT_LINE_LENGTH);
	CALL(WriteAToHL_BCTimes);
}

PORT_JUMP(DelayTextThenGoToNextLine)
{
	gb->cpu.a = rd8(gb, wTextDelayCounter);
	alu_cp(gb, gb->cpu.a, 10);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	JUMP_TO(ADDR_GoToNextLine); // fallthrough
}

PORT_JUMP(Func_ac9e4)
{
	SET_HL(gb, wPalConfig2);
	JUMP_TO(ADDR_Func_ac9ec); // jr
}

PORT_JUMP(Func_ac9e9)
{
	SET_HL(gb, wPalConfig1);
	JUMP_TO(ADDR_Func_ac9ec); // fallthrough
}

PORT(GoToNextLine)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurTextLinePos, 0);
	wr8(gb, wTextDelayCounter, 0);
	SET_HL(gb, wCurTextLine);
	wr8(gb, wCurTextLine, alu_inc(gb, rd8(gb, wCurTextLine)));
}

// ld hl, counter / xor a / ld [hld], a / inc [hl]
static void reset_and_advance(GB *gb, uint16_t counter)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, counter, 0);
	SET_HL(gb, counter - 1);
	wr8(gb, counter - 1, alu_inc(gb, rd8(gb, counter - 1)));
}

// c = frame to wait for
static void wait_and_advance(GB *gb, uint16_t counter)
{
	gb->cpu.a = rd8(gb, counter);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (gb->cpu.f & FLAG_C)
		return;
	reset_and_advance(gb, counter);
}

PORT(Func_ad330) { wait_and_advance(gb, w2d891); }
PORT(Func_ad335) { reset_and_advance(gb, w2d891); }
PORT(Func_ad011) { wait_and_advance(gb, w2d014); }
PORT(Func_ad016) { reset_and_advance(gb, w2d014); }

// hl = palettes: fills 4 palettes' worth of words with white, twice over
PORT(Func_aca28)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.c = 4 * PAL_SIZE;
	do {
		gb->cpu.a = 0xff; // LOW(PALRGB_WHITE)
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.a = 0x7f; // HIGH(PALRGB_WHITE)
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
	} while (gb->cpu.c);
	SET_HL(gb, hl);
}

PORT(Func_aca63)
{
	SET_HL(gb, wOWUIObj1XCoord);
	wr8(gb, wOWUIObj1XCoord, alu_dec(gb, rd8(gb, wOWUIObj1XCoord)));
}

PORT(Func_aca68)
{
	SET_HL(gb, wOWUIObj1XCoord);
	wr8(gb, wOWUIObj1XCoord, alu_inc(gb, rd8(gb, wOWUIObj1XCoord)));
}

// ld hl, src / ld de, dest / ld b, n / call CopyHLToDE
static void copy_bytes(GB *gb, uint16_t src, uint16_t dest, uint8_t n)
{
	SET_HL(gb, src);
	SET_DE(gb, dest);
	gb->cpu.b = n;
	CALL(CopyHLToDE);
}

PORT(Func_aca09)
{
	copy_bytes(gb, wTempPals2, wTempOBPals, 8 * PAL_SIZE);
	copy_bytes(gb, wTempPals1, wTempBGPals, 8 * PAL_SIZE);
}

PORT(ClearText)
{
	gb->cpu.a = 0x7f; // '@'
	SET_HL(gb, wText);
	SET_BC(gb, 4 * TEXT_LINE_LENGTH);
	CALL(WriteAToHL_BCTimes);
}

// de = BG map coord (low byte of the row and column)
PORT(ClearTextboxAtCoord)
{
	gb->cpu.b = (uint8_t)(((v0BGMap0 + 0x100) >> 8) - 0x80);
	gb->cpu.a = rd8(gb, wBGMapToPrintText);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		gb->cpu.b = (uint8_t)(((v0BGMap1 + 0x100) >> 8) - 0x80);
	uint16_t hl = wHDMA;
	gb->cpu.a = (uint8_t)(wClearedTextBuffer >> 8);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = (uint8_t)wClearedTextBuffer;
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.a = gb->cpu.e;
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = BANK_ClearTextboxAtCoord;
	wr8(gb, wHDMABank, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wHDMADestVRAMBank, 0);
	wr8(gb, wPendingCharDest + 0, 0);
}

PORT_JUMP(DebugSceneWario)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = rd8(gb, wDebugSceneWarioState);
	if (alu_bit(gb, gb->cpu.b, B_PAD_RIGHT)) {
		alu_cp(gb, gb->cpu.a, NUM_SCENEWARIO_STATES - 1);
		if (gb->cpu.f & FLAG_C)
			gb->cpu.a = alu_inc(gb, gb->cpu.a);
	} else {
		if (!alu_bit(gb, gb->cpu.b, B_PAD_LEFT)) {
			RET();
			return;
		}
		uint8_t a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (a)
			gb->cpu.a = alu_dec(gb, gb->cpu.a);
	}
	wr8(gb, wDebugSceneWarioState, gb->cpu.a);
	JUMP_TO(ADDR_SetSceneWarioState); // fallthrough
}

// rst JumpTable, with the table at `table` (right after the rst): leaves every
// register as JumpTable does and returns the target (= hl).
static uint16_t jumptable(GB *gb, uint16_t table)
{
	gb_push16(gb, table); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	return REG_HL(gb);
}

// UpdateSceneWario.UpdateAnimation
static void scene_wario_update_animation(GB *gb, uint16_t frameset)
{
	SET_DE(gb, frameset);
	gb->cpu.a = rd8(gb, wSceneWarioOAMBank);
	gb->cpu.b = gb->cpu.a;
	SET_HL(gb, wSceneWarioDuration);
	CALL(UpdateOWAnimation);
}

static void scene_wario_do_animation(GB *gb);

// UpdateSceneWario.SetStateIfFinished: b = state
static void scene_wario_set_state_if_finished(GB *gb)
{
	gb->cpu.a = rd8(gb, wOWAnimationFinished);
	alu_cp(gb, gb->cpu.a, 0xff);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = gb->cpu.b;
	gb_call(gb, ADDR_SetSceneWarioState);
	scene_wario_do_animation(gb); // jp .DoAnimation
}

static void scene_wario_anim_then_state(GB *gb, uint16_t frameset, uint8_t state)
{
	scene_wario_update_animation(gb, frameset);
	gb->cpu.b = state;
	scene_wario_set_state_if_finished(gb);
}

// UpdateSceneWario.asm_ac65f
static void scene_wario_look_around_sfx(GB *gb)
{
	gb->cpu.a = rd8(gb, wSceneWarioState);
	alu_cp(gb, gb->cpu.a, SCENEWARIO_LOOK_BACK);
	if (gb->cpu.f & FLAG_Z)
		return;
	SET_HL(gb, wSceneWarioFrame);
	gb->cpu.a = rd8(gb, w2d14c);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_ac67e
		gb->cpu.a = rd8(gb, wSceneWarioFrame);
		alu_cp(gb, gb->cpu.a, 0x00);
		if (!(gb->cpu.f & FLAG_Z)) {
			alu_cp(gb, gb->cpu.a, 0x04);
			if (!(gb->cpu.f & FLAG_Z))
				return;
		}
		play_sfx(gb, SFX_0F1);
		gb->cpu.a = 0x01;
		wr8(gb, w2d14c, gb->cpu.a);
		return;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	// .asm_ac672
	gb->cpu.a = rd8(gb, wSceneWarioFrame);
	alu_cp(gb, gb->cpu.a, 0x00);
	if (gb->cpu.f & FLAG_Z)
		return;
	alu_cp(gb, gb->cpu.a, 0x04);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d14c, 0);
}

// UpdateSceneWario.LookUp, after .SetStateIfFinished
static void scene_wario_look_up_sfx(GB *gb)
{
	gb->cpu.a = rd8(gb, wSceneWarioState);
	alu_cp(gb, gb->cpu.a, SCENEWARIO_IDLE_RIGHT);
	if (gb->cpu.f & FLAG_Z)
		return;
	SET_HL(gb, wSceneWarioFrame);
	gb->cpu.a = rd8(gb, w2d14c);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_ac718
		gb->cpu.a = rd8(gb, wSceneWarioFrame);
		alu_cp(gb, gb->cpu.a, 0x0c);
		if (gb->cpu.f & FLAG_Z) {
			play_sfx(gb, SFX_011);
			gb->cpu.a = 0x02;
		} else {
			alu_cp(gb, gb->cpu.a, 0x0d);
			if (!(gb->cpu.f & FLAG_Z))
				return;
			play_sfx(gb, SFX_0F2);
			gb->cpu.a = 0x01;
		}
		wr8(gb, w2d14c, gb->cpu.a); // .asm_ac72c
		return;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_ac70a
		gb->cpu.a = rd8(gb, wSceneWarioFrame);
		alu_cp(gb, gb->cpu.a, 0x0d);
	} else {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		// .asm_ac713
		gb->cpu.a = rd8(gb, wSceneWarioFrame);
		alu_cp(gb, gb->cpu.a, 0x0c);
	}
	// .asm_ac70d
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d14c, 0);
}

// UpdateSceneWario.DoAnimation, for a state within the table
static void scene_wario_do_animation(GB *gb)
{
	gb->cpu.a = rd8(gb, wSceneWarioState);
	uint8_t state = gb->cpu.a;
	jumptable(gb, ADDR_UpdateSceneWario + 0x23);
	switch (state) {
	case SCENEWARIO_NONE: return;
	case SCENEWARIO_LOOK_FRONT: scene_wario_update_animation(gb, ADDR_Frameset_14d18); return;
	case SCENEWARIO_LOOK_BACK: scene_wario_update_animation(gb, ADDR_Frameset_14d15); return;
	case 3: // .TurnFrontLeft
		scene_wario_anim_then_state(gb, ADDR_Frameset_14cbe, SCENEWARIO_IDLE_LEFT); return;
	case 4: // .TurnFrontRight
		scene_wario_anim_then_state(gb, ADDR_Frameset_14cc5, SCENEWARIO_IDLE_RIGHT); return;
	case SCENEWARIO_TURN_BACK_LEFT: scene_wario_anim_then_state(gb, ADDR_Frameset_14d0b, SCENEWARIO_LOOK_BACK); return;
	case SCENEWARIO_TURN_BACK_RIGHT: scene_wario_anim_then_state(gb, ADDR_Frameset_14d10, SCENEWARIO_LOOK_BACK); return;
	case SCENEWARIO_WALK_LEFT:
		scene_wario_update_animation(gb, ADDR_Frameset_149b4);
		gb_call(gb, ADDR_Func_ac4fa);
		return;
	case SCENEWARIO_WALK_RIGHT:
		scene_wario_update_animation(gb, ADDR_Frameset_149c5);
		gb_call(gb, ADDR_Func_ac4fa);
		return;
	case SCENEWARIO_CARRY_WALK:
		scene_wario_update_animation(gb, ADDR_Frameset_149fe);
		gb_call(gb, ADDR_Func_ac4fa);
		return;
	case SCENEWARIO_CARRY_IDLE: scene_wario_update_animation(gb, ADDR_Frameset_14a20); return;
	case SCENEWARIO_PICKUP: scene_wario_anim_then_state(gb, ADDR_Frameset_163ec, SCENEWARIO_CARRY_IDLE); return;
	case SCENEWARIO_IDLE_LEFT: scene_wario_update_animation(gb, ADDR_Frameset_14252); return;
	case SCENEWARIO_IDLE_RIGHT: scene_wario_update_animation(gb, ADDR_Frameset_1425f); return;
	case SCENEWARIO_NOD: scene_wario_update_animation(gb, ADDR_Frameset_d4de4); return;
	case SCENEWARIO_NOD_ONCE: scene_wario_anim_then_state(gb, ADDR_Frameset_d4de4, SCENEWARIO_LOOK_BACK); return;
	case SCENEWARIO_LOOK_AROUND:
		scene_wario_anim_then_state(gb, ADDR_Frameset_aa6a5, SCENEWARIO_LOOK_TURN_FORWARD);
		scene_wario_look_around_sfx(gb);
		return;
	case SCENEWARIO_LOOK_TURN_FORWARD: scene_wario_anim_then_state(gb, ADDR_Frameset_aa6c2, SCENEWARIO_LOOK_FORWARD); return;
	case SCENEWARIO_LOOK_FORWARD: scene_wario_update_animation(gb, ADDR_Frameset_aa6c7); return;
	case SCENEWARIO_LOOK_AROUND_THEN_TURN:
		scene_wario_anim_then_state(gb, ADDR_Frameset_aa6a5, SCENEWARIO_TURN_LEFT_END);
		scene_wario_look_around_sfx(gb);
		return;
	case SCENEWARIO_LOOK_TURN_RIGHT:
		scene_wario_anim_then_state(gb, ADDR_Frameset_aa6ca, SCENEWARIO_TURN_RIGHT_END);
		scene_wario_look_around_sfx(gb);
		return;
	case SCENEWARIO_ON_FIRE: scene_wario_update_animation(gb, ADDR_Frameset_aa8ec); return;
	case SCENEWARIO_CRASH_LAND_RIGHT: scene_wario_anim_then_state(gb, ADDR_Frameset_aa8f5, SCENEWARIO_STUNNED_RIGHT); return;
	case SCENEWARIO_CRASH_TUMBLE: scene_wario_update_animation(gb, ADDR_Frameset_aa902); return;
	case SCENEWARIO_CRASH_TUMBLE_END: scene_wario_anim_then_state(gb, ADDR_Frameset_aa902, SCENEWARIO_STUNNED_RIGHT); return;
	case SCENEWARIO_STUNNED_RIGHT: scene_wario_update_animation(gb, ADDR_Frameset_aa90d); return;
	case SCENEWARIO_LOOK_UP:
		scene_wario_anim_then_state(gb, ADDR_Frameset_aa913, SCENEWARIO_IDLE_RIGHT);
		scene_wario_look_up_sfx(gb);
		return;
	case SCENEWARIO_CRASH_LAND_LEFT: scene_wario_anim_then_state(gb, ADDR_Frameset_aa924, SCENEWARIO_STUNNED_LEFT); return;
	case SCENEWARIO_STUNNED_LEFT: scene_wario_update_animation(gb, ADDR_Frameset_aa931); return;
	case SCENEWARIO_LOOK_MUSIC_BOX: scene_wario_anim_then_state(gb, ADDR_Frameset_aab27, SCENEWARIO_HOLD_MUSIC_BOX); return;
	case SCENEWARIO_PICKUP_MUSIC_BOX: scene_wario_anim_then_state(gb, ADDR_Frameset_aab32, SCENEWARIO_LOOK_UP_SURPRISED); return;
	case SCENEWARIO_HOLD_MUSIC_BOX: scene_wario_update_animation(gb, ADDR_Frameset_aab3f); return;
	case SCENEWARIO_LOOK_UP_SURPRISED: scene_wario_update_animation(gb, ADDR_Frameset_aab42); return;
	case SCENEWARIO_CARRY_SNAP_FINGERS: scene_wario_anim_then_state(gb, ADDR_Frameset_aab45, SCENEWARIO_CARRY_IDLE); return;
	case SCENEWARIO_SLEEP:
		scene_wario_update_animation(gb, ADDR_Frameset_aaccb);
		gb->cpu.a = rd8(gb, wOWAnimationFinished);
		wr8(gb, w2d14e, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, 0xff);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		play_sfx(gb, SFX_SNORE);
		return;
	case SCENEWARIO_WAKE_UP: scene_wario_anim_then_state(gb, ADDR_Frameset_aace2, SCENEWARIO_SIT_IDLE); return;
	case SCENEWARIO_SIT_STILL: scene_wario_update_animation(gb, ADDR_Frameset_aacf7); return;
	case SCENEWARIO_SIT_IDLE: scene_wario_update_animation(gb, ADDR_Frameset_aacfa); return;
	case SCENEWARIO_JUMP_AND_BOW:
		scene_wario_update_animation(gb, ADDR_Frameset_ab03c);
		// .Func_ac7b7
		gb->cpu.a = rd8(gb, wSceneWarioFrame);
		alu_cp(gb, gb->cpu.a, 0x07);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = rd8(gb, wSceneWarioUnk7);
			gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
			if (gb->cpu.f & FLAG_Z) {
				gb->cpu.a = alu_xor(gb, gb->cpu.a, 0x1);
				wr8(gb, wSceneWarioUnk7, gb->cpu.a);
				play_sfx(gb, SFX_LAND);
			}
		}
		gb->cpu.b = SCENEWARIO_LOOK_BACK;
		scene_wario_set_state_if_finished(gb);
		return;
	case SCENEWARIO_SNAP_FINGERS: scene_wario_anim_then_state(gb, ADDR_Frameset_ab06b, SCENEWARIO_LOOK_BACK); return;
	case SCENEWARIO_SHOW_MUSIC_BOXES_1: scene_wario_anim_then_state(gb, ADDR_Frameset_ab08c, SCENEWARIO_SHOW_MUSIC_BOXES_2); return;
	case SCENEWARIO_SHOW_MUSIC_BOXES_5: scene_wario_anim_then_state(gb, ADDR_Frameset_ab098, SCENEWARIO_LOOK_BACK); return;
	case SCENEWARIO_PANIC_JUMP:
		scene_wario_update_animation(gb, ADDR_Frameset_ab09d);
		// .Func_ac7ff
		gb->cpu.a = rd8(gb, wOWAnimationFinished);
		alu_cp(gb, gb->cpu.a, 0xff);
		if (gb->cpu.f & FLAG_Z)
			play_sfx(gb, SFX_LAND);
		gb->cpu.b = SCENEWARIO_LOOK_BACK;
		scene_wario_set_state_if_finished(gb);
		return;
	case SCENEWARIO_PANIC: scene_wario_update_animation(gb, ADDR_Frameset_ab09d); return;
	case SCENEWARIO_BOW: scene_wario_update_animation(gb, ADDR_Frameset_ab0ac); return;
	case SCENEWARIO_SHOW_MUSIC_BOXES_2: scene_wario_anim_then_state(gb, ADDR_Frameset_ab14d, SCENEWARIO_SHOW_MUSIC_BOXES_3); return;
	case SCENEWARIO_SHOW_MUSIC_BOXES_3: scene_wario_update_animation(gb, ADDR_Frameset_ab160); return;
	case SCENEWARIO_SHOW_MUSIC_BOXES_4: scene_wario_anim_then_state(gb, ADDR_Frameset_ab163, SCENEWARIO_SHOW_MUSIC_BOXES_5); return;
	case SCENEWARIO_TURN_FRONT_TO_BACK_RIGHT: scene_wario_anim_then_state(gb, ADDR_Frameset_1fc439, SCENEWARIO_LOOK_BACK); return;
	case SCENEWARIO_TURN_FRONT_TO_BACK_LEFT: scene_wario_anim_then_state(gb, ADDR_Frameset_1fc446, SCENEWARIO_LOOK_BACK); return;
	case SCENEWARIO_TURN_BACK_TO_RIGHT: scene_wario_anim_then_state(gb, ADDR_Frameset_1fc453, SCENEWARIO_IDLE_RIGHT); return;
	case SCENEWARIO_TURN_BACK_TO_LEFT: scene_wario_anim_then_state(gb, ADDR_Frameset_1fc45a, SCENEWARIO_IDLE_LEFT); return;
	case SCENEWARIO_TURN_RIGHT_TO_FRONT: scene_wario_anim_then_state(gb, ADDR_Frameset_1fc461, SCENEWARIO_LOOK_FRONT); return;
	case SCENEWARIO_TURN_LEFT_TO_FRONT: scene_wario_anim_then_state(gb, ADDR_Frameset_1fc46a, SCENEWARIO_LOOK_FRONT); return;
	case SCENEWARIO_TURN_FRONT_TO_LEFT: scene_wario_anim_then_state(gb, ADDR_Frameset_1fc473, SCENEWARIO_IDLE_LEFT); return;
	case SCENEWARIO_TURN_FRONT_TO_RIGHT: scene_wario_anim_then_state(gb, ADDR_Frameset_1fc47c, SCENEWARIO_IDLE_RIGHT); return;
	case SCENEWARIO_TURN_LEFT_END: scene_wario_anim_then_state(gb, ADDR_Frameset_1fc485, SCENEWARIO_IDLE_RIGHT); return;
	case SCENEWARIO_TURN_RIGHT_END: scene_wario_anim_then_state(gb, ADDR_Frameset_1fc488, SCENEWARIO_IDLE_LEFT); return;
	case SCENEWARIO_PANIC_RUN_LEFT:
		scene_wario_update_animation(gb, ADDR_Frameset_16b5a);
		gb_call(gb, ADDR_Func_ac515);
		return;
	case SCENEWARIO_PANIC_RUN_RIGHT:
		scene_wario_update_animation(gb, ADDR_Frameset_16b63);
		gb_call(gb, ADDR_Func_ac515);
		return;
	case SCENEWARIO_IDLE_FRONT: scene_wario_update_animation(gb, ADDR_Frameset_1fc316); return;
	}
}

// prints text given in wTextBuffer
PORT_JUMP(PrintText)
{
	gb->cpu.a = rd8(gb, wPendingCharDest + 0);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	// if A button or down is pressed, then text delay is halved
	gb->cpu.b = 4;
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_A | PAD_DOWN);
	if (!(gb->cpu.f & FLAG_Z))
		gb->cpu.b = alu_srl(gb, gb->cpu.b);
	gb->cpu.a = rd8(gb, wTextDelayCounter);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTextDelayCounter, 0);
	gb->cpu.a = gb->cpu.c;
	wr8(gb, wCurTextBufferLine, gb->cpu.a);
	for (;;) {
		// .got_buffer_pos
		gb->cpu.c = alu_swap(gb, gb->cpu.c);
		gb->cpu.e = gb->cpu.c; // *$10
		gb->cpu.a = rd8(gb, wCurTextLinePos);
		gb->cpu.d = gb->cpu.a;
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
		gb->cpu.c = gb->cpu.a;
		gb->cpu.b = alu_rl(gb, 0);
		uint16_t hl = alu_add16(gb, wText, REG_BC(gb));
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, 0x7f); // '@' (no character)
		if (!(gb->cpu.f & FLAG_Z)) {
			wr8(gb, wPendingCharTile, gb->cpu.a);
			gb->cpu.a = 0x1 | BG_BANK1;
			wr8(gb, wPendingCharAttr, gb->cpu.a);
			hl = 0x9800 + 14 * 0x20 + 2; // hlbgcoord 2, 14
			gb->cpu.a = rd8(gb, wBGMapToPrintText);
			gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
			if (!(gb->cpu.f & FLAG_Z))
				hl = 0x9c00 + 14 * 0x20 + 2; // hlbgcoord 2, 14, v0BGMap1
			gb->cpu.e = alu_sla(gb, gb->cpu.e);
			gb->cpu.a = gb->cpu.d;
			gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.e);
			gb->cpu.e = gb->cpu.a;
			gb->cpu.d = alu_rl(gb, 0);
			// first line always starts 1 tile to the left
			gb->cpu.a = rd8(gb, wCurTextBufferLine);
			gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
			if (gb->cpu.f & FLAG_Z)
				hl = (uint16_t)((hl & 0xff00) | alu_dec(gb, (uint8_t)hl));
			hl = alu_add16(gb, hl, REG_DE(gb));
			gb->cpu.a = (uint8_t)hl;
			wr8(gb, wPendingCharDest + 0, gb->cpu.a);
			gb->cpu.a = (uint8_t)(hl >> 8);
			wr8(gb, wPendingCharDest + 1, gb->cpu.a);
			gb->cpu.a = rd8(gb, wPendingCharTile);
			alu_cp(gb, gb->cpu.a, 0x7e); // ' '
			if (!(gb->cpu.f & FLAG_Z))
				play_sfx(gb, SFX_130);
			SET_HL(gb, wCurTextLinePos);
			wr8(gb, wCurTextLinePos, alu_inc(gb, rd8(gb, wCurTextLinePos)));
			gb->cpu.a = rd8(gb, wCurTextLinePos);
			alu_cp(gb, gb->cpu.a, TEXT_LINE_LENGTH);
			if (!(gb->cpu.f & FLAG_Z)) {
				RET();
				return;
			}
			JUMP_TO(ADDR_GoToNextLine);
			return;
		}
		// .no_char: skip characters until a character is found or the
		// end of the line is reached
		SET_HL(gb, wCurTextLinePos);
		wr8(gb, wCurTextLinePos, alu_inc(gb, rd8(gb, wCurTextLinePos)));
		gb->cpu.a = rd8(gb, wCurTextLinePos);
		alu_cp(gb, gb->cpu.a, TEXT_LINE_LENGTH);
		if (gb->cpu.f & FLAG_Z) {
			JUMP_TO(ADDR_GoToNextLine);
			return;
		}
		gb->cpu.a = rd8(gb, wCurTextBufferLine);
		gb->cpu.c = gb->cpu.a;
	}
}

PORT_JUMP(PrintText_Line1) { gb->cpu.c = 0; JUMP_TO(ADDR_PrintText); }
PORT_JUMP(PrintText_Line2) { gb->cpu.c = 1; JUMP_TO(ADDR_PrintText); }
PORT_JUMP(PrintText_Line3) { gb->cpu.c = 2; JUMP_TO(ADDR_PrintText); }
PORT_JUMP(PrintText_Line4) { gb->cpu.c = 3; JUMP_TO(ADDR_PrintText); }

PORT_JUMP(ClearTextbox)
{
	gb->cpu.d = 6 * TILEMAP_WIDTH;
	gb->cpu.e = 0x8 - 1; // $8 dma_tiles
	JUMP_TO(ADDR_ClearTextboxAtCoord);
}

PORT_JUMP(ClearTextboxExceptHeader)
{
	gb->cpu.d = 7 * TILEMAP_WIDTH;
	gb->cpu.e = 0x6 - 1; // $6 dma_tiles
	JUMP_TO(ADDR_ClearTextboxAtCoord); // fallthrough
}

PORT_JUMP(InitTextPrinting)
{
	gb_call(gb, ADDR_ClearText);
	SET_DE(gb, wTextLine1);
	SET_HL(gb, wTextBuffer);
	gb->cpu.b = TEXT_LINE_LENGTH;
	CALL(CopyHLToDE);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, wTextPrintInfo);
	SET_BC(gb, wTextPrintInfoEnd - wTextPrintInfo);
	CALL(WriteAToHL_BCTimes);
	gb->cpu.a = 0x8e;
	wr8(gb, wSceneObj1 + 0, gb->cpu.a);
	gb->cpu.a = 0x98;
	wr8(gb, wSceneObj1 + 1, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, wSceneObj1State);
	CALL(SetSceneObjState);
	JUMP_TO(ADDR_Func_aca6d);
}

PORT(VBlank_PrologueEpilogue)
{
	SET_HL(gb, ADDR_VBlank_PrologueEpilogue + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 0x7e; // .end - .Func
	CALL(CopyHLToDE);
}

PORT_JUMP(Func_acacd)
{
	gb->cpu.a = 1;
	SET_HL(gb, wCurTextBufferPage);
	alu_cp(gb, gb->cpu.a, rd8(gb, wCurTextBufferPage));
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = rd8(gb, wJoypadPressed);
		if (!alu_bit(gb, gb->cpu.a, B_PAD_A)) {
			RET();
			return;
		}
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		SET_HL(gb, wSceneObj1State);
		CALL(SetSceneObjState);
	}
	// .clear
	gb_call(gb, ADDR_ClearTextboxExceptHeader);
	JUMP_TO(ADDR_Func_aca6d);
}

// prints the line in wCurTextLine, lines[] = the PrintText_Line* for each
static void print_text_lines(GB *gb, const uint16_t *lines, int n)
{
	SET_HL(gb, wTextDelayCounter);
	wr8(gb, wTextDelayCounter, alu_inc(gb, rd8(gb, wTextDelayCounter)));
	gb->cpu.a = alu_and(gb, rd8(gb, wCurTextLine), rd8(gb, wCurTextLine));
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_DelayTextThenGoToNextLine);
		return;
	}
	for (int i = 0; i < n; i++) {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z) {
			JUMP_TO(lines[i]);
			return;
		}
	}
	gb->cpu.a = 0x80;
	wr8(gb, wCurTextLine, gb->cpu.a);
	RET();
}

PORT_JUMP(PrintTextWithoutHeader)
{
	static const uint16_t lines[] = {ADDR_PrintText_Line2, ADDR_PrintText_Line3, ADDR_PrintText_Line4};
	print_text_lines(gb, lines, 3);
}

PORT_JUMP(PrintTextWithHeader)
{
	static const uint16_t lines[] = {ADDR_PrintText_Line1, ADDR_PrintText_Line2, ADDR_PrintText_Line3, ADDR_PrintText_Line4};
	print_text_lines(gb, lines, 4);
}

PORT_JUMP(LoadNextObjectiveText)
{
	SET_DE(gb, wTextLine1);
	SET_HL(gb, wTextBuffer);
	gb->cpu.b = TEXT_LINE_LENGTH;
	CALL(CopyHLToDE);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wIsNextObjectiveTheTemple, 0);
	FARCALL(GetMapSideAndLevelForNextTreasure);
	gb->cpu.a = gb->cpu.b;
	SET_HL(gb, 0x20);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		// .check_the_temple
		gb->cpu.a = alu_and(gb, gb->cpu.d, gb->cpu.d);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = TRUE;
			wr8(gb, wIsNextObjectiveTheTemple, gb->cpu.a);
		}
	} else {
		static const uint16_t offsets[] = {0x120, 0x200, 0x2e0};
		int i;
		for (i = 0; i < 3; i++) {
			SET_HL(gb, offsets[i]);
			gb->cpu.a = alu_dec(gb, gb->cpu.a);
			if (gb->cpu.f & FLAG_Z)
				break;
		}
		if (i == 3) {
			JUMP_TO(ADDR_LoadNextObjectiveText + 0x3a - 3); // debug_nop
			return;
		}
	}
	// .not_temple
	SET_BC(gb, wTextBuffer);
	uint16_t hl = alu_add16(gb, REG_HL(gb), wTextBuffer);
	gb->cpu.a = alu_swap(gb, gb->cpu.d);
	gb->cpu.a = alu_rlca(gb, gb->cpu.a);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = 0x00;
	hl = alu_add16(gb, hl, REG_BC(gb));
	SET_HL(gb, hl);
	gb->cpu.a = alu_and(gb, rd8(gb, wLanguage), rd8(gb, wLanguage));
	if (!(gb->cpu.f & FLAG_Z)) {
		// .english
		SET_DE(gb, wTextLine3);
		gb->cpu.b = 2 * TEXT_LINE_LENGTH;
		CALL(CopyHLToDE);
		SET_DE(gb, wTextLine2);
		SET_HL(gb, wTextBuffer + TEXT_LINE_LENGTH);
		gb->cpu.b = TEXT_LINE_LENGTH;
		CALL(CopyHLToDE);
		RET();
		return;
	}
	SET_DE(gb, wTextLine2);
	gb->cpu.b = TEXT_LINE_LENGTH;
	CALL(CopyHLToDE);
	SET_DE(gb, wTextLine3);
	SET_HL(gb, wTextBuffer + TEXT_LINE_LENGTH);
	gb->cpu.b = TEXT_LINE_LENGTH;
	CALL(CopyHLToDE);
	gb->cpu.a = 0x7f; // '@'
	SET_HL(gb, wTextLine4);
	SET_BC(gb, TEXT_LINE_LENGTH);
	CALL(WriteAToHL_BCTimes);
	RET();
}

PORT_JUMP(Func_acae7)
{
	gb_call(gb, ADDR_PrintTextWithoutHeader);
	gb->cpu.a = rd8(gb, wCurTextLine);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_aca6d);
}

PORT(PrologueFade)
{
	FARCALL(_PrologueFade);
}

PORT_JUMP(Func_aca9e)
{
	gb_call(gb, ADDR_PrintTextWithHeader);
	gb->cpu.a = rd8(gb, wCurTextLine);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	SET_HL(gb, wCurTextBufferPage);
	wr8(gb, wCurTextBufferPage, alu_inc(gb, rd8(gb, wCurTextBufferPage)));
	JUMP_TO(ADDR_Func_aca6d);
}

// Func_adbfe (the golf building scene) uses wPalFade6Colour2Green* as its
// state (Current), substate (Target) and frame counter (Sign)
#define GOLF_STATE wPalFade6Colour2GreenCurrent
#define GOLF_SUBSTATE wPalFade6Colour2GreenTarget
#define GOLF_COUNTER wPalFade6Colour2GreenSign

PORT(Func_ac9ca)
{
	FARCALL(Func_84e43);
	gb->cpu.a = rd8(gb, wPalConfig1TotalSteps);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	wr8(gb, w2d014, gb->cpu.a);
	SET_HL(gb, w2d013);
	wr8(gb, w2d013, alu_inc(gb, rd8(gb, w2d013)));
}

// unreferenced
PORT_JUMP(Func_aca20)
{
	SET_HL(gb, wTempPals1);
	JUMP_TO(ADDR_Func_aca28);
}

PORT_JUMP(Func_aca25)
{
	SET_HL(gb, wTempPals2);
	JUMP_TO(ADDR_Func_aca28); // fallthrough
}

PORT_JUMP(Func_ad329)
{
	gb->cpu.a = rd8(gb, wSceneWarioState);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_ad335);
}

PORT_JUMP(Func_ad33c)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d890, 0);
	wr8(gb, w2d891, 0);
	SET_HL(gb, w2d891);
	JUMP_TO(ADDR_Func_ad016);
}

PORT(HandleTempleShakeAndRocks)
{
	FARCALL(_HandleTempleShakeAndRocks);
}

enum { AC0B6_ANIM, AC0B6_STATE, AC0B6_SFX30, AC0B6_CARRY, AC0B6_SFX08E, AC0B6_SFX11A, AC0B6_BIG };

// HiddenFigureRevealScene: hand-computed local offsets, checked against the ROM
#define HF_REVEAL (ADDR_HiddenFigureRevealScene)

// ld [$ff00+c], a
static void vbf_ldh_c(GB *gb)
{
	wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), gb->cpu.a);
}

// ld a, [hli]
static void vbf_ld_a_hli(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl);
	SET_HL(gb, (uint16_t)(hl + 1));
}

// ld a, [wROMBank] / push af / ld a, b / bankswitch
static void vbf_push_bank_switch(GB *gb)
{
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = gb->cpu.b;
	bankswitch(gb, gb->cpu.a);
}

// pop af / bankswitch
static void vbf_pop_bank(GB *gb)
{
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}

// ld a, [wLCDCFlagsToFlip] / and a / jr z, .skip / ld hl, rLCDC / xor [hl] /
// ld [hl], a / xor a / ld [wLCDCFlagsToFlip], a
static void vbf_flip_lcdc(GB *gb)
{
	gb->cpu.a = rd8(gb, wLCDCFlagsToFlip);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a)
		return;
	SET_HL(gb, rLCDC);
	gb->cpu.a = alu_xor(gb, gb->cpu.a, rd8(gb, rLCDC));
	wr8(gb, rLCDC, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wLCDCFlagsToFlip, 0);
}

// ld a, $02 / ldh [rWBK], a / apply wSCY and wSCX
static void vbf_wram2_scroll(GB *gb)
{
	gb->cpu.a = 0x02; // BANK("WRAM2")
	wr8(gb, rWBK, gb->cpu.a);
	wr8(gb, rSCY, rd8(gb, wSCY));
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
}

// VBlank_PrologueEpilogue.Func, as copied to wVBlankFunc (see ram_code.c).
// Its calls return to base + n, the block's start wherever it runs from.
PORT_LOCAL_JUMP(VBlank_PrologueEpilogue, Func)
{
	uint16_t base = gb->cpu.pc;
	vbf_wram2_scroll(gb);

	gb->cpu.a = alu_and(gb, rd8(gb, wHDMABank), rd8(gb, wHDMABank));
	if (gb->cpu.a) {
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wHDMABank, 0);
		vbf_push_bank_switch(gb);
		gb->cpu.a = rd8(gb, wHDMADestVRAMBank);
		wr8(gb, rVBK, gb->cpu.a);
		SET_HL(gb, wHDMA);
		vbf_ld_a_hli(gb);
		gb->cpu.c = (uint8_t)rVDMA_SRC_HIGH;
		vbf_ldh_c(gb);
		for (int i = 0; i < 4; i++) {
			vbf_ld_a_hli(gb);
			gb->cpu.c = alu_inc(gb, gb->cpu.c);
			vbf_ldh_c(gb);
		}
		vbf_pop_bank(gb);
	}

	// .skip_hdma_config
	SET_HL(gb, wPendingCharDest);
	gb->cpu.a = alu_and(gb, rd8(gb, wPendingCharDest), rd8(gb, wPendingCharDest));
	if (gb->cpu.a) {
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wPendingCharDest, 0);
		SET_HL(gb, wPendingCharDest + 1);
		vbf_ld_a_hli(gb);
		gb->cpu.d = gb->cpu.a;
		vbf_ld_a_hli(gb);
		wr8(gb, REG_DE(gb), gb->cpu.a); // wPendingCharTile
		gb->cpu.a = 0x01; // BANK("VRAM1")
		wr8(gb, rVBK, gb->cpu.a);
		gb->cpu.a = rd8(gb, REG_HL(gb)); // wPendingCharAttr
		wr8(gb, REG_DE(gb), gb->cpu.a);
	}

	// .skip_char
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, wPalConfig1);
	gb->cpu.a = alu_and(gb, rd8(gb, wPalConfig1), rd8(gb, wPalConfig1));
	if (gb->cpu.a)
		gb_call_ret(gb, ADDR_ApplyPalConfig, (uint16_t)(base + 0x62));

	SET_HL(gb, wPalConfig2);
	gb->cpu.a = alu_and(gb, rd8(gb, wPalConfig2), rd8(gb, wPalConfig2));
	if (gb->cpu.a)
		gb_call_ret(gb, ADDR_ApplyPalConfig, (uint16_t)(base + 0x6a));

	vbf_flip_lcdc(gb);

	// .dma_transfer
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	JUMP_TO(hTransferVirtualOAM);
}

// _GolfBuilding.func, as copied to wVBlankFunc (see ram_code.c): starts the
// HDMA transfer in wHDMA, if there's one
PORT_LOCAL_JUMP(_GolfBuilding, func)
{
	gb->cpu.a = 2; // BANK("WRAM2")
	wr8(gb, rWBK, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCY);
	wr8(gb, rSCY, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
	gb->cpu.a = rd8(gb, wHDMABank);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wHDMABank, 0);
		gb->cpu.a = rd8(gb, wROMBank);
		push_af(gb);
		gb->cpu.a = gb->cpu.b;
		bankswitch(gb, gb->cpu.a);
		uint16_t hl = wHDMA;
		gb->cpu.c = (uint8_t)rVDMA_SRC_HIGH;
		for (int i = 0; i < 5; i++) {
			if (i)
				gb->cpu.c = alu_inc(gb, gb->cpu.c);
			gb->cpu.a = rd8(gb, hl++);
			wr8(gb, (uint16_t)(0xff00 | gb->cpu.c), gb->cpu.a);
		}
		SET_HL(gb, hl);
		pop_af(gb);
		bankswitch(gb, gb->cpu.a);
	}
	// .done
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	JUMP_TO(hTransferVirtualOAM);
}
