// A base for the C++ modules that still have to behave exactly like the
// asm toward register-level callers (most were converted from the
// register-level C by tools/c2cpp.py): the level engine, the home
// routines and the rest. Its members are the asm idioms
// these modules are written in (ld / st, the flags of and / cp / bit,
// push af, sramswitch, the Wario animation and collision box helpers),
// each leaving the registers and flags the instructions do, on the real
// CPU state and memory map.
//
// The macros (CALL, JUMP, FARCALL, RET...) are the calls and jumps between
// routines, by address, with the asm's return addresses where a callee may
// read them (see HANDOFF.md).
#pragma once

#include "alu.hpp"
#include "level/collisions.hpp"
#include "native.hpp"
#include "routines.hpp"
#include "home/sprites.hpp"

#include <initializer_list>

extern "C" {
void port_call(GB *gb, int bank, uint16_t addr, const char *name);
void port_jump(GB *gb, int bank, uint16_t addr, const char *name);
void port_hcall(GB *gb, uint16_t addr);
}

// Calls and jumps, as in ported.h, on the module's gb_
#define CALL(name) port_call(gb_, BANK_##name, ADDR_##name, #name)
#define FARCALL(name) port_farcall(gb_, BANK_##name, ADDR_##name)
#define FARCALL_BANK(bank, name) port_farcall(gb_, (bank), ADDR_##name)
#define HCALL(name) port_hcall(gb_, ADDR_##name)
#define FARCALL_RET(name, ret) port_farcall_ret(gb_, BANK_##name, ADDR_##name, (ret))
#define JUMP(name) port_jump(gb_, BANK_##name, ADDR_##name, #name)
#undef JUMP_TO // native.hpp's, on gb
#define JUMP_TO(addr) (cpu_.pc = static_cast<uint16_t>(addr))
#define RET() (cpu_.pc = gb_pop16(gb_))
// jumps and calls within a bank, by address (they run whatever's mapped)
#define LJUMP(name) JUMP_TO(ADDR_##name)
#define LCALL(name) gb_call(gb_, ADDR_##name)
#define ZF (cpu_.f & FLAG_Z)
#define CF (cpu_.f & FLAG_C)

namespace wl3 {

constexpr uint8_t OAM1 = BANK_OAM_14000; // BANK("Wario OAM 1")
constexpr uint8_t OAM2 = BANK_OAM_1fc000; // BANK("Wario OAM 2")
constexpr uint8_t OAM3 = BANK_OAM_1dc000; // BANK("Wario OAM 3")

class AsmModule {
public:
	explicit AsmModule(GB *gb) : gb_(gb), cpu_(gb->cpu), mem_(gb), routines_(gb), alu_(gb->cpu) {}

protected:
	uint8_t rd8(uint16_t addr) const { return gb_read(gb_, addr); }
	void wr8(uint16_t addr, uint8_t v) { gb_write(gb_, addr, v); }
	uint16_t rd16(uint16_t addr) const { return gb_read16(gb_, addr); }
	void wr16(uint16_t addr, uint16_t v) { gb_write16(gb_, addr, v); }

	// ld a, v / ld [addr], a
	void st(uint16_t addr, uint8_t v)
	{
		cpu_.a = v;
		wr8(addr, v);
	}
	// ld a, [addr]
	uint8_t ld(uint16_t addr)
	{
		cpu_.a = rd8(addr);
		return cpu_.a;
	}
	void xor_a()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z;
	}
	// ld a, [addr] / and a: true if nz
	bool ld_and(uint16_t addr)
	{
		uint8_t v = ld(addr);
		return alu_.and_(v, v) != 0;
	}
	// ld a, b / and a: true if nz
	bool b_and()
	{
		cpu_.a = cpu_.b;
		return alu_.and_(cpu_.a, cpu_.a) != 0;
	}
	// Wario's collision checks (farcalled), true if they found one, with
	// the ld a, b / and a that tests their result in b
	bool centre_collision()
	{
		return collision_check(BANK_CheckCentreCollision, ADDR_CheckCentreCollision, level::asm_centre_collision);
	}
	bool front_collision()
	{
		return collision_check(BANK_CheckFrontCollision, ADDR_CheckFrontCollision, level::asm_front_collision);
	}
	bool up_collision() { return collision_check(BANK_CheckUpCollision, ADDR_CheckUpCollision, level::asm_up_collision); }
	bool airborne_collision()
	{
		return collision_check(BANK_CheckAirborneCollision, ADDR_CheckAirborneCollision,
			level::asm_airborne_collision);
	}
	bool collision_check(uint8_t bank, uint16_t addr, void (*check)(GB *))
	{
		run_check(bank, addr, check);
		return b_and();
	}
	// The same farcalls with no test after them, for callers that set a and
	// f before they read them (a and f are left as the check leaves them,
	// not as hCallFunc's pop af would): the result in b
	void run_centre_collision() { run_check(BANK_CheckCentreCollision, ADDR_CheckCentreCollision, level::asm_centre_collision); }
	void run_front_collision() { run_check(BANK_CheckFrontCollision, ADDR_CheckFrontCollision, level::asm_front_collision); }
	void run_up_collision() { run_check(BANK_CheckUpCollision, ADDR_CheckUpCollision, level::asm_up_collision); }
	void run_airborne_collision()
	{
		run_check(BANK_CheckAirborneCollision, ADDR_CheckAirborneCollision, level::asm_airborne_collision);
	}
	void run_check(uint8_t bank, uint16_t addr, void (*check)(GB *))
	{
		routines_.in_bank(bank, addr, [&] { check(gb_); });
	}
	// ld a, [addr] / bit n, a
	bool ld_bit(uint16_t addr, int n) { return alu_.bit(ld(addr), n); }
	// ld a, [addr] / and mask: true if nz
	bool ld_mask(uint16_t addr, uint8_t mask)
	{
		cpu_.a = alu_.and_(rd8(addr), mask);
		return cpu_.a != 0;
	}
	// ld a, [addr] / cp v
	void ld_cp(uint16_t addr, uint8_t v) { alu_.cp(ld(addr), v); }

	// the play_sfx macro: a ends as the SFX's low byte
	void play_sfx(uint16_t sfx)
	{
		wr8(hSFXID + 0, static_cast<uint8_t>(sfx >> 8));
		cpu_.a = static_cast<uint8_t>(sfx);
		mem_.sfx().lo() = cpu_.a;
	}
	void bankswitch(uint8_t bank)
	{
		mem_.rom_bank() = bank;
		wr8(rROMB0 + 0x100, bank);
	}
	void sramswitch(uint8_t bank)
	{
		mem_.sram_bank() = bank;
		wr8(rRAMB + 0x100, bank);
	}
	// push af / pop af, on the real stack
	void push_af() { gb_push16(gb_, REG_AF(gb_)); }
	void pop_af()
	{
		uint16_t v = gb_pop16(gb_);
		cpu_.a = static_cast<uint8_t>(v >> 8);
		cpu_.f = static_cast<uint8_t>(v & 0xf0);
	}
	// emulated time passing, as the asm's polling loops
	void idle(int cycles)
	{
		if (!gb_->no_tick)
			gb_tick(gb_, cycles);
	}

	// wDMASourceBank/Ptr = gfx, call LoadWarioGfx
	void load_gfx(uint8_t bank, uint16_t ptr)
	{
		mem_.dma_source_bank() = bank;
		wr8(wDMASourcePtr + 0, static_cast<uint8_t>(ptr >> 8));
		st(wDMASourcePtr + 1, static_cast<uint8_t>(ptr));
		home::load_wario_gfx(gb_);
	}
	void set_oam(uint8_t bank, uint16_t ptr)
	{
		mem_.wario_oam_bank() = bank;
		wr8(wWarioOAMPtr + 0, static_cast<uint8_t>(ptr >> 8));
		st(wWarioOAMPtr + 1, static_cast<uint8_t>(ptr));
	}
	void set_frameset(uint16_t ptr)
	{
		wr8(wWarioFramesetPtr + 0, static_cast<uint8_t>(ptr >> 8));
		st(wWarioFramesetPtr + 1, static_cast<uint8_t>(ptr));
	}
	// ld a, BANK("Wario OAM n") / ldh [hCallFuncBank], a / hcall UpdateWarioAnimation
	void anim(uint8_t bank)
	{
		home::farcall(gb_, bank, ADDR_UpdateWarioAnimation, [&] { home::update_wario_animation_regs(gb_, bank); });
	}
	// bottom -1, left -9, right 9, top -27 (a = -27)
	void coll_box_blrt()
	{
		mem_.collision_box_bottom() = static_cast<uint8_t>(-1);
		mem_.collision_box_left() = static_cast<uint8_t>(-9);
		mem_.collision_box_right() = 9;
		st(wCollisionBoxTop, static_cast<uint8_t>(-27));
	}
	// bottom -1, top -27, left -9, right 9 (a = 9)
	void coll_box_btlr()
	{
		mem_.collision_box_bottom() = static_cast<uint8_t>(-1);
		mem_.collision_box_top() = static_cast<uint8_t>(-27);
		mem_.collision_box_left() = static_cast<uint8_t>(-9);
		st(wCollisionBoxRight, 9);
	}
	void update_pos_y()
	{
		wr8(wWarioYPos + 0, mem_.y_pos_hi());
		cpu_.a = rd8(hYPosHi + 1);
		mem_.wario_y().lo() = cpu_.a;
		SET_HL(gb_, hYPosHi + 1);
		SET_DE(gb_, wWarioYPos + 1);
	}
	void get_pos()
	{
		for (int i = 0; i < 4; i++) {
			cpu_.a = rd8(wWarioPos + i);
			wr8(hPos + i, cpu_.a);
		}
		SET_HL(gb_, wWarioPos + 3);
		SET_DE(gb_, hPos + 3);
	}
	// ld a, [wSFXLoopCounter] / sub 1 / ...: the sfx every reload frames
	void sfx_loop(uint8_t reload, uint16_t sfx)
	{
		cpu_.a = alu_.sub(mem_.sfx_loop_counter(), 1);
		mem_.sfx_loop_counter() = cpu_.a;
		if (!CF)
			return;
		st(wSFXLoopCounter, reload);
		play_sfx(sfx);
	}
	void frameset_by_dir(uint16_t left, uint16_t right) { set_frameset(ld_and(wDirection) ? right : left); }
	bool room_transition() { return ld_and(wRoomTransitionParam); }
	bool anim_ended() { return ld_and(wWarioAnimationEnded); }
	void move_by_dir()
	{
		if (ld_and(wDirection))
			CALL(MoveWarioRight);
		else
			CALL(MoveWarioLeft);
	}
	void center_on_ladder()
	{
		cpu_.a = alu_.and_(mem_.wario_x().lo(), 0xf0);
		cpu_.a = alu_.add(cpu_.a, 0x08);
		mem_.wario_x().lo() = cpu_.a;
	}
	// ld hl, wWarioStateCounter / inc [hl] / ld a, [hl]
	uint8_t inc_counter()
	{
		SET_HL(gb_, wWarioStateCounter);
		cpu_.a = alu_.inc(mem_.wario_state_counter());
		mem_.wario_state_counter() = cpu_.a;
		return cpu_.a;
	}
	bool facing_right() { return ld_and(wDirection); }
	void collision_box(int8_t bottom, int8_t top, int8_t left, int8_t right)
	{
		st(wCollisionBoxBottom, static_cast<uint8_t>(bottom));
		st(wCollisionBoxTop, static_cast<uint8_t>(top));
		st(wCollisionBoxLeft, static_cast<uint8_t>(left));
		st(wCollisionBoxRight, static_cast<uint8_t>(right));
	}
	// xor a / ld [addr], a for each address (up to a 0)
	void clear(const uint16_t *addrs)
	{
		xor_a();
		for (; *addrs; addrs++)
			wr8(*addrs, 0);
	}
	void reset_anim()
	{
		xor_a();
		mem_.frame_duration() = 0;
		mem_.animation_frame() = 0;
	}
	void touch_states(uint8_t t, uint8_t ca94)
	{
		st(wTouchState, t);
		st(wStingTouchState, t);
		st(wca94, ca94);
	}
	void flip_dir()
	{
		cpu_.a = alu_.xor_(mem_.direction(), 0x1);
		mem_.direction() = cpu_.a;
	}
	bool flip_dir_and()
	{
		flip_dir();
		return alu_.and_(cpu_.a, cpu_.a) != 0;
	}

	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
	Alu alu_;
};

} // namespace wl3
