// home/object_mechanics (src/home/object_mechanics.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"
#include "object_mechanics.hpp"

namespace wl3::home {

void move_object_x(GB *gb, int dx)
{
	Memory mem(gb);
	mem.word(wCurObjXPos) = static_cast<uint16_t>(mem.word(wCurObjXPos) + dx);
}

void move_object_y(GB *gb, int dy)
{
	Memory mem(gb);
	mem.word(wCurObjYPos) = static_cast<uint16_t>(mem.word(wCurObjYPos) + dy);
}

void set_object_frameset_ptr(GB *gb, uint16_t frameset)
{
	Memory mem(gb);
	mem.word(wCurObjFramesetPtr) = frameset;
	mem[wCurObjFrameDuration] = 0;
	mem[wCurObjFramesetOffset] = 0;
}

// UpdateObjSprite, plain: see object_mechanics.hpp
void update_obj_sprite(GB *gb, uint16_t id, uint16_t pos)
{
	Memory mem(gb);
	uint8_t bank = mem[wROMBank];
	uint8_t v = mem[id];
	bankswitch(gb, static_cast<uint8_t>((((v << 4) | (v >> 4)) & 0x7) | BANK_OAM_180000), ADDR_UpdateObjSprite);
	uint8_t y = mem[pos], x = mem[pos + 1], frame = mem[pos + 2];
	mem[wCurSpriteYCoord] = y;
	mem[wCurSpriteXCoord] = x;
	mem[wCurSpriteFrame] = frame;
	// the OAM pointer plus frame * 2 (an 8-bit add a)
	uint16_t entry = static_cast<uint16_t>(mem.word(pos + 3) + static_cast<uint8_t>(frame * 2));
	uint16_t data = mem.word(entry);
	uint16_t oam = static_cast<uint16_t>((wVirtualOAM & 0xff00) | mem[wVirtualOAMByteSize]);
	while ((oam & 0xff) < (wVirtualOAMEnd & 0xff) && mem[data] != 0x80) {
		mem[oam++] = static_cast<uint8_t>(mem[data++] + y); // SPRITEOAMSTRUCT_YCOORD
		mem[oam++] = static_cast<uint8_t>(mem[data++] + x); // SPRITEOAMSTRUCT_XCOORD
		mem[oam++] = mem[data++]; // SPRITEOAMSTRUCT_TILE_ID
		mem[oam++] = mem[data]; // SPRITEOAMSTRUCT_ATTRIBUTES
		mem[wVirtualOAMByteSize] = static_cast<uint8_t>(oam);
		data++;
	}
	bankswitch(gb, bank, ADDR_UpdateObjSprite);
}

namespace {

class HomeObjectMechanics : public AsmModule {
public:
	using AsmModule::AsmModule;

	// The wrappers jump and call with JUMP_TO / gb_call rather than JUMP / CALL:
	// bankswitch only sets the low ROM bank register, so if the high bit is set
	// the asm runs whatever is mapped, and so must the port.

	// The homejp macro, up to the jp: ld a, [wROMBank] / push af /
	// ld a, BANK(target) / bankswitch
	void homejp(uint8_t bank)
	{
		cpu_.a = rd8(wROMBank);
		push_af();
		cpu_.a = bank;
		bankswitch(bank);
	}

	// The homecall macro: homejp, but with a call, then pop af / bankswitch
	void homecall(uint8_t bank, uint16_t addr)
	{
		homejp(bank);
		gb_call(gb_, addr);
		pop_af();
		bankswitch(cpu_.a);
	}

	// some bosses hold the number of hits required to be defeated
	// in the substate variable of their object struct
	// this routine decrements this counter to a minimum of 0
	void port_DecrementRemainingBossHits()
	{
		cpu_.l = OBJ_SUBSTATE;
		uint16_t hl = REG_HL(gb_);
		cpu_.a = alu_.and_(rd8(hl), 0x0f);
		cpu_.a = alu_.dec(cpu_.a);
		if (cpu_.f & FLAG_Z)
			return;
		wr8(hl, alu_.dec(rd8(hl)));
	}

	void port_UpdateObjectFrame()
	{
		cpu_.a = alu_.sub(rd8(wCurObjFrameDuration), 1);
		SET_HL(gb_, wCurObjFrameDuration - 1);
		if (!(cpu_.f & FLAG_C))
			return;
		cpu_.a = rd8(wROMBank);
		push_af();
		bankswitch(BANK_Frameset_68000);
		cpu_.e = rd8(wCurObjFramesetPtr);
		cpu_.d = rd8(wCurObjFramesetPtr + 1);
		SET_HL(gb_, wCurObjFramesetOffset);
		cpu_.c = alu_.add(rd8(wCurObjFramesetOffset), cpu_.e);
		cpu_.b = alu_.adc(cpu_.d, 0);
		cpu_.a = rd8(REG_BC(gb_));
		alu_.cp(cpu_.a, 0xff);
		if (cpu_.f & FLAG_Z)
			cpu_.a = rd8(REG_DE(gb_)); // first frame
		wr8(wCurObjFrame, cpu_.a);
		// HomeJumpRet
		pop_af();
		bankswitch(cpu_.a);
	}

	void port_UpdateObjectAnimation()
	{
		cpu_.a = alu_.sub(rd8(wCurObjFrameDuration), 1);
		wr8(wCurObjFrameDuration, cpu_.a);
		SET_HL(gb_, wCurObjFrameDuration - 1);
		if (!(cpu_.f & FLAG_C))
			return;
		cpu_.a = rd8(wROMBank);
		push_af();
		bankswitch(BANK_Frameset_68000);
		cpu_.e = rd8(wCurObjFramesetPtr);
		cpu_.d = rd8(wCurObjFramesetPtr + 1);
		cpu_.c = alu_.add(rd8(wCurObjFramesetOffset), cpu_.e);
		cpu_.b = alu_.adc(cpu_.d, 0);
		cpu_.a = rd8(REG_BC(gb_));
		alu_.cp(cpu_.a, 0xff);
		if (!(cpu_.f & FLAG_Z)) {
			wr8(wCurObjFrame, cpu_.a);
			cpu_.a = alu_.add(rd8(wCurObjFramesetOffset), 2);
			wr8(wCurObjFramesetOffset, cpu_.a);
			SET_BC(gb_, REG_BC(gb_) + 1);
			cpu_.a = rd8(REG_BC(gb_));
			wr8(wCurObjFrameDuration, cpu_.a);
		} else {
			// .reset
			cpu_.a = 2;
			wr8(wCurObjFramesetOffset, 2);
			cpu_.a = rd8(REG_DE(gb_)); // first frame
			wr8(wCurObjFrame, cpu_.a);
			SET_DE(gb_, REG_DE(gb_) + 1);
			cpu_.a = rd8(REG_DE(gb_)); // first duration
			wr8(wCurObjFrameDuration, cpu_.a);
		}
		SET_HL(gb_, wCurObjFrameDuration);
		// HomeJumpRet
		pop_af();
		bankswitch(cpu_.a);
	}

	// Object positions are 16-bit little-endian: the low byte at the label, the
	// high byte after it.

	// ld a, [hl] / add or sub n / ld [hli], a / ret nc / inc or dec [hl]
	void move_pos_by(uint16_t pos, uint8_t n, bool subtract)
	{
		uint8_t lo = rd8(pos);
		cpu_.a = subtract ? alu_.sub(lo, n) : alu_.add(lo, n);
		wr8(pos, cpu_.a);
		SET_HL(gb_, pos + 1);
		if (!(cpu_.f & FLAG_C))
			return;
		uint8_t hi = rd8(pos + 1);
		wr8(pos + 1, subtract ? alu_.dec(hi) : alu_.inc(hi));
	}

	// ld hl, pos / inc [hl] / ret nz / inc l / inc [hl]
	void inc_pos(uint16_t pos)
	{
		SET_HL(gb_, pos);
		uint8_t lo = alu_.inc(rd8(pos));
		wr8(pos, lo);
		if (lo)
			return;
		cpu_.l = alu_.inc(cpu_.l);
		uint16_t hl = REG_HL(gb_);
		wr8(hl, alu_.inc(rd8(hl)));
	}

	// ld a, [wGlobalCounter] / rra: the slow movers only move every other frame.
	// Returns the carry (bit 0 of the counter).
	bool rra_global_counter()
	{
		uint8_t v = rd8(wGlobalCounter);
		bool carry = v & 1;
		cpu_.a = (uint8_t)((v >> 1) | ((cpu_.f & FLAG_C) ? 0x80 : 0));
		cpu_.f = carry ? FLAG_C : 0;
		return carry;
	}

	void port_MoveObjectLeftByVar2()
	{
		cpu_.b = rd8(wCurObjVar2);
		move_pos_by(wCurObjXPos, cpu_.b, true);
	}

	void port_MoveObjectRightByVar2()
	{
		cpu_.b = rd8(wCurObjVar2);
		move_pos_by(wCurObjXPos, cpu_.b, false);
	}

	void port_MoveObjectUpByVar2()
	{
		cpu_.b = rd8(wCurObjVar2);
		move_pos_by(wCurObjYPos, cpu_.b, true);
	}

	void port_MoveObjectDownByVar2()
	{
		cpu_.b = rd8(wCurObjVar2);
		move_pos_by(wCurObjYPos, cpu_.b, false);
	}

	void port_MoveObjectLeft_Fast()
	{
		move_pos_by(wCurObjXPos, 2, true);
	}

	void port_MoveObjectRight_Fast()
	{
		move_pos_by(wCurObjXPos, 2, false);
	}

	void port_MoveObjectDown_Fast()
	{
		move_pos_by(wCurObjYPos, 2, false);
	}

	void port_MoveObjectUp_Fast()
	{
		move_pos_by(wCurObjYPos, 2, true);
	}

	// moves current object right
	// by 0.5 pixel per frame
	void port_MoveObjectRight_Slow()
	{
		if (!rra_global_counter())
			inc_pos(wCurObjXPos);
	}

	// moves current object right
	// at 1 pixel per frame
	void port_MoveObjectRight()
	{
		inc_pos(wCurObjXPos);
	}

	// moves current object left
	// by 0.5 pixel per frame
	void port_MoveObjectLeft_Slow()
	{
		if (rra_global_counter())
			move_pos_by(wCurObjXPos, 1, true);
	}

	// moves current object left
	// at 1 pixel per frame
	void port_MoveObjectLeft()
	{
		move_pos_by(wCurObjXPos, 1, true);
	}

	// moves current object down
	// at 0.5 pixel per frame
	void port_MoveObjectDown_Slow()
	{
		if (!rra_global_counter())
			inc_pos(wCurObjYPos);
	}

	// moves current object down
	// at 1 pixel per frame
	void port_MoveObjectDown()
	{
		inc_pos(wCurObjYPos);
	}

	void port_MoveObjectUp_Slow()
	{
		if (rra_global_counter())
			move_pos_by(wCurObjYPos, 1, true);
	}

	// moves current object up
	// at 1 pixel per frame
	void port_MoveObjectUp()
	{
		move_pos_by(wCurObjYPos, 1, true);
	}

	// input:
	// - de = frameset pointer
	void port_SetObjectFramesetPtr()
	{
		wr8(wCurObjFramesetPtr + 0, cpu_.e);
		wr8(wCurObjFramesetPtr + 1, cpu_.d);
		wr8(wCurObjFramesetPtr + 2, 0); // OBJ_FRAME_DURATION
		wr8(wCurObjFramesetPtr + 3, 0); // OBJ_FRAMESET_OFFSET
		SET_HL(gb_, wCurObjFramesetPtr + 4);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
	}

	// The tail of routines entered with homejp, which pushed the caller's bank.
	void port_HomeJumpRet()
	{
		pop_af();
		bankswitch(cpu_.a);
	}

	// if Wario is underneath a one-way platform,
	// then don't process any actions for it
	void port_SetOneWayPlatformAction()
	{
		cpu_.a = alu_.and_(rd8(wCurObjFlags), OBJFLAG_STEPPED);
		if (cpu_.a)
			return;
		cpu_.b = rd8(wCurObjCollBoxTop);
		// wCurObjCollBoxTop + wCurObjScreenYPos + $2a + 3
		cpu_.b = alu_.add(alu_.add(rd8(wCurObjScreenYPos), 0x2a + 3), cpu_.b);
		cpu_.a = alu_.add(rd8(wWarioScreenYPos), 0x2a);
		alu_.cp(cpu_.a, cpu_.b);
		if (cpu_.f & FLAG_C)
			return; // Wario is over the collision box
		cpu_.a = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
		wr8(wCurObjAction, cpu_.a);
	}
	void port_Func_3173()
	{
		homejp(BANK_Func_62e57);
		JUMP_TO(ADDR_Func_62e57);
	}

	void port_VanishObject2()
	{
		homejp(BANK__VanishObject2);
		JUMP_TO(ADDR__VanishObject2);
	}

	void port_Func_3191()
	{
		homejp(BANK_Func_62e8f);
		JUMP_TO(ADDR_Func_62e8f);
	}

	void port_Func_31a0()
	{
		homejp(BANK_Func_62ea5);
		JUMP_TO(ADDR_Func_62ea5);
	}

	void port_Func_31af()
	{
		homejp(BANK_Func_62f76);
		JUMP_TO(ADDR_Func_62f76);
	}

	void port_Func_31be()
	{
		homejp(BANK_Func_62f8c);
		JUMP_TO(ADDR_Func_62f8c);
	}

	void port_Func_31cd()
	{
		homejp(BANK_Func_62eaa);
		JUMP_TO(ADDR_Func_62eaa);
	}

	void port_Func_31dc()
	{
		homejp(BANK_Func_62ec0);
		JUMP_TO(ADDR_Func_62ec0);
	}

	void port_Func_31eb()
	{
		homejp(BANK_Func_62f91);
		JUMP_TO(ADDR_Func_62f91);
	}

	void port_Func_31fa()
	{
		homejp(BANK_Func_62fa7);
		JUMP_TO(ADDR_Func_62fa7);
	}

	void port_Func_3209()
	{
		homejp(BANK_Func_62e74);
		JUMP_TO(ADDR_Func_62e74);
	}

	void port_Func_3218()
	{
		homejp(BANK_Func_62e8a);
		JUMP_TO(ADDR_Func_62e8a);
	}

	void port_Func_3227()
	{
		homejp(BANK_Func_62f5b);
		JUMP_TO(ADDR_Func_62f5b);
	}

	void port_Func_3236()
	{
		homejp(BANK_Func_62f71);
		JUMP_TO(ADDR_Func_62f71);
	}

	void port_Func_3245()
	{
		homejp(BANK_Func_62c9f);
		JUMP_TO(ADDR_Func_62c9f);
	}

	void port_Func_3254()
	{
		homejp(BANK_Func_62d7d);
		JUMP_TO(ADDR_Func_62d7d);
	}

	void port_Func_3263()
	{
		homejp(BANK_Func_6303f);
		JUMP_TO(ADDR_Func_6303f);
	}

	void port_Func_3272()
	{
		homejp(BANK_Func_63050);
		JUMP_TO(ADDR_Func_63050);
	}

	void port_Func_3281()
	{
		homejp(BANK_Func_6305f);
		JUMP_TO(ADDR_Func_6305f);
	}

	void port_ObjState_Fall()
	{
		homejp(BANK__ObjState_Fall);
		JUMP_TO(ADDR__ObjState_Fall);
	}

	void port_ObjState_StandingFall()
	{
		homejp(BANK__ObjState_StandingFall);
		JUMP_TO(ADDR__ObjState_StandingFall);
	}

	void port_Func_32ae()
	{
		homejp(BANK_Func_62ae5);
		JUMP_TO(ADDR_Func_62ae5);
	}

	void port_Func_32bd()
	{
		homejp(BANK_Func_62bce);
		JUMP_TO(ADDR_Func_62bce);
	}

	void port_Func_32cc()
	{
		homejp(BANK_Func_62ca8);
		JUMP_TO(ADDR_Func_62ca8);
	}

	void port_Func_32db()
	{
		homejp(BANK_Func_62d86);
		JUMP_TO(ADDR_Func_62d86);
	}

	void port_Func_32ea()
	{
		homejp(BANK_Func_628ea);
		JUMP_TO(ADDR_Func_628ea);
	}

	void port_Func_32f9()
	{
		homejp(BANK_Func_629a6);
		JUMP_TO(ADDR_Func_629a6);
	}

	void port_Func_3308()
	{
		homejp(BANK_Func_62908);
		JUMP_TO(ADDR_Func_62908);
	}

	void port_Func_3317()
	{
		homejp(BANK_Func_62898);
		JUMP_TO(ADDR_Func_62898);
	}

	void port_Func_3326()
	{
		homejp(BANK_Func_62840);
		JUMP_TO(ADDR_Func_62840);
	}

	void port_Func_3335()
	{
		homejp(BANK_Func_61f54);
		JUMP_TO(ADDR_Func_61f54);
	}

	void port_Func_3344()
	{
		homejp(BANK_Func_620a6);
		JUMP_TO(ADDR_Func_620a6);
	}

	void port_Func_3353()
	{
		homejp(BANK_Func_621fb);
		JUMP_TO(ADDR_Func_621fb);
	}

	void port_Func_3362()
	{
		homejp(BANK_Func_622bd);
		JUMP_TO(ADDR_Func_622bd);
	}

	void port_Func_3371()
	{
		homejp(BANK_Func_62926);
		JUMP_TO(ADDR_Func_62926);
	}

	void port_Func_3380()
	{
		homejp(BANK_Func_62382);
		JUMP_TO(ADDR_Func_62382);
	}

	void port_Func_338f()
	{
		homejp(BANK_Func_6247b);
		JUMP_TO(ADDR_Func_6247b);
	}

	void port_Func_339e()
	{
		homejp(BANK_Func_62574);
		JUMP_TO(ADDR_Func_62574);
	}

	void port_Func_33ad()
	{
		homejp(BANK_Func_62605);
		JUMP_TO(ADDR_Func_62605);
	}

	void port_ObjState_FullThrowLeft()
	{
		homejp(BANK_Func_626da);
		JUMP_TO(ADDR_Func_626da);
	}

	void port_ObjState_FullThrowRight()
	{
		homejp(BANK_Func_62768);
		JUMP_TO(ADDR_Func_62768);
	}

	void port_ObjState_BumpLeft()
	{
		homejp(BANK__ObjState_BumpLeft);
		JUMP_TO(ADDR__ObjState_BumpLeft);
	}

	void port_ObjState_BumpRight()
	{
		homejp(BANK__ObjState_BumpRight);
		JUMP_TO(ADDR__ObjState_BumpRight);
	}

	void port_Func_33f8()
	{
		homejp(BANK_Func_62892);
		JUMP_TO(ADDR_Func_62892);
	}

	void port_Func_3407()
	{
		homejp(BANK_Func_6283a);
		JUMP_TO(ADDR_Func_6283a);
	}

	void port_CreateObjectAtRelativePos()
	{
		homecall(BANK__CreateObjectAtRelativePos, ADDR__CreateObjectAtRelativePos);
	}

	void port_CreateObjectFromCurObjPos()
	{
		homecall(BANK__CreateObjectFromCurObjPos, ADDR__CreateObjectFromCurObjPos);
	}

	void port_Func_3444()
	{
		homecall(BANK_Func_632ac, ADDR_Func_632ac);
	}

	void port_CountActiveTransientObjects()
	{
		homecall(BANK__CountActiveTransientObjects, ADDR__CountActiveTransientObjects);
	}

	void port_ApplyObjSetMovement()
	{
		homecall(BANK__ApplyObjSetMovement, ADDR__ApplyObjSetMovement);
	}

	void port_ApplyObjYMovement_Loop()
	{
		homecall(BANK__ApplyObjYMovement_Loop, ADDR__ApplyObjYMovement_Loop);
	}

	void port_ApplyObjXMovement_Loop()
	{
		homecall(BANK__ApplyObjXMovement_Loop, ADDR__ApplyObjXMovement_Loop);
	}

	void port_ApplyObjYMovement()
	{
		homecall(BANK__ApplyObjYMovement, ADDR__ApplyObjYMovement);
	}

	void port_ApplyObjXMovement()
	{
		homecall(BANK__ApplyObjXMovement, ADDR__ApplyObjXMovement);
	}

	void port_VanishObject()
	{
		homecall(BANK__VanishObject, ADDR__VanishObject);
	}

	void port_Func_34fc()
	{
		homecall(BANK_Func_63209, ADDR_Func_63209);
	}

	// The wrappers that run ProcessBlock: homecall, then ld a, b
	void homecall_process_block(uint8_t bank, uint16_t addr)
	{
		homejp(bank);
		gb_call(gb_, addr);
		pop_af();
		bankswitch(cpu_.a);
		cpu_.a = cpu_.b;
	}

	void port_Func_3513() { homecall_process_block(BANK_Func_19bc3, ADDR_Func_19bc3); }
	void port_Func_352b() { homecall_process_block(BANK_Func_19b7b, ADDR_Func_19b7b); }
	void port_Func_3543() { homecall_process_block(BANK_Func_19b51, ADDR_Func_19b51); }
	void port_ProcessBlockOnLeft() { homecall_process_block(BANK__ProcessBlockOnLeft, ADDR__ProcessBlockOnLeft); }
	void port_ProcessBlockOnRight() { homecall_process_block(BANK__ProcessBlockOnRight, ADDR__ProcessBlockOnRight); }
	void port_Func_358b() { homecall_process_block(BANK_Func_19b9b, ADDR_Func_19b9b); }
	void port_Func_35a3() { homecall_process_block(BANK_Func_19b8b, ADDR_Func_19b8b); }

	// ld a, c / add a / add a / add a / or autoinc / ldh [index], a /
	// ld c, LOW(data)
	void select_pal(uint16_t index_reg, uint8_t autoinc, uint16_t data_reg)
	{
		cpu_.a = alu_.add(cpu_.c, cpu_.c);
		cpu_.a = alu_.add(cpu_.a, cpu_.a);
		cpu_.a = alu_.add(cpu_.a, cpu_.a); // *8
		cpu_.a = alu_.or_(cpu_.a, autoinc);
		wr8(index_reg, cpu_.a);
		cpu_.c = (uint8_t)data_reg;
	}

	// wait_not_hblank / wait_hblank
	void wait_next_hblank()
	{
		while (!(cpu_.a = alu_.and_(rd8(rSTAT), STAT_MODE)))
			idle(32);
		while ((cpu_.a = alu_.and_(rd8(rSTAT), STAT_MODE)))
			idle(32);
	}

	// copies b palettes from hl, one per H-Blank
	void set_pals(uint16_t index_reg, uint8_t autoinc, uint16_t data_reg)
	{
		select_pal(index_reg, autoinc, data_reg);
		uint16_t hl = REG_HL(gb_);
		do {
			wait_next_hblank();
			for (int i = 0; i < PAL_SIZE; i++) {
				cpu_.a = rd8(hl++);
				wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
			}
			cpu_.b = alu_.dec(cpu_.b);
		} while (cpu_.b);
		SET_HL(gb_, hl);
	}

	// blacks out b palettes, one per H-Blank
	void black_out_pals(uint16_t index_reg, uint8_t autoinc, uint16_t data_reg)
	{
		select_pal(index_reg, autoinc, data_reg);
		do {
			wait_next_hblank();
			cpu_.a = alu_.xor_(cpu_.a, cpu_.a);
			for (int i = 0; i < PAL_SIZE; i++)
				wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
			cpu_.b = alu_.dec(cpu_.b);
		} while (cpu_.b);
	}

	// c = OB pal index, b = num of pals, hl = pals
	void port_SetOBPals() { set_pals(rOBPI, OBPI_AUTOINC, rOBPD); }
	// c = BG pal index, b = num of pals, hl = pals
	void port_SetBGPals() { set_pals(rBGPI, BGPI_AUTOINC, rBGPD); }
	// blacks out b OB palettes starting from OB palette index c
	void port_BlackOutOBPals() { black_out_pals(rOBPI, OBPI_AUTOINC, rOBPD); }
	// blacks out b BG palettes starting from BG palette index c
	void port_BlackOutBGPals() { black_out_pals(rBGPI, BGPI_AUTOINC, rBGPD); }
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeObjectMechanics;

// only DrawObjects calls it, and reads none after it
PORT_EX(UpdateObjSprite, 0, ALL_REGISTERS) { wl3::home::update_obj_sprite(gb, REG_HL(gb), static_cast<uint16_t>(gb->cpu.h << 8 | gb->cpu.e)); }
PORT_EX(DecrementRemainingBossHits, 0, 0) { HomeObjectMechanics(gb).port_DecrementRemainingBossHits(); }
PORT_EX(UpdateObjectFrame, 0, 0) { HomeObjectMechanics(gb).port_UpdateObjectFrame(); }
PORT_EX(UpdateObjectAnimation, 0, 0) { HomeObjectMechanics(gb).port_UpdateObjectAnimation(); }
PORT_EX(MoveObjectLeftByVar2, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectLeftByVar2(); }
PORT_EX(MoveObjectRightByVar2, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectRightByVar2(); }
PORT_EX(MoveObjectUpByVar2, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectUpByVar2(); }
PORT_EX(MoveObjectDownByVar2, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectDownByVar2(); }
PORT_EX(MoveObjectLeft_Fast, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectLeft_Fast(); }
PORT_EX(MoveObjectRight_Fast, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectRight_Fast(); }
PORT_EX(MoveObjectDown_Fast, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectDown_Fast(); }
PORT_EX(MoveObjectUp_Fast, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectUp_Fast(); }
PORT_EX(MoveObjectRight_Slow, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectRight_Slow(); }
PORT_EX(MoveObjectRight, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectRight(); }
PORT_EX(MoveObjectLeft_Slow, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectLeft_Slow(); }
PORT_EX(MoveObjectLeft, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectLeft(); }
PORT_EX(MoveObjectDown_Slow, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectDown_Slow(); }
PORT_EX(MoveObjectDown, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectDown(); }
PORT_EX(MoveObjectUp_Slow, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectUp_Slow(); }
PORT_EX(MoveObjectUp, 0, 0) { HomeObjectMechanics(gb).port_MoveObjectUp(); }
PORT_EX(SetObjectFramesetPtr, 0, 0) { HomeObjectMechanics(gb).port_SetObjectFramesetPtr(); }
PORT_EX(HomeJumpRet, 0, 0) { HomeObjectMechanics(gb).port_HomeJumpRet(); }
PORT_EX(SetOneWayPlatformAction, 0, 0) { HomeObjectMechanics(gb).port_SetOneWayPlatformAction(); }
PORT_EX(Func_3173, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3173(); }
PORT_EX(VanishObject2, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_VanishObject2(); }
PORT_EX(Func_3191, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3191(); }
PORT_EX(Func_31a0, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_31a0(); }
PORT_EX(Func_31af, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_31af(); }
PORT_EX(Func_31be, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_31be(); }
PORT_EX(Func_31cd, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_31cd(); }
PORT_EX(Func_31dc, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_31dc(); }
PORT_EX(Func_31eb, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_31eb(); }
PORT_EX(Func_31fa, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_31fa(); }
PORT_EX(Func_3209, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3209(); }
PORT_EX(Func_3218, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3218(); }
PORT_EX(Func_3227, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3227(); }
PORT_EX(Func_3236, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3236(); }
PORT_EX(Func_3245, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3245(); }
PORT_EX(Func_3254, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3254(); }
PORT_EX(Func_3263, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3263(); }
PORT_EX(Func_3272, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3272(); }
PORT_EX(Func_3281, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3281(); }
PORT_EX(ObjState_Fall, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_ObjState_Fall(); }
PORT_EX(ObjState_StandingFall, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_ObjState_StandingFall(); }
PORT_EX(Func_32ae, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_32ae(); }
PORT_EX(Func_32bd, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_32bd(); }
PORT_EX(Func_32cc, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_32cc(); }
PORT_EX(Func_32db, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_32db(); }
PORT_EX(Func_32ea, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_32ea(); }
PORT_EX(Func_32f9, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_32f9(); }
PORT_EX(Func_3308, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3308(); }
PORT_EX(Func_3317, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3317(); }
PORT_EX(Func_3326, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3326(); }
PORT_EX(Func_3335, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3335(); }
PORT_EX(Func_3344, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3344(); }
PORT_EX(Func_3353, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3353(); }
PORT_EX(Func_3362, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3362(); }
PORT_EX(Func_3371, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3371(); }
PORT_EX(Func_3380, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3380(); }
PORT_EX(Func_338f, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_338f(); }
PORT_EX(Func_339e, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_339e(); }
PORT_EX(Func_33ad, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_33ad(); }
PORT_EX(ObjState_FullThrowLeft, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_ObjState_FullThrowLeft(); }
PORT_EX(ObjState_FullThrowRight, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_ObjState_FullThrowRight(); }
PORT_EX(ObjState_BumpLeft, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_ObjState_BumpLeft(); }
PORT_EX(ObjState_BumpRight, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_ObjState_BumpRight(); }
PORT_EX(Func_33f8, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_33f8(); }
PORT_EX(Func_3407, HOOK_MANUAL_RET, 0) { HomeObjectMechanics(gb).port_Func_3407(); }
PORT_EX(CreateObjectAtRelativePos, 0, 0) { HomeObjectMechanics(gb).port_CreateObjectAtRelativePos(); }
PORT_EX(CreateObjectFromCurObjPos, 0, 0) { HomeObjectMechanics(gb).port_CreateObjectFromCurObjPos(); }
PORT_EX(Func_3444, 0, ALL_REGISTERS & ~R_E) { HomeObjectMechanics(gb).port_Func_3444(); }
PORT_EX(CountActiveTransientObjects, 0, 0) { HomeObjectMechanics(gb).port_CountActiveTransientObjects(); }
// The movement wrappers homecall update_objects.cpp's plain movement code,
// which leaves b and c (and for the set movement and Func_34fc all of them)
// as it pleases: the objects read only the d, e and h these pass on
// (live.py), and e from Func_3444.
PORT_EX(ApplyObjSetMovement, 0, ALL_REGISTERS) { HomeObjectMechanics(gb).port_ApplyObjSetMovement(); }
PORT_EX(ApplyObjYMovement_Loop, 0, R_B | R_C) { HomeObjectMechanics(gb).port_ApplyObjYMovement_Loop(); }
PORT_EX(ApplyObjXMovement_Loop, 0, R_B | R_C) { HomeObjectMechanics(gb).port_ApplyObjXMovement_Loop(); }
PORT_EX(ApplyObjYMovement, 0, R_B | R_C) { HomeObjectMechanics(gb).port_ApplyObjYMovement(); }
PORT_EX(ApplyObjXMovement, 0, R_B | R_C) { HomeObjectMechanics(gb).port_ApplyObjXMovement(); }
// homecalls the plain _VanishObject; its callers read no register after it
PORT_EX(VanishObject, 0, ALL_REGISTERS) { HomeObjectMechanics(gb).port_VanishObject(); }
PORT_EX(Func_34fc, 0, ALL_REGISTERS) { HomeObjectMechanics(gb).port_Func_34fc(); }
PORT_EX(Func_3513, 0, 0) { HomeObjectMechanics(gb).port_Func_3513(); }
PORT_EX(Func_352b, 0, 0) { HomeObjectMechanics(gb).port_Func_352b(); }
PORT_EX(Func_3543, 0, 0) { HomeObjectMechanics(gb).port_Func_3543(); }
PORT_EX(ProcessBlockOnLeft, 0, 0) { HomeObjectMechanics(gb).port_ProcessBlockOnLeft(); }
PORT_EX(ProcessBlockOnRight, 0, 0) { HomeObjectMechanics(gb).port_ProcessBlockOnRight(); }
PORT_EX(Func_358b, 0, 0) { HomeObjectMechanics(gb).port_Func_358b(); }
PORT_EX(Func_35a3, 0, 0) { HomeObjectMechanics(gb).port_Func_35a3(); }
PORT_EX(SetOBPals, 0, 0) { HomeObjectMechanics(gb).port_SetOBPals(); }
PORT_EX(SetBGPals, 0, 0) { HomeObjectMechanics(gb).port_SetBGPals(); }
PORT_EX(BlackOutOBPals, 0, 0) { HomeObjectMechanics(gb).port_BlackOutOBPals(); }
PORT_EX(BlackOutBGPals, 0, 0) { HomeObjectMechanics(gb).port_BlackOutBGPals(); }
