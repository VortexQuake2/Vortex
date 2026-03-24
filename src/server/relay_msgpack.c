#include "relay_msgpack.h"

// Serialization helper macros
#define PACK_STR(pk, str) \
    msgpack_pack_str(pk, strlen(str)); \
    msgpack_pack_str_body(pk, str, strlen(str));

void msgpack_pack_imodifier(msgpack_packer* pk, const imodifier_t* mod) {
    msgpack_pack_array(pk, 4);
    msgpack_pack_int(pk, mod->type);
    msgpack_pack_int(pk, mod->index);
    msgpack_pack_int(pk, mod->value);
    msgpack_pack_int(pk, mod->set);
}

void msgpack_pack_item(msgpack_packer* pk, const item_t* item) {
    msgpack_pack_array(pk, 11);
    msgpack_pack_int(pk, item->itemtype);
    msgpack_pack_int(pk, item->itemLevel);
    msgpack_pack_int(pk, item->quantity);
    msgpack_pack_int(pk, item->untradeable);
    PACK_STR(pk, item->id);
    PACK_STR(pk, item->name);
    msgpack_pack_int(pk, item->numMods);
    msgpack_pack_int(pk, item->setCode);
    msgpack_pack_int(pk, item->classNum);
    
    msgpack_pack_array(pk, MAX_VRXITEMMODS);
    for (size_t i = 0; i < MAX_VRXITEMMODS; i++) {
        msgpack_pack_imodifier(pk, &item->modifiers[i]);
    }
    msgpack_pack_int(pk, (int)item->isUnique);
}

void msgpack_pack_upgrade(msgpack_packer* pk, const upgrade_t* upg) {
    msgpack_pack_array(pk, 6);
    msgpack_pack_int(pk, upg->level);
    msgpack_pack_int(pk, upg->soft_max);
    msgpack_pack_int(pk, upg->hard_max);
    msgpack_pack_int(pk, upg->modifier);
    msgpack_pack_int(pk, (int)upg->disable);
    msgpack_pack_int(pk, upg->general_skill);
}

void msgpack_pack_talent(msgpack_packer* pk, const talent_t* tal) {
    msgpack_pack_array(pk, 3);
    msgpack_pack_int(pk, tal->id);
    msgpack_pack_int(pk, tal->upgradeLevel);
    msgpack_pack_int(pk, tal->maxLevel);
}

void msgpack_pack_talentlist(msgpack_packer* pk, const talentlist_t* list) {
    msgpack_pack_array(pk, 3);
    msgpack_pack_int(pk, list->count);
    msgpack_pack_int(pk, list->talentPoints);
    msgpack_pack_array(pk, MAX_TALENTS);
    for (size_t i = 0; i < MAX_TALENTS; i++) {
        msgpack_pack_talent(pk, &list->talent[i]);
    }
}

void msgpack_pack_weaponskill(msgpack_packer* pk, const weaponskill_t* ws) {
    msgpack_pack_array(pk, 4);
    msgpack_pack_int(pk, ws->level);
    msgpack_pack_int(pk, ws->current_level);
    msgpack_pack_int(pk, ws->soft_max);
    msgpack_pack_int(pk, ws->hard_max);
}

void msgpack_pack_weapon(msgpack_packer* pk, const weapon_t* wp) {
    msgpack_pack_array(pk, 2);
    msgpack_pack_int(pk, (int)wp->disable);
    msgpack_pack_array(pk, MAX_WEAPONMODS);
    for (size_t i = 0; i < MAX_WEAPONMODS; i++) {
        msgpack_pack_weaponskill(pk, &wp->mods[i]);
    }
}

void msgpack_pack_prestigelist(msgpack_packer* pk, const prestigelist_t* pre) {
    msgpack_pack_array(pk, 7);
    msgpack_pack_uint32(pk, pre->points);
    msgpack_pack_uint32(pk, pre->total);
    msgpack_pack_array(pk, MAX_ABILITIES);
    for (int i = 0; i < MAX_ABILITIES; i++) {
        msgpack_pack_uint8(pk, pre->softmaxBump[i]);
    }
    msgpack_pack_uint32(pk, pre->creditLevel);
    msgpack_pack_uint32(pk, pre->abilityPoints);
    msgpack_pack_uint32(pk, pre->weaponPoints);
    
    // abilitybitmap_t classSkill
    msgpack_pack_array(pk, MAX_ABILITIES / 32 + 1);
    for (size_t i = 0; i < MAX_ABILITIES / 32 + 1; i++) {
        msgpack_pack_uint32(pk, pre->classSkill[i]);
    }
}

void msgpack_pack_skills(msgpack_packer* pk, const skills_t* skills, int connection_id) {
    msgpack_pack_array(pk, 17);

    // Progression
    msgpack_pack_array(pk, 15);
    {
        msgpack_pack_long(pk, skills->experience);
        msgpack_pack_long(pk, skills->next_level);
        msgpack_pack_int(pk, skills->administrator);
        msgpack_pack_int(pk, skills->level);
        msgpack_pack_int(pk, skills->speciality_points);
        msgpack_pack_int(pk, skills->weapon_points);
        msgpack_pack_int(pk, skills->respawn_weapon);
        msgpack_pack_int(pk, skills->class_num);
        msgpack_pack_int(pk, skills->boss);
        msgpack_pack_int(pk, skills->current_health);
        msgpack_pack_int(pk, skills->max_health);
        msgpack_pack_int(pk, skills->current_armor);
        msgpack_pack_int(pk, skills->max_armor);
        msgpack_pack_uint64(pk, skills->credits);
        msgpack_pack_uint64(pk, skills->weapon_respawns);
    }

    // Stats
    msgpack_pack_array(pk, 21);
    {
        msgpack_pack_int(pk, skills->max_streak);
        msgpack_pack_uint64(pk, skills->frags);
        msgpack_pack_uint64(pk, skills->fragged);
        msgpack_pack_uint64(pk, skills->shots);
        msgpack_pack_uint64(pk, skills->shots_hit);
        msgpack_pack_uint64(pk, skills->num_sprees);
        msgpack_pack_uint32(pk, skills->suicides);
        msgpack_pack_uint32(pk, skills->teleports);
        msgpack_pack_uint32(pk, skills->spree_wars);
        msgpack_pack_uint32(pk, skills->break_sprees);
        msgpack_pack_uint32(pk, skills->break_spree_wars);
        msgpack_pack_uint32(pk, skills->num_2fers);
        msgpack_pack_uint32(pk, skills->flag_pickups); // ctf
        msgpack_pack_uint32(pk, skills->flag_captures);
        msgpack_pack_uint32(pk, skills->flag_returns);
        msgpack_pack_uint32(pk, skills->flag_kills);
        msgpack_pack_uint32(pk, skills->offense_kills);
        msgpack_pack_uint32(pk, skills->defense_kills);
        msgpack_pack_uint32(pk, skills->assists);
        msgpack_pack_uint32(pk, skills->playingtime);
        msgpack_pack_uint32(pk, skills->total_playtime);
    }

    msgpack_pack_array(pk, MAX_ITEMS);
    for (int i = 0; i < MAX_ITEMS; i++) {
        msgpack_pack_int(pk, skills->inventory[i]);
    }

    PACK_STR(pk, skills->password);
    PACK_STR(pk, skills->member_since);
    PACK_STR(pk, skills->last_played);
    PACK_STR(pk, skills->player_name);
    // empty owners must be passed as nil
    if (strlen(skills->owner) > 0) {
        PACK_STR(pk, skills->owner);
    } else {
        msgpack_pack_nil(pk);
    }
    PACK_STR(pk, skills->masterpw);
    PACK_STR(pk, skills->title);

    msgpack_pack_int(pk, skills->nerfme);
    msgpack_pack_int(pk, connection_id);

    msgpack_pack_array(pk, MAX_VRXITEMS);
    for (size_t i = 0; i < MAX_VRXITEMS; i++) {
        msgpack_pack_item(pk, &skills->items[i]);
    }

    msgpack_pack_array(pk, MAX_WEAPONS);
    for (size_t i = 0; i < MAX_WEAPONS; i++) {
        msgpack_pack_weapon(pk, &skills->weapons[i]);
    }

    msgpack_pack_array(pk, MAX_ABILITIES);
    for (size_t i = 0; i < MAX_ABILITIES; i++) {
        msgpack_pack_upgrade(pk, &skills->abilities[i]);
    }

    msgpack_pack_talentlist(pk, &skills->talents);
    msgpack_pack_prestigelist(pk, &skills->prestige);
}

// Deserialization helpers
#define UNPACK_INT(obj, var) \
    if (obj->type != MSGPACK_OBJECT_POSITIVE_INTEGER && obj->type != MSGPACK_OBJECT_NEGATIVE_INTEGER) return false; \
    var = (int)obj->via.i64;

#define UNPACK_UINT(obj, var) \
    if (obj->type != MSGPACK_OBJECT_POSITIVE_INTEGER) return false; \
    var = (unsigned int)obj->via.u64;

#define UNPACK_LONG(obj, var) \
    if (obj->type != MSGPACK_OBJECT_POSITIVE_INTEGER && obj->type != MSGPACK_OBJECT_NEGATIVE_INTEGER) return false; \
    var = (long)obj->via.i64;

#define UNPACK_ULONG(obj, var) \
    if (obj->type != MSGPACK_OBJECT_POSITIVE_INTEGER) return false; \
    var = (unsigned long)obj->via.u64;

#define UNPACK_FLOAT(obj, var) \
    if (obj->type != MSGPACK_OBJECT_FLOAT32 && obj->type != MSGPACK_OBJECT_FLOAT64 && obj->type != MSGPACK_OBJECT_POSITIVE_INTEGER && obj->type != MSGPACK_OBJECT_NEGATIVE_INTEGER) return false; \
    if (obj->type == MSGPACK_OBJECT_FLOAT32 || obj->type == MSGPACK_OBJECT_FLOAT64) var = (float)obj->via.f64; \
    else var = (float)obj->via.i64;

#define UNPACK_STR(obj, var, maxlen) \
    if (obj->type != MSGPACK_OBJECT_STR) return false; \
    { size_t len = obj->via.str.size < maxlen - 1 ? obj->via.str.size : maxlen - 1; \
    memcpy(var, obj->via.str.ptr, len); \
    var[len] = '\0'; }

#define UNPACK_ARRAY(obj, expected_size) \
    if (obj->type != MSGPACK_OBJECT_ARRAY || obj->via.array.size != expected_size) return false;

qboolean msgpack_unpack_imodifier(msgpack_object* obj, imodifier_t* mod) {
    UNPACK_ARRAY(obj, 4);
    const msgpack_object* p = obj->via.array.ptr;
    UNPACK_INT((&p[0]), mod->type);
    UNPACK_INT((&p[1]), mod->index);
    UNPACK_INT((&p[2]), mod->value);
    UNPACK_INT((&p[3]), mod->set);
    return true;
}

qboolean msgpack_unpack_item(msgpack_object* obj, item_t* item) {
    UNPACK_ARRAY(obj, 11);
    const msgpack_object* p = obj->via.array.ptr;
    UNPACK_INT((&p[0]), item->itemtype);
    UNPACK_INT((&p[1]), item->itemLevel);
    UNPACK_INT((&p[2]), item->quantity);
    UNPACK_INT((&p[3]), item->untradeable);
    UNPACK_STR((&p[4]), item->id, sizeof item->id);
    UNPACK_STR((&p[5]), item->name, sizeof item->name);
    UNPACK_INT((&p[6]), item->numMods);
    UNPACK_INT((&p[7]), item->setCode);
    UNPACK_INT((&p[8]), item->classNum);
    
    UNPACK_ARRAY((&p[9]), MAX_VRXITEMMODS);
    for (int i = 0; i < MAX_VRXITEMMODS; i++) {
        if (!msgpack_unpack_imodifier(&p[9].via.array.ptr[i], &item->modifiers[i])) return false;
    }
    
    int isUnique;
    UNPACK_INT((&p[10]), isUnique);
    item->isUnique = isUnique;
    return true;
}

qboolean msgpack_unpack_upgrade(msgpack_object* obj, upgrade_t* upg) {
    UNPACK_ARRAY(obj, 6);
    const msgpack_object* p = obj->via.array.ptr;
    UNPACK_INT((&p[0]), upg->level);
    UNPACK_INT((&p[1]), upg->soft_max);
    UNPACK_INT((&p[2]), upg->hard_max);
    UNPACK_INT((&p[3]), upg->modifier);
    int disable;
    UNPACK_INT((&p[4]), disable);
    UNPACK_INT((&p[5]), upg->general_skill);
    upg->disable = disable;
    return true;
}

qboolean msgpack_unpack_talent(msgpack_object* obj, talent_t* tal) {
    UNPACK_ARRAY(obj, 3);
    const msgpack_object* p = obj->via.array.ptr;
    UNPACK_INT((&p[0]), tal->id);
    UNPACK_INT((&p[1]), tal->upgradeLevel);
    UNPACK_INT((&p[2]), tal->maxLevel);
    return true;
}

qboolean msgpack_unpack_talentlist(msgpack_object* obj, talentlist_t* list) {
    UNPACK_ARRAY(obj, 3);
    const msgpack_object* p = obj->via.array.ptr;
    UNPACK_INT((&p[0]), list->count);
    UNPACK_INT((&p[1]), list->talentPoints);
    UNPACK_ARRAY((&p[2]), MAX_TALENTS);
    for (int i = 0; i < MAX_TALENTS; i++) {
        if (!msgpack_unpack_talent(&p[2].via.array.ptr[i], &list->talent[i])) return false;
    }
    return true;
}

qboolean msgpack_unpack_weaponskill(msgpack_object* obj, weaponskill_t* ws) {
    UNPACK_ARRAY(obj, 4);
    const msgpack_object* p = obj->via.array.ptr;
    UNPACK_INT((&p[0]), ws->level);
    UNPACK_INT((&p[1]), ws->current_level);
    UNPACK_INT((&p[2]), ws->soft_max);
    UNPACK_INT((&p[3]), ws->hard_max);
    return true;
}

qboolean msgpack_unpack_weapon(msgpack_object* obj, weapon_t* wp) {
    UNPACK_ARRAY(obj, 2);
    const msgpack_object* p = obj->via.array.ptr;
    int disable;
    UNPACK_INT((&p[0]), disable);
    wp->disable = (qboolean)disable;
    UNPACK_ARRAY((&p[1]), MAX_WEAPONMODS);
    for (int i = 0; i < MAX_WEAPONMODS; i++) {
        if (!msgpack_unpack_weaponskill(&p[1].via.array.ptr[i], &wp->mods[i])) return false;
    }
    return true;
}

qboolean msgpack_unpack_prestigelist(msgpack_object* obj, prestigelist_t* pre) {
    UNPACK_ARRAY(obj, 7);
    const msgpack_object* p = obj->via.array.ptr;
    UNPACK_UINT((&p[0]), pre->points);
    UNPACK_UINT((&p[1]), pre->total);
    UNPACK_ARRAY((&p[2]), MAX_ABILITIES);
    for (int i = 0; i < MAX_ABILITIES; i++) {
        if (p[2].via.array.ptr[i].type != MSGPACK_OBJECT_POSITIVE_INTEGER) return false;
        pre->softmaxBump[i] = (uint8_t)p[2].via.array.ptr[i].via.u64;
    }
    UNPACK_UINT((&p[3]), pre->creditLevel);
    UNPACK_UINT((&p[4]), pre->abilityPoints);
    UNPACK_UINT((&p[5]), pre->weaponPoints);
    UNPACK_ARRAY((&p[6]), (MAX_ABILITIES / 32 + 1));
    for (int i = 0; i < MAX_ABILITIES / 32 + 1; i++) {
        UNPACK_UINT((&p[6].via.array.ptr[i]), pre->classSkill[i]);
    }
    return true;
}

qboolean msgpack_unpack_skills(msgpack_object* obj, skills_t* skills) {
    UNPACK_ARRAY(obj, 17);
    msgpack_object* p = obj->via.array.ptr;

    // Progression
    UNPACK_ARRAY((&p[0]), 15);
    {
        const msgpack_object* prog = p[0].via.array.ptr;
        UNPACK_LONG((&prog[0]), skills->experience);
        UNPACK_LONG((&prog[1]), skills->next_level);
        UNPACK_INT((&prog[2]), skills->administrator);
        UNPACK_INT((&prog[3]), skills->level);
        UNPACK_INT((&prog[4]), skills->speciality_points);
        UNPACK_INT((&prog[5]), skills->weapon_points);
        UNPACK_INT((&prog[6]), skills->respawn_weapon);
        UNPACK_INT((&prog[7]), skills->class_num);
        UNPACK_INT((&prog[8]), skills->boss);
        UNPACK_INT((&prog[9]), skills->current_health);
        UNPACK_INT((&prog[10]), skills->max_health);
        UNPACK_INT((&prog[11]), skills->current_armor);
        UNPACK_INT((&prog[12]), skills->max_armor);
        UNPACK_UINT((&prog[13]), skills->credits);
        UNPACK_UINT((&prog[14]), skills->weapon_respawns);
    }

    // Stats
    UNPACK_ARRAY((&p[1]), 21);
    {
        const msgpack_object* stats = p[1].via.array.ptr;
        UNPACK_INT((&stats[0]), skills->max_streak);
        UNPACK_UINT((&stats[1]), skills->frags);
        UNPACK_UINT((&stats[2]), skills->fragged);
        UNPACK_ULONG((&stats[3]), skills->shots);
        UNPACK_ULONG((&stats[4]), skills->shots_hit);
        UNPACK_UINT((&stats[5]), skills->num_sprees);
        UNPACK_UINT((&stats[6]), skills->suicides);
        UNPACK_UINT((&stats[7]), skills->teleports);
        UNPACK_UINT((&stats[8]), skills->spree_wars);
        UNPACK_UINT((&stats[9]), skills->break_sprees);
        UNPACK_UINT((&stats[10]), skills->break_spree_wars);
        UNPACK_UINT((&stats[11]), skills->num_2fers);
        UNPACK_UINT((&stats[12]), skills->flag_pickups);
        UNPACK_UINT((&stats[13]), skills->flag_captures);
        UNPACK_UINT((&stats[14]), skills->flag_returns);
        UNPACK_UINT((&stats[15]), skills->flag_kills);
        UNPACK_UINT((&stats[16]), skills->offense_kills);
        UNPACK_UINT((&stats[17]), skills->defense_kills);
        UNPACK_UINT((&stats[18]), skills->assists);
        UNPACK_UINT((&stats[19]), skills->playingtime);
        UNPACK_UINT((&stats[20]), skills->total_playtime);
    }

    UNPACK_ARRAY((&p[2]), MAX_ITEMS);
    for (int i = 0; i < MAX_ITEMS; i++) {
        UNPACK_INT((&p[2].via.array.ptr[i]), skills->inventory[i]);
    }

    UNPACK_STR((&p[3]), skills->password, sizeof skills->password);
    UNPACK_STR((&p[4]), skills->member_since, sizeof skills->member_since);
    UNPACK_STR((&p[5]), skills->last_played, sizeof skills->last_played);
    UNPACK_STR((&p[6]), skills->player_name, sizeof skills->player_name);
    if (p[7].type == MSGPACK_OBJECT_NIL) {
        memset(skills->owner, 0, sizeof skills->owner);
    } else {
        UNPACK_STR((&p[7]), skills->owner, sizeof skills->owner);
    }
    if (p[8].type == MSGPACK_OBJECT_NIL) {
        memset(skills->masterpw, 0, sizeof skills->masterpw);
    } else {
        UNPACK_STR((&p[8]), skills->masterpw, sizeof skills->masterpw);
    }
    UNPACK_STR((&p[9]), skills->title, sizeof skills->title);

    UNPACK_INT((&p[10]), skills->nerfme);

    int connection_id;
    UNPACK_INT((&p[11]), connection_id);

    UNPACK_ARRAY((&p[12]), MAX_VRXITEMS);
    for (int i = 0; i < MAX_VRXITEMS; i++) {
        if (!msgpack_unpack_item(&p[12].via.array.ptr[i], &skills->items[i])) return false;
    }

    UNPACK_ARRAY((&p[13]), MAX_WEAPONS);
    for (int i = 0; i < MAX_WEAPONS; i++) {
        if (!msgpack_unpack_weapon(&p[13].via.array.ptr[i], &skills->weapons[i])) return false;
    }

    UNPACK_ARRAY((&p[14]), MAX_ABILITIES);
    for (int i = 0; i < MAX_ABILITIES; i++) {
        if (!msgpack_unpack_upgrade(&p[14].via.array.ptr[i], &skills->abilities[i])) return false;
    }

    if (!msgpack_unpack_talentlist(&p[15], &skills->talents)) return false;
    if (!msgpack_unpack_prestigelist(&p[16], &skills->prestige)) return false;

    return true;
}
