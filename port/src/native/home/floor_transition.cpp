// home/floor_transition (src/home/floor_transition.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeFloorTransition : public AsmModule {
public:
	using AsmModule::AsmModule;

	// returns in c:
	// 6 if   $0 <= ypos <  $80
	// 5 if  $80 <= ypos < $100
	// 4 if $100 <= ypos < $180
	// 3 if $180 <= ypos < $200
	// 2 if $200 <= ypos < $280
	// 1 if $280 <= ypos < $300
	void port_GetFloorForYPos()
	{
		uint8_t hi = alu_.dec(rd8(wWarioYPos + 0));
		if (!hi)
			cpu_.c = 3;
		else if (!alu_.dec(hi))
			cpu_.c = 1;
		else
			cpu_.c = 5;

		cpu_.a = rd8(wWarioYPos + 1);
		alu_.cp(cpu_.a, 0x80);
		if (cpu_.f & FLAG_C)
			cpu_.c = alu_.inc(cpu_.c); // < $80
	}

	void port_UpdateFloor()
	{
		CALL(GetFloorForYPos);
		cpu_.a = cpu_.c;
		wr8(wFloor, cpu_.a);
	}

	// sets off the floor transition, after wFloor and wFloorTransitionTimer
	void start_floor_transition(uint8_t dir)
	{
		play_sfx(SFX_PIPE_EXIT);
		cpu_.a = dir;
		wr8(wFloorTransitionDir, dir);
		cpu_.a = TRUE;
		wr8(wIsFloorTransition, TRUE);
	}

	// c = new floor
	void port_StartDownwardsFloorTransition()
	{
		cpu_.a = cpu_.c;
		wr8(wFloor, cpu_.a);
		alu_.cp(cpu_.a, 5);
		cpu_.a = (cpu_.f & FLAG_Z) ? 0x68 : 0x80;
		wr8(wFloorTransitionTimer, cpu_.a);
		start_floor_transition(FLOOR_TRANSITION_UP);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wCameraRoomYScrollLimit, 0);
	}

	// c = new floor
	void port_StartUpwardsFloorTransition()
	{
		cpu_.a = cpu_.c;
		wr8(wFloor, cpu_.a);
		start_floor_transition(FLOOR_TRANSITION_DOWN);
		cpu_.a = 0x80;
		wr8(wFloorTransitionTimer, 0x80);
		cpu_.a = 0;
		cpu_.f = FLAG_Z; // xor a
		wr8(wCameraRoomYScrollLimit, 0);
	}

	// The floor check: CAM_TRANSITIONS and Wario on another floor; a = wFloor - c
	bool floor_changed()
	{
		cpu_.a = alu_.and_(rd8(wCameraConfigFlags), CAM_SCROLLING_MASK);
		alu_.cp(cpu_.a, CAM_TRANSITIONS);
		if (cpu_.f & FLAG_C)
			return false;
		CALL(GetFloorForYPos);
		cpu_.a = alu_.sub(rd8(wFloor), cpu_.c);
		return true;
	}

	void port_HandleDownwardsFloorTransition()
	{
		if (floor_changed() && !(cpu_.f & (FLAG_Z | FLAG_C))) {
			JUMP(StartDownwardsFloorTransition);
			return;
		}
		RET();
	}

	// unreferenced?
	void port_Func_1180()
	{
		if (floor_changed() && !(cpu_.f & FLAG_Z)) {
			if (cpu_.f & FLAG_C)
				JUMP(StartUpwardsFloorTransition);
			else
				JUMP(StartDownwardsFloorTransition);
			return;
		}
		RET();
	}

	void port_HandleUpwardsFloorTransition()
	{
		if (floor_changed() && (cpu_.f & FLAG_C)) {
			JUMP(StartUpwardsFloorTransition);
			return;
		}
		RET();
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeFloorTransition;

PORT_EX(GetFloorForYPos, 0, 0) { HomeFloorTransition(gb).port_GetFloorForYPos(); }
PORT_EX(UpdateFloor, 0, 0) { HomeFloorTransition(gb).port_UpdateFloor(); }
PORT_EX(StartDownwardsFloorTransition, 0, 0) { HomeFloorTransition(gb).port_StartDownwardsFloorTransition(); }
PORT_EX(StartUpwardsFloorTransition, 0, 0) { HomeFloorTransition(gb).port_StartUpwardsFloorTransition(); }
PORT_EX(HandleDownwardsFloorTransition, HOOK_MANUAL_RET, 0) { HomeFloorTransition(gb).port_HandleDownwardsFloorTransition(); }
PORT_EX(Func_1180, HOOK_MANUAL_RET, 0) { HomeFloorTransition(gb).port_Func_1180(); }
PORT_EX(HandleUpwardsFloorTransition, HOOK_MANUAL_RET, 0) { HomeFloorTransition(gb).port_HandleUpwardsFloorTransition(); }
