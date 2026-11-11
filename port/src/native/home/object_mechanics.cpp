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
	mem.cur_obj_x() = static_cast<uint16_t>(mem.cur_obj_x() + dx);
}

void move_object_y(GB *gb, int dy)
{
	Memory mem(gb);
	mem.cur_obj_y() = static_cast<uint16_t>(mem.cur_obj_y() + dy);
}

void set_object_frameset_ptr(GB *gb, uint16_t frameset)
{
	Memory mem(gb);
	mem.word(wCurObjFramesetPtr) = frameset;
	mem.cur_obj_frame_duration() = 0;
	mem.cur_obj_frameset_offset() = 0;
}

// UpdateObjSprite, plain: see object_mechanics.hpp
void update_obj_sprite(GB *gb, uint16_t id, uint16_t pos)
{
	Memory mem(gb);
	uint8_t bank = mem.rom_bank();
	uint8_t v = mem[id];
	// the object's OAM bank, from its id's high nibble
	auto sprite_bank = static_cast<uint8_t>((((v << 4) | (v >> 4)) & 0x7) | BANK_OAM_180000);
	bankswitch(gb, sprite_bank, ADDR_UpdateObjSprite);
	uint8_t y = mem[pos], x = mem[pos + 1], frame = mem[pos + 2];
	mem.cur_sprite_y_coord() = y;
	mem.cur_sprite_x_coord() = x;
	mem.cur_sprite_frame() = frame;
	// the OAM pointer plus frame * 2 (an 8-bit add a)
	uint16_t entry = static_cast<uint16_t>(mem.word(pos + 3) + static_cast<uint8_t>(frame * 2));
	RomPtr data(gb, sprite_bank, RomPtr(gb, sprite_bank, entry).word());
	uint16_t oam = static_cast<uint16_t>((wVirtualOAM & 0xff00) | mem.virtual_oam_byte_size());
	while ((oam & 0xff) < (wVirtualOAMEnd & 0xff) && *data != 0x80) {
		mem[oam++] = static_cast<uint8_t>(data[0] + y); // SPRITEOAMSTRUCT_YCOORD
		mem[oam++] = static_cast<uint8_t>(data[1] + x); // SPRITEOAMSTRUCT_XCOORD
		mem[oam++] = data[2]; // SPRITEOAMSTRUCT_TILE_ID
		mem[oam++] = data[3]; // SPRITEOAMSTRUCT_ATTRIBUTES
		mem.virtual_oam_byte_size() = static_cast<uint8_t>(oam);
		data += 4;
	}
	bankswitch(gb, bank, ADDR_UpdateObjSprite);
}

void update_object_animation(GB *gb)
{
	Memory mem(gb);
	uint8_t duration = mem.cur_obj_frame_duration();
	mem.cur_obj_frame_duration() = static_cast<uint8_t>(duration - 1);
	if (duration) // sub 1 / ret nc
		return;
	uint8_t bank = mem.rom_bank();
	bankswitch(gb, BANK_Frameset_68000, ADDR_UpdateObjectAnimation);
	RomPtr frameset(gb, BANK_Frameset_68000, mem.word(wCurObjFramesetPtr));
	RomPtr entry = frameset + mem.cur_obj_frameset_offset();
	if (*entry != 0xff) {
		mem.cur_obj_frame() = *entry;
		mem.cur_obj_frameset_offset() = static_cast<uint8_t>(mem.cur_obj_frameset_offset() + 2);
		mem.cur_obj_frame_duration() = entry[1];
	} else { // .reset
		mem.cur_obj_frameset_offset() = 2;
		mem.cur_obj_frame() = frameset[0];
		mem.cur_obj_frame_duration() = frameset[1];
	}
	bankswitch(gb, bank, ADDR_HomeJumpRet);
}

void update_object_frame(GB *gb)
{
	Memory mem(gb);
	if (mem.cur_obj_frame_duration()) // sub 1 / ret nc
		return;
	uint8_t bank = mem.rom_bank();
	bankswitch(gb, BANK_Frameset_68000, ADDR_UpdateObjectFrame);
	RomPtr frameset(gb, BANK_Frameset_68000, mem.word(wCurObjFramesetPtr));
	uint8_t frame = frameset[mem.cur_obj_frameset_offset()];
	mem.cur_obj_frame() = frame == 0xff ? frameset[0] : frame;
	bankswitch(gb, bank, ADDR_HomeJumpRet);
}

void set_one_way_platform_action(GB *gb)
{
	Memory mem(gb);
	if (mem.cur_obj_flags() & OBJFLAG_STEPPED)
		return;
	// wCurObjCollBoxTop + wCurObjScreenYPos + $2a + 3, against Wario's + $2a
	auto top = static_cast<uint8_t>(mem.cur_obj_screen_y_pos() + 0x2a + 3 + mem.cur_obj_coll_box_top());
	if (static_cast<uint8_t>(mem.wario_screen_y_pos() + 0x2a) < top)
		return; // Wario is over the collision box
	mem.cur_obj_action() = (1 << NO_ACTIONS_F) | 1; // no_actions_for 1
}

namespace {

class HomeObjectMechanics : public AsmModule {
public:
	using AsmModule::AsmModule;

	// The wrappers call with gb_call rather than CALL:
	// bankswitch only sets the low ROM bank register, so if the high bit is set
	// the asm runs whatever is mapped, and so must the port.

	// The homejp macro, up to the jp: ld a, [wROMBank] / push af /
	// ld a, BANK(target) / bankswitch
	void homejp(uint8_t bank)
	{
		cpu_.a = mem_.rom_bank();
		push_af();
		cpu_.a = bank;
		bankswitch(bank);
	}

	// the routine at bank:addr (in play found by its bank)
	void call_in(uint8_t bank, uint16_t addr)
	{
		if (gb_->test.active || !gb_call_native_in(gb_, bank, addr))
			gb_call(gb_, addr);
	}

	// The homecall macro: homejp, but with a call, then pop af / bankswitch
	void homecall(uint8_t bank, uint16_t addr)
	{
		homejp(bank);
		call_in(bank, addr);
		pop_af();
		bankswitch(cpu_.a);
	}

	// The tail of routines entered with homejp, which pushed the caller's bank.
	void port_HomeJumpRet()
	{
		pop_af();
		bankswitch(cpu_.a);
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
		call_in(bank, addr);
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

	// The palette routines, plain (home/pals.cpp), leaving what the asm
	// does: b 0 from the last dec b (z, n; c clear from the and / or
	// before), c the data register, hl past the source, a the last byte
	// (0 for the black outs)
	void pals_regs(bool bg)
	{
		cpu_.b = 0;
		cpu_.c = static_cast<uint8_t>(bg ? rBGPD : rOBPD);
		cpu_.f = FLAG_Z | FLAG_N;
	}
	void set_pals_hook(bool bg)
	{
		uint8_t last;
		uint16_t end = home::set_pals(gb_, bg, REG_HL(gb_), cpu_.b, cpu_.c, &last);
		pals_regs(bg);
		cpu_.a = last;
		SET_HL(gb_, end);
	}
	void black_out_pals_hook(bool bg)
	{
		home::black_out_pals(gb_, bg, cpu_.b, cpu_.c);
		pals_regs(bg);
		cpu_.a = 0;
	}

	// c = OB pal index, b = num of pals, hl = pals
	void port_SetOBPals() { set_pals_hook(false); }
	// c = BG pal index, b = num of pals, hl = pals
	void port_SetBGPals() { set_pals_hook(true); }
	// blacks out b OB palettes starting from OB palette index c
	void port_BlackOutOBPals() { black_out_pals_hook(false); }
	// blacks out b BG palettes starting from BG palette index c
	void port_BlackOutBGPals() { black_out_pals_hook(true); }
};

} // namespace

} // namespace wl3::home

namespace wl3::level {
// The homejp state wrappers below run their states plain (update_objects.cpp)
bool object_state(GB *gb, uint16_t wrapper);
} // namespace wl3::level

using wl3::home::HomeObjectMechanics;
using namespace wl3::home;

// only DrawObjects calls it, and reads none after it
PORT_EX(UpdateObjSprite, 0, ALL_REGISTERS) { wl3::home::update_obj_sprite(gb, REG_HL(gb), static_cast<uint16_t>(gb->cpu.h << 8 | gb->cpu.e)); }
// The object movers, animation and frameset, plain: the objects (C++)
// call them plain (Object::call_home) and UpdateObjects reads no register
// after the animation, so the hooks clobber them all.
namespace {

void move_slow(GB *gb, bool x, int d)
{
	// ld a, [wGlobalCounter] / rra: right and down on even frames, left
	// and up on odd ones
	bool odd = wl3::Memory(gb).global_counter() & 1;
	if (odd == (d < 0)) {
		if (x)
			wl3::home::move_object_x(gb, d);
		else
			wl3::home::move_object_y(gb, d);
	}
}

uint8_t var2(GB *gb) { return wl3::Memory(gb).cur_obj_var2(); }

} // namespace

// h = the object's page: its hits left (the substate's low nibble) down
// one, unless it's the last
PORT_EX(DecrementRemainingBossHits, 0, ALL_REGISTERS)
{
	wl3::Memory mem(gb);
	auto substate = static_cast<uint16_t>(gb->cpu.h << 8 | OBJ_SUBSTATE);
	if ((mem[substate] & 0x0f) != 1)
		mem[substate] = static_cast<uint8_t>(mem[substate] - 1);
}
PORT_EX(UpdateObjectFrame, 0, ALL_REGISTERS) { update_object_frame(gb); }
PORT_EX(UpdateObjectAnimation, 0, ALL_REGISTERS) { update_object_animation(gb); }
PORT_EX(MoveObjectLeftByVar2, 0, ALL_REGISTERS) { move_object_x(gb, -var2(gb)); }
PORT_EX(MoveObjectRightByVar2, 0, ALL_REGISTERS) { move_object_x(gb, var2(gb)); }
PORT_EX(MoveObjectUpByVar2, 0, ALL_REGISTERS) { move_object_y(gb, -var2(gb)); }
PORT_EX(MoveObjectDownByVar2, 0, ALL_REGISTERS) { move_object_y(gb, var2(gb)); }
PORT_EX(MoveObjectLeft_Fast, 0, ALL_REGISTERS) { move_object_x(gb, -2); }
PORT_EX(MoveObjectRight_Fast, 0, ALL_REGISTERS) { move_object_x(gb, 2); }
PORT_EX(MoveObjectDown_Fast, 0, ALL_REGISTERS) { move_object_y(gb, 2); }
PORT_EX(MoveObjectUp_Fast, 0, ALL_REGISTERS) { move_object_y(gb, -2); }
PORT_EX(MoveObjectRight_Slow, 0, ALL_REGISTERS) { move_slow(gb, true, 1); }
PORT_EX(MoveObjectRight, 0, ALL_REGISTERS) { move_object_x(gb, 1); }
PORT_EX(MoveObjectLeft_Slow, 0, ALL_REGISTERS) { move_slow(gb, true, -1); }
PORT_EX(MoveObjectLeft, 0, ALL_REGISTERS) { move_object_x(gb, -1); }
PORT_EX(MoveObjectDown_Slow, 0, ALL_REGISTERS) { move_slow(gb, false, 1); }
PORT_EX(MoveObjectDown, 0, ALL_REGISTERS) { move_object_y(gb, 1); }
PORT_EX(MoveObjectUp_Slow, 0, ALL_REGISTERS) { move_slow(gb, false, -1); }
PORT_EX(MoveObjectUp, 0, ALL_REGISTERS) { move_object_y(gb, -1); }
// de = the frameset
PORT_EX(SetObjectFramesetPtr, 0, ALL_REGISTERS) { set_object_frameset_ptr(gb, REG_DE(gb)); }
PORT_EX(HomeJumpRet, 0, 0) { HomeObjectMechanics(gb).port_HomeJumpRet(); }
// the objects (C++) read no register after it
PORT_EX(SetOneWayPlatformAction, 0, ALL_REGISTERS) { set_one_way_platform_action(gb); }
PORT_EX(Func_3173, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3173); }
PORT_EX(VanishObject2, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_VanishObject2); }
PORT_EX(Func_3191, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3191); }
PORT_EX(Func_31a0, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_31a0); }
PORT_EX(Func_31af, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_31af); }
PORT_EX(Func_31be, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_31be); }
PORT_EX(Func_31cd, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_31cd); }
PORT_EX(Func_31dc, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_31dc); }
PORT_EX(Func_31eb, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_31eb); }
PORT_EX(Func_31fa, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_31fa); }
PORT_EX(Func_3209, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3209); }
PORT_EX(Func_3218, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3218); }
PORT_EX(Func_3227, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3227); }
PORT_EX(Func_3236, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3236); }
PORT_EX(Func_3245, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3245); }
PORT_EX(Func_3254, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3254); }
PORT_EX(Func_3263, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3263); }
PORT_EX(Func_3272, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3272); }
PORT_EX(Func_3281, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3281); }
PORT_EX(ObjState_Fall, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_ObjState_Fall); }
PORT_EX(ObjState_StandingFall, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_ObjState_StandingFall); }
PORT_EX(Func_32ae, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_32ae); }
PORT_EX(Func_32bd, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_32bd); }
PORT_EX(Func_32cc, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_32cc); }
PORT_EX(Func_32db, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_32db); }
PORT_EX(Func_32ea, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_32ea); }
PORT_EX(Func_32f9, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_32f9); }
PORT_EX(Func_3308, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3308); }
PORT_EX(Func_3317, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3317); }
PORT_EX(Func_3326, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3326); }
PORT_EX(Func_3335, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3335); }
PORT_EX(Func_3344, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3344); }
PORT_EX(Func_3353, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3353); }
PORT_EX(Func_3362, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3362); }
PORT_EX(Func_3371, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3371); }
PORT_EX(Func_3380, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3380); }
PORT_EX(Func_338f, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_338f); }
PORT_EX(Func_339e, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_339e); }
PORT_EX(Func_33ad, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_33ad); }
PORT_EX(ObjState_FullThrowLeft, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_ObjState_FullThrowLeft); }
PORT_EX(ObjState_FullThrowRight, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_ObjState_FullThrowRight); }
PORT_EX(ObjState_BumpLeft, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_ObjState_BumpLeft); }
PORT_EX(ObjState_BumpRight, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_ObjState_BumpRight); }
PORT_EX(Func_33f8, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_33f8); }
PORT_EX(Func_3407, 0, ALL_REGISTERS) { wl3::level::object_state(gb, ADDR_Func_3407); }
PORT_EX(CreateObjectAtRelativePos, 0, ALL_REGISTERS) { HomeObjectMechanics(gb).port_CreateObjectAtRelativePos(); }
PORT_EX(CreateObjectFromCurObjPos, 0, ALL_REGISTERS) { HomeObjectMechanics(gb).port_CreateObjectFromCurObjPos(); }
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
PORT_EX(Func_3513, 0, ALL_REGISTERS & ~(R_A | R_B)) { HomeObjectMechanics(gb).port_Func_3513(); }
PORT_EX(Func_352b, 0, ALL_REGISTERS & ~(R_A | R_B)) { HomeObjectMechanics(gb).port_Func_352b(); }
PORT_EX(Func_3543, 0, ALL_REGISTERS & ~(R_A | R_B)) { HomeObjectMechanics(gb).port_Func_3543(); }
PORT_EX(ProcessBlockOnLeft, 0, ALL_REGISTERS & ~(R_A | R_B)) { HomeObjectMechanics(gb).port_ProcessBlockOnLeft(); }
PORT_EX(ProcessBlockOnRight, 0, ALL_REGISTERS & ~(R_A | R_B)) { HomeObjectMechanics(gb).port_ProcessBlockOnRight(); }
PORT_EX(Func_358b, 0, ALL_REGISTERS & ~(R_A | R_B)) { HomeObjectMechanics(gb).port_Func_358b(); }
PORT_EX(Func_35a3, 0, ALL_REGISTERS & ~(R_A | R_B)) { HomeObjectMechanics(gb).port_Func_35a3(); }
PORT_EX(SetOBPals, 0, 0) { HomeObjectMechanics(gb).port_SetOBPals(); }
PORT_EX(SetBGPals, 0, 0) { HomeObjectMechanics(gb).port_SetBGPals(); }
PORT_EX(BlackOutOBPals, 0, 0) { HomeObjectMechanics(gb).port_BlackOutOBPals(); }
PORT_EX(BlackOutBGPals, 0, 0) { HomeObjectMechanics(gb).port_BlackOutBGPals(); }
