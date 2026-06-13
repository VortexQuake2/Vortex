#include "g_local.h"

constexpr size_t MAX_BACKPACKS = 32;
constexpr size_t PACK_COUNT = MAX_ITEMS * MAX_BACKPACKS;
constexpr size_t PACKMEM_SIZE = sizeof(int32_t) * PACK_COUNT;
int32_t* packItems;
bool packInUse[MAX_BACKPACKS];

void vrx_backpack_init() {
    packItems = gi.TagMalloc(PACKMEM_SIZE, TAG_GAME);
    memset(packInUse, 0, sizeof packInUse);
}

static size_t backpack_index(int32_t* idx) {
    if (idx < packItems)
        return SIZE_MAX;
    auto nr = idx - packItems;

    if (nr >= PACK_COUNT)
        return SIZE_MAX;

    return nr / MAX_ITEMS;
}

static int32_t* backpack_alloc() {
   for (size_t i = 0; i < MAX_BACKPACKS; i++) {
       if (packInUse[i])
           continue;

       packInUse[i] = true;
       const auto ret = &packItems[i * MAX_ITEMS];
       memset(ret, 0, sizeof(int32_t) * MAX_ITEMS);
       return ret;
   }

   return nullptr;
}

void Backpack_Touch(edict_t *pack, edict_t *other, cplane_t *plane, csurface_t *surf) {
    int i, quantity;
    gitem_t *item = itemlist;
    //if (!G_ClientExists(other)) return;

    //GHz START
    // if this is a player-controlled monster, then the player should
    // be able to pick up the items that the monster touches
    if (PM_MonsterHasPilot(other))
        other = other->activator;
    //GHz END

    if (!other || !other->inuse || !other->client || G_IsSpectator(other))
        return;

    if (!pack->packitems) {
        G_FreeEdict(pack);
        return;
    }


    //Add_credits(other,pack->style);
    // Play standard item pickup sound...
    gi.sound(other, CHAN_ITEM, gi.soundindex("items/pkup.wav"), 1.0, ATTN_NORM, 0);
    // Step thru array of items in backpack..
    for (i = 0; i < game.num_items; i++, item++) {
        quantity = pack->packitems[ITEM_INDEX(item)];
        if (quantity < 1) continue;
        if (item == Fdi_SHOTGUN ||
            item == Fdi_HYPERBLASTER ||
            item == Fdi_SUPERSHOTGUN ||
            item == Fdi_GRENADELAUNCHER ||
            item == Fdi_CHAINGUN ||
            item == Fdi_RAILGUN ||
            item == Fdi_MACHINEGUN ||
            item == Fdi_BFG ||
            item == Fdi_ROCKETLAUNCHER ||
            item == Fdi_BLASTER)
            other->client->pers.inventory[ITEM_INDEX(item)] = 1;
        else
            other->client->pers.inventory[ITEM_INDEX(item)] += quantity;
    }
    //gi.unlinkentity(pack);  // Make pack disappear..
    packInUse[backpack_index(pack->packitems)] = false;
    Check_full(other);
    G_FreeEdict(pack);
}

qboolean IsBackpack(edict_t *ent) {
    if (!ent || !ent->inuse)
        return false;
    if (ent->style && ent->owner && ent->owner == world && ent->movetype == MOVETYPE_TOSS
        && !ent->takedamage && !ent->health && ent->deadflag == DEAD_DEAD && ent->solid == SOLID_TRIGGER)
        return true;
    return false;
}

void vrx_toss_backpack(edict_t *player, edict_t *attacker) {
    int i, quantity;
    gitem_t *item = itemlist;
    edict_t *pack;
    vec3_t forward, right, up;
    float dist;
    vec3_t v;
    const edict_t *enemy = NULL;

    if (!deathmatch->value) return;

    // az: tossing backpacks at this point, given persistent inventory, is nonsense.
    if (pvm->value)
        return;

    if (!vrx_spawn_nonessential_ent(player->s.origin))
        return;

    if (player->enemy && player->enemy != player) {
        if (player->enemy->classname[0] == 'p') {
            VectorSubtract(player->s.origin, player->enemy->s.origin, v);
            dist = VectorLength(v);
            if (dist < 500) enemy = player->enemy;
        }
    }

    auto packitems = backpack_alloc();
    if (!packitems)
        return;

    // Create backpack entity
    pack = G_Spawn();
    pack->packitems = packitems;
    // Now.. Fill up the Backpack
    // Look thru list of all possible items..
    for (i = 0; i < game.num_items; i++, item++) {
        if (!item->classname) continue;
        if (item == FindItem("Body Armor") ||
            item == FindItem("Power Cube") ||
            item == FindItem("tballs"))
            continue;
        // Does player have any of this item in their inventory?
        quantity = player->client->pers.inventory[ITEM_INDEX(item)];
        // Then.. add this item to the backpack
        pack->packitems[ITEM_INDEX(item)] = quantity;
    }
    // ------------------------------
    // Finish making the pack entity
    // ------------------------------
    pack->owner = world;
    pack->movetype = MOVETYPE_TOSS;
    pack->takedamage = DAMAGE_NO;
    pack->health = 0;
    pack->deadflag = DEAD_DEAD; // So not in game..
    pack->solid = SOLID_TRIGGER;
    pack->s.modelindex = gi.modelindex("models/items/pack/tris.md2");
    pack->classname = "item_pack";
    pack->s.effects = 0;
    VectorCopy(player->s.origin, pack->s.origin);
    pack->s.origin[2] += 32;
    VectorSet(pack->mins, -16, -16, -16); // Size of standard Q2 pack
    VectorSet(pack->maxs, 16, 16, 16);
    AngleVectors(player->s.angles, forward, right, up);
    VectorScale(forward, 300, pack->velocity);
    VectorMA(pack->velocity, 200 + crandom() * 10.0, up, pack->velocity);
    VectorMA(pack->velocity, crandom() * 10.0, right, pack->velocity);
    VectorClear(pack->avelocity); // No angular rotation
    VectorClear(pack->s.angles); // No tilt..
    pack->touch = Backpack_Touch;
    pack->nextthink = level.time + 30.0;
    pack->think = G_FreeEdict;
    pack->style = player->myskills.level;
    gi.linkentity(pack);
}
