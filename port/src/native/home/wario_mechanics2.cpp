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
	uint16_t x = mem.wario_x();
	if (left)
		x = static_cast<uint16_t>(x - static_cast<uint8_t>(-mem.collision_box_left() - 3));
	else
		x = static_cast<uint16_t>(x + static_cast<uint8_t>(mem.collision_box_right() - 3));
	mem.x_pos_lo() = static_cast<uint8_t>(x);
	mem[hXPosLo - 1] = static_cast<uint8_t>(x >> 8);
	mem[hXPosLo - 2] = mem.wario_y().lo();
	mem[hXPosLo - 3] = mem.wario_y().hi();
}

void block_below(GB *gb)
{
	Routines(gb).in_bank(BANK_Func_19b51, ADDR_Func_19b51, [&] { level::check_block_below(gb); });
}

} // namespace

bool is_on_slippery_ground(GB *gb)
{
	Memory mem(gb);
	mem.is_on_slippery_ground() = 0;
	pos_at_box_edge(mem, true);
	block_below(gb);
	if (mem.is_on_slippery_ground())
		return true;
	pos_at_box_edge(mem, false);
	block_below(gb);
	return mem.is_on_slippery_ground() != 0;
}

void release_owl(GB *gb)
{
	Memory mem(gb);
	// hPos = wWarioPos + (-$18, 0)
	mem.x_pos_lo() = mem.wario_x().lo();
	mem[hXPosLo - 1] = mem.wario_x().hi();
	uint16_t y = static_cast<uint16_t>(mem.wario_y() - 0x18);
	mem[hXPosLo - 2] = static_cast<uint8_t>(y);
	mem[hXPosLo - 3] = static_cast<uint8_t>(y >> 8);
	Routines(gb).in_bank(BANK_CreateParticle, ADDR_CreateParticle, [&] { level::create_particle(gb, PARTICLE_OWL); });
}

} // namespace wl3::home

PORT_EX(IsOnSlipperyGround, 0, ALL_REGISTERS) { wl3::home::is_on_slippery_ground(gb); }
PORT_EX(ReleaseOwl, 0, ALL_REGISTERS) { wl3::home::release_owl(gb); }
