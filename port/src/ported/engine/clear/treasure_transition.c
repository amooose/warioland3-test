// Port of src/engine/clear/treasure_transition.asm
#include "ported.h"

PORT(SetTreasureTransitionParam)
{
	static const uint8_t treasures[][2] = {
		{LEAD_OVERALLS_T, LEAD_OVERALLS},
		{SWIMMING_FLIPPERS_T, SWIMMING_FLIPPERS},
		{HEAD_SMASH_HELMET_T, HEAD_SMASH_HELMET},
		{GRAB_GLOVE_T, GRAB_GLOVE},
		{GARLIC_T, GARLIC},
		{SUPER_JUMP_SLAM_OVERALLS_T, SUPER_JUMP_SLAM_OVERALLS},
		{HIGH_JUMP_BOOTS_T, HIGH_JUMP_BOOTS},
		{PRINCE_FROGS_GLOVES_T, PRINCE_FROGS_GLOVES},
		{SUPER_GRAB_GLOVES_T, SUPER_GRAB_GLOVES},
	};
	gb->cpu.a = rd8(gb, wLevelEndScreen);
	alu_cp(gb, gb->cpu.a, LVLEND_NO_TREASURE);
	if (gb->cpu.f & FLAG_Z) {
		gb->cpu.a = TRANSITION_RETURN_TO_MAP;
		wr8(gb, wTransitionParam, TRANSITION_RETURN_TO_MAP);
		return;
	}
	gb->cpu.a = alu_dec(gb, gb->cpu.a);
	gb->cpu.b = gb->cpu.a;
	gb->cpu.a = alu_srl(gb, rd8(gb, wLevel));
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_srl(gb, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.a);
	gb->cpu.a = alu_add(gb, gb->cpu.a, gb->cpu.b);
	gb->cpu.e = gb->cpu.a;
	gb->cpu.d = 0;
	uint16_t hl = alu_add16(gb, ADDR_LevelTreasureIDs_WithoutTemple, REG_DE(gb));
	SET_HL(gb, hl);
	gb->cpu.a = rd8(gb, hl);
	wr8(gb, wTransitionParam, gb->cpu.a);
	for (unsigned i = 0; i < sizeof(treasures) / sizeof(treasures[0]); i++) {
		alu_cp(gb, gb->cpu.a, treasures[i][0]);
		if (gb->cpu.f & FLAG_Z) {
			gb->cpu.a = treasures[i][1] | ACTION_HELP_LEVEL_CLEAR;
			wr8(gb, wPowerUpLevel, gb->cpu.a);
			return;
		}
	}
}
