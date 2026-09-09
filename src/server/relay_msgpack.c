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

void msgpack_pack_upgrade(msgpack_packer* pk, const upgrade_t* upg, int index) {
    msgpack_pack_array(pk, 7);
    msgpack_pack_int(pk, index);
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

    int bumpAbilityCount = 0;
    for (int i = 0; i < MAX_ABILITIES; i++) {
        if (pre->softmaxBump[i] > 0) {
            bumpAbilityCount++;
        }
    }

    msgpack_pack_array(pk, bumpAbilityCount);
    for (int i = 0; i < MAX_ABILITIES; i++) {
        if (pre->softmaxBump[i] > 0) {
            msgpack_pack_array(pk, 2);
            msgpack_pack_uint16(pk, i);
            msgpack_pack_uint8(pk, pre->softmaxBump[i]);
        }
    }

    msgpack_pack_uint32(pk, pre->creditLevel);
    msgpack_pack_uint32(pk, pre->abilityPoints);
    msgpack_pack_uint32(pk, pre->weaponPoints);



    int classAbilityCount = 0;
    for (int i = 0; i < MAX_ABILITIES; i++) {
        if (vrx_prestige_has_ability(pre, i)) {
            classAbilityCount++;
        }
    }

    // abilitybitmap_t classSkill
    msgpack_pack_array(pk, classAbilityCount);
    for (size_t i = 0; i < MAX_ABILITIES; i++) {
        if (vrx_prestige_has_ability(pre, i)) {
            msgpack_pack_uint16(pk, i);
        }
    }
}

void msgpack_pack_skills(msgpack_packer* pk, const playertransfer_t* transfer, int connection_id) {
    auto sk = transfer->skills;
    auto pst = transfer->stats;

    msgpack_pack_array(pk, 17);

    // Progression
    msgpack_pack_array(pk, 15);
    {
        msgpack_pack_long(pk, sk->experience);
        msgpack_pack_long(pk, sk->next_level);
        msgpack_pack_int(pk, sk->administrator);
        msgpack_pack_int(pk, sk->level);
        msgpack_pack_int(pk, sk->speciality_points);
        msgpack_pack_int(pk, sk->weapon_points);
        msgpack_pack_int(pk, sk->respawn_weapon);
        msgpack_pack_int(pk, sk->class_num);
        msgpack_pack_int(pk, sk->boss);
        // msgpack_pack_int(pk, skills->current_health);
        // msgpack_pack_int(pk, skills->max_health);
        // msgpack_pack_int(pk, skills->current_armor);
        // msgpack_pack_int(pk, skills->max_armor);
         msgpack_pack_int(pk, 0);
         msgpack_pack_int(pk, 0);
         msgpack_pack_int(pk, 0);
         msgpack_pack_int(pk, 0);
        msgpack_pack_uint64(pk, sk->credits);
        msgpack_pack_uint64(pk, sk->weapon_respawns);
    }

    // Stats
    msgpack_pack_array(pk, 21);
    {
        msgpack_pack_int(pk, pst->max_streak);
        msgpack_pack_uint64(pk, pst->frags);
        msgpack_pack_uint64(pk, pst->fragged);
        msgpack_pack_uint64(pk, pst->shots);
        msgpack_pack_uint64(pk, pst->shots_hit);
        msgpack_pack_uint64(pk, pst->num_sprees);
        msgpack_pack_uint32(pk, pst->suicides);
        msgpack_pack_uint32(pk, pst->teleports);
        msgpack_pack_uint32(pk, pst->spree_wars);
        msgpack_pack_uint32(pk, pst->break_sprees);
        msgpack_pack_uint32(pk, pst->break_spree_wars);
        msgpack_pack_uint32(pk, pst->num_2fers);
        msgpack_pack_uint32(pk, pst->flag_pickups); // ctf
        msgpack_pack_uint32(pk, pst->flag_captures);
        msgpack_pack_uint32(pk, pst->flag_returns);
        msgpack_pack_uint32(pk, pst->flag_kills);
        msgpack_pack_uint32(pk, pst->offense_kills);
        msgpack_pack_uint32(pk, pst->defense_kills);
        msgpack_pack_uint32(pk, pst->assists);
        msgpack_pack_uint32(pk, pst->playingtime);
        msgpack_pack_uint32(pk, pst->total_playtime);
    }

    msgpack_pack_array(pk, MAX_ITEMS);
    for (int i = 0; i < MAX_ITEMS; i++) {
        msgpack_pack_int(pk, pst->inventory[i]);
    }

    PACK_STR(pk, pst->password);
    PACK_STR(pk, pst->member_since);
    PACK_STR(pk, pst->last_played);
    PACK_STR(pk, pst->player_name);
    // empty owners must be passed as nil
    if (strlen(pst->owner) > 0) {
        PACK_STR(pk, pst->owner);
    } else {
        msgpack_pack_nil(pk);
    }
    PACK_STR(pk, pst->masterpw);
    PACK_STR(pk, pst->title);

    msgpack_pack_int(pk, sk->nerfme);
    msgpack_pack_int(pk, connection_id);

    msgpack_pack_array(pk, MAX_VRXITEMS);
    for (size_t i = 0; i < MAX_VRXITEMS; i++) {
        msgpack_pack_item(pk, &pst->items[i]);
    }

    msgpack_pack_array(pk, MAX_WEAPONS);
    for (size_t i = 0; i < MAX_WEAPONS; i++) {
        msgpack_pack_weapon(pk, &pst->weapons[i]);
    }

    int numAbilities = CountAbilities(sk);
    msgpack_pack_array(pk, numAbilities);
    for (size_t i = 0; i < numAbilities; i++) {
        int index = FindAbilityIndex(i + 1, sk);
        if (index == -1)
            continue;
        msgpack_pack_upgrade(pk, &sk->abilities[i], index);
    }

    msgpack_pack_talentlist(pk, &sk->talents);
    msgpack_pack_prestigelist(pk, &sk->prestige);
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
    UNPACK_ARRAY(obj, 7);
    const msgpack_object* p = obj->via.array.ptr;
    int index;

    UNPACK_INT((&p[0]), index);

    if (index < 0 || index >= MAX_ABILITIES)
        return false;

    upgrade_t *_upg = &upg[index];

    UNPACK_INT((&p[1]), _upg->level);
    UNPACK_INT((&p[2]), _upg->soft_max);
    UNPACK_INT((&p[3]), _upg->hard_max);
    UNPACK_INT((&p[4]), _upg->modifier);
    int disable;
    UNPACK_INT((&p[5]), disable);
    UNPACK_INT((&p[6]), _upg->general_skill);
    _upg->disable = disable;
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
    if (p[2].type != MSGPACK_OBJECT_ARRAY) return false;
    for (int i = 0; i < p[2].via.array.size; i++) {
        const auto inner_ = &p[2].via.array.ptr[i];
        if (inner_->type != MSGPACK_OBJECT_ARRAY) return false;

        if (inner_->via.array.size != 2) return false;
        if (inner_->via.array.ptr[0].type != MSGPACK_OBJECT_POSITIVE_INTEGER) return false;
        if (inner_->via.array.ptr[1].type != MSGPACK_OBJECT_POSITIVE_INTEGER) return false;

        auto ab = inner_->via.array.ptr[0].via.u64;
        auto val = inner_->via.array.ptr[1].via.u64;
        pre->softmaxBump[ab] = val;
    }
    UNPACK_UINT((&p[3]), pre->creditLevel);
    UNPACK_UINT((&p[4]), pre->abilityPoints);
    UNPACK_UINT((&p[5]), pre->weaponPoints);

    if (p[6].type != MSGPACK_OBJECT_ARRAY)
        return false;
    for (int i = 0; i < p[6].via.array.size; i++) {
        uint16_t index;
        UNPACK_UINT((&p[6].via.array.ptr[i]), index);

        if (index > MAX_ABILITIES)
            return false;

        pre->classSkill[index / 32] |= 1 << (index % 32);
    }
    return true;
}

qboolean msgpack_unpack_skills(msgpack_object* obj, playertransfer_t* transfer) {
    auto skills = transfer->skills;
    auto pst = transfer->stats;
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
        // UNPACK_INT((&prog[9]), skills->current_health);
        // UNPACK_INT((&prog[10]), skills->max_health);
        // UNPACK_INT((&prog[11]), skills->current_armor);
        // UNPACK_INT((&prog[12]), skills->max_armor);
        UNPACK_UINT((&prog[13]), skills->credits);
        UNPACK_UINT((&prog[14]), skills->weapon_respawns);
    }

    // Stats
    UNPACK_ARRAY((&p[1]), 21);
    {
        const msgpack_object* stats = p[1].via.array.ptr;
        UNPACK_INT((&stats[0]), pst->max_streak);
        UNPACK_UINT((&stats[1]), pst->frags);
        UNPACK_UINT((&stats[2]), pst->fragged);
        UNPACK_ULONG((&stats[3]), pst->shots);
        UNPACK_ULONG((&stats[4]), pst->shots_hit);
        UNPACK_UINT((&stats[5]), pst->num_sprees);
        UNPACK_UINT((&stats[6]), pst->suicides);
        UNPACK_UINT((&stats[7]), pst->teleports);
        UNPACK_UINT((&stats[8]), pst->spree_wars);
        UNPACK_UINT((&stats[9]), pst->break_sprees);
        UNPACK_UINT((&stats[10]), pst->break_spree_wars);
        UNPACK_UINT((&stats[11]), pst->num_2fers);
        UNPACK_UINT((&stats[12]), pst->flag_pickups);
        UNPACK_UINT((&stats[13]), pst->flag_captures);
        UNPACK_UINT((&stats[14]), pst->flag_returns);
        UNPACK_UINT((&stats[15]), pst->flag_kills);
        UNPACK_UINT((&stats[16]), pst->offense_kills);
        UNPACK_UINT((&stats[17]), pst->defense_kills);
        UNPACK_UINT((&stats[18]), pst->assists);
        UNPACK_UINT((&stats[19]), pst->playingtime);
        UNPACK_UINT((&stats[20]), pst->total_playtime);
    }

    UNPACK_ARRAY((&p[2]), MAX_ITEMS);
    for (int i = 0; i < MAX_ITEMS; i++) {
        UNPACK_INT((&p[2].via.array.ptr[i]), pst->inventory[i]);
    }

    UNPACK_STR((&p[3]), pst->password, sizeof pst->password);
    UNPACK_STR((&p[4]), pst->member_since, sizeof pst->member_since);
    UNPACK_STR((&p[5]), pst->last_played, sizeof pst->last_played);
    UNPACK_STR((&p[6]), pst->player_name, sizeof pst->player_name);
    if (p[7].type == MSGPACK_OBJECT_NIL) {
        memset(pst->owner, 0, sizeof pst->owner);
    } else {
        UNPACK_STR((&p[7]), pst->owner, sizeof pst->owner);
    }
    if (p[8].type == MSGPACK_OBJECT_NIL) {
        memset(pst->masterpw, 0, sizeof pst->masterpw);
    } else {
        UNPACK_STR((&p[8]), pst->masterpw, sizeof pst->masterpw);
    }
    UNPACK_STR((&p[9]), pst->title, sizeof pst->title);

    UNPACK_INT((&p[10]), skills->nerfme);

    int connection_id;
    UNPACK_INT((&p[11]), connection_id);

    UNPACK_ARRAY((&p[12]), MAX_VRXITEMS);
    for (int i = 0; i < MAX_VRXITEMS; i++) {
        if (!msgpack_unpack_item(&p[12].via.array.ptr[i], &pst->items[i])) return false;
    }

    if (p[13].type != MSGPACK_OBJECT_ARRAY) return false;

    // don't lose data if we can help it. we have to be very intentional and
    // remove the excess weapons from the database.
    if (p[13].via.array.size > MAX_WEAPONS) return false;

    for (int i = 0; i < MAX_WEAPONS; i++) {
        // we added a weapon?
        if (i >= p[13].via.array.size) {
            memset(&pst->weapons[i], 0, sizeof(pst->weapons[i]));
            continue;
        }
        if (!msgpack_unpack_weapon(&p[13].via.array.ptr[i], &pst->weapons[i])) return false;
    }

    if (p[14].type != MSGPACK_OBJECT_ARRAY)
        return false;

    int abilCount = p[14].via.array.size;
    for (int i = 0; i < abilCount; i++) {
        if (!msgpack_unpack_upgrade(&p[14].via.array.ptr[i], skills->abilities))
            return false;
    }

    if (!msgpack_unpack_talentlist(&p[15], &skills->talents)) return false;
    if (!msgpack_unpack_prestigelist(&p[16], &skills->prestige)) return false;

    return true;
}
