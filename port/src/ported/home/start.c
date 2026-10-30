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
PORT(UnreferencedUnmuteSound)
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

// handle playing SFX and music
PORT(HandleSound)
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
// frame per pass, in two parts around its halt, which the interpreter runs
// so that the V-Blank interrupt wakes it as on the hardware. Every call
// returns to the asm's own return address.
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
	JUMP_TO(GAME_LOOP_HALT);
}

PORT_LOCAL_JUMP(InitWithoutDemoPowerUpReset, wait_vblank_exec)
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
