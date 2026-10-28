// Port of src/engine/level/objects/rock.asm
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


#define RK(off) (ADDR_RockFunc + (off))
#define RK_UPDATE RK(0x28)

static void set_state(GB *gb, uint8_t state)
{
	A = state;
	wr8(gb, wCurObjState, A);
}

static void no_actions(GB *gb)
{
	A = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, A);
}

PORT(RockFunc)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(RK_UPDATE >> 8); wr_hld(gb, A);
	A = (uint8_t)RK_UPDATE; wr_hld(gb, A);
	SET_DE(gb, ADDR_Frameset_69bfe);
	CALL(SetObjectFramesetPtr);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0x06; wr_hld(gb, A);
	A = 0xf9; wr_hld(gb, A);
	A = 0x00; wr_hl(gb, A);
	set_state(gb, 0x31);
}

PORT_LOCAL_JUMP(RockFunc, Update)
{
	A = rd8(gb, wCurObjState);
	JUMP_TO(jumptable(gb, RK_UPDATE + 4));
}

static void rk_4aa0a(GB *gb)
{
	no_actions(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	ld_l(gb, OBJ_UNK_02);
	FARCALL(DespawnObject);
}

PORT_LOCAL(RockFunc, Destroy)
{
	set_state(gb, 0x2f);
	SET_DE(gb, ADDR_Frameset_69c01);
	CALL(SetObjectFramesetPtr);
	A = 0x19; wr_hli(gb, A);
	ld_l(gb, OBJ_FLAGS);
	res_mhl(gb, OBJFLAG_GRABBED_F);
	set_mhl(gb, OBJFLAG_NO_COLLISION_F);
	play_sfx(gb, SFX_TEMPLE_ROCK);
	rk_4aa0a(gb);
}

PORT_LOCAL(RockFunc, Func_4aa0a) { rk_4aa0a(gb); }
PORT_LOCAL(RockFunc, Func_4aa26) {}

static void rk_bump(GB *gb, uint8_t state)
{
	ld_hl(gb, wCurObjState);
	A = state; wr_hld(gb, A);
	A = 0x0c;
	wr8(gb, wCurObjStateDuration, A);
	A = 0x01;
	wr8(gb, wCurObjVar2, A);
}

PORT_LOCAL(RockFunc, Func_4aa27) { rk_bump(gb, 0x32); }
PORT_LOCAL(RockFunc, Func_4aa2f) { rk_bump(gb, 0x33); }

static void rk_4aa46(GB *gb, uint8_t state)
{
	set_state(gb, state);
	xor_a(gb);
	wr8(gb, wCurObjVar3, A);
	A = 0x02;
	wr8(gb, wCurObjVar2, A);
}

PORT_LOCAL(RockFunc, Func_4aa40) { rk_4aa46(gb, 0x44); }
PORT_LOCAL(RockFunc, Func_4aa44) { rk_4aa46(gb, 0x45); }

static void rk_slide(GB *gb, uint8_t state, uint8_t var2)
{
	ld_hl(gb, wCurObjState);
	A = state; wr_hld(gb, A);
	dec_l(gb);
	A = 0x41; wr_hld(gb, A);
	A = var2; wr_hld(gb, A);
	A = 0x01; wr_hld(gb, A);
}

PORT_LOCAL_JUMP(RockFunc, Func_4aa53)
{
	rk_slide(gb, 0x36, 0xc0);
	JUMP(Func_3335);
}

PORT_LOCAL_JUMP(RockFunc, Func_4aa66)
{
	rk_slide(gb, 0x37, 0xe0);
	JUMP(Func_3344);
}

static void rk_grabbed(GB *gb, uint8_t state)
{
	set_state(gb, state);
	ld_hl(gb, wCurObjFlags);
	set_mhl(gb, OBJFLAG_GRABBED_F);
}

static void rk_flip(GB *gb)
{
	ld_hl(gb, wCurObjSubState);
	A = alu_xor(gb, rd_hl(gb), 0x80);
	wr_hl(gb, A);
}

static void rk_4aa9d(GB *gb);

static void rk_4aa83(GB *gb)
{
	A = rd8(gb, wDirection);
	if (alu_and(gb, A, A)) {
		JUMP(Func_3353);
		return;
	}
	rk_flip(gb);
	rk_grabbed(gb, 0x48);
	rk_4aa9d(gb);
}

static void rk_4aa9d(GB *gb)
{
	A = rd8(gb, wDirection);
	if (!alu_and(gb, A, A)) {
		JUMP(Func_3362);
		return;
	}
	rk_flip(gb);
	rk_grabbed(gb, 0x49);
	rk_4aa83(gb);
}

PORT_LOCAL_JUMP(RockFunc, Func_4aa79)
{
	rk_grabbed(gb, 0x49);
	rk_4aa83(gb);
}

PORT_LOCAL_JUMP(RockFunc, Func_4aa83) { rk_4aa83(gb); }

PORT_LOCAL_JUMP(RockFunc, Func_4aa93)
{
	rk_grabbed(gb, 0x48);
	rk_4aa9d(gb);
}

PORT_LOCAL_JUMP(RockFunc, Func_4aa9d) { rk_4aa9d(gb); }

PORT_LOCAL(RockFunc, Func_4aaae)
{
	ld_hl(gb, wCurObjAction);
	A = 0x9f; wr_hld(gb, A);
	A = 0x42; wr_hld(gb, A);
	dec_l(gb);
	xor_a(gb);
	wr_hl(gb, A);
}

PORT_LOCAL_JUMP(RockFunc, Func_4aabb)
{
	set_state(gb, 0x5e);
	JUMP(ObjState_FullThrowLeft);
}

PORT_LOCAL_JUMP(RockFunc, Func_4aac3)
{
	set_state(gb, 0x5f);
	JUMP(ObjState_FullThrowRight);
}

// .asm_4aae9: a/c = the update function, b = var2
static void rk_hit(GB *gb, uint16_t f)
{
	A = (uint8_t)(f >> 8);
	gb->cpu.c = (uint8_t)f;
	gb->cpu.b = 0x02;
	ld_hl(gb, wCurObjUpdateFunction + 1);
	wr_hld(gb, A);
	wr_hl(gb, gb->cpu.c);
	ld_l(gb, OBJ_VAR_3);
	xor_a(gb);
	wr_hld(gb, A);
	wr_hl(gb, gb->cpu.b);
	no_actions(gb);
}

PORT_LOCAL(RockFunc, Func_4aacb) { rk_hit(gb, ADDR_Func_33f8); }
PORT_LOCAL(RockFunc, Func_4aad3) { rk_hit(gb, ADDR_Func_3407); }
PORT_LOCAL(RockFunc, Func_4aadb) { rk_hit(gb, ADDR_Func_3317); }
PORT_LOCAL(RockFunc, Func_4aae3) { rk_hit(gb, ADDR_Func_3326); }
