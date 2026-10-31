// What the demonstrations show (common1.asm and common2.asm): the prompts,
// Wario's looks, the enemies and objects, and their sprites; see demo.hpp.
#include "demo.hpp"

namespace wl3::action_help {

// ---------------------------------------------------------------------------
// The shared states

void Demo::init_demo()
{
	wram_.animated_tiles_frame_duration() = 0;
	wram_.animated_tiles_gfx() = 0;
	wram_.animated_tiles_frame_count() = 0;
	wram_.animated_tiles_frame() = 0;
	wram_.action_help_frozen() = 0;
	wram_.room_animated_tiles_enabled() = 0;
	wram_.scy() = 0x04;
	routines_.farcall(BANK_Func_1e48bc, ADDR_Func_1e48bc);
	routines_.farcall(BANK_Func_1e4a47, ADDR_Func_1e4a47);
	clear_dpad();
	clear_buttons();
	timer() = 0x60;
	next_state();
}

void Demo::flash(MenuObject obj, uint16_t off, uint16_t on)
{
	wram_.action_help_frozen() = TRUE;
	if (wram_.action_help_counter() & 0x03)
		return;
	uint8_t t = ++timer();
	if (!(t & 0x01)) {
		obj.set_frameset(off);
		return;
	}
	play_sfx(mem_, SFX_0EC);
	obj.set_frameset(on);
	if (t < 0x07)
		return;
	timer() = 0;
	wram_.action_help_frozen() = 0;
	next_state();
}

// ---------------------------------------------------------------------------
// Wario

void Demo::set_wario_oam(uint16_t oam)
{
	mem_[wActionHelpWarioOAMPtr + 0] = static_cast<uint8_t>(oam >> 8);
	mem_[wActionHelpWarioOAMPtr + 1] = static_cast<uint8_t>(oam);
}

void Demo::set_wario_oam(uint16_t oam, uint8_t bank)
{
	set_wario_oam(oam);
	wram_.action_help_wario_oam_bank() = bank;
}

void Demo::show_wario(uint16_t sfx, uint8_t gfx_bank, uint16_t gfx, uint16_t frameset, uint16_t oam)
{
	if (sfx)
		play_sfx(mem_, sfx);
	queue_wario_gfx(gfx_bank, gfx);
	wario_frameset(frameset);
	set_wario_oam(oam);
}

void Demo::show_initial_wario_walk()
{
	queue_wario_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	wario().init(0x40, 0x40, ADDR_Frameset_149c5);
	wario_var() = 0;
	wram_.sfx_timer() = 0;
	set_wario_oam(ADDR_OAM_1426c);
}

void Demo::show_wario_idle()
{
	show_wario(0, BANK_WarioIdleGfx, ADDR_WarioIdleGfx, ADDR_Frameset_1425f, ADDR_OAM_14000);
}

void Demo::show_wario_walk()
{
	show_wario(0, BANK_WarioWalkGfx, ADDR_WarioWalkGfx, ADDR_Frameset_149c5, ADDR_OAM_1426c);
}

void Demo::show_wario_jump()
{
	show_wario(SFX_JUMP, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx, ADDR_Frameset_15f97, ADDR_OAM_15955);
	wario_var() = 0;
}

void Demo::show_wario_jump_left()
{
	show_wario(SFX_JUMP, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx, ADDR_Frameset_15f94, ADDR_OAM_15955);
	wario_var() = 0;
}

void Demo::show_wario_attack()
{
	queue_wario_gfx(BANK_WarioAttackGfx, ADDR_WarioAttackGfx);
	wram_.sfx_timer() = 0;
	bool garlic = wram_.action_help_power_up() >= GARLIC;
	wario_frameset(garlic ? ADDR_Frameset_1522d : ADDR_Frameset_151fd);
	set_wario_oam(ADDR_OAM_14d1b);
}

void Demo::show_wario_smash_attack()
{
	bool super = wram_.action_help_power_up() >= SUPER_JUMP_SLAM_OVERALLS;
	wario_frameset(super ? ADDR_Frameset_16055 : ADDR_Frameset_15f91);
	wario_var() = 0x18;
}

void Demo::show_wario_ground_pound()
{
	play_sfx(mem_, SFX_GROUND_POUND);
	wario_frameset(ADDR_Frameset_16009);
}

void Demo::show_wario_ground_shake()
{
	play_sfx(mem_, SFX_GROUND_SHAKE);
	wario_frameset(ADDR_Frameset_15fc5);
}

void Demo::show_wario_land()
{
	play_sfx(mem_, SFX_LAND);
	wario_frameset(ADDR_Frameset_15fa1);
}

void Demo::show_wario_swim()
{
	show_wario(SFX_SPLASH, BANK_WarioSwimGfx, ADDR_WarioSwimGfx, ADDR_Frameset_1555b, ADDR_OAM_15254);
}

void Demo::show_wario_submerge()
{
	wario_frameset(ADDR_Frameset_1558b);
	wram_.sfx_timer() = 0;
}

void Demo::show_wario_underwater_thrust()
{
	wario_frameset(ADDR_Frameset_155c5);
	wram_.sfx_timer() = 0;
}

void Demo::show_wario_grab()
{
	show_wario(SFX_GRAB, BANK_WarioThrowGfx, ADDR_WarioThrowGfx, ADDR_Frameset_1640c, ADDR_OAM_1606a);
}

void Demo::show_wario_slow_grab()
{
	show_wario(SFX_GRAB, BANK_WarioThrowGfx, ADDR_WarioThrowGfx, ADDR_Frameset_16415, ADDR_OAM_1606a);
}

void Demo::show_wario_begin_grab()
{
	show_wario(0, BANK_WarioWalkGfx, ADDR_WarioWalkGfx, ADDR_Frameset_14a23, ADDR_OAM_1426c);
}

void Demo::show_wario_begin_throw()
{
	show_wario(SFX_02C, BANK_WarioThrowGfx, ADDR_WarioThrowGfx, ADDR_Frameset_1642f, ADDR_OAM_1606a);
}

void Demo::show_wario_bump()
{
	show_wario(SFX_BUMP, BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx, ADDR_Frameset_15f7f, ADDR_OAM_15955);
}

void Demo::show_wario_slide()
{
	show_wario(0, BANK_WarioSlideGfx, ADDR_WarioSlideGfx, ADDR_Frameset_1671d, ADDR_OAM_1644a);
}

void Demo::show_wario_charge_throw()
{
	wario_frameset(ADDR_Frameset_16435);
	wram_.sfx_timer() = 0;
}

// ---------------------------------------------------------------------------
// The enemies and objects

void Demo::show_paragoom_walk()
{
	init_object(enemy(), 0x40, 0xe8, ADDR_Frameset_1e6f36, ADDR_OAM_1e6bab, BANK_OAM_1e6bab);
}

void Demo::show_count_richtertoff_idle()
{
	init_object(enemy(), 0x40, 0xe8, ADDR_Frameset_1e6f4d, ADDR_OAM_1e6bab, BANK_OAM_1e6bab, false);
}

void Demo::show_enemy_vanish(MenuObject obj)
{
	obj.set_frameset(ADDR_Frameset_d4a5);
	mem_[static_cast<uint16_t>(obj.addr() + 8)] = static_cast<uint8_t>(ADDR_OAM_d20e >> 8);
	mem_[static_cast<uint16_t>(obj.addr() + 9)] = static_cast<uint8_t>(ADDR_OAM_d20e);
	mem_[static_cast<uint16_t>(obj.addr() + 10)] = BANK_OAM_d20e;
	play_sfx(mem_, SFX_VANISH);
}

void Demo::show_vertical_block_pair()
{
	init_object(enemy(), 0x40, 0xe8, ADDR_Frameset_1e6b93, ADDR_OAM_1e697c, BANK_OAM_1e697c, false);
}

void Demo::show_slide()
{
	init_object(enemy(), 0x40, 0xd8, ADDR_Frameset_1e6b88, ADDR_OAM_1e697c, BANK_OAM_1e697c, false);
}

void Demo::show_overhead_pillar()
{
	init_object(object(wMenuObj6), 0x20, 0xe8, ADDR_Frameset_1e6b85, ADDR_OAM_1e697c, BANK_OAM_1e697c, false);
}

void Demo::hide_block_debris(MenuObject obj)
{
	init_object(obj, 0xa0, 0xe8, ADDR_Frameset_1e6b64, ADDR_OAM_1e697c, BANK_OAM_1e697c);
}

void Demo::show_block_debris(MenuObject obj, uint8_t y, uint8_t x)
{
	play_sfx(mem_, SFX_BLOCK_BREAK);
	// each read just before its write: obj may be wMenuObj4 itself
	obj.y() = static_cast<uint8_t>(enemy().y() - y);
	obj.x() = static_cast<uint8_t>(enemy().x() - x);
	init_object(obj, obj.y(), obj.x(), ADDR_Frameset_d49a, ADDR_OAM_d20e, BANK_OAM_d20e);
}

// ---------------------------------------------------------------------------
// Sprites

void Demo::load_object_sprite(MenuObject obj)
{
	obj.set_cur_sprite();
	// the OAM pointer (high byte first) and bank, after the frameset
	uint16_t base = obj.addr();
	uint16_t oam_field = static_cast<uint16_t>((base & 0xff00) | static_cast<uint8_t>(base + 8));
	uint16_t oam = static_cast<uint16_t>(mem_[oam_field] << 8 | mem_[static_cast<uint16_t>(oam_field + 1)]);
	uint8_t bank = mem_[static_cast<uint16_t>(oam_field + 2)];
	wram_.temp_bank() = bank;
	SET_HL(gb_, oam);
	routines_.farcall(bank, ADDR_LoadSprite);
}

void Demo::load_wario_sprite()
{
	wario().set_cur_sprite();
	SET_HL(gb_, static_cast<uint16_t>(mem_[wActionHelpWarioOAMPtr] << 8 | mem_[wActionHelpWarioOAMPtr + 1]));
	routines_.farcall(0x05, ADDR_LoadSprite);
}

void SpecialDemo::load_wario_sprite_from_bank()
{
	wario().set_cur_sprite();
	SET_HL(gb_, static_cast<uint16_t>(mem_[wActionHelpWarioOAMPtr] << 8 | mem_[wActionHelpWarioOAMPtr + 1]));
	uint8_t bank = wram_.action_help_wario_oam_bank();
	wram_.temp_bank() = bank;
	routines_.farcall(bank, ADDR_LoadSprite);
}

// ---------------------------------------------------------------------------
// The transformations' (common2.asm)

void SpecialDemo::hide_block_debris_far(MenuObject obj)
{
	routines_.in_bank(BANK_ActionHelp_HideBlockDebris, ADDR_ActionHelp_HideBlockDebris,
		[&] { hide_block_debris(obj); });
}

void SpecialDemo::flash_b_far()
{
	routines_.in_bank(BANK_ActionHelp_FlashBInput, ADDR_ActionHelp_FlashBInput, [&] { flash_b(); });
}

void SpecialDemo::flash_a_far()
{
	routines_.in_bank(BANK_ActionHelp_FlashAInput, ADDR_ActionHelp_FlashAInput, [&] { flash_a(); });
}

void SpecialDemo::show_wario_idle2()
{
	queue_wario_gfx(BANK_WarioIdleGfx, ADDR_WarioIdleGfx);
	wario_frameset(ADDR_Frameset_1425f);
	set_wario_oam(ADDR_OAM_14000, BANK_OAM_14000);
}

void SpecialDemo::show_wario_walk2()
{
	queue_wario_gfx(BANK_WarioWalkGfx, ADDR_WarioWalkGfx);
	wario_frameset(ADDR_Frameset_149c5);
	set_wario_oam(ADDR_OAM_1426c);
}

void SpecialDemo::show_wario_from(uint8_t gfx_bank, uint16_t gfx, uint16_t frameset, uint16_t oam, uint8_t oam_bank)
{
	queue_wario_gfx(gfx_bank, gfx);
	wario_frameset(frameset);
	wario_var() = 0;
	set_wario_oam(oam, oam_bank);
}

void SpecialDemo::show_wario_jump2()
{
	show_wario_from(BANK_WarioAirborneGfx, ADDR_WarioAirborneGfx, ADDR_Frameset_15f97, ADDR_OAM_15955, BANK_OAM_15955);
}

void SpecialDemo::show_wario_hang_stopped()
{
	show_wario_from(BANK_WarioHangGfx, ADDR_WarioHangGfx, ADDR_Frameset_1feec4, ADDR_OAM_1fee05, BANK_OAM_1fee05);
}

void SpecialDemo::show_wario_hang_rail_right()
{
	show_wario_from(BANK_WarioHangGfx, ADDR_WarioHangGfx, ADDR_Frameset_1feeb4, ADDR_OAM_1fee05, BANK_OAM_1fee05);
}

void SpecialDemo::show_wario_bat_transform()
{
	queue_wario_gfx(BANK_WarioBatGfx, ADDR_WarioBatGfx);
	wario_frameset(ADDR_Frameset_1fd479);
	set_wario_oam(ADDR_OAM_1fd031);
}

void SpecialDemo::show_wario_bat(uint16_t frameset)
{
	wram_.sfx_timer() = 0;
	wario_frameset(frameset);
}

void SpecialDemo::show_wario_vampire_transform()
{
	play_sfx(mem_, SFX_045);
	wario_frameset(ADDR_Frameset_1fd439);
}

void SpecialDemo::show_owl_fly_up()
{
	uint8_t y = static_cast<uint8_t>(wario().y() - 0x20);
	uint8_t x = static_cast<uint8_t>(wario().x() - 0x08);
	init_object(enemy(), y, x, ADDR_Frameset_d7b4, ADDR_OAM_d763, BANK_OAM_d763);
}

} // namespace wl3::action_help

// Called from the demonstrations only.
PORT_COVERED(ActionHelp_Init);
PORT_COVERED(ActionHelp_FlashDDownInput);
PORT_COVERED(ActionHelp_FlashDUpInput);
PORT_COVERED(ActionHelp_FlashBInput);
PORT_COVERED(ActionHelp_FlashAInput);
PORT_COVERED(ActionHelp_ClearDPadInput);
PORT_COVERED(ActionHelp_ShowDRightInput);
PORT_COVERED(ActionHelp_ShowDLeftInput);
PORT_COVERED(ActionHelp_ShowDDownInput);
PORT_COVERED(ActionHelp_ShowDUpInput);
PORT_COVERED(ActionHelp_ShowDRightAndDDownInput);
PORT_COVERED(ActionHelp_ClearButtonsInput);
PORT_COVERED(ActionHelp_ShowAInput);
PORT_COVERED(ActionHelp_ShowBInput);
PORT_COVERED(ActionHelp_HideObj5BlockDebris);
PORT_COVERED(ActionHelp_HideBlockDebris);
PORT_COVERED(ActionHelp_ShowBlockDebrisWithYOffset);
PORT_COVERED(ActionHelp_ShowBlockDebris);
PORT_COVERED(ActionHelp_ShowInitialWarioWalk);
PORT_COVERED(ActionHelp_ShowWarioJump);
PORT_COVERED(ActionHelp_ShowWarioJumpLeft);
PORT_COVERED(ActionHelp_ShowWarioSmashAttack);
PORT_COVERED(ActionHelp_ShowWarioGroundPound);
PORT_COVERED(ActionHelp_ShowWarioGroundShake);
PORT_COVERED(ActionHelp_ShowWarioLand);
PORT_COVERED(ActionHelp_ShowWarioIdle);
PORT_COVERED(ActionHelp_ShowWarioWalk);
PORT_COVERED(ActionHelp_ShowWarioSwim);
PORT_COVERED(ActionHelp_ShowWarioSwimLeft);
PORT_COVERED(ActionHelp_ShowWarioSubmerge);
PORT_COVERED(ActionHelp_ShowWarioUnderwaterThrust);
PORT_COVERED(ActionHelp_ShowWarioUnderwaterStopped);
PORT_COVERED(ActionHelp_ShowWarioGrab);
PORT_COVERED(ActionHelp_ShowWarioSlowGrab);
PORT_COVERED(ActionHelp_ShowWarioBeginGrab);
PORT_COVERED(ActionHelp_ShowWarioGrabWalk);
PORT_COVERED(ActionHelp_ShowWarioBeginThrow);
PORT_COVERED(ActionHelp_ShowWarioThrow);
PORT_COVERED(ActionHelp_ShowWarioChargeThrow);
PORT_COVERED(ActionHelp_ShowWarioFullyChargedGrab);
PORT_COVERED(ActionHelp_ShowWarioFullyChargedThrow);
PORT_COVERED(ActionHelp_ShowWarioAttack);
PORT_COVERED(ActionHelp_ShowWarioRoll);
PORT_COVERED(ActionHelp_ShowWarioBump);
PORT_COVERED(ActionHelp_ShowWarioSlide);
PORT_COVERED(ActionHelp_ShowParaGoomWalk);
PORT_COVERED(ActionHelp_ShowParaGoomStun);
PORT_COVERED(ActionHelp_ShowParaGoomThrown);
PORT_COVERED(ActionHelp_ShowObj4EnemyVanish);
PORT_COVERED(ActionHelp_ShowEnemyVanish);
PORT_COVERED(ActionHelp_ShowFutamoguAngry);
PORT_COVERED(ActionHelp_ShowFutamoguHappy);
PORT_COVERED(ActionHelp_ShowCountRichtertoffIdle);
PORT_COVERED(ActionHelp_ShowCountRichtertoffStun);
PORT_COVERED(ActionHelp_ShowCountRichtertoffThrown);
PORT_COVERED(ActionHelp_ShowSingleSolidBlock);
PORT_COVERED(ActionHelp_ShowVerticalBlockPair);
PORT_COVERED(ActionHelp_ShowSlide);
PORT_COVERED(ActionHelp_ShowOverheadPillar);
PORT_COVERED(LoadActionHelpWarioSprite);
PORT_COVERED(LoadActionHelpWarioSprite_FromBank);
PORT_COVERED(LoadActionHelpSprite_Far);
PORT_COVERED(ActionHelp2_Init);
PORT_COVERED(ActionHelp2_FlashBInput);
PORT_COVERED(ActionHelp2_FlashAInput);
PORT_COVERED(ActionHelp2_ClearDPadInput);
PORT_COVERED(ActionHelp2_ShowDLeftInput);
PORT_COVERED(ActionHelp2_ShowDRightInput);
PORT_COVERED(ActionHelp2_ShowDUpInput);
PORT_COVERED(ActionHelp2_ClearButtonsInput);
PORT_COVERED(ActionHelp2_ShowAInput);
PORT_COVERED(ActionHelp2_ShowBInput);
PORT_COVERED(ActionHelp2_ShowWarioIdle);
PORT_COVERED(ActionHelp2_ShowWarioWalk);
PORT_COVERED(ActionHelp2_ShowWarioJump);
PORT_COVERED(ActionHelp_ShowWarioHangVertical);
PORT_COVERED(ActionHelp_ShowWarioHangHorizontal);
PORT_COVERED(ActionHelp_ShowWarioHangOwlRight);
PORT_COVERED(ActionHelp_ShowWarioOwlMoveHang);
PORT_COVERED(ActionHelp_ShowWarioOwlMoveHangRight);
PORT_COVERED(ActionHelp_ShowWarioHangDrop);
PORT_COVERED(ActionHelp_ShowWarioHangStopped);
PORT_COVERED(ActionHelp_ShowWarioHangRailRight);
PORT_COVERED(ActionHelp_ShowWarioBatTransform);
PORT_COVERED(ActionHelp_ShowWarioBatRight);
PORT_COVERED(ActionHelp_ShowWarioBatLeft);
PORT_COVERED(ActionHelp_ShowWarioVampireTransform);
PORT_COVERED(ActionHelp_ShowOwlFlyUp);
PORT_COVERED(ActionHelp_ShowRailRotateRight);
PORT_COVERED(ActionHelp_ShowRailRotateLeft);
