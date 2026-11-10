// A level's frame (src/engine/level/update_level.asm): particles, Wario's
// state, the objects, their interactions and drawing, the camera, the
// edges and blocks to draw, the floor transitions; then the end screens,
// multi-blocks, the clock and pausing.
//
// Plain C++ between its calls. Each call pushes the asm's own return
// address (UpdateLevel + offset), as the block queue code can read its
// callers' return addresses off the stack.
#include "../native.hpp"
#include "../routines.hpp"
#include "frame.hpp"

#include <initializer_list>

namespace wl3::level {

namespace {

constexpr uint16_t UL(uint16_t offset) { return static_cast<uint16_t>(ADDR_UpdateLevel + offset); }

// the return addresses of the .free_camera / .yscroll_camera paths' calls
struct CamPath {
	uint16_t wario, objects, screen_pos, interactions, no_prio, draw_wario, with_prio, clear_oam, vertical,
		horizontal;
};

constexpr CamPath FREE_CAMERA = {0x29b, 0x2c1, 0x2c7, 0x2db, 0x2f5, 0x2fb, 0x311, 0x317, 0x321, 0x324};
constexpr CamPath YSCROLL_CAMERA = {0x33b, 0x361, 0x367, 0x37b, 0x395, 0x39b, 0x3b1, 0x3b7, 0x3c1, 0};

class LevelFrame {
public:
	explicit LevelFrame(GB *gb) : gb_(gb), cpu_(gb->cpu), mem_(gb), routines_(gb) {}

	void update(); // UpdateLevel

private:
	void sram_switch(uint8_t bank)
	{
		mem_[wSRAMBank] = bank;
		mem_[rRAMB + 0x100] = bank;
	}
	// fn with the Level Objects WRAM bank mapped (push_wram / pop_wram)
	template <class F>
	void in_objects_wram(F fn)
	{
		uint8_t wbk = mem_[rWBK];
		mem_[rWBK] = BANK_wObjects;
		fn();
		mem_[rWBK] = wbk;
	}
	void wario_flag(bool on) { mem_[wBlockFuncWarioFlag] = on ? TRUE : FALSE; }
	void scroll_from_camera()
	{
		mem_[wSCY] = mem_[wCameraSCY + 1];
		mem_[wSCX] = mem_[wCameraSCX + 1];
	}
	void cam_path(const CamPath &p);
	void process_blocks(); // .asm_8229
	void check_end_screen(); // .check_end_screen
	void end_level();
	void pause();

	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
};

// the frame of the free and y-scrolling cameras
void LevelFrame::cam_path(const CamPath &p)
{
	wario_flag(true);
	routines_.in_bank(BANK_UpdateWarioStates, ADDR_UpdateWarioStates, [&] { update_wario_states(gb_); });
	scroll_from_camera();
	wario_flag(false);
	in_objects_wram([&] { routines_.in_bank(BANK_UpdateObjects, ADDR_UpdateObjects, [&] { update_objects(gb_); }); });
	set_wario_screen_pos(gb_);
	wario_flag(true);
	routines_.in_bank(BANK_ProcessInteractions, ADDR_ProcessInteractions, [&] { process_interactions(gb_); });
	wario_flag(false);
	in_objects_wram([&] { routines_.in_bank(BANK_DrawObjects_NoPriority, ADDR_DrawObjects_NoPriority, [&] { draw_objects_no_priority(gb_); }); });
	home::draw_wario(gb_);
	in_objects_wram([&] { routines_.in_bank(BANK_DrawObjects_WithPriority, ADDR_DrawObjects_WithPriority, [&] { draw_objects_with_priority(gb_); }); });
	home::clear_unused_virtual_oam(gb_);
	mem_[wScrollUpEdge] = 0;
	mem_[wScrollDownEdge] = 0;
	level_scroll_vertical(gb_);
	if (p.horizontal)
		level_scroll_horizontal(gb_);
}

// the level's end screens: game over, the epilogue, or the clear screen
void LevelFrame::end_level()
{
	mem_[wResetDisabled] = TRUE;
	for (uint16_t addr : {wGroundShakeCounter, wSCYShake, wAnimatedTilesFrameDuration, wRoomPalCycleDuration,
		     wRoomAnimatedTilesEnabled, wRoomAnimatedPalsEnabled, wWaterCurrent, wLastWaterCurrent, wCurWaterCurrent,
		     wSwimmingDirectionInput})
		mem_[addr] = 0;

	uint8_t end = mem_[wLevelEndScreen];
	if (end != LVLEND_GAME_OVER && end != LVLEND_EPILOGUE) { // the clear screen
		mem_[wState] = static_cast<uint8_t>(mem_[wState] + 1); // ST_CLEAR
		mem_[wSubState] = (mem_[wGameModeFlags] & (1 << MODE_TIME_ATTACK_F)) ? SST_CLEAR_TIME_ATTACK : 0;
		return;
	}

	Byte modes = mem_[wGameModeFlags];
	if (end == LVLEND_GAME_OVER) {
		mem_[wTransitionParam] = TRANSITION_GAME_OVER;
	} else {
		if (mem_[wNumCollectedTreasures] == 1) {
			mem_[wTransitionParam] = TRANSITION_EPILOGUE_PERFECT;
			modes |= 1 << MODE_TIME_ATTACK_F;
		} else {
			mem_[wTransitionParam] = TRANSITION_EPILOGUE_NOT_PERFECT;
		}
		modes |= 1 << MODE_GAME_CLEARED_F;
	}
	modes |= 1 << MODE_FOUGHT_A_HIDDEN_FIGURE_F;
	mem_[wState] = ST_EPILOGUE;
	mem_[wSubState] = 0;
}

// SELECT or START: pauses (or just a sound, when blind or in a boss battle)
void LevelFrame::pause()
{
	if (mem_[wTransformation] == TRANSFORMATION_BLIND || mem_[wIsBossBattle]) {
		play_sfx(mem_, SFX_0E5);
		return;
	}
	play_sfx(mem_, SFX_0E4);
	stop_music(mem_);
	mem_[wRoomAnimatedPalsEnabled] = 0;
	mem_[wUnused_IsPaused] = TRUE;
	mem_[wPendingSubState] = mem_[wSubState];
	mem_[wState] = ST_PAUSE_MENU;
	mem_[wSubState] = 0;
}

// the end screens, multi-blocks, the clock and pausing
void LevelFrame::check_end_screen()
{
	if (mem_[wState] != ST_LEVEL)
		return;
	if (uint8_t end = mem_[wLevelEndScreen]) {
		if (!(end & 0x80))
			end_level();
		return;
	}

	// .no_end_screen
	if (mem_[wMultiBlockParam]) { // the multi-block next frame
		mem_[wPendingSubState] = mem_[wSubState];
		mem_[wSubState] = SST_LEVEL_MULTI_BLOCK;
		return;
	}

	// .tick_time
	tick_level_time(gb_);
	if (mem_[wFloorTransitionDir] || mem_[wRoomTransitionParam])
		return;
	if (mem_[wJoypadPressed] & (PAD_SELECT | PAD_START))
		pause();
}

// .asm_8229: the edges and changed blocks into the queues (with SRAM1
// mapped), the queue's V-Blank code, then .check_end_screen
void LevelFrame::process_blocks()
{
	uint8_t sram = mem_[wSRAMBank];
	sram_switch(BANK_sLevelBlockObjectMap); // BANK("SRAM1")
	queue_scrolled_edge(gb_);
	queue_changed_block(gb_);
	cpu_.ime = false; // di
	set_tile_queue_processing_functions(gb_);
	cpu_.ime = true; // ei
	cpu_.ime_delay = 0;
	sram_switch(sram);
	check_end_screen();
}

void LevelFrame::update()
{
	mem_[wRoomAnimatedPalsEnabled] = TRUE;
	routines_.in_bank(BANK_UpdateParticles, ADDR_UpdateParticles, [&] { update_particles(gb_); });

	uint8_t mode = mem_[wCameraConfigFlags] & CAM_SCROLLING_MASK;
	if (mode == CAM_FREE || mode == CAM_YSCROLL) {
		cam_path(mode == CAM_FREE ? FREE_CAMERA : YSCROLL_CAMERA);
		process_blocks();
		return;
	}

	// no Wario state update during a floor transition
	if (!mem_[wFloorTransitionDir]) {
		wario_flag(true);
		routines_.in_bank(BANK_UpdateWarioStates, ADDR_UpdateWarioStates, [&] { update_wario_states(gb_); });
	}

	// .skip_update_wario_state
	if (mem_[wCameraConfigFlags] == HIDDEN_FIGURE_CAMCONFIG) { // .shake_override
		uint8_t shake = mem_[wSCYShake];
		mem_[wSCY] = static_cast<uint8_t>(mem_[wHiddenFigureSCYShake] + shake);
		mem_[wSCX] = mem_[wc08b];
		mem_[rWY] = static_cast<uint8_t>(mem_[wc08d] + shake);
	} else {
		scroll_from_camera();
	}

	// .applied_cam_scrolling
	wario_flag(false);
	in_objects_wram([&] { routines_.in_bank(BANK_UpdateObjects, ADDR_UpdateObjects, [&] { update_objects(gb_); }); });
	set_wario_screen_pos(gb_);

	// in the Hidden Figure's battle, wAutoMoveState $02 starts the fence
	// shake sliding
	if (mem_[wLevel] == THE_TEMPLE && mem_[wAutoMoveState] == 0x02)
		routines_.farcall(BANK_SetState_FenceShakeSliding, ADDR_SetState_FenceShakeSliding);

	// .process_interactions
	in_objects_wram([&] {
		wario_flag(true);
		routines_.in_bank(BANK_ProcessInteractions, ADDR_ProcessInteractions, [&] { process_interactions(gb_); });
		wario_flag(false);
	});
	in_objects_wram([&] { routines_.in_bank(BANK_DrawObjects_NoPriority, ADDR_DrawObjects_NoPriority, [&] { draw_objects_no_priority(gb_); }); });
	home::draw_wario(gb_);
	in_objects_wram([&] { routines_.in_bank(BANK_DrawObjects_WithPriority, ADDR_DrawObjects_WithPriority, [&] { draw_objects_with_priority(gb_); }); });
	home::clear_unused_virtual_oam(gb_);

	bool floor = (mem_[wFloorTransitionDir] & (FLOOR_TRANSITION_DOWN | FLOOR_TRANSITION_UP)) && !mem_[wIsDMATransferPending];
	if (!floor) { // .no_floor_transition
		uint8_t config = mem_[wCameraConfigFlags];
		if (config == HIDDEN_FIGURE_CAMCONFIG) {
			check_end_screen();
			return;
		}
		if ((config & CAM_SCROLLING_MASK) != (CAM_XSCROLL2 | CAM_TRANSITIONS))
			level_scroll_horizontal(gb_);
		process_blocks();
		return;
	}

	// a floor transition: the camera moves 4 pixels a frame
	mem_[wCameraYDelta] = 0;
	mem_[wPrevWarioScreenYPos] = mem_[wWarioScreenYPos];
	uint8_t timer = static_cast<uint8_t>(mem_[wFloorTransitionTimer] - 4);
	if (!(mem_[wFloorTransitionDir] & (1 << FLOOR_TRANSITION_UP_F))) { // down
		mem_[wCameraYDelta] = static_cast<uint8_t>(mem_[wCameraYDelta] - 4);
		mem_[wFloorTransitionTimer] = timer;
		if (!timer) {
			mem_[wFloorTransitionDir] = mem_[wFloorTransitionDir] & ~(1 << FLOOR_TRANSITION_DOWN_F);
			mem_[wIsFloorTransition] = 0;
		}
	} else { // .transition_up: until the timer runs out or the camera's at the bottom
		mem_[wCameraYDelta] = static_cast<uint8_t>(mem_[wCameraYDelta] + 4);
		mem_[wFloorTransitionTimer] = timer;
		bool end = !timer;
		if (!end && mem_[wScrollDownEdge] == 1) {
			mem_[wFloorTransitionTimer] = 0;
			end = true;
		}
		if (end) { // .end_up_transition
			mem_[wFloorTransitionDir] = mem_[wFloorTransitionDir] & ~(1 << FLOOR_TRANSITION_UP_F);
			mem_[wIsFloorTransition] = 0;
		}
	}

	// .asm_8209
	mem_[wScrollUpEdge] = 0;
	mem_[wScrollDownEdge] = 0;
	level_scroll_vertical(gb_);
	process_blocks();
}

} // namespace

} // namespace wl3::level

// Only _LevelStateTable's jumptable reaches it, from LevelStateTable, which
// sets a and f itself after, and whose callers read none of b to l
PORT_EX(UpdateLevel, 0, ALL_REGISTERS) { wl3::level::LevelFrame(gb).update(); }
