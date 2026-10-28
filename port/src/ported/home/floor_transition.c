// Port of src/home/floor_transition.asm
#include "ported.h"

// returns in c:
// 6 if   $0 <= ypos <  $80
// 5 if  $80 <= ypos < $100
// 4 if $100 <= ypos < $180
// 3 if $180 <= ypos < $200
// 2 if $200 <= ypos < $280
// 1 if $280 <= ypos < $300
PORT(GetFloorForYPos)
{
	uint8_t hi = alu_dec(gb, rd8(gb, wWarioYPos + 0));
	if (!hi)
		gb->cpu.c = 3;
	else if (!alu_dec(gb, hi))
		gb->cpu.c = 1;
	else
		gb->cpu.c = 5;

	gb->cpu.a = rd8(gb, wWarioYPos + 1);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (gb->cpu.f & FLAG_C)
		gb->cpu.c = alu_inc(gb, gb->cpu.c); // < $80
}

PORT(UpdateFloor)
{
	CALL(GetFloorForYPos);
	gb->cpu.a = gb->cpu.c;
	wr8(gb, wFloor, gb->cpu.a);
}

// sets off the floor transition, after wFloor and wFloorTransitionTimer
static void start_floor_transition(GB *gb, uint8_t dir)
{
	play_sfx(gb, SFX_PIPE_EXIT);
	gb->cpu.a = dir;
	wr8(gb, wFloorTransitionDir, dir);
	gb->cpu.a = TRUE;
	wr8(gb, wIsFloorTransition, TRUE);
}

// c = new floor
PORT(StartDownwardsFloorTransition)
{
	gb->cpu.a = gb->cpu.c;
	wr8(gb, wFloor, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 5);
	gb->cpu.a = (gb->cpu.f & FLAG_Z) ? 0x68 : 0x80;
	wr8(gb, wFloorTransitionTimer, gb->cpu.a);
	start_floor_transition(gb, FLOOR_TRANSITION_UP);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCameraRoomYScrollLimit, 0);
}

// c = new floor
PORT(StartUpwardsFloorTransition)
{
	gb->cpu.a = gb->cpu.c;
	wr8(gb, wFloor, gb->cpu.a);
	start_floor_transition(gb, FLOOR_TRANSITION_DOWN);
	gb->cpu.a = 0x80;
	wr8(gb, wFloorTransitionTimer, 0x80);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCameraRoomYScrollLimit, 0);
}

// The floor check: CAM_TRANSITIONS and Wario on another floor; a = wFloor - c
static bool floor_changed(GB *gb)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wCameraConfigFlags), CAM_SCROLLING_MASK);
	alu_cp(gb, gb->cpu.a, CAM_TRANSITIONS);
	if (gb->cpu.f & FLAG_C)
		return false;
	CALL(GetFloorForYPos);
	gb->cpu.a = alu_sub(gb, rd8(gb, wFloor), gb->cpu.c);
	return true;
}

PORT_JUMP(HandleDownwardsFloorTransition)
{
	if (floor_changed(gb) && !(gb->cpu.f & (FLAG_Z | FLAG_C))) {
		JUMP(StartDownwardsFloorTransition);
		return;
	}
	RET();
}

// unreferenced?
PORT_JUMP(Func_1180)
{
	if (floor_changed(gb) && !(gb->cpu.f & FLAG_Z)) {
		if (gb->cpu.f & FLAG_C)
			JUMP(StartUpwardsFloorTransition);
		else
			JUMP(StartDownwardsFloorTransition);
		return;
	}
	RET();
}

PORT_JUMP(HandleUpwardsFloorTransition)
{
	if (floor_changed(gb) && (gb->cpu.f & FLAG_C)) {
		JUMP(StartUpwardsFloorTransition);
		return;
	}
	RET();
}
