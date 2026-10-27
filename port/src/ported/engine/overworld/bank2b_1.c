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
