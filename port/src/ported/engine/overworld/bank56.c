// Port of src/engine/overworld/bank56.asm
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

// jumptable on a (n entries): returns the index, or -1 after jumping to the
// target of an out-of-range index (the asm runs whatever it points at)
static int dispatch(GB *gb, uint16_t table, int n)
{
	int idx = gb->cpu.a;
	uint16_t target = jumptable(gb, table);
	if (idx >= n) {
		JUMP_TO(target);
		return -1;
	}
	return idx;
}

// ld hl, addr / xor a / ld [hld], a / inc [hl]
static void clear_and_advance(GB *gb, uint16_t addr)
{
	gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
	wr8(gb, addr, gb->cpu.a);
	uint16_t hl = (uint16_t)(addr - 1);
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	SET_HL(gb, hl);
}

// ld a, [addr] / and a: true on z
static bool and_mem_z(GB *gb, uint16_t addr)
{
	gb->cpu.a = rd8(gb, addr);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	return gb->cpu.f & FLAG_Z;
}

// ld a, [bc] / cp n: true on c
static bool bc_below(GB *gb, uint8_t n)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, n);
	return gb->cpu.f & FLAG_C;
}

// --- screen shake ---

// ld hl, wSCX / inc or dec [hl] `n` times
static void scx_step(GB *gb, bool inc, int n)
{
	SET_HL(gb, wSCX);
	for (int i = 0; i < n; i++) {
		uint8_t v = rd8(gb, wSCX);
		wr8(gb, wSCX, inc ? alu_inc(gb, v) : alu_dec(gb, v));
	}
}

// ld hl, w2d897 / xor a / ld [hld], a / ld a, [hl] / sub $07 / ld [hl], a
static void shake_reset(GB *gb)
{
	gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
	wr8(gb, w2d897, gb->cpu.a);
	SET_HL(gb, w2d896);
	gb->cpu.a = alu_sub(gb, rd8(gb, w2d896), 0x07);
	wr8(gb, w2d896, gb->cpu.a);
}

PORT_JUMP(HandleTempleScreenShake)
{
	gb->cpu.a = rd8(gb, w2d896);
	SET_HL(gb, w2d897);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	wr8(gb, w2d897, alu_inc(gb, rd8(gb, w2d897)));
	SET_BC(gb, w2d897);
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	int idx = dispatch(gb, ADDR_HandleTempleScreenShake + 0xb, 20);
	if (idx < 0)
		return;
	switch (idx) {
	case 0: // .Init
		SET_HL(gb, w2d897);
		gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
		wr8(gb, w2d897, gb->cpu.a);
		SET_HL(gb, w2d896);
		wr8(gb, w2d896, gb->cpu.a);
		wr8(gb, wSCX, gb->cpu.a);
		wr8(gb, wSCY, gb->cpu.a);
		break;
	case 1: case 10: // .Next
		clear_and_advance(gb, w2d897);
		break;
	case 2: // .SlowScrollLeftWithSFX
		play_sfx(gb, SFX_01F);
		// fallthrough
	case 4: case 6: case 8: case 11: // .SlowScrollLeft
		if (bc_below(gb, 2))
			break;
		scx_step(gb, false, 1);
		clear_and_advance(gb, w2d897);
		break;
	case 3: case 5: case 7: // .SlowScrollRight
		if (bc_below(gb, 2))
			break;
		scx_step(gb, true, 1);
		clear_and_advance(gb, w2d897);
		break;
	case 9: // .SlowScrollRightAndReset
		if (bc_below(gb, 2))
			break;
		scx_step(gb, true, 1);
		shake_reset(gb);
		break;
	case 12: // .FastScrollRightWithSFX
		play_sfx(gb, SFX_01F);
		// fallthrough
	case 14: case 16: case 18: // .FastScrollRight
		if (bc_below(gb, 2))
			break;
		scx_step(gb, true, 2);
		clear_and_advance(gb, w2d897);
		break;
	case 13: case 15: case 17: // .FastScrollLeft
		if (bc_below(gb, 2))
			break;
		scx_step(gb, false, 2);
		clear_and_advance(gb, w2d897);
		break;
	case 19: // .FastScrollLeftAndReset
		if (bc_below(gb, 2))
			break;
		scx_step(gb, false, 2);
		shake_reset(gb);
		break;
	}
	RET();
}

// --- falling and bouncing rocks ---

// hl = rock respawn time, bc = rock counter
PORT(Func_1580c8)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, rd8(gb, REG_HL(gb)));
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.l = alu_dec(gb, gb->cpu.l);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl); // size
	hl--;
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		SET_HL(gb, hl);
		gb->cpu.l = alu_dec(gb, gb->cpu.l);
		gb->cpu.a = 0x01;
		wr8(gb, REG_HL(gb), gb->cpu.a); // action
		return;
	}
	wr8(gb, hl, gb->cpu.a); // counter
	hl--;
	wr8(gb, hl, gb->cpu.a); // action
	SET_HL(gb, hl);
}

PORT(StubTempleRockFunc) {}

// a = size, c = random seed, hl = obj state
PORT_JUMP(SetTempleRockSize_WithLarge)
{
	alu_cp(gb, gb->cpu.a, TEMPLE_ROCK_SIZE_LARGE);
	gb->cpu.a = 0x9;
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_and(gb, gb->cpu.c, 0x3);
		alu_cp(gb, gb->cpu.a, 0x3);
		if (gb->cpu.f & FLAG_Z)
			gb->cpu.a = alu_dec(gb, gb->cpu.a);
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.b);
	}
	JUMP(SetSceneObjState);
}

// a = size, c = random seed, hl = obj state
PORT_JUMP(SetTempleRockSize_WithoutLarge)
{
	alu_cp(gb, gb->cpu.a, TEMPLE_ROCK_SIZE_LARGE);
	gb->cpu.a = 0x9;
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_and(gb, gb->cpu.c, 0x1);
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.b);
	}
	JUMP(SetSceneObjState);
}

// a = y, d/e = x coords (by bit 3 of c), hl = obj
static void spawn_rock(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, gb->cpu.a);
	hl++;
	gb->cpu.a = gb->cpu.d;
	if (alu_bit(gb, gb->cpu.c, 3))
		gb->cpu.a = gb->cpu.e;
	wr8(gb, hl, gb->cpu.a);
	SET_HL(gb, hl);
}

PORT(SpawnRock_Top)
{
	gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
	spawn_rock(gb);
}

PORT(SpawnRock) { spawn_rock(gb); }

PORT(SpawnRock_Bottom)
{
	gb->cpu.a = 0x60;
	spawn_rock(gb);
}

PORT(PlayFallingRockSFX) { play_sfx(gb, SFX_TEMPLE_ROCK); }

// bc = rock counter
PORT(GetRockYVel)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x2c);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = 0x28;
		wr8(gb, REG_BC(gb), gb->cpu.a);
	}
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	uint16_t hl = alu_add16(gb, ADDR_GetRockYVel + 0x11, REG_DE(gb)); // .y_vel_table
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
}

// temple_rock_fall
static void rock_fall(GB *gb, uint16_t routine, uint16_t action, uint16_t counter,
	uint16_t size, uint16_t respawn, uint16_t obj, uint16_t obj_state, bool large,
	uint8_t x1, uint8_t x2, uint8_t max_y)
{
	gb->cpu.a = rd8(gb, action);
	SET_HL(gb, counter);
	wr8(gb, counter, alu_inc(gb, rd8(gb, counter)));
	SET_BC(gb, counter);
	int idx = dispatch(gb, (uint16_t)(routine + 0x8), 4);
	if (idx < 0)
		return;
	switch (idx) {
	case 0: // StubTempleRockFunc
		break;
	case 1: // .Spawn
		gb->cpu.a = rd8(gb, rDIV);
		gb->cpu.c = gb->cpu.a;
		SET_HL(gb, obj);
		gb->cpu.d = x1;
		gb->cpu.e = x2;
		gb_call(gb, ADDR_SpawnRock_Top);
		gb->cpu.a = rd8(gb, size);
		SET_HL(gb, obj_state);
		gb_call(gb, large ? ADDR_SetTempleRockSize_WithLarge : ADDR_SetTempleRockSize_WithoutLarge);
		clear_and_advance(gb, counter); // .Next
		break;
	case 2: // .Update
		gb_call(gb, ADDR_GetRockYVel);
		SET_HL(gb, obj); // YCoord
		gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, obj));
		wr8(gb, obj, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, max_y);
		if (gb->cpu.f & FLAG_C)
			break;
		gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
		wr8(gb, obj_state, gb->cpu.a);
		gb->cpu.c = 0x3f;
		gb->cpu.a = rd8(gb, size);
		alu_cp(gb, gb->cpu.a, TEMPLE_ROCK_SIZE_LARGE);
		if (gb->cpu.f & FLAG_Z)
			gb->cpu.c = 0x1f;
		gb->cpu.a = alu_and(gb, rd8(gb, rDIV), gb->cpu.c);
		wr8(gb, respawn, gb->cpu.a);
		gb_call(gb, ADDR_PlayFallingRockSFX);
		clear_and_advance(gb, counter); // .Next
		break;
	case 3: // .Reset
		SET_HL(gb, respawn);
		JUMP_TO(ADDR_Func_1580c8);
		return;
	}
	RET();
}

#define ROCK_FALL(n, obj, large, x1, x2, max_y) \
	rock_fall(gb, ADDR_HandleTempleRock##n##Fall, wTempleRock##n, wTempleRock##n##Counter, \
		wTempleRock##n##Size, wTempleRock##n##RespawnTime, obj, obj##State, large, x1, x2, max_y)

PORT_JUMP(HandleTempleRock1Fall) { ROCK_FALL(1, wSceneObj12, true, 32, 48, 96); }
PORT_JUMP(HandleTempleRock3Fall) { ROCK_FALL(3, wSceneObj14, false, 40, 56, 96); }
PORT_JUMP(HandleTempleRock5Fall) { ROCK_FALL(5, wSceneObj1, false, 16, 64, 160); }
PORT_JUMP(HandleTempleRock2Fall) { ROCK_FALL(2, wSceneObj13, true, 112, 128, 96); }
PORT_JUMP(HandleTempleRock4Fall) { ROCK_FALL(4, wSceneObj15, false, 104, 120, 96); }
PORT_JUMP(HandleTempleRock6Fall) { ROCK_FALL(6, wSceneObj2, false, 96, 144, 160); }

// temple_rock_bounce
static void rock_bounce(GB *gb, uint16_t routine, uint16_t action, uint16_t counter,
	uint16_t size, uint16_t obj, uint16_t obj_state, bool fast, bool left,
	uint8_t x1, uint8_t x2)
{
	gb->cpu.a = rd8(gb, action);
	int idx = dispatch(gb, (uint16_t)(routine + 0x4), 4);
	if (idx < 0)
		return;
	switch (idx) {
	case 0: // StubTempleRockFunc
		break;
	case 1: // .Spawn
		gb->cpu.a = rd8(gb, rDIV);
		gb->cpu.c = gb->cpu.a;
		SET_HL(gb, obj);
		gb->cpu.d = x1;
		gb->cpu.e = x2;
		gb_call(gb, ADDR_SpawnRock_Bottom);
		gb->cpu.a = rd8(gb, size);
		SET_HL(gb, obj_state);
		gb_call(gb, ADDR_SetTempleRockSize_WithoutLarge);
		gb_call(gb, ADDR_PlayFallingRockSFX);
		clear_and_advance(gb, counter); // .Next
		break;
	case 2: // .Update
		if (fast) {
			for (int i = 0; i < 2; i++) {
				SET_HL(gb, obj);
				gb_call(gb, left ? ADDR_BounceTempleRockLeft : ADDR_BounceTempleRockRight);
				if (gb->cpu.f & FLAG_Z) {
					clear_and_advance(gb, counter); // .Next
					break;
				}
			}
		} else {
			SET_DE(gb, ADDR_Data_158897);
			SET_HL(gb, obj);
			if (left)
				CALL(ApplyOWMovement);
			else
				CALL(ApplyOWMovement_Mirrored);
			if (gb->cpu.f & FLAG_Z)
				clear_and_advance(gb, counter); // .Next
		}
		break;
	case 3: // .Reset
		gb->cpu.a = rd8(gb, size);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z))
			gb->cpu.a = 0x1; // spawn
		wr8(gb, action, gb->cpu.a);
		gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
		wr8(gb, obj_state, gb->cpu.a);
		break;
	}
	RET();
}

#define ROCK_BOUNCE(n, obj, fast, left, x1, x2) \
	rock_bounce(gb, ADDR_HandleTempleRock##n##Bounce, wTempleRock##n##Action, wTempleRock##n##Counter, \
		wTempleRock##n##Size, obj, obj##State, fast, left, x1, x2)

PORT_JUMP(HandleTempleRock7Bounce) { ROCK_BOUNCE(7, wSceneObj8, false, true, 32, 40); }
PORT_JUMP(HandleTempleRock8Bounce) { ROCK_BOUNCE(8, wSceneObj9, false, false, 120, 128); }
PORT_JUMP(HandleTempleRock9Bounce) { ROCK_BOUNCE(9, wSceneObj10, true, true, 24, 32); }
PORT_JUMP(HandleTempleRock10Bounce) { ROCK_BOUNCE(10, wSceneObj11, true, false, 128, 136); }

PORT_JUMP(BounceTempleRockRight)
{
	SET_DE(gb, ADDR_Data_1588de);
	JUMP(ApplyOWMovement_Mirrored);
}

PORT_JUMP(BounceTempleRockLeft)
{
	SET_DE(gb, ADDR_Data_1588de);
	JUMP(ApplyOWMovement);
}

PORT(_HandleTempleShakeAndRocks)
{
	gb_call(gb, ADDR_HandleTempleScreenShake);
	gb_call(gb, ADDR_HandleTempleRock1Fall);
	gb_call(gb, ADDR_HandleTempleRock2Fall);
	gb_call(gb, ADDR_HandleTempleRock3Fall);
	gb_call(gb, ADDR_HandleTempleRock4Fall);
	gb_call(gb, ADDR_HandleTempleRock5Fall);
	gb_call(gb, ADDR_HandleTempleRock6Fall);
	gb_call(gb, ADDR_HandleTempleRock7Bounce);
	gb_call(gb, ADDR_HandleTempleRock8Bounce);
	gb_call(gb, ADDR_HandleTempleRock9Bounce);
	gb_call(gb, ADDR_HandleTempleRock10Bounce);
}

// --- Func_15848a: the music box notes ---

#define F15848A(off) ((uint16_t)(ADDR_Func_15848a + (off)))

// .Func_15864a / .Func_15866e: a = table[[bc]], cp $80
static void note_offset(GB *gb, uint16_t table)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0x00;
	uint16_t hl = alu_add16(gb, table, REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x80);
}

// ld hl, obj / ld a, [wSceneObj1YCoord] / sub dy / ld [hli], a /
// ld a, [wSceneObj1XCoord] / add or sub dx / ld [hl], a / ld a, $15 /
// ld hl, obj state / call SetSceneObjState
static void place_note(GB *gb, uint16_t obj, uint8_t dy, bool add_x, uint8_t dx, uint16_t obj_state)
{
	gb->cpu.a = alu_sub(gb, rd8(gb, wSceneObj1YCoord), dy);
	wr8(gb, obj, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSceneObj1XCoord);
	gb->cpu.a = add_x ? alu_add(gb, gb->cpu.a, dx) : alu_sub(gb, gb->cpu.a, dx);
	wr8(gb, (uint16_t)(obj + 1), gb->cpu.a);
	gb->cpu.a = 0x15;
	SET_HL(gb, obj_state);
	CALL(SetSceneObjState);
}

// ld hl, var / ld a, [hli] / inc [hl] / ld b, h / ld c, l / jumptable
static int note_dispatch(GB *gb, uint16_t var, uint16_t table, int n, bool set_bc)
{
	gb->cpu.a = rd8(gb, var);
	uint16_t hl = (uint16_t)(var + 1);
	SET_HL(gb, hl);
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	if (set_bc)
		SET_BC(gb, hl);
	return dispatch(gb, table, n);
}

PORT_LOCAL_JUMP(Func_15848a, Func_15849a)
{
	int idx = note_dispatch(gb, w2d8a0, F15848A(0x16), 3, false);
	if (idx < 0)
		return;
	switch (idx) {
	case 0:
		gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
		wr8(gb, w2d8a1, gb->cpu.a);
		break;
	case 1:
		SET_HL(gb, wSceneObj1YCoord);
		gb->cpu.a = 0x18;
		wr8(gb, wSceneObj1YCoord, gb->cpu.a);
		gb->cpu.a = rd8(gb, wSceneObj9XCoord);
		wr8(gb, (uint16_t)(wSceneObj1YCoord + 1), gb->cpu.a);
		gb->cpu.a = 0x16;
		SET_HL(gb, wSceneObj1State);
		CALL(SetSceneObjState);
		clear_and_advance(gb, w2d8a1);
		break;
	case 2:
		gb->cpu.a = alu_and(gb, rd8(gb, w2d8a1), 0x03);
		if (!(gb->cpu.f & FLAG_Z))
			break;
		SET_HL(gb, wSceneObj1YCoord);
		wr8(gb, wSceneObj1YCoord, alu_inc(gb, rd8(gb, wSceneObj1YCoord)));
		gb->cpu.a = rd8(gb, wSceneObj1YCoord);
		alu_cp(gb, gb->cpu.a, 0x58);
		if (!(gb->cpu.f & FLAG_Z))
			break;
		gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
		wr8(gb, wSceneObj1State, gb->cpu.a);
		wr8(gb, w2d8a0, gb->cpu.a);
		break;
	}
	RET();
}

PORT_LOCAL_JUMP(Func_15848a, Func_1584da)
{
	int idx = note_dispatch(gb, wTempleMusicBox1, F15848A(0x58), 3, true);
	if (idx < 0)
		return;
	switch (idx) {
	case 0:
		if (and_mem_z(gb, wSceneObj1State))
			break;
		clear_and_advance(gb, wTempleMusicBox1Counter);
		break;
	case 1:
		SET_HL(gb, wSceneObj5);
		place_note(gb, wSceneObj5, 0x08, false, 0x03, wSceneObj5State);
		clear_and_advance(gb, wTempleMusicBox1Counter);
		break;
	case 2:
		note_offset(gb, F15848A(0x1cf)); // .data_1
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
			wr8(gb, wSceneObj5State, gb->cpu.a);
			wr8(gb, wTempleMusicBox1, gb->cpu.a);
			break;
		}
		SET_HL(gb, wSceneObj5);
		gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, wSceneObj5));
		wr8(gb, wSceneObj5, gb->cpu.a);
		if (bc_below(gb, 0x03), !(gb->cpu.f & FLAG_Z))
			break;
		gb->cpu.a = 0x14;
		wr8(gb, wSceneObj5State, gb->cpu.a);
		break;
	}
	RET();
}

PORT_LOCAL_JUMP(Func_15848a, Func_158529)
{
	int idx = note_dispatch(gb, wPlanePart2Counter, F15848A(0xa7), 5, true);
	if (idx < 0)
		return;
	switch (idx) {
	case 0:
		if (and_mem_z(gb, wSceneObj1State))
			break;
		clear_and_advance(gb, wTempleMusicBox3);
		break;
	case 1:
		if (bc_below(gb, 0x0a))
			break;
		clear_and_advance(gb, wTempleMusicBox3);
		break;
	case 2:
		SET_HL(gb, wSceneObj6);
		place_note(gb, wSceneObj6, 0x08, true, 0x03, wSceneObj6State);
		clear_and_advance(gb, wTempleMusicBox3);
		break;
	case 3:
		note_offset(gb, F15848A(0x1cf)); // .data_1
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
			wr8(gb, wSceneObj6State, gb->cpu.a);
			clear_and_advance(gb, wTempleMusicBox3);
			break;
		}
		SET_HL(gb, wSceneObj6);
		gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, wSceneObj6));
		wr8(gb, wSceneObj6, gb->cpu.a);
		if (bc_below(gb, 0x03), !(gb->cpu.f & FLAG_Z))
			break;
		gb->cpu.a = 0x14;
		wr8(gb, wSceneObj6State, gb->cpu.a);
		break;
	case 4:
		if (!and_mem_z(gb, wSceneObj1State))
			gb->cpu.a = 0x02;
		wr8(gb, wPlanePart2Counter, gb->cpu.a);
		break;
	}
	RET();
}

// .Func_1585bf / .Func_15861e: moves the note up and x by one every other frame
static bool move_note(GB *gb, uint16_t obj, bool inc_x, uint16_t obj_state)
{
	note_offset(gb, F15848A(0x1f3)); // .data_2
	if (gb->cpu.f & FLAG_Z)
		return false;
	gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, obj));
	wr8(gb, obj, gb->cpu.a);
	uint16_t hl = (uint16_t)(obj + 1);
	SET_HL(gb, hl);
	gb->cpu.a = alu_and(gb, rd8(gb, REG_BC(gb)), 0x01);
	if (!(gb->cpu.f & FLAG_Z)) {
		uint8_t v = rd8(gb, hl);
		wr8(gb, hl, inc_x ? alu_inc(gb, v) : alu_dec(gb, v));
	}
	if (bc_below(gb, 0x03), !(gb->cpu.f & FLAG_Z))
		return true;
	gb->cpu.a = 0x14;
	wr8(gb, obj_state, gb->cpu.a);
	return true;
}

PORT_LOCAL_JUMP(Func_15848a, Func_15858c)
{
	int idx = note_dispatch(gb, wTempleMusicBox4, F15848A(0x10a), 3, true);
	if (idx < 0)
		return;
	switch (idx) {
	case 0:
		if (and_mem_z(gb, wSceneObj1State))
			break;
		clear_and_advance(gb, wTempleMusicBox4Counter);
		break;
	case 1:
		SET_HL(gb, wSceneObj7);
		place_note(gb, wSceneObj7, 0x06, false, 0x04, wSceneObj7State);
		clear_and_advance(gb, wTempleMusicBox4Counter);
		break;
	case 2:
		if (move_note(gb, wSceneObj7, false, wSceneObj7State))
			break;
		gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
		wr8(gb, wSceneObj7State, gb->cpu.a);
		wr8(gb, wTempleMusicBox4, gb->cpu.a);
		break;
	}
	RET();
}

PORT_LOCAL_JUMP(Func_15848a, Func_1585e1)
{
	int idx = note_dispatch(gb, wTempleMusicBox5Counter, F15848A(0x15f), 5, true);
	if (idx < 0)
		return;
	switch (idx) {
	case 0:
		if (and_mem_z(gb, wSceneObj1State))
			break;
		clear_and_advance(gb, wPalFade2Colour3GreenUnk1);
		break;
	case 1:
		if (bc_below(gb, 0x08))
			break;
		clear_and_advance(gb, wPalFade2Colour3GreenUnk1);
		break;
	case 2:
		SET_HL(gb, wSceneObj8);
		place_note(gb, wSceneObj8, 0x10, true, 0x06, wSceneObj8State);
		clear_and_advance(gb, wPalFade2Colour3GreenUnk1);
		break;
	case 3:
		if (move_note(gb, wSceneObj8, true, wSceneObj8State))
			break;
		gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
		wr8(gb, wSceneObj8State, gb->cpu.a);
		clear_and_advance(gb, wPalFade2Colour3GreenUnk1);
		break;
	case 4:
		if (!and_mem_z(gb, wSceneObj1State))
			gb->cpu.a = 0x02;
		wr8(gb, wTempleMusicBox5Counter, gb->cpu.a);
		break;
	}
	RET();
}

PORT(Func_15848a)
{
	gb_call(gb, F15848A(0x10)); // .Func_15849a
	gb_call(gb, F15848A(0x50)); // .Func_1584da
	gb_call(gb, F15848A(0x9f)); // .Func_158529
	gb_call(gb, F15848A(0x102)); // .Func_15858c
	gb_call(gb, F15848A(0x157)); // .Func_1585e1
}

// --- Func_15868e ---

#define F15868E(off) ((uint16_t)(ADDR_Func_15868e + (off)))

enum { C_RET, P_30_50, P_2E_4E, P_2E_52, P_31_4F, P_31_51, M_897, M_8DE, M_925, S_1 };

// .Func_158869 and co: ld a, y / ld [hli], a / ld a, x / ld [hli], a / set 2, l /
// ldh a, [rDIV] / and $01 / ld b, $29 / add b / jp SetSceneObjState
static void place_obj(GB *gb, uint16_t obj, uint8_t y, uint8_t x)
{
	wr8(gb, obj, y);
	wr8(gb, (uint16_t)(obj + 1), x);
	SET_HL(gb, (uint16_t)(obj + 2));
	gb->cpu.l |= 1 << 2;
	gb->cpu.a = alu_and(gb, rd8(gb, rDIV), 0x01);
	gb->cpu.b = 0x29;
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.b);
	CALL(SetSceneObjState);
}

static void obj_dispatch(GB *gb, uint16_t routine, uint16_t var, uint16_t obj, bool mirrored, const uint8_t *cases)
{
	gb->cpu.a = rd8(gb, var);
	int idx = dispatch(gb, (uint16_t)(routine + 0x4), 8);
	if (idx < 0)
		return;
	uint16_t data = 0;
	switch (cases[idx]) {
	case C_RET:
		RET();
		return;
	case S_1:
		gb->cpu.a = 0x01;
		wr8(gb, var, gb->cpu.a);
		RET();
		return;
	case P_30_50: SET_HL(gb, obj); place_obj(gb, obj, 0x30, 0x50); break;
	case P_2E_4E: SET_HL(gb, obj); place_obj(gb, obj, 0x2e, 0x4e); break;
	case P_2E_52: SET_HL(gb, obj); place_obj(gb, obj, 0x2e, 0x52); break;
	case P_31_4F: SET_HL(gb, obj); place_obj(gb, obj, 0x31, 0x4f); break;
	case P_31_51: SET_HL(gb, obj); place_obj(gb, obj, 0x31, 0x51); break;
	case M_897: data = ADDR_Data_158897; break;
	case M_8DE: data = ADDR_Data_1588de; break;
	case M_925: data = ADDR_Data_158925; break;
	}
	if (data) {
		SET_DE(gb, data);
		SET_HL(gb, obj);
		if (mirrored)
			CALL(ApplyOWMovement_Mirrored);
		else
			CALL(ApplyOWMovement);
		if (!(gb->cpu.f & FLAG_Z)) {
			RET();
			return;
		}
	}
	SET_HL(gb, var);
	wr8(gb, var, alu_inc(gb, rd8(gb, var)));
	RET();
}

PORT_LOCAL_JUMP(Func_15868e, Func_1586a1)
{
	static const uint8_t cases[8] = { C_RET, P_2E_4E, M_897, P_30_50, M_8DE, P_31_4F, M_925, S_1 };
	obj_dispatch(gb, F15868E(0x13), w2d150, wSceneObj2, false, cases);
}

PORT_LOCAL_JUMP(Func_15868e, Func_1586ed)
{
	static const uint8_t cases[8] = { C_RET, P_30_50, M_8DE, P_31_4F, M_925, P_2E_4E, M_897, S_1 };
	obj_dispatch(gb, F15868E(0x5f), wd152, wSceneObj5, false, cases);
}

PORT_LOCAL_JUMP(Func_15868e, Func_158739)
{
	static const uint8_t cases[8] = { C_RET, P_31_4F, M_925, P_2E_4E, M_897, P_30_50, M_8DE, S_1 };
	obj_dispatch(gb, F15868E(0xab), wd154, wSceneObj9, false, cases);
}

PORT_LOCAL_JUMP(Func_15868e, Func_158785)
{
	static const uint8_t cases[8] = { C_RET, P_2E_52, M_897, P_30_50, M_8DE, P_31_51, M_925, S_1 };
	obj_dispatch(gb, F15868E(0xf7), w2d151, wSceneObj3, true, cases);
}

PORT_LOCAL_JUMP(Func_15868e, Func_1587d1)
{
	static const uint8_t cases[8] = { C_RET, P_30_50, M_8DE, P_31_51, M_925, P_2E_52, M_897, S_1 };
	obj_dispatch(gb, F15868E(0x143), wd153, wSceneObj8, true, cases);
}

PORT_LOCAL_JUMP(Func_15868e, Func_15881d)
{
	static const uint8_t cases[8] = { C_RET, P_31_51, M_925, P_30_50, M_8DE, P_2E_52, M_897, S_1 };
	obj_dispatch(gb, F15868E(0x18f), wd155, wSceneObj10, true, cases);
}

PORT(Func_15868e)
{
	gb_call(gb, F15868E(0x13)); // .Func_1586a1
	gb_call(gb, F15868E(0xf7)); // .Func_158785
	gb_call(gb, F15868E(0x5f)); // .Func_1586ed
	gb_call(gb, F15868E(0x143)); // .Func_1587d1
	gb_call(gb, F15868E(0xab)); // .Func_158739
	gb_call(gb, F15868E(0x18f)); // .Func_15881d
}

// --- best times ---

// .PrintLevelTime
static void print_level_time(GB *gb)
{
	gb->cpu.a = rd8(gb, w2dfff);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = 0x00;
	uint16_t hl = alu_add16(gb, wLevelTimeAttackScores, REG_BC(gb));
	gb->cpu.a = rd8(gb, hl);
	hl++;
	gb->cpu.d = gb->cpu.a; // minutes
	gb->cpu.e = rd8(gb, hl); // seconds
	gb->cpu.b = 2;
	hl = w2df00;
	for (;;) {
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0xf0);
		gb->cpu.a = alu_swap(gb, gb->cpu.a);
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
		gb->cpu.a = alu_add(gb, gb->cpu.a, 0xa0);
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.a = alu_and(gb, gb->cpu.d, 0x0f);
		gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
		gb->cpu.a = alu_add(gb, gb->cpu.a, 0xa0); // 0 digit
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
		if (gb->cpu.f & FLAG_Z)
			break;
		gb->cpu.a = 0xb6; // colon
		wr8(gb, hl++, gb->cpu.a);
		gb->cpu.d = gb->cpu.e;
		gb->cpu.a = gb->cpu.e;
	}
	// .write_tiles
	gb->cpu.a = rd8(gb, w2dfff);
	SET_HL(gb, ADDR_PrintBestTimes + 0x63); // .bg_map_address_table
	CALL(GetPointerFromTableDE);
	gb->cpu.b = 0x00;
	for (;;) {
		gb->cpu.c = 5; // num tiles wide
		hl = w2df00;
		do {
			gb->cpu.a = alu_add(gb, rd8(gb, hl++), gb->cpu.b);
			wr8(gb, REG_DE(gb), gb->cpu.a);
			gb->cpu.e = alu_inc(gb, gb->cpu.e);
			gb->cpu.c = alu_dec(gb, gb->cpu.c);
		} while (!(gb->cpu.f & FLAG_Z));
		SET_HL(gb, hl);
		gb->cpu.b = alu_inc(gb, gb->cpu.b);
		gb->cpu.a = gb->cpu.b;
		alu_cp(gb, gb->cpu.a, 2);
		if (gb->cpu.f & FLAG_Z)
			return;
		gb->cpu.a = alu_add(gb, (uint8_t)(TILEMAP_WIDTH - 5), gb->cpu.e);
		gb->cpu.e = gb->cpu.a;
		gb->cpu.a = alu_adc(gb, gb->cpu.d, 0x00);
		gb->cpu.d = gb->cpu.a;
	}
}

PORT(PrintBestTimes)
{
	gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a); // LEVEL_OUT_OF_THE_WOODS
	wr8(gb, w2dfff, gb->cpu.a);
	for (;;) {
		print_level_time(gb);
		SET_HL(gb, w2dfff);
		wr8(gb, w2dfff, alu_inc(gb, rd8(gb, w2dfff)));
		gb->cpu.a = NUM_LEVELS - 1;
		alu_cp(gb, gb->cpu.a, rd8(gb, w2dfff));
		if (gb->cpu.f & FLAG_C)
			return;
	}
}

PORT(_InitBestTimeList)
{
	CALL(DisableLCD);
	CALL(FillBGMap0_With7f);
	CALL(ClearVirtualOAM);

	// play_music2 MUSIC_TREASURE_COLLECTION
	gb->cpu.a = (uint8_t)(MUSIC_TREASURE_COLLECTION >> 8);
	wr8(gb, hMusicIDBackup + 0, gb->cpu.a);
	wr8(gb, hMusicID + 0, gb->cpu.a);
	gb->cpu.a = (uint8_t)MUSIC_TREASURE_COLLECTION;
	wr8(gb, hMusicIDBackup + 1, gb->cpu.a);
	wr8(gb, hMusicID + 1, gb->cpu.a);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCX, 0);
	wr8(gb, wSCX, 0);
	gb->cpu.a = rd8(gb, wObj1SubState);
	gb->cpu.a = alu_rra(gb, gb->cpu.a);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.a)
		gb->cpu.a = 0x70;
	wr8(gb, rSCY, gb->cpu.a);
	wr8(gb, wSCY, gb->cpu.a);

	gb->cpu.a = 0x02;
	wr8(gb, rWBK, gb->cpu.a);
	// .VBlank_158a14
	SET_HL(gb, ADDR__InitBestTimeList + 0xb4); // .func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 0x16; // .func_end - .func
	CALL(CopyHLToDE);
	// .Func_158a09
	SET_HL(gb, wCollectionRow);
	SET_BC(gb, 0x800);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	CALL(WriteAToHL_BCTimes);

	SET_HL(gb, ADDR_Pals_158b63);
	CALL(LoadPalsToTempPals1);

	// decompress_tiles0 BestTimeListGfx, v0Tiles1, $20
	gb->cpu.a = BANK_BestTimeListGfx;
	wr8(gb, wTempBank, gb->cpu.a);
	SET_HL(gb, ADDR_BestTimeListGfx);
	SET_BC(gb, v0Tiles1 + 0x20 * TILE_SIZE);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
	// decompress_attrmap BestTimeListAttrmap, v1BGMap0
	gb->cpu.a = 1;
	wr8(gb, rVBK, 1);
	gb->cpu.a = BANK_BestTimeListAttrmap;
	wr8(gb, wTempBank, gb->cpu.a);
	SET_HL(gb, ADDR_BestTimeListAttrmap);
	SET_BC(gb, v1BGMap0);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	// decompress_tilemap BestTimeListTilemap, v0BGMap0
	gb->cpu.a = BANK_BestTimeListTilemap;
	wr8(gb, wTempBank, gb->cpu.a);
	SET_HL(gb, ADDR_BestTimeListTilemap);
	SET_BC(gb, v0BGMap0);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);

	gb_call(gb, ADDR_PrintBestTimes);
	gb->cpu.a = LCDC_BG_ON | LCDC_OBJ_ON | LCDC_ON;
	wr8(gb, rLCDC, gb->cpu.a);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}
