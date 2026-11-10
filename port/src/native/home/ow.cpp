// Home's overworld helpers; see ow.hpp.
#include "ow.hpp"

#include "../flags.hpp"
#include "memory.hpp"
#include "pals.hpp"

namespace wl3::home {

namespace {

// ld a, [wROMBank] / push af / ld a, b / bankswitch ... pop af / bankswitch
template <class F>
auto in_bank(GB *gb, uint8_t bank, uint16_t routine, F fn)
{
	uint8_t saved = Memory(gb).rom_bank();
	bankswitch(gb, bank, routine);
	auto result = fn();
	bankswitch(gb, saved, routine);
	return result;
}

// wCurSprite*'s position: a sprite's y and x plus OAM's offsets, less the
// scroll for a position on the BG map
void set_cur_sprite(Memory mem, uint16_t obj, uint8_t scy, uint8_t scx)
{
	mem.cur_sprite_y_coord() = static_cast<uint8_t>(mem[obj] - scy + 0x10);
	mem.cur_sprite_x_coord() = static_cast<uint8_t>(mem[static_cast<uint16_t>(obj + 1)] - scx + 0x08);
	mem.cur_sprite_frame() = mem[static_cast<uint16_t>(obj + 2)];
	mem.cur_sprite_attributes() = mem[static_cast<uint16_t>(obj + 3)];
}

// where UpdateOWAnimation writes a new frame: two bytes before the
// duration, stepping back with dec hl, then dec l (no carry into h)
uint16_t ow_frame_addr(uint16_t duration)
{
	uint16_t hl = static_cast<uint16_t>(duration - 1);
	return static_cast<uint16_t>((hl & 0xff00) | static_cast<uint8_t>(hl - 1));
}

} // namespace

int update_ow_animation(GB *gb, uint8_t bank, uint16_t duration, uint16_t frameset)
{
	Memory mem(gb);
	mem.ow_animation_finished() = 0;
	return in_bank(gb, bank, ADDR_UpdateOWAnimation, [&] {
		uint8_t left = mem[duration];
		mem[duration] = static_cast<uint8_t>(left - 1);
		if (left)
			return -1;
		uint16_t offset_addr = static_cast<uint16_t>(duration + 1);
		uint8_t offset = mem[offset_addr];
		uint16_t entry = static_cast<uint16_t>(frameset + offset);
		auto rom = [&](uint16_t addr) { return RomPtr(gb, bank, addr)[0]; }; // the frameset, in bank
		uint8_t frame = rom(entry);
		if (frame == 0xff) { // the end: start over
			mem.ow_animation_finished() = frame;
			entry = frameset;
			frame = rom(frameset);
			offset = 0;
		}
		mem[offset_addr] = static_cast<uint8_t>(offset + 2);
		mem[duration] = static_cast<uint8_t>(rom(static_cast<uint16_t>(entry + 1)) - 1);
		mem[ow_frame_addr(duration)] = frame;
		return static_cast<int>(frame);
	});
}

void copy_far_bytes(GB *gb, uint8_t bank, uint16_t src, uint16_t dest, uint8_t count)
{
	in_bank(gb, bank, ADDR_CopyFarBytes, [&] { return copy(gb, RomPtr(gb, bank, src), dest, count ? count : 0x100); });
}

void copy_far_bytes_long(GB *gb, uint8_t bank, uint16_t src, uint16_t dest, uint8_t pages)
{
	in_bank(gb, bank, ADDR_CopyFarBytes_Long, [&] { return copy(gb, RomPtr(gb, bank, src), dest, pages ? pages * 0x100u : 0x10000u); });
}

void load_far_tiles(GB *gb, uint8_t bank, uint16_t src)
{
	in_bank(gb, bank, ADDR_LoadFarTiles, [&] { return copy(gb, RomPtr(gb, bank, src), v0Tiles2, 0x80 * TILE_SIZE); });
}

void load_far_pals_to_temp_pals1(GB *gb, uint8_t bank, uint16_t pals)
{
	in_bank(gb, bank, ADDR_LoadFarPalsToTempPals1, [&] { return copy_short(gb, RomPtr(gb, bank, pals), wTempPals1, 8 * PAL_SIZE); });
}

void load_far_pals_to_temp_pals2(GB *gb, uint8_t bank, uint16_t pals)
{
	in_bank(gb, bank, ADDR_LoadFarPalsToTempPals2, [&] { return copy_short(gb, RomPtr(gb, bank, pals), wTempPals2, 8 * PAL_SIZE); });
}

void set_scene_obj_state(GB *gb, uint16_t state_addr, uint8_t state)
{
	Memory mem(gb);
	mem[state_addr] = state;
	mem[static_cast<uint16_t>(state_addr - 1)] = 0;
	mem[static_cast<uint16_t>(state_addr - 2)] = 0;
}

SpriteEnd load_ow_sprite(GB *gb, uint8_t bank, uint16_t table, uint16_t obj)
{
	set_cur_sprite(Memory(gb), obj, 0, 0);
	return in_bank(gb, bank, ADDR_LoadOWSprite, [&] { return load_sprite(gb, RomPtr(gb, bank, table)); });
}

SpriteEnd load_ow_sprite_with_scroll(GB *gb, uint8_t bank, uint16_t table, uint16_t obj)
{
	Memory mem(gb);
	set_cur_sprite(mem, obj, mem.scy(), mem.scx());
	return load_ow_sprite_got_params(gb, bank, table);
}

SpriteEnd load_ow_sprite_got_params(GB *gb, uint8_t bank, uint16_t table)
{
	return in_bank(gb, bank, ADDR_LoadOWSpriteWithScroll_GotParams, [&] { return load_sprite(gb, RomPtr(gb, bank, table)); });
}

// a byte per 8 treasures, bit by the treasure's low 3 bits
bool is_treasure_collected(GB *gb, uint8_t treasure)
{
	return Memory(gb)[static_cast<uint16_t>(wTreasuresCollected + treasure / 8)] >> (treasure & 7) & 1;
}

void play_overworld_music(GB *gb)
{
	Memory mem(gb);
	uint16_t music = mem.unk_2d011() ? MUSIC_OVERWORLD_NIGHT : MUSIC_OVERWORLD_DAY;
	mem[hMusicIDBackup + 0] = mem.music().hi() = static_cast<uint8_t>(music >> 8);
	mem[hMusicIDBackup + 1] = mem.music().lo() = static_cast<uint8_t>(music);
}

uint16_t ow_anim_object(uint16_t duration, int frame)
{
	uint16_t hl = frame < 0 ? static_cast<uint16_t>(duration + 1) : ow_frame_addr(duration);
	return static_cast<uint16_t>(hl & 0xfff8);
}

} // namespace wl3::home

using namespace wl3::home;
namespace flags = wl3::flags;

// b = bank, hl = the duration, de = the frameset. Leaves hl at the object's
// start (the duration's 8-byte block) and bc at its state (+6); a and f
// are popped (the bank it came from; z, from the xor a before the push).
PORT_EX(UpdateOWAnimation, 0, 0)
{
	uint16_t duration = REG_HL(gb);
	uint8_t bank = wl3::Memory(gb).rom_bank();
	int frame = update_ow_animation(gb, gb->cpu.b, duration, REG_DE(gb));
	uint16_t hl = frame < 0 ? static_cast<uint16_t>(duration + 1) : ow_frame_addr(duration);
	if (frame >= 0)
		gb->cpu.d = static_cast<uint8_t>(frame);
	gb->cpu.h = static_cast<uint8_t>(hl >> 8);
	gb->cpu.l = hl & 0xf8;
	gb->cpu.b = gb->cpu.h;
	gb->cpu.c = static_cast<uint8_t>(gb->cpu.l + 6);
	gb->cpu.a = bank;
	gb->cpu.f = FLAG_Z;
}

// The far copies: hl and de past what was copied, b (or bc) 0; a and f are
// popped (the bank it came from, the entry's flags).
static void far_copied(GB *gb, uint8_t bank, uint16_t src, uint16_t dest, uint32_t n)
{
	SET_HL(gb, static_cast<uint16_t>(src + n));
	SET_DE(gb, static_cast<uint16_t>(dest + n));
	gb->cpu.b = 0;
	gb->cpu.a = bank;
}

// b = bank, hl = source, de = destination, c = count
PORT_EX(CopyFarBytes, 0, 0)
{
	uint8_t bank = wl3::Memory(gb).rom_bank();
	uint16_t src = REG_HL(gb), dest = REG_DE(gb);
	copy_far_bytes(gb, gb->cpu.b, src, dest, gb->cpu.c);
	far_copied(gb, bank, src, dest, gb->cpu.c ? gb->cpu.c : 0x100);
}

// c = pages
PORT_EX(CopyFarBytes_Long, 0, 0)
{
	uint8_t bank = wl3::Memory(gb).rom_bank();
	uint16_t src = REG_HL(gb), dest = REG_DE(gb);
	uint8_t pages = gb->cpu.c;
	copy_far_bytes_long(gb, gb->cpu.b, src, dest, pages);
	far_copied(gb, bank, src, dest, pages ? pages * 0x100u : 0x10000u);
	gb->cpu.c = 0;
}

PORT_EX(LoadFarTiles, 0, 0)
{
	uint8_t bank = wl3::Memory(gb).rom_bank();
	uint16_t src = REG_HL(gb);
	load_far_tiles(gb, gb->cpu.b, src);
	far_copied(gb, bank, src, v0Tiles2, 0x80 * TILE_SIZE);
	gb->cpu.c = 0;
}

// after CopyHLToDE_Short to a temp palette buffer
static void far_pals_regs(GB *gb, uint8_t bank, uint16_t pals, uint16_t dest)
{
	SET_HL(gb, static_cast<uint16_t>(pals + 8 * PAL_SIZE));
	gb->cpu.d = dest >> 8;
	gb->cpu.e = static_cast<uint8_t>(dest + 8 * PAL_SIZE);
	gb->cpu.b = 0;
	gb->cpu.a = bank;
}

PORT_EX(LoadFarPalsToTempPals1, 0, 0)
{
	uint8_t bank = wl3::Memory(gb).rom_bank();
	uint16_t pals = REG_HL(gb);
	load_far_pals_to_temp_pals1(gb, gb->cpu.b, pals);
	far_pals_regs(gb, bank, pals, wTempPals1);
}

PORT_EX(LoadFarPalsToTempPals2, 0, 0)
{
	uint8_t bank = wl3::Memory(gb).rom_bank();
	uint16_t pals = REG_HL(gb);
	load_far_pals_to_temp_pals2(gb, gb->cpu.b, pals);
	far_pals_regs(gb, bank, pals, wTempPals2);
}

// hl = the state, a = the new one
PORT_EX(SetSceneObjState, 0, 0)
{
	uint16_t state_addr = REG_HL(gb);
	set_scene_obj_state(gb, state_addr, gb->cpu.a);
	SET_HL(gb, static_cast<uint16_t>(state_addr - 2));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

// The sprite loaders leave LoadSprite's registers, then pop a (the bank
// they came from) and f (pushed after their last add, or the entry's).
static void ow_sprite_regs(GB *gb, uint8_t bank, uint8_t f, const SpriteEnd &end)
{
	set_sprite_regs(gb, end);
	gb->cpu.a = bank;
	gb->cpu.f = f;
}

// hl = the sprite's 4 bytes, b = the OAM data's bank, de = the OAM data
PORT_EX(LoadOWSprite, 0, 0)
{
	wl3::Memory mem(gb);
	uint16_t obj = REG_HL(gb);
	uint8_t bank = mem.rom_bank();
	uint8_t f = flags::add(mem[static_cast<uint16_t>(obj + 1)], 0x08);
	ow_sprite_regs(gb, bank, f, load_ow_sprite(gb, gb->cpu.b, REG_DE(gb), obj));
}

PORT_EX(LoadOWSpriteWithScroll, 0, 0)
{
	wl3::Memory mem(gb);
	uint16_t obj = REG_HL(gb);
	uint8_t bank = mem.rom_bank();
	uint8_t f = flags::add(static_cast<uint8_t>(mem[static_cast<uint16_t>(obj + 1)] - mem.scx()), 0x08);
	ow_sprite_regs(gb, bank, f, load_ow_sprite_with_scroll(gb, gb->cpu.b, REG_DE(gb), obj));
}

// b = bank, hl = the OAM data
PORT_EX(LoadOWSpriteWithScroll_GotParams, 0, 0)
{
	uint8_t bank = wl3::Memory(gb).rom_bank();
	ow_sprite_regs(gb, bank, gb->cpu.f, load_ow_sprite_got_params(gb, gb->cpu.b, REG_HL(gb)));
}

PORT_EX(PlayOverworldMusic, 0, 0)
{
	uint8_t night = wl3::Memory(gb).unk_2d011();
	play_overworld_music(gb);
	gb->cpu.a = static_cast<uint8_t>(night ? MUSIC_OVERWORLD_NIGHT : MUSIC_OVERWORLD_DAY);
	gb->cpu.f = flags::and_(night);
}
