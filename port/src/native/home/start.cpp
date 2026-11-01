// home/start (src/home/start.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

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
		gb_call_ret(gb_, ADDR_UpdateJoypad, GAME_LOOP + 0x03);
		cpu_.a = rd8(wResetDisabled);
		if (!alu_.and_(cpu_.a, cpu_.a)) {
			// restart the game if all four buttons are down
			cpu_.a = alu_.and_(rd8(wJoypadDown), PAD_BUTTONS);
			alu_.cp(cpu_.a, PAD_A | PAD_B | PAD_SELECT | PAD_START);
			if (cpu_.f & FLAG_Z) {
				gb_call_ret(gb_, ADDR_TurnMusicOff, GAME_LOOP + 0x15);
				SET_BC(gb_, SOUND_OFF);
				gb_call_ret(gb_, ADDR_TurnOffSFXID, GAME_LOOP + 0x1b);
				gb_call_ret(gb_, ADDR_UpdateAudio, GAME_LOOP + 0x1e);
				JUMP_TO(ADDR_Init);
				return;
			}
		}
		// .no_reset: where all main game logic is handled
		gb_call_ret(gb_, ADDR_MainStateTable, GAME_LOOP + 0x24);

		// The game logic took most of the frame on the hardware, but native code
		// takes no time: without this, LY would still be in V-Blank here, and
		// UpdateRoomAnimatedTiles / UpdateRoomAnimatedPals, which only animate
		// when LY is well before V-Blank, would never animate. Let V-Blank end
		// as it would have. (Not under test, where time is frozen.)
		if (!gb_->test.active)
			while (rd8(rLY) >= LY_VBLANK)
				idle(32);

		// update animated tiles if enabled (farcall)
		cpu_.a = rd8(wRoomAnimatedTilesEnabled);
		if (alu_.and_(cpu_.a, cpu_.a)) {
			cpu_.a = BANK_UpdateRoomAnimatedTiles;
			wr8(hCallFuncBank, cpu_.a);
			cpu_.a = (uint8_t)ADDR_UpdateRoomAnimatedTiles;
			wr8(hCallFuncPointer + 0, cpu_.a);
			cpu_.a = (uint8_t)(ADDR_UpdateRoomAnimatedTiles >> 8);
			wr8(hCallFuncPointer + 1, cpu_.a);
			gb_call_ret(gb_, hCallFunc, GAME_LOOP + 0x39);
		}
		// .skip_animated_tiles: update animated pals if enabled
		cpu_.a = rd8(wRoomAnimatedPalsEnabled);
		if (alu_.and_(cpu_.a, cpu_.a))
			gb_call_ret(gb_, ADDR_UpdateRoomAnimatedPals, GAME_LOOP + 0x42);

		// .skip_animated_pals: enable V-Blank handler and wait for it
		cpu_.a = TRUE;
		wr8(wEnableVBlankFunc, TRUE);
		cpu_.halted = true; // halt
		JUMP_TO(GAME_LOOP_HALT + 1);
	}

	// .after_halt and .wait_vblank_exec end back at .GameLoop, where no register
	// is live: bc and de are as HandleSound leaves them (see HandleSound).
	#define LOOP_CLOBBERS (R_B | R_C | R_D | R_E)
	void port_InitWithoutDemoPowerUpReset__after_halt() { JUMP_TO(WAIT_VBLANK_EXEC); } // nop

	void port_InitWithoutDemoPowerUpReset__wait_vblank_exec()
	{
		cpu_.a = rd8(wVBlankFuncExecuted);
		if (!alu_.and_(cpu_.a, cpu_.a)) {
			// not yet: back here, with interrupts serviced in between
			JUMP_TO(WAIT_VBLANK_EXEC);
			return;
		}
		// increment global counter
		SET_HL(gb_, wGlobalCounter);
		wr8(wGlobalCounter, alu_.inc(rd8(wGlobalCounter)));
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a (FALSE)
		wr8(wEnableVBlankFunc, FALSE);
		wr8(wVBlankFuncExecuted, FALSE);
		// audio handler
		gb_call_ret(gb_, ADDR_HandleSound, WAIT_VBLANK_EXEC + 0x14);
		JUMP_TO(GAME_LOOP); // loop back to game loop
	}

	void port_DebugReset() { JUMP(Init); }

	// The boot and reset code, falling into the main game loop. Calls return to
	// the asm's own return addresses (all in ROM0, from the call bytes).
	#define INIT_WO (ADDR_InitWithoutDemoPowerUpReset) // $0161

	void port_Start()
	{
		alu_.and_(cpu_.a, cpu_.a);
		alu_.cp(cpu_.a, BOOTUP_A_CGB);
		bool cgb = cpu_.f & FLAG_Z;
		cpu_.a = FALSE;
		if (cgb)
			cpu_.a = alu_.inc(cpu_.a); // TRUE
		// .not_cgb
		wr8(hCGB, cpu_.a);
		cpu_.a = 0x00;
		wr8(hUnused_fffd, 0x00);
		JUMP_TO(ADDR_Init); // fallthrough
	}

	void port_Init()
	{
		// reset power up to show in Demo
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(hDemoPowerUp, 0);
		JUMP_TO(INIT_WO); // fallthrough
	}

	void port_InitWithoutDemoPowerUpReset()
	{
		// init stack
		cpu_.ime = false; // di
		cpu_.ime_delay = 0;
		cpu_.sp = wStackTop - 1;
		SET_HL(gb_, cpu_.sp); // ld hl, sp+$00
		cpu_.f = 0;
		cpu_.c = 0; // aka $100 bytes
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		uint16_t hl = REG_HL(gb_);
		do { // .loop_clear_stack
			wr8(hl--, cpu_.a);
			cpu_.c = alu_.dec(cpu_.c);
		} while (!(cpu_.f & FLAG_Z));
		SET_HL(gb_, hl);

		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rVBK, 0);
		wr8(rWBK, 0);
		wr8(rRP, 0);

		// set lcd on and wait for VBlank
		cpu_.a = LCDC_ON;
		wr8(rLCDC, cpu_.a);
		for (;;) { // .wait_vblank
			cpu_.a = rd8(rLY);
			alu_.cp(cpu_.a, LY_VBLANK + 4);
			if (cpu_.f & FLAG_Z)
				break;
			idle(32);
		}

		// enable BG and OBJ
		cpu_.a = LCDC_BG_ON | LCDC_OBJ_ON;
		wr8(rLCDC, cpu_.a);
		gb_call_ret(gb_, ADDR_ClearVRAM, INIT_WO + 0x25);
		gb_call_ret(gb_, ADDR_EnableDoubleSpeed, INIT_WO + 0x28);

		cpu_.a = rd8(hCGB);
		if (alu_.and_(cpu_.a, cpu_.a)) {
			// is CGB
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
			wr8(rVBK, 0);
			wr8(rWBK, 0);
			wr8(rRP, 0);
		}

		// .clear_wram: clear all WRAM except stack
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		SET_HL(gb_, 0xc000);  // STARTOF(WRAM0)
		SET_BC(gb_, 0x0f00);  // SIZEOF(WRAM0) - SIZEOF("Stack")
		gb_call_ret(gb_, ADDR_WriteAToHL_BCTimes, INIT_WO + 0x3e);
		gb_call_ret(gb_, ADDR_ClearWRAM, INIT_WO + 0x41);

		// clear OAM
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		SET_HL(gb_, 0xfe00); // STARTOF(OAM)
		SET_BC(gb_, 0x100);
		gb_call_ret(gb_, ADDR_WriteAToHL_BCTimes, INIT_WO + 0x4b);

		// clears most of HRAM except hDemoPowerUp, hUnused_fffd and hCGB
		SET_HL(gb_, 0xff80); // STARTOF(HRAM)
		cpu_.b = (uint8_t)(hDemoPowerUp - 0xff80);
		gb_call_ret(gb_, ADDR_WriteAToHL_BTimes, INIT_WO + 0x53);

		// clear BGMap0 and BGMap1
		gb_call_ret(gb_, ADDR_FillBGMap0_With7f, INIT_WO + 0x56);
		gb_call_ret(gb_, ADDR_ClearBGMap1, INIT_WO + 0x59);

		cpu_.a = BANK_MainStateTable; // BANK("Main State Table")
		bankswitch(cpu_.a);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rROMB1 + 0x100, 0);
		sramswitch(0); // xor a / sramswitch: SRAM0

		wr8(rIF, 0);
		wr8(rIE, 0);
		wr8(rSCY, 0);
		wr8(rSCX, 0);
		wr8(rSTAT, 0);

		// init hTransferVirtualOAM
		cpu_.c = (uint8_t)hTransferVirtualOAM;
		// should be ld b, (PushOAM.end - PushOAM) + 1
		cpu_.b = (uint8_t)(ADDR_WriteAToHL_BCTimes - ADDR_PushOAM + 1);
		hl = ADDR_PushOAM;
		do { // .loop_write_oam_func
			cpu_.a = rd8(hl++);
			wr8((uint16_t)(0xff00 + cpu_.c), cpu_.a);
			cpu_.c = alu_.inc(cpu_.c);
			cpu_.b = alu_.dec(cpu_.b);
		} while (!(cpu_.f & FLAG_Z));
		SET_HL(gb_, hl);

		// init VBlank function, HRAM call function
		gb_call_ret(gb_, ADDR_VBlank_Ret, INIT_WO + 0x87);
		gb_call_ret(gb_, ADDR_InitHRAMCallFunc, INIT_WO + 0x8a);

		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(rIF, 0);
		cpu_.a = IE_VBLANK;
		wr8(rIE, cpu_.a);
		gb_call_ret(gb_, ADDR_InitLCD, INIT_WO + 0x94);

		// enable SRAM
		cpu_.a = RAMG_SRAM_ENABLE;
		wr8(rRAMG, cpu_.a);
		FARCALL_RET(ValidateSaveData, INIT_WO + 0xa8);
		cpu_.ime = true; // ei

		// if not playing on CGB, then show the GB Incompatible screen
		cpu_.a = rd8(hCGB);
		if (!alu_.and_(cpu_.a, cpu_.a)) {
			cpu_.a = ST_GB_INCOMPATIBLE;
			wr8(wState, cpu_.a);
			cpu_.a = 0;
			cpu_.f = FLAG_Z; // xor a
			wr8(wSubState, 0);
		} else {
			// otherwise, init the game state
			gb_call_ret(gb_, ADDR_InitGameState, INIT_WO + 0xbc);
		}

		// .skip_init_game_state
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wJoypadPressed, 0);
		gb_call_ret(gb_, ADDR_FillWhiteBGPal, INIT_WO + 0xc3);
		cpu_.a = LCDC_ON;
		wr8(rLCDC, cpu_.a);

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
PORT_LOCAL_EX(InitWithoutDemoPowerUpReset, GameLoop, HOOK_MANUAL_RET, 0) { HomeStart(gb).port_InitWithoutDemoPowerUpReset__GameLoop(); }
PORT_LOCAL_EX(InitWithoutDemoPowerUpReset, after_halt, HOOK_MANUAL_RET, LOOP_CLOBBERS) { HomeStart(gb).port_InitWithoutDemoPowerUpReset__after_halt(); }
PORT_LOCAL_EX(InitWithoutDemoPowerUpReset, wait_vblank_exec, HOOK_MANUAL_RET, LOOP_CLOBBERS) { HomeStart(gb).port_InitWithoutDemoPowerUpReset__wait_vblank_exec(); }
PORT_EX(DebugReset, HOOK_MANUAL_RET, 0) { HomeStart(gb).port_DebugReset(); }
PORT_EX(Start, HOOK_MANUAL_RET, 0) { HomeStart(gb).port_Start(); }
PORT_EX(Init, HOOK_MANUAL_RET, 0) { HomeStart(gb).port_Init(); }
PORT_EX(InitWithoutDemoPowerUpReset, HOOK_MANUAL_RET, 0) { HomeStart(gb).port_InitWithoutDemoPowerUpReset(); }
