// Port of src/engine/overworld/bank2d.asm
#include "ported.h"

// sets the coords and state
// of SceneObj in hl, with params in de
PORT(InitSceneObjParams)
{
	uint16_t hl = REG_HL(gb), de = REG_DE(gb);
	wr8(gb, hl++, rd8(gb, de));   // y
	de++;
	wr8(gb, hl++, rd8(gb, de));   // x
	uint8_t l = alu_inc(gb, (uint8_t)hl);
	l = alu_inc(gb, l);           // inc l / inc l
	hl = (uint16_t)((hl & 0xff00) | l);
	gb->cpu.f = FLAG_Z;           // xor a
	wr8(gb, hl++, 0);             // duration
	wr8(gb, hl++, 0);             // frameset offset
	de++;
	gb->cpu.a = rd8(gb, de);
	wr8(gb, hl, gb->cpu.a);       // state
	SET_HL(gb, hl);
	SET_DE(gb, de);
}

// Animates the scene object at wCurSceneObjPtr.
PORT_JUMP(UpdateCurOWObjAnimation)
{
	gb->cpu.b = 0x2a; // bank of the overworld framesets
	gb->cpu.l = rd8(gb, wCurSceneObjPtr + 0) | (1 << 2); // set 2, l
	gb->cpu.a = rd8(gb, wCurSceneObjPtr + 1);
	gb->cpu.h = gb->cpu.a;
	JUMP(UpdateOWAnimation);
}

// When the animation of the scene object in hl has finished, resets it and
// sets its unk6 to b.
PORT(Func_b4a0d)
{
	alu_cp(gb, rd8(gb, wOWAnimationFinished), 0xff);
	if (!(gb->cpu.f & FLAG_Z)) {
		gb->cpu.a = rd8(gb, wOWAnimationFinished);
		return;
	}
	uint16_t hl = (uint16_t)(REG_HL(gb) | (1 << 2)); // set 2, l
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, hl++, 0);   // duration
	wr8(gb, hl++, 0);   // frameset offset
	gb->cpu.a = gb->cpu.b;
	wr8(gb, hl, gb->cpu.a); // unk6
	SET_HL(gb, hl);
	SET_BC(gb, hl);
}

// xor a / ld hl, wOWFuncCounter / ld [hli], a / inc [hl]
static void advance_ow_func(GB *gb)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWFuncCounter, 0);
	SET_HL(gb, wOWFuncCounter + 1);
	wr8(gb, wOWFuncCounter + 1, alu_inc(gb, rd8(gb, wOWFuncCounter + 1)));
}

// a = number of frames to wait
PORT(WaitOWFunc)
{
	SET_HL(gb, wOWFuncCounter);
	alu_cp(gb, gb->cpu.a, rd8(gb, wOWFuncCounter));
	if (!(gb->cpu.f & FLAG_C))
		return;
	advance_ow_func(gb);
}

PORT(AdvanceOWFunc)
{
	advance_ow_func(gb);
}

PORT(EndOWFunc)
{
	gb->cpu.a = 0;
	gb->cpu.f = FLAG_Z; // xor a
	wr8(gb, wOWSceneActionParam, 0);
	wr8(gb, wOWFuncCounter, 0);
	wr8(gb, wOWFuncCounter + 1, 0); // w2d062
	SET_HL(gb, wOWFuncCounter + 1);
}

PORT_JUMP(WaitOWObjDisappear)
{
	gb->cpu.b = 0x00;
	JUMP(WaitOWObjState); // fallthrough
}

// advances the OW func once the state at hl is b
PORT_JUMP(WaitOWObjState)
{
	gb->cpu.a = rd8(gb, REG_HL(gb));
	alu_cp(gb, gb->cpu.a, gb->cpu.b);
	if (!(gb->cpu.f & FLAG_Z)) {
		RET();
		return;
	}
	JUMP(AdvanceOWFunc); // jr
}
