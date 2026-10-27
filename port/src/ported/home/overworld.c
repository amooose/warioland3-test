// Port of src/home/overworld.asm
#include "ported.h"

// ld a, [wROMBank] / push af / ld a, b / bankswitch
static void push_bank_switch_to_b(GB *gb)
{
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = gb->cpu.b;
	bankswitch(gb, gb->cpu.a);
}

// pop af / bankswitch
static void pop_bank(GB *gb)
{
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}

// input:
// - hl = wSceneObjXDuration
// - de = frameset pointer
// - b  = bank
// output:
// - bc = wSceneObjXState
// - hl = wSceneObjYCoord
PORT(UpdateOWAnimation)
{
	wr8(gb, wOWAnimationFinished, 0);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a, pushed with the bank below
	push_bank_switch_to_b(gb);

	uint16_t hl = REG_HL(gb);
	uint8_t duration = rd8(gb, hl);
	wr8(gb, hl++, (uint8_t)(duration - 1));
	if (duration == 0) {
		// next frame: frameset entries are (frame, duration) pairs
		uint8_t offset = rd8(gb, hl);
		uint16_t de = REG_DE(gb);
		uint16_t entry = (uint16_t)(de + offset);
		uint8_t frame = rd8(gb, entry);
		if (frame == 0xff) {
			// frameset end: restart from its beginning
			wr8(gb, wOWAnimationFinished, frame);
			entry = de;
			frame = rd8(gb, de);
			offset = 0;
		}
		gb->cpu.d = frame;
		wr8(gb, hl--, (uint8_t)(offset + 2)); // frameset offset
		entry++;
		wr8(gb, hl--, (uint8_t)(rd8(gb, entry) - 1)); // duration
		hl = (uint16_t)((hl & 0xff00) | ((hl - 1) & 0xff)); // dec l
		wr8(gb, hl, frame);
	}

	// point to the start of the scene object, and its state
	uint8_t l = hl & 0xf8;
	gb->cpu.h = (uint8_t)(hl >> 8);
	gb->cpu.l = l;
	gb->cpu.b = gb->cpu.h;
	gb->cpu.c = (uint8_t)(l + 6);
	pop_bank(gb);
}

// copies c bytes from b:hl to de
PORT(CopyFarBytes)
{
	push_bank_switch_to_b(gb);
	gb->cpu.b = gb->cpu.c;
	CALL(CopyHLToDE);
	pop_bank(gb);
}

// copies c * $100 bytes from bank b
// at address hl to de
PORT(CopyFarBytes_Long)
{
	push_bank_switch_to_b(gb);
	gb->cpu.b = gb->cpu.c;
	gb->cpu.c = 0;
	CALL(CopyHLToDE_BC);
	pop_bank(gb);
}

PORT(LoadFarTiles)
{
	push_bank_switch_to_b(gb);
	SET_BC(gb, 0x80 * TILE_SIZE);
	SET_DE(gb, v0Tiles2);
	CALL(CopyHLToDE_BC);
	pop_bank(gb);
}

// sets scene obj's state and
// resets its animation data
// a = state
PORT(SetSceneObjState)
{
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl--, 0);
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

PORT(LoadFarPalsToTempPals1)
{
	push_bank_switch_to_b(gb);
	CALL(LoadPalsToTempPals1);
	pop_bank(gb);
}

PORT(LoadFarPalsToTempPals2)
{
	push_bank_switch_to_b(gb);
	CALL(LoadPalsToTempPals2);
	pop_bank(gb);
}

// hl = OAM data
// b = sprite bank
// de = pointer to sprite
PORT(LoadOWSprite)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.c = 0x10;
	wr8(gb, wCurSpriteYCoord, alu_add(gb, rd8(gb, hl++), gb->cpu.c));
	gb->cpu.c = 0x08;
	wr8(gb, wCurSpriteXCoord, alu_add(gb, rd8(gb, hl++), gb->cpu.c));
	wr8(gb, wCurSpriteFrame, rd8(gb, hl++));
	wr8(gb, wCurSpriteAttributes, rd8(gb, hl));

	SET_HL(gb, REG_DE(gb));
	push_bank_switch_to_b(gb);
	CALL(LoadSprite);
	pop_bank(gb);
}

// returns TRUE in a and carry set if treasure
// in input a has already been collected
// if not collected returns z set
// a = treasure constant
PORT(IsTreasureCollected)
{
	uint8_t treasure = gb->cpu.a;
	uint8_t e = alu_srl(gb, alu_srl(gb, alu_srl(gb, treasure))); // / 8
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, wTreasuresCollected, e);

	// the treasure's bit within its byte
	e = 1;
	if (alu_bit(gb, treasure, 2))
		e = alu_swap(gb, e); // = $10
	uint8_t d = alu_and(gb, treasure, 0x03);
	uint8_t a = e;
	if (d) {
		a = alu_rla(gb, a);
		d = alu_dec(gb, d);
		if (d) {
			a = alu_rla(gb, a);
			d = alu_dec(gb, d);
			if (d)
				a = alu_rla(gb, a);
		}
	}
	gb->cpu.e = e;
	gb->cpu.d = a;
	SET_HL(gb, hl);
	gb->cpu.a = alu_and(gb, a, rd8(gb, hl));
	if (!gb->cpu.a)
		return; // not obtained
	gb->cpu.a = TRUE;
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | FLAG_C); // scf
}

// hl += a * 2
PORT(AddOffsetInPointerTable)
{
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.a = alu_adc(gb, 0, 0);
	gb->cpu.d = gb->cpu.a;
	SET_HL(gb, alu_add16(gb, REG_HL(gb), REG_DE(gb)));
}

// b = OAM bank, hl = OAM
PORT(LoadOWSpriteWithScroll_GotParams)
{
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = gb->cpu.b;
	bankswitch(gb, gb->cpu.a);
	CALL(LoadSprite);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}

// ld a, [hli] / ld h, [hl] / ld l, a, with the second register pair
// written to hi:lo
static void load_pointer(GB *gb, uint8_t *hi, uint8_t *lo)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl++);
	*hi = rd8(gb, hl);
	if (hi != &gb->cpu.h) {
		gb->cpu.h = (uint8_t)(hl >> 8);
		gb->cpu.l = (uint8_t)hl;
	}
	*lo = gb->cpu.a;
}

// outputs in hl the pointer corresponding to the index given in register a
// from the pointer table given in hl
PORT(GetPointerFromTableHL)
{
	CALL(AddOffsetInPointerTable);
	load_pointer(gb, &gb->cpu.h, &gb->cpu.l);
}

// same as GetPointerFromTableHL but outputs pointer to de
PORT(GetPointerFromTableDE)
{
	CALL(AddOffsetInPointerTable);
	load_pointer(gb, &gb->cpu.d, &gb->cpu.e);
}

// outputs in hl the cth word contained in the ath table entry in the
// pointer table in hl
PORT(GetCthWordFromAthTable)
{
	CALL(AddOffsetInPointerTable);
	load_pointer(gb, &gb->cpu.h, &gb->cpu.l);
	gb->cpu.a = gb->cpu.c;
	CALL(AddOffsetInPointerTable);
}

PORT(GetByteFromPointerInHL)
{
	load_pointer(gb, &gb->cpu.h, &gb->cpu.l);
	gb->cpu.a = rd8(gb, REG_HL(gb));
}

// b = sprite bank
// de = pointer to sprite
// hl = y, x, frame and attributes (relative to the BG)
PORT_JUMP(LoadOWSpriteWithScroll)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.c = rd8(gb, wSCY);
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), gb->cpu.c);
	gb->cpu.c = 0x10;
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	wr8(gb, wCurSpriteYCoord, gb->cpu.a);
	gb->cpu.c = rd8(gb, wSCX);
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), gb->cpu.c);
	gb->cpu.c = 0x08;
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	wr8(gb, wCurSpriteXCoord, gb->cpu.a);
	wr8(gb, wCurSpriteFrame, rd8(gb, hl++));
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wCurSpriteAttributes, gb->cpu.a);
	gb->cpu.h = gb->cpu.d;
	gb->cpu.l = gb->cpu.e;
	JUMP(LoadOWSpriteWithScroll_GotParams); // fallthrough
}

// hl = y, x in pixels
// output: hl = wOWTilemapPtr = their tile in wTilemap
PORT(GetOWCoordInTilemap)
{
	uint16_t hl = REG_HL(gb);
	uint8_t y = alu_and(gb, rd8(gb, hl++), 0xf8);
	uint8_t c = (uint8_t)(y << 2 | y >> 6); // rlca / rlca: y / 8 * 32
	gb->cpu.b = alu_and(gb, c, 0x0f);
	gb->cpu.c = alu_and(gb, c, 0xf0);
	uint8_t x = alu_and(gb, rd8(gb, hl), 0xf8);
	x = (uint8_t)(x << 1 | x >> 7); // rlca
	x = alu_swap(gb, x); // x / 8
	gb->cpu.a = alu_add(gb, x, gb->cpu.c);
	gb->cpu.l = gb->cpu.a;
	wr8(gb, wOWTilemapPtr + 0, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.b, (uint8_t)(wTilemap >> 8));
	gb->cpu.h = gb->cpu.a;
	wr8(gb, wOWTilemapPtr + 1, gb->cpu.a);
}

// loads source address of tiles
// with the given level's name for HDMA transfer
// writes them to v0Tiles2 tile $50
// b = map side constant (NORTH, ...)
// d = level index within that map side
PORT(LoadLevelName)
{
	gb->cpu.a = alu_add(gb, BANK_LevelNamesJPGfx, rd8(gb, wLanguage));
	SET_HL(gb, wLanguage);
	wr8(gb, wHDMABank, gb->cpu.a);
	wr8(gb, wHDMA, 0);
	gb->cpu.c = 0x00;
	gb->cpu.b = alu_swap(gb, gb->cpu.b); // *$10
	uint16_t hl = alu_add16(gb, ADDR_LevelNamesJPGfx, REG_BC(gb)); // aka LevelNamesENGfx
	gb->cpu.d = alu_add(gb, gb->cpu.d, gb->cpu.d); // *2
	gb->cpu.e = 0x00;
	hl = alu_add16(gb, hl, REG_DE(gb));
	SET_HL(gb, hl);
	wr8(gb, w2d0b5SourceHi, gb->cpu.h);
	wr8(gb, w2d0b5SourceLo, gb->cpu.l);
	wr8(gb, w2d0b5DestHi, 0x15);
	wr8(gb, w2d0b5DestLo, 0);
	gb->cpu.f = FLAG_Z; // xor a
	gb->cpu.a = 32 - 1; // 32 dma_tiles
	wr8(gb, w2d0b5Mode, gb->cpu.a);
}

// play_music2: sets hMusicIDBackup and hMusicID
static void play_music2(GB *gb, uint16_t music)
{
	gb->cpu.a = (uint8_t)(music >> 8);
	wr8(gb, hMusicIDBackup + 0, gb->cpu.a);
	wr8(gb, hMusicID + 0, gb->cpu.a);
	gb->cpu.a = (uint8_t)music;
	wr8(gb, hMusicIDBackup + 1, gb->cpu.a);
	wr8(gb, hMusicID + 1, gb->cpu.a);
}

PORT(PlayOverworldMusic)
{
	gb->cpu.a = rd8(gb, w2d011);
	bool night = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	play_music2(gb, night ? MUSIC_OVERWORLD_NIGHT : MUSIC_OVERWORLD_DAY);
}
