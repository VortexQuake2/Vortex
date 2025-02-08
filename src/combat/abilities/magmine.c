#include "g_local.h"

#define MAGMINE_FRAMES_START	1
#define MAGMINE_FRAMES_END		5

void magmine_throwsparks(edict_t *self) {
    int i;
    vec3_t start, up;

    AngleVectors(self->s.angles, NULL, NULL, up);
    VectorCopy(self->s.origin, start);
    start[2] = self->absmax[2];
    //start[2] += 8;

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

void drone_pain(edict_t* self, edict_t* other, float kick, int damage);
void magmine_attack(edict_t* self, edict_t* enemy, int pull) {
    //int pull;
    vec3_t start, end, dir;

    // magmine will not pull a target that is below it
    // this prevents players from placing magmines to pull a target off their feet
    if (enemy->absmin[2] + 1 < self->absmin[2])
        return;

    //gi.dprintf("%s: pull %d enemy: %s\n", __func__, pull, enemy->classname);

    G_EntMidPoint(enemy, end);
    G_EntMidPoint(self, start);
    VectorSubtract(end, start, dir);
    VectorNormalize(dir);

    //pull = MAGMINE_DEFAULT_PULL + MAGMINE_ADDON_PULL * self->monsterinfo.level;
    if (enemy->groundentity)
        pull *= 2;

    // pull them in!
    T_Damage(enemy, self, self, dir, end, vec3_origin, 0, pull, 0, 0);

    if (level.time > self->wait) {
        gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/tlaser.wav"), 1, ATTN_IDLE, 0);

        // force monsters to get angry and attack the magmine (despite FL_NOTARGET)
        if (enemy->pain)
            enemy->pain(enemy, self, 0, 0);

        self->wait = level.time + 2;
    }
}

qboolean magmine_findtarget(edict_t *self, float range, int pull) 
{
    edict_t *other = NULL;
    qboolean found_target = false;

    while ((other = findclosestradius(other, self->s.origin, range)) != NULL){
    //while ((other = findclosestradius_targets(other, self, self->dmg_radius)) != NULL) {
        if (other == self)
            continue;
        if (!G_ValidTarget(self, other, true, true))
            continue;
        magmine_attack(self, other, pull);
        found_target = true;
        //self->enemy = other;
        //return true;
    }
    //return false;
    return found_target;
}

void magmine_use_energy(edict_t* self, int energy_use)
{
    // light_level is our current ammo level
    if (self->light_level > energy_use)
    {
        self->light_level -= energy_use;
        return;
    }

    //self->nextthink = level.time + FRAMETIME;
    if (level.time > self->msg_time)
    {
        self->monsterinfo.selected_time = level.time + 1.0; // blink
        self->msg_time = level.time + 10.0;
    }

    if (self->light_level > 0)
    {
        safe_cprintf(self->creator, PRINT_HIGH, "Your mag mine's energy cells are depleted. Touch it to recharge.\n");
        self->light_level = 0;
    }
}

void magmine_remove(edict_t* self, qboolean print)
{
    if (self->deadflag == DEAD_DEAD)
        return;

    // prepare for removal
    self->deadflag = DEAD_DEAD;
    self->takedamage = DAMAGE_NO;
    self->think = BecomeExplosion1;
    self->nextthink = level.time + FRAMETIME;

    if (self->creator && self->creator->inuse)
    {
        self->creator->num_magmine--;

        if (print)
            safe_cprintf(self->creator, PRINT_HIGH, "%d/%d mag mines remaining.\n",
                self->creator->num_magmine, (int)MAGMINE_MAX_COUNT);
    }
}

void magmine_think(edict_t *self) 
{
    // check for valid position
    if (gi.pointcontents(self->s.origin) & CONTENTS_SOLID) 
    {
        gi.dprintf("WARNING: A mag mine was removed from map due to invalid position.\n");
        safe_cprintf(self->creator, PRINT_HIGH, "Your mag mine was removed.\n");
        magmine_remove(self, true);
        return;
    }

    V_HealthCache(self, (int)(0.2 * self->max_health), 1);
    int cells_used = MAGMINE_IDLE_CELLS; // idle

    if (self->light_level > 0) // magmine has enough energy to operate
    {
        int pull = MAGMINE_DEFAULT_PULL + MAGMINE_ADDON_PULL * self->monsterinfo.level;
        if (magmine_findtarget(self, self->dmg_radius, pull))
        {
            magmine_throwsparks(self);
            cells_used = MAGMINE_ACTIVE_CELLS; // active
        }

        // energy use
        if (level.time > self->delay)
        {
            self->delay = level.time + 1.0;
            magmine_use_energy(self, cells_used);
        }
        /*
        qboolean shouldCallThrowSparks = false;

        if (!self->enemy) {
            if (magmine_findtarget(self)) {
                magmine_attack(self);
                shouldCallThrowSparks = true;
            }
        }
        else if (G_ValidTarget(self, self->enemy, true, true)
            && (entdist(self, self->enemy) <= self->dmg_radius)) {
            magmine_attack(self);
            shouldCallThrowSparks = true;
        }
        else {
            self->enemy = NULL;
        }

        if (shouldCallThrowSparks) {
            magmine_throwsparks(self);
        }*/


    }
    // Animate the mag mine
    self->s.frame++;
    if (self->s.frame > MAGMINE_FRAMES_END)
        self->s.frame = MAGMINE_FRAMES_START;

    // Set shell color
    M_SetEffects(self);

    self->nextthink = level.time + FRAMETIME;
}
/*
void magmine_reload (edict_t* self, edict_t* other)
{
    int	player_ammo;
    edict_t* player;

    // entity must be alive
    if (!G_EntIsAlive(other))
        return;
    // must be a player entity
    if (other->client)
        player = other;
    else if (PM_MonsterHasPilot(other))
        player = other->owner;
    else
        return;

    // must be in need of ammo
    if (self->health < self->max_health)
    {
        player_ammo = player->client->pers.inventory[cell_index];

        //If player has more cells than needed to fill up the magmine
        if (self->health + 4 * player_ammo > self->max_health)
        {
            player_ammo -= 0.25 * (self->max_health - self->health);
            self->health = self->max_health;
        }
        else	//Player loads all their cells into the magmine
        {
            self->health += 4 * player_ammo;
            player_ammo = 0;
        }

        // has player's inventory been modified?
        if (player->client->pers.inventory[cell_index] != player_ammo)
        {
            player->client->pers.inventory[cell_index] = player_ammo; // update player's ammo
            gi.sound(self, CHAN_ITEM, gi.soundindex("misc/w_pkup.wav"), 1, ATTN_STATIC, 0);
        }
    }
}
*/
void minisentry_reload(edict_t* self, edict_t* other);
void drone_heal(edict_t* self, edict_t* other, qboolean heal_while_being_damaged);

void magmine_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
    V_Touch(self, other, plane, surf);

    // limit how often sentry can be repaired/reloaded
    if (self->sentrydelay > level.time)
        return;
    // other must be valid and a client
    if (!G_EntIsAlive(other) || !other->client)
        return;
    // only teammates can repair/reload
    if (!OnSameTeam(self, other))
        return;

    minisentry_reload(self, other);
    if (self->health < self->max_health)
    {
        drone_heal(self, other, true);
        gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/repair.wav"), 1, ATTN_NORM, 0);
    }
    //safe_cprintf(other, PRINT_HIGH, "Mag mine repaired and reloaded. %d/%da\n", self->health, self->light_level);
    self->sentrydelay = level.time + 1.0;
}

void magmine_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
    safe_cprintf(self->creator, PRINT_HIGH, "Mag mine destroyed.\n");
    magmine_remove(self, true);
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
    mine->s.angles[PITCH] = 0;
    mine->s.angles[ROLL] = 0;
    mine->think = magmine_think;
    mine->touch = magmine_touch;
    mine->nextthink = level.time + FRAMETIME;
    mine->s.modelindex = gi.modelindex("models/objects/magmine/tris.md2");
    mine->solid = SOLID_BBOX;
    mine->movetype = MOVETYPE_TOSS;
    mine->clipmask = MASK_MONSTERSOLID;
    mine->mass = 500;
    mine->classname = "magmine";
    mine->takedamage = DAMAGE_YES;
    mine->flags |= FL_NOTARGET; // AI will ignore
    mine->monsterinfo.level = ent->myskills.abilities[MAGMINE].current_level * skill_mult;
    mine->health = MAGMINE_DEFAULT_HEALTH + MAGMINE_ADDON_HEALTH * mine->monsterinfo.level;
    mine->max_health = mine->health;
    mine->dmg_radius = MAGMINE_RANGE;
    mine->mtype = M_MAGMINE;//4.5
    mine->die = magmine_die;
    mine->monsterinfo.jumpdn = MAGMINE_INITIAL_AMMO + MAGMINE_ADDON_AMMO * mine->monsterinfo.level; // max ammo
    mine->light_level = mine->monsterinfo.jumpdn; // current ammo
    mine->num_hammers = cell_index; // this magmine uses cells for ammo
    VectorSet(mine->mins, -12, -12, -4);
    VectorSet(mine->maxs, 12, 12, 0);
    layout_add_tracked_entity(&ent->client->layout, mine); // add to HUD

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

    //ent->magmine = mine; // link to owner
    ent->num_magmine++;
    safe_cprintf(ent, PRINT_HIGH, "Mag mine built (%d/%d).\n", ent->num_magmine, (int)MAGMINE_MAX_COUNT);

    ent->client->pers.inventory[power_cube_index] -= cost;
    ent->client->ability_delay = level.time + MAGMINE_DELAY * delay_mult;
    ent->holdtime = level.time + MAGMINE_DELAY * delay_mult;
}

void RemoveMagmines(edict_t* ent)
{
    edict_t* e = NULL;

    while ((e = G_Find(e, FOFS(classname), "magmine")) != NULL)
    {
        if (e && (e->creator == ent))
            magmine_remove(e, false);
    }

    // reset mag mine counter
    ent->num_magmine = 0;
}

void Cmd_SpawnMagmine_f(edict_t *ent) {
    int talentLevel, cost = MAGMINE_COST;
    float skill_mult = 1.0, cost_mult = 1.0, delay_mult = 1.0;//Talent: Rapid Assembly & Precision Tuning
    char *opt = gi.argv(1);

    if (ent->myskills.abilities[MAGMINE].disable)
        return;

    if (Q_strcasecmp(gi.args(), "count") == 0)
    {
        safe_cprintf(ent, PRINT_HIGH, "You have %d/%d mag mines.\n",
            ent->num_magmine, (int)MAGMINE_MAX_COUNT);
        return;
    }

    if (Q_strcasecmp(gi.args(), "remove") == 0)
    {
        RemoveMagmines(ent);
        safe_cprintf(ent, PRINT_HIGH, "All mag mines removed.\n");
        return;
    }

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

    if (ent->num_magmine >= (int)MAGMINE_MAX_COUNT)
    {
        safe_cprintf(ent, PRINT_HIGH, "Can't build any more mag mines.\n");
        return;
    }

    magmine_spawn(ent, cost, skill_mult, delay_mult);
}