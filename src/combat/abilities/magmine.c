


#include "g_local.h"

void magmine_throwsparks(edict_t *self) {
    int i;
    vec3_t start, up;

    AngleVectors(self->s.angles, NULL, NULL, up);
    VectorCopy(self->s.origin, start);
    start[2] += 8;

    for (i = 0; i < 8; i++) {
        start[2] += 4;

        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_LASER_SPARKS);
        gi.WriteByte(1); // particle count
        gi.WritePosition(start);
        gi.WriteDir(up);
        gi.WriteByte(12); // particle color
        gi.multicast(start, MULTICAST_PVS);
    }
}

qboolean magmine_findtarget(edict_t *self) {
    edict_t *other = NULL;

    while ((other = findclosestradius_targets(other,
                                      self, self->dmg_radius)) != NULL) {
        if (!G_ValidTarget_Lite(self, other, true))
            continue;
        //if (!BrainValidTarget(self, other))
        //	continue;
        self->enemy = other;
        return true;
    }
    return false;
}

void magmine_attack(edict_t *self) {
    int pull;
    vec3_t start, end, dir;

    // magmine will not pull a target that is below it
    // this prevents players from placing magmines to pull a target off their feet
    if (self->enemy->absmin[2] + 1 < self->absmin[2])
        return;

    G_EntMidPoint(self->enemy, end);
    G_EntMidPoint(self, start);
    VectorSubtract(end, start, dir);
    VectorNormalize(dir);

    pull = MAGMINE_DEFAULT_PULL + MAGMINE_ADDON_PULL * self->monsterinfo.level;
    if (self->enemy->groundentity)
        pull *= 2;

    // pull them in!
    T_Damage(self->enemy, self, self, dir, end, vec3_origin, 0, pull, 0, 0);

    if (level.time > self->delay) {
        gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/tlaser.wav"), 1, ATTN_IDLE, 0);
        self->delay = level.time + 2;
    }
}

void magmine_think(edict_t *self) {
    // check for valid position
    if (gi.pointcontents(self->s.origin) & CONTENTS_SOLID) {
        gi.dprintf("WARNING: A mag mine was removed from map due to invalid position.\n");
        safe_cprintf(self->creator, PRINT_HIGH, "Your mag mine was removed.\n");
        self->creator->magmine = NULL;
        G_FreeEdict(self);
        return;
    }

    if (!self->enemy) {
        if (magmine_findtarget(self)) {
            magmine_attack(self);
            magmine_throwsparks(self);
        }
    } else if (G_ValidTarget(self, self->enemy, true)
               && (entdist(self, self->enemy) <= self->dmg_radius)) {
        magmine_attack(self);
        magmine_throwsparks(self);
    } else {
        self->enemy = NULL;
    }

    //magmine_seteffects(self);
    M_SetEffects(self);//4.5
    self->nextthink = level.time + FRAMETIME;
}

void magmine_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point) {
    if (self->deadflag == DEAD_DEAD)
        return;

    if (debuginfo->value)
        gi.dprintf("DEBUG: magmine_die()\n");

    safe_cprintf(self->creator, PRINT_HIGH, "Your mag mine was destroyed.\n");
    self->creator->magmine = NULL;
    //BecomeExplosion1(self);

    // prepare this entity for removal
    self->think = BecomeExplosion1;
    self->nextthink = level.time + FRAMETIME;
    self->deadflag = DEAD_DEAD;
    self->takedamage = DAMAGE_NO;
    self->solid = SOLID_NOT;
    gi.unlinkentity(self);
}

void magmine_spawn(edict_t *ent, int cost, float skill_mult, float delay_mult) {
    edict_t *mine;
    vec3_t forward, right, start, end, offset;
    trace_t tr;

    if (debuginfo->value)
        gi.dprintf("DEBUG: magmine_spawn()\n");

    mine = G_Spawn();
    mine->creator = ent;
    VectorCopy(ent->s.angles, mine->s.angles);
    mine->s.angles[PITCH] = 270;
    mine->s.angles[ROLL] = 0;
    mine->think = magmine_think;
    mine->touch = V_Touch;
    mine->nextthink = level.time + FRAMETIME;
    mine->s.modelindex = gi.modelindex("models/objects/minelite/light1/tris.md2");
    mine->solid = SOLID_BBOX;
    mine->movetype = MOVETYPE_TOSS;
    mine->clipmask = MASK_MONSTERSOLID;
    mine->mass = 500;
    mine->classname = "magmine";
    mine->takedamage = DAMAGE_YES;
    mine->monsterinfo.level = ent->myskills.abilities[MAGMINE].current_level * skill_mult;
    mine->health = MAGMINE_DEFAULT_HEALTH + MAGMINE_ADDON_HEALTH * mine->monsterinfo.level;
    mine->max_health = mine->health;
    mine->dmg_radius = MAGMINE_RANGE;
    mine->mtype = M_MAGMINE;//4.5
    mine->die = magmine_die;
    VectorSet(mine->mins, -12, -12, -4);
    VectorSet(mine->maxs, 12, 12, 0);

    // calculate starting position
    AngleVectors(ent->client->v_angle, forward, right, NULL);
    VectorSet(offset, 0, 7, ent->viewheight - 8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
    VectorMA(start, 128, forward, end);
    tr = gi.trace(start, mine->mins, mine->maxs, end, ent, MASK_SHOT);

    if (tr.fraction < 1) {
        // failed to spawn
        G_FreeEdict(mine);
        return;
    }

    VectorCopy(tr.endpos, mine->s.origin);
    ent->magmine = mine; // link to owner
    ent->client->pers.inventory[power_cube_index] -= cost;
    ent->client->ability_delay = level.time + MAGMINE_DELAY * delay_mult;
    ent->holdtime = level.time + MAGMINE_DELAY * delay_mult;
}

void Cmd_SpawnMagmine_f(edict_t *ent) {
    int talentLevel, cost = MAGMINE_COST;
    float skill_mult = 1.0, cost_mult = 1.0, delay_mult = 1.0;//Talent: Rapid Assembly & Precision Tuning
    char *opt = gi.argv(1);

    if (ent->myskills.abilities[MAGMINE].disable)
        return;

    //Talent: Rapid Assembly
    talentLevel = vrx_get_talent_level(ent, TALENT_RAPID_ASSEMBLY);
    if (talentLevel > 0)
        delay_mult -= 0.1 * talentLevel;
        //Talent: Precision Tuning
    else if ((talentLevel = vrx_get_talent_level(ent, TALENT_PRECISION_TUNING)) > 0) {
        cost_mult += PRECISION_TUNING_COST_FACTOR * talentLevel;
        delay_mult += PRECISION_TUNING_DELAY_FACTOR * talentLevel;
        skill_mult += PRECISION_TUNING_SKILL_FACTOR * talentLevel;
    }
    cost *= cost_mult;

    if (!G_CanUseAbilities(ent, ent->myskills.abilities[MAGMINE].current_level, cost))
        return;

    if (!strcmp(opt, "self")) {
        if (vrx_get_talent_level(ent, TALENT_MAGMINESELF)) {
            ent->automag = !ent->automag;
            safe_cprintf(ent, PRINT_HIGH, "Auto Magmine %s\n", ent->automag ? "enabled" : "disabled");
        } else
            safe_cprintf(ent, PRINT_HIGH, "You haven't upgraded this talent.\n");

        return;
    }

    if (ent->magmine && ent->magmine->inuse) {
        safe_cprintf(ent, PRINT_HIGH, "Removed mag mine.\n");
        BecomeExplosion1(ent->magmine);
        ent->magmine = NULL;
        return;
    }

    magmine_spawn(ent, cost, skill_mult, delay_mult);
}