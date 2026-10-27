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
