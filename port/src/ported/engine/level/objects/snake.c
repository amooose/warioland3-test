// Port of src/engine/level/objects/snake.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define SNAKE_FIRE_UPDATE (ADDR_SnakeFireFunc + 0x13) // .Update

PORT(SnakeFireFunc)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	uint16_t hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_INVISIBLE));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(SNAKE_FIRE_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)SNAKE_FIRE_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

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


#define SN(off) (ADDR_SnakeFunc + (off))
#define SN_INIT SN(0x14)
#define SN_UPDATE SN(0x53)

static void set_update_l(GB *gb, uint16_t f)
{
	ld_l(gb, OBJ_UPDATE_FUNCTION + 1);
	A = (uint8_t)(f >> 8); wr_hld(gb, A);
	A = (uint8_t)f; wr_hld(gb, A);
}

PORT(SnakeFunc)
{
	ld_hl(gb, wCurObjUpdateFunction + 1);
	A = (uint8_t)(SN_INIT >> 8); wr_hld(gb, A);
	A = (uint8_t)SN_INIT; wr_hld(gb, A);
	ld_l(gb, OBJ_VAR_2);
	A = rd8(gb, wCurObjYPos + 0); wr_hli(gb, A);
	A = rd8(gb, wCurObjYPos + 1); wr_hl(gb, A);
}

PORT_LOCAL(SnakeFunc, Init)
{
	ld_hl(gb, wCurObjFlags);
	res_mhl(gb, OBJFLAG_INVISIBLE_F);
	set_update_l(gb, SN_UPDATE);
	ld_l(gb, OBJ_SUBSTATE);
	res_mhl(gb, OBJSUBFLAG_UNINITIALISED_F);
	gb->cpu.b = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, A, gb->cpu.b);
	if (CF) {
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
		SET_DE(gb, ADDR_Frameset_6881d);
	} else {
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
		SET_DE(gb, ADDR_Frameset_68804);
	}
	CALL(SetObjectFramesetPtr);
	A = 0x78; wr_hli(gb, A);
	xor_a(gb);
	wr_hl(gb, A);
	ld_l(gb, OBJ_COLLBOX_RIGHT);
	A = 0xff; wr_hld(gb, A);
	A = 0xf1; wr_hld(gb, A);
	A = 0x0f; wr_hl(gb, A);
}

// .Func_49b76: rises out of / sinks into the pipe at the screen edges
static void sn_49b76(GB *gb)
{
	A = rd8(gb, wCurObjScreenYPos);
	alu_cp(gb, A, 0x10);
	bool b8a = CF;
	if (!b8a) {
		alu_cp(gb, A, 0xc0);
		if (CF) {
			alu_cp(gb, A, 0xa0);
			b8a = !CF;
		}
	}
	if (b8a) {
		A = 0x48;
		wr8(gb, wCurObjState, A);
	} else {
		xor_a(gb);
		wr8(gb, wCurObjState, A);
	}
}

PORT_LOCAL(SnakeFunc, Func_49b76) { sn_49b76(gb); }

// .Func_49af0: turns towards Wario
static void sn_49af0(GB *gb)
{
	ld_hl(gb, wCurObjVar1);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		if (dec_mhl(gb))
			return;
		ld_l(gb, OBJ_SUBSTATE);
		A = alu_and(gb, rd_hl(gb), 0x0f);
		bool right = alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F);
		alu_cp(gb, A, 0x04);
		if (!right)
			SET_DE(gb, ZF ? ADDR_Frameset_68852 : ADDR_Frameset_68804);
		else
			SET_DE(gb, ZF ? ADDR_Frameset_688c6 : ADDR_Frameset_6881d);
		CALL(SetObjectFramesetPtr);
		return;
	}
	gb->cpu.c = 0x2a;
	A = alu_add(gb, rd8(gb, wWarioScreenYPos), gb->cpu.c);
	gb->cpu.b = A;
	A = alu_add(gb, rd8(gb, wCurObjScreenYPos), gb->cpu.c);
	A = alu_sub(gb, A, gb->cpu.b);
	alu_cp(gb, A, 0x28);
	if (!CF) {
		alu_cp(gb, A, 0xe8);
		if (CF)
			return;
	}
	ld_hl(gb, wCurObjSubState);
	A = alu_add(gb, rd8(gb, wWarioScreenXPos), gb->cpu.c);
	gb->cpu.b = A;
	A = alu_add(gb, rd8(gb, wCurObjScreenXPos), gb->cpu.c);
	A = alu_sub(gb, A, gb->cpu.b);
	if (!CF) {
		if (!alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F))
			return;
		res_mhl(gb, OBJSUBFLAG_HDIR_F);
		A = alu_and(gb, rd_hl(gb), 0x0f);
		alu_cp(gb, A, 0x04);
		SET_DE(gb, ZF ? ADDR_Frameset_6884b : ADDR_Frameset_6883d);
	} else {
		if (alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F))
			return;
		set_mhl(gb, OBJSUBFLAG_HDIR_F);
		A = alu_and(gb, rd_hl(gb), 0x0f);
		alu_cp(gb, A, 0x04);
		SET_DE(gb, ZF ? ADDR_Frameset_68844 : ADDR_Frameset_68836);
	}
	CALL(SetObjectFramesetPtr);
	inc_l(gb);
	A = 0x14;
	wr_hl(gb, A);
}

PORT_LOCAL(SnakeFunc, Func_49af0) { sn_49af0(gb); }

// .Func_49a85 (hl = OBJ_VAR_1)
static void sn_49a85(GB *gb)
{
	xor_a(gb);
	wr_hl(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	A = alu_and(gb, rd_hl(gb), 0xf0);
	A = alu_or(gb, A, 0x08);
	wr_hl(gb, A);
	A = alu_rlca(gb, A);
	SET_DE(gb, CF ? ADDR_Frameset_6881d : ADDR_Frameset_68804);
	CALL(SetObjectFramesetPtr);
	A = 0x01; wr_hli(gb, A);
	RETURN;
}

PORT_LOCAL_JUMP(SnakeFunc, Func_49a85) { sn_49a85(gb); }

PORT_LOCAL_JUMP(SnakeFunc, Update)
{
	CALL(SetOneWayPlatformAction);
	A = 0x0f;
	wr8(gb, wCurObjCollBoxBottom, A);
	sn_49af0(gb);
	ld_hl(gb, wCurObjState);
	A = rd_hld(gb);
	if (alu_and(gb, A, A)) {
		inc_l(gb);
		gb->cpu.b = 0x00;
		wr_hl(gb, gb->cpu.b);
		ld_l(gb, OBJ_FLAGS);
		res_mhl(gb, OBJFLAG_NO_COLLISION_F);
		alu_cp(gb, A, 0x0a);
		if (ZF) {
			play_sfx(gb, SFX_VANISH);
			ld_l(gb, OBJ_VAR_1);
			sn_49a85(gb);
			return;
		}
		A = alu_and(gb, A, 0xfe);
		alu_cp(gb, A, 0x04);
		bool hit = ZF;
		if (!hit) {
			alu_cp(gb, A, 0x28);
			hit = ZF;
		}
		if (hit) {
			ld_l(gb, OBJ_VAR_1);
			sn_49a85(gb);
			return;
		}
		ld_l(gb, OBJ_SUBSTATE);
	}
	// .asm_49920
	A = alu_and(gb, rd_hl(gb), 0x0f);
	JUMP_TO(jumptable(gb, SN(0x86)));
}

PORT_LOCAL(SnakeFunc, Func_49943)
{
	ld_hl(gb, wCurObjStateDuration);
	if (dec_mhl(gb))
		return;
	A = 0x3e;
	wr_hl(gb, A);
	ld_l(gb, OBJ_SUBSTATE);
	inc_mhl(gb);
	play_sfx(gb, SFX_11A);
}

PORT_LOCAL_JUMP(SnakeFunc, Func_49957)
{
	sn_49b76(gb);
	ld_hl(gb, wCurObjStateDuration);
	if (!dec_mhl(gb)) {
		A = 0x53; wr_hli(gb, A);
		ld_l(gb, OBJ_SUBSTATE);
		inc_mhl(gb);
		RETURN;
	}
	ld_l(gb, OBJ_FLAGS);
	if (!alu_bit(gb, rd_hl(gb), OBJFLAG_STEPPED_F)) {
		JUMP(MoveObjectUp_Slow);
		return;
	}
	res_mhl(gb, OBJFLAG_STEPPED_F);
	xor_a(gb);
	wr8(gb, wCurObjCollBoxBottom, A);
	A = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (!CF)
		RETURN;
	gb->cpu.b = 0x01;
	gb_call_ret(gb, ADDR_MoveWarioUp, SN(0xd9));
	gb_call_ret(gb, ADDR_HandleUpwardsFloorTransition, SN(0xdc));
	JUMP(MoveObjectUp);
}

PORT_LOCAL(SnakeFunc, Func_49984)
{
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		dec_mhl(gb);
		return;
	}
	inc_l(gb);
	A = rd_hl(gb);
	bool nz = alu_and(gb, A, A);
	ld_l(gb, OBJ_SUBSTATE);
	if (nz) {
		inc_mhl(gb);
		A = 0x31;
		wr8(gb, wCurObjStateDuration, A);
		return;
	}
	A = rd8(gb, wCurObjFrame);
	if (!alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F)) {
		alu_cp(gb, A, 0x05);
		if (!ZF)
			return;
		SET_DE(gb, ADDR_Frameset_68852);
	} else {
		alu_cp(gb, A, 0x0e);
		if (!ZF)
			return;
		SET_DE(gb, ADDR_Frameset_688c6);
	}
	inc_mhl(gb);
	CALL(SetObjectFramesetPtr);
	A = 0x31; wr_hli(gb, A);
}

// ld a, [hld] / cp $0a / ret nz / ld a, [hld] / and a / ret nz / play_sfx
// (hl = OBJ_FRAMESET_OFFSET)
static void sn_frame_sfx(GB *gb)
{
	A = rd_hld(gb);
	alu_cp(gb, A, 0x0a);
	if (!ZF)
		return;
	A = rd_hld(gb);
	if (alu_and(gb, A, A))
		return;
	play_sfx(gb, SFX_08A);
}

PORT_LOCAL(SnakeFunc, Func_499b8)
{
	ld_hl(gb, wCurObjStateDuration);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		dec_mhl(gb);
		ld_l(gb, OBJ_FRAMESET_OFFSET);
		sn_frame_sfx(gb);
		return;
	}
	inc_l(gb);
	A = rd_hl(gb);
	if (alu_and(gb, A, A))
		return;
	A = rd8(gb, wCurObjFrame);
	ld_l(gb, OBJ_SUBSTATE);
	if (!alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F)) {
		alu_cp(gb, A, 0x22);
		if (!ZF)
			return;
		SET_DE(gb, ADDR_Frameset_6886f);
	} else {
		alu_cp(gb, A, 0x45);
		if (!ZF)
			return;
		SET_DE(gb, ADDR_Frameset_688e3);
	}
	inc_mhl(gb);
	CALL(SetObjectFramesetPtr);
	A = 0x31; wr_hli(gb, A);
	A = 0x02; wr_hl(gb, A);
}

// .asm_49a5f
static void sn_49a5f(GB *gb)
{
	A = 0x31;
	wr8(gb, wCurObjStateDuration, A);
	ld_hl(gb, wCurObjSubState);
	A = alu_and(gb, rd_hl(gb), 0xf0);
	A = alu_or(gb, A, 0x07);
	wr_hl(gb, A);
	A = alu_rlca(gb, A);
	SET_DE(gb, CF ? ADDR_Frameset_6891d : ADDR_Frameset_688a9);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(SnakeFunc, Func_499f8)
{
	ld_hl(gb, wCurObjVar1);
	A = 0x02; wr_hld(gb, A);
	if (dec_mhl(gb)) {
		dec_l(gb);
		sn_frame_sfx(gb);
		RETURN;
	}
	// .asm_49a12: needs a free object slot for the fire
	A = (uint8_t)wObj1;
	gb->cpu.b = (uint8_t)wObj1;
	gb->cpu.h = (uint8_t)(wObj1 >> 8);
	for (;;) {
		ld_l(gb, OBJ_FLAGS);
		A = alu_add(gb, A, gb->cpu.l);
		gb->cpu.l = A;
		if (!alu_bit(gb, rd_hl(gb), OBJFLAG_ACTIVE_F))
			break;
		A = gb->cpu.b;
		alu_cp(gb, A, (uint8_t)wObj8);
		if (ZF) {
			sn_49a5f(gb);
			return;
		}
		A = alu_add(gb, A, OBJ_STRUCT_LENGTH);
		gb->cpu.b = A;
	}
	A = 0x93;
	wr8(gb, wCurObjStateDuration, A);
	play_sfx(gb, SFX_089);
	ld_hl(gb, wCurObjSubState);
	inc_mhl(gb);
	if (!alu_bit(gb, rd_hl(gb), OBJSUBFLAG_HDIR_F)) {
		SET_DE(gb, ADDR_Frameset_6888c);
		CALL(SetObjectFramesetPtr);
		SET_BC(gb, ADDR_ObjParams_SnakeFireLeft);
	} else {
		SET_DE(gb, ADDR_Frameset_68900);
		CALL(SetObjectFramesetPtr);
		SET_BC(gb, ADDR_ObjParams_SnakeFireRight);
	}
	JUMP(CreateObjectAtRelativePos);
}

PORT_LOCAL_JUMP(SnakeFunc, Func_49a57)
{
	ld_hl(gb, wCurObjVar1);
	A = 0x02; wr_hld(gb, A);
	if (dec_mhl(gb))
		RETURN;
	sn_49a5f(gb);
}

PORT_LOCAL_JUMP(SnakeFunc, Func_49a7c)
{
	ld_hl(gb, wCurObjVar1);
	A = 0x02; wr_hld(gb, A);
	if (dec_mhl(gb))
		RETURN;
	inc_l(gb);
	sn_49a85(gb);
}

PORT_LOCAL_JUMP(SnakeFunc, Func_49aa1)
{
	sn_49b76(gb);
	ld_hl(gb, wCurObjVar2);
	A = rd_hli(gb);
	gb->cpu.b = A;
	A = rd8(gb, wCurObjYPos + 0);
	alu_cp(gb, A, gb->cpu.b);
	bool back = false;
	if (ZF) {
		gb->cpu.b = rd_hl(gb);
		A = rd8(gb, wCurObjYPos + 1);
		alu_cp(gb, A, gb->cpu.b);
		back = ZF;
	}
	if (back) {
		// .asm_49ae2
		ld_l(gb, OBJ_STATE_DURATION);
		A = 0x78;
		wr_hl(gb, A);
		ld_l(gb, OBJ_SUBSTATE);
		A = alu_and(gb, rd_hl(gb), 0xf0);
		A = alu_or(gb, A, 0x01);
		wr_hld(gb, A);
		RETURN;
	}
	ld_l(gb, OBJ_STATE_DURATION);
	A = rd_hl(gb);
	if (alu_and(gb, A, A)) {
		dec_mhl(gb);
		play_sfx(gb, SFX_08B);
	}
	ld_l(gb, OBJ_FLAGS);
	if (!alu_bit(gb, rd_hl(gb), OBJFLAG_STEPPED_F)) {
		JUMP(MoveObjectDown_Slow);
		return;
	}
	res_mhl(gb, OBJFLAG_STEPPED_F);
	xor_a(gb);
	wr8(gb, wCurObjCollBoxBottom, A);
	A = alu_rra(gb, rd8(gb, wGlobalCounter));
	if (!CF)
		RETURN;
	gb->cpu.b = 0x01;
	gb_call_ret(gb, ADDR_MoveWarioDown, SN(0x23e));
	gb_call_ret(gb, ADDR_HandleDownwardsFloorTransition, SN(0x241));
	JUMP(MoveObjectDown);
}

// The fire stays while its snake (the active object with id $2d that
// created it) is in substate 6.
PORT_LOCAL(SnakeFireFunc, Update)
{
	ld_hl(gb, wCurObjUnk01);
	A = rd_hli(gb);
	gb->cpu.c = A;
	gb->cpu.b = rd_hl(gb);
	gb->cpu.e = OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT;
	gb->cpu.d = 0x2d;
	gb->cpu.h = (uint8_t)(wObjects >> 8);
	bool found = false;
	for (int n = 0; n < NUM_OBJECTS && !found; n++) {
		uint8_t base = (uint8_t)(wObjects + n * OBJ_STRUCT_LENGTH);
		ld_l(gb, (uint8_t)(base + OBJ_UNK_02));
		A = rd_hld(gb);
		alu_cp(gb, A, gb->cpu.b);
		if (!ZF)
			continue;
		A = rd_hld(gb);
		alu_cp(gb, A, gb->cpu.c);
		if (!ZF)
			continue;
		A = alu_and(gb, rd_hl(gb), gb->cpu.e);
		A = alu_dec(gb, A);
		if (!ZF)
			continue;
		A = rd8(gb, (uint16_t)(wObjects + n * OBJ_STRUCT_LENGTH + OBJ_ID));
		alu_cp(gb, A, gb->cpu.d);
		found = ZF;
	}
	if (found) {
		A = alu_add(gb, gb->cpu.l, OBJ_SUBSTATE - OBJ_FLAGS);
		gb->cpu.l = A;
		A = alu_and(gb, rd_hl(gb), 0x0f);
		alu_cp(gb, A, 0x06);
		if (ZF)
			return;
	}
	xor_a(gb);
	wr8(gb, wCurObjFlags, A);
}
