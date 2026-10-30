// Port of src/engine/overworld/bank2d.asm
#include "ported.h"

// rst JumpTable, with the table at `table` (right after the rst): leaves every
// register as JumpTable does and returns the target (= hl).
static uint16_t jumptable(GB *gb, uint16_t table)
{
	gb_push16(gb, table); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	return REG_HL(gb);
}


// sets the coords and state
// of SceneObj in hl, with params in de
PORT(InitSceneObjParams)
{
	uint16_t hl = REG_HL(gb), de = REG_DE(gb);
	wr8(gb, hl++, rd8(gb, de));   // y
	de++;
	wr8(gb, hl++, rd8(gb, de));   // x
	uint8_t l = alu_inc(gb, (uint8_t)hl);
	l = alu_inc(gb, l);           // inc l / inc l
	hl = (uint16_t)((hl & 0xff00) | l);
	gb->cpu.f = FLAG_Z;           // xor a
	wr8(gb, hl++, 0);             // duration
	wr8(gb, hl++, 0);             // frameset offset
	de++;
	gb->cpu.a = rd8(gb, de);
	wr8(gb, hl, gb->cpu.a);       // state
	SET_HL(gb, hl);
	SET_DE(gb, de);
}

// Animates the scene object at wCurSceneObjPtr.
PORT_JUMP(UpdateCurOWObjAnimation)
{
	gb->cpu.b = 0x2a; // bank of the overworld framesets
	gb->cpu.l = rd8(gb, wCurSceneObjPtr + 0) | (1 << 2); // set 2, l
	gb->cpu.a = rd8(gb, wCurSceneObjPtr + 1);
	gb->cpu.h = gb->cpu.a;
	JUMP(UpdateOWAnimation);
}

// When the animation of the scene object in hl has finished, resets it and
// sets its unk6 to b.
PORT(Func_b4a0d)
{
	alu_cp(gb, rd8(gb, wOWAnimationFinished), 0xff);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = rd8(gb, wOWAnimationFinished);
		return;
	}
	uint16_t hl = (uint16_t)(REG_HL(gb) | (1 << 2)); // set 2, l
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);   // duration
	wr8(gb, hl++, 0);   // frameset offset
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl, gb->cpu.a); // unk6
	SET_HL(gb, hl);
	SET_BC(gb, hl);
}

// xor a / ld hl, wOWFuncCounter / ld [hli], a / inc [hl]
static void advance_ow_func(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWFuncCounter, 0);
	SET_HL(gb, wOWFuncCounter + 1);
	wr8(gb, wOWFuncCounter + 1, alu_inc(gb, rd8(gb, wOWFuncCounter + 1)));
}

// a = number of frames to wait
PORT(WaitOWFunc)
{
	SET_HL(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, rd8(gb, wOWFuncCounter));
	if (!(gb->cpu.f & FLAG_C))
		return;
	advance_ow_func(gb);
}

PORT(AdvanceOWFunc)
{
	advance_ow_func(gb);
}

PORT(EndOWFunc)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWSceneActionParam, 0);
	wr8(gb, wOWFuncCounter, 0);
	wr8(gb, wOWFuncCounter + 1, 0); // w2d062
	SET_HL(gb, wOWFuncCounter + 1);
}

PORT_JUMP(WaitOWObjDisappear)
{
	gb->cpu.b = 0x00;
	JUMP_TO(ADDR_WaitOWObjState); // fallthrough
}

// advances the OW func once the state at hl is b
PORT_JUMP(WaitOWObjState)
{
	gb->cpu.a = rd8(gb, REG_HL(gb));
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc); // jr
}

static void cur_scene_obj_sprite(GB *gb)
{
	gb->cpu.b = 0x2a;
	gb->cpu.l = rd8(gb, wCurSceneObjPtr + 0);
	gb->cpu.a = rd8(gb, wCurSceneObjPtr + 1);
	gb->cpu.h = gb->cpu.a;
}

PORT_JUMP(LoadCurSceneObjSpriteWithScroll)
{
	cur_scene_obj_sprite(gb);
	JUMP(LoadOWSpriteWithScroll);
}

PORT_JUMP(LoadCurSceneObjSprite)
{
	cur_scene_obj_sprite(gb);
	JUMP(LoadOWSprite);
}

PORT(Func_b4000) {}

PORT(Func_b791d) {}

PORT_JUMP(Func_b4141)
{
	gb->cpu.b = 0x00;
	JUMP_TO(ADDR_Func_b4143); // fallthrough
}

PORT_JUMP(Func_b4143)
{
	// sla c / ret nc
	uint8_t c = gb->cpu.c;
	gb->cpu.c = (uint8_t)(c << 1);
	gb->cpu.f = (gb->cpu.c ? 0 : FLAG_Z) | (c & 0x80 ? FLAG_C : 0);
	if (!(c & 0x80)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj5);
	SET_DE(gb, ADDR_Func_b4143 + 0x19); // .data_1
	gb_call(gb, ADDR_InitSceneObjParams);
	SET_HL(gb, wSceneObj5Attributes);
	wr8(gb, wSceneObj5Attributes, gb->cpu.b);
	SET_HL(gb, wSceneObj1);
	SET_DE(gb, ADDR_Func_b4143 + 0x1c); // .data_2
	JUMP_TO(ADDR_InitSceneObjParams);
}

PORT_JUMP(Func_b4162)
{
	gb->cpu.b = 0x00;
	JUMP_TO(ADDR_Func_b4143);
}

PORT_JUMP(Func_b4e50)
{
	SET_DE(gb, ADDR_OAM_a884a);
	JUMP_TO(ADDR_LoadCurSceneObjSpriteWithScroll);
}

PORT_JUMP(Func_b507f)
{
	SET_DE(gb, ADDR_OAM_a8000);
	JUMP_TO(ADDR_LoadCurSceneObjSpriteWithScroll);
}

PORT_JUMP(Func_b4e4d)
{
	gb_call(gb, ADDR_UpdateCurOWObjAnimation);
	JUMP_TO(ADDR_Func_b4e50); // fallthrough
}

PORT_JUMP(Func_b507c)
{
	gb_call(gb, ADDR_UpdateCurOWObjAnimation);
	JUMP_TO(ADDR_Func_b507f); // fallthrough
}

// ld hl, obj / ld de, data / call InitSceneObjParams
static void init_obj(GB *gb, uint16_t obj, uint16_t data)
{
	SET_HL(gb, obj);
	SET_DE(gb, data);
	gb_call(gb, ADDR_InitSceneObjParams);
}

// ld hl, obj / ld de, data / jp InitSceneObjParams
static void init_obj_jump(GB *gb, uint16_t obj, uint16_t data)
{
	SET_HL(gb, obj);
	SET_DE(gb, data);
	JUMP_TO(ADDR_InitSceneObjParams);
}

// ld a, c / and a: returns true on the ret z
static bool c_is_zero(GB *gb)
{
	gb->cpu.a = alu_and(gb, gb->cpu.c, gb->cpu.c);
	return gb->cpu.f & FLAG_Z;
}

PORT_JUMP(Func_b4082)
{
	init_obj_jump(gb, wSceneObj13, ADDR_Func_b4082 + 0x9);
}

PORT_JUMP(Func_b408e)
{
	gb->cpu.c = alu_dec(gb, gb->cpu.c);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	init_obj_jump(gb, wSceneObj10, ADDR_Func_b408e + 0xb);
}

PORT_JUMP(Func_b409c)
{
	gb->cpu.a = GREEN_MUSIC_BOX;
	CALL(IsTreasureCollected);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wCurEvent), rd8(gb, wCurEvent));
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = alu_and(gb, rd8(gb, wNextMapSide), rd8(gb, wNextMapSide));
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = rd8(gb, wMapSideLevelID);
			alu_cp(gb, gb->cpu.a, OWNORTH_SEA_TURTLE_ROCKS);
			if (gb->cpu.f & FLAG_Z) {
				RET();
				return;
			}
		}
	}
	init_obj_jump(gb, wSceneObj3, ADDR_Func_b409c + 0x21);
}

// c = 0: nothing, $80: data_1, else data_2
static void init_obj_by_c(GB *gb, uint16_t obj, uint16_t data_1, uint16_t data_2)
{
	if (c_is_zero(gb)) {
		RET();
		return;
	}
	SET_HL(gb, obj);
	SET_DE(gb, data_2);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z)
		SET_DE(gb, data_1);
	JUMP_TO(ADDR_InitSceneObjParams);
}

PORT_JUMP(Func_b40c0)
{
	init_obj_by_c(gb, wSceneObj4, ADDR_Func_b40c0 + 0x13, ADDR_Func_b40c0 + 0x16);
}

PORT_JUMP(Func_b438f)
{
	init_obj_by_c(gb, wSceneObj4, ADDR_Func_b438f + 0x13, ADDR_Func_b438f + 0x16);
}

// c = 0: nothing, 1: obj_2 only, else both, then obj_2's state is cleared
// returns true if both were initialised (and the routine hasn't returned)
static bool init_one_or_two_objs(GB *gb, uint16_t obj_1, uint16_t data_1, uint16_t obj_2, uint16_t data_2)
{
	if (c_is_zero(gb)) {
		RET();
		return false;
	}
	gb->cpu.c = alu_dec(gb, gb->cpu.c);
	if (gb->cpu.f & FLAG_Z) {
		init_obj_jump(gb, obj_2, data_2);
		return false;
	}
	init_obj(gb, obj_1, data_1);
	init_obj(gb, obj_2, data_2);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	return true;
}

PORT_JUMP(Func_b40d9)
{
	if (!init_one_or_two_objs(gb, wSceneObj1, ADDR_Func_b40d9 + 0x20, wSceneObj2, ADDR_Func_b40d9 + 0x23))
		return;
	wr8(gb, wSceneObj2State, 0);
	RET();
}

PORT_JUMP(Func_b43a8)
{
	if (!init_one_or_two_objs(gb, wSceneObj1, ADDR_Func_b43a8 + 0x2f, wSceneObj5, ADDR_Func_b43a8 + 0x32))
		return;
	wr8(gb, wSceneObj5State, 0);
	static const uint16_t coords[] = {
		wAttrmap + 5 * TILEMAP_WIDTH + 16,
		wAttrmap + 6 * TILEMAP_WIDTH + 16,
		wAttrmap + 7 * TILEMAP_WIDTH + 0,
	};
	for (int i = 0; i < 3; i++) {
		SET_HL(gb, coords[i]);
		wr8(gb, coords[i], (uint8_t)(rd8(gb, coords[i]) & ~(1 << B_BG_PRIO)));
	}
	RET();
}

PORT_JUMP(Func_b40ff)
{
	if (c_is_zero(gb)) {
		RET();
		return;
	}
	gb->cpu.c = alu_sla(gb, gb->cpu.c);
	if (gb->cpu.f & FLAG_C) {
		init_obj(gb, wSceneObj6, ADDR_Func_b40ff + 0x3b);
		init_obj_jump(gb, wSceneObj7, ADDR_Func_b40ff + 0x3e);
		return;
	}
	gb->cpu.a = 0x01;
	wr8(gb, w2d12c, gb->cpu.a);
	init_obj(gb, wSceneObj6, ADDR_Func_b40ff + 0x23);
	gb->cpu.a = 0x04;
	wr8(gb, w2d12e, gb->cpu.a);
	init_obj_jump(gb, wSceneObj7, ADDR_Func_b40ff + 0x26);
}

PORT(Func_b4140) {}

PORT_JUMP(Func_b4166) { gb->cpu.b = 0x02; JUMP_TO(ADDR_Func_b4143); }
PORT_JUMP(Func_b416a) { gb->cpu.b = 0x03; JUMP_TO(ADDR_Func_b4143); }
PORT_JUMP(Func_b416e) { gb->cpu.b = 0x03; JUMP_TO(ADDR_Func_b4143); }
PORT_JUMP(Func_b4172) { gb->cpu.b = 0x02; JUMP_TO(ADDR_Func_b4143); }
PORT_JUMP(Func_b4176) { gb->cpu.b = 0x01; JUMP_TO(ADDR_Func_b4143); }

PORT_JUMP(Func_b42e7)
{
	gb->cpu.a = rd8(gb, wTransitionParam);
	alu_cp(gb, gb->cpu.a, TRANSITION_EPILOGUE_NOT_PERFECT);
	if (!(gb->cpu.f & FLAG_Z)) {
		alu_cp(gb, gb->cpu.a, TRANSITION_EPILOGUE_PERFECT);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, wSceneObj1);
	SET_BC(gb, 15 * 8);
	CALL(WriteAToHL_BCTimes);
	init_obj_jump(gb, wSceneObj1, ADDR_Func_b42e7 + 0x1f);
}

PORT_JUMP(Func_b431f)
{
	gb->cpu.a = gb->cpu.c;
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z) {
		init_obj(gb, wSceneObj15, ADDR_Func_b431f + 0x2c);
		init_obj_jump(gb, wSceneObj14, ADDR_Func_b431f + 0x2f);
		return;
	}
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		SET_HL(gb, ADDR_Data_8550b);
		FARCALL(Func_854ee);
	}
	RET();
}

PORT_JUMP(Func_b4351)
{
	gb->cpu.c = alu_dec(gb, gb->cpu.c);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	init_obj_jump(gb, wSceneObj1, ADDR_Func_b4351 + 0xb);
}

PORT(Func_b435f)
{
	init_obj(gb, wSceneObj8, ADDR_Func_b435f + 0x2a);
	gb->cpu.a = rd8(gb, w2d065);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = alu_and(gb, rd8(gb, wCurEvent), rd8(gb, wCurEvent));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	init_obj(gb, wSceneObj7, ADDR_Func_b435f + 0x2d);
	gb->cpu.a = alu_and(gb, rd8(gb, w2d011), rd8(gb, w2d011));
	gb->cpu.a = (gb->cpu.f & FLAG_Z) ? 0x06 : 0x08;
	wr8(gb, wSceneObj8State, gb->cpu.a);
}

PORT(UpdateOWTornado)
{
	SET_DE(gb, ADDR_Data_b6337);
	SET_HL(gb, wSceneObj1);
	CALL(ApplyOWMovement);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = alu_and(gb, rd8(gb, wCurEvent), rd8(gb, wCurEvent));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = alu_and(gb, rd8(gb, wOWBarsState), rd8(gb, wOWBarsState));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	play_sfx(gb, SFX_11D);
}

PORT_JUMP(Func_b4e7c) { SET_DE(gb, ADDR_Frameset_a8de2); JUMP_TO(ADDR_Func_b4e4d); }
PORT_JUMP(Func_b5004) { SET_DE(gb, ADDR_Frameset_a85cd); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b500e) { SET_DE(gb, ADDR_Frameset_a85d0); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b5018) { SET_DE(gb, ADDR_Frameset_a862a); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b50a7) { SET_DE(gb, ADDR_Frameset_a877a); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b50ac) { SET_DE(gb, ADDR_Frameset_a877d); JUMP_TO(ADDR_Func_b507c); }

// bc = OW obj
PORT_JUMP(Func_b4e8b)
{
	gb_push16(gb, REG_BC(gb));
	SET_DE(gb, ADDR_Frameset_a8e29);
	gb_call(gb, ADDR_UpdateCurOWObjAnimation);
	uint16_t hl = gb_pop16(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, w2d011), rd8(gb, w2d011));
	gb->cpu.b = (gb->cpu.f & FLAG_Z) ? 0x00 : 0x06;
	hl = (uint16_t)((hl | (1 << 0)) & ~(1 << 2)); // set 0, l / res 2, l
	wr8(gb, hl, gb->cpu.b);
	SET_HL(gb, hl);
	JUMP_TO(ADDR_Func_b4e50);
}

// a = [w2d062]: rst JumpTable on the table right after it
static void ow_func_jumptable(GB *gb, uint16_t routine)
{
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, (uint16_t)(routine + 4)));
}

// ld a, n / jp WaitOWFunc
static void wait_ow_func(GB *gb, uint8_t n)
{
	gb->cpu.a = n;
	JUMP_TO(ADDR_WaitOWFunc);
}

// ld a, state / ld hl, obj state / call SetSceneObjState / play_sfx sfx /
// jp AdvanceOWFunc
static void set_obj_state_sfx_advance(GB *gb, uint8_t state, uint16_t obj_state, uint16_t sfx)
{
	gb->cpu.a = state;
	SET_HL(gb, obj_state);
	CALL(SetSceneObjState);
	play_sfx(gb, sfx);
	JUMP_TO(ADDR_AdvanceOWFunc);
}

PORT_JUMP(NOWFunc_OpenGate) { ow_func_jumptable(gb, ADDR_NOWFunc_OpenGate); }
PORT_LOCAL_JUMP(NOWFunc_OpenGate, Wait1) { wait_ow_func(gb, 0x04); }
PORT_LOCAL_JUMP(NOWFunc_OpenGate, Wait2) { wait_ow_func(gb, 0x04); }
PORT_LOCAL_JUMP(NOWFunc_OpenGate, OpenDoor)
{
	set_obj_state_sfx_advance(gb, 0x04, wSceneObj10State, SFX_108);
}
PORT_LOCAL_JUMP(NOWFunc_OpenGate, WaitAnim)
{
	SET_HL(gb, wSceneObj10State);
	JUMP_TO(ADDR_WaitOWObjDisappear);
}

PORT_JUMP(NOWFunc_CutTree) { ow_func_jumptable(gb, ADDR_NOWFunc_CutTree); }
PORT_LOCAL_JUMP(NOWFunc_CutTree, Wait1) { wait_ow_func(gb, 4); }
PORT_LOCAL_JUMP(NOWFunc_CutTree, Wait2) { wait_ow_func(gb, 4); }
PORT_LOCAL_JUMP(NOWFunc_CutTree, SetTreeFall)
{
	set_obj_state_sfx_advance(gb, 0x02, wSceneObj8State, SFX_102);
}
PORT_LOCAL_JUMP(NOWFunc_CutTree, WaitFall)
{
	gb->cpu.a = rd8(gb, wSceneObj8Frame);
	alu_cp(gb, gb->cpu.a, 0x07);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	play_sfx(gb, SFX_01A);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(NOWFunc_CutTree, WaitDisappear)
{
	SET_HL(gb, wSceneObj8State);
	JUMP_TO(ADDR_WaitOWObjDisappear);
}

PORT_JUMP(Func_b7ab7)
{
	gb->cpu.a = rd8(gb, wCurEvent);
	alu_cp(gb, gb->cpu.a, 0x04);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	JUMP_TO(ADDR_UpdateOWTornado);
}

PORT_JUMP(Func_b403e)
{
	gb->cpu.c = alu_dec(gb, gb->cpu.c);
	if (!(gb->cpu.f & FLAG_Z))
		init_obj(gb, wSceneObj8, ADDR_Func_b403e + 0xe);
	// .Func_b4055
	gb->cpu.a = alu_and(gb, rd8(gb, wCurEvent), rd8(gb, wCurEvent));
	if (gb->cpu.f & FLAG_Z) {
		init_obj(gb, wSceneObj15, ADDR_Func_b403e + 0x3e);
		init_obj(gb, wSceneObj14, ADDR_Func_b403e + 0x41);
		gb->cpu.a = alu_and(gb, rd8(gb, w2d011), rd8(gb, w2d011));
		if (!(gb->cpu.f & FLAG_Z)) {
			gb->cpu.a = 0x18;
			wr8(gb, wSceneObj15State, gb->cpu.a);
			gb->cpu.a = 0x1a;
			wr8(gb, wSceneObj14State, gb->cpu.a);
		}
	}
	JUMP_TO(ADDR_Func_b4082);
}

// ld a, c / and $f8 / ld hl, wCurSceneObjPtr / ld [hli], a / ld [hl], b /
// ld a, [bc]
static void set_cur_scene_obj_ptr(GB *gb)
{
	gb->cpu.a = alu_and(gb, gb->cpu.c, 0xf8);
	wr8(gb, wCurSceneObjPtr + 0, gb->cpu.a);
	wr8(gb, wCurSceneObjPtr + 1, gb->cpu.b);
	SET_HL(gb, wCurSceneObjPtr + 1);
	gb->cpu.a = rd8(gb, REG_BC(gb));
}

// bc = scene obj state
PORT_JUMP(Func_b4fa2)
{
	set_cur_scene_obj_ptr(gb);
	JUMP_TO(ADDR_Func_b4fab); // fallthrough
}

PORT_JUMP(Func_b4fab)
{
	JUMP_TO(jumptable(gb, ADDR_Func_b4fab + 1));
}

// bc = scene obj state
PORT_JUMP(Func_b4dfb)
{
	set_cur_scene_obj_ptr(gb);
	JUMP_TO(ADDR_Func_b4e04); // fallthrough
}

PORT_JUMP(Func_b4e04)
{
	JUMP_TO(jumptable(gb, ADDR_Func_b4e04 + 1));
}

PORT_JUMP(Func_b7a6d)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	JUMP_TO(jumptable(gb, ADDR_Func_b7a6d + 2));
}

PORT_JUMP(Func_b4a37)
{
	gb_call(gb, ADDR_Func_b4a3d);
	JUMP_TO(ADDR_Func_b4aa9);
}

PORT_JUMP(Func_b508a)
{
	gb_call(gb, ADDR_Func_b4a0d);
	if (gb->cpu.f & FLAG_Z)
		JUMP_TO(ADDR_Func_b4fab);
	else
		JUMP_TO(ADDR_Func_b507f);
}

PORT_JUMP(Func_b5085)
{
	gb_call(gb, ADDR_UpdateCurOWObjAnimation);
	gb->cpu.b = 0x00;
	JUMP_TO(ADDR_Func_b508a); // fallthrough
}

// ld de, frameset / call UpdateCurOWObjAnimation / ld b, state / jp target
static void anim_b_jump(GB *gb, uint16_t frameset, uint8_t state, uint16_t target)
{
	SET_DE(gb, frameset);
	gb_call(gb, ADDR_UpdateCurOWObjAnimation);
	gb->cpu.b = state;
	JUMP_TO(target);
}

PORT_JUMP(Func_b5009) { SET_DE(gb, ADDR_Frameset_a8590); JUMP_TO(ADDR_Func_b5085); }
PORT_JUMP(Func_b5013) { SET_DE(gb, ADDR_Frameset_a85d3); JUMP_TO(ADDR_Func_b5085); }
PORT_JUMP(Func_b5093) { anim_b_jump(gb, ADDR_Frameset_a8782, 0x18, ADDR_Func_b508a); }
PORT_JUMP(Func_b509d) { anim_b_jump(gb, ADDR_Frameset_a8793, 0x19, ADDR_Func_b508a); }

PORT_JUMP(_DoOWFunction)
{
	SET_HL(gb, wOWFuncCounter);
	wr8(gb, wOWFuncCounter, alu_inc(gb, rd8(gb, wOWFuncCounter)));
	gb->cpu.a = rd8(gb, wCurMapSide);
	JUMP_TO(jumptable(gb, ADDR__DoOWFunction + 8));
}

PORT_JUMP(NorthOWFunctions)
{
	gb->cpu.a = alu_dec(gb, rd8(gb, wOWSceneActionParam));
	JUMP_TO(jumptable(gb, ADDR_NorthOWFunctions + 5));
}

PORT_JUMP(_InitOWObjects)
{
	gb->cpu.a = rd8(gb, w2d065);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, wOWSceneMapSide);
	JUMP_TO(jumptable(gb, ADDR__InitOWObjects + 8));
}

PORT_JUMP(InitOWObjects_North)
{
	gb->cpu.a = rd8(gb, wOWSceneActionParam);
	JUMP_TO(jumptable(gb, ADDR_InitOWObjects_North + 4));
}

PORT_JUMP(InitOWObjects_West)
{
	gb->cpu.a = rd8(gb, wOWSceneActionParam);
	JUMP_TO(jumptable(gb, ADDR_InitOWObjects_West + 4));
}

// calls `func` with bc = each of the n obj states, the last one a tail jump
// `ld bc, state` / `call func` pairs (6 bytes each) from `start`, ending
// in `ld bc, state` / `jp func`. Calls push the asm's own return address:
// a fuzzed callee can come back with another bank mapped, and then the asm
// goes on running whatever that bank holds there, so this does too.
static void for_each_obj_state(GB *gb, uint16_t start, uint16_t func, const uint16_t *states, int n)
{
	int bank = gb->rom_bank;
	for (int i = 0; i < n - 1; i++) {
		SET_BC(gb, states[i]);
		uint16_t ret = (uint16_t)(start + 6 * (i + 1));
		gb_call_ret(gb, func, ret);
		if (gb->rom_bank != bank) {
			JUMP_TO(ret);
			return;
		}
	}
	SET_BC(gb, states[n - 1]);
	JUMP_TO(func);
}

PORT_JUMP(UpdateMapSideOWAnimations)
{
	gb->cpu.a = rd8(gb, wCurMapSide);
	JUMP_TO(jumptable(gb, ADDR_UpdateMapSideOWAnimations + 4));
}
PORT_LOCAL_JUMP(UpdateMapSideOWAnimations, North)
{
	static const uint16_t states[] = {
		wSceneObj2State, wSceneObj3State, wSceneObj6State,
		wSceneObj7State, wSceneObj14State, wSceneObj15State,
	};
	for_each_obj_state(gb, ADDR_UpdateMapSideOWAnimations + 0xc, ADDR_Func_b7984, states, 6);
}
PORT_LOCAL_JUMP(UpdateMapSideOWAnimations, West)
{
	static const uint16_t states[] = {wSceneObj1State, wSceneObj7State, wSceneObj8State, wSceneObj5State};
	for_each_obj_state(gb, ADDR_UpdateMapSideOWAnimations + 0x30, ADDR_Func_b7a6d, states, 4);
}
PORT_LOCAL_JUMP(UpdateMapSideOWAnimations, South)
{
	static const uint16_t states[] = {wSceneObj7State, wSceneObj14State, wSceneObj15State};
	for_each_obj_state(gb, ADDR_UpdateMapSideOWAnimations + 0x48, ADDR_Func_b7c18, states, 3);
}
PORT_LOCAL_JUMP(UpdateMapSideOWAnimations, East)
{
	static const uint16_t states[] = {wSceneObj5State, wSceneObj7State};
	for_each_obj_state(gb, ADDR_UpdateMapSideOWAnimations + 0x5a, ADDR_Func_b7b61, states, 2);
}

static const uint16_t scene_objs_1_to_4[] = {wSceneObj1State, wSceneObj2State, wSceneObj3State, wSceneObj4State};

PORT_JUMP(Func_b4a3d)
{
	gb->cpu.a = rd8(gb, wCurMapSide);
	JUMP_TO(jumptable(gb, ADDR_Func_b4a3d + 4));
}
PORT_LOCAL_JUMP(Func_b4a3d, North) { for_each_obj_state(gb, ADDR_Func_b4a3d + 0xc, ADDR_Func_b4fa2, scene_objs_1_to_4, 4); }
PORT_LOCAL_JUMP(Func_b4a3d, West) { for_each_obj_state(gb, ADDR_Func_b4a3d + 0x24, ADDR_Func_b4dfb, scene_objs_1_to_4, 4); }
PORT_LOCAL_JUMP(Func_b4a3d, South) { for_each_obj_state(gb, ADDR_Func_b4a3d + 0x3c, ADDR_Func_b4cc2, scene_objs_1_to_4, 4); }
PORT_LOCAL_JUMP(Func_b4a3d, East) { for_each_obj_state(gb, ADDR_Func_b4a3d + 0x54, ADDR_Func_b4b99, scene_objs_1_to_4, 4); }

PORT_JUMP(Func_b4aa9)
{
	gb->cpu.a = rd8(gb, wCurMapSide);
	JUMP_TO(jumptable(gb, ADDR_Func_b4aa9 + 4));
}
PORT_LOCAL_JUMP(Func_b4aa9, North)
{
	static const uint16_t states[] = {
		wSceneObj5State, wSceneObj6State, wSceneObj7State, wSceneObj8State, wSceneObj9State,
		wSceneObj10State, wSceneObj13State, wSceneObj14State, wSceneObj15State,
	};
	for_each_obj_state(gb, ADDR_Func_b4aa9 + 0xc, ADDR_Func_b4fa2, states, 9);
}
PORT_LOCAL_JUMP(Func_b4aa9, West)
{
	static const uint16_t states[] = {
		wSceneObj5State, wSceneObj6State, wSceneObj7State, wSceneObj8State, wSceneObj10State,
		wSceneObj11State, wSceneObj12State, wSceneObj13State, wSceneObj14State, wSceneObj15State,
	};
	for_each_obj_state(gb, ADDR_Func_b4aa9 + 0x42, ADDR_Func_b4dfb, states, 10);
}
PORT_LOCAL_JUMP(Func_b4aa9, South)
{
	static const uint16_t states[] = {
		wSceneObj5State, wSceneObj6State, wSceneObj7State, wSceneObj8State,
		wSceneObj9State, wSceneObj10State, wSceneObj14State, wSceneObj15State,
	};
	for_each_obj_state(gb, ADDR_Func_b4aa9 + 0x7e, ADDR_Func_b4cc2, states, 8);
}
PORT_LOCAL_JUMP(Func_b4aa9, East)
{
	static const uint16_t states[] = {
		wSceneObj5State, wSceneObj6State, wSceneObj7State, wSceneObj8State,
		wSceneObj9State, wSceneObj10State, wSceneObj11State, wSceneObj12State,
		wSceneObj13State, wSceneObj14State, wSceneObj15State,
	};
	for_each_obj_state(gb, ADDR_Func_b4aa9 + 0xae, ADDR_Func_b4b99, states, 11);
}

// bc = scene obj state
PORT_JUMP(Func_b7984)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	JUMP_TO(jumptable(gb, ADDR_Func_b7984 + 2));
}

// ld a, state / ld hl, obj state / call SetSceneObjState
static void set_scene_obj_state(GB *gb, uint8_t state, uint16_t obj_state)
{
	gb->cpu.a = state;
	SET_HL(gb, obj_state);
	CALL(SetSceneObjState);
}

PORT_LOCAL(Func_b7984, Func_b79de)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wCurEvent), rd8(gb, wCurEvent));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = rd8(gb, wLevel2);
	alu_cp(gb, gb->cpu.a, 0x06);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	set_scene_obj_state(gb, 0x06, wSceneObj3State);
	play_sfx(gb, SFX_BLOCK_BREAK);
}

// sets the obj's state if w2d011 is zero (day) or not (night)
static void set_state_day_night(GB *gb, bool night, uint8_t state, uint16_t obj_state)
{
	gb->cpu.a = alu_and(gb, rd8(gb, w2d011), rd8(gb, w2d011));
	if (!(gb->cpu.f & FLAG_Z) != night)
		return;
	set_scene_obj_state(gb, state, obj_state);
}

PORT_LOCAL(Func_b7984, Func_b79fa) { set_state_day_night(gb, false, 0x17, wSceneObj15State); }
PORT_LOCAL(Func_b7984, Func_b7a08) { set_state_day_night(gb, true, 0x16, wSceneObj15State); }
PORT_LOCAL(Func_b7984, Func_b7a16) { set_state_day_night(gb, false, 0x1b, wSceneObj14State); }
PORT_LOCAL(Func_b7984, Func_b7a24) { set_state_day_night(gb, true, 0x1a, wSceneObj14State); }

// Func_b7984.Func_b7a5a
static void func_b7a5a(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wCurEvent), rd8(gb, wCurEvent));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = alu_and(gb, rd8(gb, w2d124), rd8(gb, w2d124));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	alu_cp(gb, gb->cpu.a, 0x02);
	if (gb->cpu.f & FLAG_Z)
		return;
	alu_cp(gb, gb->cpu.a, 0x07);
}

PORT_LOCAL(Func_b7984, Func_b7a32)
{
	func_b7a5a(gb);
	SET_HL(gb, w2d125);
	if (!(gb->cpu.f & FLAG_Z)) {
		wr8(gb, w2d125, alu_inc(gb, rd8(gb, w2d125)));
		gb->cpu.a = rd8(gb, w2d125);
		alu_cp(gb, gb->cpu.a, 0x78);
		if (gb->cpu.f & FLAG_C)
			return;
	}
	// .asm_b7a3f
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d125, 0);
	wr8(gb, w2d124, 0);
	func_b7a5a(gb);
	gb->cpu.b = 0x1d;
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0x01;
		wr8(gb, w2d124, gb->cpu.a);
		gb->cpu.b = 0x0d;
	}
	// .asm_b7a52
	set_scene_obj_state(gb, gb->cpu.b, wSceneObj2State);
}

// decompress_tilemap: decompresses src (in bank) to dest
static void decompress(GB *gb, uint8_t bank, uint16_t src, uint16_t dest)
{
	gb->cpu.a = bank;
	wr8(gb, wTempBank, bank);
	SET_HL(gb, src);
	SET_BC(gb, dest);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
}

// decompress_attrmap: the same, into VRAM bank 1
static void decompress_vram1(GB *gb, uint8_t bank, uint16_t src, uint16_t dest)
{
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, 1);
	decompress(gb, bank, src, dest);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

// ld a, $80 / cp c: returns true on the ret nz
static bool c_isnt_80(GB *gb)
{
	gb->cpu.a = 0x80;
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	return !(gb->cpu.f & FLAG_Z);
}

PORT_JUMP(Func_b417a)
{
	if (c_isnt_80(gb)) {
		RET();
		return;
	}
	init_obj(gb, wSceneObj1, ADDR_Func_b417a + 0xbe);
	init_obj(gb, wSceneObj5, ADDR_Func_b417a + 0xc1);
	init_obj(gb, wSceneObj8, ADDR_Func_b417a + 0xc4);
	init_obj(gb, wSceneObj9, ADDR_Func_b417a + 0xc7);
	init_obj(gb, wSceneObj10, ADDR_Func_b417a + 0xca);
	static const uint16_t attributes[] = {
		wSceneObj5Attributes, wSceneObj8Attributes, wSceneObj9Attributes, wSceneObj10Attributes,
	};
	for (int i = 0; i < 4; i++) {
		SET_HL(gb, attributes[i]);
		wr8(gb, attributes[i], (uint8_t)(rd8(gb, attributes[i]) | (1 << 2)));
	}
	FARCALL(Func_80bd9);
	FARCALL(VBlank_Overworld);
	decompress(gb, BANK_OverworldBottomBarTreasuresTilemap, ADDR_OverworldBottomBarTreasuresTilemap, v0BGMap1);
	decompress_vram1(gb, BANK_OverworldBottomBarTreasuresAttrmap, ADDR_OverworldBottomBarTreasuresAttrmap, v1BGMap1);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wWX, 0);
	wr8(gb, rWX, 0);
	gb->cpu.a = SCREEN_HEIGHT_PX;
	wr8(gb, wWY, gb->cpu.a);
	wr8(gb, rWY, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a (BOTBAR_CLOSED)
	wr8(gb, wBottomBarAction, 0);
	// copy empty treasure gfx
	SET_HL(gb, ADDR_TreasureGfx);
	SET_DE(gb, v0Tiles1 + 0x10 * TILE_SIZE);
	gb->cpu.c = 4 * TILE_SIZE;
	gb->cpu.b = BANK_TreasureGfx;
	JUMP(CopyFarBytes);
}

PORT(Func_b4247)
{
	if (c_isnt_80(gb))
		return;
	decompress_vram1(gb, BANK_OverworldTopBarDayNightAttrmap, ADDR_OverworldTopBarDayNightAttrmap, v1BGMap0 + 29 * TILEMAP_WIDTH);
	decompress(gb, BANK_OverworldTopBarDayNightTilemap, ADDR_OverworldTopBarDayNightTilemap, v0BGMap0 + 29 * TILEMAP_WIDTH);
	SET_HL(gb, v0BGMap0 + 30 * TILEMAP_WIDTH);
	SET_DE(gb, wAttrmap + 21 * TILEMAP_WIDTH);
	gb->cpu.b = 2 * TILEMAP_WIDTH;
	CALL(CopyHLToDE);
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, 1);
	SET_HL(gb, v0BGMap0 + 30 * TILEMAP_WIDTH);
	SET_DE(gb, wAttrmap + 19 * TILEMAP_WIDTH);
	gb->cpu.b = 2 * TILEMAP_WIDTH;
	CALL(CopyHLToDE);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, ADDR_Pals_84a20);
	SET_DE(gb, wTempPals2 + 4 * PAL_SIZE);
	gb->cpu.c = 4 * PAL_SIZE;
	gb->cpu.b = BANK_Pals_84a20;
	CALL(CopyFarBytes);
	init_obj(gb, wSceneObj5, ADDR_Func_b4247 + 0x9d);
	FARCALL(VBlank_Overworld);
	FARCALL(DrawCoinCount);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj6State, 0);
	wr8(gb, wSceneObj7State, 0);
	wr8(gb, wSceneObj2State, 0);
}

PORT(Func_b586d)
{
	FARCALL(Func_854ee);
}

PORT_JUMP(Func_b43dd)
{
	if (c_is_zero(gb)) {
		RET();
		return;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		SET_HL(gb, ADDR_Data_85549);
		JUMP_TO(ADDR_Func_b586d);
		return;
	}
	// .asm_b43e9
	init_obj(gb, wSceneObj11, ADDR_Func_b43dd + 0x50);
	init_obj(gb, wSceneObj12, ADDR_Func_b43dd + 0x53);
	init_obj(gb, wSceneObj13, ADDR_Func_b43dd + 0x56);
	init_obj(gb, wSceneObj14, ADDR_Func_b43dd + 0x59);
	init_obj(gb, wSceneObj15, ADDR_Func_b43dd + 0x5c);
	init_obj(gb, wSceneObj10, ADDR_Func_b43dd + 0x5f);
	SET_HL(gb, ADDR_Func_b43dd + 0x63); // .pal
	SET_DE(gb, wTempPals2 + 2 * PAL_SIZE);
	gb->cpu.b = 1 * PAL_SIZE;
	CALL(CopyHLToDE);
	JUMP_TO(ADDR_LoadOverworldRocksGfx);
}

PORT_JUMP(Func_b4448)
{
	if (c_is_zero(gb)) {
		RET();
		return;
	}
	gb->cpu.a = alu_sla(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_C) {
		// .asm_b4455
		gb->cpu.a = rd8(gb, wOWSceneActionParam);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = rd8(gb, wTempUnlockableConnectionID);
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		if (gb->cpu.f & FLAG_Z) {
			init_obj(gb, wSceneObj11, ADDR_Func_b43dd + 0x50);
			init_obj(gb, wSceneObj10, ADDR_Func_b43dd + 0x5f);
			JUMP_TO(ADDR_LoadOverworld4Gfx);
			return;
		}
	}
	// .asm_b444f
	SET_HL(gb, ADDR_Data_85536);
	JUMP_TO(ADDR_Func_b586d);
}

PORT_JUMP(Func_b4474)
{
	if (c_is_zero(gb)) {
		RET();
		return;
	}
	gb_call(gb, ADDR_LoadOverworld3Gfx);
	init_obj(gb, wSceneObj6, ADDR_Func_b4474 + 0x24);
	gb->cpu.a = alu_add(gb, rd8(gb, w2d065), rd8(gb, w2d065));
	if (!(gb->cpu.f & FLAG_Z)) {
		SET_HL(gb, ADDR_Data_8556e);
		JUMP_TO(ADDR_Func_b586d);
		return;
	}
	// .asm_b448f
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj6State, 0);
	SET_HL(gb, wSceneObj6YCoord);
	wr8(gb, wSceneObj6YCoord, alu_inc(gb, rd8(gb, wSceneObj6YCoord)));
	RET();
}

// --- WOWFunc_ClearTornado and the OW scene helpers ---

PORT_JUMP(WOWFunc_ClearTornado)
{
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_WOWFunc_ClearTornado + 4));
}

PORT_LOCAL_JUMP(WOWFunc_ClearTornado, Wait1)
{
	gb->cpu.a = 20;
	JUMP_TO(ADDR_WaitOWFunc);
}

PORT_LOCAL_JUMP(WOWFunc_ClearTornado, ShowTornado)
{
	gb_call(gb, ADDR_UpdateOWTornado);
	gb->cpu.a = rd8(gb, wSceneObj1Unk7);
	alu_cp(gb, gb->cpu.a, 0x01);
	if (gb->cpu.f & FLAG_Z) {
		play_sfx(gb, SFX_11D);
		RET();
		return;
	}
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}

PORT_LOCAL_JUMP(WOWFunc_ClearTornado, ClearTornado)
{
	SET_DE(gb, ADDR_Data_b6478);
	SET_HL(gb, wSceneObj1);
	CALL(ApplyOWMovement);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}

PORT_LOCAL_JUMP(WOWFunc_ClearTornado, HideTornado)
{
	gb->cpu.a = 0x00;
	SET_HL(gb, wSceneObj1State);
	CALL(SetSceneObjState);
	JUMP_TO(ADDR_AdvanceOWFunc);
}

PORT_JUMP(LoadOWSceneLevelName)
{
	gb->cpu.a = rd8(gb, wCurMapSide);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	uint16_t hl = alu_add16(gb, ADDR_LoadOWSceneLevelName + 0x25, REG_DE(gb)); // .MapSidePointers
	gb->cpu.a = rd8(gb, hl);
	hl++;
	gb->cpu.h = rd8(gb, hl);
	gb->cpu.l = gb->cpu.a;
	gb->cpu.a = rd8(gb, wOWSceneActionParam);
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	hl = alu_add16(gb, REG_HL(gb), REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.d = rd8(gb, hl);
	gb->cpu.a = alu_and(gb, gb->cpu.b, gb->cpu.b);
	if (!(gb->cpu.f & FLAG_Z))
		gb->cpu.d = alu_dec(gb, gb->cpu.d);
	CALL(LoadLevelName);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPalConfig1TotalSteps, 0);
	JUMP_TO(ADDR_AdvanceOWFunc);
}

PORT_JUMP(WaitOWSceneLevelNameFadeIn)
{
	FARCALL(FadeInLevelName);
	uint8_t c = gb->cpu.c;
	gb->cpu.c = alu_srl(gb, c);
	if (c & 1) {
		RET(); // not done
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}

PORT_JUMP(WaitOWScene16Frames)
{
	gb->cpu.a = 16;
	JUMP_TO(ADDR_WaitOWFunc);
}

PORT_JUMP(Func_b5868)
{
	gb->cpu.a = TRUE;
	wr8(gb, wOWPendingTileUpdate, TRUE);
	JUMP_TO(ADDR_Func_b586d); // fallthrough
}

PORT(Func_b587d) { FARCALL(Func_8518c); }
PORT(Func_b588d) { FARCALL(Func_85145); }

PORT(GetSceneFallingYOffset)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 100); // .yoffsets_end - .yoffsets
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = 100 - 4;
		wr8(gb, hl, gb->cpu.a);
	}
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	hl = alu_add16(gb, ADDR_GetSceneFallingYOffset + 0x11, REG_DE(gb)); // .yoffsets
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
}

PORT_JUMP(Func_b4e5b)
{
	gb_call(gb, ADDR_Func_b4a0d);
	if (gb->cpu.f & FLAG_Z)
		JUMP_TO(ADDR_Func_b4e04);
	else
		JUMP_TO(ADDR_Func_b4e50);
}

PORT_JUMP(Func_b4e65)
{
	SET_DE(gb, ADDR_OAM_a8f57);
	JUMP_TO(ADDR_LoadCurSceneObjSpriteWithScroll);
}

PORT_JUMP(Func_b4c5a)
{
	SET_DE(gb, ADDR_OAM_a980c);
	JUMP_TO(ADDR_LoadCurSceneObjSpriteWithScroll);
}

PORT_JUMP(Func_b4da3)
{
	SET_DE(gb, ADDR_OAM_a9161);
	JUMP_TO(ADDR_LoadCurSceneObjSpriteWithScroll);
}

PORT(Func_b4dd4) { play_sfx(gb, SFX_12F); }

PORT(Func_b4873) {}

PORT_JUMP(Func_b4874)
{
	SET_HL(gb, ADDR_Func_b4874 + 0xb); // .pal
	SET_DE(gb, wTempPals1 + 4 * PAL_SIZE);
	gb->cpu.b = 1 * PAL_SIZE;
	JUMP(CopyHLToDE);
}

// sets the priority bit of b bytes
// starting from hl in BGMap
PORT(SetBGMapBytesPriority)
{
	uint16_t hl = REG_HL(gb);
	do {
		wr8(gb, hl, rd8(gb, hl) | BG_PRIO);
		hl++;
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (!(gb->cpu.f & FLAG_Z));
	SET_HL(gb, hl);
}

// decompress_tiles0: ld a, BANK(gfx) / ld [wTempBank], a / ld hl, gfx /
// ld bc, dest / ld a, [wTempBank] / ldh [hCallFuncBank], a / hcall Decompress
static void decompress_tiles0(GB *gb, uint8_t bank, uint16_t gfx, uint16_t dest)
{
	gb->cpu.a = bank;
	wr8(gb, wTempBank, bank);
	SET_HL(gb, gfx);
	SET_BC(gb, dest);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
}

PORT(LoadOverworld3Gfx) { decompress_tiles0(gb, BANK_Overworld3Gfx, ADDR_Overworld3Gfx, v0Tiles0 + 0x00 * TILE_SIZE); }
PORT(LoadOverworld4Gfx) { decompress_tiles0(gb, BANK_Overworld4Gfx, ADDR_Overworld4Gfx, v0Tiles0 + 0x00 * TILE_SIZE); }
PORT(LoadOverworldRocksGfx) { decompress_tiles0(gb, BANK_OverworldRocksGfx, ADDR_OverworldRocksGfx, v0Tiles0 + 0x58 * TILE_SIZE); }
PORT(LoadOverworldTowerGfx) { decompress_tiles0(gb, BANK_OverworldTowerGfx, ADDR_OverworldTowerGfx, v0Tiles0 + 0x60 * TILE_SIZE); }
PORT(LoadOverworldFanGfx) { decompress_tiles0(gb, BANK_OverworldFanGfx, ADDR_OverworldFanGfx, v0Tiles0 + 0x60 * TILE_SIZE); }
PORT(LoadOverworldExplosion1Gfx) { decompress_tiles0(gb, BANK_OverworldExplosion1Gfx, ADDR_OverworldExplosion1Gfx, v0Tiles0 + 0x00 * TILE_SIZE); }
PORT(LoadOverworldThunderGfx) { decompress_tiles0(gb, BANK_OverworldThunderGfx, ADDR_OverworldThunderGfx, v0Tiles0 + 0x00 * TILE_SIZE); }
PORT(LoadOverworldExplosion2Gfx) { decompress_tiles0(gb, BANK_OverworldExplosion2Gfx, ADDR_OverworldExplosion2Gfx, v0Tiles0 + 0x00 * TILE_SIZE); }
PORT(LoadOverworldTorchGfx) { decompress_tiles0(gb, BANK_OverworldTorchGfx, ADDR_OverworldTorchGfx, v0Tiles0 + 0x40 * TILE_SIZE); }
PORT(LoadOverworldFireGfx) { decompress_tiles0(gb, BANK_OverworldFireGfx, ADDR_OverworldFireGfx, v0Tiles0 + 0x00 * TILE_SIZE); }
PORT(LoadOverworldExplosion3Gfx) { decompress_tiles0(gb, BANK_OverworldExplosion3Gfx, ADDR_OverworldExplosion3Gfx, v0Tiles0 + 0x00 * TILE_SIZE); }
PORT(LoadOverworldSmokeGfx) { decompress_tiles0(gb, BANK_OverworldSmokeGfx, ADDR_OverworldSmokeGfx, v0Tiles0 + 0x32 * TILE_SIZE); }

// --- more scene object animation states ---

PORT(Func_b4f58)
{
	uint16_t hl = wTempPals1 + 5 * PAL_SIZE;
	gb->cpu.a = rd8(gb, wSceneObj11Frame);
	alu_cp(gb, gb->cpu.a, 0x57);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = rd8(gb, wSceneObj11State);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z))
			hl = ADDR_Func_b4f58 + 0x22; // .pal
	}
	SET_HL(gb, hl);
	SET_DE(gb, wTempBGPals + 5 * PAL_SIZE);
	gb->cpu.b = 1 * PAL_SIZE;
	CALL(CopyHLToDE);
	SET_HL(gb, wPalConfig1);
	gb->cpu.a = (uint8_t)rBGPI;
	wr8(gb, wPalConfig1, gb->cpu.a);
}

PORT_JUMP(Func_b50e1)
{
	gb_call(gb, ADDR_UpdateCurOWObjAnimation);
	SET_DE(gb, ADDR_OAM_aa580);
	JUMP_TO(ADDR_LoadCurSceneObjSpriteWithScroll);
}

PORT_JUMP(Func_b50ea)
{
	SET_DE(gb, ADDR_Frameset_aa252);
	gb_call(gb, ADDR_UpdateCurOWObjAnimation);
	SET_DE(gb, ADDR_OAM_aa0cf);
	JUMP_TO(ADDR_LoadCurSceneObjSpriteWithScroll);
}

PORT_JUMP(Func_b5114)
{
	gb_call(gb, ADDR_UpdateCurOWObjAnimation);
	SET_DE(gb, ADDR_OAM_a9fe2); // .asm_b5117
	JUMP_TO(ADDR_LoadCurSceneObjSpriteWithScroll);
}

PORT_JUMP(Func_b50fb) { anim_b_jump(gb, ADDR_Frameset_aa0c1, 0x27, ADDR_Func_b5114 + 0xe); } // .asm_b5122
PORT_JUMP(Func_b510a) { anim_b_jump(gb, ADDR_Frameset_aa0bc, 0x25, ADDR_Func_b5114 + 0xe); } // .asm_b5122

PORT_JUMP(Func_b5136)
{
	SET_DE(gb, ADDR_Frameset_a883b);
	gb_call(gb, ADDR_UpdateCurOWObjAnimation);
	gb->cpu.b = 0x00;
	gb_call(gb, ADDR_Func_b4a0d);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_Func_b4fab);
		return;
	}
	SET_DE(gb, ADDR_OAM_a87fa);
	JUMP_TO(ADDR_LoadCurSceneObjSprite);
}

PORT_JUMP(Func_b5027) { anim_b_jump(gb, ADDR_Frameset_a8639, 0x09, ADDR_Func_b508a); }
PORT_JUMP(Func_b5036) { anim_b_jump(gb, ADDR_Frameset_a8646, 0x0b, ADDR_Func_b508a); }
PORT_JUMP(Func_b5045) { anim_b_jump(gb, ADDR_Frameset_a867f, 0x0e, ADDR_Func_b508a); }
PORT_JUMP(Func_b504f) { anim_b_jump(gb, ADDR_Frameset_a86da, 0x0e, ADDR_Func_b508a); }
PORT_JUMP(Func_b50b6) { anim_b_jump(gb, ADDR_Frameset_a873d, 0x0e, ADDR_Func_b508a); }
PORT_JUMP(Func_b50c0) { anim_b_jump(gb, ADDR_Frameset_a87d2, 0x20, ADDR_Func_b508a); }

// --- scene object animation stubs (b4e56..b5130) ---

PORT_JUMP(Func_b4e56)
{
	gb_call(gb, ADDR_UpdateCurOWObjAnimation);
	gb->cpu.b = 0x00;
	JUMP_TO(ADDR_Func_b4e5b); // fallthrough
}

PORT_JUMP(Func_b4e62)
{
	gb_call(gb, ADDR_UpdateCurOWObjAnimation);
	JUMP_TO(ADDR_Func_b4e65); // fallthrough
}

PORT_JUMP(Func_b4e6b)
{
	gb_call(gb, ADDR_UpdateCurOWObjAnimation);
	gb->cpu.b = 0x00;
	JUMP_TO(ADDR_Func_b4e70); // fallthrough
}

PORT_JUMP(Func_b4e70)
{
	gb_call(gb, ADDR_Func_b4a0d);
	if (gb->cpu.f & FLAG_Z)
		JUMP_TO(ADDR_Func_b4e04);
	else
		JUMP_TO(ADDR_Func_b4e65);
}

PORT_JUMP(Func_b4e77) { SET_DE(gb, ADDR_Frameset_a8de9); JUMP_TO(ADDR_Func_b4e56); }
PORT_JUMP(Func_b4e81) { SET_DE(gb, ADDR_Frameset_a8dd1); JUMP_TO(ADDR_Func_b4e4d); }
PORT_JUMP(Func_b4e86) { SET_DE(gb, ADDR_Frameset_a8e1e); JUMP_TO(ADDR_Func_b4e56); }
PORT_JUMP(Func_b4ea4) { SET_DE(gb, ADDR_Frameset_a8e2c); JUMP_TO(ADDR_Func_b4e4d); }

PORT(Func_b4ea9)
{
	SET_DE(gb, ADDR_Frameset_a8e5d);
	gb_call(gb, ADDR_Func_b4e56);
	gb->cpu.a = rd8(gb, wSceneObj7State);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	wr8(gb, wSceneObj8Duration, gb->cpu.a);
	gb->cpu.a = 0x08;
	wr8(gb, wSceneObj8State, gb->cpu.a);
}

PORT_JUMP(Func_b4ebd) { SET_DE(gb, ADDR_Frameset_a8e6c); JUMP_TO(ADDR_Func_b4e4d); }
PORT_JUMP(Func_b4ec2) { SET_DE(gb, ADDR_Frameset_a8df8); JUMP_TO(ADDR_Func_b4e4d); }
PORT_JUMP(Func_b4ec8) { anim_b_jump(gb, ADDR_Frameset_a8e04, 0x0c, ADDR_Func_b4e5b); }
PORT_JUMP(Func_b4ed2) { SET_DE(gb, ADDR_Frameset_a8e01); JUMP_TO(ADDR_Func_b4e4d); }
PORT_JUMP(Func_b4ed8) { anim_b_jump(gb, ADDR_Frameset_a8e11, 0x0e, ADDR_Func_b4e5b); }
PORT_JUMP(Func_b4ee3) { SET_DE(gb, ADDR_Frameset_a8f17); JUMP_TO(ADDR_Func_b4e4d); }
PORT_JUMP(Func_b4ee9) { anim_b_jump(gb, ADDR_Frameset_a9090, 0x11, ADDR_Func_b4e70); }
PORT_JUMP(Func_b4ef4) { anim_b_jump(gb, ADDR_Frameset_a90eb, 0x11, ADDR_Func_b4e70); }
PORT_JUMP(Func_b4eff) { SET_DE(gb, ADDR_Frameset_a9140); JUMP_TO(ADDR_Func_b4e62); }
PORT_JUMP(Func_b4f05) { SET_DE(gb, ADDR_Frameset_a9143); JUMP_TO(ADDR_Func_b4e62); }
PORT_JUMP(Func_b4f0b) { anim_b_jump(gb, ADDR_Frameset_a914e, 0x11, ADDR_Func_b4e70); }
PORT_JUMP(Func_b4f16) { SET_DE(gb, ADDR_Frameset_a8e9d); JUMP_TO(ADDR_Func_b4e56); }
PORT_JUMP(Func_b4f1c) { SET_DE(gb, ADDR_Frameset_a8eac); JUMP_TO(ADDR_Func_b4e56); }
PORT_JUMP(Func_b4f22) { SET_DE(gb, ADDR_Frameset_a8eb7); JUMP_TO(ADDR_Func_b4e56); }
PORT_JUMP(Func_b4f28) { SET_DE(gb, ADDR_Frameset_a8ec2); JUMP_TO(ADDR_Func_b4e4d); }
PORT_JUMP(Func_b4f2e) { SET_DE(gb, ADDR_Frameset_a8ec7); JUMP_TO(ADDR_Func_b4e4d); }
PORT_JUMP(Func_b4f34) { SET_DE(gb, ADDR_Frameset_a8eca); JUMP_TO(ADDR_Func_b4e4d); }
PORT_JUMP(Func_b4f3a) { SET_DE(gb, ADDR_Frameset_a8eca); JUMP_TO(ADDR_Func_b4e56); }
PORT_JUMP(Func_b4f40) { SET_DE(gb, ADDR_Frameset_a8ed3); JUMP_TO(ADDR_Func_b4e4d); }
PORT_JUMP(Func_b4f46) { SET_DE(gb, ADDR_Frameset_a8eda); JUMP_TO(ADDR_Func_b4e4d); }
PORT_JUMP(Func_b4f4c) { SET_DE(gb, ADDR_Frameset_a8ee1); JUMP_TO(ADDR_Func_b4e4d); }

PORT_JUMP(Func_b4f52)
{
	SET_DE(gb, ADDR_Frameset_a8ee8);
	gb_call(gb, ADDR_Func_b4e4d);
	JUMP_TO(ADDR_Func_b4f58); // fallthrough
}

PORT_JUMP(Func_b4f82)
{
	SET_DE(gb, ADDR_Frameset_a8ee8);
	gb_call(gb, ADDR_Func_b4e56);
	JUMP_TO(ADDR_Func_b4f58);
}

PORT_JUMP(Func_b4f8a) { SET_DE(gb, ADDR_Frameset_a8f28); JUMP_TO(ADDR_Func_b4e4d); }
PORT_JUMP(Func_b4f90) { SET_DE(gb, ADDR_Frameset_a8f31); JUMP_TO(ADDR_Func_b4e4d); }
PORT_JUMP(Func_b4f96) { SET_DE(gb, ADDR_Frameset_a8f47); JUMP_TO(ADDR_Func_b4e4d); }
PORT_JUMP(Func_b4f9c) { SET_DE(gb, ADDR_Frameset_a8f3a); JUMP_TO(ADDR_Func_b4e4d); }
PORT_JUMP(Func_b501d) { SET_DE(gb, ADDR_Frameset_a8653); JUMP_TO(ADDR_Func_b5085); }
PORT_JUMP(Func_b5022) { SET_DE(gb, ADDR_Frameset_a862d); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b5031) { SET_DE(gb, ADDR_Frameset_a8636); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b5040) { SET_DE(gb, ADDR_Frameset_a866e); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b5059) { SET_DE(gb, ADDR_Frameset_a872f); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b505e) { SET_DE(gb, ADDR_Frameset_a8732); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b5063) { SET_DE(gb, ADDR_Frameset_a8750); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b5068) { SET_DE(gb, ADDR_Frameset_a8761); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b506d) { SET_DE(gb, ADDR_Frameset_a879a); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b5072) { SET_DE(gb, ADDR_Frameset_a8764); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b5077) { SET_DE(gb, ADDR_Frameset_a876f); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b50b1) { SET_DE(gb, ADDR_Frameset_a879f); JUMP_TO(ADDR_Func_b5085); }
PORT_JUMP(Func_b50ca) { SET_DE(gb, ADDR_Frameset_a87d9); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b50cf) { SET_DE(gb, ADDR_Frameset_a87dc); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b50d4) { SET_DE(gb, ADDR_Frameset_a87df); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b50d9) { SET_DE(gb, ADDR_Frameset_aa5f5); JUMP_TO(ADDR_Func_b50e1); }
PORT_JUMP(Func_b50de) { SET_DE(gb, ADDR_Frameset_aa5f8); JUMP_TO(ADDR_Func_b50e1); }
PORT_JUMP(Func_b50f6) { SET_DE(gb, ADDR_Frameset_aa0a3); JUMP_TO(ADDR_Func_b5114); }
PORT_JUMP(Func_b5105) { SET_DE(gb, ADDR_Frameset_aa09d); JUMP_TO(ADDR_Func_b5114); }
PORT_JUMP(Func_b512a) { SET_DE(gb, ADDR_Frameset_a87e8); JUMP_TO(ADDR_Func_b507c); }
PORT_JUMP(Func_b5130) { SET_DE(gb, ADDR_Frameset_a87f1); JUMP_TO(ADDR_Func_b507c); }

// ---- InitOWObjects_South / _East entries ----

// ld a, $80 / cp c
static bool c_is_80(GB *gb)
{
	gb->cpu.a = 0x80;
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	return gb->cpu.f & FLAG_Z;
}

// ld a, [wOWSceneActionParam] / ld b, a / ld a, [wTempUnlockableConnectionID] / cp b
static bool is_temp_unlockable(GB *gb)
{
	gb->cpu.a = rd8(gb, wOWSceneActionParam);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = rd8(gb, wTempUnlockableConnectionID);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	return gb->cpu.f & FLAG_Z;
}

PORT_JUMP(Func_b44b7)
{
	if (!c_is_80(gb)) {
		RET();
		return;
	}
	gb->cpu.a = rd8(gb, wOWSceneActionParam);
	SET_HL(gb, wTempUnlockableConnectionID);
	alu_cp(gb, gb->cpu.a, rd8(gb, wTempUnlockableConnectionID));
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	init_obj(gb, wSceneObj2, ADDR_Func_b44b7 + 0x2a); // .data_2
	init_obj(gb, wSceneObj3, ADDR_Func_b44b7 + 0x2d); // .data_3
	init_obj_jump(gb, wSceneObj4, ADDR_Func_b44b7 + 0x27); // .data_1
}

PORT_JUMP(Func_b44e7)
{
	if (c_is_zero(gb)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj4);
	SET_DE(gb, ADDR_Func_b44e7 + 0x1f); // .data_2
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_Z) {
		if (!is_temp_unlockable(gb)) {
			RET();
			return;
		}
		SET_DE(gb, ADDR_Func_b44e7 + 0x1c); // .data_1
	}
	JUMP_TO(ADDR_InitSceneObjParams);
}

PORT(Func_b4509)
{
	init_obj(gb, wSceneObj6, ADDR_Func_b4509 + 0x14); // .data
	gb->cpu.a = alu_dec(gb, rd8(gb, w2d065));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	SET_HL(gb, wSceneObj6State);
	wr8(gb, wSceneObj6State, alu_inc(gb, rd8(gb, wSceneObj6State)));
	wr8(gb, wSceneObj6State, alu_inc(gb, rd8(gb, wSceneObj6State)));
}

PORT_JUMP(Func_b4520)
{
	if (c_is_80(gb)) {
		init_obj(gb, wSceneObj2, ADDR_Func_b4520 + 0x3e); // .data_1
		init_obj(gb, wSceneObj10, ADDR_Func_b4520 + 0x41); // .data_2
		init_obj(gb, wSceneObj5, ADDR_Func_b4520 + 0x44); // call .asm_b4539
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wSceneObj5State, 0);
		JUMP_TO(ADDR_LoadOverworldThunderGfx);
		return;
	}
	gb->cpu.c = alu_dec(gb, gb->cpu.c);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	SET_HL(gb, ADDR_Data_855b2);
	FARCALL(Func_854ee);
	init_obj_jump(gb, wSceneObj5, ADDR_Func_b4520 + 0x44); // .data_3
}

PORT_JUMP(Func_b4567)
{
	gb->cpu.a = alu_dec(gb, gb->cpu.c);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_b459a
		SET_HL(gb, ADDR_Data_85579);
		FARCALL(Func_854ee);
		gb->cpu.a = rd8(gb, wCurEvent);
		if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			RET();
			return;
		}
		init_obj(gb, wSceneObj15, ADDR_Func_b4567 + 0x5e); // .data_3
		gb->cpu.a = rd8(gb, w2d011);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			RET();
			return;
		}
		gb->cpu.a = 0x1e;
		wr8(gb, wSceneObj15State, gb->cpu.a);
		RET();
		return;
	}
	gb->cpu.c = alu_sla(gb, gb->cpu.c);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	init_obj(gb, wSceneObj2, ADDR_Func_b4567 + 0x2d); // .data_1
	init_obj(gb, wSceneObj3, ADDR_Func_b4567 + 0x30); // .data_2

	gb->cpu.b = 3;
	SET_DE(gb, TILEMAP_WIDTH - 1);
	uint16_t hl = wAttrmap + 9 * TILEMAP_WIDTH + 14;
	do {
		wr8(gb, hl, rd8(gb, hl) | BG_PRIO);
		hl++;
		wr8(gb, hl, rd8(gb, hl) | BG_PRIO);
		hl = alu_add16(gb, hl, REG_DE(gb));
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (!(gb->cpu.f & FLAG_Z));
	SET_HL(gb, hl);
	JUMP_TO(ADDR_LoadOverworldTowerGfx);
}

PORT_JUMP(Func_b45c8)
{
	if (!c_is_zero(gb)) {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z) {
			RET();
			return;
		}
		init_obj(gb, wSceneObj1, ADDR_Func_b45c8 + 0x27); // .data_2
		init_obj(gb, wSceneObj8, ADDR_Func_b45c8 + 0x2a); // .data_3
		gb_call(gb, ADDR_LoadOverworldFanGfx);
	}
	init_obj_jump(gb, wSceneObj9, ADDR_Func_b45c8 + 0x24); // .data_1
}

PORT_JUMP(Func_b45f5)
{
	if (c_is_zero(gb)) {
		RET();
		return;
	}
	gb->cpu.c = alu_dec(gb, gb->cpu.c);
	if (gb->cpu.f & FLAG_Z) {
		init_obj_jump(gb, wSceneObj7, ADDR_Func_b45f5 + 0x23); // .data_2
		return;
	}
	init_obj(gb, wSceneObj1, ADDR_Func_b45f5 + 0x20); // .data_1
	init_obj(gb, wSceneObj7, ADDR_Func_b45f5 + 0x23); // call .asm_b460c
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj7State, 0);
	RET();
}

// ld a, c / and a / ret z / sla c / jr c, .init / ld hl, data / jp Func_b586d
// .init: init obj 2, jp LoadOverworldThunderGfx
static void thunder_or_b586d(GB *gb, uint16_t data, uint16_t obj_data)
{
	if (c_is_zero(gb)) {
		RET();
		return;
	}
	gb->cpu.c = alu_sla(gb, gb->cpu.c);
	if (!(gb->cpu.f & FLAG_C)) {
		SET_HL(gb, data);
		JUMP_TO(ADDR_Func_b586d);
		return;
	}
	init_obj(gb, wSceneObj2, obj_data);
	JUMP_TO(ADDR_LoadOverworldThunderGfx);
}

PORT_JUMP(Func_b461b) { thunder_or_b586d(gb, ADDR_Data_855f9, ADDR_Func_b461b + 0x19); }
PORT_JUMP(Func_b4637) { thunder_or_b586d(gb, ADDR_Data_8560e, ADDR_Func_b4637 + 0x19); }

PORT_JUMP(Func_b4653)
{
	gb->cpu.c = alu_sla(gb, gb->cpu.c);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	init_obj(gb, wSceneObj2, ADDR_Func_b4653 + 0xf); // .data
	JUMP_TO(ADDR_LoadOverworldExplosion1Gfx);
}

PORT(Func_b4665)
{
	gb->cpu.a = 0;
	alu_cp(gb, 0, gb->cpu.c); // xor a / cp c
	if (gb->cpu.f & FLAG_Z)
		return;
	init_obj(gb, wSceneObj14, ADDR_Func_b4665 + 0x20); // .data
	if (c_is_80(gb))
		return;
	gb->cpu.a = rd8(gb, w2d011);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	gb->cpu.a = 0x80;
	wr8(gb, wSceneObj14XCoord, gb->cpu.a);
	gb->cpu.a = 0x15;
	wr8(gb, wSceneObj14YCoord, gb->cpu.a);
}

PORT_JUMP(Func_b46a0)
{
	gb->cpu.a = alu_add(gb, gb->cpu.c, gb->cpu.c);
	if (gb->cpu.f & FLAG_Z)
		gb_call(gb, ADDR_Func_b4874);
	if (c_is_80(gb)) {
		init_obj_jump(gb, wSceneObj1, ADDR_Func_b46a0 + 0x36); // .data
		return;
	}
	gb->cpu.c = alu_dec(gb, gb->cpu.c);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d03e, 0);
	wr8(gb, w2d046, 0);

	SET_HL(gb, ADDR_OWSceneFreezeSeaTilemap + 10 * TILEMAP_WIDTH + 15);
	SET_DE(gb, wTilemap + 10 * TILEMAP_WIDTH + 15);
	gb->cpu.c = 0x91;
	gb->cpu.b = BANK_OWSceneFreezeSeaTilemap;
	CALL(CopyFarBytes);
	SET_HL(gb, ADDR_OWSceneFreezeSeaAttrmap + 0 * TILEMAP_WIDTH + 15);
	SET_DE(gb, wAttrmap + 10 * TILEMAP_WIDTH + 15);
	gb->cpu.c = 0x91;
	gb->cpu.b = BANK_OWSceneFreezeSeaAttrmap;
	JUMP(CopyFarBytes);
}

PORT_JUMP(Func_b46d9)
{
	if (!c_is_zero(gb)) {
		alu_cp(gb, gb->cpu.a, 0x80);
		if (!(gb->cpu.f & FLAG_Z)) {
			SET_HL(gb, ADDR_Data_8563e);
			JUMP_TO(ADDR_Func_b586d);
			return;
		}
		// .asm_b46f0
		init_obj(gb, wSceneObj6, ADDR_Func_b46d9 + 0x33); // call .asm_b46e7
		init_obj(gb, wSceneObj15, ADDR_Func_b46d9 + 0x36); // .data_2
		SET_HL(gb, wAttrmap + 10 * TILEMAP_WIDTH + 3);
		gb->cpu.b = 4;
		gb_call(gb, ADDR_SetBGMapBytesPriority);
		SET_HL(gb, wAttrmap + 11 * TILEMAP_WIDTH + 3);
		gb->cpu.b = 4;
		JUMP_TO(ADDR_SetBGMapBytesPriority);
		return;
	}
	init_obj_jump(gb, wSceneObj6, ADDR_Func_b46d9 + 0x33); // .data_1
}

PORT_JUMP(Func_b4712)
{
	if (c_is_zero(gb)) {
		RET();
		return;
	}
	// .Func_b4757
	gb->cpu.a = rd8(gb, wTempOWSceneAction);
	alu_cp(gb, gb->cpu.a, UNLOCK_LEVEL);
	if (!(gb->cpu.f & FLAG_Z)) {
		init_obj(gb, wSceneObj7, ADDR_Func_b4712 + 0x42); // .data_2
		gb->cpu.a = rd8(gb, w2d011);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			gb->cpu.a = 0x13;
			wr8(gb, wSceneObj7State, gb->cpu.a);
		}
	}
	if (c_is_80(gb)) {
		init_obj(gb, wSceneObj1, ADDR_Func_b4712 + 0x3f); // .data_1
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wSceneObj7State, 0);
		JUMP_TO(ADDR_LoadOverworldExplosion2Gfx);
		return;
	}
	static const uint16_t data[] = {
		ADDR_Data_85af4, ADDR_Data_85afd, ADDR_Data_85b0a, ADDR_Data_85b1b, ADDR_Data_85b2c,
	};
	for (unsigned i = 0; i < sizeof(data) / sizeof(data[0]); i++) {
		SET_HL(gb, data[i]);
		gb_call(gb, ADDR_Func_b586d);
	}
	SET_HL(gb, ADDR_Data_85b3d);
	JUMP_TO(ADDR_Func_b586d);
}

PORT_JUMP(Func_b4771)
{
	gb->cpu.c = alu_sla(gb, gb->cpu.c);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	init_obj(gb, wSceneObj2, ADDR_Func_b4771 + 0x1b); // .data_1
	init_obj(gb, wSceneObj1, ADDR_Func_b4771 + 0x1e); // .data_2
	gb_call(gb, ADDR_LoadOverworldExplosion3Gfx);
	JUMP_TO(ADDR_LoadOverworldSmokeGfx);
}

PORT_JUMP(Func_b4792)
{
	if (c_is_zero(gb)) {
		RET();
		return;
	}
	gb->cpu.c = alu_sla(gb, gb->cpu.c);
	if ((gb->cpu.f & FLAG_C) && is_temp_unlockable(gb)) {
		init_obj(gb, wSceneObj10, ADDR_Func_b4792 + 0x50); // .data_1
		init_obj(gb, wSceneObj12, ADDR_Func_b4792 + 0x56); // .data_3
		init_obj(gb, wSceneObj13, ADDR_Func_b4792 + 0x59); // .data_4
		init_obj(gb, wSceneObj14, ADDR_Func_b4792 + 0x5c); // .data_5
		init_obj(gb, wSceneObj15, ADDR_Func_b4792 + 0x5f); // .data_6
		init_obj(gb, wSceneObj11, ADDR_Func_b4792 + 0x62); // .data_7
		gb_call(gb, ADDR_LoadOverworldFireGfx);
	}
	SET_HL(gb, ADDR_Data_85b48);
	JUMP_TO(ADDR_Func_b586d);
}

PORT_JUMP(Func_b47f7)
{
	gb->cpu.a = gb->cpu.c;
	gb->cpu.c = alu_dec(gb, gb->cpu.c);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		init_obj(gb, wSceneObj9, ADDR_Func_b47f7 + 0x1e); // call .asm_b480c
		init_obj(gb, wSceneObj1, ADDR_Func_b47f7 + 0x21); // .data_2
		JUMP_TO(ADDR_LoadOverworldTorchGfx);
		return;
	}
	init_obj_jump(gb, wSceneObj9, ADDR_Func_b47f7 + 0x1e); // .data_1
}

PORT(Func_b481b)
{
	if (c_is_zero(gb))
		return;
	init_obj(gb, wSceneObj5, ADDR_Func_b481b + 0x14); // .data
	gb->cpu.c = alu_sla(gb, gb->cpu.c);
	if (!(gb->cpu.f & FLAG_C))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj5State, 0);
}

PORT_JUMP(Func_b4832)
{
	if (c_is_zero(gb)) {
		RET();
		return;
	}
	gb->cpu.c = alu_dec(gb, gb->cpu.c);
	if (gb->cpu.f & FLAG_Z) {
		init_obj_jump(gb, wSceneObj8, ADDR_Func_b4832 + 0x3b); // .data_1
		return;
	}
	init_obj(gb, wSceneObj8, ADDR_Func_b4832 + 0x3b); // call .asm_b4864
	gb->cpu.a = alu_add(gb, rd8(gb, wSceneObj8YCoord), 0x08);
	wr8(gb, wSceneObj8YCoord, gb->cpu.a);
	init_obj(gb, wSceneObj2, ADDR_Func_b4832 + 0x3e); // .data_2
	SET_HL(gb, wAttrmap + 7 * TILEMAP_WIDTH + 14);
	gb->cpu.b = 3;
	gb_call(gb, ADDR_SetBGMapBytesPriority);
	SET_HL(gb, wAttrmap + 8 * TILEMAP_WIDTH + 14);
	gb->cpu.b = 3;
	gb_call(gb, ADDR_SetBGMapBytesPriority);
	SET_HL(gb, wAttrmap + 9 * TILEMAP_WIDTH + 14);
	gb->cpu.b = 2;
	JUMP_TO(ADDR_SetBGMapBytesPriority);
}

// unreferenced
PORT(Func_b4895)
{
	uint16_t hl = REG_HL(gb), de = REG_DE(gb);
	gb->cpu.a = rd8(gb, de);
	wr8(gb, hl++, gb->cpu.a);
	de++;
	gb->cpu.a = rd8(gb, de);
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
	SET_DE(gb, de);
	CALL(GetOWCoordInTilemap);
	de = (uint16_t)(REG_DE(gb) + 1);
	SET_DE(gb, de);
	gb->cpu.a = rd8(gb, de);
	wr8(gb, REG_HL(gb), gb->cpu.a);
	gb->cpu.h = alu_dec(gb, gb->cpu.h);
	gb->cpu.h = alu_dec(gb, gb->cpu.h);
	gb->cpu.h = alu_dec(gb, gb->cpu.h);
	de++;
	SET_DE(gb, de);
	gb->cpu.a = rd8(gb, de);
	wr8(gb, REG_HL(gb), gb->cpu.a);
}

PORT_JUMP(Func_b4c57)
{
	gb_call(gb, ADDR_UpdateCurOWObjAnimation);
	JUMP_TO(ADDR_Func_b4c5a); // fallthrough
}

PORT_JUMP(Func_b4da0)
{
	gb_call(gb, ADDR_UpdateCurOWObjAnimation);
	JUMP_TO(ADDR_Func_b4da3); // fallthrough
}

PORT_JUMP(Func_b4dc0)
{
	SET_DE(gb, ADDR_Frameset_a97d4);
	gb_call(gb, ADDR_UpdateCurOWObjAnimation);
	gb->cpu.a = rd8(gb, wOWAnimationFinished);
	alu_cp(gb, gb->cpu.a, 0xff);
	if (gb->cpu.f & FLAG_Z)
		gb_call(gb, ADDR_Func_b4dd4);
	SET_DE(gb, ADDR_OAM_a9161);
	JUMP_TO(ADDR_LoadCurSceneObjSpriteWithScroll);
}

// ld hl, state / ld b, n / jp WaitOWObjState
static void wait_obj_state(GB *gb, uint16_t obj_state, uint8_t n)
{
	SET_HL(gb, obj_state);
	gb->cpu.b = n;
	JUMP_TO(ADDR_WaitOWObjState);
}

// play_sfx sfx / jp AdvanceOWFunc
static void sfx_advance(GB *gb, uint16_t sfx)
{
	play_sfx(gb, sfx);
	JUMP_TO(ADDR_AdvanceOWFunc);
}

PORT_JUMP(NOWFunc_Garlic) { JUMP_TO(ADDR_EndOWFunc); }

PORT_JUMP(NOWFunc_MagicSeed) { ow_func_jumptable(gb, ADDR_NOWFunc_MagicSeed); }
PORT_LOCAL_JUMP(NOWFunc_MagicSeed, Wait1) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(NOWFunc_MagicSeed, PlayFallSFX) { sfx_advance(gb, SFX_119); }
PORT_LOCAL_JUMP(NOWFunc_MagicSeed, SeedFall)
{
	SET_HL(gb, wOWFuncCounter);
	gb_call(gb, ADDR_GetSceneFallingYOffset);
	SET_HL(gb, wSceneObj4YCoord);
	gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, wSceneObj4YCoord));
	wr8(gb, wSceneObj4YCoord, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x4c);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	set_obj_state_sfx_advance(gb, 0x08, wSceneObj4State, SFX_014);
}
PORT_LOCAL_JUMP(NOWFunc_MagicSeed, WaitSeedFall) { wait_obj_state(gb, wSceneObj4State, 0x09); }
PORT_LOCAL_JUMP(NOWFunc_MagicSeed, Wait2) { wait_ow_func(gb, 30); }
PORT_LOCAL_JUMP(NOWFunc_MagicSeed, GrowStalk) { set_obj_state_sfx_advance(gb, 0x0a, wSceneObj4State, SFX_11A); }
PORT_LOCAL_JUMP(NOWFunc_MagicSeed, WaitGrowStalk) { wait_obj_state(gb, wSceneObj4State, 0x0b); }

// NOWFunc_SummonSnake.MoveNotes
static void move_notes(GB *gb, uint16_t flag)
{
	gb->cpu.a = rd8(gb, flag);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	SET_HL(gb, wSceneObj1XCoord);
	wr8(gb, wSceneObj1XCoord, alu_dec(gb, rd8(gb, wSceneObj1XCoord)));
	gb->cpu.a = alu_add(gb, rd8(gb, wSceneObj1XCoord), 0x18);
	alu_cp(gb, gb->cpu.a, 0xf0);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, flag, 0);
}

PORT_JUMP(NOWFunc_SummonSnake)
{
	move_notes(gb, wSceneObj1Unk7);
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_NOWFunc_SummonSnake + 7));
}
PORT_LOCAL_JUMP(NOWFunc_SummonSnake, Wait1) { wait_ow_func(gb, 8); }
PORT_LOCAL_JUMP(NOWFunc_SummonSnake, PlayFluteSFX)
{
	gb->cpu.a = 0x01;
	wr8(gb, wSceneObj1Unk7, gb->cpu.a);
	sfx_advance(gb, SFX_118);
}
PORT_LOCAL_JUMP(NOWFunc_SummonSnake, WaitNotesAndShowSnake)
{
	SET_HL(gb, wSceneObj1XCoord);
	gb->cpu.a = 0x25;
	alu_cp(gb, gb->cpu.a, rd8(gb, wSceneObj1XCoord));
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	set_obj_state_sfx_advance(gb, 0x0c, wSceneObj2State, SFX_TEMPLE_ROCK);
}
// ld a, [wSceneObj2Frame] / cp n / ret nz / jp AdvanceOWFunc
static void wait_obj2_frame(GB *gb, uint8_t n)
{
	gb->cpu.a = rd8(gb, wSceneObj2Frame);
	alu_cp(gb, gb->cpu.a, n);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(NOWFunc_SummonSnake, WaitSnakeComeOut) { wait_obj2_frame(gb, 0x37); }
PORT_LOCAL_JUMP(NOWFunc_SummonSnake, WaitSnakeComeIn) { wait_obj2_frame(gb, 0x35); }

PORT_JUMP(NOWFunc_PlayMusicBox) { ow_func_jumptable(gb, ADDR_NOWFunc_PlayMusicBox); }
PORT_LOCAL_JUMP(NOWFunc_PlayMusicBox, Wait1) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(NOWFunc_PlayMusicBox, PlayMusicBox)
{
	gb->cpu.a = rd8(gb, wOWSceneActionParam);
	alu_cp(gb, gb->cpu.a, NOWFUNC_GOLD_MUSIC_BOX);
	sfx_advance(gb, (gb->cpu.f & FLAG_Z) ? SFX_122 : SFX_11F);
}
PORT_LOCAL_JUMP(NOWFunc_PlayMusicBox, Wait2) { wait_ow_func(gb, 254); }

PORT_JUMP(NOWFunc_Crayon) { ow_func_jumptable(gb, ADDR_NOWFunc_Crayon); }
PORT_LOCAL_JUMP(NOWFunc_Crayon, Wait1) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(NOWFunc_Crayon, Wait4) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(NOWFunc_Crayon, PlayFallSFX) { sfx_advance(gb, SFX_119); }
PORT_LOCAL_JUMP(NOWFunc_Crayon, CrayonFall)
{
	SET_HL(gb, wOWFuncCounter);
	gb_call(gb, ADDR_GetSceneFallingYOffset);
	SET_HL(gb, wSceneObj5YCoord);
	gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, wSceneObj5YCoord));
	wr8(gb, wSceneObj5YCoord, gb->cpu.a);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = alu_add(gb, gb->cpu.a, (uint8_t)-6);
	wr8(gb, wSceneObj1YCoord, gb->cpu.a);
	gb->cpu.a = gb->cpu.c;
	alu_cp(gb, gb->cpu.a, 0x40);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj1State, 0);
	sfx_advance(gb, SFX_014);
}
PORT_LOCAL_JUMP(NOWFunc_Crayon, Wait2) { wait_ow_func(gb, 4); }
PORT_LOCAL_JUMP(NOWFunc_Crayon, CrayonLift)
{
	gb->cpu.a = 0x1e;
	SET_HL(gb, wSceneObj5State);
	CALL(SetSceneObjState);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(NOWFunc_Crayon, WaitCrayonLift) { wait_obj_state(gb, wSceneObj5State, 0x20); }
PORT_LOCAL_JUMP(NOWFunc_Crayon, Wait3) { wait_ow_func(gb, 10); }
PORT_LOCAL_JUMP(NOWFunc_Crayon, Scribble)
{
	SET_DE(gb, ADDR_Data_b6294);
	SET_HL(gb, wSceneObj5);
	CALL(ApplyOWMovement);
	gb->cpu.a = rd8(gb, wSceneObj5Unk7);
	alu_cp(gb, gb->cpu.a, 0x0a);
	if (gb->cpu.f & FLAG_Z) {
		play_sfx(gb, SFX_12D);
		RET();
		return;
	}
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(NOWFunc_Crayon, AdvanceIfCollectedAllCrayons)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wCrayonFlags), ALL_CRAYONS);
	alu_cp(gb, gb->cpu.a, ALL_CRAYONS);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_AdvanceOWFunc);
		return;
	}
	gb->cpu.a = 0x0c;
	wr8(gb, w2d062, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWFuncCounter, 0);
	RET();
}
PORT_LOCAL_JUMP(NOWFunc_Crayon, LoadNewOWPal)
{
	SET_HL(gb, wTempPals1);
	SET_DE(gb, wTempBGPals);
	gb->cpu.b = 8 * PAL_SIZE;
	CALL(CopyHLToDE);
	SET_HL(gb, ADDR_Pals_84500);
	SET_DE(gb, wTempPals1);
	gb->cpu.c = 8 * PAL_SIZE;
	gb->cpu.b = BANK_Pals_84500;
	CALL(CopyFarBytes);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPalConfig1TotalSteps, 0);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(NOWFunc_Crayon, FadePal)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wOWFuncCounter), 7);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb_call(gb, ADDR_Func_b587d);
	RET();
}

PORT_JUMP(WOWFunc_ElevatorWorking) { ow_func_jumptable(gb, ADDR_WOWFunc_ElevatorWorking); }
PORT_LOCAL_JUMP(WOWFunc_ElevatorWorking, Wait1) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(WOWFunc_ElevatorWorking, SetElevatorWorking)
{
	gb->cpu.a = 0x06;
	SET_HL(gb, wSceneObj8State);
	CALL(SetSceneObjState);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(WOWFunc_ElevatorWorking, Wait2) { wait_ow_func(gb, 200); }

// MagicSeed .SeedFall: obj 4 falls until y >= limit, then gets state
static void seed_fall(GB *gb, uint8_t limit, uint8_t state)
{
	SET_HL(gb, wOWFuncCounter);
	gb_call(gb, ADDR_GetSceneFallingYOffset);
	SET_HL(gb, wSceneObj4YCoord);
	gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, wSceneObj4YCoord));
	wr8(gb, wSceneObj4YCoord, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, limit);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	set_obj_state_sfx_advance(gb, state, wSceneObj4State, SFX_014);
}

PORT_JUMP(WOWFunc_MagicSeed) { ow_func_jumptable(gb, ADDR_WOWFunc_MagicSeed); }
PORT_LOCAL_JUMP(WOWFunc_MagicSeed, Wait1) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(WOWFunc_MagicSeed, PlayFallSFX) { sfx_advance(gb, SFX_119); }
PORT_LOCAL_JUMP(WOWFunc_MagicSeed, SeedFall) { seed_fall(gb, 0x5b, 0x0b); }
PORT_LOCAL_JUMP(WOWFunc_MagicSeed, WaitSeedFall) { wait_obj_state(gb, wSceneObj4State, 0x0c); }
PORT_LOCAL_JUMP(WOWFunc_MagicSeed, Wait2) { wait_ow_func(gb, 30); }
PORT_LOCAL_JUMP(WOWFunc_MagicSeed, GrowStalk) { set_obj_state_sfx_advance(gb, 0x0d, wSceneObj4State, SFX_11A); }
PORT_LOCAL_JUMP(WOWFunc_MagicSeed, WaitGrowStalk) { wait_obj_state(gb, wSceneObj4State, 0x0e); }

// .WaitNotesAndShowSnake: once obj 1 is at x, sets the snake's state
static void wait_notes_show_snake(GB *gb, uint8_t x, uint8_t state, uint16_t obj_state)
{
	SET_HL(gb, wSceneObj1XCoord);
	gb->cpu.a = x;
	alu_cp(gb, gb->cpu.a, rd8(gb, wSceneObj1XCoord));
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	set_obj_state_sfx_advance(gb, state, obj_state, SFX_TEMPLE_ROCK);
}

// ld a, [addr] / cp n / ret nz / jp AdvanceOWFunc
static void wait_value(GB *gb, uint16_t addr, uint8_t n)
{
	gb->cpu.a = rd8(gb, addr);
	alu_cp(gb, gb->cpu.a, n);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}

// .PlayFluteSFX
static void play_flute_sfx(GB *gb, uint16_t flag)
{
	gb->cpu.a = 0x01;
	wr8(gb, flag, gb->cpu.a);
	sfx_advance(gb, SFX_118);
}

PORT_JUMP(WOWFunc_SummonSnake)
{
	move_notes(gb, wSceneObj1Unk7); // .MoveNotes
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_WOWFunc_SummonSnake + 7));
}
PORT_LOCAL_JUMP(WOWFunc_SummonSnake, Wait1) { wait_ow_func(gb, 8); }
PORT_LOCAL_JUMP(WOWFunc_SummonSnake, PlayFluteSFX) { play_flute_sfx(gb, wSceneObj1Unk7); }
PORT_LOCAL_JUMP(WOWFunc_SummonSnake, WaitNotesAndShowSnake) { wait_notes_show_snake(gb, 0x26, 0x0f, wSceneObj5State); }
PORT_LOCAL_JUMP(WOWFunc_SummonSnake, WaitSnakeComeOut) { wait_value(gb, wSceneObj5Frame, 0x0a); }

PORT_JUMP(SOWFunc_MagicSeed) { ow_func_jumptable(gb, ADDR_SOWFunc_MagicSeed); }
PORT_LOCAL_JUMP(SOWFunc_MagicSeed, Wait1) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(SOWFunc_MagicSeed, PlayFallSFX) { sfx_advance(gb, SFX_119); }
PORT_LOCAL_JUMP(SOWFunc_MagicSeed, SeedFall) { seed_fall(gb, 0x56, 0x06); }
PORT_LOCAL_JUMP(SOWFunc_MagicSeed, WaitSeedFall) { wait_obj_state(gb, wSceneObj4State, 0x03); }
PORT_LOCAL_JUMP(SOWFunc_MagicSeed, Wait2) { wait_ow_func(gb, 30); }
PORT_LOCAL_JUMP(SOWFunc_MagicSeed, GrowStalk) { set_obj_state_sfx_advance(gb, 0x02, wSceneObj4State, SFX_11A); }
PORT_LOCAL_JUMP(SOWFunc_MagicSeed, WaitGrowStalk) { wait_obj_state(gb, wSceneObj4State, 0x01); }

PORT_JUMP(SOWFunc_YellowMusicBox) { ow_func_jumptable(gb, ADDR_SOWFunc_YellowMusicBox); }
PORT_LOCAL_JUMP(SOWFunc_YellowMusicBox, Wait1) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(SOWFunc_YellowMusicBox, Wait2) { wait_ow_func(gb, 40); }
PORT_LOCAL_JUMP(SOWFunc_YellowMusicBox, PlayMusicBoxSFX) { sfx_advance(gb, SFX_11E); }
PORT_LOCAL_JUMP(SOWFunc_YellowMusicBox, WaitMusic) { wait_ow_func(gb, 254); }
PORT_LOCAL_JUMP(SOWFunc_YellowMusicBox, LowerBridge)
{
	play_sfx(gb, SFX_123);
	gb->cpu.a = 0x08;
	SET_HL(gb, wSceneObj6State);
	CALL(SetSceneObjState);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(SOWFunc_YellowMusicBox, ShowLoweredBridge) { wait_obj_state(gb, wSceneObj6State, 0x09); }

PORT_JUMP(SOWFunc_SummonSnake)
{
	move_notes(gb, wSceneObj7Unk7); // .MoveNotes
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_SOWFunc_SummonSnake + 7));
}
PORT_LOCAL_JUMP(SOWFunc_SummonSnake, Wait1) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(SOWFunc_SummonSnake, PlayFluteSFX) { play_flute_sfx(gb, wSceneObj7Unk7); }
PORT_LOCAL_JUMP(SOWFunc_SummonSnake, WaitNotesAndShowSnake) { wait_notes_show_snake(gb, 0x08, 0x13, wSceneObj7State); }
PORT_LOCAL_JUMP(SOWFunc_SummonSnake, WaitSnakeComeOut) { wait_value(gb, wSceneObj7Frame, 0x31); }

PORT_JUMP(SOWFunc_GreenMusicBox) { ow_func_jumptable(gb, ADDR_SOWFunc_GreenMusicBox); }
PORT_LOCAL_JUMP(SOWFunc_GreenMusicBox, Wait1) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(SOWFunc_GreenMusicBox, Wait2) { wait_ow_func(gb, 40); }
PORT_LOCAL_JUMP(SOWFunc_GreenMusicBox, PlayMusicBoxSFX) { sfx_advance(gb, SFX_120); }
PORT_LOCAL_JUMP(SOWFunc_GreenMusicBox, WaitMusic) { wait_ow_func(gb, 254); }
PORT_LOCAL_JUMP(SOWFunc_GreenMusicBox, BlowUpCave)
{
	SET_HL(gb, ADDR_Data_8560e);
	gb_call(gb, ADDR_Func_b5868);
	set_obj_state_sfx_advance(gb, 0x1b, wSceneObj2State, SFX_129);
}
PORT_LOCAL_JUMP(SOWFunc_GreenMusicBox, HideSmoke)
{
	SET_HL(gb, wSceneObj2State);
	JUMP_TO(ADDR_WaitOWObjDisappear);
}

PORT_JUMP(SOWFunc_FullMoon) { ow_func_jumptable(gb, ADDR_SOWFunc_FullMoon); }
PORT_LOCAL_JUMP(SOWFunc_FullMoon, Wait1) { wait_ow_func(gb, 40); }
PORT_LOCAL_JUMP(SOWFunc_FullMoon, Wait3) { wait_ow_func(gb, 60); }
PORT_LOCAL_JUMP(SOWFunc_FullMoon, PlayGongSFX) { sfx_advance(gb, SFX_105); }
PORT_LOCAL_JUMP(SOWFunc_FullMoon, Wait2) { wait_ow_func(gb, 30); }
PORT_LOCAL_JUMP(SOWFunc_FullMoon, MoveMoon)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x07);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	wr8(gb, wSceneObj14, alu_dec(gb, rd8(gb, wSceneObj14)));
	SET_HL(gb, wSceneObj14);
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x7b);
	if (gb->cpu.f & FLAG_Z) {
		play_sfx(gb, SFX_11A);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x80);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}

PORT_JUMP(EOWFunc_ShowWarpedVoid) { ow_func_jumptable(gb, ADDR_EOWFunc_ShowWarpedVoid); }
PORT_LOCAL_JUMP(EOWFunc_ShowWarpedVoid, Func_b786d) { wait_ow_func(gb, 60); }
PORT_LOCAL_JUMP(EOWFunc_ShowWarpedVoid, Func_b7872) { wait_ow_func(gb, 160); }
PORT_LOCAL_JUMP(EOWFunc_ShowWarpedVoid, Func_b7877) { set_obj_state_sfx_advance(gb, 0x1b, wSceneObj5State, SFX_128); }
PORT_LOCAL_JUMP(EOWFunc_ShowWarpedVoid, Func_b788a) { wait_obj_state(gb, wSceneObj5State, 0x19); }

PORT_JUMP(EOWFunc_RaisePipe) { ow_func_jumptable(gb, ADDR_EOWFunc_RaisePipe); }
PORT_LOCAL_JUMP(EOWFunc_RaisePipe, Func_b78aa) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(EOWFunc_RaisePipe, Func_b78af) { wait_ow_func(gb, 60); }
PORT_LOCAL_JUMP(EOWFunc_RaisePipe, Func_b78b4)
{
	gb->cpu.a = 0x37;
	wr8(gb, wSceneObj8, gb->cpu.a);
	gb->cpu.a = 0x1d;
	SET_HL(gb, wSceneObj8State);
	CALL(SetSceneObjState);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(EOWFunc_RaisePipe, Func_b78c4)
{
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, 0x10);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	sfx_advance(gb, SFX_FAT_WALK);
}
PORT_LOCAL_JUMP(EOWFunc_RaisePipe, Func_b78d5) { wait_obj_state(gb, wSceneObj8State, 0x1c); }

// ld a, [addr] / and a: true if nonzero
static bool nonzero(GB *gb, uint16_t addr)
{
	gb->cpu.a = rd8(gb, addr);
	return alu_and(gb, gb->cpu.a, gb->cpu.a) != 0;
}

// ld a, state / ld hl, obj_state / call SetSceneObjState
static void set_state(GB *gb, uint8_t state, uint16_t obj_state)
{
	gb->cpu.a = state;
	SET_HL(gb, obj_state);
	CALL(SetSceneObjState);
}

PORT(Func_b7ac0)
{
	if (nonzero(gb, wCurEvent))
		return;
	SET_HL(gb, wSceneObj8Unk7);
	if (!nonzero(gb, wSceneObj8Unk7)) {
		if (nonzero(gb, wSceneObj7State))
			return;
		if (nonzero(gb, wOWPalTransitionState))
			return;
		wr8(gb, wSceneObj8Unk7, alu_inc(gb, rd8(gb, wSceneObj8Unk7)));
	}
	// .asm_b7ad7
	if (nonzero(gb, w2d011)) {
		// .asm_b7af0
		gb->cpu.a = 0x08;
		wr8(gb, wSceneObj8State, gb->cpu.a);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wSceneObj8Duration, 0);
	} else {
		gb->cpu.a = rd8(gb, wOWPalTransitionState);
		alu_cp(gb, gb->cpu.a, 0x02);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		gb->cpu.a = 0x07;
	}
	// .asm_b7ae5
	SET_HL(gb, wSceneObj7State);
	CALL(SetSceneObjState);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj8Unk7, 0);
}

PORT(Func_b7afb)
{
	if (nonzero(gb, wCurEvent))
		return;
	SET_HL(gb, wSceneObj8Unk7);
	if (!nonzero(gb, wSceneObj8Unk7)) {
		if (nonzero(gb, wOWPalTransitionState))
			return;
		wr8(gb, wSceneObj8Unk7, alu_inc(gb, rd8(gb, wSceneObj8Unk7)));
	}
	// .asm_b7b0d
	if (nonzero(gb, w2d011)) {
		gb->cpu.a = rd8(gb, wOWPalTransitionState);
		alu_cp(gb, gb->cpu.a, 0x02);
		if (!(gb->cpu.f & FLAG_Z))
			return;
	}
	// .asm_b7b19
	gb->cpu.a = 0x06;
	wr8(gb, wSceneObj8State, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj8Duration, 0);
	wr8(gb, wSceneObj8Unk7, 0);
}

// Func_b7b4e / Func_b7c84: z if no event, flag is 0 and the level is lvl_1,
// else nz (z/c flags from the last cp)
static void level_check(GB *gb, uint16_t flag, uint8_t lvl_1, uint8_t lvl_2)
{
	if (nonzero(gb, wCurEvent))
		return;
	if (nonzero(gb, flag))
		return;
	gb->cpu.a = rd8(gb, wMapSideLevelID);
	alu_cp(gb, gb->cpu.a, lvl_1);
	if (gb->cpu.f & FLAG_Z)
		return;
	alu_cp(gb, gb->cpu.a, lvl_2);
}

PORT(Func_b7b4e) { level_check(gb, w2d12a, 0x01, 0x02); }
PORT(Func_b7c84) { level_check(gb, w2d12e, 0x00, 0x01); }

// Func_b7b26 / Func_b7c5c
static void level_timer(GB *gb, uint16_t check, uint16_t counter, uint16_t flag,
	uint8_t state_nz, uint8_t state_z, uint16_t obj_state)
{
	gb_call(gb, check);
	SET_HL(gb, counter);
	if (!(gb->cpu.f & FLAG_Z)) {
		wr8(gb, counter, alu_inc(gb, rd8(gb, counter)));
		gb->cpu.a = rd8(gb, counter);
		alu_cp(gb, gb->cpu.a, 0x78);
		if (gb->cpu.f & FLAG_C)
			return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, counter, 0);
	wr8(gb, flag, 0);
	gb_call(gb, check);
	gb->cpu.b = state_nz;
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = 0x01;
		wr8(gb, flag, gb->cpu.a);
		gb->cpu.b = state_z;
	}
	gb->cpu.a = gb->cpu.b;
	SET_HL(gb, obj_state);
	CALL(SetSceneObjState);
}

PORT(Func_b7b26) { level_timer(gb, ADDR_Func_b7b4e, w2d12b, w2d12a, 0x13, 0x10, wSceneObj5State); }
PORT(Func_b7c5c) { level_timer(gb, ADDR_Func_b7c84, w2d12f, w2d12e, 0x1d, 0x14, wSceneObj7State); }

PORT(Func_b7be9)
{
	gb->cpu.a = rd8(gb, wOWSceneAction);
	alu_cp(gb, gb->cpu.a, UNLOCK_LEVEL);
	if (!(gb->cpu.f & FLAG_Z)) {
		alu_cp(gb, gb->cpu.a, HIGHLIGHT_LEVEL);
		if (!(gb->cpu.f & FLAG_Z)) {
			nonzero(gb, wCurEvent);
			return;
		}
	}
	set_state(gb, 0x00, wSceneObj7State);
	gb->cpu.a = 0x01;
	alu_and(gb, gb->cpu.a, gb->cpu.a);
}

// Func_b7ba9 / Func_b7bc9
static void day_night_state(GB *gb, bool night, uint8_t state)
{
	gb_call(gb, ADDR_Func_b7be9);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	if (nonzero(gb, w2d011) == night) {
		gb->cpu.a = rd8(gb, wOWPalTransitionState);
		alu_cp(gb, gb->cpu.a, 0x02);
		if (!(gb->cpu.f & FLAG_Z))
			return;
	} else {
		if (nonzero(gb, wOWPalTransitionState))
			return;
	}
	set_state(gb, state, wSceneObj7State);
}

PORT(Func_b7ba9) { day_night_state(gb, true, 0x13); }
PORT(Func_b7bc9) { day_night_state(gb, false, 0x12); }

PORT(Func_b7c05)
{
	if (nonzero(gb, wCurEvent))
		return;
	SET_HL(gb, w2d12b);
	wr8(gb, w2d12b, alu_inc(gb, rd8(gb, w2d12b)));
	if (!(gb->cpu.f & FLAG_Z))
		return;
	set_state(gb, 0x1a, wSceneObj5State);
}

PORT(Func_b7c97)
{
	if (nonzero(gb, w2d011))
		return;
	set_state(gb, 0x1f, wSceneObj15State);
}

PORT(Func_b7ca5)
{
	if (!nonzero(gb, w2d011))
		return;
	set_state(gb, 0x1e, wSceneObj15State);
}

// dec [wSceneObj14YCoord] / inc l / inc [hl]
static void move_obj14_up_right(GB *gb)
{
	SET_HL(gb, wSceneObj14YCoord);
	wr8(gb, wSceneObj14YCoord, alu_dec(gb, rd8(gb, wSceneObj14YCoord)));
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	wr8(gb, REG_HL(gb), alu_inc(gb, rd8(gb, REG_HL(gb))));
}

// ld hl, wSceneObj14YCoord / ld [hl], $25 / inc l / ld [hl], $70
static void reset_obj14(GB *gb)
{
	SET_HL(gb, wSceneObj14YCoord);
	wr8(gb, wSceneObj14YCoord, 0x25);
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	wr8(gb, REG_HL(gb), 0x70);
}

PORT(Func_b7cb3)
{
	if (nonzero(gb, wCurEvent))
		return;
	if (nonzero(gb, w2d07c)) {
		// .asm_b7cc0
		gb->cpu.a = rd8(gb, wSceneObj14XCoord);
		alu_cp(gb, gb->cpu.a, 0x80);
		if (gb->cpu.f & FLAG_Z)
			return;
		if (!nonzero(gb, w2d07d)) {
			// .Func_b7cda
			SET_HL(gb, w2d07d);
			wr8(gb, w2d07d, alu_inc(gb, rd8(gb, w2d07d)));
			reset_obj14(gb);
		}
		gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		move_obj14_up_right(gb);
		return;
	}
	// .asm_b7ce7
	gb->cpu.a = rd8(gb, wSceneObj14XCoord);
	alu_cp(gb, gb->cpu.a, 0x70);
	if (gb->cpu.f & FLAG_Z)
		return;
	if (!nonzero(gb, w2d07d)) {
		// .Func_b7d0d
		SET_HL(gb, w2d07d);
		wr8(gb, w2d07d, alu_inc(gb, rd8(gb, w2d07d)));
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	move_obj14_up_right(gb);
	gb->cpu.a = rd8(gb, REG_HL(gb));
	alu_cp(gb, gb->cpu.a, 0x88);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	reset_obj14(gb);
}

// ld hl, addr / n times inc [hl] (n > 0) or dec [hl] (n < 0)
static void bump(GB *gb, uint16_t addr, int n)
{
	SET_HL(gb, addr);
	for (; n > 0; n--)
		wr8(gb, addr, alu_inc(gb, rd8(gb, addr)));
	for (; n < 0; n++)
		wr8(gb, addr, alu_dec(gb, rd8(gb, addr)));
}

// Func_b71fc.asm_b7304
static void func_b7304(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWUIObj1Attributes, 0);
	SET_HL(gb, wOWUIObj1Frame);
	wr8(gb, wOWUIObj1Frame, alu_inc(gb, rd8(gb, wOWUIObj1Frame)));
}

// Func_b71fc.Func_b7273: false if it jumped out of its table
static bool func_b7273(GB *gb)
{
	SET_HL(gb, wOWUIObj1Frame);
	gb->cpu.a = rd8(gb, wOWUIObj1Frame);
	SET_HL(gb, wOWUIObj1Attributes);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return true;
	wr8(gb, wOWUIObj1Attributes, alu_inc(gb, rd8(gb, wOWUIObj1Attributes)));
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	uint16_t target = jumptable(gb, ADDR_Func_b71fc + 0x80);
	uint8_t limit, sub;
	switch (target - ADDR_Func_b71fc) {
	case 0xcc: limit = 3; sub = 0; goto wait_attr;   // .Func_b72c8
	case 0xd4: limit = 3; sub = 4; goto wait_attr;   // .Func_b72d0
	case 0xe4: limit = 1; sub = 0; goto wait_attr;   // .Func_b72e0
	case 0xec: limit = 1; sub = 0x10; goto wait_attr; // .Func_b72e8
	case 0xfc: // .Func_b72f8
		play_sfx(gb, SFX_01F);
		bump(gb, wSCY, -1);
		break;
	case 0x104: bump(gb, wSCY, -1); break; // .Func_b7300
	case 0x10f: bump(gb, wSCY, 1); break; // .Func_b730b
	case 0x115: // .Func_b7311
		bump(gb, wSCY, -4);
		play_sfx(gb, SFX_01F);
		break;
	case 0x126: bump(gb, wSCY, 4); break; // .Func_b7322
	case 0x12f: bump(gb, wSCX, -4); break; // .Func_b732b
	case 0x138: bump(gb, wSCX, -2); break; // .Func_b7334
	case 0x13f: // .Func_b733b
		play_sfx(gb, SFX_063);
		bump(gb, wSCX, 4);
		break;
	case 0x147: bump(gb, wSCX, 4); break; // .Func_b7343
	case 0x150: // .Func_b734c
		bump(gb, wSCX, 4);
		func_b7304(gb);
		goto clear_frame;
	case 0x160: bump(gb, wSCX, 2); break; // .Func_b735c
	case 0x167: // .Func_b7363
		play_sfx(gb, SFX_063);
		bump(gb, wSCX, 1);
		break;
	case 0x16f: bump(gb, wSCX, 1); break; // .asm_b736b
	case 0x175: // .Func_b7371
		bump(gb, wSCX, 1);
		func_b7304(gb);
		goto clear_frame;
	case 0x17d: bump(gb, wSCX, -8); break; // .Func_b7379
	case 0x18b: bump(gb, wSCX, 8); break; // .Func_b7387
	default:
		gb_push16(gb, ADDR_Func_b71fc + 3); // return address of call .Func_b7273
		JUMP_TO(target);
		return false;
	}
	func_b7304(gb);
	return true;
wait_attr:
	gb->cpu.a = rd8(gb, wOWUIObj1Attributes);
	alu_cp(gb, gb->cpu.a, limit);
	if (gb->cpu.f & FLAG_C)
		return true;
	if (sub) {
		gb->cpu.a = alu_sub(gb, rd8(gb, wOWUIObj1Frame), sub);
		wr8(gb, wOWUIObj1Frame, gb->cpu.a);
	}
	func_b7304(gb);
	return true;
clear_frame:
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWUIObj1Frame, 0);
	return true;
}

PORT_JUMP(Func_b71fc)
{
	if (!func_b7273(gb))
		return;
	SET_HL(gb, wOWUIObj1YCoord);
	gb->cpu.a = rd8(gb, wOWUIObj1YCoord);
	SET_HL(gb, wOWUIObj1XCoord);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		RET();
		return;
	}
	wr8(gb, wOWUIObj1XCoord, alu_inc(gb, rd8(gb, wOWUIObj1XCoord)));
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	uint16_t target = jumptable(gb, ADDR_Func_b71fc + 0xc);
	uint8_t frame;
	switch (target - ADDR_Func_b71fc) {
	case 0x22: // .Func_b721e
		RET();
		return;
	case 0x23: frame = 0x01; break; // .Func_b721f
	case 0x35: // .Func_b7231
	case 0x4d: // .Func_b7249
		gb->cpu.a = rd8(gb, wOWUIObj1Frame);
		alu_cp(gb, gb->cpu.a, (target - ADDR_Func_b71fc) == 0x35 ? 0x01 : 0x11);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wOWUIObj1Frame, 0);
			wr8(gb, wOWUIObj1YCoord, 0);
		}
		RET();
		return;
	case 0x43: // .Func_b723f
		gb->cpu.a = rd8(gb, wOWUIObj1Frame);
		alu_cp(gb, gb->cpu.a, 0x01);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
		frame = 0x11;
		break;
	case 0x49: frame = 0x11; break; // .Func_b7245
	case 0x5b: frame = 0x15; break; // .Func_b7257
	case 0x69: frame = 0x1e; break; // .Func_b7265
	case 0x5f: // .Func_b725b
	case 0x6d: // .Func_b7269
		gb->cpu.a = rd8(gb, wOWUIObj1Frame);
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wOWUIObj1YCoord, 0);
		}
		RET();
		return;
	default:
		JUMP_TO(target);
		return;
	}
	// .asm_b7221
	gb->cpu.a = frame;
	wr8(gb, wOWUIObj1Frame, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWUIObj1Attributes, 0);
	SET_HL(gb, wOWUIObj1YCoord);
	wr8(gb, wOWUIObj1YCoord, alu_inc(gb, rd8(gb, wOWUIObj1YCoord)));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWUIObj1XCoord, 0);
	RET();
}

PORT(Func_b6055)
{
	if (!nonzero(gb, wOWUIObj2Attributes))
		return;
	SET_HL(gb, wOWUIObj2Duration);
	wr8(gb, wOWUIObj2Duration, alu_inc(gb, rd8(gb, wOWUIObj2Duration)));
	gb->cpu.a = rd8(gb, wOWUIObj2Duration);
	alu_cp(gb, gb->cpu.a, 0x1a);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWUIObj2Duration, 0);
	play_sfx(gb, SFX_076);
}

// fills b pals' colours at hl with the flash colour
PORT(Func_b69a6)
{
	uint16_t hl = REG_HL(gb);
	do {
		gb->cpu.a = 0x9c;
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.a = 0x73;
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (!(gb->cpu.f & FLAG_Z));
	SET_HL(gb, hl);
}

PORT_JUMP(WOWFunc_RedMusicBox)
{
	// .Func_b608a
	SET_HL(gb, wSceneObj6Unk7);
	if (nonzero(gb, wSceneObj6Unk7)) {
		alu_cp(gb, gb->cpu.a, 0x01);
		if (gb->cpu.f & FLAG_Z)
			play_sfx(gb, SFX_076);
		wr8(gb, wSceneObj6Unk7, alu_inc(gb, rd8(gb, wSceneObj6Unk7)));
		gb->cpu.a = rd8(gb, wSceneObj6Unk7);
		alu_cp(gb, gb->cpu.a, 0x1b);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.a = 0x01;
			wr8(gb, wSceneObj6Unk7, gb->cpu.a);
		}
	}
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_WOWFunc_RedMusicBox + 7));
}
PORT_LOCAL_JUMP(WOWFunc_RedMusicBox, Wait1) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(WOWFunc_RedMusicBox, Func_b60aa) { sfx_advance(gb, SFX_121); }
PORT_LOCAL_JUMP(WOWFunc_RedMusicBox, Func_b60b5) { wait_ow_func(gb, 254); }
PORT_LOCAL_JUMP(WOWFunc_RedMusicBox, Func_b60ba)
{
	set_state(gb, 0x20, wSceneObj6State);
	SET_HL(gb, ADDR_Data_8556e);
	gb_call(gb, ADDR_Func_b5868);
	gb->cpu.a = 0x01;
	wr8(gb, wSceneObj6Unk7, gb->cpu.a);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(WOWFunc_RedMusicBox, Func_b60d0) { wait_ow_func(gb, 23); }
PORT_LOCAL_JUMP(WOWFunc_RedMusicBox, Func_b60d5)
{
	set_state(gb, 0x21, wSceneObj6State);
	SET_HL(gb, wSceneObj6);
	wr8(gb, wSceneObj6, alu_dec(gb, rd8(gb, wSceneObj6)));
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(WOWFunc_RedMusicBox, Func_b60e4) { wait_ow_func(gb, 10); }
PORT_LOCAL_JUMP(WOWFunc_RedMusicBox, Func_b60e9)
{
	set_state(gb, 0x22, wSceneObj6State);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(WOWFunc_RedMusicBox, Func_b60f4)
{
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, 106);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj6Unk7, 0);
	JUMP_TO(ADDR_AdvanceOWFunc);
}

// unreferenced
PORT_JUMP(Func_b5849)
{
	gb->cpu.a = 4;
	JUMP_TO(ADDR_WaitOWFunc); // fallthrough
}

PORT_JUMP(NOWFunc_BottomBar) { ow_func_jumptable(gb, ADDR_NOWFunc_BottomBar); }
PORT_LOCAL_JUMP(NOWFunc_BottomBar, Wait1) { wait_ow_func(gb, 40); }
PORT_LOCAL_JUMP(NOWFunc_BottomBar, Wait4) { wait_ow_func(gb, 60); }
PORT_LOCAL_JUMP(NOWFunc_BottomBar, LoadBottomBarPal)
{
	SET_HL(gb, ADDR_Pals_84000 + 7 * PAL_SIZE);
	SET_DE(gb, wTempPals1 + 7 * PAL_SIZE);
	gb->cpu.b = BANK_Pals_84000;
	gb->cpu.c = 1 * PAL_SIZE;
	CALL(CopyFarBytes);
	wr8(gb, wPalConfig1 + 0, (uint8_t)rBGPI);
	wr8(gb, wPalConfig1 + 1, BGPI_AUTOINC | (7 * PAL_SIZE));
	wr8(gb, wPalConfig1 + 2, 1);
	wr8(gb, wPalConfig1 + 3, (uint8_t)((wTempPals1 + 7 * PAL_SIZE) >> 8));
	gb->cpu.a = (uint8_t)(wTempPals1 + 7 * PAL_SIZE);
	wr8(gb, wPalConfig1 + 4, gb->cpu.a);
	SET_HL(gb, wPalConfig1 + 5);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(NOWFunc_BottomBar, RaiseBBtnIndicator)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wOWFuncCounter), 0x01);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj1);
	wr8(gb, wSceneObj1, alu_dec(gb, rd8(gb, wSceneObj1)));
	gb->cpu.a = rd8(gb, wSceneObj1);
	alu_cp(gb, gb->cpu.a, 0x94);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	sfx_advance(gb, SFX_113);
}
PORT_LOCAL_JUMP(NOWFunc_BottomBar, Wait2) { wait_ow_func(gb, 45); }
PORT_LOCAL_JUMP(NOWFunc_BottomBar, BlinkBBtnIndicator)
{
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	gb->cpu.b = gb->cpu.a;
	bool z = !alu_bit(gb, gb->cpu.a, 3);
	gb->cpu.a = z ? 0x22 : 0x23;
	wr8(gb, wSceneObj1State, gb->cpu.a);
	gb->cpu.a = 47;
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(NOWFunc_BottomBar, StartOpenBottomBar)
{
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, 45);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	gb->cpu.a = 0x23;
	wr8(gb, wSceneObj1State, gb->cpu.a);
	sfx_advance(gb, SFX_113);
}
PORT_LOCAL_JUMP(NOWFunc_BottomBar, Wait3) { wait_ow_func(gb, 10); }
PORT_LOCAL_JUMP(NOWFunc_BottomBar, OpenBottomBar)
{
	bump(gb, wWY, -1);
	gb->cpu.a = rd8(gb, wWY);
	bump(gb, wSceneObj1, -1);
	bump(gb, wSceneObj5YCoord, -1);
	bump(gb, wSceneObj8YCoord, -1);
	bump(gb, wSceneObj9YCoord, -1);
	bump(gb, wSceneObj10YCoord, -1);
	alu_cp(gb, gb->cpu.a, 0x78);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}

#define SOBJ_X (wSceneObj6XCoord - wSceneObj6)
#define SOBJ_FRAME (wSceneObj6Frame - wSceneObj6)
#define SOBJ_STATE (wSceneObj6State - wSceneObj6)

// Func_b542e.asm_b5458 / .asm_b54dd
static void leaf_timer_next(GB *gb, uint16_t counter)
{
	SET_HL(gb, counter);
	wr8(gb, counter, alu_inc(gb, rd8(gb, counter)));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, counter + 1, 0);
}

// Func_b542e.Func_b5435 / .Func_b54ba (start = that label): false if it
// jumped out of its table
static bool leaf_timer(GB *gb, uint16_t start, uint16_t ret, uint16_t counter, uint16_t obj)
{
	SET_HL(gb, counter);
	gb->cpu.a = rd8(gb, counter);
	SET_HL(gb, counter + 1);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return true;
	wr8(gb, counter + 1, alu_inc(gb, rd8(gb, counter + 1)));
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	uint16_t target = jumptable(gb, (uint16_t)(start + 9));
	uint8_t limit, state;
	switch (target - start) {
	case 0x15: limit = 0x78; state = 0x12; break;
	case 0x2c: limit = 0x5a; state = 0x13; break;
	case 0x3c: limit = 0x28; state = 0x10; break;
	case 0x51: limit = 0x5a; state = 0; break;
	case 0x59:
		gb->cpu.a = alu_and(gb, rd8(gb, counter + 1), 0x0f);
		if (!(gb->cpu.f & FLAG_Z))
			return true;
		SET_HL(gb, obj);
		wr8(gb, obj, alu_inc(gb, rd8(gb, obj)));
		gb->cpu.a = rd8(gb, obj);
		alu_cp(gb, gb->cpu.a, 0x48);
		if (!(gb->cpu.f & FLAG_Z))
			return true;
		leaf_timer_next(gb, counter);
		return true;
	case 0x69:
		SET_HL(gb, obj + SOBJ_FRAME);
		gb->cpu.a = rd8(gb, obj + SOBJ_FRAME);
		SET_HL(gb, obj + SOBJ_FRAME - 1);
		alu_cp(gb, gb->cpu.a, 0x46);
		if (!(gb->cpu.f & FLAG_Z))
			return true;
		gb->cpu.l = alu_dec(gb, gb->cpu.l);
		gb->cpu.a = rd8(gb, REG_HL(gb));
		if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
			return true;
		set_state(gb, 0x11, obj + SOBJ_STATE);
		gb->cpu.a = 0x01;
		wr8(gb, counter, gb->cpu.a);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, counter + 1, 0);
		SET_HL(gb, counter + 1);
		return true;
	default:
		gb_push16(gb, ret);
		JUMP_TO(target);
		return false;
	}
	gb->cpu.a = rd8(gb, counter + 1);
	alu_cp(gb, gb->cpu.a, limit);
	if (gb->cpu.f & FLAG_C)
		return true;
	if (state) {
		set_state(gb, state, obj + SOBJ_STATE);
		if (state == 0x10) {
			gb->cpu.a = 0x38;
			wr8(gb, obj, gb->cpu.a); // YCoord
		}
	}
	leaf_timer_next(gb, counter);
	return true;
}

PORT_JUMP(Func_b542e)
{
	if (!leaf_timer(gb, ADDR_Func_b542e + 0x07, ADDR_Func_b542e + 3, w2d12c, wSceneObj6))
		return;
	if (!leaf_timer(gb, ADDR_Func_b542e + 0x8c, ADDR_Func_b542e + 6, w2d12e, wSceneObj7))
		return;
	RET();
}

// ld de, data / ld hl, obj / call ApplyOWMovement: true if it finished (z)
static bool ow_movement(GB *gb, uint16_t data, uint16_t obj)
{
	SET_DE(gb, data);
	SET_HL(gb, obj);
	CALL(ApplyOWMovement);
	return gb->cpu.f & FLAG_Z;
}

// NOWFunc_FallLeaves.HandleLeaf1Movement / .HandleLeaf3Movement
static void handle_leaf_movement(GB *gb, uint16_t counter, uint16_t obj, uint16_t obj2,
	uint16_t flag2, bool sfx)
{
	SET_HL(gb, counter);
	gb->cpu.a = rd8(gb, counter);
	SET_HL(gb, counter + 1);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		// .lift_leaf
		if (!ow_movement(gb, ADDR_Data_b6101, obj))
			return; // hasn't finished movement
		SET_HL(gb, counter);
		wr8(gb, counter, alu_inc(gb, rd8(gb, counter)));
		set_state(gb, 0x15, obj + SOBJ_STATE);
		set_state(gb, 0x15, obj2 + SOBJ_STATE);
		gb->cpu.a = 0x01;
		wr8(gb, flag2, gb->cpu.a);
		gb->cpu.a = rd8(gb, obj + SOBJ_X);
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		wr8(gb, obj + SOBJ_X, gb->cpu.a);
		gb->cpu.a = rd8(gb, obj);
		wr8(gb, obj2, gb->cpu.a);
		gb->cpu.a = rd8(gb, obj + SOBJ_X);
		for (int i = 0; i < 6; i++)
			gb->cpu.a = alu_inc(gb, gb->cpu.a);
		wr8(gb, obj2 + SOBJ_X, gb->cpu.a);
		if (sfx)
			play_sfx(gb, SFX_08E);
		return;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	// .blow_away_leaf
	if (!ow_movement(gb, ADDR_Data_b6196, obj))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, counter, 0);
}

// NOWFunc_FallLeaves.HandleLeaf2Movement / .HandleLeaf4Movement
static void handle_leaf_fall(GB *gb, uint16_t flag, uint16_t obj)
{
	SET_HL(gb, flag);
	gb->cpu.a = rd8(gb, flag);
	SET_HL(gb, flag + 1);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	if (!ow_movement(gb, ADDR_Data_b6239, obj))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, flag, 0);
}

PORT_JUMP(NOWFunc_FallLeaves)
{
	handle_leaf_movement(gb, w2d12c, wSceneObj6, wSceneObj8, w2d130, false);
	handle_leaf_movement(gb, w2d12e, wSceneObj7, wSceneObj9, w2d132, true);
	handle_leaf_fall(gb, w2d130, wSceneObj8);
	handle_leaf_fall(gb, w2d132, wSceneObj9);
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_NOWFunc_FallLeaves + 0x10));
}
PORT_LOCAL_JUMP(NOWFunc_FallLeaves, Wait1) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(NOWFunc_FallLeaves, Wait3) { wait_ow_func(gb, 40); }
PORT_LOCAL_JUMP(NOWFunc_FallLeaves, Func_b5316)
{
	set_state(gb, 0x14, wSceneObj7State);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(NOWFunc_FallLeaves, StartLeaf1MovementAndPlaySFX)
{
	gb->cpu.a = 0x01;
	wr8(gb, w2d12c, gb->cpu.a);
	sfx_advance(gb, SFX_08E);
}
PORT_LOCAL_JUMP(NOWFunc_FallLeaves, Wait2) { wait_ow_func(gb, 8); }
PORT_LOCAL_JUMP(NOWFunc_FallLeaves, StartLeaf3Movement)
{
	gb->cpu.a = 0x01;
	wr8(gb, w2d12e, gb->cpu.a);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(NOWFunc_FallLeaves, WaitLeafMovement)
{
	if (nonzero(gb, w2d12e)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}

// WOWFunc_Rain.UpdatePals
static void rain_update_pals(GB *gb)
{
	if (!nonzero(gb, wPalConfig1TotalSteps))
		return;
	SET_HL(gb, wPalConfig1TotalSteps);
	wr8(gb, wPalConfig1TotalSteps, alu_inc(gb, rd8(gb, wPalConfig1TotalSteps)));
	gb->cpu.a = rd8(gb, wPalConfig1TotalSteps);
	static const struct { uint8_t step; uint16_t pals; } steps[] = {
		{0x09, ADDR_Pals_8698d}, {0x12, ADDR_Pals_8694d}, {0x16, ADDR_Pals_8698d},
		{0x1a, ADDR_Pals_8694d}, {0x1e, ADDR_Pals_8698d}, {0x22, ADDR_Pals_8694d},
		{0x26, ADDR_Pals_8698d}, {0x29, ADDR_Pals_8694d}, {0x2c, ADDR_Pals_84140},
	};
	uint16_t pals = 0;
	for (unsigned i = 0; i < sizeof(steps) / sizeof(steps[0]); i++) {
		alu_cp(gb, gb->cpu.a, steps[i].step);
		if (gb->cpu.f & FLAG_Z) {
			pals = steps[i].pals;
			break;
		}
	}
	if (!pals) {
		alu_cp(gb, gb->cpu.a, 0x82);
		if (gb->cpu.f & FLAG_C)
			return;
		gb->cpu.a = 0x01;
		wr8(gb, wPalConfig1TotalSteps, gb->cpu.a);
		return;
	}
	// .apply_pals
	SET_HL(gb, pals);
	SET_DE(gb, wTempPals1);
	gb->cpu.b = BANK_Pals_84140;
	gb->cpu.c = 8 * PAL_SIZE;
	CALL(CopyFarBytes);
	wr8(gb, wPalConfig1 + 0, (uint8_t)rBGPI);
	wr8(gb, wPalConfig1 + 1, BGPI_AUTOINC | 0);
	wr8(gb, wPalConfig1 + 2, 1 * PAL_SIZE);
	wr8(gb, wPalConfig1 + 3, (uint8_t)(wTempPals1 >> 8));
	gb->cpu.a = (uint8_t)wTempPals1;
	wr8(gb, wPalConfig1 + 4, gb->cpu.a);
	SET_HL(gb, wPalConfig1 + 5);
}

PORT_JUMP(WOWFunc_Rain)
{
	rain_update_pals(gb);
	// .PlayThunderSFX
	SET_HL(gb, w2d13f);
	if (nonzero(gb, w2d13f)) {
		wr8(gb, w2d13f, alu_inc(gb, rd8(gb, w2d13f)));
		gb->cpu.a = rd8(gb, w2d13f);
		alu_cp(gb, gb->cpu.a, 0x1b);
		if (!(gb->cpu.f & FLAG_C)) {
			play_sfx(gb, SFX_076);
			gb->cpu.a = 0x01;
			wr8(gb, w2d13f, gb->cpu.a);
		}
	}
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_WOWFunc_Rain + 0xa));
}
PORT_LOCAL_JUMP(WOWFunc_Rain, Wait3) { wait_ow_func(gb, 60); }
PORT_LOCAL_JUMP(WOWFunc_Rain, InitPalConfig)
{
	gb->cpu.a = 0x5a;
	wr8(gb, wPalConfig1TotalSteps, gb->cpu.a);
	gb->cpu.a = 0x01;
	wr8(gb, w2d13f, gb->cpu.a);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(WOWFunc_Rain, Wait1) { wait_ow_func(gb, 180); }
PORT_LOCAL_JUMP(WOWFunc_Rain, ShowRainClouds)
{
	set_state(gb, 0x01, wSceneObj15State);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(WOWFunc_Rain, WaitRainClouds) { wait_value(gb, wSceneObj15Frame, 0x0e); }
PORT_LOCAL_JUMP(WOWFunc_Rain, FillPool)
{
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, 2);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	SET_HL(gb, ADDR_Data_8550b);
	gb_call(gb, ADDR_Func_b5868);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(WOWFunc_Rain, Wait2) { wait_ow_func(gb, 120); }
PORT_LOCAL_JUMP(WOWFunc_Rain, WaitRainAndHideClouds)
{
	SET_HL(gb, wPalConfig1TotalSteps);
	gb->cpu.a = rd8(gb, wPalConfig1TotalSteps);
	alu_cp(gb, gb->cpu.a, 0x3c);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPalConfig1TotalSteps, 0);
	wr8(gb, w2d13f, 0);
	set_state(gb, 0x04, wSceneObj14State);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(WOWFunc_Rain, WaitCloudsDisappear)
{
	SET_HL(gb, wSceneObj14State);
	JUMP_TO(ADDR_WaitOWObjDisappear);
}

PORT_JUMP(SOWFunc_SendSeeds) { ow_func_jumptable(gb, ADDR_SOWFunc_SendSeeds); }
PORT_LOCAL_JUMP(SOWFunc_SendSeeds, Wait1) { wait_ow_func(gb, 30); }
PORT_LOCAL_JUMP(SOWFunc_SendSeeds, Wait2) { wait_ow_func(gb, 10); }
// .asm_b6606
static void show_seed(GB *gb, uint16_t obj_state)
{
	SET_HL(gb, obj_state);
	gb->cpu.a = 0x06;
	CALL(SetSceneObjState);
	sfx_advance(gb, SFX_TEMPLE_ROCK);
}
PORT_LOCAL_JUMP(SOWFunc_SendSeeds, ShowSeed1) { show_seed(gb, wSceneObj4State); }
PORT_LOCAL_JUMP(SOWFunc_SendSeeds, WaitShowSeed) { wait_ow_func(gb, 30); }
PORT_LOCAL_JUMP(SOWFunc_SendSeeds, ShowSeed2) { show_seed(gb, wSceneObj2State); }
PORT_LOCAL_JUMP(SOWFunc_SendSeeds, ShowSeed3)
{
	gb->cpu.a = 0x01;
	wr8(gb, w2d125, gb->cpu.a);
	show_seed(gb, wSceneObj3State);
}
PORT_LOCAL_JUMP(SOWFunc_SendSeeds, PlayFlyingSFX) { sfx_advance(gb, SFX_07F); }

// SOWFunc_SendSeeds.GetXOffset
static void seeds_get_x_offset(GB *gb, uint16_t hl)
{
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (gb->cpu.f & FLAG_C)
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = 0x00;
	hl = alu_add16(gb, ADDR_SOWFunc_SendSeeds + 0xe5, REG_BC(gb)); // .x_offsets
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
}

// .asm_b6675 / .asm_b6696
static void seeds_start_next(GB *gb, uint16_t flag)
{
	gb->cpu.a = 0x01;
	wr8(gb, flag, gb->cpu.a);
	play_sfx(gb, SFX_07F);
}

// dec [hl] x3 / ld a, [hl] / cp $1a / z: start next / cp $f0 / ret c / clear flag
static void seeds_move_up(GB *gb, uint16_t hl, uint16_t flag, uint16_t next)
{
	bump(gb, hl, -3);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x1a);
	if (gb->cpu.f & FLAG_Z) {
		seeds_start_next(gb, next);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0xf0);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, flag, 0);
}

PORT_LOCAL_JUMP(SOWFunc_SendSeeds, MoveSeeds)
{
	if (nonzero(gb, w2d125)) {
		// .MoveSeed2
		seeds_get_x_offset(gb, w2d125);
		gb->cpu.a = alu_cpl(gb, gb->cpu.a);
		gb->cpu.a = alu_inc(gb, gb->cpu.a); // = -a
		gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, wSceneObj2XCoord));
		wr8(gb, wSceneObj2XCoord, gb->cpu.a);
		seeds_move_up(gb, wSceneObj2XCoord - 1, w2d125, w2d129);
	}
	if (nonzero(gb, w2d129)) // .MoveSeed1
		seeds_move_up(gb, wSceneObj4YCoord, w2d129, w2d127);
	if (nonzero(gb, w2d127)) {
		// .MoveSeed3
		seeds_get_x_offset(gb, w2d127);
		gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, wSceneObj3XCoord));
		wr8(gb, wSceneObj3XCoord, gb->cpu.a);
		bump(gb, wSceneObj3XCoord - 1, -3);
	}
	SET_HL(gb, wSceneObj3XCoord);
	gb->cpu.a = rd8(gb, wSceneObj3XCoord);
	alu_cp(gb, gb->cpu.a, 0xf0);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d127, 0);
	JUMP_TO(ADDR_AdvanceOWFunc);
}

// SOWFunc_RaiseTower.Func_b6a16 / .Func_b6a2e: shakes obj 3
static void tower_shake(GB *gb, uint8_t mask, uint8_t mask2)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), mask);
	if (gb->cpu.f & FLAG_Z) {
		SET_HL(gb, wSceneObj3YCoord);
		gb->cpu.a = rd8(gb, wSceneObj3YCoord);
		wr8(gb, wSceneObj3YCoord, alu_inc(gb, gb->cpu.a));
		return;
	}
	gb->cpu.a = alu_and(gb, gb->cpu.a, mask2);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	SET_HL(gb, wSceneObj3YCoord);
	wr8(gb, wSceneObj3YCoord, alu_dec(gb, rd8(gb, wSceneObj3YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj3YCoord);
	alu_cp(gb, gb->cpu.a, 0x48);
}

PORT_JUMP(SOWFunc_RaiseTower) { ow_func_jumptable(gb, ADDR_SOWFunc_RaiseTower); }
PORT_LOCAL_JUMP(SOWFunc_RaiseTower, Wait1) { wait_ow_func(gb, 30); }
PORT_LOCAL_JUMP(SOWFunc_RaiseTower, Wait4) { wait_ow_func(gb, 40); }
PORT_LOCAL_JUMP(SOWFunc_RaiseTower, StartDust) { set_obj_state_sfx_advance(gb, 0x0b, wSceneObj2State, SFX_11B); }
PORT_LOCAL_JUMP(SOWFunc_RaiseTower, Wait2) { wait_ow_func(gb, 45); }
PORT_LOCAL_JUMP(SOWFunc_RaiseTower, RaiseTower)
{
	tower_shake(gb, 0x07, 0x01);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj3YCoord);
	gb->cpu.a = alu_add(gb, 0x10, rd8(gb, wSceneObj3YCoord));
	wr8(gb, wSceneObj3YCoord, gb->cpu.a);
	SET_HL(gb, wSceneObj3State);
	gb->cpu.a = alu_inc(gb, rd8(gb, wSceneObj3State));
	CALL(SetSceneObjState);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(SOWFunc_RaiseTower, FinishRaiseTower)
{
	tower_shake(gb, 0x07, 0x01);
	alu_cp(gb, gb->cpu.a, 0x50);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(SOWFunc_RaiseTower, Func_b6a0f)
{
	tower_shake(gb, 0x0f, 0x03);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(SOWFunc_RaiseTower, Wait3) { wait_ow_func(gb, 40); }
PORT_LOCAL_JUMP(SOWFunc_RaiseTower, Func_b6a4b)
{
	set_state(gb, 0x00, wSceneObj2State);
	set_state(gb, 0x00, wSceneObj3State);
	SET_HL(gb, ADDR_Data_85579);
	gb_call(gb, ADDR_Func_b5868);
	JUMP_TO(ADDR_AdvanceOWFunc);
}

// SOWFunc_Fan.Func_b6b08 / .Func_b6b62: starts the gust obj
static void fan_gust_start(GB *gb, uint16_t counter, uint16_t obj)
{
	set_state(gb, 0x18, obj + SOBJ_STATE);
	wr8(gb, obj, 0x5c);
	gb->cpu.a = 0x82;
	wr8(gb, obj + 1, gb->cpu.a);
	SET_HL(gb, counter);
	wr8(gb, counter, alu_inc(gb, rd8(gb, counter)));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, counter + 1, 0);
}

// SOWFunc_Fan.Func_b6b22 / .Func_b6b7c: moves the gust obj up right
static void fan_gust_move(GB *gb, uint16_t counter, uint16_t obj)
{
	SET_HL(gb, counter + 1);
	wr8(gb, counter + 1, alu_inc(gb, rd8(gb, counter + 1)));
	gb->cpu.a = rd8(gb, counter + 1);
	alu_cp(gb, gb->cpu.a, 0x02);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, counter + 1, 0);
	SET_HL(gb, obj);
	wr8(gb, obj, alu_dec(gb, rd8(gb, obj)));
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	wr8(gb, REG_HL(gb), alu_inc(gb, rd8(gb, REG_HL(gb))));
	gb->cpu.a = rd8(gb, REG_HL(gb));
	alu_cp(gb, gb->cpu.a, 0x8c);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	SET_HL(gb, counter);
	wr8(gb, counter, alu_inc(gb, rd8(gb, counter)));
}

// ld hl, counter / ld a, [hl] / and a / ret z / dec a / rst JumpTable
// (table right after, at start + 8); -1 = out of the table (jumped there)
static int fan_dispatch(GB *gb, uint16_t start, uint16_t ret, uint16_t counter, int *off)
{
	SET_HL(gb, counter);
	gb->cpu.a = rd8(gb, counter);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return 0;
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	uint16_t target = jumptable(gb, (uint16_t)(start + 8));
	*off = target - ADDR_SOWFunc_Fan;
	(void)ret;
	return 1;
}

PORT_JUMP(SOWFunc_Fan)
{
	int off;
	// .Func_b6af2
	if (fan_dispatch(gb, ADDR_SOWFunc_Fan + 0x8e, 0, w2d122, &off)) {
		switch (off) {
		case 0xa4: fan_gust_start(gb, w2d122, wSceneObj1); break; // .Func_b6b08
		case 0xbe: fan_gust_move(gb, w2d122, wSceneObj1); break; // .Func_b6b22
		case 0xd7: // .Func_b6b3b
			set_state(gb, 0x00, wSceneObj1State);
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, w2d122, 0);
			break;
		default:
			gb_push16(gb, ADDR_SOWFunc_Fan + 3);
			JUMP_TO(ADDR_SOWFunc_Fan + off);
			return;
		}
	}
	// .Func_b6b48
	if (fan_dispatch(gb, ADDR_SOWFunc_Fan + 0xe4, 0, w2d124, &off)) {
		switch (off) {
		case 0xfe: fan_gust_start(gb, w2d124, wSceneObj2); break; // .Func_b6b62
		case 0x118: fan_gust_move(gb, w2d124, wSceneObj2); break; // .Func_b6b7c
		case 0x131: // .Func_b6b95
			set_state(gb, 0x00, wSceneObj2State);
			SET_HL(gb, w2d124);
			wr8(gb, w2d124, alu_inc(gb, rd8(gb, w2d124)));
			break;
		case 0x13e: // .Func_b6ba2
			gb->cpu.a = rd8(gb, wSceneObj8Frame);
			alu_cp(gb, gb->cpu.a, 0x4a);
			if (!(gb->cpu.f & FLAG_Z))
				break;
			set_state(gb, 0x19, wSceneObj8State);
			SET_HL(gb, w2d124);
			wr8(gb, w2d124, alu_inc(gb, rd8(gb, w2d124)));
			gb->cpu.l = alu_inc(gb, gb->cpu.l);
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, REG_HL(gb), 0);
			break;
		case 0x154: // .Func_b6bb8
			SET_HL(gb, w2d125);
			wr8(gb, w2d125, alu_inc(gb, rd8(gb, w2d125)));
			gb->cpu.a = rd8(gb, w2d125);
			alu_cp(gb, gb->cpu.a, 0x18);
			if (!(gb->cpu.f & FLAG_Z))
				break;
			set_state(gb, 0x00, wSceneObj8State);
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, w2d124, 0);
			break;
		default:
			gb_push16(gb, ADDR_SOWFunc_Fan + 6);
			JUMP_TO(ADDR_SOWFunc_Fan + off);
			return;
		}
	}
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_SOWFunc_Fan + 0xa));
}
PORT_LOCAL_JUMP(SOWFunc_Fan, Func_b6a86) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(SOWFunc_Fan, Func_b6a8b) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(SOWFunc_Fan, Func_b6a90) { set_obj_state_sfx_advance(gb, 0x1a, wSceneObj8State, SFX_12F); }
PORT_LOCAL_JUMP(SOWFunc_Fan, Func_b6aa3) { wait_ow_func(gb, 24); }
PORT_LOCAL_JUMP(SOWFunc_Fan, Func_b6aa8)
{
	gb->cpu.a = 0x01;
	wr8(gb, w2d122, gb->cpu.a);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(SOWFunc_Fan, Func_b6ab0)
{
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, 0x0a);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.a = 0x01;
	wr8(gb, w2d124, gb->cpu.a);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(SOWFunc_Fan, Func_b6abe)
{
	gb->cpu.a = rd8(gb, wSceneObj9Frame);
	alu_cp(gb, gb->cpu.a, 0x21);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	set_state(gb, 0x11, wSceneObj9State);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(SOWFunc_Fan, Func_b6acf) { wait_ow_func(gb, 12); }
PORT_LOCAL_JUMP(SOWFunc_Fan, Func_b6ad4)
{
	SET_HL(gb, wOWFuncCounter);
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, 0x08);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWFuncCounter, 0);
	gb->cpu.a = alu_dec(gb, rd8(gb, wSceneObj9YCoord));
	wr8(gb, wSceneObj9YCoord, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x45);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(SOWFunc_Fan, Func_b6aea)
{
	if (nonzero(gb, w2d124)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}

PORT_JUMP(EOWFunc_Explosives) { ow_func_jumptable(gb, ADDR_EOWFunc_Explosives); }
PORT_LOCAL_JUMP(EOWFunc_Explosives, Func_b6d3d) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(EOWFunc_Explosives, Func_b6d42) { wait_ow_func(gb, 40); }
PORT_LOCAL_JUMP(EOWFunc_Explosives, Func_b6d47) { wait_ow_func(gb, 50); }
PORT_LOCAL_JUMP(EOWFunc_Explosives, CheckMapSide)
{
	gb->cpu.a = rd8(gb, wCurMapSide);
	alu_cp(gb, gb->cpu.a, SOUTH);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_AdvanceOWFunc);
		return;
	}
	gb->cpu.a = 0x0e;
	wr8(gb, w2d062, gb->cpu.a);
	RET();
}
// ld a, state / ld hl, wSceneObj2State / call SetSceneObjState /
// .asm_b6d62: play_sfx SFX_129 / jp AdvanceOWFunc
static void explosion(GB *gb, uint8_t state, uint16_t obj_state)
{
	set_obj_state_sfx_advance(gb, state, obj_state, SFX_129);
}
PORT_LOCAL_JUMP(EOWFunc_Explosives, Func_b6d5a) { explosion(gb, 0x1c, wSceneObj2State); }
PORT_LOCAL_JUMP(EOWFunc_Explosives, Func_b6d6d)
{
	SET_HL(gb, wSceneObj2State);
	JUMP_TO(ADDR_WaitOWObjDisappear);
}
// .asm_b6d76
static void explosion_at(GB *gb, uint8_t d, uint8_t e)
{
	gb->cpu.d = d;
	gb->cpu.e = e;
	gb->cpu.a = alu_add(gb, 0x20, d);
	wr8(gb, wSceneObj2YCoord, gb->cpu.a);
	gb->cpu.a = alu_add(gb, 0x44, e);
	wr8(gb, wSceneObj2YCoord + 1, gb->cpu.a);
	explosion(gb, 0x1c, wSceneObj2State);
}
PORT_LOCAL_JUMP(EOWFunc_Explosives, Func_b6d73) { explosion_at(gb, (uint8_t)-5, 4); }
PORT_LOCAL_JUMP(EOWFunc_Explosives, Func_b6d83) { explosion_at(gb, 3, 5); }
PORT_LOCAL_JUMP(EOWFunc_Explosives, Func_b6d88) { explosion(gb, 0x08, wSceneObj2State); }
PORT_LOCAL_JUMP(EOWFunc_Explosives, Func_b6d92) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(EOWFunc_Explosives, Func_b6d97) { wait_ow_func(gb, 10); }
PORT_LOCAL_JUMP(EOWFunc_Explosives, Func_b6d9c)
{
	gb->cpu.b = EAST;
	gb->cpu.d = OWEAST_THE_COLOSSAL_HOLE;
	CALL(LoadLevelName);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPalConfig1TotalSteps, 0);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(EOWFunc_Explosives, Func_b6daa) { explosion(gb, 0x22, wSceneObj1State); }
PORT_LOCAL_JUMP(EOWFunc_Explosives, Func_b6db4)
{
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, 0x14);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	wr8(gb, wSceneObj2YCoord, 0x46);
	gb->cpu.a = 0x50;
	wr8(gb, wSceneObj2YCoord + 1, gb->cpu.a);
	explosion(gb, 0x22, wSceneObj2State);
}
PORT_LOCAL_JUMP(EOWFunc_Explosives, Func_b6dcd)
{
	gb->cpu.a = 0x03;
	wr8(gb, w2d0e0, gb->cpu.a);
	FARCALL(Func_852e5);
	if (nonzero(gb, w2d0e0)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}

// Frameset setters reached through the Func_b4ba2 / Func_b4ccb jumptables
PORT_JUMP(Func_b4be9) { SET_DE(gb, ADDR_Frameset_a9e61); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4bee) { SET_DE(gb, ADDR_Frameset_a9e72); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4bf3) { SET_DE(gb, ADDR_Frameset_a9e75); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4bf8) { SET_DE(gb, ADDR_Frameset_a9e94); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c02) { SET_DE(gb, ADDR_Frameset_a9e8c); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c07) { SET_DE(gb, ADDR_Frameset_a9e91); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c16) { SET_DE(gb, ADDR_Frameset_a9ed3); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c25) { SET_DE(gb, ADDR_Frameset_a9ef1); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c2a) { SET_DE(gb, ADDR_Frameset_a9ee3); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c39) { SET_DE(gb, ADDR_Frameset_a9efd); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c3e) { SET_DE(gb, ADDR_Frameset_a9ee0); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c43) { SET_DE(gb, ADDR_Frameset_a9f02); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c48) { SET_DE(gb, ADDR_Frameset_a9f0d); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c4d) { SET_DE(gb, ADDR_Frameset_a9ecd); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c52) { SET_DE(gb, ADDR_Frameset_a9ed0); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c6d) { SET_DE(gb, ADDR_Frameset_a9eaa); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c72) { SET_DE(gb, ADDR_Frameset_a9eb3); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c77) { SET_DE(gb, ADDR_Frameset_a9ebc); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c7c) { SET_DE(gb, ADDR_Frameset_a9ec5); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c86) { SET_DE(gb, ADDR_Frameset_a9f36); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4c9f) { SET_DE(gb, ADDR_Frameset_a9fa7); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4cae) { SET_DE(gb, ADDR_Frameset_a9fc9); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4cb3) { SET_DE(gb, ADDR_Frameset_a9fbb); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4cb8) { SET_DE(gb, ADDR_Frameset_a9fc2); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4cbd) { SET_DE(gb, ADDR_Frameset_a9fdf); JUMP_TO(ADDR_Func_b4c57); }
PORT_JUMP(Func_b4d0e) { SET_DE(gb, ADDR_Frameset_a96c0); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4d1f) { SET_DE(gb, ADDR_Frameset_a967d); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4d25) { SET_DE(gb, ADDR_Frameset_a9680); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4d2b) { SET_DE(gb, ADDR_Frameset_a9674); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4d3c) { SET_DE(gb, ADDR_Frameset_a96af); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4d4b) { SET_DE(gb, ADDR_Frameset_a96bd); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4d50) { SET_DE(gb, ADDR_Frameset_a96d1); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4d55) { SET_DE(gb, ADDR_Frameset_a969a); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4d5a) { SET_DE(gb, ADDR_Frameset_a96a3); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4d5f) { SET_DE(gb, ADDR_Frameset_a96a6); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4d64) { SET_DE(gb, ADDR_Frameset_a96a9); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4d69) { SET_DE(gb, ADDR_Frameset_a96ac); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4d6e) { SET_DE(gb, ADDR_Frameset_a96d8); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4d91) { SET_DE(gb, ADDR_Frameset_a979f); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4d96) { SET_DE(gb, ADDR_Frameset_a97a2); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4db6) { SET_DE(gb, ADDR_Frameset_a97e1); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4dbb) { SET_DE(gb, ADDR_Frameset_a97ec); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4df1) { SET_DE(gb, ADDR_Frameset_a97fe); JUMP_TO(ADDR_Func_b4da0); }
PORT_JUMP(Func_b4df6) { SET_DE(gb, ADDR_Frameset_a9803); JUMP_TO(ADDR_Func_b4da0); }

PORT_JUMP(Func_b7b61)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	JUMP_TO(jumptable(gb, ADDR_Func_b7b61 + 2));
}

PORT_JUMP(Func_b7c18)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	JUMP_TO(jumptable(gb, ADDR_Func_b7c18 + 2));
}

#define WOW_EARTHQUAKE_B5D0A (ADDR_WOWFunc_Earthquake + 0xa2) // .Func_b5d0a

PORT_JUMP(EOWFunc_Daytime) { ow_func_jumptable(gb, ADDR_EOWFunc_Daytime); }
PORT_LOCAL_JUMP(EOWFunc_Daytime, Func_b78eb) { wait_ow_func(gb, 60); }
PORT_LOCAL_JUMP(EOWFunc_Daytime, Func_b78f0) { wait_ow_func(gb, 60); }
PORT_LOCAL_JUMP(EOWFunc_Daytime, Func_b78f5)
{
	FARCALL(Func_84b97);
	if (nonzero(gb, wOWPalTransitionState)) {
		RET();
		return;
	}
	gb->cpu.ime = false; // di
	FARCALL(VBlank_OWScene);
	gb->cpu.ime = true; // ei
	gb->cpu.ime_delay = 0;
	JUMP_TO(ADDR_AdvanceOWFunc);
}

// ld hl, w2d03e / ld [hli], a (a = n) / xor a / ld [hli], a / ld [hli], a
static void set_2d03e(GB *gb, uint8_t n)
{
	wr8(gb, w2d03e, n);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d03e + 1, 0);
	wr8(gb, w2d03e + 2, 0);
	SET_HL(gb, w2d03e + 3);
}

// start of an earthquake shake: wOWUIObj1YCoord = 1, w2d049 = 1, w2d03e = 3
static void shake_start(GB *gb)
{
	gb->cpu.a = 0x01;
	wr8(gb, wOWUIObj1YCoord, 0x01);
	wr8(gb, w2d049, 0x01);
	gb->cpu.a = 0x03;
	set_2d03e(gb, 0x03);
	JUMP_TO(ADDR_AdvanceOWFunc);
}

// ld a, [wOWFuncCounter] / cp n: true (ret c) if below n
static bool counter_below(GB *gb, uint8_t n)
{
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, n);
	return gb->cpu.f & FLAG_C;
}

PORT_JUMP(SOWFunc_Earthquake)
{
	gb_call(gb, ADDR_Func_b71fc);
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_SOWFunc_Earthquake + 7));
}
PORT_LOCAL_JUMP(SOWFunc_Earthquake, Func_b6c53) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(SOWFunc_Earthquake, Func_b6c58) { wait_ow_func(gb, 60); }
PORT_LOCAL_JUMP(SOWFunc_Earthquake, Func_b6c5d) { shake_start(gb); }
PORT_LOCAL_JUMP(SOWFunc_Earthquake, Func_b6c73) { wait_ow_func(gb, 120); }
// .asm_b6c86
static void sow_earthquake_obj2(GB *gb)
{
	set_state(gb, 0x1b, wSceneObj2State);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(SOWFunc_Earthquake, Func_b6c78)
{
	SET_HL(gb, ADDR_Data_855f9);
	gb_call(gb, ADDR_Func_b5868);
	play_sfx(gb, SFX_061);
	sow_earthquake_obj2(gb);
}
PORT_LOCAL_JUMP(SOWFunc_Earthquake, Func_b6c91)
{
	SET_HL(gb, wSceneObj2State);
	JUMP_TO(ADDR_WaitOWObjDisappear);
}
PORT_LOCAL_JUMP(SOWFunc_Earthquake, Func_b6c97)
{
	gb->cpu.a = alu_add(gb, 0x02, rd8(gb, wSceneObj2YCoord));
	wr8(gb, wSceneObj2YCoord, gb->cpu.a);
	gb->cpu.a = alu_add(gb, 0xfe, rd8(gb, wSceneObj2YCoord + 1));
	wr8(gb, wSceneObj2YCoord + 1, gb->cpu.a);
	sow_earthquake_obj2(gb);
}
// end of an earthquake shake: w2d03e = $0e, w2d049 = 0
static void shake_end(GB *gb)
{
	gb->cpu.a = 0x0e;
	set_2d03e(gb, 0x0e);
	wr8(gb, w2d049, 0);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(SOWFunc_Earthquake, Func_b6ca4)
{
	gb->cpu.a = 0x03;
	wr8(gb, wOWUIObj1YCoord, 0x03);
	shake_end(gb);
}

// EOWFunc_BurnVines.Func_b77ce
static void burn_vines_sfx(GB *gb)
{
	SET_HL(gb, w2d123);
	wr8(gb, w2d123, alu_inc(gb, rd8(gb, w2d123)));
	gb->cpu.a = rd8(gb, w2d123);
	alu_cp(gb, gb->cpu.a, 0x0f);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d123, 0);
	gb->cpu.a = rd8(gb, wSceneObj1State);
	alu_cp(gb, gb->cpu.a, 0x15);
	if (gb->cpu.f & FLAG_Z) {
		play_sfx(gb, SFX_126);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x16);
	if (gb->cpu.f & FLAG_Z) {
		play_sfx(gb, SFX_125);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x14);
	if (gb->cpu.f & FLAG_Z)
		play_sfx(gb, SFX_01E);
}

PORT_JUMP(EOWFunc_BurnVines)
{
	burn_vines_sfx(gb);
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_EOWFunc_BurnVines + 7));
}
PORT_LOCAL_JUMP(EOWFunc_BurnVines, Func_b7803) { wait_ow_func(gb, 40); }
PORT_LOCAL_JUMP(EOWFunc_BurnVines, Func_b7808) { wait_ow_func(gb, 30); }
PORT_LOCAL_JUMP(EOWFunc_BurnVines, Func_b780d)
{
	set_state(gb, 0x15, wSceneObj1State);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(EOWFunc_BurnVines, Func_b7818) { wait_ow_func(gb, 60); }
PORT_LOCAL_JUMP(EOWFunc_BurnVines, Func_b781d)
{
	set_state(gb, 0x16, wSceneObj1State);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(EOWFunc_BurnVines, Func_b7828) { wait_ow_func(gb, 30); }
PORT_LOCAL_JUMP(EOWFunc_BurnVines, Func_b782d)
{
	set_state(gb, 0x14, wSceneObj1State);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(EOWFunc_BurnVines, Func_b7838) { wait_ow_func(gb, 90); }
PORT_LOCAL_JUMP(EOWFunc_BurnVines, Func_b783d)
{
	set_state(gb, 0x00, wSceneObj9State);
	set_state(gb, 0x15, wSceneObj1State); // jr .Func_b780d
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(EOWFunc_BurnVines, Func_b7847) { wait_ow_func(gb, 40); }
PORT_LOCAL_JUMP(EOWFunc_BurnVines, Func_b784c)
{
	set_state(gb, 0x18, wSceneObj1State);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(EOWFunc_BurnVines, Func_b7857)
{
	SET_HL(gb, wSceneObj1State);
	JUMP_TO(ADDR_WaitOWObjDisappear);
}

PORT_JUMP(EOWFunc_Earthquake)
{
	gb_call(gb, ADDR_Func_b71fc);
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_EOWFunc_Earthquake + 7));
}
PORT_LOCAL_JUMP(EOWFunc_Earthquake, Func_b7138) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(EOWFunc_Earthquake, Func_b713d) { wait_ow_func(gb, 40); }
PORT_LOCAL_JUMP(EOWFunc_Earthquake, Func_b7142)
{
	for (int row = 10; row <= 11; row++) {
		uint16_t hl = (uint16_t)(wAttrmap + row * TILEMAP_WIDTH + 2);
		gb->cpu.b = 0x04;
		do {
			wr8(gb, hl, rd8(gb, hl) | (1 << B_BG_PRIO));
			hl++;
			gb->cpu.b = alu_dec(gb, gb->cpu.b);
		} while (!(gb->cpu.f & FLAG_Z));
		SET_HL(gb, hl);
	}
	gb->cpu.a = TRUE;
	wr8(gb, wOWPendingTileUpdate, TRUE);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(EOWFunc_Earthquake, Func_b7160)
{
	set_state(gb, 0x03, wSceneObj6State);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(EOWFunc_Earthquake, Func_b716b) { wait_ow_func(gb, 10); }
PORT_LOCAL_JUMP(EOWFunc_Earthquake, Func_b7170)
{
	gb->cpu.a = 0x01;
	wr8(gb, wOWUIObj1YCoord, 0x01);
	set_state(gb, 0x06, wSceneObj15State);
	set_state(gb, 0x04, wSceneObj6State);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
// ld a, [wOWFuncCounter] / cp n / ret c / ld hl, wSceneObj6YCoord / inc [hl] /
// jp AdvanceOWFunc
static void eow_earthquake_move(GB *gb, uint8_t n)
{
	if (counter_below(gb, n)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj6YCoord);
	wr8(gb, wSceneObj6YCoord, alu_inc(gb, rd8(gb, wSceneObj6YCoord)));
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(EOWFunc_Earthquake, Func_b7188) { eow_earthquake_move(gb, 0x0a); }
PORT_LOCAL_JUMP(EOWFunc_Earthquake, Func_b7195) { eow_earthquake_move(gb, 0x20); }
PORT_LOCAL_JUMP(EOWFunc_Earthquake, Func_b71a2) { eow_earthquake_move(gb, 0x1c); }
PORT_LOCAL_JUMP(EOWFunc_Earthquake, Func_b71af) { eow_earthquake_move(gb, 0x18); }
PORT_LOCAL_JUMP(EOWFunc_Earthquake, Func_b71bc) { eow_earthquake_move(gb, 0x14); }
PORT_LOCAL_JUMP(EOWFunc_Earthquake, Func_b71c9)
{
	gb->cpu.a = 0x03;
	wr8(gb, wOWUIObj1YCoord, 0x03);
	set_state(gb, 0x07, wSceneObj15State);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(EOWFunc_Earthquake, Func_b71d9)
{
	set_state(gb, 0x05, wSceneObj6State);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(EOWFunc_Earthquake, Func_b71e4)
{
	gb->cpu.a = rd8(gb, wSceneObj6Frame);
	alu_cp(gb, gb->cpu.a, 0x09);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(EOWFunc_Earthquake, Func_b71ed)
{
	if (counter_below(gb, 0x09)) {
		RET();
		return;
	}
	SET_HL(gb, ADDR_Data_8563e);
	gb_call(gb, ADDR_Func_b5868);
	JUMP_TO(ADDR_AdvanceOWFunc);
}

// NOWFunc_Prologue.Func_b59e5
static void prologue_sfx(GB *gb)
{
	if (!nonzero(gb, wOWUIObj2Attributes))
		return;
	SET_HL(gb, wOWUIObj2Duration);
	wr8(gb, wOWUIObj2Duration, alu_inc(gb, rd8(gb, wOWUIObj2Duration)));
	gb->cpu.a = rd8(gb, wOWUIObj2Duration);
	alu_cp(gb, gb->cpu.a, 0x01);
	if (gb->cpu.f & FLAG_Z) {
		play_sfx(gb, SFX_08E);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x30);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWUIObj2Duration, 0);
}

PORT_JUMP(NOWFunc_Prologue)
{
	gb_call(gb, ADDR_Func_b71fc);
	FARCALL(Func_15868e);
	prologue_sfx(gb);
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_NOWFunc_Prologue + 0x19));
}
PORT_LOCAL_JUMP(NOWFunc_Prologue, Func_b5949)
{
	if (counter_below(gb, 60)) {
		RET();
		return;
	}
	shake_start(gb);
}
PORT_LOCAL_JUMP(NOWFunc_Prologue, Func_b5965)
{
	if (counter_below(gb, 134)) {
		RET();
		return;
	}
	gb->cpu.a = 0x04;
	wr8(gb, wOWUIObj1YCoord, 0x04);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(NOWFunc_Prologue, Func_b5973)
{
	gb->cpu.a = rd8(gb, wOWUIObj1YCoord);
	alu_cp(gb, gb->cpu.a, 0x05);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	set_obj_state_sfx_advance(gb, 0x2b, wSceneObj1State, SFX_061);
}
PORT_LOCAL_JUMP(NOWFunc_Prologue, Func_b598c)
{
	if (nonzero(gb, wOWUIObj1YCoord)) {
		RET();
		return;
	}
	shake_end(gb);
}
PORT_LOCAL_JUMP(NOWFunc_Prologue, Func_b59a0) { wait_ow_func(gb, 86); }
PORT_LOCAL_JUMP(NOWFunc_Prologue, Func_b59a5)
{
	if (counter_below(gb, 6)) {
		RET();
		return;
	}
	gb->cpu.a = 0x01;
	wr8(gb, w2d150, 0x01);
	wr8(gb, wOWUIObj2Attributes, 0x01);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
// .asm_b59b9
static void prologue_set_flag(GB *gb, uint16_t flag)
{
	SET_HL(gb, flag);
	if (counter_below(gb, 6)) {
		RET();
		return;
	}
	gb->cpu.a = 0x01;
	wr8(gb, flag, 0x01);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(NOWFunc_Prologue, Func_b59b6) { prologue_set_flag(gb, w2d151); }
PORT_LOCAL_JUMP(NOWFunc_Prologue, Func_b59c5) { prologue_set_flag(gb, wd152); }
PORT_LOCAL_JUMP(NOWFunc_Prologue, Func_b59ca) { prologue_set_flag(gb, wd153); }
PORT_LOCAL_JUMP(NOWFunc_Prologue, Func_b59cf) { prologue_set_flag(gb, wd154); }
PORT_LOCAL_JUMP(NOWFunc_Prologue, Func_b59d4) { prologue_set_flag(gb, wd155); }
PORT_LOCAL_JUMP(NOWFunc_Prologue, Func_b59d9) { wait_ow_func(gb, 120); }
PORT_LOCAL_JUMP(NOWFunc_Prologue, Func_b59de)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWUIObj2Attributes, 0);
	JUMP_TO(ADDR_EndOWFunc);
}

// WOWFunc_Earthquake.asm_b5d2d
static void wow_earthquake_next(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWUIObj1FramesetOffset, 0);
	SET_HL(gb, wOWUIObj1FramesetOffset - 1);
	wr8(gb, wOWUIObj1FramesetOffset - 1, alu_inc(gb, rd8(gb, wOWUIObj1FramesetOffset - 1)));
}
PORT_LOCAL(WOWFunc_Earthquake, Func_b5d0a)
{
	SET_HL(gb, wOWUIObj1Duration + 1);
	gb->cpu.a = rd8(gb, wOWUIObj1Duration);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	wr8(gb, wOWUIObj1Duration + 1, alu_inc(gb, rd8(gb, wOWUIObj1Duration + 1)));
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	gb_call(gb, jumptable(gb, WOW_EARTHQUAKE_B5D0A + 9));
}
PORT_LOCAL(WOWFunc_Earthquake, Func_b5d25)
{
	set_state(gb, 0x14, wSceneObj13State);
	wow_earthquake_next(gb);
}
PORT_LOCAL(WOWFunc_Earthquake, Func_b5d34)
{
	set_state(gb, 0x15, wSceneObj15State);
	wow_earthquake_next(gb);
}
PORT_LOCAL(WOWFunc_Earthquake, Func_b5d3e)
{
	set_state(gb, 0x14, wSceneObj14State);
	wow_earthquake_next(gb);
}
PORT_LOCAL(WOWFunc_Earthquake, Func_b5d48)
{
	set_state(gb, 0x15, wSceneObj12State);
	wow_earthquake_next(gb);
}
PORT_LOCAL(WOWFunc_Earthquake, Func_b5d52)
{
	gb->cpu.a = rd8(gb, wOWUIObj1FramesetOffset);
	alu_cp(gb, gb->cpu.a, 0x0a);
	if (gb->cpu.f & FLAG_C)
		return;
	wow_earthquake_next(gb);
}
PORT_LOCAL(WOWFunc_Earthquake, Func_b5d5a)
{
	gb->cpu.a = 0x01;
	wr8(gb, wOWUIObj1Duration, 0x01);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWUIObj1Duration + 1, 0);
	SET_HL(gb, wOWUIObj1Duration + 1);
}

PORT_JUMP(WOWFunc_Earthquake)
{
	gb_call(gb, ADDR_Func_b71fc);
	gb_call(gb, WOW_EARTHQUAKE_B5D0A);
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_WOWFunc_Earthquake + 0x0a));
}
PORT_LOCAL_JUMP(WOWFunc_Earthquake, Wait1) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(WOWFunc_Earthquake, Func_b5c8f) { wait_ow_func(gb, 60); }
PORT_LOCAL_JUMP(WOWFunc_Earthquake, Func_b5c94)
{
	gb->cpu.a = 0x01;
	wr8(gb, wOWUIObj1YCoord, 0x01);
	wr8(gb, wOWUIObj1Duration, 0x01);
	set_state(gb, 0x17, wSceneObj11State);
	set_state(gb, 0x19, wSceneObj10State);
	gb->cpu.a = 0x01;
	wr8(gb, w2d049, 0x01);
	gb->cpu.a = 0x03;
	set_2d03e(gb, 0x03);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(WOWFunc_Earthquake, WaitShake) { wait_ow_func(gb, 120); }
PORT_LOCAL_JUMP(WOWFunc_Earthquake, Func_b5cc4)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWUIObj1Duration, 0);
	gb->cpu.a = 0x03;
	wr8(gb, wOWUIObj1YCoord, 0x03);
	set_state(gb, 0x18, wSceneObj11State);
	set_state(gb, 0x1a, wSceneObj10State);
	shake_end(gb);
}
PORT_LOCAL_JUMP(WOWFunc_Earthquake, WaitPostShake) { wait_ow_func(gb, 60); }
PORT_LOCAL_JUMP(WOWFunc_Earthquake, DropRocks)
{
	set_state(gb, 0x16, wSceneObj13State);
	SET_HL(gb, ADDR_Data_85549);
	gb_call(gb, ADDR_Func_b5868);
	sfx_advance(gb, SFX_061);
}

// NOWFunc_DayNightSpell.Func_b57f4
static void day_night_spell_b57f4(GB *gb)
{
	gb->cpu.a = (uint8_t)((wAttrmap + 21 * TILEMAP_WIDTH) >> 8);
	wr8(gb, wHDMASourceHi, gb->cpu.a);
	wr8(gb, wHDMASourceLo, (uint8_t)(wAttrmap + 21 * TILEMAP_WIDTH));
	wr8(gb, wHDMADestHi, (uint8_t)(((v0BGMap0 + 30 * TILEMAP_WIDTH) >> 8) - 0x80));
	wr8(gb, wHDMADestLo, (uint8_t)(v0BGMap0 + 30 * TILEMAP_WIDTH));
	wr8(gb, wHDMAMode, 2 * 2 - 1);
	wr8(gb, w2d0b5SourceHi, (uint8_t)((wAttrmap + 19 * TILEMAP_WIDTH) >> 8));
	wr8(gb, w2d0b5SourceLo, (uint8_t)(wAttrmap + 19 * TILEMAP_WIDTH));
	wr8(gb, w2d0b5DestHi, (uint8_t)(((v0BGMap0 + 30 * TILEMAP_WIDTH) >> 8) - 0x80));
	wr8(gb, w2d0b5DestLo, (uint8_t)(v0BGMap0 + 30 * TILEMAP_WIDTH));
	gb->cpu.a = 2 * 2 - 1;
	wr8(gb, w2d0b5Mode, gb->cpu.a);
}

// NOWFunc_DayNightSpell.Func_b5827: copies a 2x2 tile and attribute block
// from hl to rows 19-22 of wAttrmap
static void day_night_spell_b5827(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	uint16_t saved = wAttrmap + 19 * TILEMAP_WIDTH + 11;
	SET_DE(gb, saved);
	gb->cpu.a = alu_add(gb, gb->cpu.e, 2 * TILEMAP_WIDTH);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.a = alu_adc(gb, 0x00, gb->cpu.d);
	gb->cpu.d = gb->cpu.a;
	gb->cpu.c = 2;
	for (;;) {
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e = alu_dec(gb, gb->cpu.e);
		gb->cpu.a = alu_add(gb, gb->cpu.e, TILEMAP_WIDTH);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.e = alu_inc(gb, gb->cpu.e);
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, REG_DE(gb), gb->cpu.a);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
		if (gb->cpu.f & FLAG_Z)
			break;
		SET_DE(gb, saved); // pop de
	}
	SET_HL(gb, hl);
}

#define DAY_NIGHT_VISIBLE_ATTR (ADDR_NOWFunc_DayNightSpell + 0xe3)
#define DAY_NIGHT_INVISIBLE_ATTR (ADDR_NOWFunc_DayNightSpell + 0xeb)
#define DAY_NIGHT_PRESSED_ATTR (ADDR_NOWFunc_DayNightSpell + 0xf3)

// NOWFunc_DayNightSpell.apply_attribute
static void day_night_apply_attr(GB *gb, uint16_t attr)
{
	SET_HL(gb, attr);
	day_night_spell_b5827(gb);
	day_night_spell_b57f4(gb);
	JUMP_TO(ADDR_AdvanceOWFunc);
}

static void day_night_set_button(GB *gb, uint8_t wait, uint8_t state, uint16_t attr, bool sfx)
{
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, wait);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	gb->cpu.a = state;
	SET_HL(gb, wSceneObj5State);
	CALL(SetSceneObjState);
	if (sfx)
		play_sfx(gb, SFX_SELECTION);
	day_night_apply_attr(gb, attr);
}

static void day_night_wait_press(GB *gb, uint8_t frame)
{
	gb->cpu.a = rd8(gb, wSceneObj5Frame);
	alu_cp(gb, gb->cpu.a, frame);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	day_night_apply_attr(gb, DAY_NIGHT_VISIBLE_ATTR);
}

// NOWFunc_DayNightSpell.OpenTopBar
static void day_night_open_top_bar(GB *gb)
{
	gb->cpu.a = rd8(gb, wSCY);
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	wr8(gb, wSCY, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0xec);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}

PORT_JUMP(NOWFunc_DayNightSpell) { ow_func_jumptable(gb, ADDR_NOWFunc_DayNightSpell); }
PORT_LOCAL_JUMP(NOWFunc_DayNightSpell, Wait1) { wait_ow_func(gb, 30); }
PORT_LOCAL_JUMP(NOWFunc_DayNightSpell, Wait3) { wait_ow_func(gb, 60); }
PORT_LOCAL_JUMP(NOWFunc_DayNightSpell, StartOpenTopBar)
{
	play_sfx(gb, SFX_0E9);
	gb_call(gb, ADDR_AdvanceOWFunc);
	day_night_open_top_bar(gb); // fallthrough
}
PORT_LOCAL_JUMP(NOWFunc_DayNightSpell, OpenTopBar) { day_night_open_top_bar(gb); }
PORT_LOCAL_JUMP(NOWFunc_DayNightSpell, Wait2) { wait_ow_func(gb, 40); }
PORT_LOCAL_JUMP(NOWFunc_DayNightSpell, SetButtonVisible) { day_night_set_button(gb, 20, 0x25, DAY_NIGHT_VISIBLE_ATTR, false); }
PORT_LOCAL_JUMP(NOWFunc_DayNightSpell, SetButtonInvisible) { day_night_set_button(gb, 20, 0x00, DAY_NIGHT_INVISIBLE_ATTR, false); }
PORT_LOCAL_JUMP(NOWFunc_DayNightSpell, WaitAndPressDayButton) { day_night_set_button(gb, 60, 0x26, DAY_NIGHT_PRESSED_ATTR, true); }
PORT_LOCAL_JUMP(NOWFunc_DayNightSpell, WaitDayButtonPress) { day_night_wait_press(gb, 0x04); }
PORT_LOCAL_JUMP(NOWFunc_DayNightSpell, DayNightTransition)
{
	FARCALL(Func_84c0d);
	gb->cpu.a = alu_and(gb, rd8(gb, wOWPalTransitionState), rd8(gb, wOWPalTransitionState));
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.ime = false; // di
	FARCALL(VBlank_Overworld);
	gb->cpu.ime = true; // ei
	gb->cpu.ime_delay = 0;
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(NOWFunc_DayNightSpell, WaitAndPressNightButton) { day_night_set_button(gb, 60, 0x28, DAY_NIGHT_PRESSED_ATTR, true); }
PORT_LOCAL_JUMP(NOWFunc_DayNightSpell, WaitNightButtonPress) { day_night_wait_press(gb, 0x06); }

#define FORM_CASTLE_B74CB (ADDR_EOWFunc_FormCastle + 0x136) // .Func_b74cb
#define FORM_CASTLE_DATA (ADDR_EOWFunc_FormCastle + 0x1bb) // .data

// EOWFunc_FormCastle.asm_b750a
static void form_castle_b750a(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWUIObj2YCoord + 1, 0);
	SET_HL(gb, wOWUIObj2YCoord);
	wr8(gb, wOWUIObj2YCoord, alu_inc(gb, rd8(gb, wOWUIObj2YCoord)));
}

// EOWFunc_FormCastle.Func_b7544: hl = .data + 2 * [wOWUIObj2Frame]
static void form_castle_b7544(GB *gb)
{
	gb->cpu.a = rd8(gb, wOWUIObj2Frame);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	SET_HL(gb, alu_add16(gb, FORM_CASTLE_DATA, REG_DE(gb)));
}

PORT_LOCAL(EOWFunc_FormCastle, Func_b74cb)
{
	SET_HL(gb, wOWUIObj2YCoord + 1);
	gb->cpu.a = alu_and(gb, rd8(gb, wOWUIObj2YCoord), rd8(gb, wOWUIObj2YCoord));
	if (gb->cpu.f & FLAG_Z)
		return;
	wr8(gb, wOWUIObj2YCoord + 1, alu_inc(gb, rd8(gb, wOWUIObj2YCoord + 1)));
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) { // .asm_b74df
		gb->cpu.a = BGPI_AUTOINC | (0 * PAL_SIZE);
		wr8(gb, wPalConfig1Index + 0, gb->cpu.a);
		wr8(gb, wPalConfig1Index + 1, 6);
		wr8(gb, wPalConfig1Index + 2, (uint8_t)(wTempBGPals >> 8));
		wr8(gb, wPalConfig1Index + 3, (uint8_t)wTempBGPals);
		wr8(gb, wPalConfig2Index + 0, BGPI_AUTOINC | (0 * PAL_SIZE));
		wr8(gb, wPalConfig2Index + 1, 6);
		wr8(gb, wPalConfig2Index + 2, (uint8_t)(wTempPals1 >> 8));
		gb->cpu.a = (uint8_t)wTempPals1;
		wr8(gb, wPalConfig2Index + 3, gb->cpu.a);
		SET_HL(gb, ADDR_Pals_84840);
		SET_DE(gb, wTempBGPals);
		gb->cpu.c = 6 * PAL_SIZE;
		gb->cpu.b = BANK_Pals_84840;
		CALL(CopyFarBytes);
		form_castle_b750a(gb);
		return;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) { // .asm_b7511
		form_castle_b7544(gb);
		gb->cpu.b = rd8(gb, REG_HL(gb));
		gb->cpu.a = rd8(gb, wOWUIObj2YCoord + 1);
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		if (gb->cpu.f & FLAG_C)
			return;
		gb->cpu.a = (uint8_t)rBGPI;
		wr8(gb, wPalConfig1, gb->cpu.a);
		SET_HL(gb, wPalConfig1 + 1);
		form_castle_b750a(gb);
		return;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) { // .asm_b7522
		form_castle_b7544(gb);
		SET_HL(gb, REG_HL(gb) + 1);
		gb->cpu.b = rd8(gb, REG_HL(gb));
		gb->cpu.a = rd8(gb, wOWUIObj2YCoord + 1);
		alu_cp(gb, gb->cpu.a, gb->cpu.b);
		if (gb->cpu.f & FLAG_C)
			return;
		gb->cpu.a = (uint8_t)rBGPI;
		wr8(gb, wPalConfig2, gb->cpu.a);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wOWUIObj2YCoord + 1, 0);
		SET_HL(gb, wOWUIObj2YCoord);
		wr8(gb, wOWUIObj2YCoord, alu_dec(gb, rd8(gb, wOWUIObj2YCoord)));
		return;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) { // .asm_b7539
		gb->cpu.a = (uint8_t)rBGPI;
		wr8(gb, wPalConfig2, gb->cpu.a);
		SET_HL(gb, wPalConfig2 + 1);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wOWUIObj2YCoord, 0);
	}
}

PORT_JUMP(EOWFunc_FormCastle)
{
	gb_call(gb, FORM_CASTLE_B74CB);
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_EOWFunc_FormCastle + 7));
}

// EOWFunc_FormCastle.asm_b7410 (called and jumped to)
static void form_castle_b7410(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wOWFuncCounter), 0x03);
	if (gb->cpu.f & FLAG_Z)
		return;
	SET_HL(gb, wOWUIObj2Frame);
	wr8(gb, wOWUIObj2Frame, alu_inc(gb, rd8(gb, wOWUIObj2Frame)));
}

// ld a, [wOWFuncCounter] / cp n / ret c / ld hl, data / call Func_b5868 /
// jp AdvanceOWFunc
static void form_castle_b5868(GB *gb, uint8_t n, uint16_t data)
{
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, n);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	SET_HL(gb, data);
	gb_call(gb, ADDR_Func_b5868);
	JUMP_TO(ADDR_AdvanceOWFunc);
}

static void set_state_advance(GB *gb, uint8_t state, uint16_t obj_state)
{
	gb->cpu.a = state;
	SET_HL(gb, obj_state);
	CALL(SetSceneObjState);
	JUMP_TO(ADDR_AdvanceOWFunc);
}

PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b73c4) { wait_ow_func(gb, 60); }
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b73c9) { wait_ow_func(gb, 40); }
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b73ce) { set_obj_state_sfx_advance(gb, 0x10, wSceneObj1State, SFX_12E); }
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b73e1) { wait_ow_func(gb, 40); }
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b73e6)
{
	gb->cpu.a = 0x01;
	wr8(gb, w2d121, 0x01);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b73ee)
{
	// .Func_b741b
	gb->cpu.a = alu_and(gb, rd8(gb, wOWFuncCounter), 0x03);
	if (!(gb->cpu.f & FLAG_Z)) {
		SET_DE(gb, ADDR_Data_b64cd);
		SET_HL(gb, wSceneObj1YCoord);
		CALL(ApplyOWMovement);
	}
	gb->cpu.a = rd8(gb, wSceneObj1Unk7);
	alu_cp(gb, gb->cpu.a, 0x30);
	if (gb->cpu.f & FLAG_Z) { // .asm_b7404
		gb->cpu.a = alu_and(gb, rd8(gb, wOWFuncCounter), 0x03);
		if (!(gb->cpu.f & FLAG_Z)) {
			gb->cpu.a = 0x01;
			wr8(gb, wOWUIObj2YCoord, 0x01);
		}
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x50);
	if (gb->cpu.f & FLAG_Z) {
		form_castle_b7410(gb);
		RET();
		return;
	}
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	form_castle_b7410(gb);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b742b) { wait_ow_func(gb, 20); }
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b7430) { set_state_advance(gb, 0x11, wSceneObj1State); }
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b743b) { form_castle_b5868(gb, 0x28, ADDR_Data_85b3d); }
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b744a) { form_castle_b5868(gb, 0x05, ADDR_Data_85b2c); }
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b7459) { form_castle_b5868(gb, 0x05, ADDR_Data_85b1b); }
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b7468) { form_castle_b5868(gb, 0x05, ADDR_Data_85b0a); }
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b7477) { form_castle_b5868(gb, 0x05, ADDR_Data_85afd); }
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b7486) { form_castle_b5868(gb, 0x05, ADDR_Data_85af4); }
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b7495) { set_state_advance(gb, 0x12, wSceneObj7State); }
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b74a0) { wait_ow_func(gb, 10); }
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b74a5) { set_state_advance(gb, 0x00, wSceneObj1State); }
PORT_LOCAL_JUMP(EOWFunc_FormCastle, Func_b74b0)
{
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, 0x0a);
	bool dec = gb->cpu.f & FLAG_Z;
	if (!dec) {
		alu_cp(gb, gb->cpu.a, 0x14);
		dec = gb->cpu.f & FLAG_Z;
	}
	if (dec) { // .asm_b74c6
		SET_HL(gb, wOWUIObj2Frame);
		wr8(gb, wOWUIObj2Frame, alu_dec(gb, rd8(gb, wOWUIObj2Frame)));
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x21);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.a = 0x04;
	wr8(gb, wOWUIObj2YCoord, 0x04);
	JUMP_TO(ADDR_AdvanceOWFunc);
}

#define CANYON_B6869 (ADDR_SOWFunc_CanyonThunder + 0xd1) // .Func_b6869
#define CANYON_B6879 (ADDR_SOWFunc_CanyonThunder + 0xe1) // .Func_b6879
#define CANYON_B68CD (ADDR_SOWFunc_CanyonThunder + 0x135) // .Func_b68cd

// SOWFunc_CanyonThunder.apply_pals
static void canyon_apply_pals(GB *gb)
{
	wr8(gb, wPalConfig1 + 0, (uint8_t)rBGPI);
	wr8(gb, wPalConfig1 + 1, BGPI_AUTOINC | (0 * PAL_SIZE));
	wr8(gb, wPalConfig1 + 2, 1 * PAL_SIZE);
	wr8(gb, wPalConfig1 + 3, (uint8_t)(wTempBGPals >> 8));
	gb->cpu.a = (uint8_t)wTempBGPals;
	wr8(gb, wPalConfig1 + 4, gb->cpu.a);
	SET_HL(gb, wPalConfig1 + 5);
}

// SOWFunc_CanyonThunder.got_pal
static void canyon_got_pal(GB *gb, uint16_t pals)
{
	SET_HL(gb, pals);
	SET_DE(gb, wTempBGPals);
	gb->cpu.b = BANK_Pals_84280; // aka BANK(Pals_8698d)
	gb->cpu.c = 8 * PAL_SIZE;
	CALL(CopyFarBytes);
	canyon_apply_pals(gb);
}

// SOWFunc_CanyonThunder.flash2_pal
static void canyon_flash2_pal(GB *gb)
{
	SET_HL(gb, wTempBGPals + 0 * PAL_SIZE);
	gb->cpu.b = 3 * PAL_SIZE;
	gb_call(gb, ADDR_Func_b69a6);
	SET_HL(gb, wTempBGPals + 7 * PAL_SIZE);
	gb->cpu.b = 4;
	gb_call(gb, ADDR_Func_b69a6);
	canyon_apply_pals(gb);
}

// ld hl, wPalConfig1TotalSteps / ld a, [hl] / and a / ret z / inc [hl]:
// false on the ret z
static bool canyon_step(GB *gb)
{
	SET_HL(gb, wPalConfig1TotalSteps);
	gb->cpu.a = alu_and(gb, rd8(gb, wPalConfig1TotalSteps), rd8(gb, wPalConfig1TotalSteps));
	if (gb->cpu.f & FLAG_Z)
		return false;
	wr8(gb, wPalConfig1TotalSteps, alu_inc(gb, rd8(gb, wPalConfig1TotalSteps)));
	return true;
}

PORT_LOCAL(SOWFunc_CanyonThunder, Func_b6869)
{
	if (!canyon_step(gb))
		return;
	alu_cp(gb, gb->cpu.a, 0x05);
	if (gb->cpu.f & FLAG_Z) {
		canyon_flash2_pal(gb);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x09);
	if (gb->cpu.f & FLAG_Z)
		canyon_got_pal(gb, ADDR_Pals_8698d); // .flash1_pal
}

PORT_LOCAL(SOWFunc_CanyonThunder, Func_b6879)
{
	if (!canyon_step(gb))
		return;
	alu_cp(gb, gb->cpu.a, 0x01);
	if (gb->cpu.f & FLAG_Z) {
		canyon_got_pal(gb, ADDR_Pals_8698d); // .flash1_pal
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x05);
	if (gb->cpu.f & FLAG_Z) {
		canyon_flash2_pal(gb);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x09);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	// .asm_b6892
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPalConfig1TotalSteps, 0);
	gb->cpu.a = rd8(gb, w2d062);
	alu_cp(gb, gb->cpu.a, 0x07);
	canyon_got_pal(gb, (gb->cpu.f & FLAG_Z) ? ADDR_Pals_8698d : ADDR_Pals_84280);
}

PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b68cd)
{
	gb->cpu.a = alu_and(gb, rd8(gb, w2d124), rd8(gb, w2d124));
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	SET_HL(gb, w2d125);
	wr8(gb, w2d125, alu_inc(gb, rd8(gb, w2d125)));
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	JUMP_TO(jumptable(gb, CANYON_B68CD + 11));
}

// SOWFunc_CanyonThunder.asm_b6910
static void canyon_b6910(GB *gb)
{
	SET_HL(gb, w2d124);
	wr8(gb, w2d124, alu_inc(gb, rd8(gb, w2d124)));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d125, 0);
}

// lb de, d, e / ld c, c / ld a, d / ld [hli], a / ld a, e / ld [hli], a /
// set 2, l / ld a, c / call SetSceneObjState
static void canyon_place(GB *gb, uint16_t obj, uint8_t d, uint8_t e, uint8_t c)
{
	gb->cpu.d = d;
	gb->cpu.e = e;
	gb->cpu.c = c;
	gb->cpu.a = d;
	wr8(gb, obj, d);
	gb->cpu.a = e;
	wr8(gb, (uint16_t)(obj + 1), e);
	uint16_t hl = (uint16_t)(obj + 2);
	hl |= 1 << 2; // set 2, l
	SET_HL(gb, hl);
	gb->cpu.a = c;
	CALL(SetSceneObjState);
}

// SOWFunc_CanyonThunder.asm_b68f6
static void canyon_b68f6(GB *gb, uint16_t obj, uint8_t d, uint8_t e, uint8_t c)
{
	canyon_place(gb, obj, d, e, c);
	gb->cpu.a = 0x01;
	wr8(gb, wPalConfig1TotalSteps, 0x01);
	gb_call(gb, CANYON_B6869);
	play_sfx(gb, SFX_124); // .asm_b6908
	canyon_b6910(gb);
}

// ld a, 7 / ld hl, w2d125 / cp [hl] / ret nc / jr .asm_b6910
static void canyon_b6925(GB *gb)
{
	gb->cpu.a = 0x07;
	SET_HL(gb, w2d125);
	alu_cp(gb, gb->cpu.a, rd8(gb, w2d125));
	if (!(gb->cpu.f & FLAG_C))
		return;
	canyon_b6910(gb);
}

PORT_LOCAL(SOWFunc_CanyonThunder, Func_b68ee) { canyon_b68f6(gb, wSceneObj2, 0x50, 0x38, 0x17); }
PORT_LOCAL(SOWFunc_CanyonThunder, Func_b6919)
{
	gb_call(gb, CANYON_B6869);
	canyon_b6925(gb);
}
PORT_LOCAL(SOWFunc_CanyonThunder, Func_b6925) { canyon_b6925(gb); }
PORT_LOCAL(SOWFunc_CanyonThunder, Func_b692e) { canyon_b68f6(gb, wSceneObj1, 0x48, 0x46, 0x12); }
PORT_LOCAL(SOWFunc_CanyonThunder, Func_b6938) { canyon_b68f6(gb, wSceneObj2, 0x42, 0x2e, 0x17); }
PORT_LOCAL(SOWFunc_CanyonThunder, Func_b6942) { canyon_b68f6(gb, wSceneObj1, 0x3e, 0x38, 0x12); }
PORT_LOCAL(SOWFunc_CanyonThunder, Func_b694c)
{
	canyon_place(gb, wSceneObj10, 0x3b, 0x1f, 0x12);
	play_sfx(gb, SFX_124); // .asm_b6908
	canyon_b6910(gb);
}
PORT_LOCAL(SOWFunc_CanyonThunder, Func_b6960)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d124, 0);
	wr8(gb, w2d125, 0);
}

PORT_JUMP(SOWFunc_CanyonThunder)
{
	gb_call(gb, CANYON_B68CD);
	gb->cpu.a = rd8(gb, w2d062);
	JUMP_TO(jumptable(gb, ADDR_SOWFunc_CanyonThunder + 7));
}

// SOWFunc_CanyonThunder.asm_b67d6
static void canyon_b67d6(GB *gb)
{
	CALL(SetSceneObjState);
	gb->cpu.a = 0x01;
	wr8(gb, wPalConfig1TotalSteps, 0x01);
	gb_call(gb, CANYON_B6879);
	sfx_advance(gb, SFX_124);
}

PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b67c7) { wait_ow_func(gb, 34); }
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b67cc) { wait_ow_func(gb, 90); }
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b67d1)
{
	SET_HL(gb, wSceneObj2State);
	gb->cpu.a = 0x12;
	canyon_b67d6(gb);
}
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b67ec)
{
	gb_call(gb, CANYON_B6879);
	wait_ow_func(gb, 16);
}
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b67f4)
{
	SET_HL(gb, wSceneObj10State);
	gb->cpu.a = 0x12;
	canyon_b67d6(gb);
}
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b67f9)
{
	gb_call(gb, CANYON_B6879);
	wait_ow_func(gb, 54);
}
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b6801)
{
	gb->cpu.a = 0x56;
	wr8(gb, wSceneObj2YCoord, 0x56);
	wr8(gb, wSceneObj2YCoord + 1, 0x47);
	SET_HL(gb, wSceneObj2State);
	gb->cpu.a = 0x17;
	canyon_b67d6(gb);
}
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b6810)
{
	gb_call(gb, CANYON_B6879);
	gb->cpu.a = alu_and(gb, rd8(gb, wPalConfig1TotalSteps), rd8(gb, wPalConfig1TotalSteps));
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b681b) { wait_ow_func(gb, 30); }
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b6820)
{
	gb->cpu.a = 0x01;
	wr8(gb, w2d124, 0x01);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b6828)
{
	gb->cpu.a = rd8(gb, w2d124);
	alu_cp(gb, gb->cpu.a, 0x07);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b6831) { wait_ow_func(gb, 7); }
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b6836)
{
	gb->cpu.a = 0x0a;
	SET_HL(gb, wSceneObj5State);
	CALL(SetSceneObjState);
	SET_HL(gb, ADDR_Data_855b2);
	gb_call(gb, ADDR_Func_b5868);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b6847) { wait_ow_func(gb, 45); }
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b684c)
{
	SET_HL(gb, ADDR_Pals_84280);
	SET_DE(gb, wTempPals1);
	gb->cpu.b = BANK_Pals_84280;
	gb->cpu.c = 8 * PAL_SIZE;
	CALL(CopyFarBytes);
	SET_HL(gb, ADDR_Pals_84980);
	SET_DE(gb, wTempPals2);
	gb->cpu.b = BANK_Pals_84980;
	gb->cpu.c = 8 * PAL_SIZE;
	CALL(CopyFarBytes);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b6968)
{
	gb->cpu.a = rd8(gb, wOWFuncCounter);
	if (!alu_bit(gb, gb->cpu.a, 0)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_b588d);
}
PORT_LOCAL_JUMP(SOWFunc_CanyonThunder, Func_b6971)
{
	SET_HL(gb, wTempPals1);
	SET_DE(gb, wTempBGPals);
	gb->cpu.b = 8 * PAL_SIZE;
	CALL(CopyHLToDE);
	SET_HL(gb, wTempPals2);
	SET_DE(gb, wTempOBPals);
	gb->cpu.b = 8 * PAL_SIZE;
	CALL(CopyHLToDE);
	SET_HL(gb, wTempPals1);
	gb->cpu.b = 3 * PAL_SIZE;
	gb_call(gb, ADDR_Func_b69a6);
	SET_HL(gb, wTempPals1 + 7 * PAL_SIZE);
	gb->cpu.b = 4;
	gb_call(gb, ADDR_Func_b69a6);
	SET_HL(gb, wTempPals2);
	gb->cpu.b = 4 * PAL_SIZE;
	gb_call(gb, ADDR_Func_b69a6);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wPalConfig1TotalSteps, 0);
	JUMP_TO(ADDR_AdvanceOWFunc);
}
