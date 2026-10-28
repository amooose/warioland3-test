// Port of src/engine/level/objects/hidden_figure.asm
#include "ported.h"

#define LO(hl, x) ((uint16_t)(((hl) & 0xff00) | (x)))

#define HIDDEN_FIGURE_FUNC_4CF55 (ADDR_HiddenFigureFaceFunc + 0x10e) // HiddenFigureFaceFunc.Func_4cf55

PORT(Func_4ce39)
{
	gb->cpu.a = 0x3c;
	wr8(gb, wCurObjStateDuration, 0x3c);
	gb->cpu.l = OBJ_UPDATE_FUNCTION + 1;
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = (uint8_t)(HIDDEN_FIGURE_FUNC_4CF55 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)HIDDEN_FIGURE_FUNC_4CF55;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// Reads the next value of the sequence at bc + [index] (the sequence ends with
// $80, after which its last value repeats) and advances the index.
static void next_offset(GB *gb, uint16_t index)
{
	SET_HL(gb, index);
	gb->cpu.a = alu_add(gb, rd8(gb, index), gb->cpu.c);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		gb->cpu.a = rd8(gb, REG_BC(gb));
	} else {
		wr8(gb, index, alu_inc(gb, rd8(gb, index)));
	}
}

// [addr] += next offset
static void add_offset(GB *gb, uint16_t index, uint16_t addr)
{
	next_offset(gb, index);
	SET_HL(gb, addr);
	gb->cpu.c = rd8(gb, addr);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	wr8(gb, addr, gb->cpu.a);
}

// The 16-bit position at hl's object's pos += next offset (sign-extended)
static void add_pos_offset(GB *gb, uint16_t index, uint8_t pos)
{
	next_offset(gb, index);
	uint16_t hl = LO(index, pos);
	alu_cp(gb, gb->cpu.a, 0x80);
	gb->cpu.c = rd8(gb, hl);
	bool negative = !(gb->cpu.f & FLAG_C);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.c);
	wr8(gb, hl++, gb->cpu.a);
	SET_HL(gb, hl);
	bool carry = gb->cpu.f & FLAG_C;
	if (!negative) {
		if (!carry)
			return;
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	} else {
		if (carry)
			return;
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	}
}

PORT(Func_4cf70) { add_offset(gb, wCurObjVar3, wHiddenFigureSCYShake); }
PORT(Func_4cf87) { add_offset(gb, wCurObjVar2, wc08b); }
PORT(Func_4cf9e) { add_pos_offset(gb, wCurObjVar3, OBJ_Y_POS); }
PORT(Func_4cfbf) { add_pos_offset(gb, wCurObjVar2, OBJ_X_POS); }

#define INC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_inc(gb, (uint8_t)h_)); }) // inc l (sets flags)
#define DEC_L(hl) ({ uint16_t h_ = (hl); LO(h_, alu_dec(gb, (uint8_t)h_)); }) // dec l (sets flags)

#define HF(off) (ADDR_HiddenFigureFunc + (off))
#define HF_FUNC_4CB9E HF(0x4d)
#define HF_FUNC_4CBC4 HF(0x73)
#define HF_JUMPTABLE (HF(0x73) + 0x0e) // after .Func_4cbc4's rst JumpTable
#define HF_FUNC_4CDBC HF(0x26b)
#define HF_FUNC_4CE01 HF(0x2b0)

// ld a, no_actions_for 1 / ld [wCurObjAction], a (after a 2-frame frame
// duration)
static void hf_hold(GB *gb)
{
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjFrameDuration, 0x02);
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
}

// ld hl, wCurObjStateDuration / dec [hl]; returns whether it ran out
static bool hf_count_down(GB *gb)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	return !d;
}

// ld a, dur / ld [hli], a / ld a, v / ld [hl], a (hl = OBJ_STATE_DURATION)
static void hf_next(GB *gb, uint8_t dur, uint8_t v)
{
	uint16_t hl = wCurObjStateDuration;
	wr8(gb, hl++, dur);
	gb->cpu.a = v;
	wr8(gb, hl, v);
	SET_HL(gb, hl);
}

PORT_JUMP(HiddenFigureFunc)
{
	gb->cpu.a = TRUE;
	wr8(gb, wIsBossBattle, TRUE);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	uint16_t hl = w1d14b;
	for (int i = 0; i < 7; i++)
		wr8(gb, hl++, 0);
	hl = w1d15a;
	wr8(gb, hl++, 0x04);
	for (int i = 0; i < 7; i++)
		wr8(gb, hl++, 0);
	gb->cpu.a = 0x02;
	wr8(gb, hl++, 0x02);
	hl = wCurObjFlags;
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | OBJFLAG_NO_COLLISION));
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(HF_FUNC_4CB9E >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)HF_FUNC_4CB9E;
	wr8(gb, hl--, gb->cpu.a);
	hl = LO(hl, OBJ_SUBSTATE);
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED));
	SET_HL(gb, hl);
	SET_DE(gb, ADDR_Frameset_6a06c);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 20;
	wr8(gb, hl++, 20);
	SET_HL(gb, hl);
	SET_BC(gb, ADDR_ObjParams_HiddenFigureFace);
	CALL(CreateObjectAtRelativePos);
	SET_BC(gb, ADDR_ObjParams_HiddenFigureLeftHand);
	CALL(CreateObjectAtRelativePos);
	SET_BC(gb, ADDR_ObjParams_HiddenFigureRightHand);
	JUMP(CreateObjectAtRelativePos);
}

PORT_LOCAL(HiddenFigureFunc, Func_4cb9e)
{
	gb->cpu.a = 0x02;
	wr8(gb, wCurObjFrameDuration, 0x02);
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
	if (!hf_count_down(gb))
		return;
	hf_next(gb, 0x1e, 0x00);
	uint16_t hl = LO(REG_HL(gb), OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(HF_FUNC_4CBC4 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)HF_FUNC_4CBC4;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = BOSS_HIDDEN_FIGURE;
	wr8(gb, wBossState, gb->cpu.a);
	CALL(UpdateLevelMusic);
}

// rst JumpTable on OBJ_VAR_1
PORT_LOCAL_JUMP(HiddenFigureFunc, Func_4cbc4)
{
	hf_hold(gb);
	gb->cpu.a = rd8(gb, wCurObjVar1);
	gb_push16(gb, HF_JUMPTABLE); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	JUMP_TO(REG_HL(gb));
}

PORT_LOCAL(HiddenFigureFunc, Func_4cbe8)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wAutoMoveState, 0);
}

PORT_LOCAL(HiddenFigureFunc, Func_4cbf5)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, w1d15e);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		uint8_t d = alu_dec(gb, rd8(gb, hl));
		wr8(gb, hl, d);
		if (d)
			return;
	}
	// .asm_4cc00
	hl = INC_L(hl);
	gb->cpu.a = 0x01;
	wr8(gb, hl++, 0x01);
	SET_HL(gb, hl);
}

// Picks the next attack.
PORT_LOCAL(HiddenFigureFunc, Func_4cc05)
{
	uint16_t hl = wCurObjStateDuration;
	wr8(gb, hl++, 0x01);
	gb->cpu.a = rd8(gb, w1d160);
	alu_cp(gb, gb->cpu.a, 0x02);
	bool reset = !(gb->cpu.f & FLAG_C);
	if (!reset) {
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = rd8(gb, w1d15e);
		reset = alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
	}
	if (reset) {
		// .asm_4cc19
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, w1d15e, 0);
		wr8(gb, w1d160, 0);
		gb->cpu.b = 0x06;
	} else {
		// .asm_4cc24
		gb->cpu.a = rd8(gb, wWarioScreenXPos);
		alu_cp(gb, gb->cpu.a, 0x58);
		gb->cpu.b = (gb->cpu.f & FLAG_C) ? 0x02 : 0x04;
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = gb->cpu.e;
		wr8(gb, w1d160, gb->cpu.a);
	}
	// .asm_4cc36
	wr8(gb, hl, gb->cpu.b);
	hl = w1d161;
	SET_HL(gb, hl);
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
}

PORT_LOCAL(HiddenFigureFunc, Func_4cc3c)
{
	gb->cpu.a = rd8(gb, w1d15c);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	gb->cpu.a = rd8(gb, w1d15b);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	if (!hf_count_down(gb))
		return;
	uint16_t hl = wCurObjStateDuration;
	wr8(gb, hl++, 0x80);
	wr8(gb, hl++, 0x07);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
	gb->cpu.b = 0x03;
	gb->cpu.a = rd8(gb, w1d15a);
	alu_cp(gb, gb->cpu.a, 0x04);
	if (!(gb->cpu.f & FLAG_Z)) {
		bool random = true;
		alu_cp(gb, gb->cpu.a, 0x03);
		if (gb->cpu.f & FLAG_C) {
			gb->cpu.a = rd8(gb, wWarioScreenXPos);
			alu_cp(gb, gb->cpu.a, 0x30);
			if (!(gb->cpu.f & FLAG_C)) {
				alu_cp(gb, gb->cpu.a, 0x80);
				if (gb->cpu.f & FLAG_C) {
					gb->cpu.a = alu_rra(gb, rd8(gb, w1d161));
					if (!(gb->cpu.f & FLAG_C)) {
						gb->cpu.a = 0x60;
						wr8(gb, wCurObjStateDuration, 0x60);
						gb->cpu.b = 0x08;
						random = false;
					}
				}
			}
		}
		if (random) {
			// .asm_4cc7b
			gb->cpu.a = alu_rra(gb, rd8(gb, rDIV));
			if (!(gb->cpu.f & FLAG_C))
				gb->cpu.b = 0x07;
		}
	}
	// .asm_4cc82
	gb->cpu.a = gb->cpu.b;
	wr8(gb, w1d14b, gb->cpu.a);
}

PORT_LOCAL(HiddenFigureFunc, Func_4cc87)
{
	uint16_t hl = w1d14e;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		alu_cp(gb, gb->cpu.a, 0x80);
		if (!(gb->cpu.f & FLAG_Z)) {
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
			return;
		}
		hf_next(gb, 0xa0, 0x08);
		return;
	}
	// .asm_4cc9e
	gb->cpu.a = rd8(gb, w1d14b);
	alu_cp(gb, gb->cpu.a, 0x07);
	if (gb->cpu.f & FLAG_Z) {
		SET_BC(gb, ADDR_Data_4c580);
	} else {
		alu_cp(gb, gb->cpu.a, 0x08);
		SET_BC(gb, (gb->cpu.f & FLAG_Z) ? ADDR_Data_4c680 : ADDR_Data_4c180);
	}
	gb_call(gb, ADDR_Func_4cf70);
	if (!hf_count_down(gb))
		return;
	hf_next(gb, 0x1e, 0x00);
	gb->cpu.a = 0x00;
	wr8(gb, w1d14b, 0x00);
	gb->cpu.a = alu_dec(gb, rd8(gb, w1d15a));
	if (gb->cpu.a)
		return;
	hl = w1d162;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d) {
		// .asm_4ccde
		gb->cpu.a = 0x02;
		wr8(gb, hl, 0x02);
		return;
	}
	gb->cpu.a = 0x06;
	wr8(gb, wCurObjVar1, 0x06);
	hl = w1d161;
	SET_HL(gb, hl);
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
}

// .Func_4cce2 / .Func_4cd45: starts a hand attack.
static void hf_attack(GB *gb, uint8_t next, uint8_t attack)
{
	if (!hf_count_down(gb))
		return;
	uint16_t hl = wCurObjStateDuration;
	wr8(gb, hl++, 0x80);
	wr8(gb, hl++, next);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
	gb->cpu.a = 0x01;
	gb->cpu.a = attack;
	wr8(gb, w1d14b, attack);
}

PORT_LOCAL(HiddenFigureFunc, Func_4cce2) { hf_attack(gb, 0x03, 0x01); }
PORT_LOCAL(HiddenFigureFunc, Func_4cd45) { hf_attack(gb, 0x05, 0x02); }

// .Func_4ccf6 / .Func_4cd59: the hand attack itself.
static void hf_hand(GB *gb, uint16_t data, uint16_t hand_flag, uint8_t var1, uint8_t attack)
{
	SET_BC(gb, ADDR_Data_4c000);
	gb_call(gb, ADDR_Func_4cf70);
	SET_BC(gb, data);
	gb_call(gb, ADDR_Func_4cf87);
	if (!hf_count_down(gb))
		return;
	gb->cpu.a = 0x00;
	wr8(gb, w1d14b, 0x00);
	gb->cpu.a = rd8(gb, hand_flag);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		hf_next(gb, 0x1e, 0x00);
		return;
	}
	hf_next(gb, 0x04, var1);
	gb->cpu.a = attack;
	wr8(gb, w1d14b, attack);
}

PORT_LOCAL(HiddenFigureFunc, Func_4ccf6) { hf_hand(gb, ADDR_Data_4c080, w1d14c, 0x09, 0x04); }
PORT_LOCAL(HiddenFigureFunc, Func_4cd59) { hf_hand(gb, ADDR_Data_4c100, w1d14d, 0x0a, 0x05); }

// .Func_4cd28 / .Func_4cd8b
static void hf_wait_hand(GB *gb, uint16_t hand_flag)
{
	gb->cpu.a = rd8(gb, w1d15a);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// .asm_4cda8: defeated
		uint16_t hl = wCurObjUpdateFunction + 1;
		gb->cpu.a = (uint8_t)(HF_FUNC_4CDBC >> 8);
		wr8(gb, hl--, gb->cpu.a);
		gb->cpu.a = (uint8_t)HF_FUNC_4CDBC;
		wr8(gb, hl--, gb->cpu.a);
		SET_HL(gb, hl);
		gb->cpu.a = 0x82;
		wr8(gb, wCurObjStateDuration, 0x82);
		gb->cpu.a = 0x06;
		wr8(gb, w1d14b, 0x06);
		return;
	}
	gb->cpu.a = rd8(gb, hand_flag);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	if (!hf_count_down(gb))
		return;
	hf_next(gb, 0x1e, 0x06);
	gb->cpu.a = 0x00;
	wr8(gb, w1d14b, 0x00);
}

PORT_LOCAL(HiddenFigureFunc, Func_4cd28) { hf_wait_hand(gb, w1d14c); }
PORT_LOCAL(HiddenFigureFunc, Func_4cd8b) { hf_wait_hand(gb, w1d14d); }

PORT_LOCAL(HiddenFigureFunc, Func_4cdbc)
{
	hf_hold(gb);
	gb_call(gb, ADDR_Func_4cfe0);
	gb->cpu.a = rd8(gb, w1d14c);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	gb->cpu.a = rd8(gb, w1d14d);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		// .asm_4cdf4
		gb->cpu.a = alu_and(gb, rd8(gb, hl), 0x3f);
		if (gb->cpu.a)
			return;
		play_sfx(gb, SFX_0CE);
		return;
	}
	gb->cpu.a = 0x82;
	wr8(gb, hl++, 0x82);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(HF_FUNC_4CE01 >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)HF_FUNC_4CE01;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	gb->cpu.a = 0x01;
	wr8(gb, w1d15f, 0x01);
	play_sfx(gb, SFX_0CF);
}

// Crumbles, then ends the level.
PORT_LOCAL_JUMP(HiddenFigureFunc, Func_4ce01)
{
	hf_hold(gb);
	uint16_t hl = wCurObjVar1;
	SET_HL(gb, hl);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), 0x07);
	if (!gb->cpu.a)
		play_sfx(gb, SFX_01F);
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	hl = DEC_L(hl);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (!d) {
		// .asm_4ce31
		gb->cpu.a = LVLEND_EPILOGUE;
		wr8(gb, wLevelEndScreen, gb->cpu.a);
		JUMP_TO(ADDR_Func_4d005);
		return;
	}
	gb_call(gb, ADDR_Func_4cfe0);
	gb->cpu.a = alu_and(gb, rd8(gb, wCurObjStateDuration), 0x03);
	if (gb->cpu.a) {
		RET();
		return;
	}
	hl = wHiddenFigureSCYShake;
	SET_HL(gb, hl);
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	RET();
}
