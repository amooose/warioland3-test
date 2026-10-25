// Port of src/home/object_mechanics.asm
#include "ported.h"

// The wrappers jump and call with JUMP_TO / gb_call rather than JUMP / CALL:
// bankswitch only sets the low ROM bank register, so if the high bit is set
// the asm runs whatever is mapped, and so must the port.

// The homejp macro, up to the jp: ld a, [wROMBank] / push af /
// ld a, BANK(target) / bankswitch
static void homejp(GB *gb, uint8_t bank)
{
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = bank;
	bankswitch(gb, bank);
}

// The homecall macro: homejp, but with a call, then pop af / bankswitch
static void homecall(GB *gb, uint8_t bank, uint16_t addr)
{
	homejp(gb, bank);
	gb_call(gb, addr);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}

// given the obj struct in hl
// update its sprite with its current position
PORT(UpdateObjSprite)
{
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = rd8(gb, REG_HL(gb)); // unk7
	gb->cpu.a = (uint8_t)(((gb->cpu.a << 4 | gb->cpu.a >> 4) & 0x7) | BANK_OAM_180000);
	bankswitch(gb, gb->cpu.a);
	uint16_t hl = (uint16_t)(gb->cpu.h << 8 | gb->cpu.e);
	wr8(gb, wCurSpriteYCoord, rd8(gb, hl++));
	wr8(gb, wCurSpriteXCoord, rd8(gb, hl++));
	uint8_t frame = rd8(gb, hl++);
	wr8(gb, wCurSpriteFrame, frame);
	uint8_t lo = rd8(gb, hl++);
	hl = (uint16_t)(rd8(gb, hl) << 8 | lo);
	// ld a, [wCurSpriteFrame] / ld d, $00 / add a / ld e, a / add hl, de
	frame = rd8(gb, wCurSpriteFrame);
	gb->cpu.a = alu_add(gb, frame, frame);
	hl = alu_add16(gb, hl, gb->cpu.a);
	uint16_t de = (uint16_t)(rd8(gb, hl + 1) << 8 | rd8(gb, hl));
	hl++;

	gb->cpu.c = rd8(gb, wCurSpriteXCoord);
	gb->cpu.b = rd8(gb, wCurSpriteYCoord);
	hl = (uint16_t)((wVirtualOAM >> 8) << 8 | rd8(gb, wVirtualOAMByteSize));
	for (;;) {
		gb->cpu.a = (uint8_t)hl;
		alu_cp(gb, gb->cpu.a, (wVirtualOAMEnd & 0xff));
		if (!(gb->cpu.f & FLAG_C))
			break;
		gb->cpu.a = rd8(gb, de);
		alu_cp(gb, gb->cpu.a, 0x80);
		if (gb->cpu.f & FLAG_Z)
			break;
		gb->cpu.a = alu_add(gb, rd8(gb, de), gb->cpu.b);
		wr8(gb, hl++, gb->cpu.a); // SPRITEOAMSTRUCT_YCOORD
		de++;
		gb->cpu.a = alu_add(gb, rd8(gb, de), gb->cpu.c);
		wr8(gb, hl++, gb->cpu.a); // SPRITEOAMSTRUCT_XCOORD
		de++;
		gb->cpu.a = rd8(gb, de);
		wr8(gb, hl++, gb->cpu.a); // SPRITEOAMSTRUCT_TILE_ID
		de++;
		gb->cpu.a = rd8(gb, de);
		wr8(gb, hl++, gb->cpu.a); // SPRITEOAMSTRUCT_ATTRIBUTES
		gb->cpu.a = (uint8_t)hl;
		wr8(gb, wVirtualOAMByteSize, gb->cpu.a);
		de++;
	}
	SET_HL(gb, hl);
	SET_DE(gb, de);
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}

// some bosses hold the number of hits required to be defeated
// in the substate variable of their object struct
// this routine decrements this counter to a minimum of 0
PORT(DecrementRemainingBossHits)
{
	gb->cpu.l = OBJ_SUBSTATE;
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, hl), 0x0f);
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
}

PORT(UpdateObjectFrame)
{
	gb->cpu.a = alu_sub(gb, rd8(gb, wCurObjFrameDuration), 1);
	SET_HL(gb, wCurObjFrameDuration - 1);
	if (!(gb->cpu.f & FLAG_C))
		return;
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	bankswitch(gb, BANK_Frameset_68000);
	gb->cpu.e = rd8(gb, wCurObjFramesetPtr);
	gb->cpu.d = rd8(gb, wCurObjFramesetPtr + 1);
	SET_HL(gb, wCurObjFramesetOffset);
	gb->cpu.c = alu_add(gb, rd8(gb, wCurObjFramesetOffset), gb->cpu.e);
	gb->cpu.b = alu_adc(gb, gb->cpu.d, 0);
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0xff);
	if (gb->cpu.f & FLAG_Z)
		gb->cpu.a = rd8(gb, REG_DE(gb)); // first frame
	wr8(gb, wCurObjFrame, gb->cpu.a);
	// HomeJumpRet
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}

PORT(UpdateObjectAnimation)
{
	gb->cpu.a = alu_sub(gb, rd8(gb, wCurObjFrameDuration), 1);
	wr8(gb, wCurObjFrameDuration, gb->cpu.a);
	SET_HL(gb, wCurObjFrameDuration - 1);
	if (!(gb->cpu.f & FLAG_C))
		return;
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	bankswitch(gb, BANK_Frameset_68000);
	gb->cpu.e = rd8(gb, wCurObjFramesetPtr);
	gb->cpu.d = rd8(gb, wCurObjFramesetPtr + 1);
	gb->cpu.c = alu_add(gb, rd8(gb, wCurObjFramesetOffset), gb->cpu.e);
	gb->cpu.b = alu_adc(gb, gb->cpu.d, 0);
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0xff);
	if (!(gb->cpu.f & FLAG_Z)) {
		wr8(gb, wCurObjFrame, gb->cpu.a);
		gb->cpu.a = alu_add(gb, rd8(gb, wCurObjFramesetOffset), 2);
		wr8(gb, wCurObjFramesetOffset, gb->cpu.a);
		SET_BC(gb, REG_BC(gb) + 1);
		gb->cpu.a = rd8(gb, REG_BC(gb));
		wr8(gb, wCurObjFrameDuration, gb->cpu.a);
	} else {
		// .reset
		gb->cpu.a = 2;
		wr8(gb, wCurObjFramesetOffset, 2);
		gb->cpu.a = rd8(gb, REG_DE(gb)); // first frame
		wr8(gb, wCurObjFrame, gb->cpu.a);
		SET_DE(gb, REG_DE(gb) + 1);
		gb->cpu.a = rd8(gb, REG_DE(gb)); // first duration
		wr8(gb, wCurObjFrameDuration, gb->cpu.a);
	}
	SET_HL(gb, wCurObjFrameDuration);
	// HomeJumpRet
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}

// Object positions are 16-bit little-endian: the low byte at the label, the
// high byte after it.

// ld a, [hl] / add or sub n / ld [hli], a / ret nc / inc or dec [hl]
static void move_pos_by(GB *gb, uint16_t pos, uint8_t n, bool subtract)
{
	uint8_t lo = rd8(gb, pos);
	gb->cpu.a = subtract ? alu_sub(gb, lo, n) : alu_add(gb, lo, n);
	wr8(gb, pos, gb->cpu.a);
	SET_HL(gb, pos + 1);
	if (!(gb->cpu.f & FLAG_C))
		return;
	uint8_t hi = rd8(gb, pos + 1);
	wr8(gb, pos + 1, subtract ? alu_dec(gb, hi) : alu_inc(gb, hi));
}

// ld hl, pos / inc [hl] / ret nz / inc l / inc [hl]
static void inc_pos(GB *gb, uint16_t pos)
{
	SET_HL(gb, pos);
	uint8_t lo = alu_inc(gb, rd8(gb, pos));
	wr8(gb, pos, lo);
	if (lo)
		return;
	gb->cpu.l = alu_inc(gb, gb->cpu.l);
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
}

// ld a, [wGlobalCounter] / rra: the slow movers only move every other frame.
// Returns the carry (bit 0 of the counter).
static bool rra_global_counter(GB *gb)
{
	uint8_t v = rd8(gb, wGlobalCounter);
	bool carry = v & 1;
	gb->cpu.a = (uint8_t)((v >> 1) | ((gb->cpu.f & FLAG_C) ? 0x80 : 0));
	gb->cpu.f = carry ? FLAG_C : 0;
	return carry;
}

PORT(MoveObjectLeftByVar2)
{
	gb->cpu.b = rd8(gb, wCurObjVar2);
	move_pos_by(gb, wCurObjXPos, gb->cpu.b, true);
}

PORT(MoveObjectRightByVar2)
{
	gb->cpu.b = rd8(gb, wCurObjVar2);
	move_pos_by(gb, wCurObjXPos, gb->cpu.b, false);
}

PORT(MoveObjectUpByVar2)
{
	gb->cpu.b = rd8(gb, wCurObjVar2);
	move_pos_by(gb, wCurObjYPos, gb->cpu.b, true);
}

PORT(MoveObjectDownByVar2)
{
	gb->cpu.b = rd8(gb, wCurObjVar2);
	move_pos_by(gb, wCurObjYPos, gb->cpu.b, false);
}

PORT(MoveObjectLeft_Fast)
{
	move_pos_by(gb, wCurObjXPos, 2, true);
}

PORT(MoveObjectRight_Fast)
{
	move_pos_by(gb, wCurObjXPos, 2, false);
}

PORT(MoveObjectDown_Fast)
{
	move_pos_by(gb, wCurObjYPos, 2, false);
}

PORT(MoveObjectUp_Fast)
{
	move_pos_by(gb, wCurObjYPos, 2, true);
}

// moves current object right
// by 0.5 pixel per frame
PORT(MoveObjectRight_Slow)
{
	if (!rra_global_counter(gb))
		inc_pos(gb, wCurObjXPos);
}

// moves current object right
// at 1 pixel per frame
PORT(MoveObjectRight)
{
	inc_pos(gb, wCurObjXPos);
}

// moves current object left
// by 0.5 pixel per frame
PORT(MoveObjectLeft_Slow)
{
	if (rra_global_counter(gb))
		move_pos_by(gb, wCurObjXPos, 1, true);
}

// moves current object left
// at 1 pixel per frame
PORT(MoveObjectLeft)
{
	move_pos_by(gb, wCurObjXPos, 1, true);
}

// moves current object down
// at 0.5 pixel per frame
PORT(MoveObjectDown_Slow)
{
	if (!rra_global_counter(gb))
		inc_pos(gb, wCurObjYPos);
}

// moves current object down
// at 1 pixel per frame
PORT(MoveObjectDown)
{
	inc_pos(gb, wCurObjYPos);
}

PORT(MoveObjectUp_Slow)
{
	if (rra_global_counter(gb))
		move_pos_by(gb, wCurObjYPos, 1, true);
}

// moves current object up
// at 1 pixel per frame
PORT(MoveObjectUp)
{
	move_pos_by(gb, wCurObjYPos, 1, true);
}

// input:
// - de = frameset pointer
PORT(SetObjectFramesetPtr)
{
	wr8(gb, wCurObjFramesetPtr + 0, gb->cpu.e);
	wr8(gb, wCurObjFramesetPtr + 1, gb->cpu.d);
	wr8(gb, wCurObjFramesetPtr + 2, 0); // OBJ_FRAME_DURATION
	wr8(gb, wCurObjFramesetPtr + 3, 0); // OBJ_FRAMESET_OFFSET
	SET_HL(gb, wCurObjFramesetPtr + 4);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

// The tail of routines entered with homejp, which pushed the caller's bank.
PORT(HomeJumpRet)
{
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}

// if Wario is underneath a one-way platform,
// then don't process any actions for it
PORT(SetOneWayPlatformAction)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wCurObjFlags), OBJFLAG_STEPPED);
	if (gb->cpu.a)
		return;
	gb->cpu.b = rd8(gb, wCurObjCollBoxTop);
	// wCurObjCollBoxTop + wCurObjScreenYPos + $2a + 3
	gb->cpu.b = alu_add(gb, alu_add(gb, rd8(gb, wCurObjScreenYPos), 0x2a + 3), gb->cpu.b);
	gb->cpu.a = alu_add(gb, rd8(gb, wWarioScreenYPos), 0x2a);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (gb->cpu.f & FLAG_C)
		return; // Wario is over the collision box
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
	wr8(gb, wCurObjAction, gb->cpu.a);
}
PORT_JUMP(Func_3173)
{
	homejp(gb, BANK_Func_62e57);
	JUMP_TO(ADDR_Func_62e57);
}

PORT_JUMP(VanishObject2)
{
	homejp(gb, BANK__VanishObject2);
	JUMP_TO(ADDR__VanishObject2);
}

PORT_JUMP(Func_3191)
{
	homejp(gb, BANK_Func_62e8f);
	JUMP_TO(ADDR_Func_62e8f);
}

PORT_JUMP(Func_31a0)
{
	homejp(gb, BANK_Func_62ea5);
	JUMP_TO(ADDR_Func_62ea5);
}

PORT_JUMP(Func_31af)
{
	homejp(gb, BANK_Func_62f76);
	JUMP_TO(ADDR_Func_62f76);
}

PORT_JUMP(Func_31be)
{
	homejp(gb, BANK_Func_62f8c);
	JUMP_TO(ADDR_Func_62f8c);
}

PORT_JUMP(Func_31cd)
{
	homejp(gb, BANK_Func_62eaa);
	JUMP_TO(ADDR_Func_62eaa);
}

PORT_JUMP(Func_31dc)
{
	homejp(gb, BANK_Func_62ec0);
	JUMP_TO(ADDR_Func_62ec0);
}

PORT_JUMP(Func_31eb)
{
	homejp(gb, BANK_Func_62f91);
	JUMP_TO(ADDR_Func_62f91);
}

PORT_JUMP(Func_31fa)
{
	homejp(gb, BANK_Func_62fa7);
	JUMP_TO(ADDR_Func_62fa7);
}

PORT_JUMP(Func_3209)
{
	homejp(gb, BANK_Func_62e74);
	JUMP_TO(ADDR_Func_62e74);
}

PORT_JUMP(Func_3218)
{
	homejp(gb, BANK_Func_62e8a);
	JUMP_TO(ADDR_Func_62e8a);
}

PORT_JUMP(Func_3227)
{
	homejp(gb, BANK_Func_62f5b);
	JUMP_TO(ADDR_Func_62f5b);
}

PORT_JUMP(Func_3236)
{
	homejp(gb, BANK_Func_62f71);
	JUMP_TO(ADDR_Func_62f71);
}

PORT_JUMP(Func_3245)
{
	homejp(gb, BANK_Func_62c9f);
	JUMP_TO(ADDR_Func_62c9f);
}

PORT_JUMP(Func_3254)
{
	homejp(gb, BANK_Func_62d7d);
	JUMP_TO(ADDR_Func_62d7d);
}

PORT_JUMP(Func_3263)
{
	homejp(gb, BANK_Func_6303f);
	JUMP_TO(ADDR_Func_6303f);
}

PORT_JUMP(Func_3272)
{
	homejp(gb, BANK_Func_63050);
	JUMP_TO(ADDR_Func_63050);
}

PORT_JUMP(Func_3281)
{
	homejp(gb, BANK_Func_6305f);
	JUMP_TO(ADDR_Func_6305f);
}

PORT_JUMP(ObjState_Fall)
{
	homejp(gb, BANK__ObjState_Fall);
	JUMP_TO(ADDR__ObjState_Fall);
}

PORT_JUMP(ObjState_StandingFall)
{
	homejp(gb, BANK__ObjState_StandingFall);
	JUMP_TO(ADDR__ObjState_StandingFall);
}

PORT_JUMP(Func_32ae)
{
	homejp(gb, BANK_Func_62ae5);
	JUMP_TO(ADDR_Func_62ae5);
}

PORT_JUMP(Func_32bd)
{
	homejp(gb, BANK_Func_62bce);
	JUMP_TO(ADDR_Func_62bce);
}

PORT_JUMP(Func_32cc)
{
	homejp(gb, BANK_Func_62ca8);
	JUMP_TO(ADDR_Func_62ca8);
}

PORT_JUMP(Func_32db)
{
	homejp(gb, BANK_Func_62d86);
	JUMP_TO(ADDR_Func_62d86);
}

PORT_JUMP(Func_32ea)
{
	homejp(gb, BANK_Func_628ea);
	JUMP_TO(ADDR_Func_628ea);
}

PORT_JUMP(Func_32f9)
{
	homejp(gb, BANK_Func_629a6);
	JUMP_TO(ADDR_Func_629a6);
}

PORT_JUMP(Func_3308)
{
	homejp(gb, BANK_Func_62908);
	JUMP_TO(ADDR_Func_62908);
}

PORT_JUMP(Func_3317)
{
	homejp(gb, BANK_Func_62898);
	JUMP_TO(ADDR_Func_62898);
}

PORT_JUMP(Func_3326)
{
	homejp(gb, BANK_Func_62840);
	JUMP_TO(ADDR_Func_62840);
}

PORT_JUMP(Func_3335)
{
	homejp(gb, BANK_Func_61f54);
	JUMP_TO(ADDR_Func_61f54);
}

PORT_JUMP(Func_3344)
{
	homejp(gb, BANK_Func_620a6);
	JUMP_TO(ADDR_Func_620a6);
}

PORT_JUMP(Func_3353)
{
	homejp(gb, BANK_Func_621fb);
	JUMP_TO(ADDR_Func_621fb);
}

PORT_JUMP(Func_3362)
{
	homejp(gb, BANK_Func_622bd);
	JUMP_TO(ADDR_Func_622bd);
}

PORT_JUMP(Func_3371)
{
	homejp(gb, BANK_Func_62926);
	JUMP_TO(ADDR_Func_62926);
}

PORT_JUMP(Func_3380)
{
	homejp(gb, BANK_Func_62382);
	JUMP_TO(ADDR_Func_62382);
}

PORT_JUMP(Func_338f)
{
	homejp(gb, BANK_Func_6247b);
	JUMP_TO(ADDR_Func_6247b);
}

PORT_JUMP(Func_339e)
{
	homejp(gb, BANK_Func_62574);
	JUMP_TO(ADDR_Func_62574);
}

PORT_JUMP(Func_33ad)
{
	homejp(gb, BANK_Func_62605);
	JUMP_TO(ADDR_Func_62605);
}

PORT_JUMP(ObjState_FullThrowLeft)
{
	homejp(gb, BANK_Func_626da);
	JUMP_TO(ADDR_Func_626da);
}

PORT_JUMP(ObjState_FullThrowRight)
{
	homejp(gb, BANK_Func_62768);
	JUMP_TO(ADDR_Func_62768);
}

PORT_JUMP(ObjState_BumpLeft)
{
	homejp(gb, BANK__ObjState_BumpLeft);
	JUMP_TO(ADDR__ObjState_BumpLeft);
}

PORT_JUMP(ObjState_BumpRight)
{
	homejp(gb, BANK__ObjState_BumpRight);
	JUMP_TO(ADDR__ObjState_BumpRight);
}

PORT_JUMP(Func_33f8)
{
	homejp(gb, BANK_Func_62892);
	JUMP_TO(ADDR_Func_62892);
}

PORT_JUMP(Func_3407)
{
	homejp(gb, BANK_Func_6283a);
	JUMP_TO(ADDR_Func_6283a);
}

PORT(CreateObjectAtRelativePos)
{
	homecall(gb, BANK__CreateObjectAtRelativePos, ADDR__CreateObjectAtRelativePos);
}

PORT(CreateObjectFromCurObjPos)
{
	homecall(gb, BANK__CreateObjectFromCurObjPos, ADDR__CreateObjectFromCurObjPos);
}

PORT(Func_3444)
{
	homecall(gb, BANK_Func_632ac, ADDR_Func_632ac);
}

PORT(CountActiveTransientObjects)
{
	homecall(gb, BANK__CountActiveTransientObjects, ADDR__CountActiveTransientObjects);
}

PORT(ApplyObjSetMovement)
{
	homecall(gb, BANK__ApplyObjSetMovement, ADDR__ApplyObjSetMovement);
}

PORT(ApplyObjYMovement_Loop)
{
	homecall(gb, BANK__ApplyObjYMovement_Loop, ADDR__ApplyObjYMovement_Loop);
}

PORT(ApplyObjXMovement_Loop)
{
	homecall(gb, BANK__ApplyObjXMovement_Loop, ADDR__ApplyObjXMovement_Loop);
}

PORT(ApplyObjYMovement)
{
	homecall(gb, BANK__ApplyObjYMovement, ADDR__ApplyObjYMovement);
}

PORT(ApplyObjXMovement)
{
	homecall(gb, BANK__ApplyObjXMovement, ADDR__ApplyObjXMovement);
}

PORT(VanishObject)
{
	homecall(gb, BANK__VanishObject, ADDR__VanishObject);
}

PORT(Func_34fc)
{
	homecall(gb, BANK_Func_63209, ADDR_Func_63209);
}

// Func_3513, Func_352b, Func_3543, ProcessBlockOnLeft, ProcessBlockOnRight,
// Func_358b and Func_35a3 stay in asm: their callees run ProcessBlock, whose
// block functions can read the stack, where a CALL's return address differs
// from the asm's.
