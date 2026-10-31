// The Prince Frog's Gloves demonstration; see demo.hpp.
#include "demo.hpp"

namespace wl3::action_help {

void PrinceFrogsGlovesDemo::run()
{
	run_state();
	draw();
}

void PrinceFrogsGlovesDemo::run_state()
{
	switch (wram_.action_help_state()) {
	case 0: call(ADDR_ActionHelp_Init); break;
	case 1:
		if (timer_done())
			start();
		break;
	case 2: wait_then_walk(); break;
	case 3: walk_to_water(); break;
	case 4: jump_in(); break;
	case 5:
		if (!timer_done())
			break;
		call(ADDR_ActionHelp_ShowDDownInput);
		call(ADDR_ActionHelp_ShowWarioSubmerge);
		next_state();
		break;
	case 6: dive(); break;
	case 7: swim_right(); break;
	case 8: swim_down(0x03, false); break;
	case 9: stop_at(0x50, false); break;
	case 10: call(ADDR_ActionHelp_FlashBInput); break;
	case 11: thrust(); break;
	case 12: stop_at(0x34, true); break;
	case 13: short_thrust(); break;
	case 14: thrust_again(); break;
	case 15: swim_left_up(); break;
	case 16: surface(); break;
	case 17: jump_out(); break;
	case 18: land(); break;
	default: unrealistic(); break;
	}
}

// Swims left to x, and stops there.
void PrinceFrogsGlovesDemo::stop_at(uint8_t x, bool b_prompt)
{
	swim();
	if (--wario().x() >= x)
		return;
	wario().x() = x;
	call(ADDR_ActionHelp_ShowWarioUnderwaterStopped);
	if (b_prompt)
		show_b();
	next_state();
}

// Thrusts right to x `to`, at half speed from x slow_from.
bool PrinceFrogsGlovesDemo::thrust_right(uint8_t slow_from, uint8_t to)
{
	Byte x = wario().x();
	if (x >= slow_from && (wram_.action_help_counter() & 0x01))
		return false;
	return ++x >= to;
}

void PrinceFrogsGlovesDemo::thrust()
{
	swim_fast();
	if (!thrust_right(0x58, 0x6c))
		return;
	call(ADDR_ActionHelp_ShowWarioSubmerge);
	clear_buttons();
	next_state();
}

// B let go early: the thrust stops.
void PrinceFrogsGlovesDemo::short_thrust()
{
	swim_fast();
	if (!thrust_right(0x3c, 0x4c))
		return;
	clear_buttons();
	timer() = 0x0c;
	next_state();
}

// B again: the thrust goes on.
void PrinceFrogsGlovesDemo::thrust_again()
{
	swim_fast();
	if (timer()) {
		if (--timer())
			return;
		call(ADDR_ActionHelp_ShowWarioUnderwaterStopped);
		show_b();
	}
	if (!thrust_right(0x58, 0x6c))
		return;
	call(ADDR_ActionHelp_ShowWarioSubmerge);
	clear_buttons();
	next_state();
}

} // namespace wl3::action_help

// Called from ActionHelp (src/native/action_help/action_help.cpp) only.
PORT_COVERED(PrinceFrogsGlovesActionHelp);
PORT_COVERED(Func_1fad42);
PORT_COVERED(Func_1fad7a);
PORT_COVERED(Func_1fad75);
PORT_COVERED(Func_1fadea);
PORT_COVERED(Func_1fadfa);
PORT_COVERED(Func_1fae0f);
PORT_COVERED(Func_1fae3d);
PORT_COVERED(Func_1fae4d);
PORT_COVERED(Func_1fae7f);
PORT_COVERED(Func_1faeba);
PORT_COVERED(Func_1faef0);
PORT_COVERED(Func_1faf19);
PORT_COVERED(Func_1faf4e);
PORT_COVERED(Func_1faf7a);
PORT_COVERED(Func_1fafb1);
PORT_COVERED(Func_1faff5);
PORT_COVERED(Func_1fb023);
PORT_COVERED(Func_1fb061);
PORT_COVERED(Func_1fb078);
