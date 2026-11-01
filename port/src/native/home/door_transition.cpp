// home/door_transition (src/home/door_transition.asm), converted from the register-level
// C port by tools/c2cpp.py: the same logic, as a class over AsmModule,
// keeping the original's registers, flags and return addresses for its
// callers (see src/native/asm_module.hpp).
#include "../asm_module.hpp"

namespace wl3::home {

namespace {

class HomeDoorTransition : public AsmModule {
public:
	using AsmModule::AsmModule;

	void port_VBlank_DoorTransition()
	{
		cpu_.ime = false; // di
		// back up current VBlank function
		SET_HL(gb_, wVBlankFunc);
		SET_DE(gb_, wBackupVBlankFunc);
		cpu_.b = 3;
		CALL(CopyHLToDE);

		// then overwrite it
		SET_HL(gb_, ADDR_VBlank_DoorTransition + 0x19); // .Func
		SET_DE(gb_, wVBlankFunc);
		cpu_.b = 3; // .end - .Func
		CALL(CopyHLToDE);
		cpu_.ime = true; // ei
		cpu_.ime_delay = 0;
	}

	// DoorTransition.PrepareNextStepInTransitionEffect
	void prepare_next_step()
	{
		cpu_.a = rd8(wDoorTransitionCounter);
		alu_.cp(cpu_.a, 16);
		if (!(cpu_.f & FLAG_C))
			return; // >= 16
		uint16_t hl = wTransitionEffectStep;
		cpu_.a = alu_.add(rd8(hl), 2);
		wr8(hl++, cpu_.a);
		// wDoorTransitionEffectBGPtr: one row below
		cpu_.a = alu_.add(rd8(hl), TILEMAP_WIDTH);
		wr8(hl++, cpu_.a);
		cpu_.a = alu_.adc(rd8(hl), 0x00);
		alu_.cp(cpu_.a, (uint8_t)((v0BGMap0 + TILEMAP_WIDTH * TILEMAP_HEIGHT) >> 8));
		if (!(cpu_.f & FLAG_C))
			cpu_.a = alu_.sub(cpu_.a, (uint8_t)((TILEMAP_WIDTH * TILEMAP_HEIGHT) >> 8)); // wrap up
		// .no_wrap_up
		wr8(hl--, cpu_.a);

		// one column left
		cpu_.a = alu_.dec(rd8(hl));
		wr8(hl, cpu_.a);
		cpu_.a = alu_.and_(cpu_.a, 0x1f);
		alu_.cp(cpu_.a, 0x1f);
		if (cpu_.f & FLAG_Z) {
			// wrap right
			cpu_.a = alu_.add(rd8(hl), TILEMAP_WIDTH);
			wr8(hl, cpu_.a);
		}
		SET_HL(gb_, hl);
	}

	void port_DoorTransition()
	{
		// push_wram $03
		cpu_.a = rd8(rWBK);
		push_af();
		cpu_.a = 0x03;
		wr8(rWBK, cpu_.a);
		cpu_.a = rd8(wDoorTransitionOrientation);
		cpu_.c = cpu_.a;
		cpu_.a = alu_.or_(rd8(wDoorTransitionCounter), cpu_.c);
		if (cpu_.f & FLAG_Z) {
			cpu_.a = 0x01;
			wr8(wUnused_c186, 0x01);

			// set VBlank function
			CALL(VBlank_DoorTransition);

			// toggle wDoorTransitionOrientation
			cpu_.a = alu_.xor_(rd8(wDoorTransitionOrientation), 0x1);
			wr8(wDoorTransitionOrientation, cpu_.a);

			cpu_.a = 1;
			wr8(wTransitionEffectStep, 1);
			// start wDoorTransitionEffectBGPtr with Wario's current tile
			cpu_.a = rd8(wWarioTileBGPtr + 0);
			wr8(wDoorTransitionEffectBGPtr + 0, cpu_.a);
			cpu_.a = rd8(wWarioTileBGPtr + 1);
			wr8(wDoorTransitionEffectBGPtr + 1, cpu_.a);
		} else {
			// .already_started: toggle wDoorTransitionOrientation
			cpu_.a = alu_.xor_(rd8(wDoorTransitionOrientation), 0x1);
			wr8(wDoorTransitionOrientation, cpu_.a);
			if (!(cpu_.f & FLAG_Z)) {
				prepare_next_step();
			} else {
				// .skip_incr_step
				SET_HL(gb_, wDoorTransitionCounter);
				wr8(wDoorTransitionCounter, alu_.inc(rd8(wDoorTransitionCounter)));
				cpu_.a = rd8(wDoorTransitionCounter);
				alu_.cp(cpu_.a, 17);
				if (!(cpu_.f & FLAG_C)) {
					// restore backed up VBlank function
					cpu_.ime = false; // di
					SET_HL(gb_, wBackupVBlankFunc);
					SET_DE(gb_, wVBlankFunc);
					cpu_.b = 3;
					CALL(CopyHLToDE);
					cpu_.ime = true; // ei
					cpu_.ime_delay = 0;

					// finish door transition and advance level state
					cpu_.a = 0;
					cpu_.f = FLAG_Z; // xor a
					wr8(wDoorTransitionCounter, 0);
					wr8(wUnused_c186, 0);
					wr8(wDoorTransitionOrientation, 0);
					// stop_music
					wr8(hMusicID + 0, 0xff);
					wr8(hMusicID + 1, 0x00);
					// stop_sfx
					wr8(hSFXID + 0, 0xff);
					cpu_.a = 0x00;
					wr8(hSFXID + 1, 0x00);
					SET_HL(gb_, wSubState);
					wr8(wSubState, alu_.inc(rd8(wSubState)));
				}
			}
		}
		// .done: pop_wram
		pop_af();
		wr8(rWBK, cpu_.a);
	}

	// one tile of the door transition effect: set the attribute's palette to 7
	// (and $7f / or $0f, in VRAM1), then overwrite the tile with $7f
	void cover_tile(uint16_t hl)
	{
		wr8(rVBK, 1); // BANK("VRAM1")
		wr8(hl, (uint8_t)((rd8(hl) & 0x7f) | 0x0f));
		wr8(rVBK, 0); // BANK("VRAM0")
		wr8(hl, 0x7f);
	}

	// do b tiles up (add hl, -TILEMAP_WIDTH, wrapping down below v0BGMap0)
	uint16_t fill_upwards(uint16_t hl, uint8_t b)
	{
		do {
			cover_tile(hl);
			hl = (uint16_t)(hl - TILEMAP_WIDTH);
			if ((hl >> 8) < (v0BGMap0 >> 8))
				hl = (uint16_t)((((hl >> 8) + 0x04) & 0xff) << 8 | (hl & 0xff)); // wrap down
		} while (--b);
		return hl;
	}

	// do b tiles left (ld [hli], wrapping a row back when l reaches a multiple of 32)
	uint16_t fill_left(uint16_t hl, uint8_t b)
	{
		do {
			cover_tile(hl);
			hl++;
			if (!(hl & 0x1f))
				hl = (uint16_t)(hl - TILEMAP_WIDTH); // wrap left
		} while (--b);
		return hl;
	}

	// reached from wVBlankFunc's jp
	void port_VBlank_DoorTransition__DoTransitionEffect()
	{
		// apply scroll
		wr8(rSCY, rd8(wSCY));
		cpu_.a = rd8(wSCX);
		wr8(rSCX, cpu_.a);

		// push_wram $03
		cpu_.a = rd8(rWBK);
		push_af();
		wr8(rWBK, 0x03);
		SET_DE(gb_, (uint16_t)-TILEMAP_WIDTH);
		uint8_t orientation = rd8(wDoorTransitionOrientation);
		uint8_t step = rd8(wTransitionEffectStep);
		uint16_t hl = (uint16_t)(rd8(wDoorTransitionEffectBGPtr + 0) |
			rd8(wDoorTransitionEffectBGPtr + 1) << 8);
		cpu_.c = step;
		if (orientation) {
			// up then left
			hl = fill_upwards(hl, step);
			hl = fill_left(hl, (uint8_t)(step + 1));
		} else {
			// .left_then_up
			hl = fill_left(hl, step);
			hl = fill_upwards(hl, step);
		}
		cpu_.b = 0;
		SET_HL(gb_, hl);
		// .asm_a8e: pop_wram
		pop_af();
		wr8(rWBK, cpu_.a);
	}
};

} // namespace

} // namespace wl3::home

using wl3::home::HomeDoorTransition;

PORT_EX(VBlank_DoorTransition, 0, 0) { HomeDoorTransition(gb).port_VBlank_DoorTransition(); }
PORT_EX(DoorTransition, 0, 0) { HomeDoorTransition(gb).port_DoorTransition(); }
PORT_LOCAL_EX(VBlank_DoorTransition, DoTransitionEffect, 0, 0) { HomeDoorTransition(gb).port_VBlank_DoorTransition__DoTransitionEffect(); }
