// The map's common animations (bank76.asm): the clouds, the sea and the
// cliffs, as tiles copied into wTilemap for the V-Blank DMA to send
// (Func_1d8c2c, which V-Blank code calls, stays C). See overworld.hpp.
#include "overworld.hpp"

namespace wl3::overworld {

namespace {

constexpr uint16_t CLOUD_TILES = wTilemap + 0x24 * TILE_SIZE;
constexpr uint16_t SEA_TILES = wTilemap + 0x2c * TILE_SIZE;

// rlca
uint8_t rotate_left(uint8_t v)
{
	return static_cast<uint8_t>(v << 1 | v >> 7);
}

} // namespace

// An animation's period, counter and frame (from addr): whether the
// counter wrapped, to the next frame.
bool CommonAnimations::tick(uint16_t addr)
{
	uint8_t period = mem_[addr];
	if (!period)
		return false;
	Byte counter = mem_[static_cast<uint16_t>(addr + 1)];
	++counter;
	if (static_cast<uint8_t>(period - 1) >= counter)
		return false;
	counter = 0;
	return true;
}

// Two tiles of gfx + offset to the animated tiles at dest_low (in their
// page); bit of w2d031 says which changed.
void CommonAnimations::copy_tiles(uint16_t src, uint16_t dest, uint8_t dest_offset, int bit)
{
	uint16_t to = static_cast<uint16_t>((dest & 0xff00) | static_cast<uint8_t>(dest + dest_offset));
	routines_.copy(src, to, 2 * TILE_SIZE);
	wram_.unk_2d031().set_bit(bit);
}

// The big clouds, by cloud type: 4 frames, $80 bytes each.
void CommonAnimations::update_big_clouds(uint16_t addr, uint8_t offset)
{
	if (!tick(addr))
		return;
	Byte frame = mem_[static_cast<uint16_t>(addr + 2)];
	frame = (frame + 1) & 0x03;
	// d = cloud type * 2 + frame / 2, e = (frame % 2) * $80, in 8 bits;
	// sla d leaves the type's top bit in the carry that rra brings in
	uint8_t type = wram_.cloud_type();
	uint8_t high = static_cast<uint8_t>(((type & 0x80) | (frame >> 1)) + (type << 1));
	uint16_t gfx = static_cast<uint16_t>(ADDR_OverworldClouds1Gfx + (high << 8 | (frame & 0x01) << 7));
	copy_tiles(static_cast<uint16_t>(gfx + offset), CLOUD_TILES, offset, 0);
}

// The sea: 6 frames, the 4th and 5th held.
void CommonAnimations::update_sea(uint16_t addr)
{
	if (!tick(addr))
		return;
	Byte frame = mem_[static_cast<uint16_t>(addr + 2)];
	frame = frame == 5 ? 0 : static_cast<uint8_t>(frame + 1);
	uint8_t step = frame;
	if (step == 4)
		step = rotate_left(step);
	else if (step != 5)
		step = rotate_left(rotate_left(step));
	uint16_t gfx = static_cast<uint16_t>(ADDR_OverworldSeaGfx + (wram_.unk_2d049() << 8) + ((step & 0x0c) << 4));
	copy_tiles(gfx, SEA_TILES, 0x00, 1);
}

// The small clouds, with the sea's graphics: 4 frames.
void CommonAnimations::update_small_clouds(uint16_t addr)
{
	if (!tick(addr))
		return;
	Byte frame = mem_[static_cast<uint16_t>(addr + 2)];
	frame = (frame + 1) & 0x03;
	uint16_t gfx = static_cast<uint16_t>(ADDR_OverworldSeaGfx + (wram_.unk_2d049() << 8) + frame * 0x40 + 0x20);
	copy_tiles(gfx, SEA_TILES, 0x20, 1);
}

void CommonAnimations::update()
{
	wram_.unk_2d031() = 0;
	static constexpr uint16_t BIG_CLOUDS[4] = {w2d032, w2d035, w2d038, w2d03b};
	for (int i = 0; i < 4; i++)
		update_big_clouds(BIG_CLOUDS[i], static_cast<uint8_t>(i * 0x20));
	update_sea(w2d03e);
	update_small_clouds(w2d041);

	// the cliffs: a frame number for the V-Blank DMA (w2d044, 0 for none)
	mem_[w2d044 + 0] = 0;
	if (!tick(w2d046))
		return;
	Byte frame = wram_.unk_2d048();
	frame = frame == 3 ? 0 : static_cast<uint8_t>(frame + 1);
	mem_[w2d044 + 0] = static_cast<uint8_t>(frame + (ADDR_OverworldCliffs1Gfx >> 8));
	mem_[w2d044 + 1] = static_cast<uint8_t>(frame >> 2 | frame << 6);
}

// The clouds of the map's type and the sea, to VRAM and wTilemap.
void CommonAnimations::load_gfx()
{
	mem_[rVBK] = 1;
	uint16_t clouds = static_cast<uint16_t>(ADDR_OverworldClouds1Gfx + wram_.cloud_type() * 0x200);
	routines_.copy(clouds, v1Tiles2 + 0x70 * TILE_SIZE, 8 * TILE_SIZE);
	routines_.copy(ADDR_OverworldSeaGfx, v1Tiles2 + 0x78 * TILE_SIZE, 4 * TILE_SIZE);
	routines_.copy(clouds, CLOUD_TILES, 8 * TILE_SIZE);
	routines_.copy(ADDR_OverworldSeaGfx, SEA_TILES, 4 * TILE_SIZE);
	mem_[rVBK] = 0;
}

} // namespace wl3::overworld

// Both are only farcalled, and their callers use no register after them
// (farcall's hCallFunc restores a and f; the map's code reloads the rest).
PORT_EX(UpdateCommonOWAnimations, 0, ALL_REGISTERS)
{
	wl3::overworld::CommonAnimations(gb).update();
}

PORT_EX(_LoadCloudAndSeaGfx, 0, ALL_REGISTERS)
{
	wl3::overworld::CommonAnimations(gb).load_gfx();
}
