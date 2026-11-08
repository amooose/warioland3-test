// home/wario_mechanics2 (src/home/wario_mechanics2.asm) as plain C++; see
// wario.hpp. The callers are C++ (the Wario states, the object
// interactions) and read no register after these.
#include "wario.hpp"

#include "../level/collisions.hpp"
#include "../level/object.hpp"
#include "../routines.hpp"
#include "memory.hpp"

namespace wl3::home {

namespace {

// hPos = Wario's position, x at the collision box's left or right edge, 3
// pixels in
void pos_at_box_edge(Memory mem, bool left)
{
	uint16_t x = static_cast<uint16_t>(mem[wWarioXPos] << 8 | mem[wWarioXPos + 1]);
	if (left)
		x = static_cast<uint16_t>(x - static_cast<uint8_t>(-mem[wCollisionBoxLeft] - 3));
	else
		x = static_cast<uint16_t>(x + static_cast<uint8_t>(mem[wCollisionBoxRight] - 3));
	mem[hXPosLo] = static_cast<uint8_t>(x);
	mem[hXPosLo - 1] = static_cast<uint8_t>(x >> 8);
	mem[hXPosLo - 2] = mem[wWarioYPos + 1];
	mem[hXPosLo - 3] = mem[wWarioYPos];
}

void block_below(GB *gb)
{
	Routines(gb).in_bank(BANK_Func_19b51, ADDR_Func_19b51, [&] { level::check_block_below(gb); });
}

} // namespace

bool is_on_slippery_ground(GB *gb)
{
	Memory mem(gb);
	mem[wIsOnSlipperyGround] = 0;
	pos_at_box_edge(mem, true);
	block_below(gb);
	if (mem[wIsOnSlipperyGround])
		return true;
	pos_at_box_edge(mem, false);
	block_below(gb);
	return mem[wIsOnSlipperyGround] != 0;
}

void release_owl(GB *gb)
{
	Memory mem(gb);
	// hPos = wWarioPos + (-$18, 0)
	mem[hXPosLo] = mem[wWarioXPos + 1];
	mem[hXPosLo - 1] = mem[wWarioXPos];
	uint16_t y = static_cast<uint16_t>((mem[wWarioYPos] << 8 | mem[wWarioYPos + 1]) - 0x18);
	mem[hXPosLo - 2] = static_cast<uint8_t>(y);
	mem[hXPosLo - 3] = static_cast<uint8_t>(y >> 8);
	Routines(gb).in_bank(BANK_CreateParticle, ADDR_CreateParticle, [&] { level::create_particle(gb, PARTICLE_OWL); });
}

} // namespace wl3::home

PORT_EX(IsOnSlipperyGround, 0, ALL_REGISTERS) { wl3::home::is_on_slippery_ground(gb); }
PORT_EX(ReleaseOwl, 0, ALL_REGISTERS) { wl3::home::release_owl(gb); }
