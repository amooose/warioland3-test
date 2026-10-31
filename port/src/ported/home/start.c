// Port of src/home/start.asm
#include "ported.h"

// unreferenced
PORT(UnreferencedMuteSound)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wSoundMuted), rd8(gb, wSoundMuted));
	if (gb->cpu.a)
		return;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hSFXID + 0, 0);
	wr8(gb, hSFXID + 1, 0);
	CALL(TurnMusicOff);
	SET_BC(gb, SOUND_OFF);
	CALL(TurnOffSFXID);
	CALL(UpdateAudio);
	gb->cpu.a = TRUE;
	wr8(gb, wSoundMuted, TRUE);
}

// unreferenced
// ends with bc and de as PlayNewMusic_SetNoise leaves them (unreferenced)
PORT_EX(UnreferencedUnmuteSound, 0, R_B | R_C | R_D | R_E)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wSoundMuted), rd8(gb, wSoundMuted));
	if (!gb->cpu.a)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSoundMuted, 0);
	wr8(gb, hSFXID + 0, 0);
	wr8(gb, hSFXID + 1, 0);
	gb->cpu.b = rd8(gb, hMusicIDBackup + 0);
	gb->cpu.c = gb->cpu.a = rd8(gb, hMusicIDBackup + 1);
	CALL(PlayNewMusic_SetNoise);
}

// handle playing SFX and music. The audio entry points leave bc and de as
// the engine's loops did (see src/native/audio): nothing after HandleSound
// reads them before setting them (the main loop goes on to UpdateJoypad).
PORT_EX(HandleSound, 0, R_B | R_C | R_D | R_E)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wSoundMuted), rd8(gb, wSoundMuted));
	if (gb->cpu.a)
		return;
	gb->cpu.a = rd8(gb, hSFXID);
	SET_HL(gb, hSFXID + 1);
	alu_cp(gb, gb->cpu.a, 0xff);
	bool play = true;
	if (gb->cpu.f & FLAG_Z) {
		SET_BC(gb, SOUND_OFF);
	} else {
		// .valid_sfx
		gb->cpu.c = rd8(gb, hSFXID + 1);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_or(gb, gb->cpu.a, gb->cpu.c);
		play = gb->cpu.a != 0;
	}
	if (play) {
		// .got_sfx
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hSFXID + 1, 0);
		wr8(gb, hSFXID + 0, 0);
		SET_HL(gb, hSFXID);
		CALL(PlaySFX);
	}

	// .music
	gb->cpu.a = rd8(gb, hMusicID);
	SET_HL(gb, hMusicID + 1);
	alu_cp(gb, gb->cpu.a, 0xff);
	if (gb->cpu.f & FLAG_Z) {
		SET_BC(gb, SOUND_OFF);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hMusicID + 1, 0);
		wr8(gb, hMusicID + 0, 0);
		SET_HL(gb, hMusicID);
		CALL(PlayNewMusic);
	} else {
		// .valid_music
		gb->cpu.c = rd8(gb, hMusicID + 1);
		gb->cpu.b = gb->cpu.a;
		gb->cpu.a = alu_or(gb, gb->cpu.a, gb->cpu.c);
		if (gb->cpu.a) {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, hMusicID + 1, 0);
			wr8(gb, hMusicID + 0, 0);
			SET_HL(gb, hMusicID);
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

PORT_LOCAL_JUMP(InitWithoutDemoPowerUpReset, GameLoop)
{
	gb_call_ret(gb, ADDR_UpdateJoypad, GAME_LOOP + 0x03);
	gb->cpu.a = rd8(gb, wResetDisabled);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// restart the game if all four buttons are down
		gb->cpu.a = alu_and(gb, rd8(gb, wJoypadDown), PAD_BUTTONS);
		alu_cp(gb, gb->cpu.a, PAD_A | PAD_B | PAD_SELECT | PAD_START);
		if (gb->cpu.f & FLAG_Z) {
			gb_call_ret(gb, ADDR_TurnMusicOff, GAME_LOOP + 0x15);
			SET_BC(gb, SOUND_OFF);
			gb_call_ret(gb, ADDR_TurnOffSFXID, GAME_LOOP + 0x1b);
			gb_call_ret(gb, ADDR_UpdateAudio, GAME_LOOP + 0x1e);
			JUMP_TO(ADDR_Init);
			return;
		}
	}
	// .no_reset: where all main game logic is handled
	gb_call_ret(gb, ADDR_MainStateTable, GAME_LOOP + 0x24);

	// The game logic took most of the frame on the hardware, but native code
	// takes no time: without this, LY would still be in V-Blank here, and
	// UpdateRoomAnimatedTiles / UpdateRoomAnimatedPals, which only animate
	// when LY is well before V-Blank, would never animate. Let V-Blank end
	// as it would have. (Not under test, where time is frozen.)
	if (!gb->test.active)
		while (rd8(gb, rLY) >= LY_VBLANK)
			idle(gb, 32);

	// update animated tiles if enabled (farcall)
	gb->cpu.a = rd8(gb, wRoomAnimatedTilesEnabled);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = BANK_UpdateRoomAnimatedTiles;
		wr8(gb, hCallFuncBank, gb->cpu.a);
		gb->cpu.a = (uint8_t)ADDR_UpdateRoomAnimatedTiles;
		wr8(gb, hCallFuncPointer + 0, gb->cpu.a);
		gb->cpu.a = (uint8_t)(ADDR_UpdateRoomAnimatedTiles >> 8);
		wr8(gb, hCallFuncPointer + 1, gb->cpu.a);
		gb_call_ret(gb, hCallFunc, GAME_LOOP + 0x39);
	}
	// .skip_animated_tiles: update animated pals if enabled
	gb->cpu.a = rd8(gb, wRoomAnimatedPalsEnabled);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		gb_call_ret(gb, ADDR_UpdateRoomAnimatedPals, GAME_LOOP + 0x42);

	// .skip_animated_pals: enable V-Blank handler and wait for it
	gb->cpu.a = TRUE;
	wr8(gb, wEnableVBlankFunc, TRUE);
	gb->cpu.halted = true; // halt
	JUMP_TO(GAME_LOOP_HALT + 1);
}

// .after_halt and .wait_vblank_exec end back at .GameLoop, where no register
// is live: bc and de are as HandleSound leaves them (see HandleSound).
#define LOOP_CLOBBERS (R_B | R_C | R_D | R_E)
PORT_LOCAL_EX(InitWithoutDemoPowerUpReset, after_halt, HOOK_MANUAL_RET, LOOP_CLOBBERS) { JUMP_TO(WAIT_VBLANK_EXEC); } // nop

PORT_LOCAL_EX(InitWithoutDemoPowerUpReset, wait_vblank_exec, HOOK_MANUAL_RET, LOOP_CLOBBERS)
{
	gb->cpu.a = rd8(gb, wVBlankFuncExecuted);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// not yet: back here, with interrupts serviced in between
		JUMP_TO(WAIT_VBLANK_EXEC);
		return;
	}
	// increment global counter
	SET_HL(gb, wGlobalCounter);
	wr8(gb, wGlobalCounter, alu_inc(gb, rd8(gb, wGlobalCounter)));
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a (FALSE)
	wr8(gb, wEnableVBlankFunc, FALSE);
	wr8(gb, wVBlankFuncExecuted, FALSE);
	// audio handler
	gb_call_ret(gb, ADDR_HandleSound, WAIT_VBLANK_EXEC + 0x14);
	JUMP_TO(GAME_LOOP); // loop back to game loop
}

PORT_JUMP(DebugReset) { JUMP(Init); }

// The boot and reset code, falling into the main game loop. Calls return to
// the asm's own return addresses (all in ROM0, from the call bytes).
#define INIT_WO (ADDR_InitWithoutDemoPowerUpReset) // $0161

PORT_JUMP(Start)
{
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, BOOTUP_A_CGB);
	bool cgb = gb->cpu.f & FLAG_Z;
	gb->cpu.a = FALSE;
	if (cgb)
		gb->cpu.a = alu_inc(gb, gb->cpu.a); // TRUE
	// .not_cgb
	wr8(gb, hCGB, gb->cpu.a);
	gb->cpu.a = 0x00;
	wr8(gb, hUnused_fffd, 0x00);
	JUMP_TO(ADDR_Init); // fallthrough
}

PORT_JUMP(Init)
{
	// reset power up to show in Demo
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hDemoPowerUp, 0);
	JUMP_TO(INIT_WO); // fallthrough
}

PORT_JUMP(InitWithoutDemoPowerUpReset)
{
	// init stack
	gb->cpu.ime = false; // di
	gb->cpu.ime_delay = 0;
	gb->cpu.sp = wStackTop - 1;
	SET_HL(gb, gb->cpu.sp); // ld hl, sp+$00
	gb->cpu.f = 0;
	gb->cpu.c = 0; // aka $100 bytes
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	uint16_t hl = REG_HL(gb);
	do { // .loop_clear_stack
		wr8(gb, hl--, gb->cpu.a);
		gb->cpu.c = alu_dec(gb, gb->cpu.c);
	} while (!(gb->cpu.f & FLAG_Z));
	SET_HL(gb, hl);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	wr8(gb, rWBK, 0);
	wr8(gb, rRP, 0);

	// set lcd on and wait for VBlank
	gb->cpu.a = LCDC_ON;
	wr8(gb, rLCDC, gb->cpu.a);
	for (;;) { // .wait_vblank
		gb->cpu.a = rd8(gb, rLY);
		alu_cp(gb, gb->cpu.a, LY_VBLANK + 4);
		if (gb->cpu.f & FLAG_Z)
			break;
		idle(gb, 32);
	}

	// enable BG and OBJ
	gb->cpu.a = LCDC_BG_ON | LCDC_OBJ_ON;
	wr8(gb, rLCDC, gb->cpu.a);
	gb_call_ret(gb, ADDR_ClearVRAM, INIT_WO + 0x25);
	gb_call_ret(gb, ADDR_EnableDoubleSpeed, INIT_WO + 0x28);

	gb->cpu.a = rd8(gb, hCGB);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		// is CGB
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, rVBK, 0);
		wr8(gb, rWBK, 0);
		wr8(gb, rRP, 0);
	}

	// .clear_wram: clear all WRAM except stack
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, 0xc000);  // STARTOF(WRAM0)
	SET_BC(gb, 0x0f00);  // SIZEOF(WRAM0) - SIZEOF("Stack")
	gb_call_ret(gb, ADDR_WriteAToHL_BCTimes, INIT_WO + 0x3e);
	gb_call_ret(gb, ADDR_ClearWRAM, INIT_WO + 0x41);

	// clear OAM
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, 0xfe00); // STARTOF(OAM)
	SET_BC(gb, 0x100);
	gb_call_ret(gb, ADDR_WriteAToHL_BCTimes, INIT_WO + 0x4b);

	// clears most of HRAM except hDemoPowerUp, hUnused_fffd and hCGB
	SET_HL(gb, 0xff80); // STARTOF(HRAM)
	gb->cpu.b = (uint8_t)(hDemoPowerUp - 0xff80);
	gb_call_ret(gb, ADDR_WriteAToHL_BTimes, INIT_WO + 0x53);

	// clear BGMap0 and BGMap1
	gb_call_ret(gb, ADDR_FillBGMap0_With7f, INIT_WO + 0x56);
	gb_call_ret(gb, ADDR_ClearBGMap1, INIT_WO + 0x59);

	gb->cpu.a = BANK_MainStateTable; // BANK("Main State Table")
	bankswitch(gb, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rROMB1 + 0x100, 0);
	sramswitch(gb, 0); // xor a / sramswitch: SRAM0

	wr8(gb, rIF, 0);
	wr8(gb, rIE, 0);
	wr8(gb, rSCY, 0);
	wr8(gb, rSCX, 0);
	wr8(gb, rSTAT, 0);

	// init hTransferVirtualOAM
	gb->cpu.c = (uint8_t)hTransferVirtualOAM;
	// should be ld b, (PushOAM.end - PushOAM) + 1
	gb->cpu.b = (uint8_t)(ADDR_WriteAToHL_BCTimes - ADDR_PushOAM + 1);
	hl = ADDR_PushOAM;
	do { // .loop_write_oam_func
		gb->cpu.a = rd8(gb, hl++);
		wr8(gb, (uint16_t)(0xff00 + gb->cpu.c), gb->cpu.a);
		gb->cpu.c = alu_inc(gb, gb->cpu.c);
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
	} while (!(gb->cpu.f & FLAG_Z));
	SET_HL(gb, hl);

	// init VBlank function, HRAM call function
	gb_call_ret(gb, ADDR_VBlank_Ret, INIT_WO + 0x87);
	gb_call_ret(gb, ADDR_InitHRAMCallFunc, INIT_WO + 0x8a);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rIF, 0);
	gb->cpu.a = IE_VBLANK;
	wr8(gb, rIE, gb->cpu.a);
	gb_call_ret(gb, ADDR_InitLCD, INIT_WO + 0x94);

	// enable SRAM
	gb->cpu.a = RAMG_SRAM_ENABLE;
	wr8(gb, rRAMG, gb->cpu.a);
	FARCALL_RET(ValidateSaveData, INIT_WO + 0xa8);
	gb->cpu.ime = true; // ei

	// if not playing on CGB, then show the GB Incompatible screen
	gb->cpu.a = rd8(gb, hCGB);
	if (!alu_and(gb, gb->cpu.a, gb->cpu.a)) {
		gb->cpu.a = ST_GB_INCOMPATIBLE;
		wr8(gb, wState, gb->cpu.a);
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wSubState, 0);
	} else {
		// otherwise, init the game state
		gb_call_ret(gb, ADDR_InitGameState, INIT_WO + 0xbc);
	}

	// .skip_init_game_state
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wJoypadPressed, 0);
	gb_call_ret(gb, ADDR_FillWhiteBGPal, INIT_WO + 0xc3);
	gb->cpu.a = LCDC_ON;
	wr8(gb, rLCDC, gb->cpu.a);

	// init audio
	gb->cpu.ime = false; // di
	gb_call_ret(gb, ADDR_InitAudio, INIT_WO + 0xcb);
	gb->cpu.ime = true; // ei
	JUMP_TO(GAME_LOOP); // main game loop
}
