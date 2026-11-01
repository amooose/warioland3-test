// Home's sprite, animation and VRAM backup routines; see sprites.hpp.
#include "sprites.hpp"

#include "../flags.hpp"
#include "memory.hpp"

namespace wl3::home {

SpriteEnd load_sprite(GB *gb, uint16_t table)
{
	Memory mem(gb);
	uint16_t data = mem.word(static_cast<uint16_t>(table + static_cast<uint8_t>(mem[wCurSpriteFrame] * 2)));
	uint8_t y = mem[wCurSpriteYCoord], x = mem[wCurSpriteXCoord];
	uint8_t oam = mem[wVirtualOAMByteSize];
	for (;;) {
		if (oam >= OAM_SIZE)
			return {data, oam, true};
		if (mem[data] == 0x80)
			return {data, oam, false};
		uint16_t dest = static_cast<uint16_t>((wVirtualOAM & 0xff00) | oam);
		mem[dest++] = static_cast<uint8_t>(mem[data++] + y);
		mem[dest++] = static_cast<uint8_t>(mem[data++] + x);
		mem[dest++] = mem[data++];
		mem[dest++] = static_cast<uint8_t>(mem[data++] ^ mem[wCurSpriteAttributes]);
		oam = static_cast<uint8_t>(dest);
		mem[wVirtualOAMByteSize] = oam;
	}
}

bool draw_wario(GB *gb, SpriteEnd *end)
{
	Memory mem(gb);
	if (mem[wTransformation] == TRANSFORMATION_INVISIBLE_WARIO) {
		// seen while the counter's upper 4 bits are set and bit 2 is clear
		uint8_t frame = ++mem[wInvisibleFrame];
		if ((frame & 0xf2) != 0xf0)
			return false;
	} else if (mem[wInvisibleFrame]) { // invincible: blinking
		return false;
	}
	mem[wTempBank] = mem[wWarioOAMBank];
	uint8_t bank = mem[wROMBank];
	bankswitch(gb, mem[wTempBank], ADDR_DrawWario);
	uint16_t table = static_cast<uint16_t>(mem[wWarioOAMPtr + 0] << 8 | mem[wWarioOAMPtr + 1]);
	mem[wCurSpriteYCoord] = mem[wWarioScreenYPos];
	mem[wCurSpriteXCoord] = mem[wWarioScreenXPos];
	mem[wCurSpriteFrame] = mem[wWarioFrame];
	mem[wCurSpriteAttributes] = mem[wWarioAttributes];
	SpriteEnd sprite = load_sprite(gb, table);
	if (end)
		*end = sprite;
	bankswitch(gb, bank, ADDR_DrawWario);
	return true;
}

namespace {

// Where an animation step reads and writes: for a new frame, the offset
// and duration are written at offset2 and duration2 (the same bytes but
// where the object's walk crosses a page).
struct AnimBytes {
	uint16_t frameset, duration, offset, frame, offset2, duration2, ended;
};

AnimStep step(GB *gb, const AnimBytes &at)
{
	Memory mem(gb);
	mem[at.ended] = FALSE;
	uint8_t left = static_cast<uint8_t>(mem[at.duration] - 1);
	mem[at.duration] = left;
	if (left != 0xff)
		return {false, false, 0, left};
	uint16_t entry = static_cast<uint16_t>(at.frameset + mem[at.offset]);
	uint8_t next = mem[entry];
	if (next == 0xff) {
		mem[at.offset] = 0;
		mem[at.duration] = 0;
		mem[at.ended] = TRUE;
		return {true, true, entry, 0};
	}
	mem[at.frame] = next;
	uint8_t offset = mem[at.offset2];
	mem[at.offset2] = static_cast<uint8_t>(offset + 2);
	mem[at.duration2] = mem[++entry];
	return {true, false, entry, offset};
}

// The asm walks the object's bytes down with dec hl, then to the frame and
// back with dec l / inc l, which never carry into h.
AnimBytes obj_anim_bytes(GB *gb, uint16_t frameset_hi)
{
	Memory mem(gb);
	uint16_t offset = static_cast<uint16_t>(frameset_hi - 3);
	uint16_t frame = static_cast<uint16_t>((offset & 0xff00) | static_cast<uint8_t>(offset - 2));
	uint16_t offset2 = static_cast<uint16_t>(frame + 1);
	offset2 = static_cast<uint16_t>((offset2 & 0xff00) | static_cast<uint8_t>(offset2 + 1));
	uint16_t frameset = static_cast<uint16_t>(mem[static_cast<uint16_t>(frameset_hi - 1)] << 8 | mem[frameset_hi]);
	return {frameset, static_cast<uint16_t>(frameset_hi - 2), offset, frame, offset2,
		static_cast<uint16_t>(offset2 + 1), wObjAnimWasReset};
}

// His frameset pointer is big-endian in wWarioFramesetPtr.
AnimBytes wario_anim_bytes(GB *gb)
{
	Memory mem(gb);
	uint16_t frameset = static_cast<uint16_t>(mem[wWarioFramesetPtr + 0] << 8 | mem[wWarioFramesetPtr + 1]);
	return {frameset, wFrameDuration, wAnimationFrame, wWarioFrame, wAnimationFrame, wFrameDuration,
		wWarioAnimationEnded};
}

} // namespace

AnimStep update_obj_anim(GB *gb, uint16_t frameset)
{
	return step(gb, obj_anim_bytes(gb, frameset));
}

AnimStep update_wario_animation(GB *gb)
{
	return step(gb, wario_anim_bytes(gb));
}

uint8_t queue_block_update(GB *gb, uint16_t block)
{
	Memory mem(gb);
	uint8_t at = mem[wc19e];
	uint16_t entry = static_cast<uint16_t>(wc18e + at);
	mem[entry] = static_cast<uint8_t>(((mem[wBlockPtrBank] - 1) << 5) + (block >> 8));
	mem[static_cast<uint16_t>(entry + 1)] = static_cast<uint8_t>(block);
	mem[wc19e] = static_cast<uint8_t>((at + 2) & 0x0f);
	return at;
}

namespace {

// wCurSprite* from a sprite's 4 bytes, its position on screen made OAM's
void set_cur_sprite(Memory mem, uint16_t obj)
{
	mem[wCurSpriteYCoord] = static_cast<uint8_t>(mem[obj] + OAM_Y_OFS);
	mem[wCurSpriteXCoord] = static_cast<uint8_t>(mem[static_cast<uint16_t>(obj + 1)] + OAM_X_OFS);
	mem[wCurSpriteFrame] = mem[static_cast<uint16_t>(obj + 2)];
	mem[wCurSpriteAttributes] = mem[static_cast<uint16_t>(obj + 3)];
}

SpriteEnd load_sprite_in(GB *gb, uint8_t bank, uint16_t table, uint16_t routine)
{
	uint8_t saved = Memory(gb)[wROMBank];
	bankswitch(gb, bank, routine);
	SpriteEnd end = load_sprite(gb, table);
	bankswitch(gb, saved, routine);
	return end;
}

} // namespace

SpriteEnd load_game_over_sprite(GB *gb, uint16_t obj)
{
	set_cur_sprite(Memory(gb), obj);
	return load_sprite_in(gb, BANK_OAM_aab5c, ADDR_OAM_aab5c, ADDR_LoadGameOverSprite);
}

SpriteEnd load_credits_music_box_sprite(GB *gb, uint16_t obj)
{
	Memory mem(gb);
	set_cur_sprite(mem, obj);
	mem[wTempBank] = mem[wCreditsMusicBoxOAMBank];
	uint16_t table = static_cast<uint16_t>(mem[wCreditsMusicBoxOAMPtr + 0] << 8 | mem[wCreditsMusicBoxOAMPtr + 1]);
	return load_sprite_in(gb, mem[wTempBank], table, ADDR_LoadCreditsMusicBoxSprite);
}

bool update_golf_object_animation(GB *gb, uint16_t frameset, uint16_t duration, uint8_t *new_duration)
{
	Memory mem(gb);
	uint8_t left = static_cast<uint8_t>(mem[duration] - 1);
	mem[duration] = left;
	if (left != 0xff)
		return false;
	uint16_t offset = static_cast<uint16_t>(duration + 1), frame = static_cast<uint16_t>(duration + 2);
	uint16_t entry = static_cast<uint16_t>(frameset + mem[offset]);
	uint8_t next = mem[entry];
	if (next == 0xff) {
		mem[wGolfAnimLastFrame] = mem[frame];
		mem[frame] = mem[frameset];
		mem[offset] = 2;
		uint8_t next_duration = mem[static_cast<uint16_t>(frameset + 1)];
		mem[duration] = next_duration;
		if (new_duration)
			*new_duration = next_duration;
		return true;
	}
	mem[frame] = next;
	mem[offset] = static_cast<uint8_t>(mem[offset] + 2);
	uint8_t next_duration = mem[static_cast<uint16_t>(entry + 1)];
	mem[duration] = next_duration;
	if (new_duration)
		*new_duration = next_duration;
	return false;
}

SpriteEnd load_golf_sprite(GB *gb, uint16_t obj)
{
	Memory mem(gb);
	mem[wCurSpriteYCoord] = mem[obj];
	mem[wCurSpriteXCoord] = mem[static_cast<uint16_t>(obj + 1)];
	mem[wCurSpriteFrame] = mem[static_cast<uint16_t>(obj + 2)];
	mem[wCurSpriteAttributes] = mem[static_cast<uint16_t>(obj + 3)];
	return load_sprite(gb, static_cast<uint16_t>(mem[wGolfOAMPtr + 0] << 8 | mem[wGolfOAMPtr + 1]));
}

void load_wario_gfx(GB *gb)
{
	Memory mem(gb);
	mem[wPendingDMASourceBank] = mem[wDMASourceBank];
	mem[wPendingDMASourcePtr + 0] = mem[wDMASourcePtr + 0];
	mem[wPendingDMASourcePtr + 1] = mem[wDMASourcePtr + 1];
	mem[wPendingDMADestinationPtr + 0] = static_cast<uint8_t>(v0Tiles0 >> 8);
	mem[wPendingDMADestinationPtr + 1] = static_cast<uint8_t>(v0Tiles0);
	mem[wPendingDMADestinationBank] = BANK_v0Tiles0;
	mem[wPendingDMALength] = 127;
	mem[wIsDMATransferPending] = TRUE;
}

namespace {

// the scroll and palettes to WRAM 3 (or back), then VRAM's four $1000
// quarters to WRAM 4 to 7
void backup_vram(GB *gb, bool save)
{
	Memory mem(gb);
	uint8_t wram_bank = mem[rWBK];
	mem[rWBK] = BANK_w3d500;
	if (save) {
		mem[w3d500] = mem[wSCY];
		mem[w3d501] = mem[wSCX];
		copy(gb, wTempPals1, w3d280, 16 * PAL_SIZE);
	} else {
		mem[wSCY] = mem[w3d500];
		mem[wSCX] = mem[w3d501];
		copy(gb, w3d280, wTempPals1, 16 * PAL_SIZE);
	}
	mem[rWBK] = wram_bank;
	static constexpr struct {
		uint8_t vram, wram;
		uint16_t tiles, backup;
	} QUARTERS[4] = {
		{0, BANK_w4d000, v0Tiles0, w4d000},
		{0, BANK_w5d000, v0Tiles2, w5d000},
		{BANK_v1Tiles0, BANK_w6d000, v1Tiles0, w6d000},
		{BANK_v1Tiles0, BANK_w7d000, v1Tiles2, w7d000},
	};
	for (const auto &quarter : QUARTERS) {
		mem[rVBK] = quarter.vram;
		wram_bank = mem[rWBK];
		mem[rWBK] = quarter.wram;
		if (save)
			copy(gb, quarter.tiles, quarter.backup, 0x1000);
		else
			copy(gb, quarter.backup, quarter.tiles, 0x1000);
		mem[rWBK] = wram_bank;
	}
	mem[rVBK] = 0;
}

} // namespace

void set_sprite_regs(GB *gb, const SpriteEnd &end)
{
	Memory mem(gb);
	gb->cpu.b = mem[wCurSpriteYCoord];
	gb->cpu.c = mem[wCurSpriteXCoord];
	SET_DE(gb, end.data);
	SET_HL(gb, static_cast<uint16_t>((wVirtualOAM & 0xff00) | end.oam));
	gb->cpu.a = end.full ? end.oam : 0x80;
	gb->cpu.f = end.full ? flags::cp(end.oam, OAM_SIZE) : flags::cp(0x80, 0x80);
}

void save_backup_vram(GB *gb)
{
	backup_vram(gb, true);
}

void load_backup_vram(GB *gb)
{
	backup_vram(gb, false);
}

} // namespace wl3::home

using namespace wl3::home;
namespace flags = wl3::flags;

namespace {


// An animation step's registers: hl where it last wrote (the offset, the
// duration on a reset, the duration again on a new frame), bc the entry,
// a and f from its last arithmetic
void anim_regs(GB *gb, const AnimStep &s, uint16_t offset, uint16_t duration, uint16_t duration2)
{
	if (!s.new_frame) {
		gb->cpu.a = s.value;
		gb->cpu.f = flags::sub(static_cast<uint8_t>(s.value + 1), 1);
		SET_HL(gb, offset);
		return;
	}
	SET_BC(gb, s.entry);
	if (s.reset) {
		SET_HL(gb, duration);
		gb->cpu.a = TRUE;
		gb->cpu.f = FLAG_Z; // xor a
		return;
	}
	SET_HL(gb, duration2);
	gb->cpu.a = wl3::Memory(gb)[s.entry];
	gb->cpu.f = flags::add(s.value, 2);
}

} // namespace

PORT_EX(LoadSprite, 0, 0)
{
	set_sprite_regs(gb, load_sprite(gb, REG_HL(gb)));
}

// a and f are popped from the push before the bankswitch (the bank it came
// from, the flags of the visibility check)
PORT_EX(DrawWario, 0, 0)
{
	wl3::Memory mem(gb);
	bool invisible = mem[wTransformation] == TRANSFORMATION_INVISIBLE_WARIO;
	uint8_t bank = mem[wROMBank];
	SpriteEnd end;
	if (draw_wario(gb, &end)) {
		set_sprite_regs(gb, end);
		gb->cpu.a = bank;
		gb->cpu.f = invisible ? flags::cp(0xf0, 0xf0) : flags::and_(0);
		return;
	}
	if (invisible) {
		SET_HL(gb, wInvisibleFrame);
		gb->cpu.a = mem[wInvisibleFrame] & 0xf2;
		gb->cpu.f = flags::cp(gb->cpu.a, 0xf0);
	} else {
		gb->cpu.a = mem[wInvisibleFrame];
		gb->cpu.f = flags::and_(gb->cpu.a);
	}
}

// hl = the object's frameset pointer (its high byte)
PORT_EX(UpdateObjAnim, 0, 0)
{
	uint16_t hl = REG_HL(gb);
	wl3::Memory mem(gb);
	gb->cpu.e = mem[hl];
	gb->cpu.d = mem[static_cast<uint16_t>(hl - 1)];
	AnimBytes at = obj_anim_bytes(gb, hl);
	anim_regs(gb, update_obj_anim(gb, hl), at.offset, at.duration, at.duration2);
}

PORT_EX(UpdateWarioAnimation, 0, 0)
{
	wl3::Memory mem(gb);
	gb->cpu.d = mem[wWarioFramesetPtr + 0];
	gb->cpu.e = mem[wWarioFramesetPtr + 1];
	anim_regs(gb, update_wario_animation(gb), wAnimationFrame, wFrameDuration, wFrameDuration);
}

PORT_EX(Func_e31, 0, 0)
{
	uint16_t block = REG_HL(gb);
	wl3::Memory mem(gb);
	// the entry's first byte (not read back: the entry can overlap wc19e)
	gb->cpu.b = static_cast<uint8_t>(((mem[wBlockPtrBank] - 1) << 5) + (block >> 8));
	uint8_t at = queue_block_update(gb, block);
	uint16_t entry = static_cast<uint16_t>(wc18e + at);
	gb->cpu.c = static_cast<uint8_t>(block);
	gb->cpu.d = 0;
	gb->cpu.e = at;
	SET_HL(gb, static_cast<uint16_t>(entry + 1));
	gb->cpu.a = mem[wc19e];
	gb->cpu.f = flags::and_(gb->cpu.a);
}

// Func_e31 for an object on screen
PORT_EX(Func_e2b, 0, 0)
{
	uint8_t obj_flags = wl3::Memory(gb)[wCurObjFlags];
	if (!(obj_flags & (1 << OBJFLAG_ON_SCREEN_F))) {
		gb->cpu.a = obj_flags;
		gb->cpu.f = flags::bit(gb->cpu.f, obj_flags, OBJFLAG_ON_SCREEN_F);
		return;
	}
	port_Func_e31(gb);
}

PORT_EX(SaveBackupVRAM, 0, 0)
{
	save_backup_vram(gb);
	SET_HL(gb, v1Tiles2 + 0x1000);
	SET_DE(gb, w7d000 + 0x1000);
	SET_BC(gb, 0);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

PORT_EX(LoadBackupVRAM, 0, 0)
{
	load_backup_vram(gb);
	SET_HL(gb, w7d000 + 0x1000);
	SET_DE(gb, v1Tiles2 + 0x1000);
	SET_BC(gb, 0);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

// a is popped from the push before the bankswitch (the bank it came from),
// f from the last add before it (x + OAM_X_OFS, read before the sprite,
// which can overwrite it, is drawn)
static uint8_t x_add_flags(GB *gb, uint16_t obj)
{
	return flags::add(wl3::Memory(gb)[static_cast<uint16_t>(obj + 1)], OAM_X_OFS);
}

static void loaded_sprite_regs(GB *gb, uint8_t bank, uint8_t f, const SpriteEnd &end)
{
	set_sprite_regs(gb, end);
	gb->cpu.a = bank;
	gb->cpu.f = f;
}

PORT_EX(LoadGameOverSprite, 0, 0)
{
	uint16_t obj = REG_HL(gb);
	uint8_t bank = wl3::Memory(gb)[wROMBank];
	uint8_t f = x_add_flags(gb, obj);
	loaded_sprite_regs(gb, bank, f, load_game_over_sprite(gb, obj));
}

PORT_EX(LoadCreditsMusicBoxSprite, 0, 0)
{
	uint16_t obj = REG_HL(gb);
	uint8_t bank = wl3::Memory(gb)[wROMBank];
	uint8_t f = x_add_flags(gb, obj);
	loaded_sprite_regs(gb, bank, f, load_credits_music_box_sprite(gb, obj));
}

PORT_EX(LoadWarioGfx, 0, 0)
{
	load_wario_gfx(gb);
	gb->cpu.a = TRUE;
}

// hl = the duration, de = the frameset; c = TRUE when it started over
PORT_EX(UpdateGolfObjectAnimation, 0, 0)
{
	wl3::Memory mem(gb);
	uint16_t duration = REG_HL(gb), frameset = REG_DE(gb);
	uint8_t left = static_cast<uint8_t>(mem[duration] - 1);
	uint8_t offset = mem[static_cast<uint16_t>(duration + 1)];
	uint16_t entry = static_cast<uint16_t>(frameset + offset);
	uint8_t next_duration = 0;
	bool over = update_golf_object_animation(gb, frameset, duration, &next_duration);
	gb->cpu.c = 0;
	if (left != 0xff) {
		gb->cpu.a = left;
		gb->cpu.f = flags::sub(static_cast<uint8_t>(left + 1), 1);
		SET_HL(gb, static_cast<uint16_t>(duration + 1));
		return;
	}
	SET_HL(gb, duration);
	if (over) {
		gb->cpu.b = static_cast<uint8_t>(entry >> 8);
		gb->cpu.c = 1;
		SET_DE(gb, static_cast<uint16_t>(frameset + 1));
		gb->cpu.a = next_duration;
		gb->cpu.f = flags::cp(0xff, 0xff);
		return;
	}
	gb->cpu.b = static_cast<uint8_t>((entry + 1) >> 8);
	gb->cpu.a = next_duration;
	gb->cpu.f = flags::add(offset, 2);
}

// hl = the golf object's sprite
PORT_EX(LoadGolfSprite, 0, 0)
{
	set_sprite_regs(gb, load_golf_sprite(gb, REG_HL(gb)));
}
