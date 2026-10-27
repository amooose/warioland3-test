// Port of src/engine/level/objects/chests.asm
#include "ported.h"

// de = frameset to set once wCurObjStateDuration runs out
PORT_JUMP(TreasureFunc)
{
	gb->cpu.a = (1 << NO_ACTIONS_F) | 1;
	wr8(gb, wCurObjAction, gb->cpu.a);
	SET_HL(gb, wCurObjStateDuration);
	gb->cpu.a = rd8(gb, wCurObjStateDuration);
	alu_and(gb, gb->cpu.a, gb->cpu.a);
	if (gb->cpu.f & FLAG_Z) {
		RET();
		return;
	}
	uint8_t duration = alu_dec(gb, gb->cpu.a);
	wr8(gb, wCurObjStateDuration, duration);
	if (duration) {
		RET();
		return;
	}
	JUMP(SetObjectFramesetPtr);
}

PORT(DeinitChest)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wCurObjFlags, 0);
}

// The object's flags and substate as the init functions set them: visible,
// no collision, update function = update, initialised.
static uint16_t init_obj(GB *gb, uint16_t update)
{
	uint16_t hl = wCurObjFlags;
	SET_HL(gb, hl);
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJFLAG_INVISIBLE) | OBJFLAG_NO_COLLISION));
	hl = (uint16_t)((hl & 0xff00) | (OBJ_UPDATE_FUNCTION + 1));
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	hl = (uint16_t)((hl & 0xff00) | OBJ_SUBSTATE);
	SET_HL(gb, hl);
	return hl;
}

PORT(ChestFunc)
{
	uint16_t hl = init_obj(gb, ADDR_ChestFunc + 0x24); // .Update
	wr8(gb, hl, (uint8_t)((rd8(gb, hl) & ~OBJSUBFLAG_UNINITIALISED) | (1 << OBJSUBFLAG_UNK_4_F)));
	SET_DE(gb, ADDR_Frameset_68000);
	CALL(SetObjectFramesetPtr);
	gb->cpu.l = OBJ_COLLBOX_RIGHT;
	hl = REG_HL(gb);
	wr8(gb, hl--, 13);
	gb->cpu.a = (uint8_t)-14;
	wr8(gb, hl--, gb->cpu.a); // OBJ_COLLBOX_LEFT
	SET_HL(gb, hl);
}

// Inits the chest unless its treasure was already collected.
static void chest_func(GB *gb, uint8_t treasure)
{
	gb->cpu.a = alu_and(gb, rd8(gb, wKeyAndTreasureFlags), treasure);
	if (gb->cpu.f & FLAG_Z)
		JUMP_TO(ADDR_ChestFunc);
	else
		JUMP_TO(ADDR_DeinitChest);
}

PORT_JUMP(GreyChestFunc) { chest_func(gb, GREY_TREASURE); }
PORT_JUMP(RedChestFunc) { chest_func(gb, RED_TREASURE); }
PORT_JUMP(GreenChestFunc) { chest_func(gb, GREEN_TREASURE); }
PORT_JUMP(BlueChestFunc) { chest_func(gb, BLUE_TREASURE); }

// these framesets use OAM data that point
// to addresses in VRAM that are loaded
// with the treasure tiles
PORT_JUMP(GreyTreasureFunc)
{
	SET_DE(gb, ADDR_Frameset_681be);
	JUMP_TO(ADDR_TreasureFunc);
}

PORT_JUMP(RedTreasureFunc)
{
	SET_DE(gb, ADDR_Frameset_681c3);
	JUMP_TO(ADDR_TreasureFunc);
}

PORT_JUMP(GreenTreasureFunc)
{
	SET_DE(gb, ADDR_Frameset_681c8);
	JUMP_TO(ADDR_TreasureFunc);
}

PORT_JUMP(BlueTreasureFunc)
{
	SET_DE(gb, ADDR_Frameset_681cd);
	JUMP_TO(ADDR_TreasureFunc); // fallthrough
}

#define CHEST_UPDATE (ADDR_ChestFunc + 0x24) // .Update
#define CHEST_RET (ADDR_ChestFunc + 0x42) // .Ret
#define CHEST_OPEN (ADDR_ChestFunc + 0x43) // .Open

// hl = wCurObjUpdateFunction + 1: stores the update pointer with ld [hld]
static void set_update(GB *gb, uint16_t update)
{
	uint16_t hl = wCurObjUpdateFunction + 1;
	gb->cpu.a = (uint8_t)(update >> 8);
	wr8(gb, hl--, gb->cpu.a);
	gb->cpu.a = (uint8_t)update;
	wr8(gb, hl--, gb->cpu.a);
	SET_HL(gb, hl);
}

// Waits for the chest to be opened (state $18 with no auto-move running).
PORT_LOCAL(ChestFunc, Update)
{
	SET_HL(gb, wCurObjState);
	gb->cpu.a = rd8(gb, wCurObjState);
	alu_cp(gb, gb->cpu.a, OBJSTATE_18);
	if (!(gb->cpu.f & FLAG_Z))
		return;
	gb->cpu.a = rd8(gb, wAutoMoveState);
	if (alu_and(gb, gb->cpu.a, gb->cpu.a))
		return;
	SET_DE(gb, ADDR_Frameset_68003);
	CALL(SetObjectFramesetPtr);
	gb->cpu.a = 86;
	wr8(gb, REG_HL(gb), 86); // ld [hli], a (hl is reloaded next)
	set_update(gb, CHEST_OPEN);
}

PORT_LOCAL(ChestFunc, Ret) {}

// The opening animation, then the treasure appears with its fanfare.
PORT_LOCAL_JUMP(ChestFunc, Open)
{
	uint16_t hl = wCurObjStateDuration;
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	alu_cp(gb, gb->cpu.a, 73);
	if (gb->cpu.f & FLAG_Z)
		play_sfx(gb, SFX_035);
	uint8_t d = alu_dec(gb, rd8(gb, hl));
	wr8(gb, hl, d);
	if (d) {
		RET();
		return;
	}
	SET_DE(gb, ADDR_Frameset_68060);
	CALL(SetObjectFramesetPtr);
	set_update(gb, CHEST_RET);
	gb->cpu.a = alu_and(gb, rd8(gb, wCurObjInteractionType), INTERACTION_MASK);
	uint16_t params;
	alu_cp(gb, gb->cpu.a, OBJ_INTERACTION_GREY_TREASURE);
	if (gb->cpu.f & FLAG_Z) {
		params = ADDR_ObjParams_GreyTreasure;
	} else {
		alu_cp(gb, gb->cpu.a, OBJ_INTERACTION_RED_TREASURE);
		if (gb->cpu.f & FLAG_Z) {
			params = ADDR_ObjParams_RedTreasure;
		} else {
			alu_cp(gb, gb->cpu.a, OBJ_INTERACTION_GREEN_TREASURE);
			params = (gb->cpu.f & FLAG_Z) ? ADDR_ObjParams_GreenTreasure : ADDR_ObjParams_BlueTreasure;
		}
	}
	SET_BC(gb, params);
	// play_music2 MUSIC_TREASURE_FANFARE
	gb->cpu.a = (uint8_t)(MUSIC_TREASURE_FANFARE >> 8);
	wr8(gb, hMusicIDBackup + 0, gb->cpu.a);
	wr8(gb, hMusicID + 0, gb->cpu.a);
	gb->cpu.a = (uint8_t)MUSIC_TREASURE_FANFARE;
	wr8(gb, hMusicIDBackup + 1, gb->cpu.a);
	wr8(gb, hMusicID + 1, gb->cpu.a);
	JUMP(CreateObjectAtRelativePos);
}
