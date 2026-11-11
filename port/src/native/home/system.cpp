// Home's input and system routines; see system.hpp.
#include "system.hpp"

#include "../flags.hpp"
#include "memory.hpp"

#include <initializer_list>

namespace wl3::home {

namespace {

constexpr uint16_t HCALL_FUNC = ADDR_InitHRAMCallFunc + 0x0c; // .Func
constexpr uint8_t HCALL_FUNC_SIZE = 0x17; // .end - .Func
constexpr uint16_t VBLANK_RET_FUNC = ADDR_VBlank_Ret + 0x0c;
constexpr uint8_t VBLANK_RET_SIZE = 0x01;
constexpr uint16_t VBLANK_TA_FUNC = ADDR_VBlank_TimeAttackDescription + 0x0c;
constexpr uint8_t VBLANK_TA_SIZE = 0x10;
constexpr uint16_t BOSS_MUSIC_IDS = ADDR_UpdateLevelMusic + 0x70; // .BossMusicIDs

} // namespace

void update_joypad(GB *gb)
{
	Memory mem(gb);
	mem[rJOYP] = JOYP_GET_CTRL_PAD;
	uint8_t dpad = static_cast<uint8_t>(~mem[rJOYP] & 0x0f);
	mem[rJOYP] = JOYP_GET_BUTTONS;
	uint8_t buttons = static_cast<uint8_t>(~mem[rJOYP] & 0x0f);
	uint8_t down = static_cast<uint8_t>(dpad << 4 | buttons);
	mem.joypad_pressed() = static_cast<uint8_t>((mem.joypad_down() ^ down) & down);
	mem.joypad_down() = down;
	mem[rJOYP] = JOYP_GET_NONE;
}

uint8_t process_dpad_repeat(GB *gb)
{
	Memory mem(gb);
	uint8_t wram_bank = mem[rWBK];
	mem[rWBK] = BANK_wDPadRepeatCounter;
	auto counter = mem.d_pad_repeat_counter();
	uint8_t added = 0;
	uint8_t down = mem.joypad_down() & PAD_CTRL_PAD;
	if (!counter || (mem.joypad_pressed() & PAD_CTRL_PAD) || !down) {
		counter = 0x10; // a new press: the first repeat's delay
	} else if (!--counter) {
		mem.joypad_pressed() = static_cast<uint8_t>(mem.joypad_pressed() | down);
		added = down;
		counter = 0x08;
	}
	mem[rWBK] = wram_bank;
	return added;
}

bool enable_double_speed(GB *gb)
{
	Memory mem(gb);
	if (mem[rSPD] & 0x80)
		return false;
	mem[rSPD] = SPD_PREPARE;
	uint8_t ie = mem[rIE];
	mem[rIE] = 0;
	mem[rJOYP] = JOYP_GET_NONE;
	// stop: with the switch armed, it changes the CPU speed
	if (gb->cpu.speed_switch_armed) {
		gb->cpu.double_speed = !gb->cpu.double_speed;
		gb->cpu.speed_switch_armed = false;
	}
	gb->div_counter = 0;
	while (!(mem[rSPD] & 0x80)) {
	}
	mem[rJOYP] = 0;
	mem[rIF] = 0;
	mem[rIE] = ie;
	return true;
}

void init_hram_call_func(GB *gb)
{
	copy(gb, HCALL_FUNC, hCallFunc, HCALL_FUNC_SIZE);
}

void clear_save_data(GB *gb)
{
	for (uint16_t vals : {sCheckVals, sBackupCheckVals, s0a000, s0a800, s0a400, s0ac00})
		fill(gb, vals, 0, 8);
	Memory(gb).unused_fffd() = 0;
}

void wait_next_hblank(GB *gb)
{
	Memory mem(gb);
	while (!(mem[rSTAT] & STAT_MODE))
		idle(gb, 32);
	while (mem[rSTAT] & STAT_MODE)
		idle(gb, 32);
}

void init_lcd(GB *gb)
{
	Memory mem(gb);
	mem.lcd_func() = 0xd9; // reti
	mem[rIF] = 0;
	mem[rIE] = static_cast<uint8_t>(mem[rIE] & ~(1 << B_IE_STAT));
	mem[rSTAT] = static_cast<uint8_t>(mem[rSTAT] & ~(1 << B_STAT_LYC));
}

void vblank_ret(GB *gb)
{
	copy(gb, VBLANK_RET_FUNC, wVBlankFunc, VBLANK_RET_SIZE);
}

void vblank_time_attack_description(GB *gb)
{
	copy(gb, VBLANK_TA_FUNC, wVBlankFunc, VBLANK_TA_SIZE);
}

namespace {

// The music UpdateLevelMusic picks: its ID, little-endian at id, and how
// it got there (for the hook's registers).
struct LevelMusic {
	uint16_t id;
	bool banked;            // read with the ROM bank pushed and switched
	bool by_transformation; // pushed for the transformation's look-up
	uint8_t index;          // e: the table offset
	bool carry;             // d: its carry
};

LevelMusic level_music(GB *gb)
{
	Memory mem(gb);
	if (uint8_t boss = mem.boss_state()) {
		uint8_t index = static_cast<uint8_t>((boss - 1) * 2);
		return {static_cast<uint16_t>(BOSS_MUSIC_IDS + index), false, false, index, false};
	}
	bool transformed = mem.transformation() != 0;
	if (transformed) {
		uint8_t index = static_cast<uint8_t>((mem.transformation() & 0x1f) * 2);
		uint16_t id = static_cast<uint16_t>(ADDR_TransformationMusicIDs + index);
		uint8_t bank = mem.rom_bank();
		bankswitch(gb, BANK_TransformationMusicIDs, ADDR_UpdateLevelMusic);
		bool has_music = RomPtr(gb, BANK_TransformationMusicIDs, id)[0] != 0xff;
		bankswitch(gb, bank, ADDR_UpdateLevelMusic);
		if (has_music)
			return {id, true, true, index, false};
	}
	unsigned twice = mem.level() * 2u;
	return {static_cast<uint16_t>(ADDR_LevelMusic + twice), true, transformed, static_cast<uint8_t>(twice),
		twice > 0xff};
}

} // namespace

// LevelMusic is read in TransformationMusicIDs' bank after a
// transformation without music (the asm doesn't switch again).
void update_level_music(GB *gb)
{
	Memory mem(gb);
	LevelMusic music = level_music(gb);
	uint8_t bank = mem.rom_bank();
	if (music.banked)
		bankswitch(gb, music.by_transformation ? BANK_TransformationMusicIDs : BANK_LevelMusic,
			ADDR_UpdateLevelMusic);
	// the bank switched to (or, for a boss's, home's table, as mapped)
	Memory ids = Memory::in(gb, music.banked ? (music.by_transformation ? BANK_TransformationMusicIDs : BANK_LevelMusic) : -1);
	mem.music().lo() = ids[music.id];
	mem.music().hi() = ids[static_cast<uint16_t>(music.id + 1)];
	if (music.banked)
		bankswitch(gb, bank, ADDR_UpdateLevelMusic);
}

void init_game_state(GB *gb)
{
	Memory mem(gb);
	mem.state() = ST_TITLE;
	mem.sub_state() = 0;
}

void set_perfect_state(GB *gb)
{
	Memory mem(gb);
	mem.state() = ST_PERFECT;
	mem.sub_state() = 0;
}

namespace {

// Why SaveGameOrEnterPerfectScreen goes to the save (the check that said
// so, for its flags), or PERFECT
enum class SaveReason { PERFECT, TIME_ATTACK, TRANSITION, NOT_CLEARED, TREASURES };

SaveReason save_reason(Memory mem)
{
	if (mem.game_mode_flags() & (1 << MODE_TIME_ATTACK_F))
		return SaveReason::TIME_ATTACK;
	uint8_t transition = mem.transition_param();
	if (transition == TRANSITION_EPILOGUE_NOT_PERFECT || transition == TRANSITION_EPILOGUE_PERFECT
		|| transition == TRANSITION_GAME_OVER)
		return SaveReason::TRANSITION;
	if (!(mem.game_mode_flags() & MODE_GAME_CLEARED))
		return SaveReason::NOT_CLEARED;
	return mem.num_collected_treasures() == 1 ? SaveReason::PERFECT : SaveReason::TREASURES;
}

} // namespace

void save_game_or_enter_perfect_screen(GB *gb)
{
	Memory mem(gb);
	if (save_reason(mem) == SaveReason::PERFECT) {
		set_perfect_state(gb);
		return;
	}
	mem.pending_sub_state() = mem.sub_state();
	mem.state() = ST_PAUSE_MENU;
	mem.sub_state() = SST_PAUSE_18;
}

namespace {

// Where CheckHasAllMusicalCoinFlags stops: the first flag clear, in byte
// `byte` at bit `bit` (byte 3, bit 0, the last), or none
struct CoinFlagCheck {
	bool all;
	int byte, bit;
};

CoinFlagCheck check_musical_coin_flags(GB *gb)
{
	Memory mem(gb);
	for (int byte = 0; byte < 3; byte++)
		for (int bit = 0; bit < 8; bit++)
			if (!(mem[static_cast<uint16_t>(wMusicalCoinFlags + byte)] >> bit & 1))
				return {false, byte, bit};
	return {(mem[wMusicalCoinFlags + 3] & 1) != 0, 3, 0};
}

} // namespace

bool has_all_musical_coin_flags(GB *gb)
{
	return check_musical_coin_flags(gb).all;
}

} // namespace wl3::home

using namespace wl3::home;
namespace flags = wl3::flags;

PORT_EX(UpdateJoypad, 0, 0)
{
	update_joypad(gb);
	wl3::Memory mem(gb);
	gb->cpu.c = mem.joypad_down();
	gb->cpu.b = gb->cpu.c & 0xf0;
	gb->cpu.a = JOYP_GET_NONE;
	gb->cpu.f = static_cast<uint8_t>(FLAG_H | (mem.joypad_pressed() ? 0 : FLAG_Z)); // and c
}

// a and f are popped by pop_wram (the WRAM bank it read, the entry's flags)
PORT_EX(ProcessDPadRepeat, 0, 0)
{
	uint8_t wram_bank = wl3::Memory(gb)[rWBK];
	if (uint8_t added = process_dpad_repeat(gb))
		gb->cpu.b = added;
	SET_HL(gb, wDPadRepeatCounter);
	gb->cpu.a = wram_bank;
}

PORT_EX(EnableDoubleSpeed, 0, 0)
{
	uint8_t carry = gb->cpu.f & FLAG_C;
	uint8_t spd = wl3::Memory(gb)[rSPD];
	uint8_t ie = wl3::Memory(gb)[rIE];
	if (!enable_double_speed(gb)) {
		gb->cpu.a = spd;
		gb->cpu.f = static_cast<uint8_t>(carry | FLAG_H); // bit 7, set
		return;
	}
	gb->cpu.a = ie; // pop af: with the flags of bit 7 (clear)
	gb->cpu.f = static_cast<uint8_t>(carry | FLAG_Z | FLAG_H);
}

PORT_EX(InitHRAMCallFunc, 0, 0)
{
	init_hram_call_func(gb);
	SET_HL(gb, HCALL_FUNC + HCALL_FUNC_SIZE);
	SET_DE(gb, hCallFunc + HCALL_FUNC_SIZE);
	gb->cpu.a = wl3::Memory(gb)[hCallFunc + HCALL_FUNC_SIZE - 1];
	gb->cpu.b = 0;
	gb->cpu.f = static_cast<uint8_t>((gb->cpu.f & FLAG_C) | FLAG_Z | FLAG_N);
}

// then on to Init
PORT_EX(ClearSaveData, HOOK_MANUAL_RET, 0)
{
	clear_save_data(gb);
	gb->cpu.a = 0;
	SET_HL(gb, s0ac00 + 8);
	gb->cpu.b = 0;
	gb->cpu.f = FLAG_Z | FLAG_N; // the last fill's dec b, after xor a
	gb->cpu.pc = ADDR_Init;
}

// returns nz if every level's Musical Coin flag is set; b and c are the
// loops' counts where it stopped
PORT_EX(CheckHasAllMusicalCoinFlags, 0, 0)
{
	CoinFlagCheck check = check_musical_coin_flags(gb);
	if (check.byte < 3) {
		gb->cpu.b = static_cast<uint8_t>(8 - check.bit);
		gb->cpu.c = static_cast<uint8_t>(3 - check.byte);
		SET_HL(gb, static_cast<uint16_t>(wMusicalCoinFlags + check.byte + 1));
	} else {
		gb->cpu.b = 0;
		gb->cpu.c = 0;
		SET_HL(gb, wMusicalCoinFlags + 3);
	}
	gb->cpu.a = check.all ? TRUE : FALSE;
	gb->cpu.f = check.all ? FLAG_C : FLAG_Z; // the last rrca's carry, or xor a
}

PORT_EX(WaitNextHBlank, 0, 0)
{
	wait_next_hblank(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z | FLAG_H; // and STAT_MODE
}

PORT_EX(InitLCD, 0, 0)
{
	init_lcd(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, rSTAT);
}

// the CopyHLToDE's registers
static void vblank_func_regs(GB *gb, uint16_t func, uint8_t size)
{
	SET_HL(gb, static_cast<uint16_t>(func + size));
	SET_DE(gb, static_cast<uint16_t>(wVBlankFunc + size));
	gb->cpu.a = wl3::Memory(gb)[static_cast<uint16_t>(func + size - 1)];
	gb->cpu.b = 0;
	gb->cpu.f = static_cast<uint8_t>((gb->cpu.f & FLAG_C) | FLAG_Z | FLAG_N);
}

PORT_EX(VBlank_Ret, 0, 0)
{
	vblank_ret(gb);
	vblank_func_regs(gb, VBLANK_RET_FUNC, VBLANK_RET_SIZE);
}

PORT_EX(VBlank_TimeAttackDescription, 0, 0)
{
	vblank_time_attack_description(gb);
	vblank_func_regs(gb, VBLANK_TA_FUNC, VBLANK_TA_SIZE);
}

// hl is left on the ID's high byte, de the table offset. After a bank
// push, a and f are popped: the bank it came from, and the flags of the
// transformation's index (z) or of the and a that found none (z, h).
PORT_EX(UpdateLevelMusic, 0, 0)
{
	wl3::Memory mem(gb);
	uint8_t bank = mem.rom_bank();
	LevelMusic music = level_music(gb);
	update_level_music(gb);
	SET_HL(gb, static_cast<uint16_t>(music.id + 1));
	gb->cpu.d = music.carry;
	gb->cpu.e = music.index;
	if (!music.banked) {
		gb->cpu.a = mem[static_cast<uint16_t>(music.id + 1)];
		// add hl, de: z kept from add a, h and c from the 16-bit add
		uint8_t f = static_cast<uint8_t>(music.index ? 0 : FLAG_Z);
		if ((BOSS_MUSIC_IDS & 0x0fff) + music.index > 0x0fff)
			f |= FLAG_H;
		if (BOSS_MUSIC_IDS + music.index > 0xffff)
			f |= FLAG_C;
		gb->cpu.f = f;
		return;
	}
	gb->cpu.a = bank;
	gb->cpu.f = music.by_transformation ? FLAG_Z : FLAG_Z | FLAG_H;
}

PORT_EX(InitGameState, 0, 0)
{
	init_game_state(gb);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

PORT_EX(SetPerfectState, 0, 0)
{
	set_perfect_state(gb);
	SET_HL(gb, wState);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
}

// a and f from the last check (or SetPerfectState's)
PORT_EX(SaveGameOrEnterPerfectScreen, 0, 0)
{
	wl3::Memory mem(gb);
	SaveReason reason = save_reason(mem);
	uint8_t carry = gb->cpu.f & FLAG_C;
	uint8_t transition = mem.transition_param();
	uint8_t treasures = mem.num_collected_treasures();
	save_game_or_enter_perfect_screen(gb);
	if (reason == SaveReason::PERFECT) {
		SET_HL(gb, wState);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		return;
	}
	gb->cpu.a = SST_PAUSE_18;
	switch (reason) {
	case SaveReason::TIME_ATTACK:
		gb->cpu.f = static_cast<uint8_t>(carry | FLAG_H); // bit, set
		break;
	case SaveReason::TRANSITION: // the cp that matched
		gb->cpu.f = flags::cp(transition, transition);
		break;
	case SaveReason::NOT_CLEARED:
		gb->cpu.f = FLAG_Z | FLAG_H; // and
		break;
	default: // dec, not to 0 (the and before it cleared carry)
		gb->cpu.f = flags::dec(0, treasures);
		break;
	}
}
