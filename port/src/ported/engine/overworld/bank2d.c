// Port of src/engine/overworld/bank2d.asm
#include "ported.h"

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
