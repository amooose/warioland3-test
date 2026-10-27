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
	JUMP(AdvanceCutsceneFunc); // jr
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
	JUMP(WaitCutsceneFunc);
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
