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

// --- Hidden Figure's left hand ---

#define LH(off) ((uint16_t)(ADDR_HiddenFigureLeftHandFunc + (off)))
#define LH_FUNC_4D0C7 LH(0x1c)
#define LH_FUNC_4D164 LH(0xb9)
#define LH_FUNC_4D19E LH(0xf3)
#define LH_FUNC_4D1B1 LH(0x106)
#define LH_FUNC_4D2C8 LH(0x21d)
#define LH_FUNC_4D379 LH(0x2ce)
#define LH_FUNC_4D37F LH(0x2d4)
#define LH_FUNC_4D39B LH(0x2f0)
#define RH_FUNC_4D655 ((uint16_t)(ADDR_HiddenFigureRightHandFunc + 0x2a9))

static inline void hf_set_bit(GB *gb, uint16_t hl, int bit)
{
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) | (1 << bit)));
}

static inline void hf_res_bit(GB *gb, uint16_t hl, int bit)
{
	wr8(gb, hl, (uint8_t)(rd8(gb, hl) & ~(1 << bit)));
}

// ld l, OBJ_UPDATE_FUNCTION + 1 / ld [hld] the update pointer
static uint16_t hf_set_update(GB *gb, uint16_t hl, uint16_t f)
{
	hl = LO(hl, OBJ_UPDATE_FUNCTION + 1);
	gb->cpu.a = (uint8_t)(f >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)f;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	return hl;
}

// ld a, [hl] / and HEAVY_OBJ / or type / ld [hl], a
static void hf_interaction(GB *gb, uint16_t hl, uint8_t type)
{
	gb->cpu.a = alu_or(gb, alu_and(gb, rd8(gb, hl), HEAVY_OBJ), type);
	wr8(gb, hl, gb->cpu.a);
}

// The interaction type and then the 4 collision box bytes, ld [hli]
static uint16_t hf_interaction_box(GB *gb, uint8_t type, const uint8_t box[4])
{
	uint16_t hl = wCurObjInteractionType;
	hf_interaction(gb, hl, type);
	hl++;
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = box[i];
		wr8(gb, hl++, gb->cpu.a);
	}
	SET_HL(gb, hl);
	return hl;
}

static void hf_no_actions(GB *gb)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
}

// .asm_4d34c: back to the resting state (.Func_4d0c7)
static void lh_rest(GB *gb)
{
	static const uint8_t box[4] = {0xe8, 0xf8, 0xf8, 0x07};
	hf_set_update(gb, wCurObjUpdateFunction + 1, LH_FUNC_4D0C7);
	uint16_t hl = hf_interaction_box(gb, OBJ_INTERACTION_0B, box);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjState, 0);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	hf_res_bit(gb, hl, OBJFLAG_NO_COLLISION_F);
	hf_res_bit(gb, hl, 4);
	SET_DE(gb, ADDR_Frameset_6a128);
	JUMP(SetObjectFramesetPtr);
}

PORT(HiddenFigureLeftHandFunc)
{
	uint16_t hl = hf_set_update(gb, wCurObjUpdateFunction + 1, LH_FUNC_4D0C7);
	hl = LO(hl, OBJ_Y_POS);
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, (uint16_t)(w1d152 + i), gb->cpu.a);
	}
	SET_HL(gb, hl);
}

PORT_LOCAL_JUMP(HiddenFigureLeftHandFunc, Func_4d0c7)
{
	hf_no_actions(gb);
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	hf_res_bit(gb, hl, OBJFLAG_NO_COLLISION_F);
	uint16_t update, frameset = ADDR_Frameset_6a0d4;
	gb->cpu.a = rd8(gb, w1d14b);
	switch (gb->cpu.a) {
	case 0x01:
		gb->cpu.a = 0x01;
		wr8(gb, w1d151, 0x01);
		update = LH_FUNC_4D2C8;
		frameset = ADDR_Frameset_6a13a;
		break;
	case 0x03:
	case 0x07:
	case 0x08: {
		static const uint8_t box[4] = {0xe0, 0x00, 0xf4, 0x00};
		hf_interaction_box(gb, OBJ_INTERACTION_4F, box);
		update = LH_FUNC_4D1B1;
		frameset = ADDR_Frameset_6a0bc;
		break;
	}
	case 0x05:
		hl = wCurObjInteractionType;
		hf_interaction(gb, hl, OBJ_INTERACTION_FULL_STING);
		SET_HL(gb, hl + 1);
		gb->cpu.a = 0x01;
		wr8(gb, w1d15c, 0x01);
		gb->cpu.a = alu_dec(gb, rd8(gb, w1d15a));
		gb->cpu.a = (gb->cpu.f & FLAG_Z) ? 0x1e : 0x3c;
		wr8(gb, wCurObjStateDuration, gb->cpu.a);
		update = LH_FUNC_4D164;
		break;
	case 0x06:
		hf_set_bit(gb, hl, 3);
		update = LH_FUNC_4D37F;
		break;
	default:
		gb->cpu.a = rd8(gb, w1d151);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z) {
			RET();
			return;
		}
		update = LH_FUNC_4D19E;
		break;
	}
	SET_BC(gb, update);
	SET_DE(gb, frameset);
	// .asm_4d154
	hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.c);
	hl = LO(hl, OBJ_VAR_2);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, hl++, 0);
	hl = LO(hl, alu_inc(gb, (uint8_t)hl));
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(HiddenFigureLeftHandFunc, Func_4d164)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		gb->cpu.a = rd8(gb, w1d15a);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z)) {
			gb->cpu.a = rd8(gb, w1d14b);
			alu_cp(gb, gb->cpu.a, 0x00);
			if (!(gb->cpu.f & FLAG_Z)) {
				RET();
				return;
			}
		}
	} else {
		SET_BC(gb, ADDR_Data_4c540);
		gb_call(gb, ADDR_Func_4cf9e);
		gb->cpu.a = rd8(gb, wCurObjVar3);
		alu_cp(gb, gb->cpu.a, 0x0a);
		if (gb->cpu.f & FLAG_Z) {
			play_sfx(gb, SFX_063);
			gb->cpu.b = 0x18;
			JUMP(DoGroundShake);
			return;
		}
		alu_cp(gb, gb->cpu.a, 0x3f);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w1d15c, 0);
	lh_rest(gb);
}

PORT_LOCAL_JUMP(HiddenFigureLeftHandFunc, Func_4d19e)
{
	SET_BC(gb, ADDR_Data_4c500);
	gb_call(gb, ADDR_Func_4cf9e);
	gb->cpu.a = rd8(gb, wCurObjVar3);
	alu_cp(gb, gb->cpu.a, 0x3a);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w1d151, 0);
	lh_rest(gb);
}

// a = [wCurObjVar2]: at $13 the frameset changes to $6a0ef, at hit_val
// the hand hits (interaction $4f), at hit_val - 1 it lands (interaction
// $50 and a sound), and at end_val it goes back to resting.
static void lh_slam_step(GB *gb, uint8_t hit_val, uint8_t end_val)
{
	gb->cpu.a = rd8(gb, wCurObjVar2);
	alu_cp(gb, gb->cpu.a, 0x13);
	if (gb->cpu.f & FLAG_Z) {
		SET_DE(gb, ADDR_Frameset_6a0ef);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	alu_cp(gb, gb->cpu.a, hit_val);
	if (gb->cpu.f & FLAG_Z) {
		uint16_t hl = wCurObjInteractionType;
		hf_interaction(gb, hl, OBJ_INTERACTION_4F);
		SET_HL(gb, hl - 1);
		SET_DE(gb, ADDR_Frameset_6a0c3);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	alu_cp(gb, gb->cpu.a, (uint8_t)(hit_val - 1));
	if (gb->cpu.f & FLAG_Z) {
		uint16_t hl = wCurObjInteractionType;
		hf_interaction(gb, hl, OBJ_INTERACTION_50);
		SET_HL(gb, hl - 1);
		play_sfx(gb, SFX_FAT_WALK);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, end_val);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	lh_rest(gb);
}

PORT_LOCAL_JUMP(HiddenFigureLeftHandFunc, Func_4d1b1)
{
	uint16_t hl = w1d14f;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		alu_cp(gb, gb->cpu.a, 0x80);
		if (!(gb->cpu.f & FLAG_Z)) {
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
			RET();
			return;
		}
		// grabbed Wario
		hl = hf_set_update(gb, wCurObjUpdateFunction + 1, LH_FUNC_4D379);
		hl = LO(hl, OBJ_Y_POS);
		gb->cpu.a = alu_sub(gb, rd8(gb, hl++), 0x01);
		wr8(gb, wWarioYPos + 1, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, hl++), 0x00);
		wr8(gb, wWarioYPos + 0, gb->cpu.a);
		gb->cpu.a = alu_add(gb, rd8(gb, hl++), 0x06);
		wr8(gb, wWarioXPos + 1, gb->cpu.a);
		gb->cpu.a = alu_adc(gb, rd8(gb, hl++), 0x00);
		wr8(gb, wWarioXPos + 0, gb->cpu.a);
		hl = LO(hl, OBJ_FLAGS);
		SET_HL(gb, hl);
		hf_set_bit(gb, hl, OBJFLAG_PRIORITY_F);
		SET_DE(gb, ADDR_Frameset_6a0f2);
		CALL(SetObjectFramesetPtr);
		// stop_music2
		gb->cpu.a = 0xff;
		wr8(gb, hMusicIDBackup + 0, 0xff);
		wr8(gb, hMusicID + 0, 0xff);
		gb->cpu.a = 0x00;
		wr8(gb, hMusicIDBackup + 1, 0x00);
		wr8(gb, hMusicID + 1, 0x00);
		RET();
		return;
	}

	hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		bool hit = false;
		alu_cp(gb, gb->cpu.a, 0x2f);
		if (gb->cpu.f & FLAG_Z) {
			play_sfx(gb, SFX_0CD);
			gb->cpu.a = 0x80;
			hit = true;
		} else {
			gb->cpu.a = alu_and(gb, gb->cpu.a, 0xfe);
			alu_cp(gb, gb->cpu.a, 0x04);
			if (gb->cpu.f & FLAG_Z) {
				gb->cpu.a = 0x0a;
				hit = true;
			}
		}
		if (hit) {
			wr8(gb, w1d14e, gb->cpu.a);
			wr8(gb, w1d14f, gb->cpu.a);
			wr8(gb, w1d150, gb->cpu.a);
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, hl, 0);
			RET();
			return;
		}
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
	}

	gb->cpu.a = rd8(gb, w1d14b);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (gb->cpu.f & FLAG_Z) {
		SET_BC(gb, ADDR_Data_4c700);
		gb_call(gb, ADDR_Func_4cfbf);
		SET_BC(gb, ADDR_Data_4c800);
		gb_call(gb, ADDR_Func_4cf9e);
		lh_slam_step(gb, 0x4e, 0x5f);
		return;
	}
	SET_BC(gb, ADDR_Data_4c380);
	gb_call(gb, ADDR_Func_4cfbf);
	gb->cpu.a = rd8(gb, w1d14b);
	alu_cp(gb, gb->cpu.a, 0x07);
	SET_BC(gb, (gb->cpu.f & FLAG_Z) ? ADDR_Data_4c600 : ADDR_Data_4c400);
	gb_call(gb, ADDR_Func_4cf9e);
	lh_slam_step(gb, 0x62, 0x7f);
}

PORT_LOCAL_JUMP(HiddenFigureLeftHandFunc, Func_4d2c8)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		alu_cp(gb, gb->cpu.a, 0x0a);
		if (gb->cpu.f & FLAG_Z) {
			static const uint8_t box[4] = {0xec, 0xfe, 0xf8, 0x07};
			gb->cpu.a = 0x2e;
			wr8(gb, hl, 0x2e);
			hl = LO(hl, OBJ_FLAGS);
			hf_set_bit(gb, hl, OBJFLAG_PRIORITY_F);
			hf_set_update(gb, hl, RH_FUNC_4D655);
			hf_interaction_box(gb, OBJ_INTERACTION_00, box);
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wCurObjVar3, 0);
			gb->cpu.a = alu_inc(gb, gb->cpu.a);
			wr8(gb, w1d14c, gb->cpu.a);
			wr8(gb, wCurObjStateDuration, gb->cpu.a);
			// stop_sfx
			gb->cpu.a = 0xff;
			wr8(gb, hSFXID + 0, 0xff);
			gb->cpu.a = 0x00;
			wr8(gb, hSFXID + 1, 0x00);
			SET_DE(gb, ADDR_Frameset_6a0b2);
			JUMP(SetObjectFramesetPtr);
			return;
		}
		gb->cpu.a = 0x01;
		wr8(gb, w1d15e, 0x01);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
	}
	SET_BC(gb, ADDR_Data_4c200);
	gb_call(gb, ADDR_Func_4cfbf);
	SET_BC(gb, ADDR_Data_4c280);
	gb_call(gb, ADDR_Func_4cf9e);
	gb->cpu.a = rd8(gb, wCurObjVar2);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (gb->cpu.f & FLAG_Z) {
		hl = wCurObjInteractionType;
		hf_interaction(gb, hl, OBJ_INTERACTION_48);
		SET_HL(gb, hl - 1);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x3c);
	if (gb->cpu.f & FLAG_Z) {
		hl = wCurObjInteractionType;
		hf_interaction(gb, hl, OBJ_INTERACTION_52);
		SET_HL(gb, hl - 1);
		play_sfx(gb, SFX_0CB);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x7f);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	lh_rest(gb);
}

PORT_LOCAL(HiddenFigureLeftHandFunc, Func_4d379)
{
	hf_no_actions(gb);
}

PORT_LOCAL(HiddenFigureLeftHandFunc, Func_4d37f)
{
	hf_no_actions(gb);
	gb->cpu.a = rd8(gb, w1d15f);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	SET_DE(gb, ADDR_Frameset_6a0bc);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 0x20;
	wr8(gb, hl++, 0x20);
	hf_set_update(gb, hl, LH_FUNC_4D39B);
}

PORT_LOCAL_JUMP(HiddenFigureLeftHandFunc, Func_4d39b)
{
	hf_no_actions(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	gb->cpu.a = alu_rra(gb, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_C)) {
		JUMP(MoveObjectUp);
		return;
	}
	RET();
}

// --- Hidden Figure's palettes ---

PORT_JUMP(Func_4cfe0)
{
	uint16_t hl;
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x0f);
	if (gb->cpu.a) {
		alu_cp(gb, gb->cpu.a, 0x08);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		hl = ADDR_Pals_4d04b;
	} else {
		gb->cpu.a = rd8(gb, w1d15a);
		alu_cp(gb, gb->cpu.a, 0x02);
		hl = (gb->cpu.f & FLAG_C) ? ADDR_Pals_4d01b : ADDR_Pals_4d07b;
	}
	SET_HL(gb, hl);
	gb->cpu.c = 0;
	gb->cpu.b = 6;
	JUMP(SetBGPals);
}

PORT_JUMP(Func_4d005)
{
	gb->cpu.a = rd8(gb, w1d15a);
	alu_cp(gb, gb->cpu.a, 0x02);
	SET_HL(gb, (gb->cpu.f & FLAG_C) ? ADDR_Pals_4d01b : ADDR_Pals_4d07b);
	gb->cpu.c = 0;
	gb->cpu.b = 6;
	JUMP(SetBGPals);
}

// --- Hidden Figure's right hand ---

#define RH(addr) ((uint16_t)(ADDR_HiddenFigureRightHandFunc + ((addr) - 0x4d3ac)))
#define RH_FUNC_4D3C8 RH(0x4d3c8)
#define RH_FUNC_4D465 RH(0x4d465)
#define RH_FUNC_4D49F RH(0x4d49f)
#define RH_FUNC_4D4B2 RH(0x4d4b2)
#define RH_FUNC_4D571 RH(0x4d571)
#define RH_FUNC_4D622 RH(0x4d622)
#define RH_FUNC_4D628 RH(0x4d628)
#define RH_FUNC_4D644 RH(0x4d644)
#define RH_JUMPTABLE RH(0x4d67e) // after .asm_4d67c's rst JumpTable

// .asm_4d5f5: back to the resting state (.Func_4d3c8)
static void rh_rest(GB *gb)
{
	static const uint8_t box[4] = {0xe8, 0xf8, 0xf8, 0x07};
	hf_set_update(gb, wCurObjUpdateFunction + 1, RH_FUNC_4D3C8);
	uint16_t hl = hf_interaction_box(gb, OBJ_INTERACTION_0B, box);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjState, 0);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	hf_res_bit(gb, hl, OBJFLAG_NO_COLLISION_F);
	hf_res_bit(gb, hl, 4);
	SET_DE(gb, ADDR_Frameset_6a131);
	JUMP(SetObjectFramesetPtr);
}

PORT(HiddenFigureRightHandFunc)
{
	uint16_t hl = hf_set_update(gb, wCurObjUpdateFunction + 1, RH_FUNC_4D3C8);
	hl = LO(hl, OBJ_Y_POS);
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, (uint16_t)(w1d156 + i), gb->cpu.a);
	}
	SET_HL(gb, hl);
}

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d3c8)
{
	hf_no_actions(gb);
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	hf_res_bit(gb, hl, OBJFLAG_NO_COLLISION_F);
	uint16_t update, frameset = ADDR_Frameset_6a0fe;
	gb->cpu.a = rd8(gb, w1d14b);
	switch (gb->cpu.a) {
	case 0x02:
		gb->cpu.a = 0x01;
		wr8(gb, w1d151, 0x01);
		update = RH_FUNC_4D571;
		frameset = ADDR_Frameset_6a145;
		break;
	case 0x03:
	case 0x07:
	case 0x08: {
		static const uint8_t box[4] = {0xe0, 0x00, 0xff, 0x0b};
		hf_interaction_box(gb, OBJ_INTERACTION_4F, box);
		update = RH_FUNC_4D4B2;
		frameset = ADDR_Frameset_6a0c8;
		break;
	}
	case 0x04:
		hl = wCurObjInteractionType;
		hf_interaction(gb, hl, OBJ_INTERACTION_FULL_STING);
		SET_HL(gb, hl + 1);
		gb->cpu.a = 0x01;
		wr8(gb, w1d15b, 0x01);
		gb->cpu.a = alu_dec(gb, rd8(gb, w1d15a));
		gb->cpu.a = (gb->cpu.f & FLAG_Z) ? 0x1e : 0x3c;
		wr8(gb, wCurObjStateDuration, gb->cpu.a);
		update = RH_FUNC_4D465;
		break;
	case 0x06:
		hf_set_bit(gb, hl, 3);
		update = RH_FUNC_4D628;
		break;
	default:
		gb->cpu.a = rd8(gb, w1d151);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z) {
			RET();
			return;
		}
		update = RH_FUNC_4D49F;
		break;
	}
	SET_BC(gb, update);
	SET_DE(gb, frameset);
	// .asm_4d455
	hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl--, gb->cpu.a);
	wr8(gb, hl, gb->cpu.c);
	hl = LO(hl, OBJ_VAR_2);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	wr8(gb, hl, 0); // ld [hl], a (not hli)
	hl = LO(hl, alu_inc(gb, (uint8_t)hl));
	wr8(gb, hl, 0);
	SET_HL(gb, hl);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d465)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
		gb->cpu.a = rd8(gb, w1d15a);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z)) {
			gb->cpu.a = rd8(gb, w1d14b);
			alu_cp(gb, gb->cpu.a, 0x00);
			if (!(gb->cpu.f & FLAG_Z)) {
				RET();
				return;
			}
		}
	} else {
		SET_BC(gb, ADDR_Data_4c540);
		gb_call(gb, ADDR_Func_4cf9e);
		gb->cpu.a = rd8(gb, wCurObjVar3);
		alu_cp(gb, gb->cpu.a, 0x0a);
		if (gb->cpu.f & FLAG_Z) {
			play_sfx(gb, SFX_063);
			gb->cpu.b = 0x18;
			JUMP(DoGroundShake);
			return;
		}
		alu_cp(gb, gb->cpu.a, 0x3f);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w1d15b, 0);
	rh_rest(gb);
}

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d49f)
{
	SET_BC(gb, ADDR_Data_4c500);
	gb_call(gb, ADDR_Func_4cf9e);
	gb->cpu.a = rd8(gb, wCurObjVar3);
	alu_cp(gb, gb->cpu.a, 0x3a);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w1d151, 0);
	rh_rest(gb);
}

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d4b2)
{
	uint16_t hl = w1d150;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		alu_cp(gb, gb->cpu.a, 0x80);
		if (!(gb->cpu.f & FLAG_Z)) {
			wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
			RET();
			return;
		}
		// .asm_4d4bf
		hl = hf_set_update(gb, wCurObjUpdateFunction + 1, RH_FUNC_4D622);
		hl = LO(hl, OBJ_FLAGS);
		SET_HL(gb, hl);
		hf_set_bit(gb, hl, OBJFLAG_PRIORITY_F);
		SET_DE(gb, ADDR_Frameset_6a11c);
		JUMP(SetObjectFramesetPtr);
		return;
	}

	// .asm_4d4d2
	hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0xfe);
		alu_cp(gb, gb->cpu.a, 0x04);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = 0x0a;
			wr8(gb, w1d14e, gb->cpu.a);
			wr8(gb, w1d14f, gb->cpu.a);
			wr8(gb, w1d150, gb->cpu.a);
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, hl, 0);
			RET();
			return;
		}
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
	}

	// .asm_4d4ef
	gb->cpu.a = rd8(gb, w1d14b);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_4d540
		SET_BC(gb, ADDR_Data_4c760);
		gb_call(gb, ADDR_Func_4cfbf);
		SET_BC(gb, ADDR_Data_4c800);
		gb_call(gb, ADDR_Func_4cf9e);
		gb->cpu.a = rd8(gb, wCurObjVar2);
		alu_cp(gb, gb->cpu.a, 0x13);
		if (gb->cpu.f & FLAG_Z) {
			play_sfx(gb, SFX_0D0);
			SET_DE(gb, ADDR_Frameset_6a119);
			JUMP(SetObjectFramesetPtr);
			return;
		}
		alu_cp(gb, gb->cpu.a, 0x4e);
		if (gb->cpu.f & FLAG_Z) {
			SET_DE(gb, ADDR_Frameset_6a0cf);
			JUMP(SetObjectFramesetPtr);
			return;
		}
		alu_cp(gb, gb->cpu.a, 0x5f);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		rh_rest(gb);
		return;
	}
	SET_BC(gb, ADDR_Data_4c480);
	gb_call(gb, ADDR_Func_4cfbf);
	gb->cpu.a = rd8(gb, w1d14b);
	alu_cp(gb, gb->cpu.a, 0x07);
	SET_BC(gb, (gb->cpu.f & FLAG_Z) ? ADDR_Data_4c600 : ADDR_Data_4c400);
	gb_call(gb, ADDR_Func_4cf9e);
	gb->cpu.a = rd8(gb, wCurObjVar2);
	alu_cp(gb, gb->cpu.a, 0x13);
	if (gb->cpu.f & FLAG_Z) {
		play_sfx(gb, SFX_0D1);
		SET_DE(gb, ADDR_Frameset_6a119);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x62);
	if (gb->cpu.f & FLAG_Z) {
		SET_DE(gb, ADDR_Frameset_6a0cf);
		JUMP(SetObjectFramesetPtr);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x32);
	if (gb->cpu.f & FLAG_Z) {
		play_sfx(gb, SFX_0D0);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x7f);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	rh_rest(gb);
}

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d571)
{
	uint16_t hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		alu_cp(gb, gb->cpu.a, 0x0a);
		if (gb->cpu.f & FLAG_Z) {
			static const uint8_t box[4] = {0xec, 0xfe, 0xf8, 0x07};
			gb->cpu.a = 0x2e;
			wr8(gb, hl, 0x2e);
			hl = LO(hl, OBJ_FLAGS);
			hf_set_bit(gb, hl, OBJFLAG_PRIORITY_F);
			hf_set_update(gb, hl, RH_FUNC_4D655);
			hf_interaction_box(gb, OBJ_INTERACTION_00, box);
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wCurObjVar3, 0);
			gb->cpu.a = alu_inc(gb, gb->cpu.a);
			wr8(gb, w1d14d, gb->cpu.a);
			wr8(gb, wCurObjStateDuration, gb->cpu.a);
			// stop_sfx
			gb->cpu.a = 0xff;
			wr8(gb, hSFXID + 0, 0xff);
			gb->cpu.a = 0x00;
			wr8(gb, hSFXID + 1, 0x00);
			SET_DE(gb, ADDR_Frameset_6a0b7);
			JUMP(SetObjectFramesetPtr);
			return;
		}
		gb->cpu.a = 0x01;
		wr8(gb, w1d15e, 0x01);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
	}
	SET_BC(gb, ADDR_Data_4c300);
	gb_call(gb, ADDR_Func_4cfbf);
	SET_BC(gb, ADDR_Data_4c280);
	gb_call(gb, ADDR_Func_4cf9e);
	gb->cpu.a = rd8(gb, wCurObjVar2);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (gb->cpu.f & FLAG_Z) {
		hl = wCurObjInteractionType;
		hf_interaction(gb, hl, OBJ_INTERACTION_48);
		SET_HL(gb, hl - 1);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x3c);
	if (gb->cpu.f & FLAG_Z) {
		hl = wCurObjInteractionType;
		hf_interaction(gb, hl, OBJ_INTERACTION_52);
		SET_HL(gb, hl - 1);
		play_sfx(gb, SFX_0CB);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x7f);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	rh_rest(gb);
}

PORT_LOCAL(HiddenFigureRightHandFunc, Func_4d622)
{
	hf_no_actions(gb);
}

PORT_LOCAL(HiddenFigureRightHandFunc, Func_4d628)
{
	hf_no_actions(gb);
	gb->cpu.a = rd8(gb, w1d15f);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	SET_DE(gb, ADDR_Frameset_6a0c8);
	CALL(SetObjectFramesetPtr);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = 0x1e;
	wr8(gb, hl++, 0x1e);
	hf_set_update(gb, hl, RH_FUNC_4D644);
}

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d644)
{
	hf_no_actions(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	gb->cpu.a = alu_rra(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_C) {
		JUMP(MoveObjectDown);
		return;
	}
	RET();
}

// Both hands' update while they're stunned or carried
PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d655)
{
	SET_HL(gb, wSCYShake);
	gb->cpu.a = alu_add(gb, rd8(gb, wc08a), rd8(gb, wSCYShake));
	wr8(gb, wSCY, gb->cpu.a);
	gb->cpu.a = rd8(gb, wc08c);
	wr8(gb, wSCX, gb->cpu.a);
	gb->cpu.a = rd8(gb, wCurObjState);
	gb->cpu.e = gb->cpu.a;
	alu_cp(gb, gb->cpu.a, 0x2d);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x0f);
		if (!gb->cpu.a)
			play_sfx(gb, SFX_06A);
	}
	// .asm_4d67c
	gb->cpu.a = gb->cpu.e;
	gb_push16(gb, RH_JUMPTABLE); // rst JumpTable
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

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d73e)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		JUMP(ObjState_Fall);
		return;
	}
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	play_sfx(gb, SFX_VANISH);
	RET();
}

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d750)
{
	hf_no_actions(gb);
	gb->cpu.a = rd8(gb, wCurObjId);
	alu_cp(gb, gb->cpu.a, HIDDEN_FIGURE_RIGHT_HAND);
	bool right = gb->cpu.f & FLAG_Z;
	uint16_t hl = right ? w1d156 : w1d152;
	gb->cpu.e = rd8(gb, hl++);
	gb->cpu.d = rd8(gb, hl++);
	gb->cpu.c = rd8(gb, hl++);
	gb->cpu.b = rd8(gb, hl);
	const uint8_t target[4] = {gb->cpu.e, gb->cpu.d, gb->cpu.c, gb->cpu.b};
	hl = wCurObjYPos;
	bool same = true;
	for (int i = 0; i < 4; i++) {
		gb->cpu.a = rd8(gb, hl++);
		alu_cp(gb, gb->cpu.a, target[i]);
		if (!(gb->cpu.f & FLAG_Z)) {
			same = false;
			break;
		}
	}
	SET_HL(gb, hl);
	if (same) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		if (right) {
			wr8(gb, w1d14d, 0);
			rh_rest(gb);
		} else {
			wr8(gb, w1d14c, 0);
			lh_rest(gb);
		}
		return;
	}

	// .asm_4d7a4
	hl = LO(hl, OBJ_Y_POS);
	gb->cpu.a = rd8(gb, hl++);
	SET_HL(gb, hl);
	alu_cp(gb, gb->cpu.a, gb->cpu.e);
	bool move_up = !(gb->cpu.f & FLAG_Z);
	if (!move_up) {
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, gb->cpu.d);
		move_up = !(gb->cpu.f & FLAG_Z);
	}
	if (move_up)
		CALL(MoveObjectUp);
	// .asm_4d7b1
	hl = LO(REG_HL(gb), OBJ_X_POS + 1);
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	bool move = true;
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, gb->cpu.c);
		if (gb->cpu.f & FLAG_Z)
			move = false;
	}
	if (move) {
		if (gb->cpu.f & FLAG_C)
			CALL(MoveObjectRight);
		else
			CALL(MoveObjectLeft);
	}
	// .asm_4d7c9
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
	bool nz = gb->cpu.a != 0;
	hl = wCurObjFlags;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	if (nz)
		gb->cpu.a &= ~(1 << 4);
	else
		gb->cpu.a |= 1 << 4;
	wr8(gb, hl, gb->cpu.a);
	RET();
}

PORT_LOCAL(HiddenFigureRightHandFunc, Func_4d7dc)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = 0x30;
	wr8(gb, hl--, 0x30);
	hl = DEC_L(hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl--, 0);
	SET_HL(gb, hl);
}

// .Func_4d7fd
static void rh_wait_release(GB *gb)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	hf_no_actions(gb);
	hl = LO(hl, OBJ_FLAGS);
	hf_res_bit(gb, hl, OBJFLAG_PRIORITY_F);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w1d15d, 0);
	hl = LO(hl, OBJ_STATE);
	gb->cpu.a = 0x2d;
	wr8(gb, hl--, 0x2d);
	gb->cpu.a = rd8(gb, hl--);
	SET_HL(gb, hl);
	gb->cpu.a = alu_rlca(gb, gb->cpu.a);
	SET_DE(gb, (gb->cpu.f & FLAG_C) ? ADDR_Frameset_6a0d4 : ADDR_Frameset_6a0fe);
	JUMP(SetObjectFramesetPtr);
}

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d7e6)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = 0x31;
	wr8(gb, hl--, 0x31);
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, w1d15d);
	alu_cp(gb, gb->cpu.a, 0x01);
	if (!(gb->cpu.f & FLAG_Z))
		gb->cpu.a = 0x46;
	wr8(gb, wCurObjStateDuration, gb->cpu.a);
	hl = wCurObjFlags;
	SET_HL(gb, hl);
	hf_res_bit(gb, hl, OBJFLAG_GRABBED_F);
	rh_wait_release(gb);
}

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d7fd)
{
	rh_wait_release(gb);
}

// .Func_4d83a / .Func_4d844
static void rh_bump(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjAction;
	gb->cpu.a = (1 << NO_ACTIONS_F) | 15; // no_actions_for 15
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	hl = LO(hl, OBJ_STATE_DURATION);
	gb->cpu.a = 0x04;
	wr8(gb, hl++, 0x04);
	hl = INC_L(hl);
	gb->cpu.a = 0x02;
	wr8(gb, hl++, 0x02);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	SET_HL(gb, hl);
}

PORT_LOCAL(HiddenFigureRightHandFunc, Func_4d824)
{
	play_sfx(gb, SFX_VANISH);
	gb->cpu.a = alu_add(gb, rd8(gb, wWarioScreenXPos), 0x2a);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_add(gb, rd8(gb, wCurObjScreenXPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	rh_bump(gb, (gb->cpu.f & FLAG_C) ? 0x39 : 0x38);
}

PORT_LOCAL(HiddenFigureRightHandFunc, Func_4d83a) { rh_bump(gb, 0x38); }
PORT_LOCAL(HiddenFigureRightHandFunc, Func_4d844) { rh_bump(gb, 0x39); }

// .Func_4d859 / .Func_4d861
static void rh_set_state_var2(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	hl = LO(hl, OBJ_VAR_2);
	gb->cpu.a = 0x02;
	wr8(gb, hl++, 0x02);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);
	SET_HL(gb, hl);
}

PORT_LOCAL(HiddenFigureRightHandFunc, Func_4d859) { rh_set_state_var2(gb, 0x44); }
PORT_LOCAL(HiddenFigureRightHandFunc, Func_4d861) { rh_set_state_var2(gb, 0x45); }

// ld hl, wCurObjState / ld a, state / ld [hld], a
static void rh_set_state(GB *gb, uint8_t state)
{
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	SET_HL(gb, hl);
}

PORT_LOCAL(HiddenFigureRightHandFunc, Func_4d86f) { rh_set_state(gb, 0x46); }
PORT_LOCAL(HiddenFigureRightHandFunc, Func_4d876) { rh_set_state(gb, 0x47); }

// .Func_4d87d / .Func_4d88b
static void rh_var2_or_state(GB *gb, uint16_t target)
{
	uint16_t hl = wCurObjVar2;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		JUMP_TO(target);
		return;
	}
	gb->cpu.a = 0x0f;
	wr8(gb, wCurObjState, 0x0f);
	RET();
}

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d87d) { rh_var2_or_state(gb, ADDR_Func_32ae); }
PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d88b) { rh_var2_or_state(gb, ADDR_Func_32bd); }

// .Func_4d899 / .Func_4d8b1
static void rh_throw(GB *gb, uint8_t state, uint8_t v, uint16_t target)
{
	gb->cpu.a = 0x01;
	wr8(gb, w1d15d, 0x01);
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	hl = DEC_L(hl);
	gb->cpu.a = 0x41;
	wr8(gb, hl--, 0x41);
	gb->cpu.a = v;
	wr8(gb, hl--, v);
	gb->cpu.a = 0x01;
	wr8(gb, hl--, 0x01);
	SET_HL(gb, hl);
	JUMP_TO(target);
}

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d899) { rh_throw(gb, 0x36, 0xc0, ADDR_Func_3335); }
PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d8b1) { rh_throw(gb, 0x37, 0xe0, ADDR_Func_3344); }

// .Func_4d8c9 / .Func_4d8e1: ld a, 1 / ld [w1d15d], a, the state, set 2
static void rh_set_carry_state(GB *gb, uint8_t state)
{
	gb->cpu.a = 0x01;
	wr8(gb, w1d15d, 0x01);
	uint16_t hl = wCurObjState;
	gb->cpu.a = state;
	wr8(gb, hl--, state);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	hf_set_bit(gb, hl, 2);
}

// .Func_4d8d8 (right = false) / .Func_4d8f0 (right = true), going through
// .Func_4d8e1 / .Func_4d8c9 until wDirection matches
static void rh_carry(GB *gb, bool right)
{
	for (;;) {
		gb->cpu.a = rd8(gb, wDirection);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!right) {
			if (!(gb->cpu.f & FLAG_Z)) {
				JUMP(Func_3353);
				return;
			}
			rh_set_carry_state(gb, 0x48);
		} else {
			if (gb->cpu.f & FLAG_Z) {
				JUMP(Func_3362);
				return;
			}
			rh_set_carry_state(gb, 0x49);
		}
		right = !right;
	}
}

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d8c9)
{
	rh_set_carry_state(gb, 0x49);
	rh_carry(gb, false);
}

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d8d8) { rh_carry(gb, false); }

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d8e1)
{
	rh_set_carry_state(gb, 0x48);
	rh_carry(gb, true);
}

PORT_LOCAL_JUMP(HiddenFigureRightHandFunc, Func_4d8f0) { rh_carry(gb, true); }

// --- Hidden Figure's face ---

#define HFF(addr) ((uint16_t)(ADDR_HiddenFigureFaceFunc + ((addr) - 0x4ce47)))
#define HFF_FUNC_4CF13 HFF(0x4cf13)
#define HFF_FUNC_4CF3F HFF(0x4cf3f)

// Picks the frameset by Wario's screen x: < $38, < $78, or further right
static void hff_face_by_x(GB *gb, uint16_t left, uint16_t middle, uint16_t right)
{
	gb->cpu.a = rd8(gb, wWarioScreenXPos);
	alu_cp(gb, gb->cpu.a, 0x38);
	if (gb->cpu.f & FLAG_C) {
		SET_DE(gb, left);
	} else {
		alu_cp(gb, gb->cpu.a, 0x78);
		SET_DE(gb, (gb->cpu.f & FLAG_C) ? middle : right);
	}
	JUMP(SetObjectFramesetPtr);
}

// .asm_4ce9d
static void hff_face_hurt(GB *gb)
{
	hff_face_by_x(gb, ADDR_Frameset_6a092, ADDR_Frameset_6a08d, ADDR_Frameset_6a097);
}

PORT_JUMP(HiddenFigureFaceFunc)
{
	uint16_t hl = w1d14e;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_Func_4ce39);
		return;
	}
	hl = wCurObjState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_4ce63
		gb->cpu.a = rd8(gb, wCurObjFramesetOffset);
		alu_cp(gb, gb->cpu.a, 0x04);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		gb->cpu.a = rd8(gb, wCurObjFrameDuration);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		gb->cpu.a = rd8(gb, w1d14c);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z)) {
			hff_face_hurt(gb);
			return;
		}
		gb->cpu.a = rd8(gb, w1d14d);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z)) {
			hff_face_hurt(gb);
			return;
		}
		gb->cpu.a = alu_dec(gb, rd8(gb, w1d15a));
		if (gb->cpu.f & FLAG_Z) {
			hff_face_hurt(gb);
			return;
		}
		hff_face_by_x(gb, ADDR_Frameset_6a07a, ADDR_Frameset_6a06c, ADDR_Frameset_6a07f);
		return;
	}
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0xfe);
	alu_cp(gb, gb->cpu.a, 0x28);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
		hl = LO(hl, OBJ_FLAGS);
		SET_HL(gb, hl);
		hf_res_bit(gb, hl, OBJFLAG_NO_COLLISION_F);
		RET();
		return;
	}

	// .asm_4ceba
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0);
	SET_DE(gb, ADDR_Frameset_6a084);
	CALL(SetObjectFramesetPtr);
	hl = REG_HL(gb);
	gb->cpu.a = 0x3c;
	wr8(gb, hl++, 0x3c);
	hf_set_update(gb, hl, HFF_FUNC_4CF13);
	gb->cpu.a = 0x01;
	wr8(gb, w1d151, 0x01);
	hl = w1d15a;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		// .asm_4cf03
		gb->cpu.a = alu_dec(gb, rd8(gb, hl));
		if (gb->cpu.f & FLAG_Z) {
			// play_music2 MUSIC_HIDDEN_FIGURE_BATTLE_2
			gb->cpu.a = (uint8_t)(MUSIC_HIDDEN_FIGURE_BATTLE_2 >> 8);
			wr8(gb, hMusicIDBackup + 0, gb->cpu.a);
			wr8(gb, hMusicID + 0, gb->cpu.a);
			gb->cpu.a = (uint8_t)MUSIC_HIDDEN_FIGURE_BATTLE_2;
			wr8(gb, hMusicIDBackup + 1, gb->cpu.a);
			wr8(gb, hMusicID + 1, gb->cpu.a);
		}
		RET();
		return;
	}
	hl = hf_set_update(gb, wCurObjUpdateFunction + 1, HFF_FUNC_4CF3F);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	hf_set_bit(gb, hl, 3);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w1d151, 0);
	// stop_music2
	gb->cpu.a = 0xff;
	wr8(gb, hMusicIDBackup + 0, 0xff);
	wr8(gb, hMusicID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hMusicIDBackup + 1, 0x00);
	wr8(gb, hMusicID + 1, 0x00);
	gb->cpu.a = 0x10;
	wr8(gb, wCurObjStateDuration, 0x10);
	gb->cpu.a = 0x02;
	wr8(gb, wAutoMoveState, 0x02);
	RET();
}

PORT_LOCAL_JUMP(HiddenFigureFaceFunc, Func_4cf13)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		gb->cpu.a = rd8(gb, hl);
		alu_cp(gb, gb->cpu.a, 0x2c);
		if (gb->cpu.f & FLAG_Z)
			play_sfx(gb, SFX_0CE);
		JUMP_TO(ADDR_Func_4cfe0);
		return;
	}
	// .asm_4cf29
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w1d151, 0);
	hl = hf_set_update(gb, hl, ADDR_HiddenFigureFaceFunc);
	hl = LO(hl, OBJ_FLAGS);
	SET_HL(gb, hl);
	hf_res_bit(gb, hl, OBJFLAG_NO_COLLISION_F);
	gb_call(gb, ADDR_Func_4d005);
	hff_face_hurt(gb);
}

PORT_LOCAL(HiddenFigureFaceFunc, Func_4cf3f)
{
	hf_no_actions(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d)
		return;
	play_sfx(gb, SFX_0CE);
}

PORT_LOCAL_JUMP(HiddenFigureFaceFunc, Func_4cf55)
{
	hf_no_actions(gb);
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	play_sfx(gb, SFX_HIDDEN_FIGURE_LAUGH);
	SET_DE(gb, ADDR_Frameset_6a071);
	JUMP(SetObjectFramesetPtr);
}
