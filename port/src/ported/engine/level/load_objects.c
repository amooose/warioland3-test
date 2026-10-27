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
