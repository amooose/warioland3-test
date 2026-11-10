// Loading and creating objects (src/engine/level/load_objects.asm): a room's
// object group (its object data, enemy graphics and palette), spawning the
// objects placed in the level map, the objects other objects create, and
// despawning them all.
//
// Nothing reads a register these leave (full fuzz tier).
#include "../flags.hpp"
#include "../native.hpp"
#include "../routines.hpp"
#include "../home/pals.hpp"
#include "object.hpp"

namespace wl3::level {

namespace {

constexpr uint16_t HIDDEN_FIGURE_POINTERS = ADDR_LoadHiddenFigureData + 0x1e; // .pointers
constexpr uint8_t ENEMY_GFX_1_SLOT_1 = 0x68; // BANK("Enemy Gfx 1 Slot 1"), the 3 others after it
constexpr uint8_t VRAM1 = BANK_v1Tiles0;

class ObjectLoader {
public:
	explicit ObjectLoader(GB *gb) : gb_(gb), mem_(gb), routines_(gb) {}

	void spawn(uint8_t index, uint16_t at); // SpawnObject
	void create_got_pos(uint8_t x_hi, uint16_t slot_pos, uint16_t data); // CreateObject_GotPos
	void create_from_cur_obj_pos(uint16_t data); // _CreateObjectFromCurObjPos
	void create_at_relative_pos(uint16_t data); // _CreateObjectAtRelativePos
	void despawn_all(); // DespawnAllObjects
	void load_group(); // LoadObjectGroupData
	void load_hidden_figure_data(); // LoadHiddenFigureData

private:
	uint16_t slot(int n) const { return static_cast<uint16_t>(wObj1 + n * OBJ_STRUCT_LENGTH); }
	int free_slot();
	uint8_t relative(uint16_t pos, uint8_t offset, uint16_t &at);
	uint16_t load_data_pointers(uint16_t from, uint8_t count);

	GB *gb_;
	Memory mem_;
	Routines routines_;
};

// the first slot without OBJFLAG_ACTIVE, or -1
int ObjectLoader::free_slot()
{
	for (int n = 0; n < NUM_OBJECTS; n++) {
		if (!(mem_[slot(n)] & 1))
			return n;
	}
	return -1;
}

// index = its entry in wObjDataPointers, block = its place in the object
// map. Spawns the block's object (bit 7 of the block set: not spawned yet)
// in the first free slot, if it's on screen.
void ObjectLoader::spawn(uint8_t index, uint16_t at)
{
	uint8_t x_hi = mem_.x_pos_hi();
	if (x_hi >= 0x0a)
		return;
	int n = free_slot();
	if (n < 0)
		return;

	Byte block = mem_[at];
	uint8_t v = block;
	if (!(v & 0x80))
		return;
	block = v & 0x7f; // spawned now

	uint16_t obj = slot(n);
	mem_[obj + OBJ_UNK_01] = static_cast<uint8_t>(at);
	mem_[obj + OBJ_UNK_02] = static_cast<uint8_t>(((mem_.unk_ccec() - 1) << 5) + (at >> 8));
	mem_[obj + OBJ_Y_POS + 0] = mem_.y_pos_lo();
	mem_[obj + OBJ_Y_POS + 1] = mem_.y_pos_hi();
	mem_[obj + OBJ_X_POS + 0] = mem_.x_pos_lo();
	mem_[obj + OBJ_X_POS + 1] = x_hi;

	uint16_t data = Word(gb_, wObjDataPointers + static_cast<uint8_t>((index - 1) << 1));
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
	mem_[obj + OBJ_FLAGS] = mem_[data + 7] | OBJFLAG_ACTIVE | OBJFLAG_INVISIBLE;
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
	mem_[field(OBJ_FLAGS)] = mem_[src] | OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT;
}

// bc = the creation data: at wCurObj's position
void ObjectLoader::create_from_cur_obj_pos(uint16_t data)
{
	int n = free_slot();
	if (n < 0)
		return;
	uint16_t obj = slot(n);
	mem_[obj + OBJ_FLAGS] = OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT;
	mem_[obj + OBJ_UNK_01] = mem_.cur_obj_unk01();
	mem_[obj + OBJ_UNK_02] = mem_.cur_obj_unk02();
	mem_[obj + OBJ_Y_POS + 0] = mem_.cur_obj_y().lo();
	mem_[obj + OBJ_Y_POS + 1] = mem_.cur_obj_y().hi();
	mem_[obj + OBJ_X_POS + 0] = mem_.cur_obj_x().lo();
	create_got_pos(mem_.cur_obj_x().hi(), obj + OBJ_X_POS + 1, data);
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
void ObjectLoader::create_at_relative_pos(uint16_t data)
{
	int n = free_slot();
	if (n < 0)
		return;
	uint16_t obj = slot(n);
	mem_[obj + OBJ_FLAGS] = OBJFLAG_ACTIVE | OBJFLAG_TRANSIENT;
	mem_[obj + OBJ_UNK_01] = mem_.cur_obj_unk01();
	mem_[obj + OBJ_UNK_02] = mem_.cur_obj_unk02();
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
	for (int n = 0; n < NUM_OBJECTS; n++) {
		uint16_t obj = static_cast<uint16_t>(wObjects + OBJ_STRUCT_LENGTH * n);
		uint8_t obj_flags = mem_[obj];
		if (!(obj_flags & 1))
			continue; // not active
		if (obj_flags & OBJFLAG_TRANSIENT) {
			mem_[obj] = 0;
			continue;
		}
		routines_.in_bank(BANK_DespawnObject, ADDR_DespawnObject, [&] { despawn_object(gb_, static_cast<uint16_t>(obj + 2)); });
	}
}

// wObjDataPtr01 on: up to count of the pointers at from, until a $ffxx
// one (hffa0 counts down); returns where the reading stopped
uint16_t ObjectLoader::load_data_pointers(uint16_t from, uint8_t count)
{
	mem_.unk_ffa0() = count;
	uint8_t dest = static_cast<uint8_t>(wObjDataPtr01); // inc e: within the page
	uint16_t src = from;
	for (;;) {
		uint8_t lo = mem_[src++];
		uint8_t hi = mem_[src++];
		if (hi == 0xff)
			break;
		mem_[static_cast<uint16_t>((wObjDataPtr01 & 0xff00) | dest++)] = lo;
		mem_[static_cast<uint16_t>((wObjDataPtr01 & 0xff00) | dest++)] = hi;
		if (!--mem_.unk_ffa0())
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
	uint8_t group = mem_.object_group();
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
	mem_.unk_ffa0() = mem_[enemies];
	static constexpr uint16_t dest[4] = {
		v1Tiles0, v1Tiles0 + 0x40 * TILE_SIZE, v1Tiles1, v1Tiles1 + 0x40 * TILE_SIZE,
	};
	uint16_t ptrs = enemies + 1;
	for (int i = 0; i < 4; i++) {
		uint16_t src = Word(gb_, ptrs);
		ptrs += 2;
		uint8_t bank = static_cast<uint8_t>(mem_.unk_ffa0() + ENEMY_GFX_1_SLOT_1 + i);
		routines_.in_bank(bank, ADDR_Decompress, [&] { routines_.decompress(src, dest[i]); }); // hcall Decompress
	}
	mem_[rVBK] = 0; // VRAM0
	home::load_obj_pals(gb_, load_data_pointers(ptrs, 13));
}

} // namespace

void spawn_object(GB *gb, uint8_t index, uint16_t at) { ObjectLoader(gb).spawn(index, at); }
void create_object_from_cur_obj_pos(GB *gb, uint16_t params) { ObjectLoader(gb).create_from_cur_obj_pos(params); }
void create_object_at_relative_pos(GB *gb, uint16_t params) { ObjectLoader(gb).create_at_relative_pos(params); }

} // namespace wl3::level

using namespace wl3::level;

// c = the entry, de = the object map entry
PORT_EX(SpawnObject, 0, ALL_REGISTERS) { spawn_object(gb, gb->cpu.c, REG_DE(gb)); }

// a = OBJ_X_POS + 1's byte, hl = where it goes, bc = the creation data
PORT_EX(CreateObject_GotPos, 0, ALL_REGISTERS) { ObjectLoader(gb).create_got_pos(gb->cpu.a, REG_HL(gb), REG_BC(gb)); }

PORT_EX(_CreateObjectFromCurObjPos, 0, ALL_REGISTERS) { ObjectLoader(gb).create_from_cur_obj_pos(REG_BC(gb)); }
PORT_EX(_CreateObjectAtRelativePos, 0, ALL_REGISTERS) { ObjectLoader(gb).create_at_relative_pos(REG_BC(gb)); }
PORT_EX(DespawnAllObjects, 0, ALL_REGISTERS) { ObjectLoader(gb).despawn_all(); }
PORT_EX(LoadHiddenFigureData, 0, ALL_REGISTERS) { ObjectLoader(gb).load_hidden_figure_data(); }

// ends in jp LoadObjPals (or LoadHiddenFigureData's ret)
// LoadRoom farcalls it and reads no registers after
PORT_EX(LoadObjectGroupData, 0, ALL_REGISTERS) { ObjectLoader(gb).load_group(); }
