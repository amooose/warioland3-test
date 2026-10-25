// Port of src/home/room_transition.asm
#include "ported.h"

PORT(TriggerRoomTransition)
{
	gb->cpu.a = 0;
	wr8(gb, wGrabState, 0);
	wr8(gb, wRoomPalCycleDuration, 0);
	wr8(gb, wRoomAnimatedPalsEnabled, 0);
	wr8(gb, wWaterCurrent, 0);
	wr8(gb, wLastWaterCurrent, 0);
	wr8(gb, wCurWaterCurrent, 0);
	wr8(gb, wBossState, 0);
	wr8(gb, wIsBossBattle, 0);
	gb->cpu.a = TRUE;
	gb->cpu.f = 0; // xor a / inc a
	wr8(gb, wIsIntangible, gb->cpu.a);

	gb->cpu.a = rd8(gb, wRoomTransitionParam);
	if (alu_bit(gb, gb->cpu.a, ROOMTRANSITIONF_DELAY_F))
		return; // delayed, don't transition for now

	uint16_t hl = wSubState;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, wRoomTransitionParam);
	if (alu_bit(gb, gb->cpu.a, ROOMTRANSITIONF_DOOR_F)) {
		wr8(gb, hl, alu_inc(gb, rd8(gb, hl))); // door transition
		FARCALL(GetWarioBGPtr);
		// push_wram $03
		gb->cpu.a = rd8(gb, rWBK);
		push_af(gb);
		gb->cpu.a = 0x03;
		wr8(gb, rWBK, gb->cpu.a);
		gb->cpu.a = gb->cpu.h;
		wr8(gb, wWarioTileBGPtr + 1, gb->cpu.a);
		gb->cpu.a = gb->cpu.l;
		wr8(gb, wWarioTileBGPtr + 0, gb->cpu.a);
		// pop_wram
		pop_af(gb);
		wr8(gb, rWBK, gb->cpu.a);
		return;
	}

	// .not_door
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	wr8(gb, hl, alu_inc(gb, rd8(gb, hl)));
	// stop_music
	wr8(gb, hMusicID + 0, 0xff);
	wr8(gb, hMusicID + 1, 0x00);
	// stop_sfx
	wr8(gb, hSFXID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hSFXID + 1, 0x00);
}
