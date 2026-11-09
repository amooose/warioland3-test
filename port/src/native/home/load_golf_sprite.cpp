// home/load_golf_sprite (src/home/load_golf_sprite.asm) as plain C++; see
// sprites.hpp. The callers are C++ (the golf V-Blank functions and
// animations), which read no register after these.
#include "../native.hpp"
#include "../routines.hpp"
#include "sprites.hpp"

namespace wl3::home {

void start_golf_vdma(GB *gb, uint16_t src)
{
	Memory mem(gb);
	mem[rVDMA_SRC_HIGH] = static_cast<uint8_t>(src >> 8);
	mem[rVDMA_SRC_LOW] = static_cast<uint8_t>(src);
	mem[rVDMA_DEST_HIGH] = mem[wdc11 + 0];
	mem[rVDMA_DEST_LOW] = mem[wdc11 + 1];
	mem[rVDMA_LEN] = mem[w1dc13];
}

void copy_golf_bg_pals(GB *gb, uint16_t src)
{
	Memory mem(gb);
	mem[rBGPI] = 0x80;
	for (int i = 0; i < 4 * 0x10; i++)
		mem[rBGPI + 1] = mem[src++];
}

namespace {

// HoldGolfWarioLastFrame: the states that stay on their last frame
void hold_golf_wario_last_frame(Memory mem)
{
	uint8_t state = mem[wGolfWarioState];
	if (state != GOLF_WARIO_CHARGING && state != GOLF_WARIO_ATTACKING_1 && state != GOLF_WARIO_MISSING &&
		state != GOLF_WARIO_TURNING)
		return;
	mem[wGolfCounter] = static_cast<uint8_t>(mem[wGolfCounter] + 1);
	mem[wGolfWarioCurrentFrame] = mem[wGolfAnimLastFrame];
}

} // namespace

void update_golf_wario_animation(GB *gb)
{
	static constexpr uint16_t OAM_BANKS = ADDR_UpdateGolfWarioAnimation + 0x75;
	static constexpr uint16_t OAM_TABLE = ADDR_UpdateGolfWarioAnimation + 0x81;
	static constexpr uint16_t FRAMESETS = ADDR_UpdateGolfWarioAnimation + 0xb1;
	Memory mem(gb);
	Routines routines(gb);
	uint8_t state = mem[wGolfWarioState];
	uint8_t oam_bank = mem[static_cast<uint16_t>(OAM_BANKS + state)];
	mem[wTempBank] = oam_bank;

	uint8_t entry = mem[wGolfWarioDir] ? static_cast<uint8_t>(state + NUM_GOLF_WARIO_STATES) : state;
	entry = static_cast<uint8_t>(entry << 1);
	uint16_t oam = static_cast<uint16_t>(OAM_TABLE + entry);
	mem[wGolfOAMPtr + 1] = mem[oam];
	mem[wGolfOAMPtr + 0] = mem[static_cast<uint16_t>(oam + 1)];
	uint16_t frameset = mem.word(static_cast<uint16_t>(FRAMESETS + entry));

	uint8_t bank = mem[wROMBank];
	routines.switch_bank(mem[wTempBank]);
	bool finished = update_golf_object_animation(gb, frameset, wGolfWarioDuration);
	mem[wHasGolfWarioAnimationFinished] = finished;
	routines.switch_bank(bank);

	if (mem[wHasGolfWarioAnimationFinished])
		hold_golf_wario_last_frame(mem);
	mem[wGolfWarioFrame] = mem[wGolfWarioCurrentFrame];

	bank = mem[wROMBank];
	routines.switch_bank(mem[wTempBank]);
	load_golf_sprite(gb, wGolfWarioSprite);
	routines.switch_bank(bank);
}

} // namespace wl3::home

using namespace wl3::home;

// hl = the source
PORT_EX(Func_2c30, 0, ALL_REGISTERS) { start_golf_vdma(gb, REG_HL(gb)); }
PORT_EX(Func_2c46, 0, ALL_REGISTERS) { copy_golf_bg_pals(gb, REG_HL(gb)); }
PORT_EX(UpdateGolfWarioAnimation, 0, ALL_REGISTERS) { update_golf_wario_animation(gb); }
PORT_COVERED(HoldGolfWarioLastFrame);
