// home/start (src/home/start.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"
#include "memory.hpp"

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

	// handle playing SFX and music. The audio entry points leave bc and de as
	// the engine's loops did (see src/native/audio): nothing after HandleSound
	// reads them before setting them (the main loop goes on to UpdateJoypad).
	void port_HandleSound()
	{
		cpu_.a = alu_.and_(rd8(wSoundMuted), rd8(wSoundMuted));
		if (cpu_.a)
			return;
		cpu_.a = rd8(hSFXID);
		SET_HL(gb_, hSFXID + 1);
		alu_.cp(cpu_.a, 0xff);
		bool play = true;
		if (cpu_.f & FLAG_Z) {
			SET_BC(gb_, SOUND_OFF);
		} else {
			// .valid_sfx
			cpu_.c = rd8(hSFXID + 1);
			cpu_.b = cpu_.a;
			cpu_.a = alu_.or_(cpu_.a, cpu_.c);
			play = cpu_.a != 0;
		}
		if (play) {
			// .got_sfx
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
			wr8(hSFXID + 1, 0);
			wr8(hSFXID + 0, 0);
			SET_HL(gb_, hSFXID);
			CALL(PlaySFX);
		}

		// .music
		cpu_.a = rd8(hMusicID);
		SET_HL(gb_, hMusicID + 1);
		alu_.cp(cpu_.a, 0xff);
		if (cpu_.f & FLAG_Z) {
			SET_BC(gb_, SOUND_OFF);
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
			wr8(hMusicID + 1, 0);
			wr8(hMusicID + 0, 0);
			SET_HL(gb_, hMusicID);
			CALL(PlayNewMusic);
		} else {
			// .valid_music
			cpu_.c = rd8(hMusicID + 1);
			cpu_.b = cpu_.a;
			cpu_.a = alu_.or_(cpu_.a, cpu_.c);
			if (cpu_.a) {
				cpu_.a = 0;
				cpu_.f = FLAG_Z; // xor a
				wr8(hMusicID + 1, 0);
				wr8(hMusicID + 0, 0);
				SET_HL(gb_, hMusicID);
				CALL(PlayNewMusic_SetNoise);
			}
		}
		// .update
		CALL(UpdateAudio);
	}

	// The main game loop (InitWithoutDemoPowerUpReset.GameLoop), one game
	// frame per pass, in parts around its halt: .GameLoop halts the CPU as the
	// halt does (the V-Blank interrupt wakes it as on the hardware), and the
	// loop goes on at .after_halt. Every call returns to the asm's own return
	// address.
	#define GAME_LOOP (ADDR_InitWithoutDemoPowerUpReset + 0xcc)        // .GameLoop
	#define GAME_LOOP_HALT (ADDR_InitWithoutDemoPowerUpReset + 0x113)   // halt
	#define WAIT_VBLANK_EXEC (ADDR_InitWithoutDemoPowerUpReset + 0x115) // .wait_vblank_exec

	void port_InitWithoutDemoPowerUpReset__GameLoop()
	{
		Memory mem(gb_);
		gb_call_ret(gb_, ADDR_UpdateJoypad, GAME_LOOP + 0x03);
		// restart the game if all four buttons are down
		if (!mem[wResetDisabled]
			&& (mem[wJoypadDown] & PAD_BUTTONS) == (PAD_A | PAD_B | PAD_SELECT | PAD_START)) {
			gb_call_ret(gb_, ADDR_TurnMusicOff, GAME_LOOP + 0x15);
			SET_BC(gb_, SOUND_OFF); // TurnOffSFXID's c goes on to UpdateAudio
			gb_call_ret(gb_, ADDR_TurnOffSFXID, GAME_LOOP + 0x1b);
			gb_call_ret(gb_, ADDR_UpdateAudio, GAME_LOOP + 0x1e);
			JUMP_TO(ADDR_Init);
			return;
		}
		// .no_reset: where all main game logic is handled
		gb_call_ret(gb_, ADDR_MainStateTable, GAME_LOOP + 0x24);

		// The game logic took most of the frame on the hardware, but native code
		// takes no time: without this, LY would still be in V-Blank here, and
		// UpdateRoomAnimatedTiles / UpdateRoomAnimatedPals, which only animate
		// when LY is well before V-Blank, would never animate. Let V-Blank end
		// as it would have. (Not under test, where time is frozen.)
		if (!gb_->test.active)
			while (mem[rLY] >= LY_VBLANK)
				idle(32);

		if (mem[wRoomAnimatedTilesEnabled]) { // farcall, to the asm's return address
			mem[hCallFuncBank] = BANK_UpdateRoomAnimatedTiles;
			mem[hCallFuncPointer + 0] = static_cast<uint8_t>(ADDR_UpdateRoomAnimatedTiles);
			mem[hCallFuncPointer + 1] = static_cast<uint8_t>(ADDR_UpdateRoomAnimatedTiles >> 8);
			gb_call_ret(gb_, hCallFunc, GAME_LOOP + 0x39);
		}
		if (mem[wRoomAnimatedPalsEnabled])
			gb_call_ret(gb_, ADDR_UpdateRoomAnimatedPals, GAME_LOOP + 0x42);

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
		gb_call_ret(gb_, ADDR_HandleSound, WAIT_VBLANK_EXEC + 0x14);
		JUMP_TO(GAME_LOOP); // loop back to game loop
	}

	void port_DebugReset() { JUMP(Init); }

	// The boot and reset code, falling into the main game loop. Calls return to
	// the asm's own return addresses (all in ROM0, from the call bytes).
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
		gb_call_ret(gb_, ADDR_ClearVRAM, INIT_WO + 0x25);
		gb_call_ret(gb_, ADDR_EnableDoubleSpeed, INIT_WO + 0x28);
		if (mem[hCGB]) {
			mem[rVBK] = 0;
			mem[rWBK] = 0;
			mem[rRP] = 0;
		}

		// .clear_wram: all WRAM but the stack, OAM, and HRAM but
		// hDemoPowerUp, hUnused_fffd and hCGB
		fill(gb_, 0xc000, 0, 0x0f00); // STARTOF(WRAM0), SIZEOF(WRAM0) - SIZEOF("Stack")
		gb_call_ret(gb_, ADDR_ClearWRAM, INIT_WO + 0x41);
		fill(gb_, 0xfe00, 0, 0x100); // STARTOF(OAM)
		fill(gb_, 0xff80, 0, hDemoPowerUp - 0xff80); // STARTOF(HRAM)

		// clear BGMap0 and BGMap1
		home::fill_bg_map0(gb_, 0x7f);
		gb_call_ret(gb_, ADDR_ClearBGMap1, INIT_WO + 0x59);

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
		gb_call_ret(gb_, ADDR_VBlank_Ret, INIT_WO + 0x87);
		gb_call_ret(gb_, ADDR_InitHRAMCallFunc, INIT_WO + 0x8a);
		mem[rIF] = 0;
		mem[rIE] = IE_VBLANK;
		gb_call_ret(gb_, ADDR_InitLCD, INIT_WO + 0x94);

		// enable SRAM
		mem[rRAMG] = RAMG_SRAM_ENABLE;
		FARCALL_RET(ValidateSaveData, INIT_WO + 0xa8);
		cpu_.ime = true; // ei

		// if not playing on CGB, then show the GB Incompatible screen,
		// otherwise init the game state
		if (!mem[hCGB]) {
			mem[wState] = ST_GB_INCOMPATIBLE;
			mem[wSubState] = 0;
		} else {
			gb_call_ret(gb_, ADDR_InitGameState, INIT_WO + 0xbc);
		}
		mem[wJoypadPressed] = 0;
		gb_call_ret(gb_, ADDR_FillWhiteBGPal, INIT_WO + 0xc3);
		mem[rLCDC] = LCDC_ON;

		// init audio
		cpu_.ime = false; // di
		gb_call_ret(gb_, ADDR_InitAudio, INIT_WO + 0xcb);
		cpu_.ime = true; // ei
		JUMP_TO(GAME_LOOP); // main game loop
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeStart;

PORT_EX(UnreferencedMuteSound, 0, 0) { HomeStart(gb).port_UnreferencedMuteSound(); }
PORT_EX(UnreferencedUnmuteSound, 0, R_B | R_C | R_D | R_E) { HomeStart(gb).port_UnreferencedUnmuteSound(); }
PORT_EX(HandleSound, 0, R_B | R_C | R_D | R_E) { HomeStart(gb).port_HandleSound(); }
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
