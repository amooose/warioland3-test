// Port of src/engine/level/load_objects.asm
#include "ported.h"

// input:
// - de = block
// - c = entry in wObjDataPointers
// Flags in between are overwritten before they matter, so only the ones
// at the exits are computed.
PORT(SpawnObject)
{
	gb->cpu.a = rd8(gb, hXPosHi);
	alu_cp(gb, gb->cpu.a, 0x0a);
	if (!(gb->cpu.f & FLAG_C))
		return;

	// find a free object slot
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	uint16_t hl = 0;
	bool found = false;
	for (int n = 0; n < NUM_OBJECTS && !found; n++) {
		hl = (uint16_t)(wObj1 + n * OBJ_STRUCT_LENGTH);
		gb->cpu.a = alu_rra(gb, rd8(gb, hl++));
		found = !(gb->cpu.f & FLAG_C);
	}
	SET_HL(gb, hl);
	if (!found)
		return;

	// bit 7 set means it's not spawned in
	uint16_t de = REG_DE(gb);
	uint8_t v = rd8(gb, de);
	gb->cpu.f = (v & 0x80) ? FLAG_C : 0; // rlca
	if (!(v & 0x80)) {
		gb->cpu.a = (uint8_t)(v << 1 | v >> 7);
		return;
	}
	// not spawned, unset bit 7
	gb->cpu.a = v & 0x7f;
	wr8(gb, de, gb->cpu.a);

	wr8(gb, hl, gb->cpu.e); // OBJ_UNK_01
	hl = (uint16_t)((hl & 0xff00) | (uint8_t)(hl + 1));
	gb->cpu.a = (uint8_t)(((rd8(gb, wccec) - 1) << 5) + gb->cpu.d);
	wr8(gb, hl++, gb->cpu.a); // OBJ_UNK_02
	wr8(gb, hl++, rd8(gb, hYPosLo)); // OBJ_Y_POS
	wr8(gb, hl++, rd8(gb, hYPosHi));
	wr8(gb, hl++, rd8(gb, hXPosLo)); // OBJ_X_POS
	gb->cpu.a = rd8(gb, hXPosHi);
	wr8(gb, hl++, gb->cpu.a);

	gb_push16(gb, hl); // push hl
	gb->cpu.c = (uint8_t)((gb->cpu.c - 1) << 1); // dec c / sla c
	gb->cpu.b = 0x00;
	uint16_t entry = (uint16_t)(wObjDataPointers + REG_BC(gb));
	uint16_t data = rd16(gb, entry);
	de = gb_pop16(gb); // pop de

	uint8_t d = (uint8_t)(de >> 8), e = (uint8_t)de;
#define DE ((uint16_t)(d << 8 | e))
	wr8(gb, DE, rd8(gb, data++)); // OBJ_ID
	e++;
	wr8(gb, DE, rd8(gb, data++)); // OBJ_INTERACTION_TYPE
	e++;
	wr8(gb, DE, rd8(gb, data++)); // OBJ_COLLBOX_TOP
	e++;
	wr8(gb, DE, 0); // OBJ_COLLBOX_BOTTOM

	e += OBJ_FRAME - OBJ_COLLBOX_BOTTOM;
	wr8(gb, DE, 0); // OBJ_FRAME
	e++;
	wr8(gb, DE, rd8(gb, data++)); // OBJ_OAM_PTR
	e++;
	wr8(gb, DE, rd8(gb, data++));

	e += OBJ_FRAME_DURATION - (OBJ_OAM_PTR + 1);
	wr8(gb, DE, 0x7f); // OBJ_FRAME_DURATION

	e += OBJ_SUBSTATE - OBJ_FRAME_DURATION;
	wr8(gb, DE, 0x1 | OBJSUBFLAG_UNINITIALISED); // OBJ_SUBSTATE
	e++;
	wr8(gb, DE, 0); // OBJ_STATE (OBJSTATE_00)
	e++;
	wr8(gb, DE, 2 | (1 << 7)); // OBJ_ACTION
	e += 2;
	wr8(gb, DE, rd8(gb, data++)); // OBJ_UPDATE_FUNCTION
	e++;
	wr8(gb, DE, rd8(gb, data++));

	e -= (OBJ_UPDATE_FUNCTION + 1) - OBJ_FLAGS;
	gb->cpu.a = alu_or(gb, rd8(gb, data), OBJFLAG_ACTIVE | OBJFLAG_INVISIBLE);
	wr8(gb, DE, gb->cpu.a); // OBJ_FLAGS
#undef DE
	gb->cpu.d = d;
	gb->cpu.e = e;
	SET_HL(gb, data);
}

// --- creating objects ---

// hl = the object slot's flags + 1 (a = written there first),
// bc = object creation data
static void create_object_got_pos(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl++, gb->cpu.a);
	gb->cpu.e = (uint8_t)hl;
	gb->cpu.d = (uint8_t)(hl >> 8);
	uint16_t bc = REG_BC(gb);
	hl = bc;
	// [de] = [hli] / inc e
#define COPY() do { gb->cpu.a = rd8(gb, hl++); wr8(gb, REG_DE(gb), gb->cpu.a); } while (0)
#define INC_E() (gb->cpu.e = alu_inc(gb, gb->cpu.e))
	for (int i = 0; i < 6; i++) { // OBJ_ID .. OBJ_COLLBOX_RIGHT
		COPY();
		INC_E();
	}
	INC_E();
	INC_E();
	INC_E();
	for (int i = 0; i < 4; i++) { // OBJ_OAM_PTR, OBJ_FRAMESET_PTR
		COPY();
		INC_E();
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, REG_DE(gb), 0); // OBJ_FRAME_DURATION
	INC_E();
	wr8(gb, REG_DE(gb), gb->cpu.a); // OBJ_FRAMESET_OFFSET
	INC_E();
	for (int i = 0; i < 6; i++) { // OBJ_STATE_DURATION .. OBJ_STATE
		COPY();
		INC_E();
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a (OBJACTION_NONE)
	wr8(gb, REG_DE(gb), 0); // OBJ_ACTION
	INC_E();
	INC_E();
	COPY(); // OBJ_UPDATE_FUNCTION
	INC_E();
	COPY();
#undef COPY
#undef INC_E
	gb->cpu.a = alu_sub(gb, gb->cpu.e, (OBJ_UPDATE_FUNCTION + 1) - OBJ_FLAGS);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.a = alu_or(gb, rd8(gb, hl), OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT); // OBJ_FLAGS
	wr8(gb, REG_DE(gb), gb->cpu.a);
	SET_HL(gb, hl);
}

PORT(CreateObject_GotPos) { create_object_got_pos(gb); }

// Finds a free object slot: hl = its flags, or false if there's none
static bool free_obj_slot(GB *gb)
{
	gb->cpu.h = (uint8_t)(wObj1Flags >> 8);
	for (int n = 0; n < NUM_OBJECTS; n++) {
		gb->cpu.l = (uint8_t)(wObj1Flags + n * OBJ_STRUCT_LENGTH);
		gb->cpu.a = alu_rra(gb, rd8(gb, REG_HL(gb)));
		if (!(gb->cpu.f & FLAG_C))
			return true; // OBJFLAG_ACTIVE not set
	}
	return false;
}

// ld a, v / ld [hli], a
static void put_hli(GB *gb, uint8_t v)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = v;
	wr8(gb, hl++, v);
	SET_HL(gb, hl);
}

// bc = object creation data
PORT(_CreateObjectFromCurObjPos)
{
	if (!free_obj_slot(gb))
		return;
	put_hli(gb, OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT);
	put_hli(gb, rd8(gb, wCurObjUnk01));
	put_hli(gb, rd8(gb, wCurObjUnk02));
	put_hli(gb, rd8(gb, wCurObjYPos + 0));
	put_hli(gb, rd8(gb, wCurObjYPos + 1));
	put_hli(gb, rd8(gb, wCurObjXPos + 0));
	gb->cpu.a = rd8(gb, wCurObjXPos + 1);
	create_object_got_pos(gb);
}

// [hli] = [pos] + the signed offset at [bc], then a = [pos + 1] adjusted
static void add_offset(GB *gb, uint16_t pos)
{
	gb->cpu.e = rd8(gb, pos);
	gb->cpu.a = alu_rla(gb, rd8(gb, REG_BC(gb)));
	bool negative = gb->cpu.f & FLAG_C;
	gb->cpu.a = alu_rra(gb, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.e);
	put_hli(gb, gb->cpu.a);
	bool carry = gb->cpu.f & FLAG_C;
	gb->cpu.a = rd8(gb, pos + 1);
	if (negative && !carry)
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
	else if (!negative && carry)
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
}

// bc = object creation data, after the y and x offsets
PORT(_CreateObjectAtRelativePos)
{
	if (!free_obj_slot(gb))
		return;
	put_hli(gb, OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT);
	put_hli(gb, rd8(gb, wCurObjUnk01));
	put_hli(gb, rd8(gb, wCurObjUnk02));
	add_offset(gb, wCurObjYPos);
	put_hli(gb, gb->cpu.a);
	SET_BC(gb, REG_BC(gb) + 1);
	add_offset(gb, wCurObjXPos);
	SET_BC(gb, REG_BC(gb) + 1);
	create_object_got_pos(gb);
}

PORT(DespawnAllObjects)
{
	gb->cpu.h = (uint8_t)(wObjects >> 8);
	for (int n = 0; n < NUM_OBJECTS; n++) {
		gb->cpu.l = (uint8_t)(wObjects + OBJ_STRUCT_LENGTH * n);
		gb->cpu.a = alu_rra(gb, rd8(gb, REG_HL(gb))); // OBJ_FLAGS
		if (!(gb->cpu.f & FLAG_C))
			continue; // skip if OBJFLAG_ACTIVE not set
		gb->cpu.a = alu_rla(gb, gb->cpu.a);
		gb->cpu.a = alu_rla(gb, gb->cpu.a);
		gb->cpu.a = alu_rla(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_C) {
			// transient
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, REG_HL(gb), 0);
			continue;
		}
		gb->cpu.l = alu_inc(gb, gb->cpu.l);
		gb->cpu.l = alu_inc(gb, gb->cpu.l);
		FARCALL(DespawnObject);
	}
}

// Loads the object data pointers of the Hidden Figure's room (.pointers,
// up to a NULL) into wObjDataPtr01 onwards.
PORT(LoadHiddenFigureData)
{
	uint16_t hl = ADDR_LoadHiddenFigureData + 0x1e; // .pointers
	gb->cpu.a = 0x0d;
	wr8(gb, hffa0, 0x0d);
	SET_DE(gb, wObjDataPtr01);
	for (;;) {
		gb->cpu.a = rd8(gb, hl++);
		gb->cpu.c = gb->cpu.a;
		gb->cpu.a = rd8(gb, hl++);
		gb->cpu.b = gb->cpu.a;
		SET_HL(gb, hl);
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z)
			return;
		gb->cpu.a = gb->cpu.c;
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = gb->cpu.b;
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = alu_dec(gb, rd8(gb, hffa0));
		wr8(gb, hffa0, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z)
			return;
	}
}

// Loads the current object group: the object data pointers (the dummy
// object in every unused slot), the chest/key/musical coin pointers and
// palette, and the 4 enemy graphics slots, then the object palettes.
PORT_JUMP(LoadObjectGroupData)
{
	uint16_t hl = wObjDataPointers;
	SET_BC(gb, ADDR_DummyObjectData);
	gb->cpu.e = 0x10;
	do {
		gb->cpu.a = gb->cpu.c;
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.a = gb->cpu.b;
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.e = alu_dec(gb, gb->cpu.e);
	} while (gb->cpu.e);
	SET_HL(gb, hl);

	gb->cpu.a = rd8(gb, wObjectGroup);
	alu_cp(gb, gb->cpu.a, OBJECT_GROUP_HIDDEN_FIGURE);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_LoadHiddenFigureData); // same bank
		return;
	}

	gb->cpu.d = 0x00;
	gb->cpu.e = gb->cpu.a;
	for (int i = 0; i < 2; i++) {
		gb->cpu.e = alu_sla(gb, gb->cpu.e);
		gb->cpu.d = alu_rl(gb, gb->cpu.d);
	}
	hl = alu_add16(gb, ADDR_ObjectGroups, REG_DE(gb));
	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, hl++);
	gb_push16(gb, hl);
	hl = (uint16_t)(gb->cpu.a << 8 | gb->cpu.c);
	SET_DE(gb, wChestObjDataPtr);
	for (int i = 0; i < 6; i++) {
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		if (i < 5)
			gb->cpu.e = alu_inc(gb, gb->cpu.e);
	}
	SET_HL(gb, hl);
	SET_DE(gb, wTempPals2 + 3 * PAL_SIZE);
	gb->cpu.b = PAL_SIZE;
	CALL(CopyHLToDE_Short);
	hl = gb_pop16(gb);

	gb->cpu.a = rd8(gb, hl++);
	gb->cpu.c = gb->cpu.a;
	hl = (uint16_t)(rd8(gb, hl) << 8 | gb->cpu.c);
	gb->cpu.a = 0x01; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hffa0, gb->cpu.a);

	static const uint16_t dest[4] = {
		v1Tiles0, v1Tiles0 + 0x40 * TILE_SIZE, v1Tiles1, v1Tiles1 + 0x40 * TILE_SIZE,
	};
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, hl++);
		gb->cpu.c = gb->cpu.a;
		gb->cpu.a = rd8(gb, hl++);
		gb_push16(gb, hl);
		SET_HL(gb, gb->cpu.a << 8 | gb->cpu.c);
		SET_BC(gb, dest[i]);
		gb->cpu.a = rd8(gb, hffa0);
		gb->cpu.e = (uint8_t)(0x68 + i); // BANK("Enemy Gfx 1 Slot n")
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.e);
		wr8(gb, hCallFuncBank, gb->cpu.a);
		HCALL(Decompress);
		hl = gb_pop16(gb);
	}

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0); // VRAM0
	gb->cpu.a = 13;
	wr8(gb, hffa0, 13);
	SET_DE(gb, wObjDataPtr01);
	for (;;) {
		gb->cpu.a = rd8(gb, hl++);
		gb->cpu.c = gb->cpu.a;
		gb->cpu.a = rd8(gb, hl++);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z)
			break;
		gb->cpu.a = gb->cpu.c;
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = gb->cpu.b;
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = alu_dec(gb, rd8(gb, hffa0));
		wr8(gb, hffa0, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z)
			break;
	}
	SET_HL(gb, hl);
	JUMP(LoadObjPals);
}
