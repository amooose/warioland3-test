// Mizuuo (mizuuo.asm): a fish swimming back and forth that now and then
// surfaces and spits at Wario; the spit flies, then drops, and splats on
// a wall, the floor or anything it hits (or is lost in water). See
// object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t SWIMMING = ADDR_MizuuoFunc + 0x23; // .Update1
constexpr uint16_t SURFACING = ADDR_MizuuoFunc + 0x8f; // .Update2
constexpr uint16_t SPITTING = ADDR_MizuuoFunc + 0xb6; // .Update3
constexpr uint16_t DIVING = ADDR_MizuuoFunc + 0xff; // .Update4
constexpr uint16_t SPLAT = ADDR_MizzouProjectileFunc + 0x72; // .Update

constexpr uint8_t SURFACE = 0x5f; // var1 at which it surfaces

class Mizuuo : public Object {
public:
	using Object::Object;

	void init(); // MizuuoFunc
	void swimming(); // .Update1
	void surfacing(); // .Update2
	void spitting(); // .Update3
	void diving(); // .Update4
	void spit(uint16_t page); // MizzouProjectileFunc; page = h
	void splat(); // .Update

private:
	uint16_t facing(uint16_t left, uint16_t right) { return facing_right() ? right : left; }
	void set_splat(); // .asm_488b5
};

} // namespace

void Mizuuo::init()
{
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	set_update(SWIMMING);
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	set_frameset(ADDR_Frameset_686e5);
	var1() = 0x30;
	var2() = 0x08;
	move_left_by_var2();
}

// turning (for the state duration), or swimming var1 pixels, turning at
// the end; at var1 $5f it surfaces instead
void Mizuuo::swimming()
{
	no_actions();
	if (state_duration()) {
		if (--state_duration())
			return;
		substate() ^= OBJSUBFLAG_HDIR;
		set_frameset(facing(ADDR_Frameset_686e5, ADDR_Frameset_686ea));
		return;
	}
	if (var1() == SURFACE) {
		--var1();
		set_update(SURFACING);
		set_frameset(facing(ADDR_Frameset_686ef, ADDR_Frameset_686f8));
		state_duration() = 0x19;
		return;
	}
	if (--var1()) {
		facing_right() ? move_right() : move_left();
		return;
	}
	set_frameset(facing(ADDR_Frameset_68713, ADDR_Frameset_6871e));
	state_duration() = 0x22;
	var1() = 0x60;
}

void Mizuuo::surfacing()
{
	no_actions();
	if (--state_duration())
		return;
	set_update(SPITTING);
	set_frameset(facing(ADDR_Frameset_686a4, ADDR_Frameset_686b9));
	state_duration() = 0x5d;
}

// spits at $24 frames left; dives at the end (or when hit)
void Mizuuo::spitting()
{
	if (state()) {
		state() = 0;
	} else if (--state_duration()) {
		if (state_duration() != 0x24)
			return;
		play_sfx(SFX_07F);
		SET_BC(gb_, facing(ADDR_ObjParams_MizzouProjectileLeft, ADDR_ObjParams_MizzouProjectileRight));
		routines_.call(ADDR_CreateObjectAtRelativePos);
		return;
	}
	set_frameset(facing(ADDR_Frameset_68701, ADDR_Frameset_6870a));
	state_duration() = 0x19;
	set_update(DIVING);
}

void Mizuuo::diving()
{
	no_actions();
	if (--state_duration())
		return;
	set_update(SWIMMING);
	set_frameset(facing(ADDR_Frameset_686e5, ADDR_Frameset_686ea));
}

void Mizuuo::set_splat()
{
	set_update(SPLAT);
	set_frameset(ADDR_Frameset_686de);
	state_duration() = 0x17;
}

// rising (var1 0), then falling once its state duration's up
void Mizuuo::spit(uint16_t page)
{
	mem_[static_cast<uint16_t>(page | OBJ_FLAGS)] |= OBJFLAG_NO_COLLISION;
	mem_[hYPosLo] = mem_[wCurObjYPos + 0];
	mem_[hYPosHi] = mem_[wCurObjYPos + 1];
	mem_[hXPosLo] = mem_[wCurObjXPos + 0];
	mem_[hXPosHi] = mem_[wCurObjXPos + 1];
	routines_.call(ADDR_Func_3513);
	if (gb_->cpu.a) {
		set_splat();
		return;
	}
	uint16_t movement = ADDR_Data_60700;
	if (var1()) {
		routines_.call(ADDR_Func_3543);
		if (gb_->cpu.a) {
			set_splat();
			return;
		}
		if (mem_[wIsInWaterOrSand]) {
			flags() = 0;
			return;
		}
		movement = ADDR_Data_60740;
	}
	SET_BC(gb_, movement);
	routines_.call(ADDR_ApplyObjYMovement);
	if (state()) {
		set_splat();
		return;
	}
	if (!state_duration() || --state_duration())
		return;
	var1() = 0x01;
	set_frameset(facing(ADDR_Frameset_686d8, ADDR_Frameset_686db));
}

void Mizuuo::splat()
{
	no_actions();
	if (!--state_duration())
		flags() = 0;
}

} // namespace wl3::level

using Mizuuo = wl3::level::Mizuuo;

PORT_EX(MizuuoFunc, 0, ALL_REGISTERS) { Mizuuo(gb).init(); }
PORT_LOCAL_EX(MizuuoFunc, Update1, 0, ALL_REGISTERS) { Mizuuo(gb).swimming(); }
PORT_LOCAL_EX(MizuuoFunc, Update2, 0, ALL_REGISTERS) { Mizuuo(gb).surfacing(); }
PORT_LOCAL_EX(MizuuoFunc, Update3, 0, ALL_REGISTERS) { Mizuuo(gb).spitting(); }
PORT_LOCAL_EX(MizuuoFunc, Update4, 0, ALL_REGISTERS) { Mizuuo(gb).diving(); }
// MoveObjectLeft_Fast / Right_Fast leave h at wCurObj's page
PORT_EX(MizzouProjectileFunc_Left, 0, ALL_REGISTERS)
{
	gb_call(gb, ADDR_MoveObjectLeft_Fast);
	Mizuuo(gb).spit(wCurObjFlags & 0xff00);
}
PORT_EX(MizzouProjectileFunc_Right, 0, ALL_REGISTERS)
{
	gb_call(gb, ADDR_MoveObjectRight_Fast);
	Mizuuo(gb).spit(wCurObjFlags & 0xff00);
}
PORT_EX(MizzouProjectileFunc, 0, ALL_REGISTERS) { Mizuuo(gb).spit(gb->cpu.h << 8); }
PORT_LOCAL_EX(MizzouProjectileFunc, Update, 0, ALL_REGISTERS) { Mizuuo(gb).splat(); }
