// home/start (src/home/start.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"
#include "memory.hpp"
#include "system.hpp"
#include "../engine/main_state_table.hpp"
#include "../audio/audio.hpp"

namespace wl3::home {

namespace {

class HomeStart : public AsmModule {
public:
	using AsmModule::AsmModule;

	// unreferenced
	void port_UnreferencedMuteSound()
	{
		cpu_.a = alu_.and_(rd8(wSoundMuted), rd8(wSoundMuted));
		if (cpu_.a)
			return;
		cpu_.f = FLAG_Z; // xor a
		wr8(hSFXID + 0, 0);
		wr8(hSFXID + 1, 0);
		CALL(TurnMusicOff);
		SET_BC(gb_, SOUND_OFF);
		CALL(TurnOffSFXID);
		CALL(UpdateAudio);
		cpu_.a = TRUE;
		wr8(wSoundMuted, TRUE);
	}

	// unreferenced
	// ends with bc and de as PlayNewMusic_SetNoise leaves them (unreferenced)
	void port_UnreferencedUnmuteSound()
	{
		cpu_.a = alu_.and_(rd8(wSoundMuted), rd8(wSoundMuted));
		if (!cpu_.a)
			return;
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wSoundMuted, 0);
		wr8(hSFXID + 0, 0);
		wr8(hSFXID + 1, 0);
		cpu_.b = rd8(hMusicIDBackup + 0);
		cpu_.c = cpu_.a = rd8(hMusicIDBackup + 1);
		CALL(PlayNewMusic_SetNoise);
	}

	// handle playing SFX and music: the SFX and music IDs requested this
	// frame ($ff: turn it off), then the engine's update. The game loop reads
	// no register after.
	void port_HandleSound()
	{
		Memory mem(gb_);
		if (mem[wSoundMuted])
			return;
		auto take = [&](uint16_t at) {
			uint16_t id = static_cast<uint16_t>(mem[at] << 8 | mem[at + 1]);
			if (id) {
				mem[at + 1] = 0;
				mem[at + 0] = 0;
			}
			return id;
		};
		uint16_t sfx = take(hSFXID);
		if (sfx >> 8 == 0xff)
			audio::play_sfx(gb_, SOUND_OFF);
		else if (sfx)
			audio::play_sfx(gb_, sfx);
		uint16_t music = take(hMusicID);
		if (music >> 8 == 0xff)
			audio::play_new_music(gb_, SOUND_OFF);
		else if (music)
			audio::play_new_music_set_noise(gb_, music);
		audio::update(gb_);
	}

	// The main game loop (InitWithoutDemoPowerUpReset.GameLoop), one game
	// frame per pass, in parts around its halt: .GameLoop halts the CPU as the
	// halt does (the V-Blank interrupt wakes it as on the hardware), and the
	// loop goes on at .after_halt.
	#define GAME_LOOP (ADDR_InitWithoutDemoPowerUpReset + 0xcc)        // .GameLoop
	#define GAME_LOOP_HALT (ADDR_InitWithoutDemoPowerUpReset + 0x113)   // halt
	#define WAIT_VBLANK_EXEC (ADDR_InitWithoutDemoPowerUpReset + 0x115) // .wait_vblank_exec

	void port_InitWithoutDemoPowerUpReset__GameLoop()
	{
		Memory mem(gb_);
		update_joypad(gb_);
		// restart the game if all four buttons are down
		if (!mem[wResetDisabled]
			&& (mem[wJoypadDown] & PAD_BUTTONS) == (PAD_A | PAD_B | PAD_SELECT | PAD_START)) {
			audio::turn_music_off(gb_);
			audio::turn_off_sfx(gb_, SOUND_OFF);
			audio::update(gb_);
			JUMP_TO(ADDR_Init);
			return;
		}
		// .no_reset: where all main game logic is handled
		engine::run_main_state(gb_);

		// The game logic took most of the frame on the hardware, but native code
		// takes no time: without this, LY would still be in V-Blank here, and
		// UpdateRoomAnimatedTiles / UpdateRoomAnimatedPals, which only animate
		// when LY is well before V-Blank, would never animate. Let V-Blank end
		// as it would have. (Not under test, where time is frozen.)
		if (!gb_->test.active)
			while (mem[rLY] >= LY_VBLANK)
				idle(32);

		if (mem[wRoomAnimatedTilesEnabled])
			routines_.farcall(BANK_UpdateRoomAnimatedTiles, ADDR_UpdateRoomAnimatedTiles);
		if (mem[wRoomAnimatedPalsEnabled])
			routines_.call(ADDR_UpdateRoomAnimatedPals);

		// enable the V-Blank handler and wait for it
		mem[wEnableVBlankFunc] = TRUE;
		cpu_.halted = true; // halt
		JUMP_TO(GAME_LOOP_HALT + 1);
	}

	void port_InitWithoutDemoPowerUpReset__after_halt() { JUMP_TO(WAIT_VBLANK_EXEC); } // nop

	void port_InitWithoutDemoPowerUpReset__wait_vblank_exec()
	{
		Memory mem(gb_);
		if (!mem[wVBlankFuncExecuted]) {
			// not yet: back here, with interrupts serviced in between
			JUMP_TO(WAIT_VBLANK_EXEC);
			return;
		}
		++mem[wGlobalCounter];
		mem[wEnableVBlankFunc] = FALSE;
		mem[wVBlankFuncExecuted] = FALSE;
		port_HandleSound();
		JUMP_TO(GAME_LOOP); // loop back to game loop
	}

	void port_DebugReset() { JUMP(Init); }

	// The boot and reset code, falling into the main game loop.
	#define INIT_WO (ADDR_InitWithoutDemoPowerUpReset) // $0161

	// a: what the boot ROM left (BOOTUP_A_CGB on a CGB)
	void port_Start()
	{
		Memory mem(gb_);
		mem[hCGB] = cpu_.a == BOOTUP_A_CGB ? TRUE : FALSE;
		mem[hUnused_fffd] = 0;
		JUMP_TO(ADDR_Init); // fallthrough
	}

	void port_Init()
	{
		Memory(gb_).byte(hDemoPowerUp) = 0; // reset power up to show in Demo
		JUMP_TO(INIT_WO); // fallthrough
	}

	void port_InitWithoutDemoPowerUpReset()
	{
		Memory mem(gb_);
		// init the stack, clearing its $100 bytes
		cpu_.ime = false; // di
		cpu_.ime_delay = 0;
		cpu_.sp = wStackTop - 1;
		for (int i = 0; i < 0x100; i++)
			mem[static_cast<uint16_t>(wStackTop - 1 - i)] = 0;
		mem[rVBK] = 0;
		mem[rWBK] = 0;
		mem[rRP] = 0;

		// set lcd on and wait for VBlank
		mem[rLCDC] = LCDC_ON;
		while (mem[rLY] != LY_VBLANK + 4)
			idle(32);

		// enable BG and OBJ
		mem[rLCDC] = LCDC_BG_ON | LCDC_OBJ_ON;
		routines_.call(ADDR_ClearVRAM);
		routines_.call(ADDR_EnableDoubleSpeed);
		if (mem[hCGB]) {
			mem[rVBK] = 0;
			mem[rWBK] = 0;
			mem[rRP] = 0;
		}

		// .clear_wram: all WRAM but the stack, OAM, and HRAM but
		// hDemoPowerUp, hUnused_fffd and hCGB
		fill(gb_, 0xc000, 0, 0x0f00); // STARTOF(WRAM0), SIZEOF(WRAM0) - SIZEOF("Stack")
		routines_.call(ADDR_ClearWRAM);
		fill(gb_, 0xfe00, 0, 0x100); // STARTOF(OAM)
		fill(gb_, 0xff80, 0, hDemoPowerUp - 0xff80); // STARTOF(HRAM)

		// clear BGMap0 and BGMap1
		home::fill_bg_map0(gb_, 0x7f);
		routines_.call(ADDR_ClearBGMap1);

		bankswitch(BANK_MainStateTable); // BANK("Main State Table")
		mem[rROMB1 + 0x100] = 0;
		sramswitch(0); // SRAM0
		mem[rIF] = 0;
		mem[rIE] = 0;
		mem[rSCY] = 0;
		mem[rSCX] = 0;
		mem[rSTAT] = 0;

		// init hTransferVirtualOAM
		// should be (PushOAM.end - PushOAM) + 1 bytes
		uint8_t at = static_cast<uint8_t>(hTransferVirtualOAM);
		for (uint16_t src = ADDR_PushOAM; src < ADDR_WriteAToHL_BCTimes + 1; src++)
			mem[static_cast<uint16_t>(0xff00 | at++)] = mem[src];

		// init VBlank function, HRAM call function
		routines_.call(ADDR_VBlank_Ret);
		routines_.call(ADDR_InitHRAMCallFunc);
		mem[rIF] = 0;
		mem[rIE] = IE_VBLANK;
		routines_.call(ADDR_InitLCD);

		// enable SRAM
		mem[rRAMG] = RAMG_SRAM_ENABLE;
		FARCALL(ValidateSaveData);
		cpu_.ime = true; // ei

		// if not playing on CGB, then show the GB Incompatible screen,
		// otherwise init the game state
		if (!mem[hCGB]) {
			mem[wState] = ST_GB_INCOMPATIBLE;
			mem[wSubState] = 0;
		} else {
			routines_.call(ADDR_InitGameState);
		}
		mem[wJoypadPressed] = 0;
		routines_.call(ADDR_FillWhiteBGPal);
		mem[rLCDC] = LCDC_ON;

		// init audio
		cpu_.ime = false; // di
		routines_.call(ADDR_InitAudio);
		cpu_.ime = true; // ei
		JUMP_TO(GAME_LOOP); // main game loop
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeStart;

PORT_EX(UnreferencedMuteSound, 0, 0) { HomeStart(gb).port_UnreferencedMuteSound(); }
PORT_EX(UnreferencedUnmuteSound, 0, R_B | R_C | R_D | R_E) { HomeStart(gb).port_UnreferencedUnmuteSound(); }
PORT_EX(HandleSound, 0, ALL_REGISTERS) { HomeStart(gb).port_HandleSound(); }
// The game loop and the boot code read memory only: no register is live
// at .GameLoop or after the halt (UpdateJoypad and HandleSound set what
// they pass on), and Start reads only a.
PORT_LOCAL_EX(InitWithoutDemoPowerUpReset, GameLoop, HOOK_MANUAL_RET, ALL_REGISTERS) { HomeStart(gb).port_InitWithoutDemoPowerUpReset__GameLoop(); }
PORT_LOCAL_EX(InitWithoutDemoPowerUpReset, after_halt, HOOK_MANUAL_RET, ALL_REGISTERS) { HomeStart(gb).port_InitWithoutDemoPowerUpReset__after_halt(); }
PORT_LOCAL_EX(InitWithoutDemoPowerUpReset, wait_vblank_exec, HOOK_MANUAL_RET, ALL_REGISTERS) { HomeStart(gb).port_InitWithoutDemoPowerUpReset__wait_vblank_exec(); }
PORT_EX(DebugReset, HOOK_MANUAL_RET, 0) { HomeStart(gb).port_DebugReset(); }
PORT_EX(Start, HOOK_MANUAL_RET, ALL_REGISTERS) { HomeStart(gb).port_Start(); }
PORT_EX(Init, HOOK_MANUAL_RET, ALL_REGISTERS) { HomeStart(gb).port_Init(); }
PORT_EX(InitWithoutDemoPowerUpReset, HOOK_MANUAL_RET, ALL_REGISTERS) { HomeStart(gb).port_InitWithoutDemoPowerUpReset(); }
