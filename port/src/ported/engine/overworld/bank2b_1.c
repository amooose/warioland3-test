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

// bc = OW obj: updates its animation with the credits OAM
PORT(Func_ac000)
{
	gb->cpu.h = gb->cpu.b;
	gb->cpu.l = (uint8_t)(gb->cpu.c & ~(1 << 1));
	gb->cpu.b = BANK_OAM_15e44a; // BANK("Credits OAM")
	CALL(UpdateOWAnimation);
	gb->cpu.a = rd8(gb, REG_BC(gb));
}

PORT_JUMP(Func_ac042) { gb->cpu.b = BANK_OAM_15e44a; JUMP(LoadOWSpriteWithScroll); }
PORT_JUMP(Func_ac047) { gb->cpu.b = BANK_OAM_15e44a; JUMP(LoadOWSpriteWithScroll); }
PORT_JUMP(Func_ac04c) { gb->cpu.b = BANK_OAM_15e44a; JUMP(LoadOWSpriteWithScroll); }
PORT_JUMP(Func_ac051) { gb->cpu.b = BANK_OAM_15e44a; JUMP(LoadOWSprite); }

// return nz if animation is not finished yet
// e = new state
PORT(SetObjStateIfAnimationIsFinished)
{
	gb->cpu.a = rd8(gb, wOWAnimationFinished);
	alu_cp(gb, gb->cpu.a, 0xff);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	uint16_t hl = (uint16_t)(REG_HL(gb) | 0x04); // set 2, l
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0); // duration
	wr8(gb, hl++, 0); // frameset offset
	gb->cpu.a = gb->cpu.e;
	wr8(gb, hl, gb->cpu.a); // state
	SET_HL(gb, hl);
	SET_BC(gb, hl);
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

// The first value of each SCENEWARIO_GROUP_* after the group
static const uint8_t scene_wario_group_ends[] = {
	SCENEWARIO_GROUP_00, SCENEWARIO_GROUP_01, SCENEWARIO_GROUP_02, SCENEWARIO_GROUP_03,
	SCENEWARIO_GROUP_04, SCENEWARIO_GROUP_05, SCENEWARIO_GROUP_06, SCENEWARIO_GROUP_07,
	SCENEWARIO_GROUP_08, SCENEWARIO_GROUP_19, SCENEWARIO_GROUP_10, SCENEWARIO_GROUP_11,
	SCENEWARIO_GROUP_12,
};

// .LoadGfx: hl = wSceneWarioStateGroup, b = group. Sets the sprite and
// starts a DMA of the group's tiles.
static void scene_wario_load_gfx(GB *gb, uint16_t hl)
{
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl++, gb->cpu.a); // wSceneWarioStateGroup
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a); // *2
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a); // *4
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c); // *6
	uint16_t table = ADDR_SetSceneWarioState + 0x8e; // .WarioSpritePointers
	gb->cpu.a = alu_add(gb, gb->cpu.a, (uint8_t)table);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.a = alu_adc(gb, (uint8_t)(table >> 8), 0);
	gb->cpu.d = gb->cpu.a;
	uint16_t de = REG_DE(gb);
	wr8(gb, hl++, rd8(gb, de++)); // wSceneWarioOAMPtr
	wr8(gb, hl++, rd8(gb, de++));
	wr8(gb, hl, rd8(gb, de++)); // wSceneWarioOAMBank
	wr8(gb, wHDMABank, rd8(gb, de++));

	gb->cpu.b = rd8(gb, de++);
	gb->cpu.a = rd8(gb, de);
	wr8(gb, wHDMA + 0, gb->cpu.a);
	wr8(gb, wHDMA + 1, gb->cpu.b);
	wr8(gb, wHDMA + 2, 0);
	wr8(gb, wHDMA + 3, 0);
	wr8(gb, wHDMA + 4, 0x80 - 1); // $80 dma_tiles
	SET_DE(gb, de);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wHDMADestVRAMBank, 0);
}

// a = SCENEWARIO_* state
PORT(SetSceneWarioState)
{
	uint8_t a = gb->cpu.a;
	wr8(gb, wSceneWarioState, a);
	uint8_t b = 0;
	for (unsigned i = 0; i < sizeof(scene_wario_group_ends); i++, b++) {
		alu_cp(gb, a, scene_wario_group_ends[i]);
		if (gb->cpu.f & FLAG_C)
			break;
	}
	if (b == sizeof(scene_wario_group_ends))
		alu_cp(gb, a, SCENEWARIO_GROUP_13);
	gb->cpu.b = b;

	gb->cpu.a = rd8(gb, wSceneWarioStateGroup);
	alu_cp(gb, gb->cpu.a, b);
	if (!(gb->cpu.f & FLAG_Z))
		scene_wario_load_gfx(gb, wSceneWarioStateGroup);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneWarioDuration + 0, 0);
	wr8(gb, wSceneWarioDuration + 1, 0);
	SET_HL(gb, wSceneWarioDuration + 1);
	wr8(gb, w2d14c, 0);
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

// bc = OW obj: updates its animation with the OAM in bank `bank`
static void update_ow_obj_animation(GB *gb, uint8_t bank)
{
	gb->cpu.h = gb->cpu.b;
	gb->cpu.l = (uint8_t)(gb->cpu.c & ~(1 << 1));
	gb->cpu.b = bank;
	CALL(UpdateOWAnimation);
	gb->cpu.a = rd8(gb, REG_BC(gb));
}

PORT(Func_ac00b) { update_ow_obj_animation(gb, BANK_OAM_15e44a); } // BANK("Credits OAM")
PORT(Func_ac016) { update_ow_obj_animation(gb, BANK_OAM_15e44a); } // BANK("Credits OAM")
PORT(Func_ac021) { update_ow_obj_animation(gb, 0x1a); } // BANK("Object Framesets")
PORT(Func_ac02c) { update_ow_obj_animation(gb, 0x01); } // BANK("Title Gfx")
PORT(Func_ac037) { update_ow_obj_animation(gb, 0x2a); } // BANK("Overworld OAM 1")

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

PORT(Func_ac4fa)
{
	SET_HL(gb, w2d14c);
	gb->cpu.a = rd8(gb, w2d14c);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		play_sfx(gb, SFX_12A);
	wr8(gb, w2d14c, alu_inc(gb, rd8(gb, w2d14c)));
	gb->cpu.c = 0x24;
	gb->cpu.a = rd8(gb, w2d14c);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d14c, 0);
}

PORT(Func_ac515)
{
	SET_HL(gb, w2d14c);
	wr8(gb, w2d14c, alu_inc(gb, rd8(gb, w2d14c)));
	gb->cpu.a = rd8(gb, w2d14c);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d14c, 0);
	play_sfx(gb, SFX_137);
}

PORT(SetFightAgainstAHiddenFigure)
{
	// stop_music2
	gb->cpu.a = 0xff;
	wr8(gb, hMusicIDBackup + 0, 0xff);
	wr8(gb, hMusicID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hMusicIDBackup + 1, 0x00);
	wr8(gb, hMusicID + 1, 0x00);
	// stop_sfx
	gb->cpu.a = 0xff;
	wr8(gb, hSFXID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hSFXID + 1, 0x00);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSubState, 0);
	SET_HL(gb, wState);
	wr8(gb, wState, alu_inc(gb, rd8(gb, wState)));
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | FLAG_C); // scf
}

// hl = scene obj y coord
PORT(InitTempleSceneMusicBox)
{
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, 0x50);
	hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl + 1)); // inc l
	wr8(gb, hl, 0x4e);
	hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl + 1)); // inc l
	hl |= 0x04; // set 2, l
	SET_HL(gb, hl);
	CALL(SetSceneObjState);
}

// hl = scene obj x coord: moves it up 1 and left 2, returns carry while
// y is < $2c
PORT(MoveMusicBoxUpLeft)
{
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 1)); // dec l
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x2c);
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

PORT_JUMP(UpdateSceneWario)
{
	if (rd8(gb, wSceneWarioState) >= NUM_SCENEWARIO_STATES) {
		// out of the table: run the asm's call .DoAnimation as is
		gb_push16(gb, ADDR_UpdateSceneWario + 3);
		gb->cpu.a = rd8(gb, wSceneWarioState);
		JUMP_TO(jumptable(gb, ADDR_UpdateSceneWario + 0x23));
		return;
	}
	scene_wario_do_animation(gb);
	gb->cpu.a = rd8(gb, wSceneWarioState);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	gb->cpu.e = rd8(gb, wSceneWarioOAMPtr + 0);
	gb->cpu.d = rd8(gb, wSceneWarioOAMPtr + 1);
	gb->cpu.a = rd8(gb, wSceneWarioOAMBank);
	gb->cpu.b = gb->cpu.a;
	SET_HL(gb, wSceneWario);
	gb->cpu.a = rd8(gb, wSceneWarioIgnoreScroll);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		JUMP(LoadOWSpriteWithScroll);
	else
		JUMP(LoadOWSprite);
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

PORT_JUMP(Func_ac056)
{
	SET_DE(gb, ADDR_Frameset_76ca);
	gb_call(gb, ADDR_Func_ac02c);
	SET_DE(gb, ADDR_OAM_75c3);
	gb->cpu.b = BANK_OAM_75c3;
	JUMP(LoadOWSprite);
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

// bc = obj state: animates the temple scene obj (Credits OAM framesets)
PORT_JUMP(UpdateTempleSceneObjAnimation)
{
	for (;;) {
		gb->cpu.a = rd8(gb, REG_BC(gb));
		uint8_t state = gb->cpu.a;
		uint16_t target = jumptable(gb, ADDR_UpdateTempleSceneObjAnimation + 2);
		static const uint16_t framesets[] = {
			0, 0, ADDR_Frameset_15e796, ADDR_Frameset_15e89c, ADDR_Frameset_15e89f,
			ADDR_Frameset_15e8a2, ADDR_Frameset_15e8a5, ADDR_Frameset_15e8a8,
			ADDR_Frameset_15e88d, ADDR_Frameset_15e896, ADDR_Frameset_15e893,
			ADDR_Frameset_15e890, ADDR_Frameset_15e899, ADDR_Frameset_15e8ab,
			ADDR_Frameset_15e8b0, ADDR_Frameset_15e8b0, ADDR_Frameset_15e8ab,
			ADDR_Frameset_15e932, ADDR_Frameset_15e95c,
		};
		if (state == 0) { // .Func_ac358
			RET();
			return;
		}
		if (state >= sizeof(framesets) / sizeof(framesets[0]) || state == 1) {
			JUMP_TO(target); // Func_ac056, or out of the table
			return;
		}
		SET_DE(gb, framesets[state]);
		if (state == 2) {
			// .Func_ac337
			gb_call(gb, ADDR_Func_ac000);
			gb->cpu.e = 0x00;
			gb_call(gb, ADDR_SetObjStateIfAnimationIsFinished);
			if (gb->cpu.f & FLAG_Z)
				continue;
			// .Func_ac331
			SET_DE(gb, ADDR_OAM_1644a);
			JUMP_TO(ADDR_Func_ac042);
			return;
		}
		if (state <= 8) {
			// .Func_ac343
			gb_call(gb, ADDR_Func_ac00b);
			SET_DE(gb, ADDR_OAM_15e813);
			JUMP_TO(ADDR_Func_ac047);
			return;
		}
		if (state <= 12) {
			// .Func_ac38e
			gb_call(gb, ADDR_Func_ac00b);
			SET_DE(gb, ADDR_OAM_15e813);
			gb->cpu.b = BANK_OAM_15e813;
			JUMP(LoadOWSprite);
			return;
		}
		gb_call(gb, ADDR_Func_ac00b);
		if (state == 15 || state == 16) {
			gb->cpu.e = state == 15 ? 0x0d : 0x0e;
			// .Func_ac3d1
			gb_call(gb, ADDR_SetObjStateIfAnimationIsFinished);
			if (gb->cpu.f & FLAG_Z)
				continue;
		}
		// .asm_ac3c4
		SET_DE(gb, ADDR_OAM_18c88a);
		gb->cpu.b = BANK_OAM_18c88a;
		JUMP(LoadOWSpriteWithScroll);
		return;
	}
}

// bc = obj state: animates the obj (Credits OAM framesets)
PORT_JUMP(Func_ac409)
{
	for (;;) {
		gb->cpu.a = rd8(gb, REG_BC(gb));
		uint8_t state = gb->cpu.a;
		uint16_t target = jumptable(gb, ADDR_Func_ac409 + 2);
		static const uint16_t framesets[] = {
			0, 0, ADDR_Frameset_15e741, ADDR_Frameset_15e748, ADDR_Frameset_15e74b,
			ADDR_Frameset_15e774, ADDR_Frameset_15e77d, ADDR_Frameset_15e780,
			ADDR_Frameset_15e793, ADDR_Frameset_15e7a3, ADDR_Frameset_15e7a6,
			ADDR_Frameset_15e7b7, ADDR_Frameset_15e7c4, ADDR_Frameset_15e7ed,
			ADDR_Frameset_15e7b7, ADDR_Frameset_15e783, ADDR_Frameset_15e78a,
			ADDR_Frameset_15e796, ADDR_Frameset_15e7f0, ADDR_Frameset_15e7f5,
			ADDR_Frameset_aad0b, ADDR_Frameset_15e7fa, ADDR_Frameset_15e801,
			ADDR_Frameset_15e80a,
		};
		if (state == 0) { // .Func_ac450
			RET();
			return;
		}
		if (state >= sizeof(framesets) / sizeof(framesets[0]) || state == 1) {
			JUMP_TO(target); // Func_ac056, or out of the table
			return;
		}
		SET_DE(gb, framesets[state]);
		if (state == 20) {
			// .Func_ac4cd
			gb_call(gb, ADDR_Func_ac037);
			gb->cpu.e = 0x00;
			gb_call(gb, ADDR_SetObjStateIfAnimationIsFinished);
			if (gb->cpu.f & FLAG_Z)
				continue;
			SET_DE(gb, ADDR_OAM_aab5c);
			gb->cpu.b = BANK_OAM_aab5c;
			JUMP(LoadOWSpriteWithScroll);
			return;
		}
		gb_call(gb, ADDR_Func_ac000);
		if (state == 14 || state == 16 || state == 17 || state == 21) {
			// .Func_ac444
			gb->cpu.e = state == 21 ? 0x16 : 0x00;
			// .set_new_state_if_finished
			gb_call(gb, ADDR_SetObjStateIfAnimationIsFinished);
			if (gb->cpu.f & FLAG_Z)
				continue;
		}
		// .Func_ac43e
		SET_DE(gb, ADDR_OAM_15e44a);
		if (state == 10 || state == 12) {
			gb_call(gb, ADDR_Func_ac042);
			gb->cpu.a = rd8(gb, wOWAnimationFinished);
			alu_cp(gb, gb->cpu.a, 0xff);
			if (gb->cpu.f & FLAG_Z)
				play_sfx(gb, state == 10 ? SFX_0F3 : SFX_0F4);
			RET();
			return;
		}
		JUMP_TO(ADDR_Func_ac042);
		return;
	}
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

// calls `func` with bc = each of the n obj states, the last one a tail jump
static void for_each_obj_state(GB *gb, uint16_t func, const uint16_t *states, int n)
{
	for (int i = 0; i < n - 1; i++) {
		SET_BC(gb, states[i]);
		gb_call(gb, func);
	}
	SET_BC(gb, states[n - 1]);
	JUMP_TO(func);
}

PORT_JUMP(Func_ac3fd)
{
	static const uint16_t states[] = {wSceneObj8State, wSceneObj9State};
	for_each_obj_state(gb, ADDR_Func_ac409, states, 2);
}

PORT_JUMP(UpdatePrologueSceneObjects)
{
	static const uint16_t states[] = {
		wCurSceneObjState, wSceneObj1State, wSceneObj2State,
		wSceneObj3State, wSceneObj4State, wSceneObj5State,
	};
	for_each_obj_state(gb, ADDR_Func_ac409, states, 6);
}

PORT_JUMP(Func_ac2b2)
{
	static const uint16_t states[] = {wCurSceneObjState, wSceneObj1State, wSceneObj2State};
	for_each_obj_state(gb, ADDR_UpdateTempleSceneObjAnimation, states, 3);
}

PORT_JUMP(UpdateTempleSceneObjs)
{
	static const uint16_t states[] = {
		wSceneObj3State, wSceneObj4State, wSceneObj7State, wSceneObj8State,
		wSceneObj9State, wSceneObj10State, wSceneObj11State, wSceneObj12State,
		wSceneObj13State, wSceneObj14State, wSceneObj15State,
	};
	for_each_obj_state(gb, ADDR_UpdateTempleSceneObjAnimation, states, 11);
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

PORT(_UpdateTempleScene)
{
	gb_call(gb, ADDR__UpdateTempleScene + 0x17); // .UpdateScene
	gb_call(gb, ADDR_Func_ac2b2);
	gb_call(gb, ADDR_UpdateSceneWario);
	gb_call(gb, ADDR_UpdateTempleSceneObjs);
	CALL(ClearUnusedVirtualOAM);
}

// _UpdateTempleScene.NextState
static void temple_scene_next_state(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTempleSceneCounter, 0);
	SET_HL(gb, wTempleSceneCounter - 1);
	wr8(gb, wTempleSceneCounter - 1, alu_inc(gb, rd8(gb, wTempleSceneCounter - 1))); // w2da04
}

PORT_LOCAL(_UpdateTempleScene, NextState) { temple_scene_next_state(gb); }

PORT_LOCAL_JUMP(_UpdateTempleScene, UpdateScene)
{
	SET_HL(gb, wTempleSceneCounter);
	wr8(gb, wTempleSceneCounter, alu_inc(gb, rd8(gb, wTempleSceneCounter)));
	SET_BC(gb, wTempleSceneCounter);
	gb->cpu.a = rd8(gb, wTempleScene);
	JUMP_TO(jumptable(gb, ADDR__UpdateTempleScene + 0x21));
}

// a = scene Wario state: call SetSceneWarioState / jr .NextState
static void set_state_next(GB *gb, uint8_t state)
{
	gb->cpu.a = state;
	gb_call(gb, ADDR_SetSceneWarioState);
	temple_scene_next_state(gb);
}

// ld a, [wTempleSceneCounter] / cp n: returns true on the ret c
static bool temple_counter_below(GB *gb, uint8_t n)
{
	gb->cpu.a = rd8(gb, wTempleSceneCounter);
	alu_cp(gb, gb->cpu.a, n);
	return gb->cpu.f & FLAG_C;
}

// ld a, [wSceneWarioState] / cp state: returns true on the ret nz
static bool scene_wario_state_isnt(GB *gb, uint8_t state)
{
	gb->cpu.a = rd8(gb, wSceneWarioState);
	alu_cp(gb, gb->cpu.a, state);
	return !(gb->cpu.f & FLAG_Z);
}

// ld hl, wTempleSceneCounter / xor a / ld [hld], a / ld [hl], a
static void reset_temple_scene_counters(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wTempleSceneCounter, 0);
	wr8(gb, wTempleSceneCounter - 1, 0); // w2da04
	SET_HL(gb, wTempleSceneCounter - 1);
}

// ld a, [w2da04] / and a / jr z / dec a / jr z ...: the substate, or -1
static int temple_substate(GB *gb, int n)
{
	gb->cpu.a = alu_and(gb, rd8(gb, w2da04), rd8(gb, w2da04));
	for (int i = 0; i < n; i++) {
		if (i)
			gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z)
			return i;
	}
	return -1;
}

PORT_LOCAL(_UpdateTempleScene, Entering)
{
	switch (temple_substate(gb, 5)) {
	case 0: // .start
		set_state_next(gb, SCENEWARIO_WALK_RIGHT);
		return;
	case 1: // .walk_right
		gb_call(gb, ADDR_ApplySceneWarioMovementRight);
		alu_cp(gb, gb->cpu.a, 0x50);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		wr8(gb, REG_HL(gb), gb->cpu.a);
		set_state_next(gb, SCENEWARIO_IDLE_RIGHT);
		return;
	case 2: // .turn
		if (temple_counter_below(gb, 8))
			return;
		set_state_next(gb, SCENEWARIO_TURN_BACK_RIGHT);
		return;
	case 3: // .look_back
		if (scene_wario_state_isnt(gb, SCENEWARIO_LOOK_BACK))
			return;
		temple_scene_next_state(gb);
		return;
	case 4: // .nod
		if (temple_counter_below(gb, 50))
			return;
		gb->cpu.a = SCENEWARIO_NOD;
		gb_call(gb, ADDR_SetSceneWarioState);
		reset_temple_scene_counters(gb);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wCurTextLine, 0);
		wr8(gb, wTextDelayCounter, 0);
		SET_HL(gb, wTempleScene);
		wr8(gb, wTempleScene, TEMPLE_SCENE_TALKING);
		return;
	}
}

PORT_LOCAL(_UpdateTempleScene, TalkToHiddenFigure)
{
	gb_call(gb, ADDR__UpdateTempleScene + 0x105); // .DelayAndPlayTempleMusic
	gb->cpu.a = alu_and(gb, rd8(gb, wIsNextObjectiveTheTemple), rd8(gb, wIsNextObjectiveTheTemple));
	if (gb->cpu.f & FLAG_Z)
		gb_call(gb, ADDR_PrintTextWithHeader);
	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadPressed), PAD_B | PAD_LEFT);
	if (gb->cpu.f & FLAG_Z)
		return;
	// .start_exit
	reset_temple_scene_counters(gb);
	gb_call(gb, ADDR_ClearTextbox);
	SET_HL(gb, wTempleScene);
	wr8(gb, wTempleScene, TEMPLE_SCENE_EXITING);
}

PORT_LOCAL(_UpdateTempleScene, Exiting)
{
	switch (temple_substate(gb, 5)) {
	case 0: // .turn_left
		set_state_next(gb, SCENEWARIO_TURN_BACK_TO_LEFT);
		return;
	case 1: // .idle_left
		if (scene_wario_state_isnt(gb, SCENEWARIO_IDLE_LEFT))
			return;
		temple_scene_next_state(gb);
		return;
	case 2: // .start_walk
		if (temple_counter_below(gb, 4))
			return;
		set_state_next(gb, SCENEWARIO_WALK_LEFT);
		return;
	case 3: // .walk_left
		gb->cpu.a = rd8(gb, wJoypadPressed);
		if (!alu_bit(gb, gb->cpu.a, B_PAD_B)) {
			gb_call(gb, ADDR_ApplySceneWarioMovementLeft);
			alu_cp(gb, gb->cpu.a, 0xf0);
			if (!(gb->cpu.f & FLAG_Z))
				return;
			set_state_next(gb, SCENEWARIO_NONE);
			return;
		}
		break; // .exit
	case 4: // .outside
		if (temple_counter_below(gb, 0x0a))
			return;
		break; // .exit
	default:
		return;
	}
	// .exit
	wr8(gb, hSFXID + 0, 0xff); // stop_sfx
	gb->cpu.a = 0x00;
	wr8(gb, hSFXID + 1, 0x00);
	gb->cpu.a = 0x01;
	wr8(gb, w2d14c, gb->cpu.a);
	gb->cpu.a = SST_OVERWORLD_08;
	wr8(gb, wSubState, gb->cpu.a);
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | FLAG_C); // scf
}

// Func_adbfe (the golf building scene) uses wPalFade6Colour2Green* as its
// state (Current), substate (Target) and frame counter (Sign)
#define GOLF_STATE wPalFade6Colour2GreenCurrent
#define GOLF_SUBSTATE wPalFade6Colour2GreenTarget
#define GOLF_COUNTER wPalFade6Colour2GreenSign

// Func_adbfe.exit
static void golf_building_exit(GB *gb)
{
	wr8(gb, hSFXID + 0, 0xff); // stop_sfx
	gb->cpu.a = 0x00;
	wr8(gb, hSFXID + 1, 0x00);
	gb->cpu.a = 0x01;
	wr8(gb, wOWUIObj2Duration, gb->cpu.a);
	gb->cpu.a = 0x08;
	wr8(gb, wSubState, gb->cpu.a);
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | FLAG_C); // scf
}

PORT(Func_adbfe)
{
	// .Func_adc0b
	bool exit = false;
	gb->cpu.a = rd8(gb, GOLF_STATE);
	alu_cp(gb, gb->cpu.a, 0x04);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = rd8(gb, wJoypadPressed);
		exit = alu_bit(gb, gb->cpu.a, B_PAD_B);
	}
	if (exit) {
		golf_building_exit(gb);
	} else {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		gb_call(gb, ADDR_Func_adbfe + 0x31); // .Func_adc2f
	}
	gb_call(gb, ADDR_UpdateSceneWario);
	CALL(ClearUnusedVirtualOAM);
}

PORT_LOCAL_JUMP(Func_adbfe, Func_adc2f)
{
	SET_HL(gb, GOLF_COUNTER);
	wr8(gb, GOLF_COUNTER, alu_inc(gb, rd8(gb, GOLF_COUNTER)));
	SET_BC(gb, GOLF_COUNTER);
	gb->cpu.a = rd8(gb, GOLF_STATE);
	JUMP_TO(jumptable(gb, ADDR_Func_adbfe + 0x3b));
}

// Func_adbfe.asm_adc43: next substate
static void golf_next_substate(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, GOLF_COUNTER, 0);
	SET_HL(gb, GOLF_SUBSTATE);
	wr8(gb, GOLF_SUBSTATE, alu_inc(gb, rd8(gb, GOLF_SUBSTATE)));
}

// Func_adbfe.asm_adc4a: a = next state
static void golf_set_state(GB *gb)
{
	wr8(gb, GOLF_STATE, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, GOLF_COUNTER, 0);
	wr8(gb, GOLF_SUBSTATE, 0);
	SET_HL(gb, GOLF_SUBSTATE);
}

// ld a, [GOLF_SUBSTATE] / and a / jr z / dec a / jr z ...: the substate,
// or -1 after n checks
static int golf_substate(GB *gb, int n)
{
	gb->cpu.a = alu_and(gb, rd8(gb, GOLF_SUBSTATE), rd8(gb, GOLF_SUBSTATE));
	for (int i = 0; i < n; i++) {
		if (i)
			gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z)
			return i;
	}
	return -1;
}

// ld a, [bc] / cp n: returns true on the ret c
static bool golf_counter_below(GB *gb, uint8_t n)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, n);
	return gb->cpu.f & FLAG_C;
}

// ld a, state / call SetSceneWarioState
static void golf_wario_state(GB *gb, uint8_t state)
{
	gb->cpu.a = state;
	gb_call(gb, ADDR_SetSceneWarioState);
}

// walks scene Wario until his x is end_x: returns true once there
static bool golf_walk(GB *gb, bool right, uint8_t end_x)
{
	gb_call(gb, right ? ADDR_ApplySceneWarioMovementRight : ADDR_ApplySceneWarioMovementLeft);
	alu_cp(gb, gb->cpu.a, end_x);
	return gb->cpu.f & FLAG_Z;
}

// .Func_adc54 / .Func_adc7e: walk out of the building and leave
static void golf_walk_out(GB *gb, uint8_t walk_state, bool right, uint8_t end_x)
{
	switch (golf_substate(gb, 4)) {
	case 0:
		golf_wario_state(gb, walk_state);
		golf_next_substate(gb);
		return;
	case 1:
		if (!golf_walk(gb, right, end_x))
			return;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		gb_call(gb, ADDR_SetSceneWarioState);
		golf_next_substate(gb);
		return;
	case 2:
		if (golf_counter_below(gb, 0x14))
			return;
		golf_next_substate(gb);
		return;
	case 3:
		golf_building_exit(gb);
		return;
	}
}

PORT_LOCAL(Func_adbfe, Func_adc54) { golf_walk_out(gb, SCENEWARIO_WALK_RIGHT, true, 0x90); }
PORT_LOCAL(Func_adbfe, Func_adc7e) { golf_walk_out(gb, SCENEWARIO_WALK_LEFT, false, 0x10); }

// .Func_adca9 / .Func_adcda: walk to the counter and turn to the front
static void golf_walk_in(GB *gb, uint8_t walk_state, bool right, uint8_t idle_state, uint8_t turn_state)
{
	switch (golf_substate(gb, 3)) {
	case 0:
		golf_wario_state(gb, walk_state);
		golf_next_substate(gb);
		return;
	case 1:
		if (!golf_walk(gb, right, 0x50))
			return;
		golf_wario_state(gb, idle_state);
		golf_next_substate(gb);
		return;
	case 2:
		if (golf_counter_below(gb, 0x08))
			return;
		golf_wario_state(gb, turn_state);
		gb->cpu.a = 0x04;
		golf_set_state(gb);
		return;
	}
}

PORT_LOCAL(Func_adbfe, Func_adca9) { golf_walk_in(gb, 0x08, true, 0x0d, 0x34); }
PORT_LOCAL(Func_adbfe, Func_adcda) { golf_walk_in(gb, 0x07, false, 0x0c, 0x35); }

PORT_LOCAL_JUMP(Func_adbfe, Func_add0b)
{
	gb->cpu.a = rd8(gb, GOLF_SUBSTATE);
	JUMP_TO(jumptable(gb, ADDR_Func_adbfe + 0x111));
}

// ld a, [wOWUIObj1State] / cp state / ret nz / jp .asm_adc43
static void golf_wait_ui_obj_state(GB *gb, uint8_t state)
{
	gb->cpu.a = rd8(gb, wOWUIObj1State);
	alu_cp(gb, gb->cpu.a, state);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	golf_next_substate(gb);
}

PORT_LOCAL(Func_adbfe, Func_add21) { golf_wait_ui_obj_state(gb, 0x01); }
PORT_LOCAL(Func_adbfe, Func_add8a) { golf_wait_ui_obj_state(gb, 0x02); }
PORT_LOCAL(Func_adbfe, Func_adda5) { golf_wait_ui_obj_state(gb, 0x0c); }
PORT_LOCAL(Func_adbfe, Func_addb7) { golf_wait_ui_obj_state(gb, 0x0d); }

PORT_LOCAL(Func_adbfe, Func_add2a)
{
	if (golf_counter_below(gb, 0x10))
		return;
	golf_wario_state(gb, 0x3c);
	golf_next_substate(gb);
}

// chooses where to go: d_right, d_left, or a/up to play
PORT_LOCAL(Func_adbfe, Func_add36)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	bool right = alu_bit(gb, gb->cpu.a, B_PAD_RIGHT);
	if (right || alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		gb->cpu.a = right ? WEST : NORTH;
		wr8(gb, wNextMapSide, gb->cpu.a);
		gb->cpu.a = right ? OW_EXIT_LEFT : OW_EXIT_RIGHT;
		wr8(gb, wMapSideLevelID, gb->cpu.a);
		wr8(gb, w2d068, gb->cpu.a);
		golf_wario_state(gb, right ? 0x37 : 0x36);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, GOLF_COUNTER, 0);
		SET_HL(gb, GOLF_SUBSTATE);
		wr8(gb, GOLF_SUBSTATE, right ? 0x07 : 0x05);
		return;
	}
	gb->cpu.a = alu_and(gb, gb->cpu.a, PAD_A | PAD_UP);
	if (gb->cpu.f & FLAG_Z)
		return;
	// .a_btn_or_d_up
	golf_wario_state(gb, 0x30);
	play_sfx(gb, SFX_12A);
	golf_next_substate(gb);
}

PORT_LOCAL(Func_adbfe, Func_add93)
{
	if (golf_counter_below(gb, 0x14))
		return;
	gb->cpu.a = LEVEL_GOLF_BUILDING;
	wr8(gb, wOWLevel, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSubState, 0);
	SET_HL(gb, wState);
	wr8(gb, wState, alu_inc(gb, rd8(gb, wState)));
}

PORT_LOCAL(Func_adbfe, Func_addae)
{
	if (golf_counter_below(gb, 0x04))
		return;
	gb->cpu.a = 0x01;
	golf_set_state(gb);
}

PORT_LOCAL(Func_adbfe, Func_addc0)
{
	if (golf_counter_below(gb, 0x04))
		return;
	gb->cpu.a = 0x00;
	golf_set_state(gb);
}

// decompress_tilemap / decompress_tiles0: decompresses src (in bank) to dest
static void decompress(GB *gb, uint8_t bank, uint16_t src, uint16_t dest)
{
	gb->cpu.a = bank;
	wr8(gb, wTempBank, bank);
	SET_HL(gb, src);
	SET_BC(gb, dest);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
}

// decompress_attrmap / decompress_tiles1: the same, into VRAM bank 1
static void decompress_vram1(GB *gb, uint8_t bank, uint16_t src, uint16_t dest)
{
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, 1);
	decompress(gb, bank, src, dest);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

// decompresses the Japanese or English text (same bank) to wTextBuffer
static void decompress_text(GB *gb, uint16_t jp, uint16_t en, uint8_t bank)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wLanguage), rd8(gb, wLanguage));
	SET_HL(gb, (gb->cpu.f & FLAG_Z) ? jp : en);
	gb->cpu.a = bank;
	wr8(gb, wTempBank, bank);
	SET_BC(gb, wTextBuffer);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
}

PORT(_InitTempleScene)
{
	CALL(DisableLCD);
	CALL(FillBGMap0_With7f);
	CALL(ClearVirtualOAM);
	// stop_music2
	gb->cpu.a = 0xff;
	wr8(gb, hMusicIDBackup + 0, 0xff);
	wr8(gb, hMusicID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hMusicIDBackup + 1, 0x00);
	wr8(gb, hMusicID + 1, 0x00);

	gb->cpu.a = 2; // BANK("WRAM2")
	wr8(gb, rWBK, 2);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCX, 0);
	wr8(gb, wSCX, 0);
	wr8(gb, rSCY, 0);
	wr8(gb, wSCY, 0);

	gb_call(gb, ADDR_VBlank_PrologueEpilogue);
	gb_call(gb, ADDR_ClearGeneralSceneRAM);
	gb_call(gb, ADDR_ClearSceneObjsRAM);

	SET_HL(gb, ADDR_Pals_86a4d);
	gb->cpu.b = BANK_Pals_86a4d;
	CALL(LoadFarPalsToTempPals1);
	decompress_vram1(gb, BANK_FontGfx, ADDR_FontGfx, v1Tiles0);
	SET_HL(gb, ADDR_TempleGfx);
	gb->cpu.b = BANK_TempleGfx;
	CALL(LoadFarTiles);
	SET_HL(gb, ADDR_TempleTilemap);
	gb->cpu.b = BANK_TempleTilemap;
	CALL(LoadFarBGMap);
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, 1);
	SET_HL(gb, ADDR_TempleAttrmap);
	gb->cpu.b = BANK_TempleAttrmap;
	CALL(LoadFarBGMap);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	gb_call(gb, ADDR_FillClearedTextBuffer);

	gb->cpu.a = GOLD_MUSIC_BOX;
	CALL(IsTreasureCollected);
	bool regular = gb->cpu.f & FLAG_Z; // not collected
	gb->cpu.a = TEMPLE_SCENE_ENTERING;
	if (!regular) {
		SET_HL(gb, wGameModeFlags);
		if (alu_bit(gb, rd8(gb, wGameModeFlags), MODE_GAME_CLEARED_F))
			regular = !alu_bit(gb, rd8(gb, wGameModeFlags), MODE_TIME_ATTACK_F);
	}
	if (!regular) {
		// .set_hidden_figure_reveal_scene
		gb->cpu.a = TEMPLE_SCENE_HIDDEN_FIGURE_REVEAL;
		wr8(gb, wTempleScene, gb->cpu.a);
		decompress_vram1(gb, BANK_TempleMusicBoxSceneGfx, ADDR_TempleMusicBoxSceneGfx, v1Tiles0);
		decompress(gb, BANK_HiddenFigureCutsceneTilemap, ADDR_HiddenFigureCutsceneTilemap, v0BGMap1);
		decompress_vram1(gb, BANK_HiddenFigureCutsceneAttrmap, ADDR_HiddenFigureCutsceneAttrmap, v1BGMap1);
		decompress_text(gb, ADDR_TextJP_HiddenFigureReplenishPower, ADDR_TextEN_HiddenFigureReplenishPower, BANK_TextJP_HiddenFigureReplenishPower);
	} else {
		// .set_regular_temple_scene
		wr8(gb, wTempleScene, gb->cpu.a);
		decompress_text(gb, ADDR_TextJP_HiddenFigureHeadFor, ADDR_TextEN_HiddenFigureHeadFor, BANK_TextJP_HiddenFigureHeadFor);
		gb_call(gb, ADDR_LoadNextObjectiveText);
	}

	// .asm_acc68
	gb->cpu.a = 0x60;
	wr8(gb, w1d140, gb->cpu.a);
	gb->cpu.a = 0xe8;
	wr8(gb, wDollBoyNoFlashingBarrel, gb->cpu.a);
	gb->cpu.a = SCENEWARIO_GROUP_NONE;
	wr8(gb, wSceneWarioStateGroup, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a (SCENEWARIO_NONE)
	gb_call(gb, ADDR_SetSceneWarioState);
	gb_call(gb, ADDR_UpdateSceneWario);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d013, 0);
	wr8(gb, w2d014, 0);
	gb->cpu.a = rd8(gb, wLCDCFlagsToFlip);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = LCDC_BG_ON | LCDC_OBJ_ON | LCDC_OBJ_16 | LCDC_WIN_9C00 | LCDC_ON;
	if (!(gb->cpu.f & FLAG_Z))
		gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.b);
	wr8(gb, rLCDC, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wLCDCFlagsToFlip, 0);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}
