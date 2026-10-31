// Port of src/engine/overworld/bank27.asm
#include "ported.h"

PORT(AdvanceCutsceneFunc)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, w2d014, 0);
	SET_HL(gb, w2d014 - 1);
	wr8(gb, w2d014 - 1, alu_inc(gb, rd8(gb, w2d014 - 1)));
}

// c = frame to wait for
PORT_JUMP(WaitCutsceneFunc)
{
	gb->cpu.a = rd8(gb, w2d014);
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceCutsceneFunc); // jr
}

static void update_ow_animation_b(GB *gb, uint8_t b)
{
	gb->cpu.b = b;
	CALL(UpdateOWAnimation);
	gb->cpu.a = rd8(gb, REG_BC(gb));
}

PORT(Func_9f0ae) { update_ow_animation_b(gb, 0x2f); }
PORT(Func_9f0b5) { update_ow_animation_b(gb, 0x1a); }

// de = OAM
static void load_ow_sprite_b(GB *gb, uint8_t b)
{
	gb->cpu.b = b;
	JUMP(LoadOWSprite);
}

PORT_JUMP(Func_9f0bc) { load_ow_sprite_b(gb, 0x2f); }
PORT_JUMP(Func_9f0c1) { load_ow_sprite_b(gb, 0x62); }
PORT_JUMP(Func_9f0c6) { load_ow_sprite_b(gb, 0x60); }
PORT_JUMP(Func_9f0cb) { load_ow_sprite_b(gb, 0x63); }

// once the animation's finished, sets the state of the obj at hl to b,
// and returns bc = its state
PORT(Func_9f0d0)
{
	gb->cpu.a = rd8(gb, wOWAnimationFinished);
	alu_cp(gb, gb->cpu.a, 0xff);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	uint16_t hl = (uint16_t)(REG_HL(gb) | 0x04); // set 2, l
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0); // duration
	wr8(gb, hl++, 0); // frameset offset
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl, gb->cpu.a); // state
	SET_HL(gb, hl);
	SET_BC(gb, hl);
}

PORT_JUMP(StartCutsceneDelay)
{
	gb->cpu.c = 30;
	JUMP_TO(ADDR_WaitCutsceneFunc);
}

// a = index + 1 into the pointer table at de
// output: hl = bc - 2, from GetPointerFromTableDE
PORT(Func_9f0a5)
{
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	CALL(GetPointerFromTableDE);
	gb->cpu.c = alu_dec(gb, gb->cpu.c);
	gb->cpu.c = alu_dec(gb, gb->cpu.c);
	gb->cpu.l = gb->cpu.c;
	gb->cpu.h = gb->cpu.b;
}

PORT(LoadCutscenes5Gfx_Vram0)
{
	// decompress_tiles0 Cutscenes5Gfx, v0Tiles2
	wr8(gb, wTempBank, BANK_Cutscenes5Gfx);
	SET_HL(gb, ADDR_Cutscenes5Gfx);
	SET_BC(gb, v0Tiles2);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
}

PORT(Func_9cd16)
{
	SET_HL(gb, wSubState);
	wr8(gb, wSubState, alu_inc(gb, rd8(gb, wSubState)));
	// stop_sfx
	wr8(gb, hSFXID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hSFXID + 1, 0x00);
}

PORT(Func_9ce1f) { play_sfx(gb, SFX_112); }

// hl = scene object
PORT(Func_9fb61)
{
	SET_DE(gb, ADDR_OAM_bc000);
	gb->cpu.a = alu_and(gb, 0xfc, gb->cpu.l);
	gb->cpu.l = gb->cpu.a;
	gb_call(gb, ADDR_Func_9f0bc);
}

// e = frame to wait for
PORT_JUMP(EndCutscene)
{
	gb->cpu.a = rd8(gb, w2d014);
	alu_cp(gb, gb->cpu.a, gb->cpu.e);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_9cd16); // jr
}

static void end_cutscene_delay(GB *gb, uint8_t e)
{
	gb->cpu.e = e;
	JUMP_TO(ADDR_EndCutscene);
}

PORT_JUMP(EndCutsceneDelay_20Frames) { end_cutscene_delay(gb, 20); }
PORT_JUMP(EndCutsceneDelay_40Frames) { end_cutscene_delay(gb, 40); }
PORT_JUMP(EndCutsceneDelay_60Frames) { end_cutscene_delay(gb, 60); }
PORT_JUMP(EndCutsceneDelay_80Frames) { end_cutscene_delay(gb, 80); }
PORT_JUMP(EndCutsceneDelay_100Frames) { end_cutscene_delay(gb, 100); }
PORT_JUMP(EndCutsceneDelay_120Frames) { end_cutscene_delay(gb, 120); }

// b = y coord, c = frame, de = y and x, hl = OAM entry
PORT_JUMP(Func_9fb53)
{
	gb->cpu.a = rd8(gb, wSceneObj1YCoord);
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, REG_DE(gb));
	wr8(gb, hl++, gb->cpu.a); // y
	gb->cpu.e = (uint8_t)(gb->cpu.e + 1); // inc e (flags overwritten below)
	gb->cpu.a = rd8(gb, REG_DE(gb));
	wr8(gb, hl++, gb->cpu.a); // x
	gb->cpu.a = gb->cpu.c;
	wr8(gb, hl++, gb->cpu.a); // frame
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl, 0); // attributes
	SET_HL(gb, hl);
	JUMP_TO(ADDR_Func_9fb61); // fallthrough
}

static void func_9fb53(GB *gb, uint16_t hl, uint8_t c, uint8_t b)
{
	gb->cpu.c = c;
	SET_HL(gb, hl);
	gb->cpu.b = b;
	JUMP_TO(ADDR_Func_9fb53);
}

PORT_JUMP(Func_9fb4c) { func_9fb53(gb, wCurSceneObj, 0, 1); }
PORT_JUMP(Func_9fb6c) { func_9fb53(gb, wCurSceneObjDuration, 1, 1); }
PORT_JUMP(Func_9fb75) { func_9fb53(gb, wCurSceneObjDuration, 2, 1); }
PORT_JUMP(Func_9fb7e) { func_9fb53(gb, wCurSceneObjDuration, 3, 1); }
PORT_JUMP(Func_9fb97) { func_9fb53(gb, wCurSceneObj, 5, 3); }
PORT_JUMP(Func_9fba0) { func_9fb53(gb, wCurSceneObjDuration, 6, 3); }

PORT_JUMP(Func_9fb87)
{
	gb->cpu.a = rd8(gb, wSceneObj1YCoord);
	alu_cp(gb, gb->cpu.a, 0x02);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.a = 0x04;
	wr8(gb, wCurSceneObjFrame, 0x04);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurSceneObjFrame + 1, 0);
	SET_HL(gb, wCurSceneObjFrame + 1);
	JUMP_TO(ADDR_Func_9fb61); // jr
}

// decompress_tiles0: ld a, BANK(gfx) / ld [wTempBank], a / ld hl, gfx /
// ld bc, dest / ld a, [wTempBank] / ldh [hCallFuncBank], a / hcall Decompress
static void decompress_tiles0(GB *gb, uint8_t bank, uint16_t gfx, uint16_t dest)
{
	gb->cpu.a = bank;
	wr8(gb, wTempBank, bank);
	SET_HL(gb, gfx);
	SET_BC(gb, dest);
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
}

// decompress_tiles1: the same into VRAM bank 1, then back to bank 0
static void decompress_tiles1(GB *gb, uint8_t bank, uint16_t gfx, uint16_t dest)
{
	gb->cpu.a = BANK_v1Tiles0;
	wr8(gb, rVBK, BANK_v1Tiles0);
	decompress_tiles0(gb, bank, gfx, dest);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

#define CUTSCENE_GFX0(name, gfx, dest) \
	static void name(GB *gb) { decompress_tiles0(gb, BANK_##gfx, ADDR_##gfx, dest); }
#define CUTSCENE_GFX1(name, gfx, dest) \
	static void name(GB *gb) { decompress_tiles1(gb, BANK_##gfx, ADDR_##gfx, dest); }

CUTSCENE_GFX0(cutscenes1_v0, Cutscenes1Gfx, v0Tiles0)
CUTSCENE_GFX1(cutscenes1_v1, Cutscenes1Gfx, v1Tiles0)
CUTSCENE_GFX0(cutscenes2_v0, Cutscenes2Gfx, v0Tiles0)
CUTSCENE_GFX0(cutscenes3_v0, Cutscenes3Gfx, v0Tiles0)
CUTSCENE_GFX0(cutscenes4_v0, Cutscenes4Gfx, v0Tiles0)
CUTSCENE_GFX0(cutscenes5_v0, Cutscenes5Gfx, v0Tiles2)
CUTSCENE_GFX1(cutscenes5_v1, Cutscenes5Gfx, v1Tiles2)
CUTSCENE_GFX0(cutscenes7_v0, Cutscenes7Gfx, v0Tiles0)
CUTSCENE_GFX1(cutscenes7_v1, Cutscenes7Gfx, v1Tiles0)
CUTSCENE_GFX0(cutscenes8_v0, Cutscenes8Gfx, v0Tiles0)
CUTSCENE_GFX1(cutscenes8_v1, Cutscenes8Gfx, v1Tiles0)
CUTSCENE_GFX0(cutscenes9_v0, Cutscenes9Gfx, v0Tiles0)
CUTSCENE_GFX1(cutscenes9_v1, Cutscenes9Gfx, v1Tiles0)
CUTSCENE_GFX0(cutscenes10_v0, Cutscenes10Gfx, v0Tiles2)
CUTSCENE_GFX1(cutscenes10_v1, Cutscenes10Gfx, v1Tiles2)

// The jumps and calls between these loaders stay in bank $27, which
// Decompress restores, so running the next one natively matches the asm.
PORT(LoadCutscenes1Gfx_Vram0) { cutscenes1_v0(gb); cutscenes5_v0(gb); }
PORT(LoadCutscenes1Gfx_Vram1) { cutscenes1_v1(gb); cutscenes5_v1(gb); }
PORT(LoadCutscenes2Gfx) { cutscenes2_v0(gb); cutscenes5_v0(gb); }
PORT(LoadCutscenes3Gfx) { cutscenes3_v0(gb); cutscenes5_v0(gb); }
PORT(LoadCutscenes4Gfx) { cutscenes4_v0(gb); cutscenes5_v0(gb); }
PORT(LoadCutscenes5Gfx_Vram1) { cutscenes5_v1(gb); }
PORT(LoadCutscenes7Gfx_Vram1) { cutscenes7_v1(gb); cutscenes5_v1(gb); }
PORT(Func_9c931) { cutscenes7_v0(gb); cutscenes5_v0(gb); }
PORT(Func_9c937) { cutscenes7_v0(gb); cutscenes10_v0(gb); }
PORT(LoadCutscenes7Gfx_Vram0) { cutscenes7_v0(gb); }
PORT(LoadCutscenes8Gfx_Vram0) { cutscenes8_v0(gb); cutscenes5_v0(gb); }
PORT(Func_9c977) { cutscenes8_v1(gb); cutscenes5_v1(gb); }
PORT(Func_9c97d) { cutscenes8_v1(gb); cutscenes10_v1(gb); }
PORT(LoadCutscenes8Gfx_Vram1) { cutscenes8_v1(gb); }
PORT(LoadCutscenes9Gfx_Vram1) { cutscenes9_v1(gb); cutscenes10_v1(gb); }
PORT(LoadCutscenes9Gfx_Vram0) { cutscenes9_v0(gb); cutscenes10_v0(gb); }
PORT(LoadCutscenes10Gfx_Vram1) { cutscenes10_v1(gb); }
PORT(LoadCutscenes10Gfx_Vram0) { cutscenes10_v0(gb); }

PORT(LoadLevelMainTiles29_Vram1)
{
	gb->cpu.a = BANK_v1Tiles0;
	wr8(gb, rVBK, gb->cpu.a);
	gb->cpu.b = BANK_LevelMainTiles29;
	SET_HL(gb, ADDR_LevelMainTiles29);
	CALL(LoadFarTiles);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

PORT(LoadLevelMainTiles29_Vram0)
{
	gb->cpu.b = BANK_LevelMainTiles29;
	SET_HL(gb, ADDR_LevelMainTiles29);
	CALL(LoadFarTiles);
}

// inc l
static uint16_t inc_l(uint16_t hl)
{
	return (uint16_t)((hl & 0xff00) | (uint8_t)(hl + 1));
}

// dec l
static uint16_t dec_l(uint16_t hl)
{
	return (uint16_t)((hl & 0xff00) | (uint8_t)(hl - 1));
}

// a = state, d = y coord, e = x coord
PORT(Func_9ccf9)
{
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, gb->cpu.d);
	hl = inc_l(hl);
	wr8(gb, hl, gb->cpu.e);
	hl = inc_l(hl);
	hl |= 0x04; // set 2, l
	SET_HL(gb, hl);
	CALL(SetSceneObjState);
}

// xor a / ld hl, start / ld bc, size / call WriteAToHL_BCTimes
static void clear_bytes(GB *gb, uint16_t start, uint16_t size)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, start);
	SET_BC(gb, size);
	CALL(WriteAToHL_BCTimes);
}

PORT(Func_9cc87)
{
	clear_bytes(gb, wTempBGPals, 8 * PAL_SIZE);
	clear_bytes(gb, wTempOBPals, 8 * PAL_SIZE);
}

PORT(ClearTempPals_Bank27)
{
	clear_bytes(gb, wTempPals1, 8 * PAL_SIZE);
	clear_bytes(gb, wTempPals2, 8 * PAL_SIZE);
}

// ld hl, src / ld de, dest / ld b, n / call CopyHLToDE
static void copy_bytes(GB *gb, uint16_t src, uint16_t dest, uint8_t n)
{
	SET_HL(gb, src);
	SET_DE(gb, dest);
	gb->cpu.b = n;
	CALL(CopyHLToDE);
}

PORT(Func_9cd74)
{
	gb_push16(gb, REG_HL(gb)); // push hl
	copy_bytes(gb, wTempPals2, wTempOBPals, 8 * PAL_SIZE);
	copy_bytes(gb, wTempPals1, wTempBGPals, 8 * PAL_SIZE);
	SET_HL(gb, gb_pop16(gb)); // pop hl
}

// Sets up palette config 1 or 2 (at wPalConfig*Register) to apply
// 8 palettes from src
static void set_pal_config(GB *gb, uint16_t source_hi, uint16_t source_lo, uint16_t index_addr,
	uint16_t number, uint16_t reg_addr, uint16_t src, uint8_t index, uint8_t reg)
{
	gb->cpu.a = (uint8_t)(src >> 8);
	wr8(gb, source_hi, gb->cpu.a);
	gb->cpu.a = (uint8_t)src;
	wr8(gb, source_lo, gb->cpu.a);
	gb->cpu.a = index;
	wr8(gb, index_addr, gb->cpu.a);
	gb->cpu.a = 8;
	wr8(gb, number, gb->cpu.a);
	gb->cpu.a = reg;
	wr8(gb, reg_addr, gb->cpu.a);
}

PORT(Func_9cd40)
{
	set_pal_config(gb, wPalConfig1SourceHi, wPalConfig1SourceLo, wPalConfig1Index,
		wPalConfig1Number, wPalConfig1Register, wTempPals1, BGPI_AUTOINC | 0, (uint8_t)rBGPI);
}

PORT(Func_9cd5a)
{
	set_pal_config(gb, wPalConfig2SourceHi, wPalConfig2SourceLo, wPalConfig2Index,
		wPalConfig2Number, wPalConfig2Register, wTempPals2, OBPI_AUTOINC | 0, (uint8_t)rOBPI);
}

static void set_top_rows_attr(GB *gb, uint16_t bgmap, uint8_t attr)
{
	gb->cpu.a = BANK_v1Tiles0;
	wr8(gb, rVBK, gb->cpu.a);
	SET_HL(gb, bgmap);
	SET_BC(gb, 5 * TILEMAP_WIDTH);
	gb->cpu.a = attr;
	CALL(WriteAToHL_BCTimes);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

PORT(SetBGPriorityOnTopRows_BGMap1) { set_top_rows_attr(gb, v1BGMap1, BG_BANK0 | BG_PRIO); }
// d = attributes
PORT(SetBGPriorityOnTopRows) { set_top_rows_attr(gb, v1BGMap0, gb->cpu.d); }

// sets BG priority on the 6x7 block at (0, 5) of v1BGMap0
PORT(Func_9cc4f)
{
	gb->cpu.a = BANK_v1Tiles0;
	wr8(gb, rVBK, gb->cpu.a);
	gb->cpu.d = BG_BANK0 | BG_PRIO;
	uint16_t hl = v1BGMap0 + 5 * TILEMAP_WIDTH;
	gb->cpu.e = 7;
	for (;;) {
		gb->cpu.b = 6;
		gb->cpu.a = gb->cpu.d;
		do {
			wr8(gb, hl++, gb->cpu.a);
			gb->cpu.b = alu_dec(gb, gb->cpu.b);
		} while (gb->cpu.b);
		gb->cpu.e = alu_dec(gb, gb->cpu.e);
		if (!gb->cpu.e)
			break;
		gb->cpu.a = alu_add(gb, TILEMAP_WIDTH - 6, (uint8_t)hl);
		uint8_t l = gb->cpu.a;
		gb->cpu.a = alu_adc(gb, 0, (uint8_t)(hl >> 8));
		hl = (uint16_t)(gb->cpu.a << 8 | l);
	}
	SET_HL(gb, hl);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

PORT_JUMP(InitCutscene49)
{
	gb->cpu.b = BANK_Pals_b8180;
	SET_HL(gb, ADDR_Pals_b8180);
	CALL(LoadFarPalsToTempPals1);
	gb->cpu.b = BANK_Pals_b8e40;
	SET_HL(gb, ADDR_Pals_b8e40);
	CALL(LoadFarPalsToTempPals2);
	JUMP_TO(ADDR_InitCutscene15 + 0x10); // InitCutscene15.asm_9c367
}

// Scene object animations. The helpers they use are in this bank, so they
// are called with gb_call, and the tail jumps (followed by the caller's
// return) become calls too.

// ld a, [bc] / and a / ret z / ld hl, framesets / call Func_9f0a5 /
// call update. Returns false on the ret z.
static bool scene_obj_anim(GB *gb, uint16_t framesets, uint16_t update)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a)
		return false;
	SET_HL(gb, framesets);
	gb_call(gb, ADDR_Func_9f0a5);
	gb_call(gb, update);
	return true;
}

// ld de, oam / jp sprite
static void scene_obj_sprite(GB *gb, uint16_t oam, uint16_t sprite)
{
	SET_DE(gb, oam);
	gb_call(gb, sprite);
}

// animates the obj at bc and loads its sprite
static void scene_obj_anim_sprite(GB *gb, uint16_t framesets, uint16_t oam)
{
	if (!scene_obj_anim(gb, framesets, ADDR_Func_9f0ae))
		return;
	scene_obj_sprite(gb, oam, ADDR_Func_9f0bc);
}

// the same, but in state `state` goes to state `next` once the animation's
// finished (and animates again)
static void scene_obj_anim_sprite_next(GB *gb, uint16_t framesets, uint16_t oam, uint8_t state, uint8_t next)
{
	for (;;) {
		if (!scene_obj_anim(gb, framesets, ADDR_Func_9f0ae))
			return;
		alu_cp(gb, gb->cpu.a, state);
		if (!(gb->cpu.f & FLAG_Z))
			break;
		gb->cpu.b = next;
		gb_call(gb, ADDR_Func_9f0d0);
		if (!(gb->cpu.f & FLAG_Z))
			break;
	}
	scene_obj_sprite(gb, oam, ADDR_Func_9f0bc);
}

PORT(Func_9f857)
{
	uint16_t hl = wSceneObj2State;
	gb->cpu.a = rd8(gb, hl--);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a) {
		SET_HL(gb, hl);
		return;
	}
	SET_HL(gb, dec_l(hl));
	SET_DE(gb, ADDR_Frameset_bf154);
	gb_call(gb, ADDR_Func_9f0ae);
	scene_obj_sprite(gb, ADDR_OAM_bf131, ADDR_Func_9f0bc);
}

PORT(Func_9f1d8)
{
	SET_BC(gb, wSceneObj2State);
	scene_obj_anim_sprite(gb, ADDR_Func_9f1d8 + 0x15, ADDR_OAM_bd615);
}

PORT(Func_9f7a2)
{
	SET_BC(gb, wSceneObj3State);
	if (!scene_obj_anim(gb, ADDR_Func_9f7a2 + 0x15, ADDR_Func_9f0b5))
		return;
	scene_obj_sprite(gb, ADDR_OAM_180b43, ADDR_Func_9f0c6);
}

// animates each obj of objs in turn
static void each_obj(GB *gb, const uint16_t *objs, int n, uint16_t framesets, uint16_t oam)
{
	for (int i = 0; i < n; i++) {
		SET_BC(gb, objs[i]);
		scene_obj_anim_sprite(gb, framesets, oam);
	}
}

static void each_obj_next(GB *gb, const uint16_t *objs, int n, uint16_t framesets, uint16_t oam,
	uint8_t state, uint8_t next)
{
	for (int i = 0; i < n; i++) {
		SET_BC(gb, objs[i]);
		scene_obj_anim_sprite_next(gb, framesets, oam, state, next);
	}
}

static const uint16_t objs_2_3[] = { wSceneObj2State, wSceneObj3State };
static const uint16_t objs_3_2[] = { wSceneObj3State, wSceneObj2State };
static const uint16_t objs_2_4_3[] = { wSceneObj2State, wSceneObj4State, wSceneObj3State };
static const uint16_t objs_3_4_2[] = { wSceneObj3State, wSceneObj4State, wSceneObj2State };
static const uint16_t objs_2_3_4_5[] = { wSceneObj2State, wSceneObj3State, wSceneObj4State, wSceneObj5State };
static const uint16_t objs_11_10_2[] = { wSceneObj11State, wSceneObj10State, wSceneObj2State };

PORT(Func_9f225) { each_obj(gb, objs_2_3, 2, ADDR_Func_9f225 + 0x1b, ADDR_OAM_bd911); }
PORT(Func_9f340) { each_obj(gb, objs_3_4_2, 3, ADDR_Func_9f340 + 0x21, ADDR_OAM_bdb06); }
PORT(Func_9f73b) { each_obj(gb, objs_2_3, 2, ADDR_Func_9f73b + 0x1b, ADDR_OAM_becdc); }
PORT(Func_9f7bb) { each_obj(gb, objs_2_3, 2, ADDR_Func_9f7bb + 0x1b, ADDR_OAM_bee6e); }
PORT(Func_9f2a2) { each_obj(gb, objs_2_4_3, 3, ADDR_Func_9f2a2 + 0x21, ADDR_OAM_bd971); }
PORT(Func_9f3cd) { each_obj(gb, objs_3_2, 2, ADDR_Func_9f3cd + 0x1b, ADDR_OAM_bddcf); }

PORT(Func_9f2cd)
{
	SET_BC(gb, wSceneObj2State);
	scene_obj_anim_sprite_next(gb, ADDR_Func_9f2cd + 0x22, ADDR_OAM_bda01, 0x01, 0x02);
}

PORT(Func_9f11e)
{
	SET_BC(gb, wSceneObj2State);
	scene_obj_anim_sprite_next(gb, ADDR_Func_9f11e + 0x22, ADDR_OAM_bd377, 0x02, 0x03);
}

PORT(Func_9f991) { each_obj_next(gb, objs_2_3, 2, ADDR_Func_9f991 + 0x28, ADDR_OAM_bf611, 0x04, 0x03); }
PORT(Func_9f9c1) { each_obj_next(gb, objs_11_10_2, 3, ADDR_Func_9f9c1 + 0x2e, ADDR_OAM_bf66d, 0x02, 0x01); }
PORT(Func_9f38d) { each_obj_next(gb, objs_2_3_4_5, 4, ADDR_Func_9f38d + 0x34, ADDR_OAM_bdce6, 0x04, 0x01); }

// plays sfx if the animation's finished
static void sfx_if_animation_finished(GB *gb, uint16_t sfx)
{
	gb->cpu.a = rd8(gb, wOWAnimationFinished);
	alu_cp(gb, gb->cpu.a, 0xff);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	play_sfx(gb, sfx);
}

PORT(Func_9f2f3)
{
	SET_BC(gb, wSceneObj2State);
	if (!scene_obj_anim(gb, ADDR_Func_9f2f3 + 0x2a, ADDR_Func_9f0b5))
		return;
	alu_cp(gb, gb->cpu.a, 0x03);
	if (gb->cpu.f & FLAG_Z)
		sfx_if_animation_finished(gb, SFX_06D);
	scene_obj_sprite(gb, ADDR_OAM_188279, ADDR_Func_9f0c1);
}

PORT(Func_9f3ee)
{
	for (int i = 0; i < 2; i++) {
		SET_BC(gb, objs_2_3[i]);
		if (!scene_obj_anim(gb, ADDR_Func_9f3ee + 0x20, ADDR_Func_9f0ae))
			continue;
		alu_cp(gb, gb->cpu.a, 0x03);
		if (gb->cpu.f & FLAG_Z)
			sfx_if_animation_finished(gb, SFX_131);
		scene_obj_sprite(gb, ADDR_OAM_bde69, ADDR_Func_9f0bc);
	}
}

PORT(Func_9f821)
{
	SET_BC(gb, wSceneObj3State);
	if (!scene_obj_anim(gb, ADDR_Func_9f821 + 0x19, ADDR_Func_9f0b5))
		return;
	alu_cp(gb, gb->cpu.a, 0x01);
	if (gb->cpu.f & FLAG_Z) {
		// .Func_9f843
		SET_DE(gb, wSceneObj3Unk7);
		gb->cpu.a = alu_inc(gb, rd8(gb, wSceneObj3Unk7));
		wr8(gb, wSceneObj3Unk7, gb->cpu.a);
		alu_cp(gb, gb->cpu.a, 0x0e);
		if (!(gb->cpu.f & FLAG_C)) {
			gb->cpu.a = 0;
			gb->cpu.f = FLAG_Z; // xor a
			wr8(gb, wSceneObj3Unk7, 0);
			play_sfx(gb, SFX_01E);
		}
	}
	scene_obj_sprite(gb, ADDR_OAM_18aba0, ADDR_Func_9f0c1);
}

PORT(Func_9cd8d)
{
	FARCALL(Func_85046);
}

// hl = compressed BG map, bc = destination
PORT(LoadCutsceneBGMap)
{
	wr8(gb, wTempBank, BANK_Cutscene01Tilemap); // BANK("Cutscene BG Maps")
	gb->cpu.a = rd8(gb, wTempBank);
	wr8(gb, hCallFuncBank, gb->cpu.a);
	HCALL(Decompress);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
}

PORT(PrologueFunc)
{
	FARCALL(Prologue);
}

PORT_JUMP(LoadCutsceneTilemap0)
{
	SET_BC(gb, v0BGMap0);
	JUMP_TO(ADDR_LoadCutsceneBGMap); // fallthrough
}

PORT_JUMP(LoadCutsceneTilemap1)
{
	SET_BC(gb, v0BGMap1);
	JUMP_TO(ADDR_LoadCutsceneBGMap);
}

// Func_9f0e0.asm_9f0e9: bc = scene obj state
static void func_9f0e9(GB *gb)
{
	for (;;) {
		gb->cpu.a = alu_and(gb, rd8(gb, REG_BC(gb)), rd8(gb, REG_BC(gb)));
		if (gb->cpu.f & FLAG_Z)
			return;
		SET_HL(gb, ADDR_Func_9f0e0 + 0x30); // .framesets
		gb_call(gb, ADDR_Func_9f0a5);
		gb_call(gb, ADDR_Func_9f0ae);
		alu_cp(gb, gb->cpu.a, 0x05);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.b = 0x00;
		} else {
			alu_cp(gb, gb->cpu.a, 0x06);
			if (!(gb->cpu.f & FLAG_Z))
				break;
			gb->cpu.b = 0x07;
		}
		gb_call(gb, ADDR_Func_9f0d0);
		if (!(gb->cpu.f & FLAG_Z))
			break;
	}
	// .asm_9f0fd
	SET_DE(gb, ADDR_OAM_bd1fd);
	gb_call(gb, ADDR_Func_9f0bc);
}

PORT(Func_9f0e0)
{
	SET_BC(gb, wSceneObj3State);
	func_9f0e9(gb);
	SET_BC(gb, wSceneObj2State);
	func_9f0e9(gb);
}

PORT_JUMP(OWCutTree)
{
	for (;;) {
		SET_DE(gb, wSceneObj2);
		gb_call(gb, ADDR_Func_9fb97);
		gb->cpu.a = rd8(gb, wSceneObj2State);
		uint16_t hl = wSceneObj2State - 1;
		gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z) {
			SET_HL(gb, hl);
			RET();
			return;
		}
		gb->cpu.l = alu_dec(gb, (uint8_t)hl);
		gb->cpu.h = (uint8_t)(hl >> 8);
		gb->cpu.b = BANK_OAM_bc122; // BANK("Overworld OAM 2")
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z) {
			// .one
			SET_DE(gb, ADDR_Frameset_bc1f3);
			CALL(UpdateOWAnimation);
			break;
		}
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (!(gb->cpu.f & FLAG_Z)) {
			JUMP_TO(ADDR_OWCutTree + 0x18 - 3); // debug_nop
			return;
		}
		// .two
		SET_DE(gb, ADDR_Frameset_bc1e8);
		CALL(UpdateOWAnimation);
		gb->cpu.b = 0x00;
		gb_call(gb, ADDR_Func_9f0d0);
		if (!(gb->cpu.f & FLAG_Z))
			break;
	}
	// .asm_9fc85
	SET_DE(gb, ADDR_OAM_bc122);
	JUMP_TO(ADDR_Func_9f0bc);
}

PORT(VBlank_Cutscene)
{
	SET_HL(gb, ADDR_VBlank_Cutscene + 0x0c); // .Func
	SET_DE(gb, wVBlankFunc);
	gb->cpu.b = 0x51; // .end - .Func
	CALL(CopyHLToDE);
}

// rst JumpTable, with the table at `table` (right after the rst): leaves every
// register as JumpTable does and returns the target (= hl).
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

PORT_JUMP(DoCutsceneFunc)
{
	CALL(ProcessDPadRepeat);
	SET_HL(gb, w2d014);
	wr8(gb, w2d014, alu_inc(gb, rd8(gb, w2d014)));
	SET_BC(gb, w2d014);
	gb->cpu.a = rd8(gb, wEventWithCutscene);
	alu_cp(gb, gb->cpu.a, NUM_EVENTS);
	if (!(gb->cpu.f & FLAG_C)) {
		JUMP_TO(ADDR_DoCutsceneFunc + 0xc8); // .OutOfBoundsCutscene
		return;
	}
	JUMP_TO(jumptable(gb, ADDR_DoCutsceneFunc + 0x12));
}

PORT_JUMP(Cutscene02Func)
{
	gb_call(gb, ADDR_Cutscene02Func + 0x6); // .PlayCutscene
	JUMP_TO(ADDR_OWCutTree);
}

PORT_LOCAL_JUMP(Cutscene02Func, PlayCutscene)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene02Func + 0xa));
}

// ld hl, wSceneObj2 / ld a, y / ld [hli], a / ld [hl], x /
// ld a, state / ld hl, wSceneObj2State / call SetSceneObjState
static void place_scene_obj2(GB *gb, uint8_t y, uint8_t x, uint8_t state)
{
	gb->cpu.a = y;
	wr8(gb, wSceneObj2 + 0, y);
	wr8(gb, wSceneObj2 + 1, x);
	gb->cpu.a = state;
	SET_HL(gb, wSceneObj2State);
	CALL(SetSceneObjState);
}

PORT_LOCAL_JUMP(Cutscene02Func, ShowAxe)
{
	place_scene_obj2(gb, SCREEN_HEIGHT_PX, 0x6c, 0x01);
	gb->cpu.a = 0x03;
	wr8(gb, wSceneObj1YCoord, gb->cpu.a);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene02Func, RaiseAxe)
{
	wr8(gb, wSceneObj2 + 0, alu_dec(gb, rd8(gb, wSceneObj2 + 0)));
	gb->cpu.a = rd8(gb, wSceneObj2 + 0);
	SET_HL(gb, wSceneObj2 + 1);
	wr8(gb, wSceneObj2 + 1, alu_dec(gb, rd8(gb, wSceneObj2 + 1)));
	alu_cp(gb, gb->cpu.a, 0x86);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_Func_9ce1f);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x60);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene02Func, Wait30) { gb->cpu.c = 30; JUMP_TO(ADDR_WaitCutsceneFunc); }
PORT_LOCAL_JUMP(Cutscene02Func, Wait60) { gb->cpu.c = 60; JUMP_TO(ADDR_WaitCutsceneFunc); }
PORT_LOCAL_JUMP(Cutscene02Func, Wait16) { gb->cpu.c = 16; JUMP_TO(ADDR_WaitCutsceneFunc); }

PORT_LOCAL_JUMP(Cutscene02Func, ShowSpeedLines)
{
	gb->cpu.a = LCDC_BG_MAP;
	wr8(gb, wLCDCFlagsToFlip, gb->cpu.a);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene02Func, HideAxe)
{
	gb->cpu.a = LCDC_BG_MAP;
	wr8(gb, wLCDCFlagsToFlip, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	SET_HL(gb, wSceneObj2State);
	CALL(SetSceneObjState);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj1YCoord, 0);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene02Func, ShowCut)
{
	place_scene_obj2(gb, 96, 66, 0x02);
	play_sfx(gb, SFX_101);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene02Func, WaitCutAnimation)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wSceneObj2State), rd8(gb, wSceneObj2State));
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_JUMP(InitCutscene02)
{
	gb->cpu.b = BANK_Pals_b8200;
	SET_HL(gb, ADDR_Pals_b8200);
	CALL(LoadFarPalsToTempPals1);
	gb->cpu.b = BANK_Pals_b8c00;
	SET_HL(gb, ADDR_Pals_b8c00);
	CALL(LoadFarPalsToTempPals2);
	gb_call(gb, ADDR_LoadCutscenes1Gfx_Vram0);
	gb_call(gb, ADDR_LoadCutsceneBGMaps01);
	JUMP_TO(ADDR_LoadCutsceneBGMaps02);
}

PORT_JUMP(InitCutscene03)
{
	gb->cpu.b = BANK_Pals_b8240;
	SET_HL(gb, ADDR_Pals_b8240);
	CALL(LoadFarPalsToTempPals1);
	gb->cpu.b = BANK_Pals_b88c0;
	SET_HL(gb, ADDR_Pals_b88c0);
	CALL(LoadFarPalsToTempPals2);
	gb_call(gb, ADDR_LoadCutscenes3Gfx);
	JUMP_TO(ADDR_LoadCutsceneBGMaps07);
}

// loads the attrmap at `attrmap` and the tilemap at `tilemap`
static void load_cutscene_bg_maps(GB *gb, uint16_t attrmap, uint16_t load_attrmap, uint16_t tilemap, uint16_t load_tilemap)
{
	SET_HL(gb, attrmap);
	gb_call(gb, load_attrmap);
	SET_HL(gb, tilemap);
	gb_call(gb, load_tilemap);
}

PORT(LoadCutsceneBGMaps01)
{
	load_cutscene_bg_maps(gb, ADDR_Cutscene01Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene01Tilemap, ADDR_LoadCutsceneTilemap0);
}

PORT(LoadCutsceneBGMaps02)
{
	load_cutscene_bg_maps(gb, ADDR_Cutscene02Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene02Tilemap, ADDR_LoadCutsceneTilemap1);
}

PORT(LoadCutsceneBGMaps07)
{
	load_cutscene_bg_maps(gb, ADDR_Cutscene07Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene07Tilemap, ADDR_LoadCutsceneTilemap0);
}

PORT_JUMP(LoadCutsceneAttrmap0)
{
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, 1);
	JUMP_TO(ADDR_LoadCutsceneTilemap0); // fallthrough
}

PORT_JUMP(LoadCutsceneAttrmap1)
{
	gb->cpu.a = 1; // BANK("VRAM1")
	wr8(gb, rVBK, 1);
	JUMP_TO(ADDR_LoadCutsceneTilemap1); // fallthrough
}

PORT_JUMP(Cutscene03Func)
{
	gb_call(gb, ADDR_Cutscene03Func + 0x6); // .PlayCutscene
	JUMP_TO(ADDR_Func_9f0e0);
}

PORT_LOCAL_JUMP(Cutscene03Func, PlayCutscene)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene03Func + 0xa));
}

// ld hl, obj / ld a, y / ld [hli], a / ld [hl], x / ld a, state /
// ld hl, obj state / call SetSceneObjState
static void place_scene_obj(GB *gb, uint16_t obj, uint8_t y, uint8_t x, uint8_t state, uint16_t obj_state)
{
	gb->cpu.a = y;
	wr8(gb, obj + 0, y);
	wr8(gb, obj + 1, x);
	gb->cpu.a = state;
	SET_HL(gb, obj_state);
	CALL(SetSceneObjState);
}

PORT_LOCAL_JUMP(Cutscene03Func, Func_9cf36)
{
	place_scene_obj(gb, wSceneObj2, 59, 40, 0x01, wSceneObj2State);
	place_scene_obj(gb, wSceneObj3, 59, 120, 0x02, wSceneObj3State);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

// bc = frame counter
PORT_LOCAL_JUMP(Cutscene03Func, Func_9cf59)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	if (alu_bit(gb, gb->cpu.a, 0)) {
		RET();
		return;
	}
	wr8(gb, wSceneObj3XCoord, alu_dec(gb, rd8(gb, wSceneObj3XCoord)));
	SET_HL(gb, wSceneObj2XCoord);
	wr8(gb, wSceneObj2XCoord, alu_inc(gb, rd8(gb, wSceneObj2XCoord)));
	gb->cpu.a = rd8(gb, wSceneObj2XCoord);
	alu_cp(gb, gb->cpu.a, 79);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene03Func, Wait30_1) { gb->cpu.c = 30; JUMP_TO(ADDR_WaitCutsceneFunc); }
PORT_LOCAL_JUMP(Cutscene03Func, Wait30_2) { gb->cpu.c = 30; JUMP_TO(ADDR_WaitCutsceneFunc); }
PORT_LOCAL_JUMP(Cutscene03Func, Wait30_3) { gb->cpu.c = 30; JUMP_TO(ADDR_WaitCutsceneFunc); }
PORT_LOCAL_JUMP(Cutscene03Func, Wait30_4) { gb->cpu.c = 30; JUMP_TO(ADDR_WaitCutsceneFunc); }
PORT_LOCAL_JUMP(Cutscene03Func, Wait60_1) { gb->cpu.c = 60; JUMP_TO(ADDR_WaitCutsceneFunc); }
PORT_LOCAL_JUMP(Cutscene03Func, Wait60_2) { gb->cpu.c = 60; JUMP_TO(ADDR_WaitCutsceneFunc); }

PORT_LOCAL_JUMP(Cutscene03Func, Func_9cf71)
{
	gb->cpu.a = 0x03;
	SET_HL(gb, wSceneObj2State);
	CALL(SetSceneObjState);
	gb->cpu.a = 80;
	wr8(gb, wSceneObj2XCoord, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj3State, 0);
	play_sfx(gb, SFX_CLICK);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene03Func, Func_9cf92)
{
	place_scene_obj(gb, wSceneObj3YCoord, 59, 80, 0x05, wSceneObj3State);
	play_sfx(gb, SFX_106);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene03Func, Func_9cfad)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wSceneObj3State), rd8(gb, wSceneObj3State));
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

// bc = frame counter
PORT_LOCAL_JUMP(Cutscene03Func, Func_9cfba)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	if (!alu_bit(gb, gb->cpu.a, 0)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj2YCoord, alu_inc(gb, rd8(gb, wSceneObj2YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj2YCoord);
	alu_cp(gb, gb->cpu.a, 0x54);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene03Func, Func_9cfce)
{
	gb->cpu.a = 0x04;
	SET_HL(gb, wSceneObj2State);
	CALL(SetSceneObjState);
	SET_HL(gb, wSceneObj2XCoord);
	wr8(gb, wSceneObj2XCoord, alu_inc(gb, rd8(gb, wSceneObj2XCoord)));
	play_sfx(gb, SFX_CLICK);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene03Func, Func_9cfea)
{
	gb->cpu.a = rd8(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj3YCoord, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSceneObj2XCoord);
	wr8(gb, wSceneObj3XCoord, gb->cpu.a);
	gb->cpu.a = 0x06;
	SET_HL(gb, wSceneObj3State);
	CALL(SetSceneObjState);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene03Func, Func_9d001)
{
	gb->cpu.a = rd8(gb, wSceneObj3State);
	alu_cp(gb, gb->cpu.a, 0x07);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene03Func, Func_9d00a)
{
	play_sfx(gb, SFX_10C);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

// --- Cutscene 04 and small helpers ---

PORT_JUMP(InitCutscene04)
{
	gb->cpu.b = BANK_Pals_b8200;
	SET_HL(gb, ADDR_Pals_b8200);
	CALL(LoadFarPalsToTempPals1);
	gb->cpu.b = BANK_Pals_b8ec0;
	SET_HL(gb, ADDR_Pals_b8ec0);
	CALL(LoadFarPalsToTempPals2);
	gb_call(gb, ADDR_LoadCutscenes8Gfx_Vram0);
	gb_call(gb, ADDR_LoadCutsceneBGMaps01);
	JUMP_TO(ADDR_LoadCutsceneBGMaps02);
}

PORT(LoadCutsceneBGMaps03)
{
	load_cutscene_bg_maps(gb, ADDR_Cutscene03Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene03Tilemap, ADDR_LoadCutsceneTilemap0);
}

PORT_JUMP(SetBGPriorityOnTopRows_Vram1)
{
	gb->cpu.d = BG_BANK1 | BG_PRIO;
	JUMP_TO(ADDR_SetBGPriorityOnTopRows);
}

PORT_JUMP(SetBGPriorityOnTopRows_Vram0)
{
	gb->cpu.d = BG_BANK0 | BG_PRIO;
	JUMP_TO(ADDR_SetBGPriorityOnTopRows); // fallthrough
}

PORT(Func_9cd9d) { FARCALL(Func_850b9); }
PORT(Func_9cdad) { FARCALL(Func_850ff); }
PORT(Func_9cdbd) { FARCALL(Func_84fff); }

PORT_JUMP(Cutscene04Func)
{
	gb_call(gb, ADDR_Cutscene04Func + 0x6); // .Func_9db09
	JUMP_TO(ADDR_Func_9f3cd);
}

PORT_LOCAL_JUMP(Cutscene04Func, Func_9db09)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene04Func + 0xa));
}

// bc = frame counter
PORT_LOCAL_JUMP(Cutscene04Func, Func_9db1b)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x14);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	place_scene_obj(gb, wSceneObj2YCoord, 128, 80, 0x02, wSceneObj2State);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

// dec [hl] / ld a, [hl]
static void dec_hl_ld_a(GB *gb)
{
	uint16_t hl = REG_HL(gb);
	wr8(gb, hl, alu_dec(gb, rd8(gb, hl)));
	gb->cpu.a = rd8(gb, hl);
}

PORT_LOCAL_JUMP(Cutscene04Func, Func_9db32)
{
	SET_HL(gb, wSceneObj2);
	dec_hl_ld_a(gb); // call .Func_9db43
	alu_cp(gb, gb->cpu.a, 0x70);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_Func_9ce1f);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x4a);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL(Cutscene04Func, Func_9db43) { dec_hl_ld_a(gb); }

// bc = frame counter
PORT_LOCAL_JUMP(Cutscene04Func, Func_9db46)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x32);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	place_scene_obj(gb, wSceneObj3YCoord, 128, 80, 0x03, wSceneObj3State);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene04Func, Func_9db5d)
{
	SET_HL(gb, wSceneObj3);
	dec_hl_ld_a(gb); // call .Func_9db43
	alu_cp(gb, gb->cpu.a, 0x4a);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	play_sfx(gb, SFX_113);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene04Func, Func_9db71)
{
	gb->cpu.a = 0x01;
	SET_HL(gb, wSceneObj2State);
	CALL(SetSceneObjState);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj3State, 0);
	gb->cpu.a = 0x08;
	wr8(gb, wCollectionRow, 0x08);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

// --- the remaining LoadCutsceneBGMapsNN ---
PORT(LoadCutsceneBGMaps12) { load_cutscene_bg_maps(gb, ADDR_Cutscene12Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene12Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps13) { load_cutscene_bg_maps(gb, ADDR_Cutscene13Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene13Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps15) { load_cutscene_bg_maps(gb, ADDR_Cutscene15Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene15Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps16) { load_cutscene_bg_maps(gb, ADDR_Cutscene16Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene16Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps17) { load_cutscene_bg_maps(gb, ADDR_Cutscene17Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene17Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps18) { load_cutscene_bg_maps(gb, ADDR_Cutscene18Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene18Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps20) { load_cutscene_bg_maps(gb, ADDR_Cutscene20Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene20Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps22) { load_cutscene_bg_maps(gb, ADDR_Cutscene22Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene22Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps23) { load_cutscene_bg_maps(gb, ADDR_Cutscene23Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene23Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps24) { load_cutscene_bg_maps(gb, ADDR_Cutscene24Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene24Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps26) { load_cutscene_bg_maps(gb, ADDR_Cutscene26Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene26Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps27) { load_cutscene_bg_maps(gb, ADDR_Cutscene27Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene27Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps28) { load_cutscene_bg_maps(gb, ADDR_Cutscene28Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene28Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps29) { load_cutscene_bg_maps(gb, ADDR_Cutscene29Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene29Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps30) { load_cutscene_bg_maps(gb, ADDR_Cutscene30Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene30Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps31) { load_cutscene_bg_maps(gb, ADDR_Cutscene31Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene31Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps32) { load_cutscene_bg_maps(gb, ADDR_Cutscene32Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene32Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps34) { load_cutscene_bg_maps(gb, ADDR_Cutscene34Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene34Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps36) { load_cutscene_bg_maps(gb, ADDR_Cutscene36Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene36Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps37) { load_cutscene_bg_maps(gb, ADDR_Cutscene37Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene37Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps39) { load_cutscene_bg_maps(gb, ADDR_Cutscene39Attrmap, ADDR_LoadCutsceneAttrmap0, ADDR_Cutscene39Tilemap, ADDR_LoadCutsceneTilemap0); }
PORT(LoadCutsceneBGMaps04) { load_cutscene_bg_maps(gb, ADDR_Cutscene04Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene04Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps05) { load_cutscene_bg_maps(gb, ADDR_Cutscene05Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene05Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps14) { load_cutscene_bg_maps(gb, ADDR_Cutscene14Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene14Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps19) { load_cutscene_bg_maps(gb, ADDR_Cutscene19Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene19Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps21) { load_cutscene_bg_maps(gb, ADDR_Cutscene21Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene21Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps25) { load_cutscene_bg_maps(gb, ADDR_Cutscene25Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene25Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps35) { load_cutscene_bg_maps(gb, ADDR_Cutscene35Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene35Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps38) { load_cutscene_bg_maps(gb, ADDR_Cutscene38Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene38Tilemap, ADDR_LoadCutsceneTilemap1); }
PORT(LoadCutsceneBGMaps040) { load_cutscene_bg_maps(gb, ADDR_Cutscene40Attrmap, ADDR_LoadCutsceneAttrmap1, ADDR_Cutscene40Tilemap, ADDR_LoadCutsceneTilemap1); }

// --- more scene object animations ---

PORT(Func_9f146)
{
	SET_BC(gb, wSceneObj2State);
	scene_obj_anim_sprite(gb, ADDR_Func_9f146 + 0x2a, ADDR_OAM_bd453);
	SET_BC(gb, wSceneObj3State);
	scene_obj_anim_sprite(gb, ADDR_Func_9f146 + 0x2a, ADDR_OAM_bd453);
	SET_DE(gb, wSceneObj2);
	gb_call(gb, ADDR_Func_9fb97);
	SET_DE(gb, wSceneObj3);
	gb_call(gb, ADDR_Func_9fba0);
}

// Func_9f174.Func_9f189
static void func_9f189(GB *gb)
{
	for (;;) {
		if (!scene_obj_anim(gb, ADDR_Func_9f174 + 0x50, ADDR_Func_9f0ae))
			return;
		alu_cp(gb, gb->cpu.a, 0x06);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.b = 0x07;
		} else {
			alu_cp(gb, gb->cpu.a, 0x08);
			if (gb->cpu.f & FLAG_Z) {
				gb->cpu.b = 0x00;
			} else {
				alu_cp(gb, gb->cpu.a, 0x05);
				if (gb->cpu.f & FLAG_Z)
					sfx_if_animation_finished(gb, SFX_10A);
				break;
			}
		}
		gb_call(gb, ADDR_Func_9f0d0);
		if (!(gb->cpu.f & FLAG_Z))
			break;
	}
	scene_obj_sprite(gb, ADDR_OAM_bd46f, ADDR_Func_9f0bc);
}

PORT(Func_9f174)
{
	SET_BC(gb, wSceneObj4State);
	func_9f189(gb);
	SET_BC(gb, wSceneObj2State);
	func_9f189(gb);
	SET_BC(gb, wSceneObj3State);
	func_9f189(gb);
	SET_BC(gb, wSceneObj5State);
	func_9f189(gb);
}

PORT(Func_9f1f3)
{
	SET_DE(gb, wSceneObj2);
	gb_call(gb, ADDR_Func_9fb75);
	SET_DE(gb, wSceneObj2);
	gb_call(gb, ADDR_Func_9fb97);
	SET_BC(gb, wSceneObj2State);
	scene_obj_anim_sprite(gb, ADDR_Func_9f1f3 + 0x2a, ADDR_OAM_bd7a1);
	SET_DE(gb, wSceneObj2);
	gb_call(gb, ADDR_Func_9fb7e);
}

// Func_9f244.Func_9f27f
static void func_9f27f(GB *gb)
{
	gb->cpu.d = gb->cpu.h;
	gb->cpu.e = gb->cpu.l | (1 << 1);
	gb->cpu.a = rd8(gb, REG_DE(gb));
	gb->cpu.c = gb->cpu.a;
	gb->cpu.e |= (1 << 0) | (1 << 2);
	gb->cpu.a = rd8(gb, REG_DE(gb));
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	bool nz = !(gb->cpu.f & FLAG_Z);
	gb->cpu.a = 0x01;
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (!nz) {
		if (gb->cpu.f & FLAG_Z)
			return;
		gb->cpu.a = 0x01;
		wr8(gb, REG_DE(gb), gb->cpu.a);
		return;
	}
	// .asm_9f295
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = alu_xor(gb, gb->cpu.a, gb->cpu.a);
	wr8(gb, REG_DE(gb), gb->cpu.a);
	play_sfx(gb, SFX_011);
}

PORT(Func_9f244)
{
	SET_BC(gb, wSceneObj5State);
	if (!scene_obj_anim(gb, ADDR_Func_9f244 + 0x1e, ADDR_Func_9f0b5))
		return;
	alu_cp(gb, gb->cpu.a, 0x02);
	if (gb->cpu.f & FLAG_Z) {
		sfx_if_animation_finished(gb, SFX_FLAP);
	} else {
		alu_cp(gb, gb->cpu.a, 0x01);
		if (gb->cpu.f & FLAG_Z)
			func_9f27f(gb);
	}
	scene_obj_sprite(gb, ADDR_OAM_188e16, ADDR_Func_9f0c1);
}

PORT(Func_9f323)
{
	SET_DE(gb, wSceneObj2);
	gb_call(gb, ADDR_Func_9fb97);
	SET_BC(gb, wSceneObj2State);
	scene_obj_anim_sprite(gb, ADDR_Func_9f323 + 0x1b, ADDR_OAM_bdabc);
}

// ---- InitCutsceneNN ----

// ld b, BANK(pals) / ld hl, pals / call LoadFarPalsToTempPals1 (or 2)
#define PALS1(p) do { gb->cpu.b = BANK_##p; SET_HL(gb, ADDR_##p); CALL(LoadFarPalsToTempPals1); } while (0)
#define PALS2(p) do { gb->cpu.b = BANK_##p; SET_HL(gb, ADDR_##p); CALL(LoadFarPalsToTempPals2); } while (0)
// ld b, BANK(tiles) / ld hl, tiles / call LoadFarTiles
#define FAR_TILES(t) do { gb->cpu.b = BANK_##t; SET_HL(gb, ADDR_##t); CALL(LoadFarTiles); } while (0)
// the same into VRAM bank 1
#define FAR_TILES1(t) do { \
	gb->cpu.a = 1; wr8(gb, rVBK, 1); \
	FAR_TILES(t); \
	gb->cpu.a = 0; gb->cpu.f = FLAG_Z; wr8(gb, rVBK, 0); \
} while (0)
// decompress_attrmap / decompress_tilemap
#define ATTRMAP(m, dest) decompress_tiles1(gb, BANK_##m, ADDR_##m, dest)
#define TILEMAP(m, dest) decompress_tiles0(gb, BANK_##m, ADDR_##m, dest)

static void lcdc_flip_bg_map(GB *gb)
{
	gb->cpu.a = LCDC_BG_MAP;
	wr8(gb, wLCDCFlagsToFlip, gb->cpu.a);
}

PORT_JUMP(InitCutscene06)
{
	gb_call(gb, ADDR_LoadCutscenes3Gfx);
	ATTRMAP(Cutscene08Attrmap, v1BGMap0);
	TILEMAP(Cutscene08Tilemap, v0BGMap0);
	PALS1(Pals_b8080);
	PALS2(Pals_b8900);
	JUMP_TO(ADDR_Func_9d01a);
}

PORT_JUMP(InitCutscene07)
{
	gb_call(gb, ADDR_LoadCutscenes1Gfx_Vram0);
	ATTRMAP(Cutscene06Attrmap, v1BGMap0);
	TILEMAP(Cutscene06Tilemap, v0BGMap0);
	PALS1(Pals_b8000);
	PALS2(Pals_b89c0);
	JUMP_TO(ADDR_Func_9d158);
}

// hl = pals for TempPals2
PORT_JUMP(Func_9c25f)
{
	gb->cpu.b = BANK_Pals_b8ac0;
	CALL(LoadFarPalsToTempPals2);
	SET_HL(gb, ADDR_Pals_b8200);
	gb->cpu.b = BANK_Pals_b8200;
	CALL(LoadFarPalsToTempPals1);
	lcdc_flip_bg_map(gb);
	gb_call(gb, ADDR_LoadCutscenes2Gfx);
	gb_call(gb, ADDR_LoadCutsceneBGMaps01);
	JUMP_TO(ADDR_LoadCutsceneBGMaps04);
}

static void func_9c25f(GB *gb, uint16_t pals)
{
	SET_HL(gb, pals);
	JUMP_TO(ADDR_Func_9c25f);
}

PORT_JUMP(InitCutscene08) { func_9c25f(gb, ADDR_Pals_b8ac0); }
PORT_JUMP(InitCutscene10) { func_9c25f(gb, ADDR_Pals_b8b00); }
PORT_JUMP(InitCutscene1d) { func_9c25f(gb, ADDR_Pals_b8b80); }
PORT_JUMP(InitCutscene17) { func_9c25f(gb, ADDR_Pals_b8b40); }
PORT_JUMP(InitCutscene25) { func_9c25f(gb, ADDR_Pals_b8bc0); }

PORT_JUMP(InitCutscene09)
{
	PALS1(Pals_b8200);
	PALS2(Pals_b8a80);
	gb_call(gb, ADDR_LoadCutscenes2Gfx);
	JUMP_TO(ADDR_LoadCutsceneBGMaps03);
}

PORT_JUMP(InitCutscene0a)
{
	PALS1(Pals_b8040);
	PALS2(Pals_b8a00);
	gb_call(gb, ADDR_LoadCutscenes1Gfx_Vram0);
	gb_call(gb, ADDR_LoadCutsceneBGMaps03);
	JUMP_TO(ADDR_Func_9d0e6);
}

PORT_JUMP(InitCutscene0c)
{
	PALS1(Pals_b8040);
	PALS2(Pals_b8d40);
	gb_call(gb, ADDR_LoadCutscenes4Gfx);
	JUMP_TO(ADDR_LoadCutsceneBGMaps03);
}

PORT_JUMP(InitCutscene0d)
{
	PALS1(Pals_b8040);
	PALS2(Pals_b8dc0);
	gb_call(gb, ADDR_LoadCutscenes4Gfx);
	gb_call(gb, ADDR_LoadCutsceneBGMaps01);
	JUMP_TO(ADDR_LoadCutsceneBGMaps02);
}

PORT_JUMP(InitCutscene0e)
{
	PALS1(Pals_b8200);
	PALS2(Pals_b8980);
	gb_call(gb, ADDR_LoadCutscenes1Gfx_Vram0);
	gb_call(gb, ADDR_LoadCutsceneBGMaps03);
	JUMP_TO(ADDR_LoadCutsceneBGMaps21);
}

PORT_JUMP(InitCutscene11)
{
	PALS1(Pals_b8040);
	PALS2(Pals_b8c40);
	gb_call(gb, ADDR_LoadCutscenes1Gfx_Vram0);
	gb_call(gb, ADDR_LoadCutsceneBGMaps03);
	JUMP_TO(ADDR_LoadCutsceneBGMaps21);
}

PORT_JUMP(InitCutscene13)
{
	PALS1(Pals_b8040);
	PALS2(Pals_b8e00);
	lcdc_flip_bg_map(gb);
	gb_call(gb, ADDR_LoadCutscenes4Gfx);
	JUMP_TO(ADDR_LoadCutsceneBGMaps05);
}

PORT_JUMP(InitCutscene14)
{
	PALS1(Pals_b8200);
	PALS2(Pals_b8a40);
	lcdc_flip_bg_map(gb);
	gb_call(gb, ADDR_LoadCutscenes2Gfx);
	gb_call(gb, ADDR_LoadCutsceneBGMaps01);
	JUMP_TO(ADDR_LoadCutsceneBGMaps05);
}

PORT_JUMP(InitCutscene15)
{
	PALS1(Pals_b81c0);
	PALS2(Pals_b8e80);
	JUMP_TO(ADDR_InitCutscene15 + 0x10); // fallthrough
}

PORT_LOCAL_JUMP(InitCutscene15, asm_9c367)
{
	gb->cpu.a = LCDC_WINDOW;
	wr8(gb, wLCDCFlagsToFlip, gb->cpu.a);
	gb->cpu.a = 0x28;
	wr8(gb, rWY, gb->cpu.a);
	wr8(gb, wCutsceneWY, gb->cpu.a);
	gb->cpu.a = 80 + WX_OFS;
	wr8(gb, rWX, gb->cpu.a);
	wr8(gb, wCutsceneWX, gb->cpu.a);
	gb_call(gb, ADDR_LoadCutscenes2Gfx);
	gb_call(gb, ADDR_LoadCutsceneBGMaps14);
	gb_call(gb, ADDR_LoadCutsceneBGMaps13);
	JUMP_TO(ADDR_Func_9da0e);
}

PORT_JUMP(InitCutscene19)
{
	PALS1(Pals_b8140);
	PALS2(Pals_b8940);
	gb_call(gb, ADDR_LoadCutscenes4Gfx);
	JUMP_TO(ADDR_LoadCutsceneBGMaps12);
}

PORT_JUMP(InitCutscene1b)
{
	gb_call(gb, ADDR_LoadCutscenes4Gfx);
	decompress_tiles1(gb, BANK_OwlGfx, ADDR_OwlGfx, v1Tiles0);
	FAR_TILES1(LevelMainTiles28);
	gb_call(gb, ADDR_LoadCutsceneBGMaps03);
	ATTRMAP(Cutscene33Attrmap, v1BGMap1);
	TILEMAP(Cutscene33Tilemap, v0BGMap1);
	PALS1(Pals_b8040);
	PALS2(Pals_b8c80);
	JUMP_TO(ADDR_SetBGPriorityOnTopRows_BGMap1);
}

PORT_JUMP(InitCutscene1e)
{
	PALS1(Pals_b80c0);
	PALS2(Pals_b8cc0);
	gb_call(gb, ADDR_LoadCutscenes3Gfx);
	ATTRMAP(Cutscene09Attrmap, v1BGMap0);
	TILEMAP(Cutscene09Tilemap, v0BGMap0);
	JUMP_TO(ADDR_Func_9d536);
}

PORT_JUMP(InitCutscene22)
{
	gb_call(gb, ADDR_LoadCutscenes3Gfx);
	ATTRMAP(Cutscene10Attrmap, v1BGMap0);
	TILEMAP(Cutscene10Tilemap, v0BGMap0);
	JUMP_TO(ADDR_ClearTempPals_Bank27);
}

PORT_JUMP(InitCutscene24)
{
	PALS1(Pals_b82c0);
	PALS2(Pals_b82c0);
	gb_call(gb, ADDR_LoadCutscenes8Gfx_Vram0);
	gb_call(gb, ADDR_LoadCutsceneBGMaps28);
	FAR_TILES1(SpecialTiles0);
	JUMP_TO(ADDR_Func_9db85);
}

PORT_JUMP(InitCutscene27)
{
	FAR_TILES(LevelMainTiles14);
	FAR_TILES1(SpecialTiles1);
	ATTRMAP(Cutscene11Attrmap, v1BGMap0);
	TILEMAP(Cutscene11Tilemap, v0BGMap0);
	SET_HL(gb, ADDR_Pals_cc5c0);
	SET_DE(gb, wTempPals1);
	gb->cpu.c = 8 * PAL_SIZE;
	gb->cpu.b = BANK_Pals_cc5c0;
	CALL(CopyFarBytes);
	decompress_tiles1(gb, BANK_CartGfx, ADDR_CartGfx, v0Tiles1);
	copy_bytes(gb, ADDR_InitCutscene27 + 0x91, wTempPals2 + 6 * PAL_SIZE, 1 * PAL_SIZE); // .palette
	JUMP_TO(ADDR_Func_9d64b);
}

PORT_JUMP(InitCutscene18)
{
	PALS1(Pals_b8200);
	PALS2(Pals_b8d80);
	gb_call(gb, ADDR_LoadCutscenes1Gfx_Vram0);
	gb_call(gb, ADDR_LoadCutsceneBGMaps01);
	JUMP_TO(ADDR_LoadCutsceneBGMaps02);
}

PORT_JUMP(InitCutscene1f)
{
	PALS1(Pals_b8280);
	PALS2(Pals_b8f00);
	gb_call(gb, ADDR_Func_9c931);
	gb_call(gb, ADDR_LoadCutsceneBGMaps16);
	gb_call(gb, ADDR_SetBGPriorityOnTopRows_Vram0);
	gb_call(gb, ADDR_Func_9cc4f);
	JUMP_TO(ADDR_Func_9dc6a);
}

PORT_JUMP(InitCutscene21)
{
	PALS1(Pals_b8300);
	PALS2(Pals_b8300);
	gb_call(gb, ADDR_Func_9c977);
	FAR_TILES(LevelMainTiles22);
	JUMP_TO(ADDR_LoadCutsceneBGMaps31);
}

PORT_JUMP(InitCutscene37)
{
	PALS1(Pals_b8240);
	PALS2(Pals_b8f40);
	gb_call(gb, ADDR_LoadCutscenes4Gfx);
	JUMP_TO(ADDR_LoadCutsceneBGMaps12);
}

PORT_JUMP(InitCutscene38)
{
	PALS1(Pals_b8200);
	PALS2(Pals_b8f80);
	gb_call(gb, ADDR_Func_9c931);
	JUMP_TO(ADDR_LoadCutsceneBGMaps03);
}

PORT_JUMP(InitCutscene30)
{
	FAR_TILES(LevelMainTiles7);
	gb_call(gb, ADDR_Func_9c977);
	gb_call(gb, ADDR_LoadCutsceneBGMaps26);
	gb_call(gb, ADDR_ClearTempPals_Bank27);
	JUMP_TO(ADDR_Func_9ded4);
}

PORT_JUMP(InitCutscene39)
{
	PALS1(Pals_b8380);
	PALS2(Pals_b8fc0);
	JUMP_TO(ADDR_LoadCutsceneBGMaps15);
}

PORT_JUMP(InitCutscene3a)
{
	gb_call(gb, ADDR_ClearTempPals_Bank27);
	gb_call(gb, ADDR_LoadCutscenes9Gfx_Vram1);
	FAR_TILES(LevelMainTiles30);
	gb_call(gb, ADDR_LoadCutsceneBGMaps37);
	gb_call(gb, ADDR_LoadCutsceneBGMaps38);
	JUMP_TO(ADDR_Func_9e018);
}

PORT_JUMP(InitCutscene3c)
{
	PALS1(Pals_b84c0);
	PALS2(Pals_b84c0);
	gb_call(gb, ADDR_Func_9c977);
	FAR_TILES(LevelMainTiles31);
	gb_call(gb, ADDR_LoadCutsceneBGMaps23);
	JUMP_TO(ADDR_SetBGPriorityOnTopRows_Vram1);
}

PORT_JUMP(InitCutscene43)
{
	PALS1(Pals_b8440);
	PALS2(Pals_b8440);
	FAR_TILES(LevelMainTiles36);
	JUMP_TO(ADDR_LoadCutsceneBGMaps20);
}

PORT_JUMP(InitCutscene54)
{
	gb_call(gb, ADDR_LoadCutscenes9Gfx_Vram1);
	FAR_TILES(LevelMainTiles24);
	gb_call(gb, ADDR_LoadCutsceneBGMaps36);
	gb_call(gb, ADDR_ClearTempPals_Bank27);
	JUMP_TO(ADDR_Func_9e2b8);
}

PORT_JUMP(InitCutscene3d)
{
	PALS1(Pals_b8700);
	PALS2(Pals_b9000);
	gb_call(gb, ADDR_LoadCutscenes9Gfx_Vram0);
	decompress_tiles1(gb, BANK_ParaGoomGfx, ADDR_ParaGoomGfx, v1Tiles0 + 0x40 * TILE_SIZE);
	FAR_TILES1(LevelMainTiles30);
	gb_call(gb, ADDR_LoadCutsceneBGMaps34);
	JUMP_TO(ADDR_LoadCutsceneBGMaps35);
}

PORT_JUMP(InitCutscene4a)
{
	PALS1(Pals_b83c0);
	PALS2(Pals_b9040);
	gb_call(gb, ADDR_Func_9c937);
	gb_call(gb, ADDR_LoadCutsceneBGMaps17);
	JUMP_TO(ADDR_Func_9e438);
}

PORT_JUMP(InitCutscene4c)
{
	gb_call(gb, ADDR_ClearTempPals_Bank27);
	gb_call(gb, ADDR_LoadCutscenes8Gfx_Vram0);
	decompress_tiles1(gb, BANK_FireGfx, ADDR_FireGfx, v1Tiles1);
	gb_call(gb, ADDR_LoadLevelMainTiles29_Vram1);
	gb_call(gb, ADDR_LoadCutsceneBGMaps32);
	JUMP_TO(ADDR_Func_9e4ae);
}

PORT_JUMP(InitCutscene4e)
{
	gb_call(gb, ADDR_ClearTempPals_Bank27);
	gb_call(gb, ADDR_LoadCutscenes1Gfx_Vram1);
	FAR_TILES(LevelMainTiles27);
	JUMP_TO(ADDR_LoadCutsceneBGMaps30);
}

PORT_JUMP(InitCutscene4f)
{
	PALS1(Pals_b8480);
	PALS2(Pals_b8480);
	FAR_TILES(LevelMainTiles23);
	gb_call(gb, ADDR_LoadCutsceneBGMaps22);
	JUMP_TO(ADDR_Func_9e5a1);
}

PORT_JUMP(InitCutscene51)
{
	PALS1(Pals_b8400);
	PALS2(Pals_b8400);
	gb_call(gb, ADDR_Func_9c931);
	gb_call(gb, ADDR_LoadCutsceneBGMaps18);
	gb_call(gb, ADDR_LoadCutsceneBGMaps19);
	gb_call(gb, ADDR_SetBGPriorityOnTopRows_Vram0);
	JUMP_TO(ADDR_Func_9e655);
}

PORT_JUMP(InitCutscene56)
{
	gb_call(gb, ADDR_ClearTempPals_Bank27);
	FAR_TILES(LevelMainTiles11);
	gb_call(gb, ADDR_Func_9c97d);
	gb_call(gb, ADDR_LoadCutsceneBGMaps24);
	gb_call(gb, ADDR_LoadCutsceneBGMaps25);
	JUMP_TO(ADDR_Func_9e6ef);
}

PORT_JUMP(InitCutscene57)
{
	gb_call(gb, ADDR_ClearTempPals_Bank27);
	gb_call(gb, ADDR_Func_9c937);
	gb_call(gb, ADDR_LoadCutsceneBGMaps29);
	gb_call(gb, ADDR_SetBGPriorityOnTopRows_Vram0);
	JUMP_TO(ADDR_Func_9e79a);
}

PORT_JUMP(InitCutscene58)
{
	gb_call(gb, ADDR_ClearTempPals_Bank27);
	gb_call(gb, ADDR_LoadCutscenes9Gfx_Vram0);
	decompress_tiles1(gb, BANK_WallCrackGfx, ADDR_WallCrackGfx, v1Tiles1);
	gb_call(gb, ADDR_LoadLevelMainTiles29_Vram1);
	gb_call(gb, ADDR_LoadCutsceneBGMaps39);
	gb_call(gb, ADDR_LoadCutsceneBGMaps040);
	JUMP_TO(ADDR_Func_9e855);
}

PORT_JUMP(InitCutscene55)
{
	gb_call(gb, ADDR_ClearTempPals_Bank27);
	gb_call(gb, ADDR_LoadLevelMainTiles29_Vram0);
	gb_call(gb, ADDR_LoadCutscenes7Gfx_Vram1);
	gb_call(gb, ADDR_LoadCutsceneBGMaps27);
	JUMP_TO(ADDR_Func_9e91a);
}

PORT(InitCutscene44)
{
	lcdc_flip_bg_map(gb);
	PALS1(Pals_b8200);
	PALS2(Pals_b90c0);
	gb_call(gb, ADDR_Func_9c931);
	decompress_tiles1(gb, BANK_OctohonGfx, ADDR_OctohonGfx, v1Tiles1 + 0x40 * TILE_SIZE);
	gb_call(gb, ADDR_LoadCutsceneBGMaps01);
	gb_call(gb, ADDR_LoadCutsceneBGMaps04);
	gb_call(gb, ADDR_SetBGPriorityOnTopRows_BGMap1);
	gb_call(gb, ADDR_SetBGPriorityOnTopRows_Vram0);
	gb_call(gb, ADDR_Func_9cc4f);
}

// ---- cutscene scene object setups ----

PORT_JUMP(Func_9d01a)
{
	place_scene_obj(gb, wSceneObj2, 72, 80, 0x01, wSceneObj2State);
	JUMP_TO(ADDR_Func_9f11e);
}

PORT_JUMP(Func_9d0e6)
{
	gb->cpu.a = 0x03;
	wr8(gb, wSceneObj1YCoord, gb->cpu.a);
	place_scene_obj(gb, wSceneObj2, 130, 113, 0x01, wSceneObj2State);
	place_scene_obj(gb, wSceneObj3, 130, 47, 0x02, wSceneObj3State);
	JUMP_TO(ADDR_Func_9f146);
}

PORT_JUMP(Func_9d158)
{
	place_scene_obj(gb, wSceneObj2, 88, 84, 0x09, wSceneObj2State);
	place_scene_obj(gb, wSceneObj3, 67, 64, 0x03, wSceneObj3State);
	place_scene_obj(gb, wSceneObj4, 88, 84, 0x0a, wSceneObj4State);
	JUMP_TO(ADDR_Func_9f174);
}

PORT_JUMP(Func_9d536)
{
	place_scene_obj(gb, wSceneObj2, 83, 80, 0x04, wSceneObj2State);
	place_scene_obj(gb, wSceneObj3YCoord, 54, 80, 0x02, wSceneObj3State);
	place_scene_obj(gb, wSceneObj4YCoord, 83, 80, 0x03, wSceneObj4State);
	JUMP_TO(ADDR_Func_9f2a2);
}

PORT_JUMP(Func_9d64b)
{
	place_scene_obj(gb, wSceneObj2, 86, 88, 0x01, wSceneObj2State);
	JUMP_TO(ADDR_Func_9f2f3);
}

PORT_JUMP(Func_9db85)
{
	place_scene_obj(gb, wSceneObj2, 104, 80, 0x01, wSceneObj2State);
	JUMP_TO(ADDR_Func_9f3ee);
}

PORT_JUMP(Func_9da0e)
{
	gb->cpu.a = 0x05;
	SET_HL(gb, wSceneObj4State);
	CALL(SetSceneObjState);
	gb->cpu.a = 96;
	wr8(gb, wSceneObj4 + 0, 96);
	wr8(gb, wSceneObj4 + 1, 80);
	gb->cpu.a = 0x06;
	SET_HL(gb, wSceneObj5State);
	CALL(SetSceneObjState);
	gb->cpu.a = 96;
	wr8(gb, wSceneObj5 + 0, 96);
	wr8(gb, wSceneObj5 + 1, 80);
	SET_HL(gb, wSceneObj5 + 1);
	JUMP_TO(ADDR_Func_9f38d);
}

// ld hl, obj / lb de, y, x / ld a, state / call Func_9ccf9
static void func_9ccf9(GB *gb, uint16_t obj, uint8_t y, uint8_t x, uint8_t state)
{
	SET_HL(gb, obj);
	gb->cpu.d = y;
	gb->cpu.e = x;
	gb->cpu.a = state;
	gb_call(gb, ADDR_Func_9ccf9);
}

PORT_JUMP(Func_9e2b8)
{
	func_9ccf9(gb, wSceneObj2, 0x58, 0x50, 0x01);
	func_9ccf9(gb, wSceneObj3, 0x70, 0x50, 0x02);
	JUMP_TO(ADDR_Func_9f73b);
}

PORT_JUMP(Func_9e438)
{
	func_9ccf9(gb, wSceneObj2, 0x40, 0xb0, 0x02);
	func_9ccf9(gb, wSceneObj3, 0x44, 0x50, 0x01);
	JUMP_TO(ADDR_Func_9f7bb);
}

PORT_JUMP(Func_9e79a)
{
	func_9ccf9(gb, wSceneObj3, 0x4e, 0x54, 0x01);
	func_9ccf9(gb, wSceneObj2, 0x10, 0x48, 0x03);
	JUMP_TO(ADDR_Func_9f991);
}

// ld c, n / jp WaitCutsceneFunc
static void wait_cutscene(GB *gb, uint8_t n)
{
	gb->cpu.c = n;
	JUMP_TO(ADDR_WaitCutsceneFunc);
}

PORT_JUMP(Cutscene1eFunc)
{
	gb_call(gb, ADDR_Cutscene1eFunc + 0x6); // .Func_9d56f
	JUMP_TO(ADDR_Func_9f2a2);
}

PORT_LOCAL_JUMP(Cutscene1eFunc, Func_9d56f)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene1eFunc + 0xa));
}

PORT_LOCAL_JUMP(Cutscene1eFunc, Func_9d589) { wait_cutscene(gb, 15); }
PORT_LOCAL_JUMP(Cutscene1eFunc, Func_9d5a1) { wait_cutscene(gb, 60); }
PORT_LOCAL_JUMP(Cutscene1eFunc, Func_9d5b9) { wait_cutscene(gb, 70); }
PORT_LOCAL_JUMP(Cutscene1eFunc, Func_9d5d2) { wait_cutscene(gb, 30); }

PORT_LOCAL_JUMP(Cutscene1eFunc, Func_9d58e)
{
	SET_HL(gb, wSceneObj3YCoord);
	wr8(gb, wSceneObj3YCoord, alu_inc(gb, rd8(gb, wSceneObj3YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj3YCoord);
	alu_cp(gb, gb->cpu.a, 0x3b);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	play_sfx(gb, SFX_SWITCH);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene1eFunc, Func_9d5a6)
{
	gb->cpu.a = 0x01;
	SET_HL(gb, wSceneObj2State);
	CALL(SetSceneObjState);
	play_sfx(gb, SFX_10E);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

// ld a, [addr] / cp n / ret nz / jp AdvanceCutsceneFunc
static void advance_if(GB *gb, uint16_t addr, uint8_t n)
{
	gb->cpu.a = rd8(gb, addr);
	alu_cp(gb, gb->cpu.a, n);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene1eFunc, Func_9d5be) { advance_if(gb, wSceneObj2Frame, 0x03); }

PORT_LOCAL_JUMP(Cutscene1eFunc, Func_9d5c7)
{
	gb->cpu.a = 0x05;
	SET_HL(gb, wSceneObj2State);
	CALL(SetSceneObjState);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_JUMP(Cutscene22Func)
{
	gb_call(gb, ADDR_Cutscene22Func + 0x6); // .Func_9d5dd
	JUMP_TO(ADDR_Func_9f2cd);
}

PORT_LOCAL_JUMP(Cutscene22Func, Func_9d5dd)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene22Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene22Func, Func_9d5f1)
{
	SET_HL(gb, ADDR_Pals_b8100);
	SET_DE(gb, wTempPals1);
	gb->cpu.c = 8 * PAL_SIZE;
	gb->cpu.b = BANK_Pals_b8100;
	CALL(CopyFarBytes);
	SET_HL(gb, ADDR_Pals_b8d00);
	SET_DE(gb, wTempPals2);
	gb->cpu.c = 8 * PAL_SIZE;
	gb->cpu.b = BANK_Pals_b8d00;
	CALL(CopyFarBytes);
	gb_call(gb, ADDR_Func_9cc87);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene22Func, Func_9d611) { wait_cutscene(gb, 45); }

PORT_LOCAL_JUMP(Cutscene22Func, Func_9d616)
{
	place_scene_obj(gb, wSceneObj2YCoord, 76, 83, 0x01, wSceneObj2State);
	play_sfx(gb, SFX_103);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene22Func, Func_9d631)
{
	gb->cpu.a = rd8(gb, wSceneObj2Frame);
	alu_cp(gb, gb->cpu.a, 0x05);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	play_sfx(gb, SFX_107);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

PORT_LOCAL_JUMP(Cutscene22Func, Func_9d642) { advance_if(gb, wSceneObj2State, 0x02); }

// Scene object animations with per-state hooks. A hook is called with
// a = the state the animation returned; it returns what the asm does next.
enum { ANIM_SPRITE, ANIM_AGAIN, ANIM_RET };
typedef int (*anim_hook)(GB *gb);

// .loop: ld a, [bc] / and a / ret z / ld hl, framesets / call Func_9f0a5 /
// call update / (hook) / ld de, oam / jp sprite
static void anim_loop(GB *gb, uint16_t framesets, uint16_t update, uint16_t oam, uint16_t sprite, anim_hook hook)
{
	for (;;) {
		if (!scene_obj_anim(gb, framesets, update))
			return;
		int r = hook ? hook(gb) : ANIM_SPRITE;
		if (r == ANIM_AGAIN)
			continue;
		if (r == ANIM_RET)
			return;
		break;
	}
	scene_obj_sprite(gb, oam, sprite);
}

static void anim_objs(GB *gb, const uint16_t *objs, int n, uint16_t framesets, uint16_t update,
	uint16_t oam, uint16_t sprite, anim_hook hook)
{
	for (int i = 0; i < n; i++) {
		SET_BC(gb, objs[i]);
		anim_loop(gb, framesets, update, oam, sprite, hook);
	}
}

// cp n (a is the state)
static bool state_is(GB *gb, uint8_t n)
{
	alu_cp(gb, gb->cpu.a, n);
	return gb->cpu.f & FLAG_Z;
}

// ld b, n / call Func_9f0d0 / jr z, .loop
static int next_state(GB *gb, uint8_t b)
{
	gb->cpu.b = b;
	gb_call(gb, ADDR_Func_9f0d0);
	return (gb->cpu.f & FLAG_Z) ? ANIM_AGAIN : ANIM_SPRITE;
}

// ld de, addr / ld a, [de] / inc a / ld [de], a /
// [cp $01 / jr z, .first_sfx] / cp limit / ret c / xor a / ld [de], a /
// [play_sfx wrap_sfx] / ret
static void tick_counter(GB *gb, uint16_t addr, int first_sfx, uint8_t limit, int wrap_sfx)
{
	SET_DE(gb, addr);
	gb->cpu.a = alu_inc(gb, rd8(gb, addr));
	wr8(gb, addr, gb->cpu.a);
	if (first_sfx >= 0) {
		alu_cp(gb, gb->cpu.a, 0x01);
		if (gb->cpu.f & FLAG_Z) {
			play_sfx(gb, (uint16_t)first_sfx);
			return;
		}
	}
	alu_cp(gb, gb->cpu.a, limit);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, addr, 0);
	if (wrap_sfx >= 0)
		play_sfx(gb, (uint16_t)wrap_sfx);
}

// ld a, b / xor $1 / ld [addr], a
static void toggle_b(GB *gb, uint16_t addr)
{
	gb->cpu.a = alu_xor(gb, gb->cpu.b, 0x01);
	wr8(gb, addr, gb->cpu.a);
}

static const uint16_t objs_2[] = { wSceneObj2State };
static const uint16_t objs_3[] = { wSceneObj3State };
static const uint16_t objs_2_4[] = { wSceneObj2State, wSceneObj4State };
static const uint16_t objs_4_2_3[] = { wSceneObj4State, wSceneObj2State, wSceneObj3State };
static const uint16_t objs_2_3_4_5_6_7[] = { wSceneObj2State, wSceneObj3State, wSceneObj4State,
	wSceneObj5State, wSceneObj6State, wSceneObj7State };
static const uint16_t objs_4_6_2_3_5[] = { wSceneObj4State, wSceneObj6State, wSceneObj2State,
	wSceneObj3State, wSceneObj5State };
static const uint16_t objs_10_11_2_3_4_5_6_7_8[] = { wSceneObj10State, wSceneObj11State,
	wSceneObj2State, wSceneObj3State, wSceneObj4State, wSceneObj5State, wSceneObj6State,
	wSceneObj7State, wSceneObj8State };

PORT(Func_9f367)
{
	SET_DE(gb, wSceneObj3);
	gb_call(gb, ADDR_Func_9fb4c);
	anim_objs(gb, objs_2, 1, ADDR_Func_9f367 + 0x24, ADDR_Func_9f0ae, ADDR_OAM_bdc9c, ADDR_Func_9f0bc, NULL);
	SET_DE(gb, wSceneObj3);
	gb_call(gb, ADDR_Func_9fb6c);
}

// Func_9f423.Func_9f45a
static void func_9f45a(GB *gb)
{
	gb->cpu.a = rd8(gb, wSceneObj3Frame);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, wSceneObj3Unk7);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = (gb->cpu.f & FLAG_Z) ? 0x07 : 0x08;
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	toggle_b(gb, wSceneObj3Unk7);
	play_sfx(gb, SFX_FAT_WALK);
}

static int hook_9f423(GB *gb)
{
	if (state_is(gb, 0x02)) {
		func_9f45a(gb);
		return next_state(gb, 0x03);
	}
	if (state_is(gb, 0x03))
		tick_counter(gb, wSceneObj3Unk7, SFX_076, 0x1a, -1);
	return ANIM_SPRITE;
}

PORT(Func_9f423)
{
	anim_objs(gb, objs_2_3, 2, ADDR_Func_9f423 + 0x2f, ADDR_Func_9f0ae, ADDR_OAM_bdfa5, ADDR_Func_9f0bc, hook_9f423);
}

static int hook_9f49c(GB *gb)
{
	if (state_is(gb, 0x01)) {
		tick_counter(gb, wSceneObj2Unk7, -1, 0x0c, SFX_133);
	} else if (state_is(gb, 0x02)) {
		tick_counter(gb, wSceneObj2Unk7, -1, 0x0c, SFX_133);
		tick_counter(gb, wSceneObj3Unk7, -1, 0x10, SFX_132);
	}
	return ANIM_SPRITE;
}

PORT(Func_9f49c)
{
	anim_objs(gb, objs_2_3_4_5, 4, ADDR_Func_9f49c + 0x2f, ADDR_Func_9f0ae, ADDR_OAM_be206, ADDR_Func_9f0bc, hook_9f49c);
}

static int hook_9f508(GB *gb)
{
	if (state_is(gb, 0x01))
		tick_counter(gb, wSceneObj2Unk7, SFX_01E, 0x0e, -1);
	return ANIM_SPRITE;
}

PORT(Func_9f508)
{
	SET_DE(gb, wSceneObj2);
	gb_call(gb, ADDR_Func_9fb97);
	anim_objs(gb, objs_2, 1, ADDR_Func_9f508 + 0x1f, ADDR_Func_9f0ae, ADDR_OAM_be2d9, ADDR_Func_9f0bc, hook_9f508);
}

static int hook_9f547(GB *gb)
{
	if (state_is(gb, 0x02))
		return next_state(gb, 0x03);
	if (state_is(gb, 0x03))
		tick_counter(gb, wSceneObj2Unk7, SFX_040, 0x10, -1);
	return ANIM_SPRITE;
}

PORT(Func_9f547)
{
	SET_DE(gb, wSceneObj2);
	gb_call(gb, ADDR_Func_9fb4c);
	anim_objs(gb, objs_2, 1, ADDR_Func_9f547 + 0x35, ADDR_Func_9f0ae, ADDR_OAM_be3ce, ADDR_Func_9f0bc, hook_9f547);
	SET_DE(gb, wSceneObj2);
	gb_call(gb, ADDR_Func_9fb6c);
}

// Func_9f5a0.Func_9f5e5
static void func_9f5e5(GB *gb)
{
	gb->cpu.a = rd8(gb, wSceneObj2Frame);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, wSceneObj2Unk7);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	bool z = gb->cpu.f & FLAG_Z;
	gb->cpu.a = 0x03;
	alu_cp(gb, gb->cpu.a, gb->cpu.c);
	if (z) {
		if (!(gb->cpu.f & FLAG_Z))
			return;
		play_sfx(gb, SFX_0BA);
	} else if (gb->cpu.f & FLAG_Z) {
		return;
	}
	toggle_b(gb, wSceneObj2Unk7);
}

static int hook_9f5a0(GB *gb)
{
	if (state_is(gb, 0x02)) {
		func_9f5e5(gb);
		return next_state(gb, 0x03);
	}
	return ANIM_SPRITE;
}

PORT(Func_9f5a0)
{
	anim_objs(gb, objs_4_6_2_3_5, 5, ADDR_Func_9f5a0 + 0x3d, ADDR_Func_9f0ae, ADDR_OAM_be807, ADDR_Func_9f0bc, hook_9f5a0);
}

static int hook_9f607(GB *gb)
{
	if (state_is(gb, 0x02))
		sfx_if_animation_finished(gb, SFX_085);
	return ANIM_SPRITE;
}

PORT(Func_9f607)
{
	anim_objs(gb, objs_2, 1, ADDR_Func_9f607 + 0x19, ADDR_Func_9f0ae, ADDR_OAM_be8fe, ADDR_Func_9f0bc, hook_9f607);
}

static int hook_9f638(GB *gb)
{
	if (state_is(gb, 0x04))
		return next_state(gb, 0x00);
	if (state_is(gb, 0x02))
		tick_counter(gb, wSceneObj3Unk7, SFX_10E, 0x48, -1);
	return ANIM_SPRITE;
}

PORT(Func_9f638)
{
	anim_objs(gb, objs_2_3_4_5_6_7, 6, ADDR_Func_9f638 + 0x44, ADDR_Func_9f0ae, ADDR_OAM_bea06, ADDR_Func_9f0bc, hook_9f638);
}

static int hook_9f6a2(GB *gb)
{
	if (state_is(gb, 0x02)) {
		gb->cpu.b = 0x03;
		gb_call(gb, ADDR_Func_9f0d0);
		if (!(gb->cpu.f & FLAG_Z))
			return ANIM_SPRITE;
		gb->cpu.a = 0x04;
		wr8(gb, wSceneObj3State, 0x04);
		return ANIM_AGAIN;
	}
	if (state_is(gb, 0x05))
		return next_state(gb, 0x00);
	if (state_is(gb, 0x06))
		return next_state(gb, 0x07);
	return ANIM_SPRITE;
}

PORT(Func_9f6a2)
{
	anim_objs(gb, objs_4_2_3, 3, ADDR_Func_9f6a2 + 0x48, ADDR_Func_9f0ae, ADDR_OAM_beafc, ADDR_Func_9f0bc, hook_9f6a2);
}

static int hook_9f6f8(GB *gb)
{
	if (state_is(gb, 0x02))
		tick_counter(gb, wSceneObj2Unk7, SFX_07D, 0x0c, -1);
	return ANIM_SPRITE;
}

PORT(Func_9f6f8)
{
	anim_objs(gb, objs_2_3, 2, ADDR_Func_9f6f8 + 0x1f, ADDR_Func_9f0ae, ADDR_OAM_bebef, ADDR_Func_9f0bc, hook_9f6f8);
}

static int hook_9f75c(GB *gb)
{
	if (state_is(gb, 0x02))
		return next_state(gb, 0x00);
	if (state_is(gb, 0x04))
		return next_state(gb, 0x00);
	if (state_is(gb, 0x06))
		return next_state(gb, 0x07);
	return ANIM_SPRITE;
}

PORT(Func_9f75c)
{
	anim_objs(gb, objs_2_4, 2, ADDR_Func_9f75c + 0x38, ADDR_Func_9f0ae, ADDR_OAM_bed42, ADDR_Func_9f0bc, hook_9f75c);
}

static int hook_9f7dc(GB *gb)
{
	if (state_is(gb, 0x02))
		return next_state(gb, 0x03);
	if (state_is(gb, 0x03))
		tick_counter(gb, wSceneObj2Unk7, -1, 0x1a, SFX_076);
	return ANIM_SPRITE;
}

PORT(Func_9f7dc)
{
	anim_objs(gb, objs_2, 1, ADDR_Func_9f7dc + 0x26, ADDR_Func_9f0ae, ADDR_OAM_befd5, ADDR_Func_9f0bc, hook_9f7dc);
}

static int hook_9f86a(GB *gb)
{
	if (state_is(gb, 0x02)) {
		tick_counter(gb, wSceneObj2Unk7, SFX_076, 0x1a, -1);
		return ANIM_SPRITE;
	}
	if (state_is(gb, 0x03)) {
		tick_counter(gb, wSceneObj3Unk7, SFX_0C4, 0x0a, -1);
		return next_state(gb, 0x05);
	}
	return ANIM_SPRITE;
}

PORT(Func_9f86a)
{
	anim_objs(gb, objs_2_3, 2, ADDR_Func_9f86a + 0x2f, ADDR_Func_9f0ae, ADDR_OAM_bf157, ADDR_Func_9f0bc, hook_9f86a);
}

// Func_9f8da.asm_9f8e3: the asm calls .Func_9f912, which can jump back into
// the loop, so the rest of .asm_9f8f9 runs after that inner run returns.
static void func_9f8e3(GB *gb)
{
	if (!scene_obj_anim(gb, ADDR_Func_9f8da + 0x3f, ADDR_Func_9f0ae))
		return;
	if (!state_is(gb, 0x03)) {
		scene_obj_sprite(gb, ADDR_OAM_bf2ab, ADDR_Func_9f0bc);
		return;
	}
	gb->cpu.b = 0x00;
	gb_call(gb, ADDR_Func_9f0d0);
	if (gb->cpu.f & FLAG_Z)
		func_9f8e3(gb);
	else
		scene_obj_sprite(gb, ADDR_OAM_bf2ab, ADDR_Func_9f0bc);
	SET_HL(gb, wSceneObj2Unk7);
	gb->cpu.a = rd8(gb, wSceneObj2Unk7);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.a)
		return;
	gb->cpu.a = rd8(gb, wSceneObj2Frame);
	alu_cp(gb, gb->cpu.a, 0x07);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = LCDC_BG_MAP;
	wr8(gb, wLCDCFlagsToFlip, gb->cpu.a);
	wr8(gb, wSceneObj2Unk7, 0x01);
}

PORT(Func_9f8da)
{
	SET_BC(gb, wSceneObj2State);
	func_9f8e3(gb);
	SET_BC(gb, wSceneObj3State);
	func_9f8e3(gb);
}

// Func_9f91f.Func_9f953
static void func_9f953(GB *gb)
{
	gb->cpu.a = rd8(gb, wSceneObj2Frame);
	gb->cpu.c = gb->cpu.a;
	gb->cpu.a = rd8(gb, wSceneObj2Unk7);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	bool z = gb->cpu.f & FLAG_Z;
	gb->cpu.a = gb->cpu.c;
	if (!z) {
		if (state_is(gb, 0x02) || state_is(gb, 0x04) || state_is(gb, 0x05))
			return;
	} else if (state_is(gb, 0x02)) {
		play_sfx(gb, SFX_011);
	} else if (state_is(gb, 0x04) || state_is(gb, 0x05)) {
		play_sfx(gb, SFX_0F1);
	} else {
		return;
	}
	toggle_b(gb, wSceneObj2Unk7);
}

static int hook_9f91f(GB *gb)
{
	if (state_is(gb, 0x01)) {
		func_9f953(gb);
		return next_state(gb, 0x02);
	}
	if (state_is(gb, 0x02)) {
		func_9f953(gb);
		return next_state(gb, 0x01);
	}
	return ANIM_SPRITE;
}

PORT(Func_9f91f)
{
	anim_objs(gb, objs_2, 1, ADDR_Func_9f91f + 0x30, ADDR_Func_9f0ae, ADDR_OAM_bf460, ADDR_Func_9f0bc, hook_9f91f);
}

static int hook_9f9f5(GB *gb)
{
	if (state_is(gb, 0x01))
		tick_counter(gb, wSceneObj3Unk7, -1, 0x1b, SFX_135);
	else if (state_is(gb, 0x02))
		tick_counter(gb, wSceneObj3Unk7, -1, 0x1b, SFX_080);
	return ANIM_SPRITE;
}

PORT(Func_9f9f5)
{
	anim_objs(gb, objs_3, 1, ADDR_Func_9f9f5 + 0x1d, ADDR_Func_9f0b5, ADDR_OAM_18c806, ADDR_Func_9f0cb, hook_9f9f5);
}

static int hook_9fa48(GB *gb)
{
	if (state_is(gb, 0x02))
		tick_counter(gb, wSceneObj2Unk7, -1, 0x0c, SFX_PLANE1);
	return ANIM_SPRITE;
}

PORT(Func_9fa48)
{
	anim_objs(gb, objs_10_11_2_3_4_5_6_7_8, 9, ADDR_Func_9fa48 + 0x49, ADDR_Func_9f0ae, ADDR_OAM_bf6f0, ADDR_Func_9f0bc, hook_9fa48);
}

static int hook_9fab4(GB *gb)
{
	if (state_is(gb, 0x01))
		tick_counter(gb, wSceneObj2Unk7, -1, 0x30, SFX_08E);
	return ANIM_SPRITE;
}

PORT(Func_9fab4)
{
	anim_objs(gb, objs_2, 1, ADDR_Func_9fab4 + 0x19, ADDR_Func_9f0ae, ADDR_OAM_bf847, ADDR_Func_9f0bc, hook_9fab4);
}

static int hook_9faea(GB *gb)
{
	if (state_is(gb, 0x01)) {
		tick_counter(gb, wSceneObj4YCoord, -1, 0x28, SFX_08F);
		return ANIM_SPRITE;
	}
	if (state_is(gb, 0x03)) {
		tick_counter(gb, wSceneObj4YCoord, -1, 0x28, SFX_083);
		return ANIM_SPRITE;
	}
	if (state_is(gb, 0x02))
		return next_state(gb, 0x03);
	return ANIM_SPRITE;
}

PORT(Func_9faea)
{
	anim_objs(gb, objs_3, 1, ADDR_Func_9faea + 0x2a, ADDR_Func_9f0b5, ADDR_OAM_18a895, ADDR_Func_9f0c1, hook_9faea);
}

// Func_9fba9.Func_9fbc5
static void func_9fbc5(GB *gb)
{
	for (;;) {
		gb->cpu.a = alu_and(gb, rd8(gb, REG_BC(gb)), rd8(gb, REG_BC(gb)));
		if (!gb->cpu.a)
			return;
		bool five = state_is(gb, 0x05);
		SET_HL(gb, ADDR_Func_9fba9 + 0x45);
		gb_call(gb, ADDR_Func_9f0a5);
		gb_call(gb, ADDR_Func_9f0ae);
		if (!five)
			break;
		if (next_state(gb, 0x06) != ANIM_AGAIN)
			break;
	}
	scene_obj_sprite(gb, ADDR_OAM_bcfb3, ADDR_Func_9f0bc);
}

PORT(Func_9fba9)
{
	gb_call(gb, ADDR_Func_9fb87);
	SET_BC(gb, wSceneObj4State);
	func_9fbc5(gb);
	SET_BC(gb, wSceneObj2State);
	func_9fbc5(gb);
	SET_BC(gb, wSceneObj3State);
	func_9fbc5(gb);
	SET_BC(gb, wSceneObj5State);
	func_9fbc5(gb);
}

PORT(Func_9fbfa)
{
	SET_DE(gb, wSceneObj2);
	gb_call(gb, ADDR_Func_9fb4c);
	// .Func_9fc09
	gb->cpu.a = rd8(gb, wSceneObj2State);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.a) {
		SET_DE(gb, ADDR_Frameset_bd1b4);
		SET_HL(gb, wSceneObj2Duration);
		gb_call(gb, ADDR_Func_9f0ae);
		scene_obj_sprite(gb, ADDR_OAM_bcfb3, ADDR_Func_9f0bc);
	}
	SET_DE(gb, wSceneObj2);
	gb_call(gb, ADDR_Func_9fb6c);
}

PORT(Func_9fc1d)
{
	SET_DE(gb, wSceneObj2);
	gb_call(gb, ADDR_Func_9fb4c);
	// .Func_9fc2c
	gb->cpu.a = rd8(gb, wSceneObj2State);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.a) {
		gb->cpu.a = alu_dec(gb, gb->cpu.a);
		SET_HL(gb, ADDR_Func_9fc1d + 0x27);
		CALL(GetPointerFromTableDE);
		SET_HL(gb, wSceneObj2Duration);
		gb_call(gb, ADDR_Func_9f0ae);
		scene_obj_sprite(gb, ADDR_OAM_bc1f6, ADDR_Func_9f0bc);
	}
	SET_DE(gb, wSceneObj2);
	gb_call(gb, ADDR_Func_9fb6c);
}

static void obj12_movement(GB *gb, uint16_t flag, uint16_t data, uint16_t obj, uint16_t state)
{
	gb->cpu.a = rd8(gb, flag);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a)
		return;
	SET_DE(gb, data);
	SET_HL(gb, obj);
	CALL(ApplyOWMovement);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	wr8(gb, flag, gb->cpu.a);
	wr8(gb, state, gb->cpu.a);
}

PORT(Func_9ea9e) { obj12_movement(gb, wSceneObj12YCoord, ADDR_Data_9ec16, wSceneObj10, wSceneObj10State); }
PORT(Func_9eab4) { obj12_movement(gb, wSceneObj12XCoord, ADDR_Data_9ec41, wSceneObj11, wSceneObj11State); }

PORT_JUMP(Func_9cdcd)
{
	static const uint8_t steps[] = { 0xfc, 0x04, 0xfd, 0x03, 0xfe, 0x02, 0xff };
	SET_HL(gb, wCollectionScrollMode);
	wr8(gb, wCollectionScrollMode, alu_inc(gb, rd8(gb, wCollectionScrollMode)));
	gb->cpu.a = rd8(gb, wCollectionLinkStateCounter);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	bool last = true;
	gb->cpu.c = 0x01;
	for (int i = 0; i < 7; i++) {
		if (i)
			gb->cpu.a = alu_dec(gb, gb->cpu.a);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.c = steps[i];
			last = false;
			break;
		}
	}
	gb->cpu.a = rd8(gb, wCollectionScrollMode);
	alu_cp(gb, gb->cpu.a, 0x06);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	SET_HL(gb, wSCY);
	gb->cpu.a = alu_add(gb, gb->cpu.c, rd8(gb, wSCY));
	wr8(gb, wSCY, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCollectionScrollMode, 0);
	SET_HL(gb, wCollectionScrollMode - 1);
	if (!last) {
		wr8(gb, wCollectionScrollMode - 1, alu_inc(gb, rd8(gb, wCollectionScrollMode - 1)));
		RET();
		return;
	}
	wr8(gb, wCollectionScrollMode - 1, 0);
	JUMP_TO(ADDR_AdvanceCutsceneFunc);
}

// Cutscene state machines: CutsceneNNFunc calls its local dispatcher (whose
// rst JumpTable table follows at +$a), then jumps to its animation routine.

static void advance(GB *gb) { JUMP_TO(ADDR_AdvanceCutsceneFunc); }

// ld a, state / ld hl, obj_state / call SetSceneObjState
static void set_scene_obj_state(GB *gb, uint16_t obj_state, uint8_t state)
{
	gb->cpu.a = state;
	SET_HL(gb, obj_state);
	CALL(SetSceneObjState);
}

// set_scene_obj_state / [play_sfx sfx] / jp AdvanceCutsceneFunc
static void set_state_advance(GB *gb, uint16_t obj_state, uint8_t state, int sfx)
{
	set_scene_obj_state(gb, obj_state, state);
	if (sfx >= 0)
		play_sfx(gb, (uint16_t)sfx);
	advance(gb);
}

// cp sfx_at / jp z, Func_9ce1f / cp stop_at / ret nz / jp AdvanceCutsceneFunc
static void sfx_or_advance_at(GB *gb, uint8_t sfx_at, uint8_t stop_at)
{
	alu_cp(gb, gb->cpu.a, sfx_at);
	if (gb->cpu.f & FLAG_Z) {
		JUMP_TO(ADDR_Func_9ce1f);
		return;
	}
	alu_cp(gb, gb->cpu.a, stop_at);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	advance(gb);
}

// ld hl, wSceneObj2YCoord / dec [hl] / ld a, [hli] / dec [hl] / (sfx_or_advance_at)
static void move_obj2_up_left(GB *gb, uint8_t sfx_at, uint8_t stop_at)
{
	wr8(gb, wSceneObj2YCoord, alu_dec(gb, rd8(gb, wSceneObj2YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj2YCoord);
	SET_HL(gb, wSceneObj2YCoord + 1);
	wr8(gb, wSceneObj2YCoord + 1, alu_dec(gb, rd8(gb, wSceneObj2YCoord + 1)));
	sfx_or_advance_at(gb, sfx_at, stop_at);
}

// ld a, n / ld [addr], a / jp AdvanceCutsceneFunc
static void store_advance(GB *gb, uint16_t addr, uint8_t n)
{
	gb->cpu.a = n;
	wr8(gb, addr, n);
	advance(gb);
}

// Cutscene06Func

PORT_JUMP(Cutscene06Func)
{
	gb_call(gb, ADDR_Cutscene06Func + 0x6); // .Func_9d033
	JUMP_TO(ADDR_Func_9f11e);
}

PORT_LOCAL_JUMP(Cutscene06Func, Func_9d033)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene06Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene06Func, Func_9d04b)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	if (state_is(gb, 0x01)) {
		play_sfx(gb, SFX_102);
		RET();
		return;
	}
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_and(gb, gb->cpu.a, 0x03);
	if (gb->cpu.a) {
		RET();
		return;
	}
	gb->cpu.a = alu_bit(gb, gb->cpu.b, 2) ? 0xff : 0x01;
	SET_HL(gb, wSceneObj2YCoord);
	gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, wSceneObj2YCoord));
	wr8(gb, wSceneObj2YCoord, gb->cpu.a);
	gb->cpu.a = gb->cpu.b;
	alu_cp(gb, gb->cpu.a, 0x20);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene06Func, Func_9d071) { wait_cutscene(gb, 30); }

PORT_LOCAL_JUMP(Cutscene06Func, Func_9d076)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	if (state_is(gb, 0x01)) {
		play_sfx(gb, SFX_TEMPLE_ROCK);
		RET();
		return;
	}
	gb->cpu.b = 0xff;
	alu_cp(gb, gb->cpu.a, 0x08);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.b = alu_dec(gb, gb->cpu.b);
		alu_cp(gb, gb->cpu.a, 0x10);
		if (!(gb->cpu.f & FLAG_C))
			gb->cpu.b = alu_dec(gb, gb->cpu.b);
	}
	gb->cpu.a = gb->cpu.b;
	SET_HL(gb, wSceneObj2YCoord);
	gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, wSceneObj2YCoord));
	wr8(gb, wSceneObj2YCoord, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x20);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene06Func, Func_9d09c)
{
	place_scene_obj(gb, wSceneObj2YCoord, 64, 80, 0x02, wSceneObj2State);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene06Func, Func_9d0af)
{
	gb->cpu.a = rd8(gb, wSceneObj2State);
	if (!state_is(gb, 0x03)) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wColourFadeDiff, 0);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene06Func, Func_9d0bc) { wait_cutscene(gb, 100); }

PORT_LOCAL_JUMP(Cutscene06Func, Func_9d0c1)
{
	gb_call(gb, ADDR_Cutscene06Func + 0xa6); // .Func_9d0d3
	gb->cpu.a = alu_and(gb, rd8(gb, REG_BC(gb)), 0x03);
	if (gb->cpu.a) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj2YCoord, alu_dec(gb, rd8(gb, wSceneObj2YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj2YCoord);
	if (!state_is(gb, 0x28)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL(Cutscene06Func, Func_9d0d3)
{
	SET_HL(gb, wColourFadeDiff);
	wr8(gb, wColourFadeDiff, alu_inc(gb, rd8(gb, wColourFadeDiff)));
	gb->cpu.a = rd8(gb, wColourFadeDiff);
	alu_cp(gb, gb->cpu.a, 0x0e);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wColourFadeDiff, 0);
	play_sfx(gb, SFX_023);
}

// Cutscene0aFunc

PORT_JUMP(Cutscene0aFunc)
{
	gb_call(gb, ADDR_Cutscene0aFunc + 0x6); // .Func_9d114
	JUMP_TO(ADDR_Func_9f146);
}

PORT_LOCAL_JUMP(Cutscene0aFunc, Func_9d114)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene0aFunc + 0xa));
}

PORT_LOCAL_JUMP(Cutscene0aFunc, Func_9d126)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	if (!alu_bit(gb, gb->cpu.a, 0)) {
		RET();
		return;
	}
	gb_call(gb, ADDR_Cutscene0aFunc + 0x25); // .Func_9d133
	if (!state_is(gb, 0x63)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL(Cutscene0aFunc, Func_9d133)
{
	gb->cpu.a = alu_dec(gb, rd8(gb, wSceneObj2YCoord));
	wr8(gb, wSceneObj2YCoord, gb->cpu.a);
	wr8(gb, wSceneObj2YCoord + 1, alu_dec(gb, rd8(gb, wSceneObj2YCoord + 1)));
	wr8(gb, wSceneObj3YCoord, gb->cpu.a);
	SET_HL(gb, wSceneObj3YCoord + 1);
	wr8(gb, wSceneObj3YCoord + 1, alu_inc(gb, rd8(gb, wSceneObj3YCoord + 1)));
}

PORT_LOCAL_JUMP(Cutscene0aFunc, Func_9d140) { wait_cutscene(gb, 50); }

PORT_LOCAL_JUMP(Cutscene0aFunc, Func_9d145)
{
	gb_call(gb, ADDR_Cutscene0aFunc + 0x25); // .Func_9d133
	play_sfx(gb, SFX_106);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene0aFunc, Func_9d153) { wait_cutscene(gb, 30); }

// Cutscene27Func

PORT_JUMP(Cutscene27Func)
{
	gb_call(gb, ADDR_Cutscene27Func + 0x6); // .Func_9d664
	JUMP_TO(ADDR_Func_9f2f3);
}

PORT_LOCAL_JUMP(Cutscene27Func, Func_9d664)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene27Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene27Func, Func_9d674) { set_state_advance(gb, wSceneObj2State, 0x02, SFX_CLICK); }
PORT_LOCAL_JUMP(Cutscene27Func, Func_9d687) { wait_cutscene(gb, 50); }
PORT_LOCAL_JUMP(Cutscene27Func, Func_9d68c) { set_state_advance(gb, wSceneObj2State, 0x03, SFX_CLICK); }
PORT_LOCAL_JUMP(Cutscene27Func, Func_9d69f) { wait_cutscene(gb, 180); }

// Cutscene0cFunc

PORT_JUMP(Cutscene0cFunc)
{
	gb_call(gb, ADDR_Cutscene0cFunc + 0x6); // .Func_9d834
	JUMP_TO(ADDR_Func_9f323);
}

PORT_LOCAL_JUMP(Cutscene0cFunc, Func_9d834)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene0cFunc + 0xa));
}

PORT_LOCAL_JUMP(Cutscene0cFunc, Func_9d840)
{
	place_scene_obj(gb, wSceneObj2YCoord, 164, 120, 0x01, wSceneObj2State);
	store_advance(gb, wSceneObj1YCoord, 0x03);
}

PORT_LOCAL_JUMP(Cutscene0cFunc, Func_9d858) { move_obj2_up_left(gb, 0x8a, 0x64); }

// Cutscene18Func

PORT_JUMP(Cutscene18Func)
{
	gb_call(gb, ADDR_Cutscene18Func + 0x6); // .Func_9d86f
	JUMP_TO(ADDR_Func_9f1f3);
}

PORT_LOCAL_JUMP(Cutscene18Func, Func_9d86f)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene18Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene18Func, Func_9d87f)
{
	place_scene_obj(gb, wSceneObj2YCoord, 164, 112, 0x04, wSceneObj2State);
	store_advance(gb, wSceneObj1YCoord, 0x03);
}

PORT_LOCAL_JUMP(Cutscene18Func, Func_9d897) { move_obj2_up_left(gb, 0x8a, 0x64); }
PORT_LOCAL_JUMP(Cutscene18Func, Func_9d8a8) { wait_cutscene(gb, 20); }
PORT_LOCAL_JUMP(Cutscene18Func, Func_9d8ad) { store_advance(gb, wCollectionRow, 0x08); }

// ld hl, addr / dec [hl] / ld a, [hl]
static void dec_load(GB *gb, uint16_t addr)
{
	SET_HL(gb, addr);
	wr8(gb, addr, alu_dec(gb, rd8(gb, addr)));
	gb->cpu.a = rd8(gb, addr);
}

// ld a, [bc] / bit 0, a / ret z (returns false on the ret)
static bool odd_frame(GB *gb)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	if (!alu_bit(gb, gb->cpu.a, 0)) {
		RET();
		return false;
	}
	return true;
}

// Cutscene0dFunc

PORT_JUMP(Cutscene0dFunc)
{
	gb_call(gb, ADDR_Cutscene0dFunc + 0x6); // .Func_9d8bb
	JUMP_TO(ADDR_Func_9f340);
}

PORT_LOCAL_JUMP(Cutscene0dFunc, Func_9d8bb)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene0dFunc + 0xa));
}

PORT_LOCAL_JUMP(Cutscene0dFunc, Func_9d8d9)
{
	place_scene_obj(gb, wSceneObj2YCoord, 156, 72, 0x01, wSceneObj2State);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene0dFunc, Func_9d8ec)
{
	dec_load(gb, wSceneObj2YCoord);
	sfx_or_advance_at(gb, 0x7a, 0x54);
}

PORT_LOCAL_JUMP(Cutscene0dFunc, Func_9d8fc) { wait_cutscene(gb, 20); }
PORT_LOCAL_JUMP(Cutscene0dFunc, Func_9d901) { store_advance(gb, wCollectionRow, 0x08); }
PORT_LOCAL_JUMP(Cutscene0dFunc, Func_9d909) { wait_cutscene(gb, 60); }

PORT_LOCAL_JUMP(Cutscene0dFunc, Func_9d90e)
{
	place_scene_obj(gb, wSceneObj3YCoord, 88, 64, 0x02, wSceneObj3State);
	play_sfx(gb, SFX_117);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene0dFunc, Func_9d929)
{
	if (!odd_frame(gb))
		return;
	dec_load(gb, wSceneObj3XCoord);
	if (!state_is(gb, 0x1c)) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj3State, 0);
	place_scene_obj(gb, wSceneObj4YCoord, 80, 60, 0x03, wSceneObj4State);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene0dFunc, Func_9d94c)
{
	gb_call(gb, ADDR_Cutscene0dFunc + 0xb2); // .Func_9d967
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj4State, 0);
	place_scene_obj(gb, wSceneObj3YCoord, 88, 64, 0x02, wSceneObj3State);
	advance(gb);
}

PORT_LOCAL(Cutscene0dFunc, Func_9d967)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	if (alu_bit(gb, gb->cpu.a, 0))
		return;
	dec_load(gb, wSceneObj4XCoord);
	alu_cp(gb, gb->cpu.a, 0x18);
}

PORT_LOCAL_JUMP(Cutscene0dFunc, Func_9d973)
{
	gb_call(gb, ADDR_Cutscene0dFunc + 0xb2); // .Func_9d967
	if (!state_is(gb, 0x18)) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj4State, 0);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene0dFunc, Func_9d980) { wait_cutscene(gb, 80); }

// Cutscene19Func

PORT_JUMP(Cutscene19Func)
{
	gb_call(gb, ADDR_Cutscene19Func + 0x6); // .Func_9d285
	JUMP_TO(ADDR_Func_9f1d8);
}

PORT_LOCAL_JUMP(Cutscene19Func, Func_9d285)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene19Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene19Func, Func_9d2a9)
{
	place_scene_obj(gb, wSceneObj2YCoord, SCREEN_HEIGHT_PX, 0x50, 0x01, wSceneObj2State);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene19Func, Func_9d2bc)
{
	dec_load(gb, wSceneObj2YCoord);
	sfx_or_advance_at(gb, 0x86, 0x60);
}

PORT_LOCAL_JUMP(Cutscene19Func, Func_9d2cc) { wait_cutscene(gb, 90); }
PORT_LOCAL_JUMP(Cutscene19Func, Func_9d2d1) { set_state_advance(gb, wSceneObj2State, 0x02, SFX_073); }
PORT_LOCAL_JUMP(Cutscene19Func, Func_9d2e4) { wait_cutscene(gb, 30); }

PORT_LOCAL_JUMP(Cutscene19Func, Func_9d2e9)
{
	gb->cpu.a = rd8(gb, wSceneObj2Frame);
	if (!state_is(gb, 0x04)) {
		RET();
		return;
	}
	set_state_advance(gb, wSceneObj2State, 0x03, -1);
}

PORT_LOCAL_JUMP(Cutscene19Func, Func_9d2fa) { wait_cutscene(gb, 60); }

// Cutscene19Func.asm_9d302: loads palette 2 of the BG from pals
static void cutscene19_load_pal(GB *gb, uint16_t pals)
{
	copy_bytes(gb, pals, wTempPals1 + 2 * PAL_SIZE, 1 * PAL_SIZE);
	gb->cpu.a = (uint8_t)((wTempPals1 + 2 * PAL_SIZE) >> 8);
	wr8(gb, wPalConfig1SourceHi, gb->cpu.a);
	gb->cpu.a = (uint8_t)(wTempPals1 + 2 * PAL_SIZE);
	wr8(gb, wPalConfig1SourceLo, gb->cpu.a);
	gb->cpu.a = BGPI_AUTOINC | (2 * PAL_SIZE);
	wr8(gb, wPalConfig1Index, gb->cpu.a);
	gb->cpu.a = 1;
	wr8(gb, wPalConfig1Number, gb->cpu.a);
	gb->cpu.a = (uint8_t)rBGPI;
	wr8(gb, wPalConfig1Register, gb->cpu.a);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene19Func, Func_9d2ff) { cutscene19_load_pal(gb, ADDR_Pals_9fc8b); }
PORT_LOCAL_JUMP(Cutscene19Func, Func_9d326) { wait_cutscene(gb, 8); }
PORT_LOCAL_JUMP(Cutscene19Func, Func_9d32b) { cutscene19_load_pal(gb, ADDR_Pals_9fc93); }
PORT_LOCAL_JUMP(Cutscene19Func, Func_9d330) { cutscene19_load_pal(gb, ADDR_Pals_9fc9b); }
PORT_LOCAL_JUMP(Cutscene19Func, Func_9d335) { cutscene19_load_pal(gb, ADDR_Pals_9fca3); }

// Cutscene13Func

PORT_JUMP(Cutscene13Func)
{
	gb_call(gb, ADDR_Cutscene13Func + 0x6); // .Func_9d98b
	JUMP_TO(ADDR_Func_9f367);
}

PORT_LOCAL_JUMP(Cutscene13Func, Func_9d98b)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene13Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene13Func, Func_9d9a1)
{
	place_scene_obj(gb, wSceneObj2YCoord, 176, 72, 0x01, wSceneObj2State);
	gb->cpu.a = rd8(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj3YCoord, gb->cpu.a);
	gb->cpu.a = rd8(gb, wSceneObj2XCoord);
	wr8(gb, wSceneObj3XCoord, gb->cpu.a);
	store_advance(gb, wSceneObj1YCoord, 0x01);
}

PORT_LOCAL_JUMP(Cutscene13Func, Func_9d9c5)
{
	dec_load(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj3YCoord, gb->cpu.a);
	sfx_or_advance_at(gb, 0x86, 0x60);
}

PORT_LOCAL_JUMP(Cutscene13Func, Func_9d9d8)
{
	dec_load(gb, wSceneObj2YCoord);
	alu_cp(gb, gb->cpu.a, 0x40);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	wr8(gb, wSceneObj2YCoord, 0x40);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene13Func, Func_9d9e5)
{
	if (!odd_frame(gb))
		return;
	SET_HL(gb, wSceneObj3YCoord);
	wr8(gb, wSceneObj3YCoord, alu_inc(gb, rd8(gb, wSceneObj3YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj3YCoord);
	if (!state_is(gb, SCREEN_HEIGHT_PX)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene13Func, Func_9d9f4) { wait_cutscene(gb, 30); }

PORT_LOCAL_JUMP(Cutscene13Func, Func_9d9f9)
{
	SET_HL(gb, wSceneObj2YCoord);
	for (int i = 0; i < 3; i++)
		wr8(gb, wSceneObj2YCoord, alu_inc(gb, rd8(gb, wSceneObj2YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj2YCoord);
	alu_cp(gb, gb->cpu.a, 0xb0);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	play_sfx(gb, SFX_061);
	advance(gb);
}

// --- batch 6: more cutscene state machines ---

// ld a, [bc] / cp n / ret c: true when the frame counter is below n
static bool bc_below(GB *gb, uint8_t n)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, n);
	return gb->cpu.f & FLAG_C;
}

// ld a, [addr] / and a: true when it's zero
static bool test_zero(GB *gb, uint16_t addr)
{
	uint8_t v = rd8(gb, addr);
	gb->cpu.a = alu_and(gb, v, v);
	return gb->cpu.f & FLAG_Z;
}

// ld hl, wSceneObj2YCoord / dec [hl] / ld a, [hl] / (sfx_or_advance_at)
static void dec_obj2_y_sfx(GB *gb, uint8_t sfx_at, uint8_t stop_at)
{
	SET_HL(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj2YCoord, alu_dec(gb, rd8(gb, wSceneObj2YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj2YCoord);
	sfx_or_advance_at(gb, sfx_at, stop_at);
}

// ld a, [addr] / cp n / ret nz / play_sfx sfx / jp AdvanceCutsceneFunc
static void sfx_advance_if(GB *gb, uint16_t addr, uint8_t n, uint16_t sfx)
{
	gb->cpu.a = rd8(gb, addr);
	alu_cp(gb, gb->cpu.a, n);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	play_sfx(gb, sfx);
	advance(gb);
}

// xor a / ld [addr], a / jp AdvanceCutsceneFunc
static void clear_advance(GB *gb, uint16_t addr)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z;
	wr8(gb, addr, 0);
	advance(gb);
}

// Cutscene07Func

PORT_JUMP(Cutscene07Func)
{
	gb_call(gb, ADDR_Cutscene07Func + 0x6); // .Func_9d191
	JUMP_TO(ADDR_Func_9f174);
}

PORT_LOCAL_JUMP(Cutscene07Func, Func_9d191)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene07Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene07Func, Func_9d1c5)
{
	if (bc_below(gb, 0x14)) {
		RET();
		return;
	}
	place_scene_obj(gb, wSceneObj5, 90, 56, 0x01, wSceneObj5State);
	play_sfx(gb, SFX_CLICK);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene07Func, Func_9d1e4) { wait_cutscene(gb, 50); }
PORT_LOCAL_JUMP(Cutscene07Func, Func_9d1e9) { set_state_advance(gb, wSceneObj5State, 0x02, SFX_CLICK); }
PORT_LOCAL_JUMP(Cutscene07Func, Func_9d1fc) { wait_cutscene(gb, 30); }
PORT_LOCAL_JUMP(Cutscene07Func, Func_9d201) { set_state_advance(gb, wSceneObj3State, 0x04, -1); }
PORT_LOCAL_JUMP(Cutscene07Func, Func_9d20c) { wait_cutscene(gb, 40); }
PORT_LOCAL_JUMP(Cutscene07Func, Func_9d211) { set_state_advance(gb, wSceneObj2State, 0x06, -1); }

PORT_LOCAL_JUMP(Cutscene07Func, Func_9d21c)
{
	if (!test_zero(gb, wSceneObj2Frame)) {
		RET();
		return;
	}
	play_sfx(gb, SFX_109);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene07Func, Func_9d22c)
{
	if (test_zero(gb, wSceneObj2Frame)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene07Func, Func_9d234) { wait_cutscene(gb, 50); }
PORT_LOCAL_JUMP(Cutscene07Func, Func_9d239) { set_state_advance(gb, wSceneObj3State, 0x05, -1); }
PORT_LOCAL_JUMP(Cutscene07Func, Func_9d244) { wait_cutscene(gb, 40); }
PORT_LOCAL_JUMP(Cutscene07Func, Func_9d249) { set_state_advance(gb, wSceneObj4State, 0x08, SFX_035); }

PORT_LOCAL_JUMP(Cutscene07Func, Func_9d25c)
{
	if (!test_zero(gb, wSceneObj4State)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene07Func, Func_9d264) { wait_cutscene(gb, 50); }

PORT_LOCAL_JUMP(Cutscene07Func, Func_9d269)
{
	play_sfx(gb, SFX_10B);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene07Func, Func_9d274)
{
	SET_HL(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj2YCoord, alu_dec(gb, rd8(gb, wSceneObj2YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj2YCoord);
	alu_cp(gb, gb->cpu.a, 0x18);
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	advance(gb);
}

// Cutscene11Func (also Cutscene0eFunc)

PORT_JUMP(Cutscene11Func)
{
	gb_call(gb, ADDR_Cutscene11Func + 0x6); // .Func_9d340
	JUMP_TO(ADDR_Func_9f1f3);
}

PORT_LOCAL_JUMP(Cutscene11Func, Func_9d340)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene11Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene11Func, Func_9d36a)
{
	gb->cpu.a = rd8(gb, wEventWithCutscene);
	alu_cp(gb, gb->cpu.a, EVENT_SUMMON_LIGHTNING);
	if (!(gb->cpu.f & FLAG_Z)) {
		advance(gb);
		return;
	}
	store_advance(gb, w2d013, 0x09);
}

PORT_LOCAL_JUMP(Cutscene11Func, Func_9d37a)
{
	place_scene_obj(gb, wSceneObj2YCoord, 172, 88, 0x01, wSceneObj2State);
	store_advance(gb, wSceneObj1YCoord, 0x01);
}

PORT_LOCAL_JUMP(Cutscene11Func, Func_9d392) { dec_obj2_y_sfx(gb, 0x92, 0x6c); }
PORT_LOCAL_JUMP(Cutscene11Func, Func_9d3a2) { wait_cutscene(gb, 60); }

// .asm_9d3aa: hl = pals
static void cutscene11_pals(GB *gb, uint16_t pals)
{
	SET_HL(gb, pals);
	SET_DE(gb, wTempPals1);
	gb->cpu.c = 8 * PAL_SIZE;
	gb->cpu.b = BANK_Pals_b8040;
	CALL(CopyFarBytes);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj1YCoord, 0);
	wr8(gb, wSceneObj2State, 0);
	gb->cpu.a = 0x08;
	wr8(gb, wCollectionRow, 0x08);
	set_pal_config(gb, wPalConfig1SourceHi, wPalConfig1SourceLo, wPalConfig1Index,
		wPalConfig1Number, wPalConfig1Register, wTempPals1, BGPI_AUTOINC | 0, (uint8_t)rBGPI);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene11Func, Func_9d3a7) { cutscene11_pals(gb, ADDR_Pals_b8040); }
PORT_LOCAL_JUMP(Cutscene11Func, Func_9d409) { cutscene11_pals(gb, ADDR_Pals_b8200); }
PORT_LOCAL_JUMP(Cutscene11Func, Func_9d3dc) { wait_cutscene(gb, 30); }

PORT_LOCAL_JUMP(Cutscene11Func, Func_9d3e1)
{
	place_scene_obj(gb, wSceneObj2YCoord, 176, 50, 0x02, wSceneObj2State);
	store_advance(gb, wSceneObj1YCoord, 0x03);
}

PORT_LOCAL_JUMP(Cutscene11Func, Func_9d3f9)
{
	SET_HL(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj2YCoord, alu_dec(gb, rd8(gb, wSceneObj2YCoord)));
	wr8(gb, wSceneObj2YCoord, alu_dec(gb, rd8(gb, wSceneObj2YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj2YCoord);
	alu_cp(gb, gb->cpu.a, 0x74);
	if (gb->cpu.f & FLAG_Z) {
		play_sfx(gb, SFX_113); // .asm_9d438
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x68);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene11Func, Func_9d40e) { wait_cutscene(gb, 30); }

PORT_LOCAL_JUMP(Cutscene11Func, Func_9d413)
{
	place_scene_obj(gb, wSceneObj2YCoord, 164, 240, 0x03, wSceneObj2State);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene11Func, Func_9d426)
{
	wr8(gb, wSceneObj2YCoord, alu_dec(gb, rd8(gb, wSceneObj2YCoord)));
	wr8(gb, wSceneObj2YCoord, alu_dec(gb, rd8(gb, wSceneObj2YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj2YCoord);
	SET_HL(gb, wSceneObj2XCoord);
	wr8(gb, wSceneObj2XCoord, alu_inc(gb, rd8(gb, wSceneObj2XCoord)));
	wr8(gb, wSceneObj2XCoord, alu_inc(gb, rd8(gb, wSceneObj2XCoord)));
	alu_cp(gb, gb->cpu.a, 0x70);
	if (gb->cpu.f & FLAG_Z) {
		play_sfx(gb, SFX_113); // .asm_9d438
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x64);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	advance(gb);
}

// Cutscene1bFunc

PORT_JUMP(Cutscene1bFunc)
{
	gb_call(gb, ADDR_Cutscene1bFunc + 0x9); // .Func_9d44a
	gb_call(gb, ADDR_Func_9f225);
	JUMP_TO(ADDR_Func_9f244);
}

PORT_LOCAL_JUMP(Cutscene1bFunc, Func_9d44a)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene1bFunc + 0xd));
}

PORT_LOCAL_JUMP(Cutscene1bFunc, Func_9d46c)
{
	place_scene_obj(gb, wSceneObj2YCoord, 50, 80, 0x01, wSceneObj2State);
	place_scene_obj(gb, wSceneObj3YCoord, 130, 80, 0x02, wSceneObj3State);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene1bFunc, Func_9d48f)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	if (!alu_bit(gb, gb->cpu.a, 0)) {
		RET();
		return;
	}
	gb_call(gb, ADDR_Cutscene1bFunc + 0x63); // .Func_9d4a4
	alu_cp(gb, gb->cpu.a, 0x5a);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	play_sfx(gb, SFX_CLICK);
	advance(gb);
}

PORT_LOCAL(Cutscene1bFunc, Func_9d4a4)
{
	wr8(gb, wSceneObj2YCoord, alu_inc(gb, rd8(gb, wSceneObj2YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj2YCoord);
	SET_HL(gb, wSceneObj3YCoord);
	wr8(gb, wSceneObj3YCoord, alu_dec(gb, rd8(gb, wSceneObj3YCoord)));
}

PORT_LOCAL_JUMP(Cutscene1bFunc, Func_9d4ae) { wait_cutscene(gb, 90); }
PORT_LOCAL_JUMP(Cutscene1bFunc, Func_9d4b3) { wait_cutscene(gb, 8); }
PORT_LOCAL_JUMP(Cutscene1bFunc, Func_9d4b8) { wait_cutscene(gb, 60); }

PORT_LOCAL_JUMP(Cutscene1bFunc, Func_9d4bd)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj2State, 0);
	wr8(gb, wSceneObj3State, 0);
	gb->cpu.b = BANK_Pals_b8340;
	gb->cpu.c = 8 * PAL_SIZE;
	SET_HL(gb, ADDR_Pals_b8340);
	SET_DE(gb, wTempPals1);
	CALL(CopyFarBytes);
	gb_call(gb, ADDR_Func_9cd40);
	gb->cpu.b = BANK_Pals_b8340;
	gb->cpu.c = 8 * PAL_SIZE;
	SET_HL(gb, ADDR_Pals_b8340);
	SET_DE(gb, wTempPals2);
	CALL(CopyFarBytes);
	gb_call(gb, ADDR_Func_9cd5a);
	store_advance(gb, wCollectionRow, 0x08);
}

PORT_LOCAL_JUMP(Cutscene1bFunc, Func_9d4ec)
{
	place_scene_obj(gb, wSceneObj5, 32, 80, 0x02, wSceneObj5State);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene1bFunc, Func_9d4ff) { wait_cutscene(gb, 20); }

PORT_LOCAL_JUMP(Cutscene1bFunc, Func_9d504)
{
	gb->cpu.b = 0x03;
	SET_HL(gb, wSceneObj5);
	gb->cpu.a = rd8(gb, wSceneObj5);
	alu_cp(gb, gb->cpu.a, 0x44);
	if (!(gb->cpu.f & FLAG_C))
		gb->cpu.b = 0x07;
	gb->cpu.a = alu_and(gb, rd8(gb, w2d014), gb->cpu.b);
	if (gb->cpu.a) {
		RET();
		return;
	}
	wr8(gb, wSceneObj5, alu_inc(gb, rd8(gb, wSceneObj5)));
	gb->cpu.a = rd8(gb, wSceneObj5);
	alu_cp(gb, gb->cpu.a, 0x4e);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene1bFunc, Func_9d51d)
{
	if (bc_below(gb, 0x1e)) {
		RET();
		return;
	}
	set_scene_obj_state(gb, wSceneObj5State, 0x01);
	SET_HL(gb, wSceneObj5);
	gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, wSceneObj5));
	wr8(gb, wSceneObj5, gb->cpu.a);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene1bFunc, Func_9d531) { wait_cutscene(gb, 100); }

// Cutscene09Func

PORT_JUMP(Cutscene09Func)
{
	gb_call(gb, ADDR_Cutscene09Func + 0x6); // .Func_9d6aa
	JUMP_TO(ADDR_Func_9fbfa);
}

PORT_LOCAL_JUMP(Cutscene09Func, Func_9d6aa)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene09Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene09Func, Func_9d6b6)
{
	place_scene_obj(gb, wSceneObj2YCoord, 176, 74, 0x01, wSceneObj2State);
	store_advance(gb, wSceneObj1YCoord, 0x01);
}

PORT_LOCAL_JUMP(Cutscene09Func, Func_9d6ce) { dec_obj2_y_sfx(gb, 0x88, 0x62); }

// Func_9d6f0: b = scene obj 2's first state

PORT_JUMP(Func_9d6f0)
{
	gb_call(gb, ADDR_Func_9d6f0 + 0x6); // .Func_9d6f6
	JUMP_TO(ADDR_Func_9fc1d);
}

PORT_LOCAL_JUMP(Func_9d6f0, Func_9d6f6)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Func_9d6f0 + 0xa));
}

PORT_LOCAL_JUMP(Func_9d6f0, Func_9d70c)
{
	place_scene_obj(gb, wSceneObj2YCoord, 176, 74, gb->cpu.b, wSceneObj2State);
	store_advance(gb, wSceneObj1YCoord, 0x01);
}

PORT_LOCAL_JUMP(Func_9d6f0, Func_9d723) { dec_obj2_y_sfx(gb, 0x88, 0x62); }
PORT_LOCAL_JUMP(Func_9d6f0, Func_9d733) { wait_cutscene(gb, 30); }

PORT_LOCAL_JUMP(Func_9d6f0, Func_9d738)
{
	SET_HL(gb, wSceneObj2State);
	gb->cpu.a = rd8(gb, wSceneObj2State);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_inc(gb, gb->cpu.a);
	CALL(SetSceneObjState);
	advance(gb);
}

PORT_LOCAL_JUMP(Func_9d6f0, Func_9d744) { wait_cutscene(gb, 50); }

PORT_LOCAL_JUMP(Func_9d6f0, Func_9d749)
{
	static const uint8_t states[] = { 0x01, 0x03, 0x05, 0x07, 0x09 };
	static const uint16_t sfx[] = { SFX_11E, SFX_11F, SFX_120, SFX_121, SFX_122 };
	gb_call(gb, ADDR_AdvanceCutsceneFunc);
	gb->cpu.a = alu_dec(gb, rd8(gb, wSceneObj2State));
	for (int i = 0; i < 5; i++) {
		alu_cp(gb, gb->cpu.a, states[i]);
		if (gb->cpu.f & FLAG_Z) {
			play_sfx(gb, sfx[i]);
			break;
		}
	}
	RET();
}

PORT_LOCAL_JUMP(Func_9d6f0, Func_9d792) { wait_cutscene(gb, 254); }

// Cutscene24Func

PORT_JUMP(Cutscene24Func)
{
	gb_call(gb, ADDR_Cutscene24Func + 0x6); // .Func_9db9e
	JUMP_TO(ADDR_Func_9f3ee);
}

PORT_LOCAL_JUMP(Cutscene24Func, Func_9db9e)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene24Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene24Func, Func_9dbb2)
{
	if (bc_below(gb, 0x28)) {
		RET();
		return;
	}
	place_scene_obj(gb, wSceneObj3YCoord, 70, 16, 0x02, wSceneObj3State);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene24Func, Func_9dbc9)
{
	SET_HL(gb, wSceneObj3XCoord);
	wr8(gb, wSceneObj3XCoord, alu_inc(gb, rd8(gb, wSceneObj3XCoord)));
	gb->cpu.a = rd8(gb, wSceneObj3XCoord);
	alu_cp(gb, gb->cpu.a, 0x30);
	if (gb->cpu.f & FLAG_Z) {
		play_sfx(gb, SFX_128);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, SCREEN_WIDTH_PX);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	clear_advance(gb, wSceneObj3State);
}

PORT_LOCAL_JUMP(Cutscene24Func, Func_9dbe5)
{
	if (bc_below(gb, 0x46)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj2YCoord, alu_inc(gb, rd8(gb, wSceneObj2YCoord)));
	set_state_advance(gb, wSceneObj2State, 0x03, -1);
}

PORT_LOCAL_JUMP(Cutscene24Func, Func_9dbf8) { wait_cutscene(gb, 40); }

PORT_LOCAL_JUMP(Cutscene24Func, Func_9dbfd)
{
	SET_HL(gb, w2d014);
	gb_call(gb, ADDR_Cutscene24Func + 0x76); // .Func_9dc0e
	SET_HL(gb, wSceneObj2YCoord);
	gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, wSceneObj2YCoord));
	wr8(gb, wSceneObj2YCoord, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x88);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	advance(gb);
}

// hl = counter: clamps it to $40 once it reaches $44, returns a = .data_9dc1f[counter]
PORT_LOCAL(Cutscene24Func, Func_9dc0e)
{
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 0x44);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = 0x40;
		wr8(gb, hl, 0x40);
	}
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	hl = alu_add16(gb, ADDR_Cutscene24Func + 0x87, REG_DE(gb)); // .data_9dc1f
	gb->cpu.a = rd8(gb, hl);
	SET_HL(gb, hl);
}

PORT_LOCAL_JUMP(Cutscene24Func, Func_9dc63) { clear_advance(gb, wSceneObj2State); }

PORT_JUMP(Func_9dc6a)
{
	place_scene_obj(gb, wSceneObj3, 96, 112, 0x04, wSceneObj3State);
	JUMP_TO(ADDR_Func_9f423);
}

// Cutscene1fFunc

PORT_JUMP(Cutscene1fFunc)
{
	gb_call(gb, ADDR_Cutscene1fFunc + 0x6); // .Func_9dc83
	JUMP_TO(ADDR_Func_9f423);
}

PORT_LOCAL_JUMP(Cutscene1fFunc, Func_9dc83)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene1fFunc + 0xa));
}

PORT_LOCAL_JUMP(Cutscene1fFunc, Func_9dc99)
{
	place_scene_obj(gb, wSceneObj2YCoord, 36, 42, 0x01, wSceneObj2State);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene1fFunc, Func_9dcac)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	if (!alu_bit(gb, gb->cpu.a, 0)) {
		RET();
		return;
	}
	wr8(gb, wSceneObj2YCoord, alu_inc(gb, rd8(gb, wSceneObj2YCoord)));
	SET_HL(gb, inc_l(wSceneObj2YCoord));
	wr8(gb, REG_HL(gb), alu_inc(gb, rd8(gb, REG_HL(gb))));
	gb->cpu.a = rd8(gb, REG_HL(gb));
	alu_cp(gb, gb->cpu.a, 0x2e);
	if (gb->cpu.f & FLAG_Z) {
		play_sfx(gb, SFX_11D);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x4a);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene1fFunc, Func_9dcca)
{
	if (bc_below(gb, 0x46)) {
		RET();
		return;
	}
	clear_advance(gb, wSceneObj2State);
}

PORT_LOCAL_JUMP(Cutscene1fFunc, Func_9dcd5)
{
	if (bc_below(gb, 0x1e)) {
		RET();
		return;
	}
	set_state_advance(gb, wSceneObj3State, 0x02, -1);
}

PORT_LOCAL_JUMP(Cutscene1fFunc, Func_9dce4) { sfx_advance_if(gb, wSceneObj3Frame, 0x0a, SFX_FAT_WALK); }
PORT_LOCAL_JUMP(Cutscene1fFunc, Func_9dcf5) { advance_if(gb, wSceneObj3State, 0x03); }
PORT_LOCAL_JUMP(Cutscene1fFunc, Func_9dcfe) { wait_cutscene(gb, 40); }

// ld hl, obj / ld a, y / ld [hli], a / ld [hl], x
static void set_yx(GB *gb, uint16_t obj, uint8_t y, uint8_t x)
{
	gb->cpu.a = y;
	wr8(gb, obj, y);
	wr8(gb, obj + 1, x);
	SET_HL(gb, obj + 1);
}

// cp b / ret nz / jp AdvanceCutsceneFunc
static void advance_if_b(GB *gb)
{
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	advance(gb);
}

// Cutscene21Func

PORT_JUMP(Cutscene21Func)
{
	gb_call(gb, ADDR_Cutscene21Func + 0x6); // .Func_9dd09
	JUMP_TO(ADDR_Func_9f49c);
}

PORT_LOCAL_JUMP(Cutscene21Func, Func_9dd09)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene21Func + 0xa));
}

PORT_LOCAL(Cutscene21Func, Func_9dd3f)
{
	SET_HL(gb, wSceneObj2YCoord);
	gb->cpu.a = alu_and(gb, rd8(gb, REG_BC(gb)), 0x03);
	if (!gb->cpu.a)
		wr8(gb, wSceneObj2YCoord, alu_dec(gb, rd8(gb, wSceneObj2YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj2YCoord);
}

// moves scene obj 2 right (dir 1) or left (dir -1) on odd frames
static void cutscene21_move_x(GB *gb, int dir)
{
	SET_HL(gb, wSceneObj2XCoord);
	gb->cpu.a = rd8(gb, REG_BC(gb));
	if (alu_bit(gb, gb->cpu.a, 0)) {
		uint8_t v = rd8(gb, wSceneObj2XCoord);
		wr8(gb, wSceneObj2XCoord, dir > 0 ? alu_inc(gb, v) : alu_dec(gb, v));
	}
	gb->cpu.a = rd8(gb, wSceneObj2XCoord);
}

PORT_LOCAL(Cutscene21Func, Func_9dd4a) { cutscene21_move_x(gb, 1); }
PORT_LOCAL(Cutscene21Func, Func_9dd55) { cutscene21_move_x(gb, -1); }

PORT_LOCAL_JUMP(Cutscene21Func, Func_9dd60)
{
	set_yx(gb, wSceneObj3YCoord, 72, 72);
	set_yx(gb, wSceneObj4YCoord, 72, 88);
	set_yx(gb, wSceneObj5YCoord, 72, 104);
	set_yx(gb, wSceneObj2YCoord, 128, 72);
	set_state_advance(gb, wSceneObj2State, 0x01, -1); // fallthrough to .Func_9dd80
}

PORT_LOCAL_JUMP(Cutscene21Func, Func_9dd80) { set_state_advance(gb, wSceneObj2State, 0x01, -1); }

PORT_LOCAL_JUMP(Cutscene21Func, Func_9dd8b)
{
	gb_call(gb, ADDR_Cutscene21Func + 0x3c); // .Func_9dd3f
	alu_cp(gb, gb->cpu.a, 0x48);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	set_state_advance(gb, wSceneObj2State, 0x02, -1);
}

PORT_LOCAL_JUMP(Cutscene21Func, Func_9dd9c) { wait_cutscene(gb, 60); }
PORT_LOCAL_JUMP(Cutscene21Func, Func_9dda1) { store_advance(gb, wSceneObj3State, 0x03); }
PORT_LOCAL_JUMP(Cutscene21Func, Func_9ddb3) { store_advance(gb, wSceneObj4State, 0x03); }
PORT_LOCAL_JUMP(Cutscene21Func, Func_9ddc2) { store_advance(gb, wSceneObj5State, 0x03); }
PORT_LOCAL_JUMP(Cutscene21Func, Func_9dddf) { store_advance(gb, wSceneObj5State, 0x04); }
PORT_LOCAL_JUMP(Cutscene21Func, Func_9ddea) { store_advance(gb, wSceneObj4State, 0x04); }
PORT_LOCAL_JUMP(Cutscene21Func, Func_9ddf5) { store_advance(gb, wSceneObj3State, 0x04); }

// call move / ld b, stop / jr .asm_9ddae
static void cutscene21_move_until(GB *gb, uint16_t move, uint8_t stop)
{
	gb_call(gb, move);
	gb->cpu.b = stop;
	advance_if_b(gb);
}

PORT_LOCAL_JUMP(Cutscene21Func, Func_9dda9) { cutscene21_move_until(gb, ADDR_Cutscene21Func + 0x47, 0x58); }
PORT_LOCAL_JUMP(Cutscene21Func, Func_9ddbb) { cutscene21_move_until(gb, ADDR_Cutscene21Func + 0x47, 0x68); }
PORT_LOCAL_JUMP(Cutscene21Func, Func_9ddd8) { cutscene21_move_until(gb, ADDR_Cutscene21Func + 0x52, 0x68); }
PORT_LOCAL_JUMP(Cutscene21Func, Func_9dde3) { cutscene21_move_until(gb, ADDR_Cutscene21Func + 0x52, 0x58); }
PORT_LOCAL_JUMP(Cutscene21Func, Func_9ddee) { cutscene21_move_until(gb, ADDR_Cutscene21Func + 0x52, 0x48); }
PORT_LOCAL_JUMP(Cutscene21Func, Func_9ddf9) { cutscene21_move_until(gb, ADDR_Cutscene21Func + 0x3c, 0x20); }

PORT_LOCAL_JUMP(Cutscene21Func, Func_9ddca)
{
	gb_call(gb, ADDR_Cutscene21Func + 0x47); // .Func_9dd4a
	alu_cp(gb, gb->cpu.a, 0x88);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	store_advance(gb, wSceneObj2YCoord, 0x38);
}

PORT_LOCAL_JUMP(Cutscene21Func, Func_9de00) { clear_advance(gb, wSceneObj2State); }

// Cutscene37Func

PORT_JUMP(Cutscene37Func)
{
	gb_call(gb, ADDR_Cutscene37Func + 0x6); // .Func_9de0d
	JUMP_TO(ADDR_Func_9f508);
}

PORT_LOCAL_JUMP(Cutscene37Func, Func_9de0d)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene37Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene37Func, Func_9de1b)
{
	place_scene_obj(gb, wSceneObj2YCoord, 192, 50, 0x01, wSceneObj2State);
	store_advance(gb, wSceneObj1YCoord, 0x03);
}

PORT_LOCAL_JUMP(Cutscene37Func, Func_9de33) { wait_cutscene(gb, 40); }
PORT_LOCAL_JUMP(Cutscene37Func, Func_9de38) { dec_obj2_y_sfx(gb, 0x8e, 0x68); }

// Cutscene38Func

PORT_JUMP(Cutscene38Func)
{
	gb_call(gb, ADDR_Cutscene38Func + 0x6); // .Func_9de4e
	JUMP_TO(ADDR_Func_9f547);
}

PORT_LOCAL_JUMP(Cutscene38Func, Func_9de4e)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene38Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene38Func, Func_9de68)
{
	if (bc_below(gb, 0x1e)) {
		RET();
		return;
	}
	place_scene_obj(gb, wSceneObj2YCoord, 152, 72, 0x01, wSceneObj2State);
	gb->cpu.a = 0x01;
	wr8(gb, wSceneObj1YCoord, 0x01);
	gb_call(gb, ADDR_Func_9ce1f);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene38Func, Func_9de87)
{
	SET_HL(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj2YCoord, alu_dec(gb, rd8(gb, wSceneObj2YCoord)));
	advance_if(gb, wSceneObj2YCoord, 0x78);
}

PORT_LOCAL_JUMP(Cutscene38Func, Func_9de92) { set_state_advance(gb, wSceneObj2State, 0x02, -1); }
PORT_LOCAL_JUMP(Cutscene38Func, Func_9de9d) { sfx_advance_if(gb, wSceneObj2Frame, 0x01, SFX_SWITCH); }
PORT_LOCAL_JUMP(Cutscene38Func, Func_9deae) { sfx_advance_if(gb, wSceneObj2Frame, 0x05, SFX_106); }
PORT_LOCAL_JUMP(Cutscene38Func, Func_9debf) { advance_if(gb, wSceneObj2State, 0x03); }
PORT_LOCAL_JUMP(Cutscene38Func, Func_9dec8) { wait_cutscene(gb, 120); }
PORT_LOCAL_JUMP(Cutscene38Func, Func_9decd) { clear_advance(gb, wSceneObj2State); }

PORT_JUMP(Func_9ded4)
{
	place_scene_obj(gb, wSceneObj2, 88, 98, 0x03, wSceneObj2State);
	place_scene_obj(gb, wSceneObj3, 88, 70, 0x01, wSceneObj3State);
	set_yx(gb, wSceneObj4, 72, 82);
	set_yx(gb, wSceneObj5, 68, 70);
	set_yx(gb, wSceneObj6, 60, 52);
	JUMP_TO(ADDR_Func_9f5a0);
}

// Cutscene30Func

PORT_JUMP(Cutscene30Func)
{
	gb_call(gb, ADDR_Cutscene30Func + 0x6); // .Func_9df15
	JUMP_TO(ADDR_Func_9f5a0);
}

PORT_LOCAL_JUMP(Cutscene30Func, Func_9df15)
{
	gb_call(gb, ADDR_Cutscene30Func + 0xa7); // .Func_9dfb6
	gb_call(gb, ADDR_Cutscene30Func + 0x97); // .Func_9dfa6
	gb_call(gb, ADDR_Cutscene30Func + 0x9f); // .Func_9dfae
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene30Func + 0x13));
}

PORT_LOCAL_JUMP(Cutscene30Func, Func_9df3c)
{
	gb_call(gb, ADDR_Func_9cc87);
	PALS1(Pals_b8500);
	PALS2(Pals_b8500);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene30Func, Func_9df52) { wait_cutscene(gb, 10); }
PORT_LOCAL_JUMP(Cutscene30Func, Func_9df57) { set_state_advance(gb, wSceneObj2State, 0x02, -1); }
PORT_LOCAL_JUMP(Cutscene30Func, Func_9df62) { advance_if(gb, wSceneObj2State, 0x03); }
PORT_LOCAL_JUMP(Cutscene30Func, Func_9df6b) { wait_cutscene(gb, 30); }

PORT_LOCAL_JUMP(Cutscene30Func, Func_9df70)
{
	gb->cpu.a = alu_and(gb, rd8(gb, w2d014), 0x07);
	if (gb->cpu.a) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj3YCoord);
	wr8(gb, wSceneObj3YCoord, alu_dec(gb, rd8(gb, wSceneObj3YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj3YCoord);
	static const uint8_t ys[] = { 0x48, 0x44, 0x3c };
	static const uint16_t states[] = { wSceneObj4State, wSceneObj5State, wSceneObj6State };
	for (int i = 0; i < 3; i++) {
		alu_cp(gb, gb->cpu.a, ys[i]);
		if (gb->cpu.f & FLAG_Z) {
			SET_HL(gb, states[i]);
			gb->cpu.a = 0x04;
			wr8(gb, states[i], 0x04);
			play_sfx(gb, SFX_00F);
			RET();
			return;
		}
	}
	alu_cp(gb, gb->cpu.a, 0x20);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	advance(gb);
}

// .asm_9dfbc: de = obj state, hl = obj: moves the obj up every other
// frame while it's on, and turns it off at y $20
static void cutscene30_rise(GB *gb, uint16_t state, uint16_t obj)
{
	SET_DE(gb, state);
	SET_HL(gb, obj);
	uint8_t v = rd8(gb, state);
	gb->cpu.a = alu_and(gb, v, v);
	if (gb->cpu.f & FLAG_Z)
		return;
	gb->cpu.a = alu_and(gb, rd8(gb, w2d014), 0x01);
	if (gb->cpu.a)
		return;
	wr8(gb, obj, alu_dec(gb, rd8(gb, obj)));
	gb->cpu.a = rd8(gb, obj);
	alu_cp(gb, gb->cpu.a, 0x20);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, state, 0);
}

PORT_LOCAL(Cutscene30Func, Func_9dfa6) { cutscene30_rise(gb, wSceneObj5State, wSceneObj5); }
PORT_LOCAL(Cutscene30Func, Func_9dfae) { cutscene30_rise(gb, wSceneObj6State, wSceneObj6); }
PORT_LOCAL(Cutscene30Func, Func_9dfb6) { cutscene30_rise(gb, wSceneObj4State, wSceneObj4); }

// Cutscene39Func

PORT_JUMP(Cutscene39Func)
{
	gb_call(gb, ADDR_Cutscene39Func + 0x6); // .Func_9dfd3
	JUMP_TO(ADDR_Func_9f607);
}

PORT_LOCAL_JUMP(Cutscene39Func, Func_9dfd3)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene39Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene39Func, Func_9dfe3)
{
	func_9ccf9(gb, wSceneObj2, 0x18, 0x54, 0x01);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene39Func, Func_9dff1)
{
	SET_HL(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj2YCoord, alu_inc(gb, rd8(gb, wSceneObj2YCoord)));
	sfx_advance_if(gb, wSceneObj2YCoord, 0x50, SFX_CLICK);
}

PORT_LOCAL_JUMP(Cutscene39Func, Func_9e004)
{
	if (bc_below(gb, 0x2d)) {
		RET();
		return;
	}
	set_state_advance(gb, wSceneObj2State, 0x02, -1);
}

PORT_LOCAL_JUMP(Cutscene39Func, Func_9e013) { wait_cutscene(gb, 100); }

PORT_JUMP(Func_9e018)
{
	func_9ccf9(gb, wSceneObj2, 0x54, 0x46, 0x01);
	set_yx(gb, wSceneObj4, 80, 56);
	set_yx(gb, wSceneObj5, 80, 72);
	set_yx(gb, wSceneObj6, 80, 88);
	set_yx(gb, wSceneObj7, 80, 104);
	JUMP_TO(ADDR_Func_9f638);
}

// ld a, [bc] / and mask: true when the result is zero
static bool bc_and_zero(GB *gb, uint8_t mask)
{
	gb->cpu.a = alu_and(gb, rd8(gb, REG_BC(gb)), mask);
	return gb->cpu.f & FLAG_Z;
}

// call Func_9cc87 / load pals into TempPals1 and TempPals2 / jp AdvanceCutsceneFunc
#define CLEAR_LOAD_PALS_ADVANCE(p) do { \
	gb_call(gb, ADDR_Func_9cc87); \
	PALS1(p); \
	PALS2(p); \
	advance(gb); \
} while (0)

// Cutscene3aFunc

PORT_JUMP(Cutscene3aFunc)
{
	gb_call(gb, ADDR_Cutscene3aFunc + 0x6); // .Func_9e04c
	JUMP_TO(ADDR_Func_9f638);
}

PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e04c)
{
	gb_call(gb, ADDR_Cutscene3aFunc + 0x10d); // .Func_9e153
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene3aFunc + 0xd));
}

PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e07d) { CLEAR_LOAD_PALS_ADVANCE(Pals_b8780); }
PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e093) { wait_cutscene(gb, 45); }

PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e098)
{
	if (!bc_and_zero(gb, 0x01)) {
		RET();
		return;
	}
	wr8(gb, wSceneObj2YCoord, alu_dec(gb, rd8(gb, wSceneObj2YCoord)));
	wr8(gb, wSceneObj2YCoord, alu_dec(gb, rd8(gb, wSceneObj2YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj2YCoord);
	SET_HL(gb, wSceneObj2XCoord);
	wr8(gb, wSceneObj2XCoord, alu_inc(gb, rd8(gb, wSceneObj2XCoord)));
	alu_cp(gb, gb->cpu.a, 0x50);
	if (gb->cpu.f & FLAG_Z) {
		play_sfx(gb, SFX_SWITCH);
		RET();
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x4c);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e0b6)
{
	if (bc_below(gb, 0x2d)) {
		RET();
		return;
	}
	func_9ccf9(gb, wSceneObj3, 0x3d, 0x52, 0x02);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e0c8)
{
	if (bc_below(gb, 0x8c)) {
		RET();
		return;
	}
	gb->cpu.a = 0x08;
	wr8(gb, wCollectionRow, 0x08);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj3State, 0);
	wr8(gb, wSceneObj2State, 0);
	gb->cpu.a = 0xff; // stop_sfx
	wr8(gb, hSFXID + 0, 0xff);
	gb->cpu.a = 0x00;
	wr8(gb, hSFXID + 1, 0x00);
	set_scene_obj_state(gb, wSceneObj4State, 0x03);
	set_scene_obj_state(gb, wSceneObj5State, 0x03);
	set_scene_obj_state(gb, wSceneObj6State, 0x03);
	set_scene_obj_state(gb, wSceneObj7State, 0x03);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e103) { wait_cutscene(gb, 80); }

// .asm_9e10b: hl = obj frame
static void cutscene3a_frame(GB *gb, uint16_t frame)
{
	SET_HL(gb, frame);
	advance_if(gb, frame, 0x0a);
}

// .asm_9e115: hl = obj state
static void cutscene3a_state4(GB *gb, uint16_t state)
{
	set_state_advance(gb, state, 0x04, -1);
}

// .asm_9e120: hl = obj state
static void cutscene3a_wait_off(GB *gb, uint16_t state)
{
	SET_HL(gb, state);
	if (!test_zero(gb, state)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e108) { cutscene3a_frame(gb, wSceneObj4Frame); }
PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e112) { cutscene3a_state4(gb, wSceneObj4State); }
PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e11d) { cutscene3a_wait_off(gb, wSceneObj4State); }
PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e126) { cutscene3a_frame(gb, wSceneObj5Frame); }
PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e12b) { cutscene3a_state4(gb, wSceneObj5State); }
PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e130) { cutscene3a_wait_off(gb, wSceneObj5State); }
PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e135) { cutscene3a_frame(gb, wSceneObj6Frame); }
PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e13a) { cutscene3a_state4(gb, wSceneObj6State); }
PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e13f) { cutscene3a_wait_off(gb, wSceneObj6State); }
PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e144) { cutscene3a_frame(gb, wSceneObj7Frame); }
PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e149) { cutscene3a_state4(gb, wSceneObj7State); }
PORT_LOCAL_JUMP(Cutscene3aFunc, Func_9e14e) { cutscene3a_wait_off(gb, wSceneObj7State); }

PORT_LOCAL(Cutscene3aFunc, Func_9e153)
{
	gb->cpu.a = rd8(gb, wSceneObj7State);
	if (!state_is(gb, 0x03))
		return;
	SET_HL(gb, wSceneObj7Unk7);
	wr8(gb, wSceneObj7Unk7, alu_inc(gb, rd8(gb, wSceneObj7Unk7)));
	gb->cpu.a = rd8(gb, wSceneObj7Unk7);
	if (state_is(gb, 0x01)) {
		play_sfx(gb, SFX_080);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x1b);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj7Unk7, 0);
}

// Cutscene3cFunc

PORT_JUMP(Cutscene3cFunc)
{
	gb_call(gb, ADDR_Cutscene3cFunc + 0x6); // .Func_9e177
	JUMP_TO(ADDR_Func_9f6a2);
}

PORT_LOCAL_JUMP(Cutscene3cFunc, Func_9e177)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene3cFunc + 0xa));
}

PORT_LOCAL_JUMP(Cutscene3cFunc, Func_9e18f)
{
	func_9ccf9(gb, wSceneObj2, 0x20, 0x42, 0x01);
	func_9ccf9(gb, wSceneObj3, 0x3e, 0x50, 0x00);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene3cFunc, Func_9e1a8)
{
	if (bc_and_zero(gb, 0x01)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj2YCoord, alu_inc(gb, rd8(gb, wSceneObj2YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj2YCoord);
	sfx_or_advance_at(gb, 0x27, 0x3a);
}

PORT_LOCAL_JUMP(Cutscene3cFunc, Func_9e1bc)
{
	if (bc_below(gb, 0x3c)) {
		RET();
		return;
	}
	set_state_advance(gb, wSceneObj2State, 0x02, -1);
}

PORT_LOCAL_JUMP(Cutscene3cFunc, Func_9e1cb) { advance_if(gb, wSceneObj2State, 0x03); }

PORT_LOCAL_JUMP(Cutscene3cFunc, Func_9e1d4)
{
	gb_call(gb, ADDR_Cutscene3cFunc + 0x83); // .Func_9e1f4
	SET_HL(gb, wSceneObj3YCoord);
	gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, wSceneObj3YCoord));
	wr8(gb, wSceneObj3YCoord, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x58);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	wr8(gb, wSceneObj3YCoord, 0x58);
	set_state_advance(gb, wSceneObj3State, 0x05, SFX_0A1);
}

// bc = frame counter: clamps it to $28 once it reaches $2c,
// returns a = .data_9e205[counter] (and bc = counter)
PORT_LOCAL(Cutscene3cFunc, Func_9e1f4)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x2c);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.a = 0x28;
		wr8(gb, REG_BC(gb), 0x28);
	}
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = 0;
	uint16_t hl = alu_add16(gb, ADDR_Cutscene3cFunc + 0x94, REG_BC(gb)); // .data_9e205
	gb->cpu.a = rd8(gb, hl);
	SET_HL(gb, hl);
}

PORT_LOCAL_JUMP(Cutscene3cFunc, Func_9e231)
{
	if (!test_zero(gb, wSceneObj3State)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene3cFunc, Func_9e239)
{
	if (bc_below(gb, 0x3c)) {
		RET();
		return;
	}
	func_9ccf9(gb, wSceneObj4, 0x58, 0x50, 0x06);
	play_sfx(gb, SFX_074);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene3cFunc, Func_9e253) { advance_if(gb, wSceneObj4State, 0x07); }

// Cutscene43Func

PORT_JUMP(Cutscene43Func)
{
	gb_call(gb, ADDR_Cutscene43Func + 0x6); // .Func_9e262
	JUMP_TO(ADDR_Func_9f6f8);
}

PORT_LOCAL_JUMP(Cutscene43Func, Func_9e262)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene43Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene43Func, Func_9e272)
{
	func_9ccf9(gb, wSceneObj2, 0x20, 0x50, 0x01);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene43Func, Func_9e280)
{
	if (!bc_and_zero(gb, 0x01)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj2YCoord, alu_inc(gb, rd8(gb, wSceneObj2YCoord)));
	sfx_advance_if(gb, wSceneObj2YCoord, 0x50, SFX_CLICK);
}

PORT_LOCAL_JUMP(Cutscene43Func, Func_9e297)
{
	if (bc_below(gb, 0x3c)) {
		RET();
		return;
	}
	set_state_advance(gb, wSceneObj2State, 0x02, -1);
}

PORT_LOCAL_JUMP(Cutscene43Func, Func_9e2a6)
{
	if (bc_below(gb, 0x2d)) {
		RET();
		return;
	}
	func_9ccf9(gb, wSceneObj3, 0x48, 0x50, 0x03);
	advance(gb);
}

// Cutscene54Func

PORT_JUMP(Cutscene54Func)
{
	gb_call(gb, ADDR_Cutscene54Func + 0x6); // .Func_9e2d7
	JUMP_TO(ADDR_Func_9f73b);
}

PORT_LOCAL_JUMP(Cutscene54Func, Func_9e2d7)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene54Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene54Func, Func_9e2ed) { CLEAR_LOAD_PALS_ADVANCE(Pals_b8740); }
PORT_LOCAL_JUMP(Cutscene54Func, Func_9e303) { wait_cutscene(gb, 40); }

PORT_LOCAL_JUMP(Cutscene54Func, Func_9e308)
{
	if (!bc_and_zero(gb, 0x01)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj3YCoord);
	wr8(gb, wSceneObj3YCoord, alu_dec(gb, rd8(gb, wSceneObj3YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj3YCoord);
	sfx_or_advance_at(gb, 0x6b, 0x58);
}

PORT_LOCAL_JUMP(Cutscene54Func, Func_9e31c)
{
	if (bc_below(gb, 0x3c)) {
		RET();
		return;
	}
	set_state_advance(gb, wSceneObj3State, 0x03, SFX_CLICK);
}

PORT_LOCAL_JUMP(Cutscene54Func, Func_9e333) { wait_cutscene(gb, 20); }

PORT_LOCAL_JUMP(Cutscene54Func, Func_9e338)
{
	if (bc_and_zero(gb, 0x03)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj2YCoord, alu_dec(gb, rd8(gb, wSceneObj2YCoord)));
	advance_if(gb, wSceneObj2YCoord, 0x20);
}

// ld a, [bc] / cp n / ret c / (set_state_advance)
static void bc_state_advance(GB *gb, uint8_t n, uint16_t obj_state, uint8_t state, int sfx)
{
	if (bc_below(gb, n)) {
		RET();
		return;
	}
	set_state_advance(gb, obj_state, state, sfx);
}

// ld a, [bc] / cp n / ret c / play_sfx sfx / jp AdvanceCutsceneFunc
static void bc_sfx_advance(GB *gb, uint8_t n, uint16_t sfx)
{
	if (bc_below(gb, n)) {
		RET();
		return;
	}
	play_sfx(gb, sfx);
	advance(gb);
}

// ld a, [bc] / cp n / ret c / (func_9ccf9) / jp AdvanceCutsceneFunc
static void bc_place_advance(GB *gb, uint8_t n, uint16_t obj, uint8_t y, uint8_t x, uint8_t state)
{
	if (bc_below(gb, n)) {
		RET();
		return;
	}
	func_9ccf9(gb, obj, y, x, state);
	advance(gb);
}

// Cutscene3dFunc

PORT_JUMP(Cutscene3dFunc)
{
	gb_call(gb, ADDR_Cutscene3dFunc + 0x9); // .Func_9e350
	gb_call(gb, ADDR_Func_9f7a2);
	JUMP_TO(ADDR_Func_9f75c);
}

PORT_LOCAL_JUMP(Cutscene3dFunc, Func_9e350)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene3dFunc + 0xd));
}

PORT_LOCAL_JUMP(Cutscene3dFunc, Func_9e376)
{
	func_9ccf9(gb, wSceneObj2, 0x6c, 0x78, 0x01);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene3dFunc, Func_9e384)
{
	wr8(gb, wSceneObj2YCoord, alu_dec(gb, rd8(gb, wSceneObj2YCoord)));
	SET_HL(gb, inc_l(wSceneObj2YCoord));
	wr8(gb, REG_HL(gb), alu_dec(gb, rd8(gb, REG_HL(gb))));
	advance_if(gb, REG_HL(gb), 0x44);
}

PORT_LOCAL_JUMP(Cutscene3dFunc, Func_9e391)
{
	if (bc_below(gb, 0x1e)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj2State);
	gb->cpu.a = alu_inc(gb, rd8(gb, wSceneObj2State));
	CALL(SetSceneObjState);
	play_sfx(gb, SFX_0C4);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene3dFunc, Func_9e3a8)
{
	if (!test_zero(gb, wSceneObj2State)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene3dFunc, Func_9e3b0) { bc_place_advance(gb, 0x0a, wSceneObj2, 0x84, 0x78, 0x03); }

PORT_LOCAL_JUMP(Cutscene3dFunc, Func_9e3c2)
{
	if (bc_below(gb, 0x3c)) {
		RET();
		return;
	}
	gb->cpu.a = 0x08;
	wr8(gb, wCollectionRow, 0x08);
	func_9ccf9(gb, wSceneObj4, 0x50, 0x50, 0x05);
	func_9ccf9(gb, wSceneObj3, 0x40, 0x50, 0x01);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene3dFunc, Func_9e3e4) { bc_state_advance(gb, 0x5a, wSceneObj4State, 0x06, -1); }
PORT_LOCAL_JUMP(Cutscene3dFunc, Func_9e3f3) { bc_sfx_advance(gb, 0x04, SFX_07A); }
PORT_LOCAL_JUMP(Cutscene3dFunc, Func_9e402) { advance_if(gb, wSceneObj4State, 0x07); }
PORT_LOCAL_JUMP(Cutscene3dFunc, Func_9e40b) { bc_state_advance(gb, 0x08, wSceneObj3State, 0x02, -1); }
PORT_LOCAL_JUMP(Cutscene3dFunc, Func_9e41a) { bc_sfx_advance(gb, 0x14, SFX_0AC); }

PORT_LOCAL_JUMP(Cutscene3dFunc, Func_9e429)
{
	if (!bc_and_zero(gb, 0x03)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj3YCoord);
	wr8(gb, wSceneObj3YCoord, alu_inc(gb, rd8(gb, wSceneObj3YCoord)));
	advance_if(gb, wSceneObj3YCoord, 0x80);
}

// Cutscene4aFunc

PORT_JUMP(Cutscene4aFunc)
{
	gb_call(gb, ADDR_Cutscene4aFunc + 0x6); // .Func_9e457
	JUMP_TO(ADDR_Func_9f7bb);
}

PORT_LOCAL_JUMP(Cutscene4aFunc, Func_9e457)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene4aFunc + 0xa));
}

PORT_LOCAL_JUMP(Cutscene4aFunc, Func_9e469)
{
	SET_HL(gb, wSceneObj2XCoord);
	wr8(gb, wSceneObj2XCoord, alu_dec(gb, rd8(gb, wSceneObj2XCoord)));
	advance_if(gb, wSceneObj2XCoord, 0x56);
}

PORT_LOCAL_JUMP(Cutscene4aFunc, Func_9e474) { bc_state_advance(gb, 0x3c, wSceneObj2State, 0x03, -1); }
PORT_LOCAL_JUMP(Cutscene4aFunc, Func_9e483) { sfx_advance_if(gb, wSceneObj2Frame, 0x04, SFX_CLICK); }
PORT_LOCAL_JUMP(Cutscene4aFunc, Func_9e494) { sfx_advance_if(gb, wSceneObj2Frame, 0x06, SFX_132); }
PORT_LOCAL_JUMP(Cutscene4aFunc, Func_9e4a5) { advance_if(gb, wSceneObj2Frame, 0x07); }

PORT_JUMP(Func_9e4ae)
{
	func_9ccf9(gb, wSceneObj2, 0x80, 0x40, 0x01);
	func_9ccf9(gb, wSceneObj3, 0x50, 0x5c, 0x01);
	SET_HL(gb, wSceneObj3Attributes);
	wr8(gb, wSceneObj3Attributes, rd8(gb, wSceneObj3Attributes) | (1 << 1));
	gb_call(gb, ADDR_Func_9f7dc);
	JUMP_TO(ADDR_Func_9f821);
}

// Cutscene4cFunc

PORT_JUMP(Cutscene4cFunc)
{
	gb_call(gb, ADDR_Cutscene4cFunc + 0x9); // .Func_9e4d8
	gb_call(gb, ADDR_Func_9f7dc);
	JUMP_TO(ADDR_Func_9f821);
}

PORT_LOCAL_JUMP(Cutscene4cFunc, Func_9e4d8)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene4cFunc + 0xd));
}

PORT_LOCAL_JUMP(Cutscene4cFunc, Func_9e4f0) { CLEAR_LOAD_PALS_ADVANCE(Pals_b86c0); }
PORT_LOCAL_JUMP(Cutscene4cFunc, Func_9e506) { wait_cutscene(gb, 40); }
PORT_LOCAL_JUMP(Cutscene4cFunc, Func_9e50b) { dec_obj2_y_sfx(gb, 0x7e, 0x58); }
PORT_LOCAL_JUMP(Cutscene4cFunc, Func_9e51b) { bc_state_advance(gb, 0x3c, wSceneObj2State, 0x02, -1); }

PORT_LOCAL_JUMP(Cutscene4cFunc, Func_9e52a)
{
	gb->cpu.a = rd8(gb, wSceneObj2State);
	alu_cp(gb, gb->cpu.a, 0x03);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene4cFunc, Func_9e532) { bc_state_advance(gb, 0x64, wSceneObj3State, 0x02, -1); }

PORT_LOCAL_JUMP(Cutscene4cFunc, Func_9e541)
{
	if (bc_below(gb, 0x3c)) {
		RET();
		return;
	}
	clear_advance(gb, wSceneObj3State);
}

// Cutscene4eFunc

PORT_JUMP(Cutscene4eFunc)
{
	gb_call(gb, ADDR_Cutscene4eFunc + 0x6); // .Func_9e552
	JUMP_TO(ADDR_Func_9f857);
}

PORT_LOCAL_JUMP(Cutscene4eFunc, Func_9e552)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene4eFunc + 0xa));
}

PORT_LOCAL_JUMP(Cutscene4eFunc, Func_9e562) { CLEAR_LOAD_PALS_ADVANCE(Pals_b8640); }
PORT_LOCAL_JUMP(Cutscene4eFunc, Func_9e578) { bc_place_advance(gb, 0x14, wSceneObj2, 0x78, 0x50, 0x01); }

PORT_LOCAL_JUMP(Cutscene4eFunc, Func_9e58a)
{
	if (!bc_and_zero(gb, 0x01)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj2YCoord, alu_dec(gb, rd8(gb, wSceneObj2YCoord)));
	sfx_advance_if(gb, wSceneObj2YCoord, 0x50, SFX_CLICK);
}

PORT_JUMP(Func_9e5a1)
{
	func_9ccf9(gb, wSceneObj2, 0x20, 0x4c, 0x01);
	func_9ccf9(gb, wSceneObj3, 0x50, 0x60, 0x04);
	JUMP_TO(ADDR_Func_9f86a);
}

// ld a, [bc] / and mask / ret nz / ld hl, addr / inc or dec [hl] / ld a, [hl]
// returns false on the ret nz
static bool bc_step(GB *gb, uint8_t mask, uint16_t addr, int dir)
{
	if (!bc_and_zero(gb, mask)) {
		RET();
		return false;
	}
	SET_HL(gb, addr);
	uint8_t v = rd8(gb, addr);
	wr8(gb, addr, dir > 0 ? alu_inc(gb, v) : alu_dec(gb, v));
	gb->cpu.a = rd8(gb, addr);
	return true;
}

// Cutscene4fFunc

PORT_JUMP(Cutscene4fFunc)
{
	gb_call(gb, ADDR_Cutscene4fFunc + 0x6); // .Func_9e5c0
	JUMP_TO(ADDR_Func_9f86a);
}

PORT_LOCAL_JUMP(Cutscene4fFunc, Func_9e5c0)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene4fFunc + 0xa));
}

PORT_LOCAL_JUMP(Cutscene4fFunc, Func_9e5e2)
{
	if (bc_step(gb, 0x01, wSceneObj2YCoord, 1))
		sfx_or_advance_at(gb, 0x31, 0x44);
}

PORT_LOCAL_JUMP(Cutscene4fFunc, Func_9e5f6) { bc_state_advance(gb, 0x3c, wSceneObj2State, 0x02, -1); }
PORT_LOCAL_JUMP(Cutscene4fFunc, Func_9e605) { wait_cutscene(gb, 45); }

PORT_LOCAL_JUMP(Cutscene4fFunc, Func_9e60a)
{
	gb->cpu.d = 0x5c;
	if (!bc_step(gb, 0x03, wSceneObj2YCoord, 1))
		return;
	alu_cp(gb, gb->cpu.a, gb->cpu.d);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene4fFunc, Func_9e61a) { wait_cutscene(gb, 8); }

PORT_LOCAL_JUMP(Cutscene4fFunc, Func_9e61f)
{
	if (bc_step(gb, 0x03, wSceneObj2YCoord, -1))
		advance_if(gb, wSceneObj2YCoord, 0x44);
}

PORT_LOCAL_JUMP(Cutscene4fFunc, Func_9e62e) { bc_state_advance(gb, 0x1e, wSceneObj2State, 0x01, -1); }
PORT_LOCAL_JUMP(Cutscene4fFunc, Func_9e63d) { bc_state_advance(gb, 0x3c, wSceneObj3State, 0x03, -1); }
PORT_LOCAL_JUMP(Cutscene4fFunc, Func_9e64c) { advance_if(gb, wSceneObj3State, 0x05); }

PORT_JUMP(Func_9e655)
{
	func_9ccf9(gb, wSceneObj2, 0x40, 0x58, 0x02);
	func_9ccf9(gb, wSceneObj3, 0x58, 0x58, 0x01);
	JUMP_TO(ADDR_Func_9f8da);
}

// Cutscene51Func

PORT_JUMP(Cutscene51Func)
{
	gb_call(gb, ADDR_Cutscene51Func + 0x6); // .Func_9e674
	JUMP_TO(ADDR_Func_9f8da);
}

PORT_LOCAL_JUMP(Cutscene51Func, Func_9e674)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene51Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene51Func, Func_9e688)
{
	if (bc_step(gb, 0x01, wSceneObj2YCoord, 1))
		sfx_or_advance_at(gb, 0x45, 0x58);
}

PORT_LOCAL_JUMP(Cutscene51Func, Func_9e69c) { set_state_advance(gb, wSceneObj2State, 0x03, -1); }
PORT_LOCAL_JUMP(Cutscene51Func, Func_9e6a7) { sfx_advance_if(gb, wSceneObj2Frame, 0x04, SFX_0A1); }

PORT_LOCAL_JUMP(Cutscene51Func, Func_9e6b8)
{
	if (!test_zero(gb, wSceneObj2State)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene51Func, Func_9e6c0) { wait_cutscene(gb, 45); }

PORT_LOCAL_JUMP(Cutscene51Func, Func_9e6c5)
{
	gb_call(gb, ADDR_Cutscene51Func + 0x69); // .Func_9e6d7
	if (bc_step(gb, 0x03, wSceneObj3YCoord, 1))
		advance_if(gb, wSceneObj3YCoord, 0x78);
}

PORT_LOCAL(Cutscene51Func, Func_9e6d7)
{
	SET_HL(gb, wSceneObj3Unk7);
	wr8(gb, wSceneObj3Unk7, alu_inc(gb, rd8(gb, wSceneObj3Unk7)));
	gb->cpu.a = rd8(gb, wSceneObj3Unk7);
	if (state_is(gb, 0x01)) {
		play_sfx(gb, SFX_01F);
		return;
	}
	alu_cp(gb, gb->cpu.a, 0x10);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj3Unk7, 0);
}

PORT_JUMP(Func_9e6ef)
{
	func_9ccf9(gb, wSceneObj2, 0x56, 0x50, 0x01);
	JUMP_TO(ADDR_Func_9f91f);
}

// Cutscene56Func

PORT_JUMP(Cutscene56Func)
{
	gb_call(gb, ADDR_Cutscene56Func + 0x6); // .Func_9e703
	JUMP_TO(ADDR_Func_9f91f);
}

PORT_LOCAL_JUMP(Cutscene56Func, Func_9e703)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene56Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene56Func, Func_9e71f) { CLEAR_LOAD_PALS_ADVANCE(Pals_b9080); }
PORT_LOCAL_JUMP(Cutscene56Func, Func_9e735) { bc_place_advance(gb, 0x2d, wSceneObj2, 0x56, 0x50, 0x01); }
PORT_LOCAL_JUMP(Cutscene56Func, Func_9e747) { advance_if(gb, wSceneObj2State, 0x02); }
PORT_LOCAL_JUMP(Cutscene56Func, Func_9e750) { advance_if(gb, wSceneObj2State, 0x01); }

PORT_LOCAL_JUMP(Cutscene56Func, Func_9e759)
{
	if (bc_below(gb, 0x14)) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj2State, 0);
	gb->cpu.a = 0x08;
	wr8(gb, wCollectionRow, 0x08);
	gb->cpu.b = BANK_Pals_b8800;
	gb->cpu.c = 8 * PAL_SIZE;
	SET_HL(gb, ADDR_Pals_b8800);
	SET_DE(gb, wTempPals1);
	CALL(CopyFarBytes);
	gb_call(gb, ADDR_Func_9cd40);
	advance(gb);
}

// .asm_9e77c: hl = pals
static void cutscene56_pals(GB *gb, uint16_t pals)
{
	SET_HL(gb, pals);
	gb_call(gb, ADDR_Func_9cd74);
	gb->cpu.b = BANK_Pals_b8840;
	CALL(LoadFarPalsToTempPals1);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene56Func, Func_9e779) { cutscene56_pals(gb, ADDR_Pals_b8840); }
PORT_LOCAL_JUMP(Cutscene56Func, Func_9e78e) { cutscene56_pals(gb, ADDR_Pals_b8880); }

// ld a, [bc] / and mask / ret nz / jp Func_9cdbd
static void bc_fade(GB *gb, uint8_t mask)
{
	if (!bc_and_zero(gb, mask)) {
		RET();
		return;
	}
	JUMP_TO(ADDR_Func_9cdbd);
}

PORT_LOCAL_JUMP(Cutscene56Func, Func_9e787) { bc_fade(gb, 0x01); }
PORT_LOCAL_JUMP(Cutscene56Func, Func_9e793) { bc_fade(gb, 0x03); }

// --- Cutscene08/10/17/1d/25Func: b = scene obj 2's first state ---

PORT_JUMP(Cutscene08Func) { gb->cpu.b = 0x01; JUMP_TO(ADDR_Func_9d6f0); }
PORT_JUMP(Cutscene10Func) { gb->cpu.b = 0x03; JUMP_TO(ADDR_Func_9d6f0); }
PORT_JUMP(Cutscene17Func) { gb->cpu.b = 0x05; JUMP_TO(ADDR_Func_9d6f0); }
PORT_JUMP(Cutscene1dFunc) { gb->cpu.b = 0x07; JUMP_TO(ADDR_Func_9d6f0); }
PORT_JUMP(Cutscene25Func) { gb->cpu.b = 0x09; JUMP_TO(ADDR_Func_9d6f0); } // fallthrough

// Cutscene57Func

PORT_JUMP(Cutscene57Func)
{
	gb_call(gb, ADDR_Cutscene57Func + 0x6); // .Func_9e7b9
	JUMP_TO(ADDR_Func_9f991);
}

PORT_LOCAL_JUMP(Cutscene57Func, Func_9e7b9)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene57Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene57Func, Func_9e7d9) { CLEAR_LOAD_PALS_ADVANCE(Pals_b8540); }

PORT_LOCAL_JUMP(Cutscene57Func, Func_9e7ef)
{
	SET_HL(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj2YCoord, alu_inc(gb, rd8(gb, wSceneObj2YCoord)));
	gb->cpu.a = rd8(gb, wSceneObj2YCoord);
	sfx_or_advance_at(gb, 0x2c, 0x52);
}

PORT_LOCAL_JUMP(Cutscene57Func, Func_9e7ff) { bc_state_advance(gb, 0x32, wSceneObj2State, 0x04, -1); }

PORT_LOCAL_JUMP(Cutscene57Func, Func_9e80e)
{
	gb->cpu.a = rd8(gb, wSceneObj2State);
	if (!state_is(gb, 0x03)) {
		RET();
		return;
	}
	set_state_advance(gb, wSceneObj3State, 0x02, SFX_105);
}

PORT_LOCAL_JUMP(Cutscene57Func, Func_9e827) { bc_state_advance(gb, 0x30, wSceneObj3State, 0x01, -1); }

// .asm_9e839: hl = pals
static void cutscene57_pals(GB *gb, uint16_t pals)
{
	SET_HL(gb, pals);
	gb_call(gb, ADDR_Func_9cd74);
	gb->cpu.b = BANK_Pals_b8580;
	CALL(LoadFarPalsToTempPals1);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene57Func, Func_9e836) { cutscene57_pals(gb, ADDR_Pals_b8580); }
PORT_LOCAL_JUMP(Cutscene57Func, Func_9e84b) { cutscene57_pals(gb, ADDR_Pals_b85c0); }
PORT_LOCAL_JUMP(Cutscene57Func, Func_9e850) { cutscene57_pals(gb, ADDR_Pals_b8600); }
PORT_LOCAL_JUMP(Cutscene57Func, Func_9e844) { bc_fade(gb, 0x01); }

PORT_JUMP(Func_9e855)
{
	func_9ccf9(gb, wSceneObj2, SCREEN_HEIGHT_PX, 0x48, 0x01);
	func_9ccf9(gb, wSceneObj3, 0x48, 0x58, 0x01);
	gb_call(gb, ADDR_Func_9f9c1);
	JUMP_TO(ADDR_Func_9f9f5);
}

// Cutscene58Func

PORT_JUMP(Cutscene58Func)
{
	gb_call(gb, ADDR_Cutscene58Func + 0x9); // .Func_9e87a
	gb_call(gb, ADDR_Func_9f9c1);
	JUMP_TO(ADDR_Func_9f9f5);
}

PORT_LOCAL_JUMP(Cutscene58Func, Func_9e87a)
{
	gb_push16(gb, REG_BC(gb));
	gb_call(gb, ADDR_Func_9ea9e);
	gb_call(gb, ADDR_Func_9eab4);
	SET_BC(gb, gb_pop16(gb));
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene58Func + 0x15));
}

PORT_LOCAL_JUMP(Cutscene58Func, Func_9e89c) { CLEAR_LOAD_PALS_ADVANCE(Pals_b87c0); }
PORT_LOCAL_JUMP(Cutscene58Func, Func_9e8b2) { wait_cutscene(gb, 30); }
PORT_LOCAL_JUMP(Cutscene58Func, Func_9e8b7) { dec_obj2_y_sfx(gb, 0x76, 0x50); }
PORT_LOCAL_JUMP(Cutscene58Func, Func_9e8c7) { bc_state_advance(gb, 0x1a, wSceneObj2State, 0x02, -1); }

// .asm_9e8dc
static void cutscene58_spawn(GB *gb)
{
	func_9ccf9(gb, wSceneObj10, 0x48, 0x54, 0x03);
	func_9ccf9(gb, wSceneObj11, 0x44, 0x58, 0x03);
	gb->cpu.a = 0x01;
	wr8(gb, wSceneObj12YCoord, 0x01);
	wr8(gb, wSceneObj12XCoord, 0x01);
	play_sfx(gb, SFX_132);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene58Func, Func_9e8d6)
{
	gb->cpu.a = rd8(gb, wSceneObj2State);
	if (!state_is(gb, 0x01)) {
		RET();
		return;
	}
	cutscene58_spawn(gb);
}

PORT_LOCAL_JUMP(Cutscene58Func, Func_9e905)
{
	gb->cpu.a = rd8(gb, wSceneObj2State);
	if (!state_is(gb, 0x01)) {
		RET();
		return;
	}
	gb->cpu.a = 0x08;
	wr8(gb, wCollectionRow, 0x08);
	set_scene_obj_state(gb, wSceneObj3State, 0x02);
	cutscene58_spawn(gb);
}

PORT_JUMP(Func_9e91a)
{
	func_9ccf9(gb, wSceneObj3, 0x58, 0x40, 0x01);
	func_9ccf9(gb, wSceneObj4, 0x58, 0x50, 0x01);
	func_9ccf9(gb, wSceneObj5, 0x58, 0x60, 0x01);
	func_9ccf9(gb, wSceneObj6, 0x58, 0x40, 0x04);
	func_9ccf9(gb, wSceneObj7, 0x58, 0x50, 0x04);
	func_9ccf9(gb, wSceneObj8, 0x58, 0x60, 0x04);
	JUMP_TO(ADDR_Func_9fa48);
}

// Cutscene49Func

PORT_JUMP(Cutscene49Func)
{
	gb_call(gb, ADDR_Cutscene49Func + 0x6); // .Func_9da37
	JUMP_TO(ADDR_Func_9f38d);
}

PORT_LOCAL_JUMP(Cutscene49Func, Func_9da37)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene49Func + 0xa));
}

PORT_LOCAL_JUMP(Cutscene49Func, Func_9da4d) { wait_cutscene(gb, 20); }

PORT_LOCAL_JUMP(Cutscene49Func, Func_9da52)
{
	set_scene_obj_state(gb, wSceneObj2State, 0x01);
	gb->cpu.a = 0;
	wr8(gb, wSceneObj2YCoord, 0);
	wr8(gb, wSceneObj2YCoord + 1, 80);
	SET_HL(gb, wSceneObj2YCoord + 1);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene49Func, Func_9da65)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	if (alu_bit(gb, gb->cpu.a, 0)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj2YCoord);
	wr8(gb, wSceneObj2YCoord, alu_inc(gb, rd8(gb, wSceneObj2YCoord)));
	sfx_advance_if(gb, wSceneObj2YCoord, 0x45, SFX_CLICK);
}

PORT_LOCAL_JUMP(Cutscene49Func, Func_9da7c) { bc_state_advance(gb, 0x14, wSceneObj2State, 0x04, -1); }

PORT_LOCAL_JUMP(Cutscene49Func, Func_9da8b)
{
	gb->cpu.a = rd8(gb, wSceneObj2State);
	if (state_is(gb, 0x01)) {
		set_scene_obj_state(gb, wSceneObj2State, 0x02);
		set_scene_obj_state(gb, wSceneObj3State, 0x03);
		gb->cpu.a = rd8(gb, wSceneObj2YCoord);
		wr8(gb, wSceneObj3YCoord, gb->cpu.a);
		gb->cpu.a = rd8(gb, wSceneObj2YCoord + 1);
		wr8(gb, wSceneObj3XCoord, gb->cpu.a);
		SET_HL(gb, wSceneObj2YCoord + 1);
		advance(gb);
		return;
	}
	gb->cpu.a = rd8(gb, wSceneObj2Frame);
	if (!state_is(gb, 0x03)) {
		RET();
		return;
	}
	if (!test_zero(gb, wSceneObj2Duration)) {
		RET();
		return;
	}
	play_sfx(gb, SFX_12C);
	RET();
}

PORT_LOCAL_JUMP(Cutscene49Func, Func_9dac4) { wait_cutscene(gb, 40); }

// ld hl, addr / inc [hl]
static void inc_mem(GB *gb, uint16_t addr)
{
	SET_HL(gb, addr);
	wr8(gb, addr, alu_inc(gb, rd8(gb, addr)));
}

// ld hl, addr / dec [hl]
static void dec_mem(GB *gb, uint16_t addr)
{
	SET_HL(gb, addr);
	wr8(gb, addr, alu_dec(gb, rd8(gb, addr)));
}

PORT_LOCAL_JUMP(Cutscene49Func, Func_9dac9)
{
	gb_call(gb, ADDR_Cutscene49Func + 0xbf); // .Func_9daf0
	if (!bc_and_zero(gb, 0x07)) {
		RET();
		return;
	}
	inc_mem(gb, wCutsceneWX);
	inc_mem(gb, wSceneObj5XCoord);
	inc_mem(gb, wSceneObj3XCoord);
	inc_mem(gb, wSCX);
	dec_mem(gb, wSceneObj4XCoord);
	gb->cpu.a = rd8(gb, wSceneObj4XCoord);
	dec_mem(gb, wSceneObj2XCoord);
	advance_if(gb, wSceneObj2XCoord, 0x40);
}

PORT_LOCAL(Cutscene49Func, Func_9daf0)
{
	inc_mem(gb, wSceneObj4Unk7);
	gb->cpu.a = rd8(gb, wSceneObj4Unk7);
	alu_cp(gb, gb->cpu.a, 0x11);
	if (gb->cpu.f & FLAG_C)
		return;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj4Unk7, 0);
	play_sfx(gb, SFX_01F);
}

// Cutscene55Func

PORT_JUMP(Cutscene55Func)
{
	gb_call(gb, ADDR_Cutscene55Func + 0x6); // .Func_9e965
	JUMP_TO(ADDR_Func_9fa48);
}

PORT_LOCAL_JUMP(Cutscene55Func, Func_9e965)
{
	gb_push16(gb, REG_BC(gb));
	gb_call(gb, ADDR_Func_9ea9e);
	gb_call(gb, ADDR_Func_9eab4);
	SET_BC(gb, gb_pop16(gb));
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene55Func + 0x12));
}

PORT_LOCAL_JUMP(Cutscene55Func, Func_9e9a3) { CLEAR_LOAD_PALS_ADVANCE(Pals_b8680); }
PORT_LOCAL_JUMP(Cutscene55Func, Func_9e9b9) { bc_place_advance(gb, 0x28, wSceneObj2, 0x46, 0x90, 0x02); }

PORT_LOCAL_JUMP(Cutscene55Func, Func_9e9cb)
{
	if (bc_step(gb, 0x01, wSceneObj2XCoord, -1))
		advance_if(gb, wSceneObj2XCoord, 0x44);
}

PORT_LOCAL_JUMP(Cutscene55Func, Func_9e9da) { wait_cutscene(gb, 45); }

PORT_LOCAL_JUMP(Cutscene55Func, Func_9e9df)
{
	if (bc_step(gb, 0x03, wSceneObj2YCoord, 1))
		sfx_advance_if(gb, wSceneObj2YCoord, 0x4e, SFX_SWITCH);
}

static void cutscene55_9e9fa(GB *gb)
{
	func_9ccf9(gb, wSceneObj10, 0x4c, 0x40, 0x03);
	func_9ccf9(gb, wSceneObj11, 0x4c, 0x48, 0x03);
	gb->cpu.a = 0x01;
	wr8(gb, wSceneObj12YCoord, 0x01);
	wr8(gb, wSceneObj12XCoord, 0x01);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene55Func, Func_9e9fa) { cutscene55_9e9fa(gb); }

PORT_LOCAL_JUMP(Cutscene55Func, Func_9e9f6)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj3State, 0);
	cutscene55_9e9fa(gb); // fallthrough
}

PORT_LOCAL_JUMP(Cutscene55Func, Func_9ea1b) { bc_state_advance(gb, 0x14, wSceneObj6State, 0x05, -1); }

PORT_LOCAL_JUMP(Cutscene55Func, Func_9ea2a)
{
	if (bc_step(gb, 0x03, wSceneObj2YCoord, -1))
		advance_if(gb, wSceneObj2YCoord, 0x46);
}

// ld e, n / .asm_9ea3b
static void cutscene55_walk(GB *gb, uint8_t e)
{
	gb->cpu.e = e;
	if (!bc_step(gb, 0x03, wSceneObj2XCoord, 1))
		return;
	alu_cp(gb, gb->cpu.a, gb->cpu.e);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene55Func, Func_9ea39) { cutscene55_walk(gb, 0x54); }
PORT_LOCAL_JUMP(Cutscene55Func, Func_9ea70) { cutscene55_walk(gb, 0x64); }

// xor a / ld [obj state], a / call .Func_9e9fa / ld b, n / .asm_9ea52
static void cutscene55_spawn(GB *gb, uint16_t obj_state, uint8_t b)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, obj_state, 0);
	gb_call(gb, ADDR_Cutscene55Func + 0x9b); // .Func_9e9fa
	gb->cpu.b = b;
	gb->cpu.a = alu_add(gb, rd8(gb, wSceneObj10XCoord), gb->cpu.b);
	wr8(gb, wSceneObj10XCoord, gb->cpu.a);
	gb->cpu.a = alu_add(gb, rd8(gb, wSceneObj11XCoord), gb->cpu.b);
	wr8(gb, wSceneObj11XCoord, gb->cpu.a);
	RET();
}

PORT_LOCAL_JUMP(Cutscene55Func, Func_9ea49) { cutscene55_spawn(gb, wSceneObj4State, 0x10); }
PORT_LOCAL_JUMP(Cutscene55Func, Func_9ea74) { cutscene55_spawn(gb, wSceneObj5State, 0x20); }
PORT_LOCAL_JUMP(Cutscene55Func, Func_9ea61) { bc_state_advance(gb, 0x14, wSceneObj7State, 0x05, -1); }
PORT_LOCAL_JUMP(Cutscene55Func, Func_9ea7f) { bc_state_advance(gb, 0x14, wSceneObj8State, 0x05, -1); }
PORT_LOCAL_JUMP(Cutscene55Func, Func_9ea8e) { wait_cutscene(gb, 30); }

PORT_LOCAL_JUMP(Cutscene55Func, Func_9ea93)
{
	inc_mem(gb, wSceneObj2XCoord);
	advance_if(gb, wSceneObj2XCoord, 0x90);
}

// Cutscene44Func

PORT_JUMP(Cutscene44Func)
{
	gb_call(gb, ADDR_Cutscene44Func + 0x9); // .Func_9ead3
	gb_call(gb, ADDR_Func_9fab4);
	JUMP_TO(ADDR_Func_9faea);
}

PORT_LOCAL_JUMP(Cutscene44Func, Func_9ead3)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene44Func + 0xd));
}

PORT_LOCAL_JUMP(Cutscene44Func, Func_9eaf7)
{
	func_9ccf9(gb, wSceneObj2, 0x20, 0x4c, 0x01);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene44Func, Func_9eb05)
{
	if (!bc_step(gb, 0x01, wSceneObj2YCoord, 1))
		return;
	alu_cp(gb, gb->cpu.a, 0xa8);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	clear_advance(gb, wSceneObj2State);
}

PORT_LOCAL_JUMP(Cutscene44Func, Func_9eb18)
{
	if (bc_below(gb, 0x1e)) {
		RET();
		return;
	}
	gb->cpu.a = 0x08;
	wr8(gb, wCollectionRow, 0x08);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene44Func, Func_9eb24)
{
	func_9ccf9(gb, wSceneObj2, 0x20, 0x50, 0x02);
	func_9ccf9(gb, wSceneObj3, 0x48, 0x20, 0x01);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene44Func, Func_9eb3d)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	alu_cp(gb, gb->cpu.a, 0x50);
	if (gb->cpu.f & FLAG_Z)
		gb_call(gb, ADDR_AdvanceCutsceneFunc);
	gb_call(gb, ADDR_Cutscene44Func + 0xf2); // .Func_9ebbc
	RET();
}

PORT_LOCAL_JUMP(Cutscene44Func, Func_9eb47)
{
	gb_call(gb, ADDR_Cutscene44Func + 0xf2); // .Func_9ebbc
	gb_call(gb, ADDR_Cutscene44Func + 0xfd); // .Func_9ebc7
	gb->cpu.a = rd8(gb, REG_HL(gb));
	alu_cp(gb, gb->cpu.a, 0x48);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	play_sfx(gb, SFX_FAT_WALK);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene44Func, Func_9eb5c)
{
	if (bc_below(gb, 0x02)) {
		RET();
		return;
	}
	SET_HL(gb, wSceneObj3XCoord);
	wr8(gb, wSceneObj3XCoord, alu_dec(gb, rd8(gb, wSceneObj3XCoord)));
	wr8(gb, wSceneObj3XCoord, alu_dec(gb, rd8(gb, wSceneObj3XCoord)));
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene44Func, Func_9eb68)
{
	if (bc_below(gb, 0x06)) {
		RET();
		return;
	}
	clear_advance(gb, wSceneObj2State);
}

PORT_LOCAL_JUMP(Cutscene44Func, Func_9eb73) { bc_state_advance(gb, 0x3c, wSceneObj3State, 0x02, SFX_11A); }

PORT_LOCAL_JUMP(Cutscene44Func, Func_9eb8a)
{
	gb->cpu.a = rd8(gb, wSceneObj3State);
	if (!state_is(gb, 0x03)) {
		RET();
		return;
	}
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj3Unk7, 0);
	wr8(gb, wSceneObj4YCoord, 0);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene44Func, Func_9eb9a) { wait_cutscene(gb, 60); }

PORT_LOCAL_JUMP(Cutscene44Func, Func_9eb9f)
{
	gb_call(gb, ADDR_Cutscene44Func + 0xfd); // .Func_9ebc7
	gb->cpu.a = rd8(gb, REG_HL(gb));
	alu_cp(gb, gb->cpu.a, 0x88);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	clear_advance(gb, wSceneObj3State);
}

PORT_LOCAL_JUMP(Cutscene44Func, Func_9ebad) { bc_sfx_advance(gb, 0x2d, SFX_08D); }

PORT_LOCAL(Cutscene44Func, Func_9ebbc)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), 0x03);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	inc_mem(gb, wSceneObj2YCoord);
}

PORT_LOCAL(Cutscene44Func, Func_9ebc7)
{
	SET_HL(gb, wSceneObj3Unk7);
	gb_call(gb, ADDR_Cutscene44Func + 0x109); // .Func_9ebd3
	SET_HL(gb, wSceneObj3XCoord);
	gb->cpu.a = alu_add(gb, gb->cpu.a, rd8(gb, wSceneObj3XCoord));
	wr8(gb, wSceneObj3XCoord, gb->cpu.a);
}

// hl = counter: returns a = .data_9ebe6[counter++], wrapping at $30
PORT_LOCAL(Cutscene44Func, Func_9ebd3)
{
	uint16_t hl = REG_HL(gb);
	for (;;) {
		gb->cpu.a = rd8(gb, hl);
		wr8(gb, hl, alu_inc(gb, gb->cpu.a));
		alu_cp(gb, gb->cpu.a, 0x30);
		if (gb->cpu.f & FLAG_C)
			break;
		gb->cpu.a = 0;
		gb->cpu.f = FLAG_Z; // xor a
		wr8(gb, hl, 0);
	}
	gb->cpu.c = gb->cpu.a;
	gb->cpu.b = 0;
	hl = alu_add16(gb, ADDR_Cutscene44Func + 0x11c, REG_BC(gb)); // .data_9ebe6
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
}

// Cutscene14Func

PORT(Cutscene14Func)
{
	gb_call(gb, ADDR_Cutscene14Func + 0x7); // .Func_9ec73
	gb_call(gb, ADDR_Func_9fba9);
}

PORT_LOCAL_JUMP(Cutscene14Func, Func_9ec73)
{
	gb->cpu.a = rd8(gb, w2d013);
	JUMP_TO(jumptable(gb, ADDR_Cutscene14Func + 0xb));
}

// ld hl, obj y / ld a, y / ld [hli], a / ld [hl], x / ld a, state /
// ld hl, obj state / call SetSceneObjState
static void place_obj_yx(GB *gb, uint16_t obj_y, uint8_t y, uint8_t x, uint16_t obj_state, uint8_t state)
{
	gb->cpu.a = y;
	wr8(gb, obj_y, y);
	wr8(gb, obj_y + 1, x);
	set_scene_obj_state(gb, obj_state, state);
}

// ld [hl], v0 / inc l / ld [hl], v1 / ... (6 bytes from hl)
static void write6_inc_l(GB *gb, uint16_t hl, const uint8_t *v)
{
	gb->cpu.h = (uint8_t)(hl >> 8);
	gb->cpu.l = (uint8_t)hl;
	for (int i = 0; i < 6; i++) {
		if (i)
			gb->cpu.l = alu_inc(gb, gb->cpu.l);
		wr8(gb, REG_HL(gb), v[i]);
	}
}

PORT_LOCAL_JUMP(Cutscene14Func, Func_9ec9b)
{
	place_obj_yx(gb, wSceneObj2YCoord, 120, 82, wSceneObj2State, 0x01);
	place_obj_yx(gb, wSceneObj3YCoord, 145, 68, wSceneObj3State, 0x02);
	place_obj_yx(gb, wSceneObj4YCoord, 145, 96, wSceneObj4State, 0x03);
	gb->cpu.a = 0xbc;
	wr8(gb, wCurSceneObj, 0xbc);
	wr8(gb, wCurSceneObj + 1, 0x50);
	gb->cpu.a = 0x02;
	wr8(gb, wSceneObj1YCoord, 0x02);
	static const uint8_t v[6] = { 0x99, 0x6e, 0xe7, 0x6e, 0x33, 0x6f };
	write6_inc_l(gb, wPalFade1, v);
	advance(gb);
}

// call .Func_9ee5b / ld a, [wGlobalCounter] / and mask: false on the ret nz
static bool cutscene14_tick(GB *gb, uint8_t mask)
{
	gb_call(gb, ADDR_Cutscene14Func + 0x1ef); // .Func_9ee5b
	gb->cpu.a = alu_and(gb, rd8(gb, wGlobalCounter), mask);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return false;
	}
	return true;
}

PORT_LOCAL_JUMP(Cutscene14Func, Func_9ecef)
{
	if (!cutscene14_tick(gb, 0x01))
		return;
	dec_mem(gb, wSceneObj2YCoord);
	dec_mem(gb, wSceneObj3YCoord);
	dec_mem(gb, wSceneObj4YCoord);
	dec_mem(gb, wCurSceneObj);
	gb->cpu.a = rd8(gb, wCurSceneObj);
	sfx_or_advance_at(gb, 0x87, 0x74);
}

PORT_LOCAL_JUMP(Cutscene14Func, Func_9ed14)
{
	gb_call(gb, ADDR_Cutscene14Func + 0x1ef); // .Func_9ee5b
	wait_cutscene(gb, 60);
}

PORT_LOCAL_JUMP(Cutscene14Func, Func_9ed1c)
{
	if (!cutscene14_tick(gb, 0x03))
		return;
	inc_mem(gb, wSceneObj2YCoord);
	inc_mem(gb, wSceneObj3YCoord);
	inc_mem(gb, wSceneObj4YCoord);
	inc_mem(gb, wCurSceneObj);
	advance_if(gb, wCurSceneObj, 0x82);
}

PORT_LOCAL_JUMP(Cutscene14Func, Func_9ed3c)
{
	if (!cutscene14_tick(gb, 0x03))
		return;
	inc_mem(gb, wCurSceneObj);
	gb->cpu.a = rd8(gb, wCurSceneObj);
	alu_cp(gb, gb->cpu.a, 0x92);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	clear_advance(gb, wSceneObj1YCoord);
}

PORT_LOCAL_JUMP(Cutscene14Func, Func_9ed54)
{
	gb_call(gb, ADDR_Cutscene14Func + 0x1ef); // .Func_9ee5b
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	static const uint8_t v[6] = { 0x82, 0x6f, 0xe3, 0x6f, 0x44, 0x70 };
	for (int i = 0; i < 6; i++)
		wr8(gb, wPalFade1 + i, v[i]);
	gb->cpu.a = 0x70;
	SET_HL(gb, wPalFade1 + 6);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene14Func, Func_9ed70)
{
	gb_call(gb, ADDR_Cutscene14Func + 0x1ef); // .Func_9ee5b
	if (!(gb->cpu.f & FLAG_C)) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene14Func, Func_9ed77) { wait_cutscene(gb, 30); }

PORT_LOCAL_JUMP(Cutscene14Func, Func_9ed7c)
{
	place_obj_yx(gb, wSceneObj5, 76, 80, wSceneObj5State, 0x04);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wSceneObj2State, 0);
	wr8(gb, wSceneObj3State, 0);
	wr8(gb, wSceneObj4State, 0);
	wr8(gb, wColourFadeDiff, 0);
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene14Func, Func_9ed9c)
{
	gb_call(gb, ADDR_Cutscene14Func + 0x138); // .Func_9eda4
	wait_cutscene(gb, 120);
}

// ld hl, wColourFadeDiff / inc [hl] / ld a, [hl] / cp $0a / ret c /
// xor a / ld [hl], a: false on the ret c
static bool fade_diff_tick(GB *gb)
{
	inc_mem(gb, wColourFadeDiff);
	gb->cpu.a = rd8(gb, wColourFadeDiff);
	alu_cp(gb, gb->cpu.a, 0x0a);
	if (gb->cpu.f & FLAG_C)
		return false;
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wColourFadeDiff, 0);
	return true;
}

PORT_LOCAL(Cutscene14Func, Func_9eda4)
{
	if (fade_diff_tick(gb))
		play_sfx(gb, SFX_10F);
}

PORT_LOCAL_JUMP(Cutscene14Func, Func_9edb7) { set_state_advance(gb, wSceneObj5State, 0x05, -1); }

PORT_LOCAL_JUMP(Cutscene14Func, Func_9edc2)
{
	gb_call(gb, ADDR_Cutscene14Func + 0x162); // .Func_9edce
	advance_if(gb, wSceneObj5State, 0x06);
}

PORT_LOCAL(Cutscene14Func, Func_9edce)
{
	if (!fade_diff_tick(gb))
		return;
	gb->cpu.a = rd8(gb, wSceneObj5Frame);
	alu_cp(gb, gb->cpu.a, 0x09);
	play_sfx(gb, (gb->cpu.f & FLAG_C) ? SFX_110 : SFX_111);
}

PORT_LOCAL_JUMP(Cutscene14Func, Func_9edf1)
{
	gb_call(gb, ADDR_Cutscene14Func + 0x162); // .Func_9edce
	wait_cutscene(gb, 90);
}

PORT_LOCAL_JUMP(Cutscene14Func, Func_9edf9)
{
	gb_call(gb, ADDR_Cutscene14Func + 0x162); // .Func_9edce
	gb->cpu.a = rd8(gb, w2d014);
	gb->cpu.b = 0x01;
	alu_cp(gb, gb->cpu.a, 0x08);
	if (!(gb->cpu.f & FLAG_C)) {
		gb->cpu.b = alu_inc(gb, gb->cpu.b);
		alu_cp(gb, gb->cpu.a, 0x10);
		if (!(gb->cpu.f & FLAG_C))
			gb->cpu.b = alu_inc(gb, gb->cpu.b);
	}
	SET_HL(gb, wSceneObj5);
	gb->cpu.a = alu_add(gb, gb->cpu.b, rd8(gb, wSceneObj5));
	wr8(gb, wSceneObj5, gb->cpu.a);
	alu_cp(gb, gb->cpu.a, 0x70);
	if (gb->cpu.f & FLAG_C) {
		RET();
		return;
	}
	advance(gb);
}

PORT_LOCAL_JUMP(Cutscene14Func, Func_9ee17)
{
	gb_call(gb, ADDR_Cutscene14Func + 0x162); // .Func_9edce
	advance_if(gb, wColourFadeDiff, 0x09);
}

PORT_LOCAL(Cutscene14Func, Func_9ee23)
{
	gb_call(gb, ADDR_Cutscene14Func + 0x1be); // .Func_9ee2a
	gb_call(gb, ADDR_Func_9cd8d);
}

PORT_LOCAL(Cutscene14Func, Func_9ee2a)
{
	inc_mem(gb, wColourFadeDiff);
	gb->cpu.a = rd8(gb, wColourFadeDiff);
	if (state_is(gb, 0x0a)) {
		play_sfx(gb, SFX_114);
		return;
	}
	if (state_is(gb, 0x14)) {
		play_sfx(gb, SFX_115);
		return;
	}
	if (state_is(gb, 0x1e))
		play_sfx(gb, SFX_116);
}

PORT_LOCAL(Cutscene14Func, Func_9ee57) { gb_call(gb, ADDR_Func_9cd16); }

PORT_LOCAL(Cutscene14Func, Func_9ee5b)
{
	SET_BC(gb, wPalFade1Colour1RedUnk1);
	SET_HL(gb, wSceneObj4);
	gb_call(gb, ADDR_Cutscene14Func + 0x20b); // .Func_9ee77
	SET_BC(gb, wPalFade1Colour1RedSign);
	SET_HL(gb, wSceneObj3);
	gb_call(gb, ADDR_Cutscene14Func + 0x20b); // .Func_9ee77
	SET_BC(gb, wPalFade1);
	SET_HL(gb, wSceneObj2);
	gb_call(gb, ADDR_Cutscene14Func + 0x20b); // .Func_9ee77
}

// .Func_9ee92: stores de at bc (c is left incremented), and a
static void func_9ee92(GB *gb)
{
	gb->cpu.a = gb->cpu.e;
	wr8(gb, REG_BC(gb), gb->cpu.a);
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.a = gb->cpu.d;
	wr8(gb, REG_BC(gb), gb->cpu.a);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
}

PORT_LOCAL(Cutscene14Func, Func_9ee92) { func_9ee92(gb); }

// bc = movement pointer, hl = scene obj: adds the next (y, x) step to it and
// advances the pointer, restarting at .data_9ee99 (and setting carry) at the end
PORT_LOCAL(Cutscene14Func, Func_9ee77)
{
	gb->cpu.a = rd8(gb, REG_BC(gb));
	gb->cpu.e = gb->cpu.a;
	gb->cpu.c = alu_inc(gb, gb->cpu.c);
	gb->cpu.a = rd8(gb, REG_BC(gb));
	gb->cpu.d = gb->cpu.a;
	gb->cpu.c = alu_dec(gb, gb->cpu.c);
	uint16_t hl = REG_HL(gb);
	gb->cpu.a = alu_add(gb, rd8(gb, REG_DE(gb)), rd8(gb, hl));
	wr8(gb, hl, gb->cpu.a);
	hl++;
	SET_HL(gb, hl);
	SET_DE(gb, (uint16_t)(REG_DE(gb) + 1));
	gb->cpu.a = alu_add(gb, rd8(gb, REG_DE(gb)), rd8(gb, hl));
	wr8(gb, hl, gb->cpu.a);
	SET_DE(gb, (uint16_t)(REG_DE(gb) + 1));
	gb->cpu.a = rd8(gb, REG_DE(gb));
	alu_cp(gb, gb->cpu.a, 0x80);
	if (!(gb->cpu.f & FLAG_Z)) {
		func_9ee92(gb); // jr nz
		return;
	}
	SET_DE(gb, ADDR_Cutscene14Func + 0x22d); // .data_9ee99
	gb_call(gb, ADDR_Cutscene14Func + 0x226); // .Func_9ee92
	gb->cpu.f = (uint8_t)((gb->cpu.f & FLAG_Z) | FLAG_C); // scf
}



// ld a, [wROMBank] / push af / ld a, b / bankswitch
static void vbf_push_bank_switch(GB *gb)
{
	gb->cpu.a = rd8(gb, wROMBank);
	push_af(gb);
	gb->cpu.a = gb->cpu.b;
	bankswitch(gb, gb->cpu.a);
}

// pop af / bankswitch
static void vbf_pop_bank(GB *gb)
{
	pop_af(gb);
	bankswitch(gb, gb->cpu.a);
}

// ld a, [wLCDCFlagsToFlip] / and a / jr z, .skip / ld hl, rLCDC / xor [hl] /
// ld [hl], a / xor a / ld [wLCDCFlagsToFlip], a
static void vbf_flip_lcdc(GB *gb)
{
	gb->cpu.a = rd8(gb, wLCDCFlagsToFlip);
	gb->cpu.a = alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (!gb->cpu.a)
		return;
	SET_HL(gb, rLCDC);
	gb->cpu.a = alu_xor(gb, gb->cpu.a, rd8(gb, rLCDC));
	wr8(gb, rLCDC, gb->cpu.a);
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wLCDCFlagsToFlip, 0);
}

// ld a, $02 / ldh [rWBK], a / apply wSCY and wSCX
static void vbf_wram2_scroll(GB *gb)
{
	gb->cpu.a = 0x02; // BANK("WRAM2")
	wr8(gb, rWBK, gb->cpu.a);
	wr8(gb, rSCY, rd8(gb, wSCY));
	gb->cpu.a = rd8(gb, wSCX);
	wr8(gb, rSCX, gb->cpu.a);
}

// VBlank_Cutscene.Func, as copied to wVBlankFunc (see ram_code.c). Its calls
// return to base + n, the block's start wherever it runs from.
PORT_LOCAL_JUMP(VBlank_Cutscene, Func)
{
	uint16_t base = gb->cpu.pc;
	vbf_wram2_scroll(gb);
	wr8(gb, rWY, rd8(gb, wCutsceneWY));
	gb->cpu.a = rd8(gb, wCutsceneWX);
	wr8(gb, rWX, gb->cpu.a);

	vbf_flip_lcdc(gb);

	// .skip_lcd_flip
	gb->cpu.b = BANK_ApplyPalConfig1;
	vbf_push_bank_switch(gb);
	gb->cpu.a = alu_and(gb, rd8(gb, wPalConfig1Register), rd8(gb, wPalConfig1Register));
	if (gb->cpu.a)
		gb_call_ret(gb, ADDR_ApplyPalConfig1, (uint16_t)(base + 0x3b));
	gb->cpu.a = alu_and(gb, rd8(gb, wPalConfig2), rd8(gb, wPalConfig2));
	if (gb->cpu.a)
		gb_call_ret(gb, ADDR_ApplyPalConfig2, (uint16_t)(base + 0x42));
	vbf_pop_bank(gb);

	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, rVBK, 0);
	gb->cpu.a = (uint8_t)(wVirtualOAM >> 8);
	JUMP_TO(hTransferVirtualOAM);
}
