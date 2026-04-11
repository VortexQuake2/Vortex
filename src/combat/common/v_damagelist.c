#include "g_local.h"

qboolean validDmgPlayer(edict_t *player) {
    return (player && player->inuse && player->client && !G_IsSpectator(player));
}

void vrx_clean_damage_list(edict_t *self, qboolean clear_all) {
    int i;

    for (i = 0; i < MAX_CLIENTS; i++) {
        // is this a valid player?
        if (!clear_all && validDmgPlayer(self->monsterinfo.dmglist[i].player))
            continue;

        // clear this entry
        self->monsterinfo.dmglist[i].player = NULL;
        self->monsterinfo.dmglist[i].damage = 0;
    }
}

dmglist_t *findDmgSlot(edict_t *self, edict_t *other) {
    int i;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (self->monsterinfo.dmglist[i].player == other)
            return &self->monsterinfo.dmglist[i];
    }
    return NULL;
}

dmglist_t *findEmptyDmgSlot(edict_t *self) {
    int i;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (!validDmgPlayer(self->monsterinfo.dmglist[i].player))
            return &self->monsterinfo.dmglist[i];
    }
    return NULL;
}

float GetTotalBossDamage(edict_t *self) {
    int i;
    float dmg = 0;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (validDmgPlayer(self->monsterinfo.dmglist[i].player))
            dmg += self->monsterinfo.dmglist[i].damage;
    }

    if (dmg < 1)
        dmg = 1;

    return dmg;
}

float GetPlayerBossDamage(edict_t *player, edict_t *boss) {
    int i;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (boss->monsterinfo.dmglist[i].player == player)
            return boss->monsterinfo.dmglist[i].damage;
    }
    return 0;
}

dmglist_t *findHighestDmgPlayer(edict_t *self) {
    int i;
    dmglist_t *slot = NULL;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (!validDmgPlayer(self->monsterinfo.dmglist[i].player))
            continue;

        if (!self->monsterinfo.dmglist[i].damage)
            continue;

        if (!slot)
            slot = &self->monsterinfo.dmglist[i];
        else if (self->monsterinfo.dmglist[i].damage > slot->damage)
            slot = &self->monsterinfo.dmglist[i];
    }

    return slot;
}

void printDmgList(edict_t *self) {
    int i;
    float percent;
    dmglist_t *slot;

    for (i = 0; i < MAX_CLIENTS; i++) {
        slot = &self->monsterinfo.dmglist[i];
        if (self->monsterinfo.dmglist[i].player) {
            percent = 100 * (slot->damage / GetTotalBossDamage(self));

            if (self->client)
                gi.dprintf("(%s) slot %d: %s, %.0f damage (%.1f%c)\n",
                           self->client->pers.netname, i, slot->player->client->pers.netname, slot->damage, percent,
                           '%');
            else if (self->mtype)
                gi.dprintf("(%s) slot %d: %s, %.0f damage (%.1f%c)\n",
                           V_GetMonsterName(self), i, slot->player->client->pers.netname, slot->damage, percent, '%');
            else
                gi.dprintf("(%s) slot %d: %s, %.0f damage (%.1f%c)\n",
                           self->classname, i, slot->player->client->pers.netname, slot->damage, percent, '%');
        }
    }
}

void AddDmgList(edict_t *self, edict_t *other, int damage) {
    edict_t *cl_ent;
    dmglist_t *slot;

    if (damage < 1)
        return;

    // sanity checks
    if (!self || !self->inuse || self->deadflag == DEAD_DEAD)
        return;
    if (!other || !other->inuse || other->deadflag == DEAD_DEAD)
        return;

    // don't count self-inflicted damage
    if (other == self)
        return;

    // we're only adding non-spectator clients to this list
    cl_ent = G_GetClient(other);
    if (!validDmgPlayer(cl_ent))
        return;

    // prune list of disconnected clients
    vrx_clean_damage_list(self, false);

    // already in the queue?
    if ((slot = findDmgSlot(self, cl_ent)) != NULL) {
        slot->damage += damage;
    }
    // add attacker to the queue
    else if ((slot = findEmptyDmgSlot(self)) != NULL) {
        slot->player = cl_ent;
        slot->damage += damage;
    }

    //gi.dprintf("adding %d damage\n", damage);
    //printDmgList(self);
}
