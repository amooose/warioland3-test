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
