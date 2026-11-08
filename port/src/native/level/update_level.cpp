// A level's frame (src/engine/level/update_level.asm): particles, Wario's
// state, the objects, their interactions and drawing, the camera, the
// edges and blocks to draw, the floor transitions; then the end screens,
// multi-blocks, the clock and pausing.
//
// Each call pushes the asm's own return address (UpdateLevel + offset), as
// the block queue code can read its callers' return addresses off the
// stack.
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"

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
	bool zf() const { return cpu_.f & FLAG_Z; }
	void xor_a()
	{
		cpu_.a = 0;
		cpu_.f = FLAG_Z;
	}
	void sram_switch(uint8_t bank)
	{
		mem_[wSRAMBank] = bank;
		mem_[rRAMB + 0x100] = bank;
	}
	void push_af() { gb_push16(gb_, REG_AF(gb_)); }
	void pop_af()
	{
		uint16_t af = gb_pop16(gb_);
		cpu_.a = static_cast<uint8_t>(af >> 8);
		cpu_.f = static_cast<uint8_t>(af & 0xf0);
	}
	// push_wram BANK("Level Objects WRAM") / pop_wram
	void push_wram()
	{
		cpu_.a = mem_[rWBK];
		push_af();
		cpu_.a = BANK_wObjects;
		mem_[rWBK] = BANK_wObjects;
	}
	void pop_wram()
	{
		pop_af();
		mem_[rWBK] = cpu_.a;
	}
	void wario_flag(bool on)
	{
		if (on)
			cpu_.a = TRUE;
		else
			xor_a();
		mem_[wBlockFuncWarioFlag] = cpu_.a;
	}
	void scroll_from_camera()
	{
		cpu_.a = mem_[wCameraSCY + 1];
		mem_[wSCY] = cpu_.a;
		cpu_.a = mem_[wCameraSCX + 1];
		mem_[wSCX] = cpu_.a;
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
	routines_.farcall_ret(BANK_UpdateWarioStates, ADDR_UpdateWarioStates, UL(p.wario));
	scroll_from_camera();
	wario_flag(false);
	push_wram();
	routines_.farcall_ret(BANK_UpdateObjects, ADDR_UpdateObjects, UL(p.objects));
	pop_wram();
	routines_.call_ret(ADDR_SetWarioScreenPos, UL(p.screen_pos));
	wario_flag(true);
	routines_.farcall_ret(BANK_ProcessInteractions, ADDR_ProcessInteractions, UL(p.interactions));
	wario_flag(false);
	push_wram();
	routines_.farcall_ret(BANK_DrawObjects_NoPriority, ADDR_DrawObjects_NoPriority, UL(p.no_prio));
	pop_wram();
	routines_.call_ret(ADDR_DrawWario, UL(p.draw_wario));
	push_wram();
	routines_.farcall_ret(BANK_DrawObjects_WithPriority, ADDR_DrawObjects_WithPriority, UL(p.with_prio));
	pop_wram();
	routines_.call_ret(ADDR_ClearUnusedVirtualOAM, UL(p.clear_oam));
	xor_a();
	mem_[wScrollUpEdge] = 0;
	mem_[wScrollDownEdge] = 0;
	routines_.call_ret(ADDR_LevelScroll_Vertical, UL(p.vertical));
	if (p.horizontal)
		routines_.call_ret(ADDR_LevelScroll_Horizontal, UL(p.horizontal));
}

// the level's end screens: game over, the epilogue, or the clear screen
void LevelFrame::end_level()
{
	cpu_.a = TRUE;
	mem_[wResetDisabled] = TRUE;
	xor_a();
	for (uint16_t addr : {wGroundShakeCounter, wSCYShake, wAnimatedTilesFrameDuration, wRoomPalCycleDuration,
		     wRoomAnimatedTilesEnabled, wRoomAnimatedPalsEnabled, wWaterCurrent, wLastWaterCurrent, wCurWaterCurrent,
		     wSwimmingDirectionInput})
		mem_[addr] = 0;

	SET_HL(gb_, wGameModeFlags);
	uint8_t end = mem_[wLevelEndScreen];
	cpu_.a = end;
	cpu_.f = flags::cp(end, LVLEND_GAME_OVER);
	bool game_over = zf();
	bool epilogue = false;
	if (!game_over) {
		cpu_.f = flags::cp(end, LVLEND_EPILOGUE);
		epilogue = zf();
	}
	if (!game_over && !epilogue) { // the clear screen
		SET_HL(gb_, wState);
		uint8_t state = mem_[wState];
		cpu_.f = flags::inc(cpu_.f, state);
		mem_[wState] = static_cast<uint8_t>(state + 1); // ST_CLEAR
		xor_a();
		mem_[wSubState] = 0;
		cpu_.a = mem_[wGameModeFlags];
		cpu_.f = flags::bit(cpu_.f, cpu_.a, MODE_TIME_ATTACK_F);
		if (!(cpu_.a & (1 << MODE_TIME_ATTACK_F)))
			return;
		cpu_.a = SST_CLEAR_TIME_ATTACK;
		mem_[wSubState] = cpu_.a;
		return;
	}

	Byte modes = mem_[wGameModeFlags];
	if (game_over) {
		cpu_.a = TRANSITION_GAME_OVER;
		mem_[wTransitionParam] = cpu_.a;
	} else {
		uint8_t treasures = mem_[wNumCollectedTreasures];
		cpu_.f = flags::dec(cpu_.f, treasures);
		if (treasures == 1) {
			cpu_.a = TRANSITION_EPILOGUE_PERFECT;
			mem_[wTransitionParam] = cpu_.a;
			modes |= 1 << MODE_TIME_ATTACK_F;
		} else {
			cpu_.a = TRANSITION_EPILOGUE_NOT_PERFECT;
			mem_[wTransitionParam] = cpu_.a;
		}
		modes |= 1 << MODE_GAME_CLEARED_F;
	}
	modes |= 1 << MODE_FOUGHT_A_HIDDEN_FIGURE_F;
	SET_HL(gb_, wState);
	mem_[wState] = ST_EPILOGUE;
	xor_a();
	mem_[wSubState] = 0;
}

// SELECT or START: pauses (or just a sound, when blind or in a boss battle)
void LevelFrame::pause()
{
	bool can_pause = false;
	cpu_.a = mem_[wTransformation];
	cpu_.f = flags::cp(cpu_.a, TRANSFORMATION_BLIND);
	if (!zf()) {
		cpu_.a = mem_[wIsBossBattle];
		cpu_.f = flags::and_(cpu_.a);
		if (zf()) {
			cpu_.a = mem_[wTransformation];
			cpu_.f = flags::cp(cpu_.a, TRANSFORMATION_BLIND);
			can_pause = !zf(); // always, here
		}
	}
	if (!can_pause) {
		play_sfx(mem_, SFX_0E5);
		cpu_.a = static_cast<uint8_t>(SFX_0E5);
		return;
	}
	play_sfx(mem_, SFX_0E4);
	stop_music(mem_);
	xor_a();
	mem_[wRoomAnimatedPalsEnabled] = 0;
	cpu_.a = TRUE;
	mem_[wUnused_IsPaused] = TRUE;
	cpu_.a = mem_[wSubState];
	mem_[wPendingSubState] = cpu_.a;
	cpu_.a = ST_PAUSE_MENU;
	mem_[wState] = ST_PAUSE_MENU;
	xor_a();
	mem_[wSubState] = 0;
}

// the end screens, multi-blocks, the clock and pausing
void LevelFrame::check_end_screen()
{
	cpu_.a = mem_[wState];
	cpu_.f = flags::cp(cpu_.a, ST_LEVEL);
	if (!zf())
		return;
	SET_HL(gb_, wLevelEndScreen);
	uint8_t end = mem_[wLevelEndScreen];
	cpu_.a = end;
	cpu_.f = flags::and_(end);
	if (end) {
		cpu_.f = flags::bit(cpu_.f, end, 7);
		if (end & 0x80)
			return;
		end_level();
		return;
	}

	// .no_end_screen
	cpu_.a = mem_[wMultiBlockParam];
	cpu_.f = flags::and_(cpu_.a);
	if (cpu_.a) { // the multi-block next frame
		cpu_.a = mem_[wSubState];
		mem_[wPendingSubState] = cpu_.a;
		cpu_.a = SST_LEVEL_MULTI_BLOCK;
		mem_[wSubState] = cpu_.a;
		return;
	}

	// .tick_time
	routines_.call_ret(ADDR_TickLevelTime, UL(0x231));

	cpu_.a = mem_[wFloorTransitionDir];
	cpu_.f = flags::and_(cpu_.a);
	if (cpu_.a)
		return;
	cpu_.a = mem_[wRoomTransitionParam];
	cpu_.f = flags::and_(cpu_.a);
	if (cpu_.a)
		return;
	cpu_.a = mem_[wJoypadPressed] & (PAD_SELECT | PAD_START);
	cpu_.f = flags::and_(cpu_.a);
	if (!cpu_.a)
		return;
	pause();
}

// .asm_8229: the edges and changed blocks into the queues (with SRAM1
// mapped), the queue's V-Blank code, then .check_end_screen
void LevelFrame::process_blocks()
{
	cpu_.a = mem_[wSRAMBank];
	push_af();
	cpu_.a = BANK_sLevelBlockObjectMap; // BANK("SRAM1")
	sram_switch(cpu_.a);
	routines_.call_ret(ADDR_Func_8ed9, UL(0x18e));
	routines_.call_ret(ADDR_Func_bb85, UL(0x191));
	cpu_.ime = false; // di
	routines_.call_ret(ADDR_SetTileQueueProcessingFunctions, UL(0x195));
	cpu_.ime = true; // ei
	cpu_.ime_delay = 0;
	pop_af();
	sram_switch(cpu_.a);
	check_end_screen();
}

void LevelFrame::update()
{
	cpu_.a = TRUE;
	mem_[wRoomAnimatedPalsEnabled] = TRUE;
	routines_.farcall_ret(BANK_UpdateParticles, ADDR_UpdateParticles, UL(0x14));

	cpu_.a = mem_[wCameraConfigFlags] & CAM_SCROLLING_MASK;
	cpu_.f = flags::cp(cpu_.a, CAM_FREE);
	if (zf()) {
		cam_path(FREE_CAMERA);
		process_blocks();
		return;
	}
	cpu_.f = flags::cp(cpu_.a, CAM_YSCROLL);
	if (zf()) {
		cam_path(YSCROLL_CAMERA);
		process_blocks();
		return;
	}

	// no Wario state update during a floor transition
	cpu_.a = mem_[wFloorTransitionDir];
	cpu_.f = flags::and_(cpu_.a);
	if (!cpu_.a) {
		wario_flag(true);
		routines_.farcall_ret(BANK_UpdateWarioStates, ADDR_UpdateWarioStates, UL(0x3d));
	}

	// .skip_update_wario_state
	cpu_.a = mem_[wCameraConfigFlags];
	cpu_.f = flags::cp(cpu_.a, HIDDEN_FIGURE_CAMCONFIG);
	if (zf()) { // .shake_override
		SET_HL(gb_, wSCYShake);
		uint8_t shake = mem_[wSCYShake];
		mem_[wSCY] = static_cast<uint8_t>(mem_[wHiddenFigureSCYShake] + shake);
		mem_[wSCX] = mem_[wc08b];
		uint8_t wy = mem_[wc08d];
		cpu_.f = flags::add(wy, shake);
		cpu_.a = static_cast<uint8_t>(wy + shake);
		mem_[rWY] = cpu_.a;
	} else {
		scroll_from_camera();
	}

	// .applied_cam_scrolling
	wario_flag(false);
	push_wram();
	routines_.farcall_ret(BANK_UpdateObjects, ADDR_UpdateObjects, UL(0x82));
	pop_wram();
	routines_.call_ret(ADDR_SetWarioScreenPos, UL(0x88));

	// in the Hidden Figure's battle, wAutoMoveState $02 starts the fence
	// shake sliding
	cpu_.a = mem_[wLevel];
	cpu_.f = flags::cp(cpu_.a, THE_TEMPLE);
	if (zf()) {
		cpu_.a = mem_[wAutoMoveState];
		cpu_.f = flags::cp(cpu_.a, 0x02);
		if (zf())
			routines_.farcall_ret(BANK_SetState_FenceShakeSliding, ADDR_SetState_FenceShakeSliding, UL(0xa5));
	}

	// .process_interactions
	push_wram();
	wario_flag(true);
	routines_.farcall_ret(BANK_ProcessInteractions, ADDR_ProcessInteractions, UL(0xc0));
	wario_flag(false);
	pop_wram();

	push_wram();
	routines_.farcall_ret(BANK_DrawObjects_NoPriority, ADDR_DrawObjects_NoPriority, UL(0xdd));
	pop_wram();
	routines_.call_ret(ADDR_DrawWario, UL(0xe3));
	push_wram();
	routines_.farcall_ret(BANK_DrawObjects_WithPriority, ADDR_DrawObjects_WithPriority, UL(0xf9));
	pop_wram();
	routines_.call_ret(ADDR_ClearUnusedVirtualOAM, UL(0xff));

	bool floor = false;
	cpu_.a = mem_[wFloorTransitionDir] & (FLOOR_TRANSITION_DOWN | FLOOR_TRANSITION_UP);
	cpu_.f = flags::and_(cpu_.a);
	if (cpu_.a) {
		cpu_.a = mem_[wIsDMATransferPending];
		cpu_.f = flags::and_(cpu_.a);
		floor = !cpu_.a;
	}
	if (!floor) { // .no_floor_transition
		cpu_.a = mem_[wCameraConfigFlags];
		cpu_.f = flags::cp(cpu_.a, HIDDEN_FIGURE_CAMCONFIG);
		if (zf()) {
			check_end_screen();
			return;
		}
		cpu_.a &= CAM_SCROLLING_MASK;
		cpu_.f = flags::cp(cpu_.a, CAM_XSCROLL2 | CAM_TRANSITIONS);
		if (!zf())
			routines_.call_ret(ADDR_LevelScroll_Horizontal, UL(0x17b));
		process_blocks();
		return;
	}

	// a floor transition: the camera moves 4 pixels a frame
	xor_a();
	mem_[wCameraYDelta] = 0;
	cpu_.a = mem_[wWarioScreenYPos];
	mem_[wPrevWarioScreenYPos] = cpu_.a;
	cpu_.a = mem_[wFloorTransitionDir];
	cpu_.b = 4;
	uint8_t timer = mem_[wFloorTransitionTimer];
	if (!(cpu_.a & (1 << FLOOR_TRANSITION_UP_F))) { // down
		mem_[wCameraYDelta] = static_cast<uint8_t>(mem_[wCameraYDelta] - 4);
		cpu_.f = flags::sub(timer, 4);
		cpu_.a = static_cast<uint8_t>(timer - 4);
		mem_[wFloorTransitionTimer] = cpu_.a;
		if (zf()) {
			SET_HL(gb_, wFloorTransitionDir);
			mem_[wFloorTransitionDir] &= ~(1 << FLOOR_TRANSITION_DOWN_F);
			xor_a();
			mem_[wIsFloorTransition] = 0;
		}
	} else { // .transition_up
		mem_[wCameraYDelta] = static_cast<uint8_t>(mem_[wCameraYDelta] + 4);
		cpu_.f = flags::sub(timer, 4);
		cpu_.a = static_cast<uint8_t>(timer - 4);
		mem_[wFloorTransitionTimer] = cpu_.a;
		bool end = zf();
		if (!end) {
			uint8_t edge = mem_[wScrollDownEdge];
			cpu_.f = flags::dec(cpu_.f, edge);
			cpu_.a = static_cast<uint8_t>(edge - 1);
			if (zf()) {
				xor_a();
				mem_[wFloorTransitionTimer] = 0;
				end = true;
			}
		}
		if (end) { // .end_up_transition
			SET_HL(gb_, wFloorTransitionDir);
			mem_[wFloorTransitionDir] &= ~(1 << FLOOR_TRANSITION_UP_F);
			xor_a();
			mem_[wIsFloorTransition] = 0;
		}
	}

	// .asm_8209
	xor_a();
	mem_[wScrollUpEdge] = 0;
	mem_[wScrollDownEdge] = 0;
	routines_.call_ret(ADDR_LevelScroll_Vertical, UL(0x169));
	process_blocks();
}

} // namespace

} // namespace wl3::level

// Only _LevelStateTable's jumptable reaches it, from LevelStateTable,
// whose callers read none of b to l
PORT_EX(UpdateLevel, 0, R_B | R_C | R_D | R_E | R_H | R_L) { wl3::level::LevelFrame(gb).update(); }
