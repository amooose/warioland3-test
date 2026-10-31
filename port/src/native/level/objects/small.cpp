// The simplest objects: the fire (fire.asm), the key (key.asm), Kobatto
// (kobatto.asm), the wall cracks (wall_crack.asm), the musical coins
// (musical_coin.asm), the clear gates (clear_gate.asm), the sparks
// (spark.asm, water_spark.asm), the stars (stars.asm), the flame block
// (flame_block.asm), Muddee's stingers (muddee_stinger.asm) and the dummy
// (dummy.asm). See object.hpp.
#include "../object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t FIRE_UPDATE = ADDR_FireFunc + 0x24; // .Update
constexpr uint16_t KEY_UPDATE = ADDR_KeyFunc + 0x25; // .Update
constexpr uint16_t KOBATTO_UPDATE = ADDR_KobattoFunc + 0x30; // .Update
constexpr uint16_t WALL_CRACK_UPDATE = ADDR_WallCrackFunc + 0x16; // .Update
constexpr uint16_t MUSICAL_COIN_UPDATE = ADDR_MusicalCoinFunc + 0x25; // .Update
constexpr uint16_t CLEAR_GATE_UPDATE = ADDR_ClearGateFunc + 0x2e; // .Update
constexpr uint16_t WATER_SPARK_IDLE = ADDR_WaterSparkFunc + 0x25; // .Func_4a482
constexpr uint16_t WATER_SPARK_SPARK = ADDR_WaterSparkFunc + 0x40; // .Func_4a49d
constexpr uint16_t SPARK_UPDATE = ADDR_SparkFunc + 0x2b; // .Update
constexpr uint16_t FLAME_BLOCK_UPDATE = ADDR_FlameBlockFunc + 0x30; // .Update
constexpr uint16_t FLAME_BLOCK_DESTROY = ADDR_FlameBlockFunc + 0x4b; // .Detroy
constexpr uint16_t STINGER_UPDATE = ADDR_MuddeeStingerFunc + 0x14; // .Func_4ca2a

class Small : public Object {
public:
	using Object::Object;

	void fire(); // FireFunc: just burns
	void key(); // KeyFunc
	void key_update(); // KeyFunc.Update
	void kobatto(); // KobattoFunc
	void kobatto_update(); // KobattoFunc.Update
	void wall_crack(uint16_t frameset); // WallCrackFunc
	void wall_crack_update(); // WallCrackFunc.Update
	void musical_coin(); // MusicalCoinFunc
	void clear_gate(uint16_t frameset); // ClearGateFunc
	void water_spark(); // WaterSparkFunc
	void water_spark_idle(); // WaterSparkFunc.Func_4a482
	void water_spark_spark(); // WaterSparkFunc.Func_4a49d
	void spark(); // SparkFunc
	void spark_update(); // SparkFunc.Update
	void stars_at_wario(); // StarsFunc_WithYOffset
	void stars_start(); // StarsFunc_SetFlags
	void stars(); // StarsFunc
	void vanishing(); // Func_4c860
	void flame_block(); // FlameBlockFunc
	void flame_block_update(); // FlameBlockFunc.Update
	void flame_block_destroy(); // FlameBlockFunc.Detroy
	void muddee_stinger(); // MuddeeStingerFunc
	void muddee_stinger_update(); // MuddeeStingerFunc.Func_4ca2a
};

} // namespace

void Small::fire()
{
	start(FIRE_UPDATE, ADDR_Frameset_69c0a);
	mem_[wCurObjCollBoxRight] = 0x07;
	mem_[wCurObjCollBoxLeft] = 0xf8;
	mem_[wCurObjCollBoxBottom] = 0x00;
}

void Small::key()
{
	start(KEY_UPDATE, ADDR_Frameset_6804f);
	mem_[wCurObjCollBoxRight] = 4;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-5);
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-4);
}

// gone once collected
// key_update; the musical coin's too
void Small::key_update()
{
	if (state() != OBJSTATE_18)
		return;
	flags() = 0;
	collect();
}

// a bat flying along Data_60ec9's path
void Small::kobatto()
{
	start(KOBATTO_UPDATE, ADDR_Frameset_69041);
	mem_[wCurObjCollBoxRight] = 6;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-7);
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-4);
	var1() = 0x01;
	var2() = ADDR_Data_60ec9 & 0xff;
	var3() = ADDR_Data_60ec9 >> 8;
}

void Small::kobatto_update()
{
	SET_DE(gb_, ADDR_Data_60ec9);
	routines_.call(ADDR_ApplyObjSetMovement);
}

void Small::wall_crack(uint16_t frameset)
{
	start(WALL_CRACK_UPDATE, frameset);
	wall_crack_update();
}

void Small::wall_crack_update()
{
	no_actions();
}

void Small::musical_coin()
{
	start(MUSICAL_COIN_UPDATE, ADDR_Frameset_68036);
	mem_[wCurObjCollBoxRight] = 1;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-17);
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-7);
}

// raised out of the way once the minigame's cleared
void Small::clear_gate(uint16_t frameset)
{
	start(CLEAR_GATE_UPDATE, frameset);
	mem_[wCurObjCollBoxRight] = 0x05;
	mem_[wCurObjCollBoxLeft] = 0xea;
	mem_[wCurObjCollBoxBottom] = 0xfe;
	if (!mem_[wIsMinigameCleared])
		return;
	var2() = 32;
	routines_.call(ADDR_MoveObjectUpByVar2);
}

void Small::water_spark()
{
	start(WATER_SPARK_IDLE, ADDR_Frameset_69b1e);
	mem_[wCurObjCollBoxRight] = 0x06;
	mem_[wCurObjCollBoxLeft] = 0xf9;
	mem_[wCurObjCollBoxBottom] = 0xfd;
}

// until something sets its state
void Small::water_spark_idle()
{
	if (!state())
		return;
	state() = 0;
	set_update(WATER_SPARK_SPARK);
	set_frameset(ADDR_Frameset_69b12);
	state_duration() = 100;
}

void Small::water_spark_spark()
{
	no_actions();
	if (--state_duration())
		return;
	set_update(WATER_SPARK_IDLE);
	set_frameset(ADDR_Frameset_69b1e);
}

void Small::spark()
{
	start(SPARK_UPDATE, ADDR_Frameset_69bd9);
	state_duration() = 0x28;
	var1() = 0x01;
	mem_[wCurObjCollBoxRight] = 0x02;
	mem_[wCurObjCollBoxLeft] = 0xfc;
	mem_[wCurObjCollBoxBottom] = 0xfb;
}

// back and forth (or up and down), $50 steps each way, every other frame
void Small::spark_update()
{
	if (mem_[wGlobalCounter] & 1)
		return;
	bool forward = var1();
	if (!--state_duration()) {
		state_duration() = 0x50;
		var1() = forward ? 0 : 1;
		return;
	}
	bool horizontal = mem_[wCurObjId] == SPARK_HORIZONTAL;
	if (forward)
		horizontal ? move_right() : move_up();
	else
		horizontal ? move_left() : move_down();
}

// StarsFunc_WithYOffset: at Wario, 4 pixels lower
void Small::stars_at_wario()
{
	uint16_t wario_y = static_cast<uint16_t>(mem_[wWarioYPos + 0] << 8 | mem_[wWarioYPos + 1]);
	y() = static_cast<uint16_t>(wario_y + 4);
	x() = static_cast<uint16_t>(mem_[wWarioXPos + 0] << 8 | mem_[wWarioXPos + 1]);
	stars_start();
}

void Small::stars_start()
{
	set_update(ADDR_StarsFunc);
	flags() = (flags() & ~(1 << OBJFLAG_GRABBED_F)) | OBJFLAG_NO_COLLISION;
	stars();
}

// gone after their time
void Small::stars()
{
	no_actions();
	if (!--state_duration())
		flags() = 0;
}

// Func_4c860: the same, despawned
void Small::vanishing()
{
	no_actions();
	if (!--state_duration())
		despawn();
}

// gone for good once its torches are lit
void Small::flame_block()
{
	if (mem_[wFlameBlockCleared]) {
		flags() = 0;
		return;
	}
	start(FLAME_BLOCK_UPDATE, ADDR_Frameset_695de);
	mem_[wCurObjCollBoxRight] = 13;
	mem_[wCurObjCollBoxLeft] = static_cast<uint8_t>(-14);
	mem_[wCurObjCollBoxBottom] = static_cast<uint8_t>(-1);
}

void Small::flame_block_update()
{
	uint8_t lit = mem_[wNumLitTorches];
	if (lit != NUM_FLAME_BLOCK_TORCHES)
		return;
	mem_[wFlameBlockCleared] = lit;
	set_frameset(ADDR_Frameset_695e7);
	state_duration() = 159;
	set_update(FLAME_BLOCK_DESTROY);
}

// melts, solid until its last 25 frames
void Small::flame_block_destroy()
{
	if (state_duration() < 25)
		no_actions();
	if (!--state_duration())
		flags() = 0;
}

void Small::muddee_stinger()
{
	flags() |= OBJFLAG_INVISIBLE | OBJFLAG_NO_COLLISION;
	substate() |= 1 << OBJSUBFLAG_UNK_4_F;
	set_update(STINGER_UPDATE);
}

// on Muddee (the first among objects 1 to 8), placed by its frame: a
// stinger's offset in Data_4caed (or Data_4cb0d for the second) for
// frames 0 to $f, else right on it and harmless
void Small::muddee_stinger_update()
{
	frame_duration() = 0x02;
	if (state() == 0x0a)
		mem_[w1d147] = 0x01;
	if (mem_[w1d147]) {
		flags() = 0;
		return;
	}
	uint16_t muddee = 0;
	for (uint16_t obj = wObj1; ; obj += OBJ_STRUCT_LENGTH) {
		if (mem_[static_cast<uint16_t>(obj + OBJ_ID)] == MUDDEE && (mem_[static_cast<uint16_t>(obj + OBJ_FLAGS)] & 1)) {
			muddee = obj;
			break;
		}
		if (obj == wObj8)
			return;
	}
	uint8_t frame = mem_[static_cast<uint16_t>(muddee + OBJ_FRAME)];
	uint16_t muddee_y = mem_.word(static_cast<uint16_t>(muddee + OBJ_Y_POS));
	uint16_t muddee_x = mem_.word(static_cast<uint16_t>(muddee + OBJ_X_POS));
	if (frame >= 0x10) {
		y() = muddee_y;
		x() = muddee_x;
		no_actions();
		return;
	}
	uint16_t offsets = mem_[wCurObjId] == MUDDEE_STINGER2 ? ADDR_Data_4cb0d : ADDR_Data_4caed;
	uint16_t offset = static_cast<uint16_t>(offsets + frame * 2);
	y() = static_cast<uint16_t>(muddee_y + static_cast<int8_t>(mem_[offset]));
	x() = static_cast<uint16_t>(muddee_x + static_cast<int8_t>(mem_[static_cast<uint16_t>(offset + 1)]));
}

} // namespace wl3::level

using Small = wl3::level::Small;

PORT_EX(FireFunc, 0, ALL_REGISTERS) { Small(gb).fire(); }
PORT_LOCAL_EX(FireFunc, Update, 0, ALL_REGISTERS) {}
PORT_EX(KeyFunc, 0, ALL_REGISTERS) { Small(gb).key(); }
PORT_LOCAL_EX(KeyFunc, Update, 0, ALL_REGISTERS) { Small(gb).key_update(); }
PORT_EX(KobattoFunc, 0, ALL_REGISTERS) { Small(gb).kobatto(); }
PORT_LOCAL_EX(KobattoFunc, Update, 0, ALL_REGISTERS) { Small(gb).kobatto_update(); }
PORT_EX(WallCrackClosedFunc, 0, ALL_REGISTERS) { Small(gb).wall_crack(ADDR_Frameset_6a052); }
PORT_EX(WallCrackOpenFunc, 0, ALL_REGISTERS) { Small(gb).wall_crack(ADDR_Frameset_6a05f); }
PORT_EX(WallCrackFunc, 0, ALL_REGISTERS) { Small(gb).wall_crack(REG_DE(gb)); }
PORT_LOCAL_EX(WallCrackFunc, Update, 0, ALL_REGISTERS) { Small(gb).wall_crack_update(); }
PORT_EX(MusicalCoinFunc, 0, ALL_REGISTERS) { Small(gb).musical_coin(); }
PORT_LOCAL_EX(MusicalCoinFunc, Update, 0, ALL_REGISTERS) { Small(gb).key_update(); }
PORT_EX(ClearGateFunc, 0, ALL_REGISTERS) { Small(gb).clear_gate(REG_DE(gb)); }
PORT_EX(ClearGate1Func, 0, ALL_REGISTERS) { Small(gb).clear_gate(ADDR_Frameset_698a8); }
PORT_EX(ClearGate2Func, 0, ALL_REGISTERS) { Small(gb).clear_gate(ADDR_Frameset_698ab); }
PORT_EX(ClearGate3Func, 0, ALL_REGISTERS) { Small(gb).clear_gate(ADDR_Frameset_698ae); }
PORT_LOCAL_EX(ClearGateFunc, Update, 0, ALL_REGISTERS) {}
PORT_EX(WaterSparkFunc, 0, ALL_REGISTERS) { Small(gb).water_spark(); }
PORT_LOCAL_EX(WaterSparkFunc, Func_4a482, 0, ALL_REGISTERS) { Small(gb).water_spark_idle(); }
PORT_LOCAL_EX(WaterSparkFunc, Func_4a49d, 0, ALL_REGISTERS) { Small(gb).water_spark_spark(); }
PORT_EX(SparkFunc, 0, ALL_REGISTERS) { Small(gb).spark(); }
PORT_LOCAL_EX(SparkFunc, Update, 0, ALL_REGISTERS) { Small(gb).spark_update(); }
PORT_EX(StarsFunc_WithYOffset, 0, ALL_REGISTERS) { Small(gb).stars_at_wario(); }
PORT_EX(StarsFunc_SetFlags, 0, ALL_REGISTERS) { Small(gb).stars_start(); }
PORT_EX(StarsFunc, 0, ALL_REGISTERS) { Small(gb).stars(); }
PORT_EX(Func_4c860, 0, ALL_REGISTERS) { Small(gb).vanishing(); }
PORT_EX(DummyObjectFunc, 0, ALL_REGISTERS) { wl3::Memory mem(gb); mem[wCurObjFlags] = 0; }
PORT_EX(FlameBlockFunc, 0, ALL_REGISTERS) { Small(gb).flame_block(); }
PORT_LOCAL_EX(FlameBlockFunc, Update, 0, ALL_REGISTERS) { Small(gb).flame_block_update(); }
PORT_LOCAL_EX(FlameBlockFunc, Detroy, 0, ALL_REGISTERS) { Small(gb).flame_block_destroy(); }
PORT_EX(MuddeeStingerFunc, 0, ALL_REGISTERS) { Small(gb).muddee_stinger(); }
PORT_LOCAL_EX(MuddeeStingerFunc, Func_4ca2a, 0, ALL_REGISTERS) { Small(gb).muddee_stinger_update(); }
