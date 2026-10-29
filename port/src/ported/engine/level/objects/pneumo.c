// Port of src/engine/level/objects/pneumo.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))
#define ZF (gb->cpu.f & FLAG_Z)
#define CF (gb->cpu.f & FLAG_C)

#define PN(off) ((uint16_t)(ADDR_PneumoFunc + (off)))
#define PN_UPDATE PN(0x41) // .Update
#define PN_TABLE PN(0x6f) // the table after .substate_table's rst JumpTable

static inline void set_bit(GB *gb, uint16_t hl, int bit)
{
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << bit)));
}

static inline void res_bit(GB *gb, uint16_t hl, int bit)
{
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << bit)));
}

static inline void and_a(GB *gb)
{
	alu_and(gb, gb->cpu.a, gb->cpu.a);
}

static inline void xor_a(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
}

// ld a, [hl] / and OBJSUBFLAGS_MASK / or v
static void substate_or(GB *gb, uint16_t hl, uint8_t v)
{
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), OBJSUBFLAGS_MASK), v);
}

// ld a, [first] / add c / ld b, a / ld a, [second] / add c / sub b
static void screen_diff(GB *gb, uint16_t first, uint16_t second)
{
	gb->cpu.a = alu_add(gb, rd8(gb, first), gb->cpu.c);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_add(gb, rd8(gb, second), gb->cpu.c);
	gb->cpu.a = alu_sub(gb, gb->cpu.a, gb->cpu.b);
}

// hYPos/hXPos = the object's position
static void obj_pos_to_hpos(GB *gb)
{
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hYPosHi, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hXPosLo, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

// hYPos = the object's y - dy, hXPos = its x (ld [hli] all the way)
static void obj_pos_up_to_hpos(GB *gb, uint8_t dy)
{
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), dy);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	wr8(gb, hYPosHi, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hXPosLo, gb->cpu.a);
	gb->cpu.a = rd8(gb, hl++);
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT(PneumoFunc)
{
	uint16_t hl = wCurObjFlags;
	res_bit(gb, hl, OBJFLAG_INVISIBLE_F);
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(PN_UPDATE >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)PN_UPDATE;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68c5c);
	CALL(SetObjectFramesetPtr);
	hl = LO(REG_HL(gb), OBJ_SUBSTATE);
	res_bit(gb, hl, OBJSUBFLAG_UNINITIALISED_F);
	hl = LO(hl, alu_dec(gb, (uint8_t)hl));
	xor_a(gb);
	wr8(gb, hl--, 0);
	hl = LO(hl, alu_dec(gb, (uint8_t)hl));
	wr8(gb, hl--, 0);
	hl = LO(hl, OBJ_COLLBOX_RIGHT);
	gb->cpu.a = 0x07;
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = 0xf8;
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = 0x02;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// .Func_456fe: back to idling
static void pn_idle(GB *gb)
{
	SET_DE(gb, ADDR_Frameset_68c5c);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = LO(REG_HL(gb), OBJ_SUBSTATE);
	substate_or(gb, hl, 0x01);
	wr8(gb, hl--, gb->cpu.a);
	xor_a(gb);
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	RET();
}

// .Func_4589d: inflates, facing Wario
static void pn_inflate(GB *gb)
{
	uint16_t hl = wCurObjSubState;
	SET_HL(gb, hl);
	substate_or(gb, hl, 0x02);
	wr8(gb, hl, gb->cpu.a);
	gb->cpu.c = 0x2a;
	screen_diff(gb, wWarioScreenXPos, wCurObjScreenXPos);
	if (CF)
		res_bit(gb, hl, OBJSUBFLAG_HDIR_F);
	else
		set_bit(gb, hl, OBJSUBFLAG_HDIR_F);
	hl = LO(hl, alu_dec(gb, (uint8_t)hl));
	xor_a(gb);
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_68c8e);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 27;
	wr8(gb, hl++, 27);
	SET_HL(gb, hl);
	RET();
}

// .asm_4581d: moves sideways by OBJ_VAR_2, turning around at a wall
static void pn_move_sideways(GB *gb, uint16_t ret_left, uint16_t ret_right, uint16_t left,
	uint16_t right, bool flip)
{
	uint16_t hl = wCurObjYPos;
	gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 4);
	wr8(gb, hYPosLo, gb->cpu.a);
	gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	wr8(gb, hYPosHi, gb->cpu.a);
	gb->cpu.a = alu_rlca(gb, rd8(gb, wCurObjSubState));
	bool facing_right = CF;
	if (facing_right) {
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), 7);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0);
	} else {
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 8);
		wr8(gb, hXPosLo, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0);
	}
	wr8(gb, hXPosHi, gb->cpu.a);
	SET_HL(gb, hl);
	gb_call_ret(gb, ADDR_Func_3513, facing_right ? ret_right : ret_left);
	and_a(gb);
	if (ZF) {
		JUMP_TO(facing_right ? right : left);
		return;
	}
	hl = wCurObjSubState;
	SET_HL(gb, hl);
	if (flip) {
		gb->cpu.a = alu_xor(gb, rd8(gb, hl), OBJSUBFLAG_HDIR);
		wr8(gb, hl, gb->cpu.a);
	} else if (facing_right) {
		res_bit(gb, hl, OBJSUBFLAG_HDIR_F);
	} else {
		set_bit(gb, hl, OBJSUBFLAG_HDIR_F);
	}
	RET();
}

static void pn_walk(GB *gb)
{
	pn_move_sideways(gb, 0x583d, 0x5850, ADDR_MoveObjectLeftByVar2, ADDR_MoveObjectRightByVar2,
		true);
}

// .Update: reacts to being hit, then runs the substate's function
PORT_LOCAL_JUMP(PneumoFunc, Update)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	and_a(gb);
	uint8_t a = gb->cpu.a;
	uint16_t func = 0;
	if (!ZF) {
		alu_cp(gb, a, 0x0e);
		if (ZF) {
			xor_a(gb);
			wr8(gb, hl--, 0);
			SET_HL(gb, hl);
			pn_inflate(gb);
			return;
		}
		alu_cp(gb, a, 0x0a);
		if (ZF) {
			JUMP(Func_3173);
			return;
		}
		alu_cp(gb, a, 0x04);
		if (CF) {
			xor_a(gb);
			wr8(gb, hl--, 0);
			SET_HL(gb, hl);
			gb->cpu.a = rd8(gb, wTransformation);
			alu_cp(gb, gb->cpu.a, TRANSFORMATION_PUFFY_WARIO);
			if (ZF) {
				pn_inflate(gb);
				return;
			}
			gb->cpu.a = alu_and(gb, rd8(gb, hl), 0x0f);
			alu_cp(gb, gb->cpu.a, 0x01);
			if (ZF) {
				pn_inflate(gb);
				return;
			}
		} else {
			alu_cp(gb, a, 0x28);
			if (ZF) {
				func = ADDR_Func_33f8;
			} else {
				alu_cp(gb, a, 0x29);
				if (ZF) {
					func = ADDR_Func_3407;
				} else {
					alu_cp(gb, a, 0x04);
					if (ZF) {
						func = ADDR_Func_3326;
					} else {
						alu_cp(gb, a, 0x05);
						if (ZF) {
							func = ADDR_Func_3317;
						} else {
							gb->cpu.a = alu_rra(gb, a);
							func = CF ? ADDR_Func_33f8 : ADDR_Func_3407;
						}
					}
				}
			}
			// .asm_456e8
			gb->cpu.a = (uint8_t)(func >> 8);
			gb->cpu.c = (uint8_t)func;
			gb->cpu.b = 0x02;
			hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
			wr8(gb, hl--, gb->cpu.a);
			wr8(gb, hl, gb->cpu.c);
			hl = LO(hl, OBJ_VAR_3);
			xor_a(gb);
			wr8(gb, hl--, 0);
			gb->cpu.a = gb->cpu.b;
			wr8(gb, hl--, gb->cpu.a);
			SET_HL(gb, hl);
			SET_DE(gb, ADDR_Frameset_68c89);
			CALL(SetObjectFramesetPtr);
			gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
			wr8(gb, wCurObjAction, gb->cpu.a);
			RET();
			return;
		}
	}
	// .substate_table
	hl = LO(hl, OBJ_SUBSTATE);
	SET_HL(gb, hl);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), 0x0f);
	// rst JumpTable
	gb_push16(gb, PN_TABLE);
	uint16_t t = gb_pop16(gb);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	t = alu_add16(gb, t, gb->cpu.e);
	gb->cpu.a = rd8(gb, t);
	gb->cpu.h = rd8(gb, (uint16_t)(t + 1));
	gb->cpu.l = gb->cpu.a;
	JUMP_TO(REG_HL(gb));
}

PORT_LOCAL_JUMP(PneumoFunc, Func_456fe) { pn_idle(gb); }

PORT_LOCAL_JUMP(PneumoFunc, Func_4589d) { pn_inflate(gb); }

// Idle: bobs up and down by Data_44000, and attacks when Wario is near
PORT_LOCAL_JUMP(PneumoFunc, Func_4570f)
{
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenYPos), 0x2a);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_add(gb, rd8(gb, wWarioScreenYPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (!CF) {
		alu_cp(gb, gb->cpu.a, 0x61);
		if (CF) {
			gb->cpu.a = rd8(gb, wTransformation);
			alu_cp(gb, gb->cpu.a, TRANSFORMATION_PUFFY_WARIO);
			if (!ZF) {
				gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x3f);
				if (ZF) {
					uint16_t hl = wCurObjSubState;
					substate_or(gb, hl, 0x06);
					wr8(gb, hl--, gb->cpu.a);
					SET_HL(gb, hl);
					RET();
					return;
				}
			}
		}
	}

	uint16_t hl = wCurObjVar3;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.d = (uint8_t)(ADDR_Data_44000 >> 8);
	for (;;) {
		gb->cpu.a = alu_add(gb, gb->cpu.a, (uint8_t)ADDR_Data_44000);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = rd8(gb, REG_DE(gb));
		alu_cp(gb, gb->cpu.a, 0x80);
		if (!ZF)
			break;
		// the end of the bobbing: attack if Wario is in range
		gb->cpu.a = rd8(gb, wTransformation);
		alu_cp(gb, gb->cpu.a, TRANSFORMATION_PUFFY_WARIO);
		if (!ZF) {
			gb->cpu.c = 0x2a;
			screen_diff(gb, wWarioScreenYPos, wCurObjScreenYPos);
			alu_cp(gb, gb->cpu.a, 0xb0);
			bool in_range = false;
			if (!CF) {
				alu_cp(gb, gb->cpu.a, 0xec);
				in_range = CF;
			}
			if (in_range) {
				screen_diff(gb, wWarioScreenXPos, wCurObjScreenXPos);
				bool attack = true, face_right = false;
				alu_cp(gb, gb->cpu.a, 0x60);
				if (CF) {
					alu_cp(gb, gb->cpu.a, 0x10);
					bool nc = !CF;
					gb->cpu.a = 0x01;
					if (!nc)
						gb->cpu.a = alu_dec(gb, gb->cpu.a);
				} else {
					alu_cp(gb, gb->cpu.a, SCREEN_WIDTH_PX);
					if (CF) {
						attack = false;
					} else {
						alu_cp(gb, gb->cpu.a, 0xf0);
						bool c = CF;
						gb->cpu.a = 0x01;
						if (!c)
							gb->cpu.a = alu_dec(gb, gb->cpu.a);
						face_right = true;
					}
				}
				if (attack) {
					hl = LO(hl, OBJ_SUBSTATE);
					if (face_right)
						set_bit(gb, hl, OBJSUBFLAG_HDIR_F);
					else
						res_bit(gb, hl, OBJSUBFLAG_HDIR_F);
					SET_DE(gb, face_right ? ADDR_Frameset_68c84 : ADDR_Frameset_68c7f);
					gb->cpu.c = 0x03;
					// .asm_457ae
					wr8(gb, wCurObjVar2, gb->cpu.a);
					substate_or(gb, hl, gb->cpu.c);
					wr8(gb, hl, gb->cpu.a);
					SET_HL(gb, hl);
					CALL(SetObjectFramesetPtr);
					hl = REG_HL(gb);
					gb->cpu.a = 1;
					wr8(gb, hl++, 1);
					SET_HL(gb, hl);
					RET();
					return;
				}
			}
		}
		// .asm_45798: start the bobbing over
		xor_a(gb);
		wr8(gb, hl, 0);
	}
	// .asm_4579c
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	hl = LO(hl, OBJ_Y_POS);
	alu_cp(gb, gb->cpu.a, 0x80);
	bool negative = !CF;
	gb->cpu.c = rd8(gb, hl);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	if (negative ? !CF : CF)
		wr8(gb, hl, negative ? alu_dec(gb, rd8(gb, hl)) : alu_inc(gb, rd8(gb, hl)));
	RET();
}

// Walking towards Wario (rising when there's a floor below)
PORT_LOCAL_JUMP(PneumoFunc, Func_457bd)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	and_a(gb);
	if (!ZF) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		RET();
		return;
	}
	xor_a(gb);
	wr8(gb, wc1ca, 0);
	obj_pos_to_hpos(gb);
	gb_call_ret(gb, ADDR_Func_352b, 0x57dc);
	and_a(gb);
	bool stop;
	if (!ZF) {
		gb->cpu.a = rd8(gb, wc1ca);
		and_a(gb);
		stop = ZF;
	} else {
		gb->cpu.a = rd8(gb, wIsInWaterOrSand);
		and_a(gb);
		stop = !ZF;
	}
	if (!stop) {
		gb->cpu.c = 0x2a;
		screen_diff(gb, wCurObjScreenYPos, wWarioScreenYPos);
		alu_cp(gb, gb->cpu.a, 0x14);
		if (!CF) {
			alu_cp(gb, gb->cpu.a, 0x80);
			if (CF) {
				// .asm_45810
				hl = wCurObjVar1;
				wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
				hl = LO(hl, OBJ_Y_POS);
				gb->cpu.a = alu_add(gb, rd8(gb, hl), 2);
				wr8(gb, hl++, gb->cpu.a);
				SET_HL(gb, hl);
				if (CF)
					wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
				pn_walk(gb);
				return;
			}
		}
	}
	// .asm_45801
	hl = wCurObjSubState;
	SET_HL(gb, hl);
	substate_or(gb, hl, 0x04);
	wr8(gb, hl, gb->cpu.a);
	gb->cpu.a = 255;
	wr8(gb, wCurObjStateDuration, 255);
	RET();
}

// Walking and sinking back down
PORT_LOCAL_JUMP(PneumoFunc, Func_4585c)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	if (ZF) {
		pn_idle(gb);
		return;
	}
	obj_pos_up_to_hpos(gb, 16);
	gb_call_ret(gb, ADDR_Func_358b, 0x5879);
	and_a(gb);
	if (ZF) {
		hl = wCurObjVar1;
		SET_HL(gb, hl);
		gb->cpu.a = rd8(gb, hl);
		and_a(gb);
		if (ZF) {
			pn_idle(gb);
			return;
		}
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		hl = LO(hl, OBJ_Y_POS);
		gb->cpu.a = alu_sub(gb, rd8(gb, hl), 2);
		wr8(gb, hl++, gb->cpu.a);
		SET_HL(gb, hl);
		if (CF)
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	}
	pn_walk(gb);
}

PORT_LOCAL_JUMP(PneumoFunc, Func_45893)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	if (!ZF)
		pn_walk(gb);
	else
		pn_idle(gb);
}

// Inflated: floats up and moves fast
PORT_LOCAL_JUMP(PneumoFunc, Func_458c7)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	if (ZF) {
		// .Func_4592e
		hl = LO(hl, alu_inc(gb, (uint8_t)hl));
		SET_HL(gb, hl);
		xor_a(gb);
		wr8(gb, hl, 0);
		pn_idle(gb);
		return;
	}
	obj_pos_up_to_hpos(gb, 16);
	gb_call_ret(gb, ADDR_Func_358b, 0x58e4);
	and_a(gb);
	if (ZF) {
		SET_BC(gb, ADDR_Data_60600);
		CALL(ApplyObjYMovement);
	}
	pn_move_sideways(gb, 0x590d, 0x5924, ADDR_MoveObjectLeft_Fast, ADDR_MoveObjectRight_Fast,
		false);
}

// Moves up or down to Wario's height (rounded down to 4 pixels)
PORT_LOCAL_JUMP(PneumoFunc, Func_45934)
{
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_PUFFY_WARIO);
	if (ZF) {
		pn_idle(gb);
		return;
	}
	gb->cpu.a = rd8(gb, wWarioYPos + 0);
	gb->cpu.h = gb->cpu.a;
	gb->cpu.a = alu_and(gb, rd8(gb, wWarioYPos + 1), 0xfc);
	gb->cpu.l = gb->cpu.a;
	SET_DE(gb, hXPosLoBackup);
	uint16_t hl = alu_add16(gb, REG_HL(gb), REG_DE(gb));
	SET_HL(gb, hl);
	SET_DE(gb, hl);
	gb->cpu.a = rd8(gb, wCurObjYPos + 1);
	alu_cp(gb, gb->cpu.a, gb->cpu.d);
	bool up;
	if (ZF) {
		gb->cpu.a = alu_and(gb, rd8(gb, wCurObjYPos + 0), 0xfc);
		alu_cp(gb, gb->cpu.a, gb->cpu.e);
		if (ZF) {
			pn_idle(gb);
			return;
		}
		up = !CF;
	} else {
		up = !CF;
	}
	if (up) {
		obj_pos_up_to_hpos(gb, 16);
		gb_call_ret(gb, ADDR_Func_358b, 0x596a);
		and_a(gb);
		if (!ZF) {
			pn_inflate(gb);
			return;
		}
		JUMP(MoveObjectUp);
		return;
	}
	// .asm_4597c
	xor_a(gb);
	wr8(gb, wc1ca, 0);
	obj_pos_to_hpos(gb);
	gb_call_ret(gb, ADDR_Func_352b, 0x5992);
	and_a(gb);
	if (!ZF) {
		gb->cpu.a = rd8(gb, wc1ca);
		and_a(gb);
		if (!ZF) {
			JUMP(MoveObjectDown_Slow);
			return;
		}
		pn_idle(gb);
		return;
	}
	gb->cpu.a = rd8(gb, wIsInWaterOrSand);
	and_a(gb);
	if (ZF) {
		JUMP(MoveObjectDown);
		return;
	}
	pn_idle(gb);
}
