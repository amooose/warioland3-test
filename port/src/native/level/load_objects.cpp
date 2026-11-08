// Loading and creating objects (src/engine/level/load_objects.asm): a room's
// object group (its object data, enemy graphics and palette), spawning the
// objects placed in the level map, the objects other objects create, and
// despawning them all.
//
// The callers are C that may read registers after these, so each entry
// point leaves the registers and flags the original does.
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"
#include "../home/pals.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t HIDDEN_FIGURE_POINTERS = ADDR_LoadHiddenFigureData + 0x1e; // .pointers
constexpr uint8_t ENEMY_GFX_1_SLOT_1 = 0x68; // BANK("Enemy Gfx 1 Slot 1"), the 3 others after it
constexpr uint8_t VRAM1 = BANK_v1Tiles0;

class ObjectLoader {
public:
	explicit ObjectLoader(GB *gb) : gb_(gb), cpu_(gb->cpu), mem_(gb), routines_(gb) {}

	void spawn(); // SpawnObject
	void create_got_pos(uint8_t x_hi, uint16_t slot_pos, uint16_t data); // CreateObject_GotPos
	void create_from_cur_obj_pos(); // _CreateObjectFromCurObjPos
	void create_at_relative_pos(); // _CreateObjectAtRelativePos
	void despawn_all(); // DespawnAllObjects
	void load_group(); // LoadObjectGroupData
	void load_hidden_figure_data(); // LoadHiddenFigureData

private:
	uint16_t slot(int n) const { return static_cast<uint16_t>(wObj1 + n * OBJ_STRUCT_LENGTH); }
	int free_slot();
	uint8_t relative(uint16_t pos, uint8_t offset, uint16_t &at);
	uint16_t load_data_pointers(uint16_t from, uint8_t count);

	GB *gb_;
	CPU &cpu_;
	Memory mem_;
	Routines routines_;
};

// the first slot without OBJFLAG_ACTIVE (rra through the carry, hl at its
// flags), or -1 with hl at the last slot's
int ObjectLoader::free_slot()
{
	for (int n = 0; n < NUM_OBJECTS; n++) {
		SET_HL(gb_, slot(n));
		cpu_.a = flags::rra(cpu_, mem_[slot(n)]);
		if (!(cpu_.f & FLAG_C))
			return n;
	}
	return -1;
}

// input: de = the block, c = its entry in wObjDataPointers. Spawns the
// block's object (bit 7 of the block set: not spawned yet) in the first
// free slot, if it's on screen.
void ObjectLoader::spawn()
{
	uint8_t x_hi = mem_[hXPosHi];
	cpu_.a = x_hi;
	cpu_.f = flags::cp(x_hi, 0x0a);
	if (x_hi >= 0x0a)
		return;

	int n = -1;
	for (int i = 0; i < NUM_OBJECTS && n < 0; i++) {
		cpu_.a = flags::rra(cpu_, mem_[slot(i)]);
		SET_HL(gb_, slot(i) + 1);
		if (!(cpu_.f & FLAG_C))
			n = i;
	}
	if (n < 0)
		return;

	Byte block = mem_[REG_DE(gb_)];
	uint8_t v = block;
	if (!(v & 0x80)) {
		cpu_.a = static_cast<uint8_t>(v << 1 | v >> 7); // rlca
		cpu_.f = 0;
		return;
	}
	block = v & 0x7f; // spawned now

	uint16_t obj = slot(n);
	mem_[obj + OBJ_UNK_01] = cpu_.e;
	mem_[obj + OBJ_UNK_02] = static_cast<uint8_t>(((mem_[wccec] - 1) << 5) + cpu_.d);
	mem_[obj + OBJ_Y_POS + 0] = mem_[hYPosLo];
	mem_[obj + OBJ_Y_POS + 1] = mem_[hYPosHi];
	mem_[obj + OBJ_X_POS + 0] = mem_[hXPosLo];
	mem_[obj + OBJ_X_POS + 1] = x_hi;

	cpu_.c = static_cast<uint8_t>((cpu_.c - 1) << 1);
	cpu_.b = 0;
	uint16_t data = Word(gb_, wObjDataPointers + cpu_.c);
	mem_[obj + OBJ_ID] = mem_[data + 0];
	mem_[obj + OBJ_INTERACTION_TYPE] = mem_[data + 1];
	mem_[obj + OBJ_COLLBOX_TOP] = mem_[data + 2];
	mem_[obj + OBJ_COLLBOX_BOTTOM] = 0;
	mem_[obj + OBJ_FRAME] = 0;
	mem_[obj + OBJ_OAM_PTR + 0] = mem_[data + 3];
	mem_[obj + OBJ_OAM_PTR + 1] = mem_[data + 4];
	mem_[obj + OBJ_FRAME_DURATION] = 0x7f;
	mem_[obj + OBJ_SUBSTATE] = 0x1 | OBJSUBFLAG_UNINITIALISED;
	mem_[obj + OBJ_STATE] = 0; // OBJSTATE_00
	mem_[obj + OBJ_ACTION] = 2 | (1 << 7);
	mem_[obj + OBJ_UPDATE_FUNCTION + 0] = mem_[data + 5];
	mem_[obj + OBJ_UPDATE_FUNCTION + 1] = mem_[data + 6];
	uint8_t obj_flags = mem_[data + 7] | OBJFLAG_ACTIVE | OBJFLAG_INVISIBLE;
	mem_[obj + OBJ_FLAGS] = obj_flags;
	SET_DE(gb_, obj);
	SET_HL(gb_, data + 7);
	cpu_.a = obj_flags;
	cpu_.f = flags::or_(obj_flags);
}

// The rest of an object made by another: x_hi to slot_pos (the slot's
// OBJ_X_POS + 1), then the creation data at bc (left in bc)
void ObjectLoader::create_got_pos(uint8_t x_hi, uint16_t slot_pos, uint16_t data)
{
	mem_[slot_pos] = x_hi;
	// ld [hli], a (16-bit), then ld e, l / ld d, h and inc e (8-bit) along
	// the fields from OBJ_ID
	uint16_t id_at = static_cast<uint16_t>(slot_pos + 1);
	auto field = [&](uint8_t f) {
		return static_cast<uint16_t>((id_at & 0xff00) | static_cast<uint8_t>(id_at + f - OBJ_ID));
	};
	uint16_t src = data;
	for (uint8_t f = OBJ_ID; f <= OBJ_COLLBOX_RIGHT; f++)
		mem_[field(f)] = mem_[src++];
	for (uint8_t f = OBJ_OAM_PTR; f < OBJ_OAM_PTR + 4; f++) // and OBJ_FRAMESET_PTR
		mem_[field(f)] = mem_[src++];
	mem_[field(OBJ_FRAME_DURATION)] = 0;
	mem_[field(OBJ_FRAMESET_OFFSET)] = 0;
	for (uint8_t f = OBJ_STATE_DURATION; f <= OBJ_STATE; f++)
		mem_[field(f)] = mem_[src++];
	mem_[field(OBJ_ACTION)] = 0; // OBJACTION_NONE
	mem_[field(OBJ_UPDATE_FUNCTION + 0)] = mem_[src++];
	mem_[field(OBJ_UPDATE_FUNCTION + 1)] = mem_[src++];
	uint8_t obj_flags = mem_[src] | OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT;
	mem_[field(OBJ_FLAGS)] = obj_flags;
	SET_DE(gb_, field(OBJ_FLAGS));
	SET_HL(gb_, src);
	SET_BC(gb_, data);
	cpu_.a = obj_flags;
	cpu_.f = flags::or_(obj_flags);
}

// bc = the creation data: at wCurObj's position
void ObjectLoader::create_from_cur_obj_pos()
{
	int n = free_slot();
	if (n < 0)
		return;
	uint16_t obj = slot(n);
	mem_[obj + OBJ_FLAGS] = OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT;
	mem_[obj + OBJ_UNK_01] = mem_[wCurObjUnk01];
	mem_[obj + OBJ_UNK_02] = mem_[wCurObjUnk02];
	mem_[obj + OBJ_Y_POS + 0] = mem_[wCurObjYPos + 0];
	mem_[obj + OBJ_Y_POS + 1] = mem_[wCurObjYPos + 1];
	mem_[obj + OBJ_X_POS + 0] = mem_[wCurObjXPos + 0];
	create_got_pos(mem_[wCurObjXPos + 1], obj + OBJ_X_POS + 1, REG_BC(gb_));
}

// pos + the signed offset: its low byte to `at`, the high byte returned
uint8_t ObjectLoader::relative(uint16_t pos, uint8_t offset, uint16_t &at)
{
	uint8_t lo = mem_[pos];
	unsigned sum = lo + offset;
	mem_[at++] = static_cast<uint8_t>(sum);
	uint8_t hi = mem_[pos + 1];
	bool carry = sum > 0xff;
	if (offset & 0x80)
		return carry ? hi : static_cast<uint8_t>(hi - 1);
	return carry ? static_cast<uint8_t>(hi + 1) : hi;
}

// bc = the creation data, starting with the y and x offsets from wCurObj
void ObjectLoader::create_at_relative_pos()
{
	int n = free_slot();
	if (n < 0)
		return;
	uint16_t obj = slot(n);
	uint16_t data = REG_BC(gb_);
	mem_[obj + OBJ_FLAGS] = OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT;
	mem_[obj + OBJ_UNK_01] = mem_[wCurObjUnk01];
	mem_[obj + OBJ_UNK_02] = mem_[wCurObjUnk02];
	uint16_t at = obj + OBJ_Y_POS;
	uint8_t y_hi = relative(wCurObjYPos, mem_[data], at);
	mem_[at++] = y_hi;
	uint8_t x_hi = relative(wCurObjXPos, mem_[data + 1], at);
	create_got_pos(x_hi, at, data + 2);
}

// despawns every object: the transient ones are cleared, the others go
// back to their place in the level map (DespawnObject)
void ObjectLoader::despawn_all()
{
	cpu_.h = wObjects >> 8;
	for (int n = 0; n < NUM_OBJECTS; n++) {
		uint8_t l = static_cast<uint8_t>(wObjects + OBJ_STRUCT_LENGTH * n);
		cpu_.l = l;
		uint8_t a = flags::rra(cpu_, mem_[REG_HL(gb_)]);
		cpu_.a = a;
		if (!(cpu_.f & FLAG_C))
			continue; // not active
		a = flags::rla(cpu_, a);
		a = flags::rla(cpu_, a);
		a = flags::rla(cpu_, a);
		cpu_.a = a;
		if (cpu_.f & FLAG_C) { // transient
			mem_[REG_HL(gb_)] = 0;
			cpu_.a = 0;
			cpu_.f = FLAG_Z;
			continue;
		}
		cpu_.f = flags::inc(cpu_.f, l);
		cpu_.f = flags::inc(cpu_.f, static_cast<uint8_t>(l + 1));
		cpu_.l = static_cast<uint8_t>(l + 2);
		routines_.farcall(BANK_DespawnObject, ADDR_DespawnObject);
	}
}

// wObjDataPtr01 on: up to count of the pointers at from, until a $ffxx
// one (hffa0 counts down); returns where the reading stopped
uint16_t ObjectLoader::load_data_pointers(uint16_t from, uint8_t count)
{
	mem_[hffa0] = count;
	uint8_t dest = static_cast<uint8_t>(wObjDataPtr01); // inc e: within the page
	uint16_t src = from;
	for (;;) {
		uint8_t lo = mem_[src++];
		uint8_t hi = mem_[src++];
		if (hi == 0xff)
			break;
		mem_[static_cast<uint16_t>((wObjDataPtr01 & 0xff00) | dest++)] = lo;
		mem_[static_cast<uint16_t>((wObjDataPtr01 & 0xff00) | dest++)] = hi;
		if (!--mem_[hffa0])
			break;
	}
	return src;
}

// the Hidden Figure's room: its object data alone
void ObjectLoader::load_hidden_figure_data()
{
	load_data_pointers(HIDDEN_FIGURE_POINTERS, 0x0d);
}

// the room's object group: the object data pointers (the dummy object in
// every unused slot), the chest, key and musical coin data and their
// palette, the 4 enemy graphics slots, then the object palettes after the
// pointers (LoadObjPals)
void ObjectLoader::load_group()
{
	for (int i = 0; i < 0x10; i++)
		Word(gb_, wObjDataPointers + 2 * i) = ADDR_DummyObjectData;
	uint8_t group = mem_[wObjectGroup];
	if (group == OBJECT_GROUP_HIDDEN_FIGURE) {
		load_hidden_figure_data();
		return;
	}

	uint16_t entry = static_cast<uint16_t>(ADDR_ObjectGroups + group * 4);
	uint16_t items = Word(gb_, entry);
	for (int i = 0; i < 6; i++) // chest, key and musical coin
		mem_[wChestObjDataPtr + i] = mem_[items + i];
	home::copy_short(gb_, static_cast<uint16_t>(items + 6), wTempPals2 + 3 * PAL_SIZE, PAL_SIZE);

	uint16_t enemies = Word(gb_, entry + 2);
	mem_[rVBK] = VRAM1;
	mem_[hffa0] = mem_[enemies];
	static constexpr uint16_t dest[4] = {
		v1Tiles0, v1Tiles0 + 0x40 * TILE_SIZE, v1Tiles1, v1Tiles1 + 0x40 * TILE_SIZE,
	};
	uint16_t ptrs = enemies + 1;
	for (int i = 0; i < 4; i++) {
		uint16_t src = Word(gb_, ptrs);
		ptrs += 2;
		uint8_t bank = static_cast<uint8_t>(mem_[hffa0] + ENEMY_GFX_1_SLOT_1 + i);
		routines_.in_bank(bank, ADDR_Decompress, [&] { routines_.decompress(src, dest[i]); }); // hcall Decompress
	}
	mem_[rVBK] = 0; // VRAM0
	home::load_obj_pals(gb_, load_data_pointers(ptrs, 13));
}

} // namespace

} // namespace wl3::level

using wl3::level::ObjectLoader;

PORT(SpawnObject) { ObjectLoader(gb).spawn(); }

// a = OBJ_X_POS + 1's byte, hl = where it goes, bc = the creation data
PORT(CreateObject_GotPos) { ObjectLoader(gb).create_got_pos(gb->cpu.a, REG_HL(gb), REG_BC(gb)); }

PORT(_CreateObjectFromCurObjPos) { ObjectLoader(gb).create_from_cur_obj_pos(); }
PORT(_CreateObjectAtRelativePos) { ObjectLoader(gb).create_at_relative_pos(); }
PORT(DespawnAllObjects) { ObjectLoader(gb).despawn_all(); }
PORT_EX(LoadHiddenFigureData, 0, ALL_REGISTERS) { ObjectLoader(gb).load_hidden_figure_data(); }

// ends in jp LoadObjPals (or LoadHiddenFigureData's ret)
// LoadRoom farcalls it and reads no registers after
PORT_EX(LoadObjectGroupData, 0, ALL_REGISTERS) { ObjectLoader(gb).load_group(); }
