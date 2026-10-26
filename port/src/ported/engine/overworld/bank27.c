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
