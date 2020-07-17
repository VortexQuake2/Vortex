
#include "g_local.h"

#define PLAGUE_DEFAULT_RADIUS    48
#define PLAGUE_ADDON_RADIUS        8
#define PLAGUE_MAX_RADIUS        128
#define PLAGUE_DURATION            999
#define PLAGUE_INITIAL_DAMAGE    0
#define PLAGUE_ADDON_DAMAGE        1
#define PLAGUE_DELAY            1.0

void PlagueCloud(edict_t *ent, edict_t *target);

void PlagueCloudSpawn(edict_t *ent) {
    float radius;
    edict_t *e = NULL;
    int levelmax;

    if (ent->myskills.abilities[PLAGUE].disable && ent->myskills.abilities[BLOOD_SUCKER].disable)
        return;

    if (!V_CanUseAbilities(ent, PLAGUE, 0, false)) {
        if (!V_CanUseAbilities(ent, BLOOD_SUCKER, 0, false)) // parasites have passive plague
            return;
    }

    if ((ent->myskills.class_num == CLASS_POLTERGEIST) && !ent->mtype && !PM_PlayerHasMonster(ent))
        return; // can't use this in human form

    if (ent->mtype == M_MYPARASITE) // you're a parasite? pick highest, plague or parasite.
        levelmax = max(ent->myskills.abilities[PLAGUE].current_level,
                       ent->myskills.abilities[BLOOD_SUCKER].current_level);
    else {
        if (!ent->myskills.abilities[PLAGUE].disable) // we have the skill, right?
            levelmax = ent->myskills.abilities[PLAGUE].current_level;
    }

    radius = PLAGUE_DEFAULT_RADIUS + PLAGUE_ADDON_RADIUS * levelmax;

    if (radius > PLAGUE_MAX_RADIUS)
        radius = PLAGUE_MAX_RADIUS;

    // find someone nearby to infect
    while ((e = findradius(e, ent->s.origin, radius)) != NULL) {
        if (!G_ValidTarget(ent, e, true))
            continue;
        //	if (HasActiveCurse(e, CURSE_PLAGUE))
        if (que_typeexists(e->curses, CURSE_PLAGUE))
            continue;
        // holy water grants temporary immunity to curses
        if (e->holywaterProtection > level.time)
            continue;

        PlagueCloud(ent, e);
    }
}

void plague_think(edict_t *self) {
    int dmg;
    float radius;
    edict_t *e = NULL;

    // plague self-terminates if:
    if (!G_EntIsAlive(self->owner) || !G_EntIsAlive(self->enemy)    //someone dies
        || (self->owner->flags & FL_WORMHOLE)                        // owner enters a wormhole
        || (self->owner->client->tball_delay > level.time)            //owner tballs away
        || (self->owner->flags & FL_CHATPROTECT)                    //3.0 owner is in chatprotect
        || ((self->owner->myskills.class_num == CLASS_POLTERGEIST) && (!self->owner->mtype) &&
            !PM_PlayerHasMonster(self->owner))  //3.0 poltergeist is in human form
        || que_findtype(self->enemy->curses, NULL, HEALING) != NULL)    //3.0 player is blessed with healing
    {
        que_removeent(self->enemy->curses, self, true);
        return;
    }

    VectorCopy(self->enemy->s.origin, self->s.origin); // follow enemy

    radius = PLAGUE_DEFAULT_RADIUS + PLAGUE_ADDON_RADIUS * self->owner->myskills.abilities[PLAGUE].current_level;

    if (radius > PLAGUE_MAX_RADIUS)
        radius = PLAGUE_MAX_RADIUS;

    // find someone nearby to infect
    while ((e = findradius(e, self->s.origin, radius)) != NULL) {
        if (e == self->enemy)
            continue;
        if (!G_ValidTarget(self, e, true))
            continue;
        // don't allow more than one curse of the same type
        if (que_typeexists(e->curses, CURSE_PLAGUE))
            continue;
        // holy water grants temporary immunity to curses
        if (e->holywaterProtection > level.time)
            continue;
        // spawn another plague cloud on this entity
        PlagueCloud(self->owner, e);
    }

    if (level.time > self->wait) {
        int maxlevel;

        if (self->owner->mtype == M_MYPARASITE)
            maxlevel = max(self->owner->myskills.abilities[PLAGUE].current_level,
                           self->owner->myskills.abilities[BLOOD_SUCKER].current_level);
        else
            maxlevel = self->owner->myskills.abilities[PLAGUE].current_level;

        dmg = (float) maxlevel / 10 * ((float) self->enemy->max_health / 20);
        if (!self->enemy->client && strcmp(self->enemy->classname, "player_tank") != 0)
            dmg *= 2; // non-clients take double damage (helps with pvm)
        if (dmg < 1)
            dmg = 1;
        if (dmg > 100)
            dmg = 100;
        T_Damage(self->enemy, self->enemy, self->owner, vec3_origin, self->enemy->s.origin, vec3_origin,
                 dmg, 0, DAMAGE_NO_ABILITIES, MOD_PLAGUE); // hurt 'em
        self->wait = level.time + PLAGUE_DELAY;
    }

    self->nextthink = level.time + FRAMETIME;

}

void PlagueCloud(edict_t *ent, edict_t *target) {
    edict_t *plague;

    plague = G_Spawn();
    plague->movetype = MOVETYPE_NOCLIP;
    plague->svflags |= SVF_NOCLIENT;
    plague->solid = SOLID_NOT;
    plague->enemy = target;
    plague->owner = ent;
//	plague->dmg = damage;
    plague->nextthink = level.time + FRAMETIME;
    plague->think = plague_think;
    plague->classname = "curse";
    plague->mtype = CURSE_PLAGUE;
    VectorCopy(ent->s.origin, plague->s.origin);

    // abort if the target has too many curses
//	if (!AddCurse(ent, target, plague, CURSE_PLAGUE, PLAGUE_DURATION))
    if (!que_addent(target->curses, plague, PLAGUE_DURATION))
        G_FreeEdict(plague);
}
