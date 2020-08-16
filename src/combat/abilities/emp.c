#include "g_local.h"
#include "characters/class_limits.h"

// from g_weapon.c
void check_dodge (edict_t *self, vec3_t start, vec3_t dir, int speed, int radius);

#define EMP_BULLET_FACTOR				0.5
#define EMP_CELL_FACTOR					0.5
#define EMP_SHELL_FACTOR				1
#define EMP_ROCKET_FACTOR				2
#define EMP_GRENADE_FACTOR				2
#define EMP_SLUG_FACTOR					2
#define EMP_MIN_RADIUS					100
#define EMP_MAX_RADIUS					300
#define EMP_MAX_DAMAGE					1000
#define EMP_INITIAL_AMMO				0
#define EMP_ADDON_AMMO					0.05	// 5% ammo per level is detonated
#define EMP_COST						25
#define EMP_DELAY						0.5
#define EMP_INITIAL_TIME				0
#define EMP_ADDON_TIME					0.5
#define EMP_INITIAL_RADIUS				150
#define EMP_ADDON_RADIUS				0
#define EMP_AMMOBOX_INITIAL_DAMAGE		50
#define EMP_AMMOBOX_ADDON_DAMAGE		5

void EmpEffects (edict_t *ent)
{
    vec3_t	start, dir;
    int		i=GetRandom(0, 2);

    start[i] = ent->absmax[i];
    i++;
    if (i>2)
        i=0;
    start[i] = GetRandom((int) ent->absmin[i], (int) ent->absmax[i]);
    i++;
    if (i>2)
        i=0;
    start[i] = GetRandom((int) ent->absmin[i], (int) ent->absmax[i]);

    VectorSubtract(ent->s.origin, start, dir);
    VectorNormalize(dir);

    // throw some sparks around the entities bounding box
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_LASER_SPARKS);
    gi.WriteByte(GetRandom(5, 15)); // number of sparks
    gi.WritePosition(start);
    gi.WriteDir(dir);
    gi.WriteByte(210); // 242 = red, 210 = green, 2 = black
    gi.multicast(start, MULTICAST_PVS);

    // make powered armor flash on and off


    // make powered armor flash on and off
    if (!ctf->value && !domination->value && ent->monsterinfo.power_armor_power)
    {
        if (ent->monsterinfo.power_armor_type == POWER_ARMOR_SHIELD)
        {
            ent->s.effects ^= EF_COLOR_SHELL;
            ent->s.renderfx ^= RF_SHELL_GREEN;
        }
        else
        {
            ent->s.effects ^= EF_POWERSCREEN;
        }
    }

}

void ExplodeAmmo (edict_t *ent, edict_t *other)
{
    int		dmg=0, qty, use;
    float	rad, initial=EMP_INITIAL_AMMO, addon=EMP_ADDON_AMMO;

    // 200-1200 max bullets
    qty = other->client->pers.inventory[bullet_index];
    use = (initial + addon * ent->monsterinfo.level) * MAX_BULLETS(other);
    if (use > qty)
        use = qty;
    dmg += EMP_BULLET_FACTOR*use;
    other->client->pers.inventory[bullet_index] -= use;
    //gi.dprintf("used %d of %d bullets\n", use, qty);

    // 200-1200 max cells
    qty = other->client->pers.inventory[cell_index];
    use = (initial + addon * ent->monsterinfo.level) * MAX_CELLS(other);
    if (use > qty)
        use = qty;
    dmg += EMP_CELL_FACTOR*use;
    other->client->pers.inventory[cell_index] -= use;
    //gi.dprintf("used %d of %d cells\n", use, qty);

    // 100-600 max shells
    qty = other->client->pers.inventory[shell_index];
    use = (initial + addon * ent->monsterinfo.level) * MAX_SHELLS(other);
    if (use > qty)
        use = qty;
    dmg += EMP_SHELL_FACTOR*use;
    other->client->pers.inventory[shell_index] -= use;
    //gi.dprintf("used %d of %d shells\n", use, qty);

    // 50-300 max rockets
    qty = other->client->pers.inventory[rocket_index];
    use = (initial + addon * ent->monsterinfo.level) * MAX_ROCKETS(other);
    if (use > qty)
        use = qty;
    dmg += EMP_ROCKET_FACTOR*use;
    other->client->pers.inventory[rocket_index] -= use;
    //gi.dprintf("used %d of %d rockets\n", use, qty);

    // 50-300 max grenades
    qty = other->client->pers.inventory[grenade_index];
    use = (initial + addon * ent->monsterinfo.level) * MAX_GRENADES(other);
    if (use > qty)
        use = qty;
    dmg += EMP_GRENADE_FACTOR*use;
    other->client->pers.inventory[grenade_index] -= use;
    //gi.dprintf("used %d of %d grenades\n", use, qty);

    // 50-300 max slugs
    qty = other->client->pers.inventory[slug_index];
    use = (initial + addon * ent->monsterinfo.level) * MAX_SLUGS(other);
    if (use > qty)
        use = qty;
    dmg += EMP_SLUG_FACTOR*use;
    other->client->pers.inventory[slug_index] -= use;
    //gi.dprintf("used %d of %d slugs\n", use, qty);

    //gi.dprintf("damage = %d\n", dmg);

    if (dmg > 0)
    {
        if (dmg > EMP_MAX_DAMAGE)
            dmg = EMP_MAX_DAMAGE;

        // hurt them
        T_Damage(other, ent, ent->owner, vec3_origin, other->s.origin, vec3_origin, dmg, 0, 0, MOD_EMP);

        // hurt others from exploding ammo
        rad = 0.5*dmg;
        if (rad < EMP_MIN_RADIUS)
            rad = EMP_MIN_RADIUS;
        if (rad > EMP_MAX_RADIUS)
            rad = EMP_MAX_RADIUS;
        T_RadiusDamage(ent, ent->owner, dmg, other, rad, MOD_EMP);

        // teleport effect to indicate exploding ammo
        gi.WriteByte (svc_temp_entity);
        gi.WriteByte (TE_TELEPORT_EFFECT);
        gi.WritePosition (other->s.origin);
        gi.multicast (other->s.origin, MULTICAST_PVS);
    }
}

qboolean EMP_ValidTarget (edict_t *self, edict_t *other)
{
    if (!other || !other->inuse)
        return false;

    // excluded entities (they have no electronics to fry)
    if (other->mtype == M_SKULL || other->mtype == M_MUTANT || other->mtype == M_SUPPLYSTATION
        || other->mtype == CTF_PLAYERSPAWN || other->mtype == TOTEM_WATER || other->mtype == TOTEM_AIR
        || other->mtype == TOTEM_DARKNESS || other->mtype == TOTEM_NATURE || other->mtype == TOTEM_FIRE
        || other->mtype == TOTEM_NATURE)
        return false;

    // can target owner
    if (self->owner && self->owner->inuse && (other == self->owner) && !(other->flags & FL_GODMODE)
        && !(other->flags & FL_CHATPROTECT) && visible(self, other))
        return true;

    return G_ValidTarget(self, other, true);
}


qboolean IsAmmoBox (edict_t *ent)
{
    return (ent && ent->inuse && (!strcmp(ent->classname, "ammo_rockets") || !strcmp(ent->classname, "ammo_grenades")
                                  || !strcmp(ent->classname, "ammo_cells") || !strcmp(ent->classname, "ammo_slugs")
                                  || !strcmp(ent->classname, "ammo_bullets") ||!strcmp(ent->classname, "ammo_shells")));
}

void EMP_Explode (edict_t *self)
{
    int		damage;
    float		radius;
    float		time = EMP_INITIAL_TIME + (EMP_ADDON_TIME * self->monsterinfo.level);
    edict_t		*e=NULL;
    qboolean	ammoBox;

    // remove grenade if owner dies or becomes invalid
    if (!G_EntIsAlive(self->owner))
    {
        G_FreeEdict(self);
        return;
    }

    //FIXME: blow up player backpacks too!
    while ((e = findradius(e, self->s.origin, self->dmg_radius)) != NULL)
    {
        ammoBox = IsAmmoBox(e);

        if (!EMP_ValidTarget(self, e) && !ammoBox)
            continue;

        // blow-up player ammo
        if (e->client)
            ExplodeAmmo(self, e);
            // blow up ammo boxes
        else if (ammoBox)
        {
            damage = EMP_AMMOBOX_INITIAL_DAMAGE+EMP_AMMOBOX_ADDON_DAMAGE*self->monsterinfo.level;
            radius = 0.5*damage;

            if (radius < EMP_MIN_RADIUS)
                radius = EMP_MIN_RADIUS;
            if (radius > EMP_MAX_RADIUS)
                radius = EMP_MAX_RADIUS;

            T_RadiusDamage(self, self->owner, damage, NULL, radius, MOD_EMP);

            // explosion effect
            gi.WriteByte (svc_temp_entity);
            gi.WriteByte (TE_EXPLOSION1);
            gi.WritePosition (e->s.origin);
            gi.multicast (e->s.origin, MULTICAST_PVS);

            // don't respawn if it's a dropped item
            if (!(e->spawnflags & (DROPPED_ITEM|DROPPED_PLAYER_ITEM)))
                SetRespawn(e, 30);
            else
                G_FreeEdict(e);
        }
            // force monsters into idle mode
        else if (!strcmp(e->classname, "drone"))
        {
            // bosses can't be stunned easily
            if (e->monsterinfo.control_cost >= M_COMMANDER_CONTROL_COST || e->monsterinfo.bonus_flags & BF_UNIQUE_FIRE
                || e->monsterinfo.bonus_flags & BF_UNIQUE_LIGHTNING)
                time *= 0.2;

            e->empeffect_time = level.time + time;
            e->empeffect_owner = self->owner;
            e->monsterinfo.pausetime = level.time + time;
            e->monsterinfo.stand(e);
        }
            // stun anything else
        else
        {
            e->empeffect_time = level.time + time;
            e->holdtime = level.time + time;
        }
    }

    gi.WriteByte (svc_temp_entity);
    gi.WriteByte (TE_TELEPORT_EFFECT);
    gi.WritePosition (self->s.origin);
    gi.multicast (self->s.origin, MULTICAST_PVS);

    G_FreeEdict(self);
}

void EMP_Touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
    // disappear when touching a sky brush
    if (surf && (surf->flags & SURF_SKY))
    {
        G_FreeEdict (ent);
        return;
    }

    // bounce off things that can't be hurt
    if (!other->takedamage)
    {
        if (random() > 0.5)
            gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb1a.wav"), 1, ATTN_NORM, 0);
        else
            gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb2a.wav"), 1, ATTN_NORM, 0);
    }
        // detonate if we touch a live entity that isn't on our team
    else if (G_EntIsAlive(other) && !OnSameTeam(ent, other))
        EMP_Explode(ent);

    // FIXME: change pitch when grenade comes to a stop so the model doesn't poke through the floor
    // this would require changing SV_Physics_Toss() to call touch() function after stopping the entity
}

void fire_emp_grenade (edict_t *self, vec3_t start, vec3_t aimdir, int slevel, float radius, int speed, float timer)
{
    edict_t	*grenade;
    vec3_t	dir;
    vec3_t	forward, right, up;

    // calling entity made a sound, used to alert monsters
    self->lastsound = level.framenum;

    // get aiming angles
    vectoangles(aimdir, dir);
    // get directional vectors
    AngleVectors(dir, forward, right, up);

    // spawn grenade entity
    grenade = G_Spawn();
    VectorCopy (start, grenade->s.origin);
    grenade->movetype = MOVETYPE_BOUNCE;
    grenade->clipmask = MASK_SHOT;
    grenade->solid = SOLID_TRIGGER;
    grenade->s.effects |= EF_GRENADE;
    grenade->s.modelindex = gi.modelindex ("models/objects/ggrenade/tris.md2");
    grenade->owner = self;
    grenade->touch = EMP_Touch;
    grenade->think = EMP_Explode;
    grenade->monsterinfo.level = slevel;
    grenade->dmg_radius = radius;
    grenade->classname = "emp grenade";
    gi.linkentity (grenade);
    grenade->nextthink = level.time + timer;

    // adjust velocity
    VectorScale (aimdir, speed, grenade->velocity);
    VectorMA (grenade->velocity, 200 + crandom() * 10.0, up, grenade->velocity);
    VectorMA (grenade->velocity, crandom() * 10.0, right, grenade->velocity);
    VectorSet (grenade->avelocity, 300, 300, 300);
}

void Cmd_TossEMP (edict_t *ent)
{
    int		cost, slevel;
    float	radius;
    vec3_t	forward, right, start, offset;

    //Talent: Bombardier - reduces grenade cost
    cost = EMP_COST - vrx_get_talent_level(ent, TALENT_BOMBARDIER);

    if (!V_CanUseAbilities(ent, EMP, cost, true))
        return;

    slevel = ent->myskills.abilities[EMP].current_level;
    radius = EMP_INITIAL_RADIUS + EMP_ADDON_RADIUS * slevel;

    // get starting position and forward vector
    AngleVectors (ent->client->v_angle, forward, right, NULL);
    VectorSet(offset, 0, 8,  ent->viewheight-8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    fire_emp_grenade(ent, start, forward, slevel, radius, 600, 2.0);

    ent->client->ability_delay = level.time + EMP_DELAY;
    ent->client->pers.inventory[power_cube_index] -= cost;
}