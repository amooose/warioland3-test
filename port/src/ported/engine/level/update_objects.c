// Port of src/engine/level/update_objects.asm
#include "ported.h"

#define OBJ_STRUCT_SIZE (wObj2Flags - wObj1Flags)

// Draws every active, on-screen, visible object whose flags (masked as
// below) match.
static void draw_objects(GB *gb, uint8_t match)
{
	for (int n = 0; n < NUM_OBJECTS; n++) {
		uint16_t obj = (uint16_t)(wObj1Flags + n * OBJ_STRUCT_SIZE);
		SET_HL(gb, obj);
		gb->cpu.a = alu_and(gb, rd8(gb, obj),
			OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN | OBJFLAG_INVISIBLE | OBJFLAG_PRIORITY);
		alu_cp(gb, gb->cpu.a, match);
		if (!(gb->cpu.f & FLAG_Z))
			continue;
		gb->cpu.e = (uint8_t)(wObj1ScreenYPos + n * OBJ_STRUCT_SIZE);
		gb->cpu.l = (uint8_t)(wObj1Id + n * OBJ_STRUCT_SIZE);
		CALL(UpdateObjSprite);
	}
}

PORT(DrawObjects_NoPriority)
{
	draw_objects(gb, OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN | OBJFLAG_PRIORITY);
}

PORT(DrawObjects_WithPriority)
{
	draw_objects(gb, OBJFLAG_ACTIVE | OBJFLAG_ON_SCREEN);
}


#define CPU (&gb->cpu)

// One of CheckObjectOnScreen's range checks of the position hi:lo against
// the camera position in hl: lo += add, then if the position is past hl,
// hl += span and lo -= sub, and the position must be below hl. A carry out
// of hi on the first step counts as in range if wrap_in, otherwise it goes
// on to the second step. Returns whether the position is in range.
static bool in_range(GB *gb, uint8_t *hi, uint8_t *lo, uint8_t add, uint8_t span, uint8_t sub,
	bool wrap_in)
{
	CPU->a = *lo = alu_add(gb, *lo, add);
	if (CPU->f & FLAG_C) {
		*hi = alu_inc(gb, *hi);
		if (CPU->f & FLAG_Z) {
			if (wrap_in)
				return true;
			goto upper;
		}
	}
	CPU->a = *hi;
	alu_cp(gb, *hi, CPU->h);
	if (CPU->f & FLAG_C)
		return false;
	if (CPU->f & FLAG_Z) {
		CPU->a = *lo;
		alu_cp(gb, *lo, CPU->l);
		if (CPU->f & FLAG_C)
			return false;
		if (CPU->f & FLAG_Z)
			return true;
	}
upper:
	CPU->a = CPU->l = alu_add(gb, CPU->l, span);
	if (CPU->f & FLAG_C)
		CPU->h = alu_inc(gb, CPU->h);
	CPU->a = *lo = alu_sub(gb, *lo, sub);
	if (CPU->f & FLAG_C) {
		*hi = alu_dec(gb, *hi);
		CPU->a = alu_inc(gb, *hi);
		if (CPU->f & FLAG_Z)
			return true;
	}
	CPU->a = *hi;
	alu_cp(gb, *hi, CPU->h);
	if (CPU->f & FLAG_C)
		return true;
	if (!(CPU->f & FLAG_Z))
		return false;
	CPU->a = *lo;
	alu_cp(gb, *lo, CPU->l);
	return CPU->f & FLAG_C;
}

// ld hl, addr / ld a, [hli] / ld l, [hl] / ld h, a (a big-endian pointer)
static void load_hl_be(GB *gb, uint16_t addr)
{
	CPU->a = rd8(gb, addr);
	CPU->l = rd8(gb, addr + 1);
	CPU->h = CPU->a;
}

// seems to be a function to load/unload objects
// that are inside and outside the screen
PORT(CheckObjectOnScreen)
{
	CPU->c = rd8(gb, wCurObjYPos + 0); // y lo
	CPU->b = rd8(gb, wCurObjYPos + 1); // y hi
	CPU->a = CPU->e = rd8(gb, wCurObjYPos + 2); // x lo
	CPU->d = rd8(gb, wCurObjYPos + 3); // x hi

	load_hl_be(gb, wCameraSCY);
	gb_push16(gb, REG_BC(gb));
	gb_push16(gb, REG_DE(gb));
	gb_push16(gb, REG_HL(gb));
	bool on_screen = false;
	if (in_range(gb, &CPU->b, &CPU->c, 24, SCREEN_HEIGHT_PX, 56, false)) {
		// .skip_y2
		load_hl_be(gb, wCameraSCX);
		on_screen = in_range(gb, &CPU->d, &CPU->e, 24, SCREEN_WIDTH_PX, 0x30, true);
	} else {
		// .asm_630fd
		CPU->a = alu_and(gb, rd8(gb, wCurObjState), 0xfe);
		alu_cp(gb, CPU->a, OBJSTATE_48);
		if (CPU->f & FLAG_Z)
			goto pop;
	}
	SET_HL(gb, wCurObjFlags);
	if (on_screen)
		wr8(gb, wCurObjFlags, rd8(gb, wCurObjFlags) | OBJFLAG_ON_SCREEN);
	else
		wr8(gb, wCurObjFlags, rd8(gb, wCurObjFlags) & ~OBJFLAG_ON_SCREEN);
pop:
	SET_HL(gb, gb_pop16(gb));
	SET_DE(gb, gb_pop16(gb));
	SET_BC(gb, gb_pop16(gb));

	// whether the object is far enough outside the screen to unload
	bool keep = false;
	CPU->a = alu_rla(gb, CPU->d);
	if (!(CPU->f & FLAG_C)) {
		CPU->a = alu_rla(gb, CPU->b);
		if (!(CPU->f & FLAG_C) && in_range(gb, &CPU->b, &CPU->c, 92, 0x90, 0xcb, true)) {
			// .asm_63147
			load_hl_be(gb, wCameraSCX);
			keep = in_range(gb, &CPU->d, &CPU->e, 0x6f, SCREEN_WIDTH_PX, 0xde, true);
		}
	}
	if (keep)
		return;

	// .asm_6317e
	CPU->a = alu_and(gb, rd8(gb, wCurObjSubState), 0xf | OBJSUBFLAG_UNK_4);
	alu_cp(gb, CPU->a, 0x0 | OBJSUBFLAG_UNK_4);
	if (!(CPU->f & FLAG_C))
		return;
	// OBJSUBFLAG_UNK_4 is not set
	SET_HL(gb, wCurObjUnk02);
	alu_and(gb, CPU->a, CPU->a);
	if (!(CPU->f & FLAG_Z)) {
		FARCALL(DespawnObject);
		return;
	}
	CPU->a = 0;
	CPU->f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}
