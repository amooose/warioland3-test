// Port of src/engine/level/update_level.asm
#include "ported.h"

#define ZF (gb->cpu.f & FLAG_Z)
#define CF (gb->cpu.f & FLAG_C)

// Every call below pushes the asm's own return address (UpdateLevel + off),
// since the block queue code reads its callers' return addresses off the
// stack.
#define UL(off) ((uint16_t)(ADDR_UpdateLevel + (off)))

static inline void xor_a(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
}

// push_wram BANK("Level Objects WRAM")
static void push_wram(GB *gb)
{
	gb->cpu.a = rd8(gb, rWBK);
	push_af(gb);
	gb->cpu.a = BANK_wObjects;
	wr8(gb, rWBK, gb->cpu.a);
}

// pop_wram
static void pop_wram(GB *gb)
{
	pop_af(gb);
	wr8(gb, rWBK, gb->cpu.a);
}

static void wario_flag(GB *gb, bool on)
{
	if (on) {
		gb->cpu.a = TRUE;
	} else {
		xor_a(gb);
	}
	wr8(gb, wBlockFuncWarioFlag, gb->cpu.a);
}

static void set_scroll_from_camera(GB *gb)
{
	gb->cpu.a = rd8(gb, wCameraSCY + 1);
	wr8(gb, wSCY, gb->cpu.a);
	gb->cpu.a = rd8(gb, wCameraSCX + 1);
	wr8(gb, wSCX, gb->cpu.a);
}

// Return-address offsets for the .free_camera / .yscroll_camera paths.
typedef struct {
	uint16_t wario, objects, screen_pos, interactions, no_prio, draw_wario, with_prio,
		clear_oam, vertical, horizontal;
} CamPath;

static const CamPath free_camera = {0x29b, 0x2c1, 0x2c7, 0x2db, 0x2f5, 0x2fb, 0x311, 0x317, 0x321, 0x324};
static const CamPath yscroll_camera = {0x33b, 0x361, 0x367, 0x37b, 0x395, 0x39b, 0x3b1, 0x3b7, 0x3c1, 0};

static void cam_path(GB *gb, const CamPath *p)
{
	wario_flag(gb, true);
	FARCALL_RET(UpdateWarioStates, UL(p->wario));
	set_scroll_from_camera(gb);
	wario_flag(gb, false);
	push_wram(gb);
	FARCALL_RET(UpdateObjects, UL(p->objects));
	pop_wram(gb);
	gb_call_ret(gb, ADDR_SetWarioScreenPos, UL(p->screen_pos));
	wario_flag(gb, true);
	FARCALL_RET(ProcessInteractions, UL(p->interactions));
	wario_flag(gb, false);
	push_wram(gb);
	FARCALL_RET(DrawObjects_NoPriority, UL(p->no_prio));
	pop_wram(gb);
	gb_call_ret(gb, ADDR_DrawWario, UL(p->draw_wario));
	push_wram(gb);
	FARCALL_RET(DrawObjects_WithPriority, UL(p->with_prio));
	pop_wram(gb);
	gb_call_ret(gb, ADDR_ClearUnusedVirtualOAM, UL(p->clear_oam));
	xor_a(gb);
	wr8(gb, wScrollUpEdge, 0);
	wr8(gb, wScrollDownEdge, 0);
	gb_call_ret(gb, ADDR_LevelScroll_Vertical, UL(p->vertical));
	if (p->horizontal)
		gb_call_ret(gb, ADDR_LevelScroll_Horizontal, UL(p->horizontal));
}

// .no_floor_transition's caller: the end-screen, multi-block, time and
// pause checks from .check_end_screen on.
static void check_end_screen(GB *gb)
{
	gb->cpu.a = rd8(gb, wState);
	alu_cp(gb, gb->cpu.a, ST_LEVEL);
	if (!ZF)
		return;
	SET_HL(gb, wLevelEndScreen);
	gb->cpu.a = alu_and(gb, rd8(gb, wLevelEndScreen), rd8(gb, wLevelEndScreen));
	if (!ZF) {
		if (alu_bit(gb, gb->cpu.a, 7))
			return;

		gb->cpu.a = TRUE;
		wr8(gb, wResetDisabled, gb->cpu.a);

		xor_a(gb);
		wr8(gb, wGroundShakeCounter, 0);
		wr8(gb, wSCYShake, 0);
		wr8(gb, wAnimatedTilesFrameDuration, 0);
		wr8(gb, wRoomPalCycleDuration, 0);
		wr8(gb, wRoomAnimatedTilesEnabled, 0);
		wr8(gb, wRoomAnimatedPalsEnabled, 0);
		wr8(gb, wWaterCurrent, 0);
		wr8(gb, wLastWaterCurrent, 0);
		wr8(gb, wCurWaterCurrent, 0);
		wr8(gb, wSwimmingDirectionInput, 0);

		SET_HL(gb, wGameModeFlags);
		gb->cpu.a = rd8(gb, wLevelEndScreen);
		alu_cp(gb, gb->cpu.a, LVLEND_GAME_OVER);
		bool game_over = ZF;
		bool epilogue = false;
		if (!game_over) {
			alu_cp(gb, gb->cpu.a, LVLEND_EPILOGUE);
			epilogue = ZF;
		}
		if (!game_over && !epilogue) {
			SET_HL(gb, wState);
			wr8(gb, wState, alu_inc(gb, rd8(gb, wState))); // ST_CLEAR
			xor_a(gb);
			wr8(gb, wSubState, 0);
			gb->cpu.a = rd8(gb, wGameModeFlags);
			if (!alu_bit(gb, gb->cpu.a, MODE_TIME_ATTACK_F))
				return;
			gb->cpu.a = SST_CLEAR_TIME_ATTACK;
			wr8(gb, wSubState, gb->cpu.a);
			return;
		}

		uint8_t flags = rd8(gb, wGameModeFlags);
		if (game_over) {
			gb->cpu.a = TRANSITION_GAME_OVER;
			wr8(gb, wTransitionParam, gb->cpu.a);
		} else {
			gb->cpu.a = alu_dec(gb, rd8(gb, wNumCollectedTreasures));
			if (ZF) {
				gb->cpu.a = TRANSITION_EPILOGUE_PERFECT;
				wr8(gb, wTransitionParam, gb->cpu.a);
				flags |= 1 << MODE_TIME_ATTACK_F;
				wr8(gb, wGameModeFlags, flags);
			} else {
				gb->cpu.a = TRANSITION_EPILOGUE_NOT_PERFECT;
				wr8(gb, wTransitionParam, gb->cpu.a);
			}
			flags = rd8(gb, wGameModeFlags);
			flags |= 1 << MODE_GAME_CLEARED_F;
			wr8(gb, wGameModeFlags, flags);
		}
		flags = rd8(gb, wGameModeFlags);
		flags |= 1 << MODE_FOUGHT_A_HIDDEN_FIGURE_F;
		wr8(gb, wGameModeFlags, flags);

		SET_HL(gb, wState);
		wr8(gb, wState, ST_EPILOGUE);
		xor_a(gb);
		wr8(gb, wSubState, 0);
		return;
	}

	// .no_end_screen
	gb->cpu.a = rd8(gb, wMultiBlockParam);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!ZF) {
		// set next frame to process multi-block
		gb->cpu.a = rd8(gb, wSubState);
		wr8(gb, wPendingSubState, gb->cpu.a);
		gb->cpu.a = SST_LEVEL_MULTI_BLOCK;
		wr8(gb, wSubState, gb->cpu.a);
		return;
	}

	// .tick_time
	gb_call_ret(gb, ADDR_TickLevelTime, UL(0x231));

	gb->cpu.a = rd8(gb, wFloorTransitionDir);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!ZF)
		return;
	gb->cpu.a = rd8(gb, wRoomTransitionParam);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!ZF)
		return;

	gb->cpu.a = alu_and(gb, rd8(gb, wJoypadPressed), PAD_SELECT | PAD_START);
	if (ZF)
		return;
	bool pause = false;
	gb->cpu.a = rd8(gb, wTransformation);
	alu_cp(gb, gb->cpu.a, TRANSFORMATION_BLIND);
	if (!ZF) {
		gb->cpu.a = rd8(gb, wIsBossBattle);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (ZF) {
			gb->cpu.a = rd8(gb, wTransformation);
			alu_cp(gb, gb->cpu.a, TRANSFORMATION_BLIND);
			pause = !ZF; // this jump will always happen
		}
	}
	if (!pause) {
		play_sfx(gb, SFX_0E5);
		return;
	}

	// .pause
	play_sfx(gb, SFX_0E4);
	// stop_music
	gb->cpu.a = 0xff;
	wr8(gb, hMusicID + 0, gb->cpu.a);
	gb->cpu.a = 0x00;
	wr8(gb, hMusicID + 1, gb->cpu.a);
	xor_a(gb);
	wr8(gb, wRoomAnimatedPalsEnabled, 0);
	gb->cpu.a = TRUE;
	wr8(gb, wUnused_IsPaused, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSubState);
	wr8(gb, wPendingSubState, gb->cpu.a);
	gb->cpu.a = ST_PAUSE_MENU;
	wr8(gb, wState, gb->cpu.a);
	xor_a(gb);
	wr8(gb, wSubState, 0);
}

// .asm_8229: the SRAM1 block processing, then .check_end_screen.
static void process_blocks(GB *gb)
{
	gb->cpu.a = rd8(gb, wSRAMBank);
	push_af(gb);
	gb->cpu.a = BANK_sLevelBlockObjectMap; // BANK("SRAM1")
	sramswitch(gb, gb->cpu.a);
	gb_call_ret(gb, ADDR_Func_8ed9, UL(0x18e));
	gb_call_ret(gb, ADDR_Func_bb85, UL(0x191));
	gb->cpu.ime = false; // di
	gb_call_ret(gb, ADDR_SetTileQueueProcessingFunctions, UL(0x195));
	gb->cpu.ime = true; // ei
	gb->cpu.ime_delay = 0;
	pop_af(gb);
	sramswitch(gb, gb->cpu.a);
	check_end_screen(gb);
}

PORT(UpdateLevel)
{
	gb->cpu.a = TRUE;
	wr8(gb, wRoomAnimatedPalsEnabled, gb->cpu.a);

	FARCALL_RET(UpdateParticles, UL(0x14));

	gb->cpu.a = alu_and(gb, rd8(gb, wCameraConfigFlags), CAM_SCROLLING_MASK);
	alu_cp(gb, gb->cpu.a, CAM_FREE);
	if (ZF) {
		cam_path(gb, &free_camera);
		process_blocks(gb);
		return;
	}
	alu_cp(gb, gb->cpu.a, CAM_YSCROLL);
	if (ZF) {
		cam_path(gb, &yscroll_camera);
		process_blocks(gb);
		return;
	}

	// if doing floor transition, don't update Wario state
	gb->cpu.a = rd8(gb, wFloorTransitionDir);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (ZF) {
		wario_flag(gb, true);
		FARCALL_RET(UpdateWarioStates, UL(0x3d));
	}

	// .skip_update_wario_state
	gb->cpu.a = rd8(gb, wCameraConfigFlags);
	alu_cp(gb, gb->cpu.a, HIDDEN_FIGURE_CAMCONFIG);
	if (ZF) {
		// .shake_override
		SET_HL(gb, wSCYShake);
		gb->cpu.a = alu_add(gb, rd8(gb, wHiddenFigureSCYShake), rd8(gb, wSCYShake));
		wr8(gb, wSCY, gb->cpu.a);
		gb->cpu.a = rd8(gb, wc08b);
		wr8(gb, wSCX, gb->cpu.a);
		gb->cpu.a = alu_add(gb, rd8(gb, wc08d), rd8(gb, wSCYShake));
		wr8(gb, rWY, gb->cpu.a);
	} else {
		set_scroll_from_camera(gb);
	}

	// .applied_cam_scrolling
	wario_flag(gb, false);

	// update objects
	push_wram(gb);
	FARCALL_RET(UpdateObjects, UL(0x82));
	pop_wram(gb);

	gb_call_ret(gb, ADDR_SetWarioScreenPos, UL(0x88));

	// if we are in Hidden Figure fight and wAutoMoveState == $02,
	// then set state to Fence Shake Sliding
	gb->cpu.a = rd8(gb, wLevel);
	alu_cp(gb, gb->cpu.a, THE_TEMPLE);
	if (ZF) {
		gb->cpu.a = rd8(gb, wAutoMoveState);
		alu_cp(gb, gb->cpu.a, 0x02);
		if (ZF)
			FARCALL_RET(SetState_FenceShakeSliding, UL(0xa5));
	}

	// .process_interactions
	push_wram(gb);
	wario_flag(gb, true);
	FARCALL_RET(ProcessInteractions, UL(0xc0));
	wario_flag(gb, false);
	pop_wram(gb);

	push_wram(gb);
	FARCALL_RET(DrawObjects_NoPriority, UL(0xdd));
	pop_wram(gb);

	gb_call_ret(gb, ADDR_DrawWario, UL(0xe3));

	push_wram(gb);
	FARCALL_RET(DrawObjects_WithPriority, UL(0xf9));
	pop_wram(gb);

	gb_call_ret(gb, ADDR_ClearUnusedVirtualOAM, UL(0xff));

	bool floor = false;
	gb->cpu.a = alu_and(gb, rd8(gb, wFloorTransitionDir), FLOOR_TRANSITION_DOWN | FLOOR_TRANSITION_UP);
	if (!ZF) {
		gb->cpu.a = rd8(gb, wIsDMATransferPending);
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		floor = ZF;
	}
	if (!floor) {
		// .no_floor_transition
		gb->cpu.a = rd8(gb, wCameraConfigFlags);
		alu_cp(gb, gb->cpu.a, HIDDEN_FIGURE_CAMCONFIG);
		if (ZF) {
			check_end_screen(gb);
			return;
		}
		gb->cpu.a = alu_and(gb, gb->cpu.a, CAM_SCROLLING_MASK);
		alu_cp(gb, gb->cpu.a, CAM_XSCROLL2 | CAM_TRANSITIONS);
		if (!ZF)
			gb_call_ret(gb, ADDR_LevelScroll_Horizontal, UL(0x17b));
		process_blocks(gb);
		return;
	}

	xor_a(gb);
	wr8(gb, wCameraYDelta, 0);
	gb->cpu.a = rd8(gb, wWarioScreenYPos);
	wr8(gb, wPrevWarioScreenYPos, gb->cpu.a);

	gb->cpu.a = rd8(gb, wFloorTransitionDir);
	if (!alu_bit(gb, gb->cpu.a, FLOOR_TRANSITION_UP_F)) {
		// transition down
		gb->cpu.b = 4;
		gb->cpu.a = alu_sub(gb, rd8(gb, wCameraYDelta), gb->cpu.b);
		wr8(gb, wCameraYDelta, gb->cpu.a);
		gb->cpu.a = alu_sub(gb, rd8(gb, wFloorTransitionTimer), gb->cpu.b);
		wr8(gb, wFloorTransitionTimer, gb->cpu.a);
		if (ZF) {
			SET_HL(gb, wFloorTransitionDir);
			wr8(gb, wFloorTransitionDir,
				rd8(gb, wFloorTransitionDir) & (uint8_t)~(1 << FLOOR_TRANSITION_DOWN_F));
			xor_a(gb);
			wr8(gb, wIsFloorTransition, 0);
		}
	} else {
		// .transition_up
		gb->cpu.b = 4;
		gb->cpu.a = alu_add(gb, rd8(gb, wCameraYDelta), gb->cpu.b);
		wr8(gb, wCameraYDelta, gb->cpu.a);
		gb->cpu.a = alu_sub(gb, rd8(gb, wFloorTransitionTimer), gb->cpu.b);
		wr8(gb, wFloorTransitionTimer, gb->cpu.a);
		bool end = ZF;
		if (!end) {
			gb->cpu.a = alu_dec(gb, rd8(gb, wScrollDownEdge));
			if (ZF) {
				xor_a(gb);
				wr8(gb, wFloorTransitionTimer, 0);
				end = true;
			}
		}
		if (end) {
			// .end_up_transition
			SET_HL(gb, wFloorTransitionDir);
			wr8(gb, wFloorTransitionDir,
				rd8(gb, wFloorTransitionDir) & (uint8_t)~(1 << FLOOR_TRANSITION_UP_F));
			xor_a(gb);
			wr8(gb, wIsFloorTransition, 0);
		}
	}

	// .asm_8209
	xor_a(gb);
	wr8(gb, wScrollUpEdge, 0);
	wr8(gb, wScrollDownEdge, 0);
	gb_call_ret(gb, ADDR_LevelScroll_Vertical, UL(0x169));
	process_blocks(gb);
}
