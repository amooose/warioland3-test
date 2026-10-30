// Port of src/engine/golf/lobby.asm
#include "ported.h"

PORT_JUMP(VBlank_1c84e0)
{
	SET_HL(gb, ADDR_VBlank_1c84e0 + 0x0b); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 0x0e; // .end - .Func
	JUMP(CopyHLToDE);
}

// rst JumpTable, with the table at `table` (right after the rst). Leaves
// every register as JumpTable does and returns the target (= hl).
static uint16_t jumptable(GB *gb, uint16_t table)
{
	gb_push16(gb, table); // rst JumpTable
	uint16_t hl = gb_pop16(gb); // pop hl
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, hl, gb->cpu.e);
	gb->cpu.a = rd8(gb, hl);
	gb->cpu.h = rd8(gb, (uint16_t)(hl + 1));
	gb->cpu.l = gb->cpu.a;
	return REG_HL(gb);
}

static void xor_a(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
}

// ld a, state / ld [wGolfWarioState], a / ld a, $01 / ld [wGolfVBlankMode], a
static void set_wario_state(GB *gb, uint8_t state)
{
	gb->cpu.a = state;
	wr8(gb, wGolfWarioState, state);
	gb->cpu.a = 0x01;
	wr8(gb, wGolfVBlankMode, 0x01);
}

// xor a / ld [wGolfCounter], a / ld [wGolfWarioDuration], a /
// ld [wGolfWarioFramesetOffset], a / ld [wGolfLobbyState], a
static void wait_input(GB *gb)
{
	xor_a(gb);
	wr8(gb, wGolfCounter, 0);
	wr8(gb, wGolfWarioDuration, 0);
	wr8(gb, wGolfWarioFramesetOffset, 0);
	wr8(gb, wGolfLobbyState, GOLFLOBBYSTATE_WAIT_INPUT);
}

static void waiting_input(GB *gb)
{
	gb->cpu.a = rd8(gb, wJoypadPressed);
	if (alu_bit(gb, gb->cpu.a, B_PAD_DOWN)) {
		// .d_down
		gb->cpu.a = rd8(gb, wGolfWarioXCoord);
		alu_cp(gb, gb->cpu.a, 0x78);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		gb->cpu.a = rd8(gb, wGolfObj4State);
		alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z)) {
			play_sfx(gb, SFX_0E5);
			gb->cpu.a = COININFO_SHOW_MESSAGE;
			wr8(gb, wGolfObj4State, gb->cpu.a);
			xor_a(gb);
			wr8(gb, wGolfObj4Duration, 0);
			wr8(gb, wGolfObj4FramesetOffset, 0);
			return;
		}
		// .enough_coins
		play_sfx(gb, SFX_PIPE_ENTER);
		gb->cpu.a = rd8(gb, wGolfPrice);
		gb->cpu.c = gb->cpu.a;
		gb->cpu.a = alu_sub(gb, rd8(gb, wNumCoins + 1), gb->cpu.c);
		gb->cpu.a = alu_daa(gb, gb->cpu.a);
		wr8(gb, wNumCoins + 1, gb->cpu.a);
		gb->cpu.a = alu_sbc(gb, rd8(gb, wNumCoins + 0), 0);
		wr8(gb, wNumCoins + 0, gb->cpu.a);

		set_wario_state(gb, GOLF_WARIO_GOING_IN_PIPE);
		gb->cpu.a = 0x80;
		wr8(gb, wGolfWarioXPos, 0x80);
		xor_a(gb);
		wr8(gb, wGolfCounter, 0);
		wr8(gb, wGolfWarioDuration, 0);
		wr8(gb, wGolfWarioFramesetOffset, 0);
		gb->cpu.a = GOLFLOBBYSTATE_ENTER_PIPE;
		wr8(gb, wGolfLobbyState, gb->cpu.a);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_UP)) {
		// .d_up
		gb->cpu.a = rd8(gb, wGolfWarioXCoord);
		alu_cp(gb, gb->cpu.a, 0x38);
		if (!(gb->cpu.f & FLAG_Z))
			return;
		play_sfx(gb, SFX_0E6);
		xor_a(gb);
		wr8(gb, wGolfWarioDir, 0);
		wr8(gb, wGolfWarioDuration, 0);
		wr8(gb, wGolfWarioFramesetOffset, 0);
		set_wario_state(gb, GOLF_WARIO_TURNING);
		gb->cpu.a = GOLFLOBBYSTATE_ENTER_DOOR;
		wr8(gb, wGolfLobbyState, gb->cpu.a);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_RIGHT)) {
		// .d_right
		gb->cpu.a = rd8(gb, wGolfWarioXCoord);
		alu_cp(gb, gb->cpu.a, 0x78);
		if (!(gb->cpu.f & FLAG_C))
			return;
		xor_a(gb);
		wr8(gb, wGolfWarioDir, 0);
		wr8(gb, wGolfWarioDuration, 0);
		wr8(gb, wGolfWarioFramesetOffset, 0);
		gb->cpu.a = GOLF_WARIO_TURNING;
		wr8(gb, wGolfWarioState, gb->cpu.a);
		gb->cpu.a = 4;
		wr8(gb, wGolfWarioYVel, 4);
		gb->cpu.a = 0x01; // aka GOLFLOBBYSTATE_WALK_RIGHT
		wr8(gb, wGolfVBlankMode, 0x01);
		wr8(gb, wGolfLobbyState, 0x01);
		return;
	}
	if (alu_bit(gb, gb->cpu.a, B_PAD_LEFT)) {
		// .d_left
		gb->cpu.a = rd8(gb, wGolfWarioXCoord);
		alu_cp(gb, gb->cpu.a, 0x78);
		if (gb->cpu.f & FLAG_C)
			return;
		xor_a(gb);
		wr8(gb, wGolfWarioDuration, 0);
		wr8(gb, wGolfWarioFramesetOffset, 0);
		set_wario_state(gb, GOLF_WARIO_TURNING);
		wr8(gb, wGolfWarioDir, 0x01);
		gb->cpu.a = GOLFLOBBYSTATE_WALK_LEFT;
		wr8(gb, wGolfLobbyState, gb->cpu.a);
	}
}

// .WalkingRight / .WalkingLeft, up to the walk: true if Wario walks
static bool turning(GB *gb, bool clear_dir)
{
	// check if still doing turning animation
	SET_HL(gb, wGolfCounter);
	gb->cpu.a = rd8(gb, wGolfCounter);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return false;
	alu_cp(gb, gb->cpu.a, 0x02);
	if (!(gb->cpu.f & FLAG_C))
		return true;
	// done turning, start walking
	wr8(gb, wGolfCounter, alu_inc(gb, rd8(gb, wGolfCounter)));
	xor_a(gb);
	if (clear_dir)
		wr8(gb, wGolfWarioDir, 0);
	wr8(gb, wGolfWarioDuration, 0);
	wr8(gb, wGolfWarioFramesetOffset, 0);
	set_wario_state(gb, GOLF_WARIO_WALKING);
	return false;
}

static void walking_right(GB *gb)
{
	if (!turning(gb, true))
		return;
	// .walk_right
	gb->cpu.a = rd8(gb, wGolfWarioXPos);
	alu_cp(gb, gb->cpu.a, 0x58);
	if (!(gb->cpu.f & FLAG_C)) {
		// .DoJumpRight
		SET_HL(gb, wGolfWarioYVel);
		gb->cpu.c = rd8(gb, wGolfWarioYVel);
		gb->cpu.a = alu_inc(gb, rd8(gb, wGolfCounter));
		wr8(gb, wGolfCounter, gb->cpu.a);
		gb->cpu.a = alu_and(gb, gb->cpu.a, 0x1);
		if (gb->cpu.f & FLAG_Z)
			wr8(gb, wGolfWarioYVel, alu_dec(gb, rd8(gb, wGolfWarioYVel))); // decrease y vel
		// .skip_decr_y_vel: apply y vel to position
		SET_DE(gb, wGolfWarioYPos);
		gb->cpu.a = alu_sub(gb, rd8(gb, wGolfWarioYPos), gb->cpu.c);
		wr8(gb, wGolfWarioYPos, gb->cpu.a);
		gb->cpu.a = gb->cpu.c;
		alu_cp(gb, gb->cpu.a, (uint8_t)-16);
		if (!(gb->cpu.f & FLAG_C)) {
			// cap y pos
			gb->cpu.a = rd8(gb, wGolfWarioYPos);
			alu_cp(gb, gb->cpu.a, 0x80);
			if (!(gb->cpu.f & FLAG_C)) {
				gb->cpu.a = 0x80;
				wr8(gb, wGolfWarioYPos, 0x80);
			}
		}
	}
	SET_HL(gb, wGolfWarioXPos);
	gb->cpu.a = rd8(gb, wGolfWarioXPos);
	alu_cp(gb, gb->cpu.a, 0x78);
	if (gb->cpu.f & FLAG_C) {
		wr8(gb, wGolfWarioXPos, alu_inc(gb, rd8(gb, wGolfWarioXPos)));
		return;
	}
	// .reached_pipe
	gb->cpu.a = 0x80;
	wr8(gb, wGolfWarioYPos, 0x80);
	gb->cpu.a = 0x78;
	wr8(gb, wGolfWarioXPos, 0x78);
	set_wario_state(gb, GOLF_WARIO_WAITING);
	wait_input(gb);
}

static void walking_left(GB *gb)
{
	if (!turning(gb, false))
		return;
	// .walk_left
	SET_DE(gb, wGolfWarioYPos);
	gb->cpu.a = rd8(gb, wGolfWarioYPos);
	alu_cp(gb, gb->cpu.a, SCREEN_HEIGHT_PX);
	if (gb->cpu.f & FLAG_C) {
		// .DoJumpLeft
		gb->cpu.a = rd8(gb, wGolfWarioXPos);
		alu_cp(gb, gb->cpu.a, 0x64);
		if (gb->cpu.f & FLAG_C) {
			gb->cpu.a = alu_add(gb, rd8(gb, wGolfWarioYPos), 0x02);
			wr8(gb, wGolfWarioYPos, gb->cpu.a);
		}
	}
	SET_HL(gb, wGolfWarioXPos);
	gb->cpu.a = rd8(gb, wGolfWarioXPos);
	alu_cp(gb, gb->cpu.a, 0x38);
	if (!(gb->cpu.f & FLAG_Z)) {
		wr8(gb, wGolfWarioXPos, alu_dec(gb, rd8(gb, wGolfWarioXPos)));
		return;
	}
	// .reached_door
	gb->cpu.a = SCREEN_HEIGHT_PX;
	wr8(gb, wGolfWarioYPos, SCREEN_HEIGHT_PX);
	gb->cpu.a = 0x38;
	wr8(gb, wGolfWarioXPos, 0x38);
	set_wario_state(gb, GOLF_WARIO_WAITING);
	wait_input(gb);
}

static void entering_pipe(GB *gb)
{
	SET_HL(gb, wGolfCounter);
	gb->cpu.a = rd8(gb, wGolfCounter);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		wr8(gb, wGolfCounter, alu_inc(gb, rd8(gb, wGolfCounter)));
		uint16_t hl = wGolfNumCoins + 1;
		gb->cpu.a = rd8(gb, wNumCoins + 1);
		wr8(gb, hl--, gb->cpu.a);
		gb->cpu.a = rd8(gb, wNumCoins + 0);
		wr8(gb, hl, gb->cpu.a);
	}
	// .asm_1c83ed
	SET_HL(gb, wGolfWarioYPos);
	gb->cpu.a = rd8(gb, wGolfWarioYPos);
	alu_cp(gb, gb->cpu.a, 0xb0);
	if (gb->cpu.f & FLAG_C) {
		wr8(gb, wGolfWarioYPos, alu_inc(gb, rd8(gb, wGolfWarioYPos)));
		return;
	}
	// .asm_1c83f7
	gb->cpu.a = SST_GOLF_HOLE;
	wr8(gb, wSubState, gb->cpu.a);
}

static void entering_door(GB *gb)
{
	SET_HL(gb, wGolfCounter);
	gb->cpu.a = rd8(gb, wGolfCounter);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		return;
	alu_cp(gb, gb->cpu.a, 1);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_1c840d
		wr8(gb, wGolfCounter, alu_inc(gb, rd8(gb, wGolfCounter)));
		xor_a(gb);
		wr8(gb, wGolfWarioDuration, 0);
		wr8(gb, wGolfWarioFramesetOffset, 0);
		set_wario_state(gb, GOLF_WARIO_ENTERING_DOOR);
		return;
	}
	alu_cp(gb, gb->cpu.a, 28);
	if (gb->cpu.f & FLAG_C) {
		wr8(gb, wGolfCounter, alu_inc(gb, rd8(gb, wGolfCounter)));
		return;
	}
	// .asm_1c8420
	xor_a(gb);
	wr8(gb, w1d800, 0);
	wr8(gb, wGolfMenuOption, 0);
	// stop_music2
	gb->cpu.a = 0xff;
	wr8(gb, hMusicIDBackup + 0, 0xff);
	wr8(gb, hMusicID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hMusicIDBackup + 1, 0x00);
	wr8(gb, hMusicID + 1, 0x00);
	gb->cpu.a = SST_GOLF_EXIT;
	wr8(gb, wSubState, gb->cpu.a);
}

static void exiting_pipe(GB *gb)
{
	SET_HL(gb, wGolfWarioYPos);
	gb->cpu.a = rd8(gb, wGolfWarioYPos);
	alu_cp(gb, gb->cpu.a, 0x80);
	if (!(gb->cpu.f & FLAG_Z)) {
		alu_cp(gb, gb->cpu.a, 0xa0);
		if (gb->cpu.f & FLAG_Z)
			play_sfx(gb, SFX_PIPE_ENTER);
		wr8(gb, wGolfWarioYPos, alu_dec(gb, rd8(gb, wGolfWarioYPos)));
		return;
	}
	// .asm_1c844f
	gb->cpu.a = GOLF_WARIO_WAITING;
	wr8(gb, wGolfWarioState, gb->cpu.a);
	gb->cpu.a = 0x78;
	wr8(gb, wGolfWarioXPos, 0x78);
	gb->cpu.a = 0x01;
	wr8(gb, wGolfVBlankMode, 0x01);
	wait_input(gb);
}

PORT_JUMP(HandleGolfLobbyStates)
{
	gb->cpu.a = rd8(gb, wGolfLobbyState);
	uint16_t target = jumptable(gb, ADDR_HandleGolfLobbyStates + 4);
	switch (target) {
	case ADDR_HandleGolfLobbyStates + 0x10: waiting_input(gb); break; // .WaitingInput
	case ADDR_HandleGolfLobbyStates + 0xeb: walking_right(gb); break; // .WalkingRight
	case ADDR_HandleGolfLobbyStates + 0x161: walking_left(gb); break; // .WalkingLeft
	case ADDR_HandleGolfLobbyStates + 0x1be: entering_pipe(gb); break; // .EnteringPipe
	case ADDR_HandleGolfLobbyStates + 0x1e1: entering_door(gb); break; // .EnteringDoor
	case ADDR_HandleGolfLobbyStates + 0x21d: exiting_pipe(gb); break; // .ExitingPipe
	default: JUMP_TO(target); return; // out of the table
	}
	RET();
}

// ld a, y / ld [obj YCoord], a / ld a, x / ld [obj XCoord], a /
// ld de, frameset / ld hl, obj Duration / call UpdateGolfObjectAnimation /
// ld a, [obj CurrentFrame] / ld [obj Frame], a / ld hl, obj Sprite
static void golf_obj(GB *gb, uint16_t ycoord, uint8_t y, uint16_t xcoord, uint8_t x,
	uint16_t frameset, uint16_t duration, uint16_t cur_frame, uint16_t frame, uint16_t sprite)
{
	gb->cpu.a = y;
	wr8(gb, ycoord, y);
	gb->cpu.a = x;
	wr8(gb, xcoord, x);
	SET_DE(gb, frameset);
	SET_HL(gb, duration);
	CALL(UpdateGolfObjectAnimation);
	gb->cpu.a = rd8(gb, cur_frame);
	wr8(gb, frame, gb->cpu.a);
	SET_HL(gb, sprite);
}
#define GOLF_OBJ(o, y, x, fs) golf_obj(gb, w##o##YCoord, (y), w##o##XCoord, (x), (fs), \
	w##o##Duration, w##o##CurrentFrame, w##o##Frame, w##o##Sprite)

PORT_JUMP(UpdateGolfLobbyAnimations)
{
	gb->cpu.a = (uint8_t)(ADDR_OAM_1cb2d3 >> 8);
	wr8(gb, wGolfOAMPtr + 0, gb->cpu.a);
	gb->cpu.a = (uint8_t)ADDR_OAM_1cb2d3;
	wr8(gb, wGolfOAMPtr + 1, gb->cpu.a);
	GOLF_OBJ(GolfObj1, 0x28, 0x24, ADDR_Frameset_1cb3b3);
	CALL(LoadGolfSprite);
	GOLF_OBJ(GolfObj3, 0x2d, 0x8c, ADDR_Frameset_1cb3c4);
	CALL(LoadGolfSprite);
	GOLF_OBJ(GolfObj5, 0x60, 0x28, ADDR_Frameset_1cb3d7);
	CALL(LoadGolfSprite);

	gb->cpu.a = rd8(gb, wGolfObj4State);
	alu_cp(gb, gb->cpu.a, COININFO_SHOW_MESSAGE);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.a = 0x50;
	wr8(gb, wGolfObj4YCoord, 0x50);
	gb->cpu.a = 0x38;
	wr8(gb, wGolfObj4XCoord, 0x38);
	gb->cpu.a = rd8(gb, wLanguage);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		SET_DE(gb, ADDR_Frameset_1cb3d4);
	else
		SET_DE(gb, ADDR_Frameset_1cb3d1); // .english
	// .asm_1c81d4
	SET_HL(gb, wGolfObj4Duration);
	CALL(UpdateGolfObjectAnimation);
	gb->cpu.a = gb->cpu.c;
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = COININFO_NOT_ENOUGH_COINS;
		wr8(gb, wGolfObj4State, gb->cpu.a);
		RET();
		return;
	}
	// .asm_1c81e4
	gb->cpu.a = rd8(gb, wGolfObj4CurrentFrame);
	wr8(gb, wGolfObj4Frame, gb->cpu.a);
	SET_HL(gb, wGolfObj4Sprite);
	JUMP(LoadGolfSprite);
}

PORT(HandleGolfLobbyWarioSprite)
{
	gb->cpu.a = rd8(gb, wGolfWarioYPos);
	wr8(gb, wGolfWarioYCoord, gb->cpu.a);
	gb->cpu.a = rd8(gb, wGolfWarioXPos);
	wr8(gb, wGolfWarioXCoord, gb->cpu.a);
	CALL(UpdateGolfWarioAnimation);
	gb_call(gb, ADDR_LoadGolfWarioStateGfx);

	// if Wario is walking, play walk sfx
	// if it's the first frame
	gb->cpu.a = rd8(gb, wGolfWarioState);
	alu_cp(gb, gb->cpu.a, GOLF_WARIO_WALKING);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = rd8(gb, wGolfWarioFramesetOffset);
	alu_cp(gb, gb->cpu.a, 0x2);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = rd8(gb, wGolfWarioDuration);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	play_sfx(gb, SFX_WALK);
}

PORT_JUMP(HandleGolfLobby)
{
	gb_call(gb, ADDR_HandleGolfLobbyStates);
	gb_call(gb, ADDR_UpdateGolfLobbyAnimations);
	gb_call(gb, ADDR_HandleGolfLobbyWarioSprite);
	CALL(ClearUnusedVirtualOAM);
	JUMP_TO(ADDR_GolfLobbyVBlank); // same bank
}

// loads all the gfx in the Golf Lobby
// and determines the price tier,
// corresponding to the number of treasures
static void load_golf_lobby(GB *gb)
{
	gb->cpu.a = BANK_GolfLobbyGfx; // BANK("Golf Gfx 2")
	wr8(gb, wTempBank, gb->cpu.a);
	gb_push16(gb, ADDR_GolfLobbyTilemap);
	gb_push16(gb, ADDR_GolfLobbyAttrmap);

	SET_HL(gb, ADDR_GolfLobbyGfx);
	SET_BC(gb, v0Tiles1);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	CALL(FarDecompress);

	gb->cpu.a = BANK_GolfLobbyGfx; // BANK("Golf Gfx 2")
	wr8(gb, wTempBank, gb->cpu.a);
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, gb_pop16(gb));
	SET_BC(gb, v1BGMap0);
	CALL(FarDecompress);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, gb_pop16(gb));
	SET_BC(gb, v0BGMap0);
	CALL(FarDecompress);

	gb->cpu.a = rd8(gb, wNumCollectedTreasures + 0);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	bool tier3 = !(gb->cpu.f & FLAG_Z);
	if (!tier3) {
		gb->cpu.a = rd8(gb, wNumCollectedTreasures + 1);
		alu_cp(gb, gb->cpu.a, GOLF_TREASURES_TIER_3);
		tier3 = !(gb->cpu.f & FLAG_C);
	}
	if (tier3) {
		gb->cpu.a = GOLF_PRICE_TIER_3;
		wr8(gb, wGolfPrice, gb->cpu.a);
		gb->cpu.d = 0xfc; gb->cpu.e = 0xfd;
		gb->cpu.b = 0xfe; gb->cpu.c = 0xff;
	} else {
		alu_cp(gb, gb->cpu.a, GOLF_TREASURES_TIER_2);
		if (gb->cpu.f & FLAG_C) {
			// tier 1
			gb->cpu.a = GOLF_PRICE_TIER_1;
			wr8(gb, wGolfPrice, gb->cpu.a);
			return;
		}
		// .tier_2
		gb->cpu.a = GOLF_PRICE_TIER_2;
		wr8(gb, wGolfPrice, gb->cpu.a);
		gb->cpu.d = 0xf8; gb->cpu.e = 0xf9;
		gb->cpu.b = 0xfa; gb->cpu.c = 0xfb;
	}
	// .got_price
	wr8(gb, v0BGMap0 + 8 * TILEMAP_WIDTH + 11, gb->cpu.d);
	wr8(gb, v0BGMap0 + 9 * TILEMAP_WIDTH + 11, gb->cpu.e);
	wr8(gb, v0BGMap0 + 8 * TILEMAP_WIDTH + 12, gb->cpu.b);
	gb->cpu.a = gb->cpu.c;
	wr8(gb, v0BGMap0 + 9 * TILEMAP_WIDTH + 12, gb->cpu.c);
}

PORT(InitGolfLobby)
{
	CALL(DisableLCD);
	CALL(ClearVirtualOAM);
	gb->cpu.a = rd8(gb, w1d800);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		CALL(SaveBackupVRAM);
	gb->cpu.a = 0x02;
	wr8(gb, w1d800, 0x02);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wRoomAnimatedTilesEnabled, 0);

	uint16_t hl = wPredeterminedGolfHole;
	uint16_t bc = (uint16_t)(wGolfRAMEnd - wPredeterminedGolfHole);
	do {
		wr8(gb, hl++, 0);
		bc--;
	} while (bc);
	SET_HL(gb, hl);
	SET_BC(gb, 0);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // or c

	SET_HL(gb, ADDR_Pals_1ca1cf);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_1ca20f);
	CALL(LoadPalsToTempPals2);

	load_golf_lobby(gb);

	gb->cpu.a = rd8(gb, wGolfMenuOption);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = GOLF_WARIO_WAITING;
		wr8(gb, wGolfWarioState, gb->cpu.a);
		gb->cpu.a = SCREEN_HEIGHT_PX;
		wr8(gb, wGolfWarioYPos, gb->cpu.a);
		gb->cpu.a = 0x38;
		wr8(gb, wGolfWarioXPos, gb->cpu.a);
	} else {
		// .asm_1c8069
		gb->cpu.a = GOLF_WARIO_GOING_IN_PIPE;
		wr8(gb, wGolfWarioState, gb->cpu.a);
		gb->cpu.a = 0xa0;
		wr8(gb, wGolfWarioYPos, gb->cpu.a);
		gb->cpu.a = 0x80;
		wr8(gb, wGolfWarioXPos, gb->cpu.a);
		gb->cpu.a = GOLFLOBBYSTATE_EXIT_PIPE;
		wr8(gb, wGolfLobbyState, gb->cpu.a);
	}
	// .asm_1c807d
	gb_call(gb, ADDR_HandleGolfLobbyWarioSprite);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wGolfMenuOption, 0);
	gb->cpu.a = rd8(gb, wNumCoins + 0);
	wr8(gb, wGolfNumCoins + 0, gb->cpu.a);
	gb->cpu.a = rd8(gb, wNumCoins + 1);
	wr8(gb, wGolfNumCoins + 1, gb->cpu.a);

	gb->cpu.a = rd8(gb, wNumCoins + 0);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = rd8(gb, wGolfPrice);
		gb->cpu.c = gb->cpu.a;
		gb->cpu.a = rd8(gb, wNumCoins + 1);
		alu_cp(gb, gb->cpu.a, gb->cpu.c);
		if (gb->cpu.f & FLAG_C) {
			gb->cpu.a = COININFO_NOT_ENOUGH_COINS;
			wr8(gb, wGolfObj4State, gb->cpu.a);
		}
	}
	// .enough_coins
	gb->cpu.a = 0x01;
	wr8(gb, wGolfVBlankMode, 0x01);
	gb_call(gb, ADDR_GolfLobbyVBlank);

	// play_music2 MUSIC_GOLF_LOBBY
	gb->cpu.a = (uint8_t)(MUSIC_GOLF_LOBBY >> 8);
	wr8(gb, hMusicIDBackup + 0, gb->cpu.a);
	wr8(gb, hMusicID + 0, gb->cpu.a);
	gb->cpu.a = (uint8_t)MUSIC_GOLF_LOBBY;
	wr8(gb, hMusicIDBackup + 1, gb->cpu.a);
	wr8(gb, hMusicID + 1, gb->cpu.a);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCY, 0);
	wr8(gb, rSCX, 0);
	wr8(gb, wSCY, 0);
	wr8(gb, wSCX, 0);

	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}

PORT_JUMP(UpdateGolfClearCutsceneAnimation)
{
	gb->cpu.a = (uint8_t)(ADDR_OAM_1cb3e0 >> 8);
	wr8(gb, wGolfOAMPtr + 0, gb->cpu.a);
	gb->cpu.a = (uint8_t)ADDR_OAM_1cb3e0;
	wr8(gb, wGolfOAMPtr + 1, gb->cpu.a);
	gb->cpu.a = rd8(gb, wGolfObj1State);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z)
		SET_DE(gb, ADDR_Frameset_1cb5cf);
	else
		SET_DE(gb, ADDR_Frameset_1cb5cc); // .asm_1c8553
	// .asm_1c8556
	SET_HL(gb, wGolfObj1Duration);
	CALL(UpdateGolfObjectAnimation);
	gb->cpu.a = rd8(gb, wGolfObj1CurrentFrame);
	wr8(gb, wGolfObj1Frame, gb->cpu.a);
	gb->cpu.a = 0x58;
	wr8(gb, wGolfObj1XCoord, 0x58);
	SET_HL(gb, wGolfObj1Sprite);
	CALL(LoadGolfSprite);
	JUMP(ClearUnusedVirtualOAM);
}

PORT_JUMP(UpdateGolfClearCutscene)
{
	SET_HL(gb, wGolfCounter);
	gb->cpu.a = rd8(gb, wGolfObj1State);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!(gb->cpu.f & FLAG_Z)) {
		// .asm_1c8530
		gb->cpu.a = rd8(gb, wGolfCounter);
		alu_cp(gb, gb->cpu.a, 0x40);
		if (!(gb->cpu.f & FLAG_C)) {
			// .asm_1c8538
			gb->cpu.a = SST_GOLF_EXIT;
			wr8(gb, wSubState, gb->cpu.a);
			RET();
			return;
		}
		wr8(gb, wGolfCounter, alu_inc(gb, rd8(gb, wGolfCounter)));
		JUMP_TO(ADDR_UpdateGolfClearCutsceneAnimation);
		return;
	}
	gb->cpu.a = rd8(gb, wGolfObj1YCoord);
	alu_cp(gb, gb->cpu.a, 0x50);
	if (gb->cpu.f & FLAG_Z) {
		// .asm_1c8525
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, wGolfCounter, 0);
		wr8(gb, wGolfObj1FramesetOffset, 0);
		gb->cpu.a = alu_inc(gb, gb->cpu.a);
		wr8(gb, wGolfObj1State, gb->cpu.a);
		JUMP_TO(ADDR_UpdateGolfClearCutsceneAnimation);
		return;
	}
	gb->cpu.a = alu_and(gb, rd8(gb, wGolfCounter), 0x0f);
	if (gb->cpu.f & FLAG_Z)
		play_sfx(gb, SFX_01F);
	wr8(gb, wGolfCounter, alu_inc(gb, rd8(gb, wGolfCounter)));
	gb->cpu.a = alu_and(gb, rd8(gb, wGolfCounter), 0x03);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = alu_dec(gb, rd8(gb, wGolfObj1YCoord));
		wr8(gb, wGolfObj1YCoord, gb->cpu.a);
	}
	JUMP_TO(ADDR_UpdateGolfClearCutsceneAnimation);
}

PORT(InitGolfClearCutscene)
{
	CALL(DisableLCD);
	CALL(ClearVirtualOAM);

	uint16_t hl = 0xd800; // STARTOF("Golf RAM")
	uint16_t bc = 0x800;  // SIZEOF("Golf RAM")
	do {
		wr8(gb, hl++, 0);
		bc--;
	} while (bc);
	SET_HL(gb, hl);
	SET_BC(gb, 0);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // or c

	SET_HL(gb, ADDR_Pals_1ca24f);
	CALL(LoadPalsToTempPals1);
	SET_HL(gb, ADDR_Pals_1ca34f);
	CALL(LoadPalsToTempPals2);

	// .LoadCutsceneGfx
	gb->cpu.a = BANK_GolfMenuGfx;
	wr8(gb, wTempBank, gb->cpu.a);
	gb_push16(gb, ADDR_GolfClearTilemap);
	gb_push16(gb, ADDR_GolfClearAttrmap);
	SET_HL(gb, ADDR_GolfMenuGfx);
	SET_BC(gb, v0Tiles0);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	CALL(FarDecompress);
	gb->cpu.a = BANK_GolfClearTilemap; // BANK("Golf Gfx 2")
	wr8(gb, wTempBank, gb->cpu.a);
	gb->cpu.a = BANK_v1Tiles0; // BANK("VRAM1")
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, gb_pop16(gb));
	SET_BC(gb, v0BGMap0);
	CALL(FarDecompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	SET_HL(gb, gb_pop16(gb));
	SET_BC(gb, v0BGMap0);
	CALL(FarDecompress);

	gb->cpu.a = 0x70;
	wr8(gb, wGolfObj1YCoord, 0x70);
	gb_call(gb, ADDR_UpdateGolfClearCutsceneAnimation);
	gb_call(gb, ADDR_VBlank_1c84e0);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rSCY, 0);
	wr8(gb, rSCX, 0);
	wr8(gb, wSCY, 0);
	wr8(gb, wSCX, 0);

	gb->cpu.a = LCDC_DEFAULT;
	wr8(gb, rLCDC, gb->cpu.a);
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
}
