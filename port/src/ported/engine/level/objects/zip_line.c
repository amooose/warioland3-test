// Port of src/engine/level/objects/zip_line.asm
#include "ported.h"

// Asm-mirroring helpers: each is one instruction on the real registers.
#define A (gb->cpu.a)
#define ZF (gb->cpu.f & FLAG_Z)
#define CF (gb->cpu.f & FLAG_C)
#define RETURN do { RET(); return; } while (0)
static inline void ld_hl(GB *gb, uint16_t v) { SET_HL(gb, v); }
static inline void ld_l(GB *gb, uint8_t v) { gb->cpu.l = v; }
static inline uint8_t rd_hl(GB *gb) { return rd8(gb, REG_HL(gb)); }
static inline void wr_hl(GB *gb, uint8_t v) { wr8(gb, REG_HL(gb), v); }
static inline uint8_t rd_hli(GB *gb) { uint16_t hl = REG_HL(gb); SET_HL(gb, hl + 1); return rd8(gb, hl); }
static inline uint8_t rd_hld(GB *gb) { uint16_t hl = REG_HL(gb); SET_HL(gb, hl - 1); return rd8(gb, hl); }
static inline void wr_hli(GB *gb, uint8_t v) { uint16_t hl = REG_HL(gb); wr8(gb, hl, v); SET_HL(gb, hl + 1); }
static inline void wr_hld(GB *gb, uint8_t v) { uint16_t hl = REG_HL(gb); wr8(gb, hl, v); SET_HL(gb, hl - 1); }
static inline void xor_a(GB *gb) { A = 0; gb->cpu.f = FLAG_Z; }
static inline void inc_l(GB *gb) { gb->cpu.l = alu_inc(gb, gb->cpu.l); }
static inline void dec_l(GB *gb) { gb->cpu.l = alu_dec(gb, gb->cpu.l); }
static inline uint8_t inc_mhl(GB *gb) { uint8_t v = alu_inc(gb, rd_hl(gb)); wr_hl(gb, v); return v; }
static inline uint8_t dec_mhl(GB *gb) { uint8_t v = alu_dec(gb, rd_hl(gb)); wr_hl(gb, v); return v; }
static inline void set_mhl(GB *gb, int bit) { wr_hl(gb, (uint8_t)(rd_hl(gb) | (1 << bit))); }
static inline void res_mhl(GB *gb, int bit) { wr_hl(gb, (uint8_t)(rd_hl(gb) & ~(1 << bit))); }
// ld a, [hl] / and HEAVY_OBJ / or x
static inline void heavy_or(GB *gb, uint8_t x) { A = alu_or(gb, alu_and(gb, rd_hl(gb), HEAVY_OBJ), x); }
// ld a, [hld] / rlca / ld_obj_frameset left, right
static inline void frameset_by_dir_hld(GB *gb, uint16_t nc, uint16_t c)
{
	A = alu_rlca(gb, rd_hld(gb));
	SET_DE(gb, CF ? c : nc);
}
// ld hl, wCurObjYPos / copy the position to hYPos / hXPos
static inline void obj_pos_to_hpos(GB *gb)
{
	ld_hl(gb, wCurObjYPos);
	A = rd_hli(gb); wr8(gb, hYPosLo, A);
	A = rd_hli(gb); wr8(gb, hYPosHi, A);
	A = rd_hli(gb); wr8(gb, hXPosLo, A);
	A = rd_hl(gb); wr8(gb, hXPosHi, A);
}

// rst JumpTable, with the table at `table`.
static inline uint16_t jumptable(GB *gb, uint16_t table)
{
	gb_push16(gb, table);
	uint16_t hl = gb_pop16(gb);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	return REG_HL(gb);
}


#define ZL(off) (ADDR_ZipLineFunc + (off))
#define ZL_49369 ZL(0x43)
#define ZL_493DC ZL(0xb6)
#define ZL_49405 ZL(0xdf)
#define ZL_49412 ZL(0xec)

static void set_update(GB *gb, uint16_t f)
{
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(f >> 8); wr_hld(gb, A);
	A = (uint8_t)f; wr_hld(gb, A);
}

static void set_update_l(GB *gb, uint16_t f)
{
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(f >> 8); wr_hld(gb, A);
	A = (uint8_t)f; wr_hld(gb, A);
}

static void no_actions(GB *gb)
{
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
}

// .Func_49356
static void zl_49356(GB *gb)
{
	SET_DE(gb, ADDR_Frameset_691c4);
	CALL(SetObjectFramesetPtr);
	set_update_l(gb, ZL_49369);
	xor_a(gb);
	wr8(gb, wCurObjState, A);
}

PORT(ZipLineFunc)
{
	ld_hl(gb, wCurObjSubState);
	A = rd8(gb, wCurObjId);
	alu_cp(gb, A, ZIP_LINE4);
	if (CF) {
		A = 0x01;
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
	} else {
		bool nz = !ZF;
		A = 0x03;
		if (nz)
			set_mhl(gb, OBJSUBFLAG_HDIR_F);
		else
			res_mhl(gb, OBJSUBFLAG_HDIR_F);
	}
	wr8(gb, wCurObjVar2, A);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	ld_l(gb, OBJ_FLAGS);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	set_mhl(gb, OBJFLAG_NO_COLLISION_F);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x03; wr_hld(gb, A);
	A = 0xfc; wr_hld(gb, A);
	A = 0xfe; wr_hl(gb, A);
	xor_a(gb);
	wr8(gb, wCurObjVar1, A);
	zl_49356(gb);
}

PORT_LOCAL(ZipLineFunc, Func_49356) { zl_49356(gb); }

// .asm_49518: the sliding sound
static void zl_49518(GB *gb)
{
	A = rd8(gb, wCurObjId);
	alu_cp(gb, A, ZIP_LINE4);
	if (!CF)
		return;
	A = alu_and(gb, rd8(gb, wGlobalCounter), 0x1f);
	if (!ZF)
		return;
	play_sfx(gb, SFX_08C);
}

// .Func_494af / .Func_494cb: moves the zip line and Wario sideways
static void zl_move_x(GB *gb, bool right)
{
	if (right)
		CALL(MoveObjectRightByVar2);
	else
		CALL(MoveObjectLeftByVar2);
	A = alu_dec(gb, rd8(gb, wCurObjVar1));
	if (!ZF)
		return;
	if (right) {
		gb_call_ret(gb, ADDR_MoveWarioRight, ZL(0x1b0));
		FARCALL(Func_19706);
	} else {
		gb_call_ret(gb, ADDR_MoveWarioLeft, ZL(0x194));
		FARCALL(Func_19713);
	}
	zl_49518(gb);
}

// .asm_494e7
static void zl_up(GB *gb)
{
	CALL(MoveObjectUpByVar2);
	A = alu_dec(gb, rd8(gb, wCurObjVar1));
	if (!ZF)
		return;
	gb_call_ret(gb, ADDR_MoveWarioUp, ZL(0x1cc));
	// jp HandleUpwardsFloorTransition (a tail call of this subroutine)
	uint16_t ret = gb_pop16(gb);
	gb_call_ret(gb, ADDR_HandleUpwardsFloorTransition, ret);
	gb_push16(gb, ret); // the caller pops it
}

// .asm_494f5
static void zl_down(GB *gb)
{
	CALL(MoveObjectDownByVar2);
	A = alu_dec(gb, rd8(gb, wCurObjVar1));
	if (!ZF)
		return;
	gb_call_ret(gb, ADDR_MoveWarioDown, ZL(0x1da));
	gb_call_ret(gb, ADDR_HandleDownwardsFloorTransition, ZL(0x1dd));
	A = rd8(gb, wCurObjId);
	alu_cp(gb, A, ZIP_LINE4);
	if (CF)
		return;
	A = alu_and(gb, rd8(gb, wGlobalCounter), 0x07);
	if (!ZF)
		return;
	play_sfx(gb, SFX_ZIP_LINE_SLIDE);
}

// .Func_49418: moves along the rope while Wario hangs from it; called with
// its return address pushed
static void zl_49418(GB *gb)
{
	A = rd8(gb, wCurObjVar1);
	if (!alu_and(gb, A, A))
		return;
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	bool right = CF;
	ld_hl(gb, wCurObjYPos);
	A = alu_sub(gb, rd_hli(gb), 0x10);
	wr8(gb, hYPosLo, A);
	A = alu_sbc(gb, rd_hli(gb), 0x00);
	wr8(gb, hYPosHi, A);
	if (right) {
		A = alu_add(gb, rd_hli(gb), 0x07);
		wr8(gb, hXPosLo, A);
		A = alu_adc(gb, rd_hli(gb), 0x00);
	} else {
		A = alu_sub(gb, rd_hli(gb), 0x08);
		wr8(gb, hXPosLo, A);
		A = alu_sbc(gb, rd_hli(gb), 0x00);
	}
	wr8(gb, hXPosHi, A);
	gb_call_ret(gb, ADDR_Func_3513, ZL(right ? 0x117 : 0x144));
	if (alu_and(gb, A, A)) {
		// .asm_4947b: the end of the rope
		A = rd8(gb, wCurObjId);
		alu_cp(gb, A, ZIP_LINE4);
		if (CF) {
			ld_hl(gb, wCurObjSubState);
			if (!alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F)) {
				set_mhl(gb, OBJSUBFLAG_HDIR_F);
				SET_DE(gb, ADDR_Frameset_691d2);
			} else {
				res_mhl(gb, OBJSUBFLAG_HDIR_F);
				SET_DE(gb, ADDR_Frameset_691c9);
			}
			CALL(SetObjectFramesetPtr);
			return;
		}
		ld_hl(gb, wCurObjVar1);
		A = alu_dec(gb, rd_hl(gb));
		if (ZF) {
			wr8(gb, wAutoMoveState, A);
			play_sfx(gb, SFX_129);
		}
		A = 0x03;
		wr_hl(gb, A);
		return;
	}
	gb_push16(gb, ZL(right ? 0x11d : 0x14a));
	zl_move_x(gb, right);
	gb_pop16(gb);
	A = rd8(gb, wCurObjId);
	alu_cp(gb, A, ZIP_LINE1);
	if (ZF)
		return;
	A = alu_rra(gb, A);
	bool down = right ? !CF : CF;
	if (down)
		zl_down(gb);
	else
		zl_up(gb);
}

PORT_LOCAL_JUMP(ZipLineFunc, Func_49418)
{
	zl_49418(gb);
	RET();
}

// .asm_493cd
static void zl_493cd(GB *gb)
{
	SET_DE(gb, ADDR_Frameset_691db);
	CALL(SetObjectFramesetPtr);
	set_update_l(gb, ZL_49412);
	RETURN;
}

PORT_LOCAL_JUMP(ZipLineFunc, Func_49369)
{
	no_actions(gb);
	gb_push16(gb, ZL(0x4b)); // call .Func_49418
	zl_49418(gb);
	gb_pop16(gb);
	A = rd8(gb, wCurObjVar1);
	alu_cp(gb, A, 0x03);
	if (ZF) {
		zl_493cd(gb);
		return;
	}
	A = rd8(gb, wCurObjState);
	if (!alu_and(gb, A, A)) {
		A = alu_add(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.c);
		gb->cpu.b = A;
		A = alu_add(gb, rd8(gb, wWarioScreenYPos), gb->cpu.c);
		A = alu_sub(gb, A, gb->cpu.b);
		alu_cp(gb, A, 0x10);
		if (CF)
			RETURN;
		alu_cp(gb, A, 0x18);
		if (!CF)
			RETURN;
		xor_a(gb);
		wr8(gb, wCurObjAction, A);
		RETURN;
	}
	// .asm_49393: Wario grabs it
	set_update(gb, ZL_493DC);
	A = 0x01;
	wr8(gb, wCurObjVar1, A);
	ld_l(gb, OBJ_Y_POS);
	A = alu_add(gb, rd_hli(gb), 0x1b);
	wr8(gb, wWarioYPos + 1, A);
	A = alu_adc(gb, rd_hli(gb), 0x00);
	wr8(gb, wWarioYPos + 0, A);
	A = alu_add(gb, rd_hli(gb), 0x04);
	wr8(gb, wWarioXPos + 1, A);
	A = alu_adc(gb, rd_hl(gb), 0x00);
	wr8(gb, wWarioXPos + 0, A);
	A = alu_rlca(gb, rd8(gb, wCurObjSubState));
	SET_DE(gb, CF ? ADDR_Frameset_691d2 : ADDR_Frameset_691c9);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(ZipLineFunc, Func_493dc)
{
	no_actions(gb);
	gb_push16(gb, ZL(0xbe)); // call .Func_49418
	zl_49418(gb);
	gb_pop16(gb);
	A = rd8(gb, wCurObjVar1);
	alu_cp(gb, A, 0x03);
	if (ZF) {
		zl_493cd(gb);
		return;
	}
	A = rd8(gb, wTransformation);
	alu_cp(gb, A, TRANSFORMATION_RAIL);
	if (ZF)
		RETURN;
	set_update(gb, ZL_49405);
	A = 0x0a;
	wr8(gb, wCurObjStateDuration, A);
	A = 0x02;
	wr8(gb, wCurObjVar1, A);
	RETURN;
}

PORT_LOCAL(ZipLineFunc, Func_49405)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	zl_49356(gb);
}

PORT_LOCAL(ZipLineFunc, Func_49412) { no_actions(gb); }
