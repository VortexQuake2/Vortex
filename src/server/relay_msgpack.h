#ifndef VORTEX_RELAY_MSGPACK_H
#define VORTEX_RELAY_MSGPACK_H

#include "g_local.h"
#include <msgpack.h>

// Serialization
void msgpack_pack_imodifier(msgpack_packer* pk, const imodifier_t* mod);
void msgpack_pack_item(msgpack_packer* pk, const item_t* item);
void msgpack_pack_upgrade(msgpack_packer* pk, const upgrade_t* upg, int index);
void msgpack_pack_talent(msgpack_packer* pk, const talent_t* tal);
void msgpack_pack_talentlist(msgpack_packer* pk, const talentlist_t* list);
void msgpack_pack_weaponskill(msgpack_packer* pk, const weaponskill_t* ws);
void msgpack_pack_weapon(msgpack_packer* pk, const weapon_t* wp);
void msgpack_pack_prestigelist(msgpack_packer* pk, const prestigelist_t* pre);
void msgpack_pack_skills(msgpack_packer* pk, const skills_t* skills, int connection_id);

// Deserialization
qboolean msgpack_unpack_imodifier(msgpack_object* obj, imodifier_t* mod);
qboolean msgpack_unpack_item(msgpack_object* obj, item_t* item);
qboolean msgpack_unpack_upgrade(msgpack_object* obj, upgrade_t* upg);
qboolean msgpack_unpack_talent(msgpack_object* obj, talent_t* tal);
qboolean msgpack_unpack_talentlist(msgpack_object* obj, talentlist_t* list);
qboolean msgpack_unpack_weaponskill(msgpack_object* obj, weaponskill_t* ws);
qboolean msgpack_unpack_weapon(msgpack_object* obj, weapon_t* wp);
qboolean msgpack_unpack_prestigelist(msgpack_object* obj, prestigelist_t* pre);
qboolean msgpack_unpack_skills(msgpack_object* obj, skills_t* skills);

#endif
