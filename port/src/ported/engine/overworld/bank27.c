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
