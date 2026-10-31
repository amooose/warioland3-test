// The torches (torch.asm): burning torches that throw embers, and the
// flame block's torches, lit by a hit and going out after a while until
// all are lit at once. See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t LIT = ADDR_FlameBlockTorchUpdate + 0x3e; // .Update
constexpr uint16_t NO_EMBERS = ADDR_TorchNoEmbersFunc + 0x0b; // .Update
constexpr uint16_t BURNING = ADDR_TorchFunc + 0x28; // .Update
constexpr uint16_t EMBER_OUT = ADDR_TorchEmberCommonFunc + 0x41; // .Destroy

class Torch : public Object {
public:
	using Object::Object;

	void torch(); // TorchFunc
	void no_embers(); // TorchNoEmbersFunc
	void flame_block_torch(); // FlameBlockTorchFunc
	void unlit(); // FlameBlockTorchUpdate
	void lit(); // .Update
	void burning(); // TorchFunc.Update
	void ember_towards_wario(); // Func_40c19
	void ember(); // TorchEmberCommonFunc
	void ember_out(); // .Destroy

private:
	void set_up(uint16_t frameset); // .got_update_func
	void set_burning(); // .got_frameset
	void relight();
};

} // namespace

void Torch::torch()
{
	set_update(BURNING);
	set_up(ADDR_Frameset_68360);
}

void Torch::no_embers()
{
	set_update(NO_EMBERS);
	set_up(ADDR_Frameset_68360);
}

void Torch::flame_block_torch()
{
	set_update(ADDR_FlameBlockTorchUpdate);
	set_up(ADDR_Frameset_68369);
}

void Torch::set_up(uint16_t frameset)
{
	set_frameset(frameset);
	state_duration() = 0;
	flags() = (flags() & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION;
	substate() &= ~OBJSUBFLAG_UNINITIALISED;
	mem_[wCurObjCollBoxRight] = 6;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-7);
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-3);
}

// out until hit
void Torch::unlit()
{
	if (!state())
		return;
	state() = 0;
	substate() |= 1 << OBJSUBFLAG_UNK_4_F;
	play_sfx(SFX_073);
	set_update(LIT);
	set_interaction(OBJ_INTERACTION_FIRE);
	set_frameset(ADDR_Frameset_68360);
	relight();
	if (++mem_[wNumLitTorches] == NUM_FLAME_BLOCK_TORCHES)
		play_sfx(SFX_0FA);
}

void Torch::relight()
{
	state_duration() = 0; // 256 frames
	var1() = 0x02;
}

// for 512 frames (from the last hit), unless all are lit by then
void Torch::lit()
{
	if (mem_[wNumLitTorches] == NUM_FLAME_BLOCK_TORCHES)
		return;
	if (state()) {
		state() = 0;
		relight();
		play_sfx(SFX_073);
	}
	if (--state_duration())
		return;
	if (--var1())
		return;
	despawn();
	--mem_[wNumLitTorches];
}

// an ember every 128 frames, alternately both ways
void Torch::burning()
{
	uint8_t next;
	uint16_t params;
	switch (state_duration()) {
	case 0:
		if (mem_[wGlobalCounter] & 0x7f)
			return;
		next = 1;
		params = ADDR_ObjParams_TorchEmberLeft1;
		break;
	case 1:
		next = 2;
		params = ADDR_ObjParams_TorchEmberRight1;
		break;
	case 2:
		if (mem_[wGlobalCounter] & 0x7f)
			return;
		next = 3;
		params = ADDR_ObjParams_TorchEmberLeft2;
		break;
	default:
		next = 0;
		params = ADDR_ObjParams_TorchEmberRight2;
		break;
	}
	state_duration() = next;
	if (flags() & OBJFLAG_ON_SCREEN)
		play_sfx(SFX_073);
	SET_BC(gb_, params);
	routines_.call(ADDR_CreateObjectFromCurObjPos);
}

void Torch::ember_towards_wario()
{
	uint8_t wario = static_cast<uint8_t>(mem_[wWarioScreenXPos] + 0x2a);
	if (static_cast<uint8_t>(screen_x() + 0x2a) < wario)
		move_right();
	else
		routines_.call(ADDR_MoveObjectLeft_Slow);
	SET_BC(gb_, ADDR_Data_60320);
	routines_.call(ADDR_ApplyObjYMovement);
	ember();
}

// arcs down until it lands (or is hit), then fizzles out
void Torch::ember()
{
	SET_BC(gb_, ADDR_Data_60320);
	routines_.call(ADDR_ApplyObjYMovement);
	flags() |= OBJFLAG_NO_COLLISION;
	if (!state()) {
		mem_[hYPosLo] = mem_[wCurObjYPos + 0];
		mem_[hYPosHi] = mem_[wCurObjYPos + 1];
		mem_[hXPosLo] = mem_[wCurObjXPos + 0];
		mem_[hXPosHi] = mem_[wCurObjXPos + 1];
		routines_.call(ADDR_Func_352b);
		if (!gb_->cpu.a)
			return;
		mem_[wCurObjYPos + 0] = mem_[hYPosLo];
		mem_[wCurObjYPos + 1] = mem_[hYPosHi];
	}
	set_update(EMBER_OUT);
	set_frameset(ADDR_Frameset_68359);
	state_duration() = 24;
}

void Torch::ember_out()
{
	no_actions();
	if (!--state_duration())
		flags() = 0;
}

} // namespace wl3::level

using Torch = wl3::level::Torch;

PORT_EX(TorchFunc, 0, ALL_REGISTERS) { Torch(gb).torch(); }
PORT_LOCAL_EX(TorchFunc, Update, 0, ALL_REGISTERS) { Torch(gb).burning(); }
PORT_EX(TorchNoEmbersFunc, 0, ALL_REGISTERS) { Torch(gb).no_embers(); }
PORT_LOCAL_EX(TorchNoEmbersFunc, Update, 0, ALL_REGISTERS) {}
PORT_EX(FlameBlockTorchFunc, 0, ALL_REGISTERS) { Torch(gb).flame_block_torch(); }
PORT_EX(FlameBlockTorchUpdate, 0, ALL_REGISTERS) { Torch(gb).unlit(); }
PORT_LOCAL_EX(FlameBlockTorchUpdate, Update, 0, ALL_REGISTERS) { Torch(gb).lit(); }
PORT_EX(Func_40c19, 0, ALL_REGISTERS) { Torch(gb).ember_towards_wario(); }
PORT_EX(TorchEmberLeft1Func, 0, ALL_REGISTERS) { gb_call(gb, ADDR_MoveObjectLeft); Torch(gb).ember(); }
PORT_EX(TorchEmberRight1Func, 0, ALL_REGISTERS) { gb_call(gb, ADDR_MoveObjectRight); Torch(gb).ember(); }
PORT_EX(TorchEmberLeft2Func, 0, ALL_REGISTERS) { gb_call(gb, ADDR_MoveObjectLeft_Slow); Torch(gb).ember(); }
PORT_EX(TorchEmberRight2Func, 0, ALL_REGISTERS) { gb_call(gb, ADDR_MoveObjectRight_Slow); Torch(gb).ember(); }
PORT_EX(TorchEmberCommonFunc, 0, ALL_REGISTERS) { Torch(gb).ember(); }
PORT_LOCAL_EX(TorchEmberCommonFunc, Destroy, 0, ALL_REGISTERS) { Torch(gb).ember_out(); }
