#include "../quake2/g_local.h"


void tech_think(edict_t *self) {
    // let previous holder pick-up this item again
    if (self->owner)
        self->owner = NULL;

    // remove self after 60 seconds
    if (self->count >= 60) {
        if (!vrx_find_random_spawn_point(self, false)) {
            BecomeTE(self);
            return;
        }
        self->count = 0; // reset timer
    }

    self->count++;
    self->nextthink = level.time + 1.0;
}

void tech_drop(edict_t *ent, gitem_t *item) {
    int index;
    edict_t *tech;

    index = ITEM_INDEX(item);

    // can't drop something we don't have
    if (!ent->client->pers.inventory[index])
        return;

    tech = Drop_Item(ent, item);
    tech->think = tech_think;
    tech->count = 0; // reset timer
    tech->nextthink = level.time + 1.0;

    // clear inventory
    ent->client->pers.inventory[index]--;

    ValidateSelectedItem(ent);
}

qboolean tech_spawn(int index) {
    edict_t *tech;

    if (index == resistance_index)
        tech = Spawn_Item(FindItem("Resistance"));
    else if (index == strength_index)
        tech = Spawn_Item(FindItem("Strength"));
    else if (index == regeneration_index)
        tech = Spawn_Item(FindItem("Regeneration"));
    else if (index == haste_index)
        tech = Spawn_Item(FindItem("Haste"));
    else
        return false;

    tech->think = tech_think;
    tech->nextthink = level.time + FRAMETIME;
    if (!vrx_find_random_spawn_point(tech, false)) {
        gi.dprintf("Failed to spawn %s\n", tech->item->pickup_name);
        G_FreeEdict(tech);
        return false;
    }
    gi.dprintf("Spawned %s\n", tech->item->pickup_name);
    return true;
}

void tech_checkrespawn(edict_t *ent) {
    int index;

    if (!ent || !ent->inuse || !ent->item)
        return;

    // get item index
    index = ITEM_INDEX(ent->item);

    if (!index)
        return;

    // try to respawn tech
    tech_spawn(index);
}

void tech_spawnall(void) {
    gi.dprintf("Spawning techs...\n");
    tech_spawn(resistance_index);
    tech_spawn(strength_index);
    tech_spawn(regeneration_index);
    tech_spawn(haste_index);
}

qboolean tech_pickup(edict_t *ent, edict_t *other) {
    int index;
    int maxLevel = 1.7 * AveragePlayerLevel();

    // can't pick-up more than 1 tech
    if (other->client->pers.inventory[resistance_index]
        || other->client->pers.inventory[strength_index]
        || other->client->pers.inventory[regeneration_index]
        || other->client->pers.inventory[haste_index]
        || (other->myskills.streak >= SPREE_START && (V_IsPVP() || ffa->value))) // don't allow to pick up techs
        return false;

    // vrc 2.32: allow players to get techs unless there's a player significantly lower
    // than himself instead of limiting techs to level 15.

    // can't pick-up if 1.7x higher than average player level
    if (other->myskills.level > maxLevel && !invasion->value)  // az only apply this restriction in invasion
        return false;


    index = ITEM_INDEX(ent->item);

    if (index == resistance_index)
        gi.sound(other, CHAN_ITEM, gi.soundindex("ctf/tech1.wav"), 1, ATTN_STATIC, 0);
    else if (index == strength_index)
        gi.sound(other, CHAN_ITEM, gi.soundindex("ctf/tech2.wav"), 1, ATTN_STATIC, 0);
    else if (index == haste_index)
        gi.sound(other, CHAN_ITEM, gi.soundindex("ctf/tech3.wav"), 1, ATTN_STATIC, 0);
    else if (index == regeneration_index)
        gi.sound(other, CHAN_ITEM, gi.soundindex("ctf/tech4.wav"), 1, ATTN_STATIC, 0);

    other->client->pers.inventory[ITEM_INDEX(ent->item)]++;
    return true;
}

void tech_dropall(edict_t *ent) {
    if (ent->client->pers.inventory[resistance_index])
        tech_drop(ent, FindItem("Resistance"));
    if (ent->client->pers.inventory[strength_index])
        tech_drop(ent, FindItem("Strength"));
    if (ent->client->pers.inventory[regeneration_index])
        tech_drop(ent, FindItem("Regeneration"));
    if (ent->client->pers.inventory[haste_index])
        tech_drop(ent, FindItem("Haste"));
}
