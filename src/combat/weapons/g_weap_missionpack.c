#include "g_local.h"

// from g_weapon.c
void check_dodge (edict_t *self, vec3_t start, vec3_t dir, int speed, int radius);
void Grenade_Explode (edict_t *ent);

#define PROX_TIME_TO_LIVE 45.0f
#define PROX_TIME_DELAY 0.5f
#define PROX_BOUND_SIZE 96.0f
#define PROX_HEALTH 20

#define TESLA_TIME_TO_LIVE 30.0f
#define TESLA_ACTIVATE_TIME 3.0f
#define TESLA_KNOCKBACK 8
#define TESLA_DAMAGE_RADIUS_MIN 96.0f
#define TESLA_EXPLOSION_DAMAGE_MULT 50
#define TESLA_EXPLOSION_RADIUS 200.0f

static void Prox_Explode(edict_t *ent);
static void prox_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
static void Prox_Field_Touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);
static void prox_seek(edict_t *ent);
static void prox_open(edict_t *ent);
static void prox_land(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

static void tesla_remove(edict_t *self);
static void tesla_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
static void tesla_blow(edict_t *self);
static void tesla_zap(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
static void tesla_think_active(edict_t *self);
static void tesla_activate(edict_t *self);
static void tesla_think(edict_t *ent);
static void tesla_lava(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

// RAFAEL
/*
=================
	fire_ionripper
=================
*/

void ionripper_sparks (edict_t *self)
{
    gi.WriteByte (svc_temp_entity);
    gi.WriteByte (TE_WELDING_SPARKS);
    gi.WriteByte (0);
    gi.WritePosition (self->s.origin);
    gi.WriteDir (vec3_origin);
    gi.WriteByte (0xe4 + (randomMT()&3));
    gi.multicast (self->s.origin, MULTICAST_PVS);

    G_FreeEdict (self);
}

// RAFAEL
void ionripper_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
    if (other == self->owner)
        return;

    if (surf && (surf->flags & SURF_SKY))
    {
        G_FreeEdict (self);
        return;
    }

    if (self->owner->client)
        PlayerNoise (self->owner, self->s.origin, PNOISE_IMPACT);

    if (other->takedamage)
    {
        T_Damage (other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 1, DAMAGE_ENERGY, MOD_RIPPER);

    }
    else
    {
        return;
    }

    G_FreeEdict (self);
}


// RAFAEL
void fire_ionripper (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect)
{
    edict_t *ion;
    trace_t tr;

    VectorNormalize (dir);

    ion = G_Spawn ();
    VectorCopy (start, ion->s.origin);
    VectorCopy (start, ion->s.old_origin);
    vectoangles (dir, ion->s.angles);
    VectorScale (dir, speed, ion->velocity);

    ion->movetype = MOVETYPE_WALLBOUNCE;
    ion->clipmask = MASK_SHOT;
    ion->solid = SOLID_BBOX;
    ion->s.effects |= effect;

    ion->s.renderfx |= RF_FULLBRIGHT;

    VectorClear (ion->mins);
    VectorClear (ion->maxs);
    ion->s.modelindex = gi.modelindex ("models/objects/boomrang/tris.md2");
    ion->s.sound = gi.soundindex ("misc/lasfly.wav");
    ion->owner = self;
    ion->touch = ionripper_touch;
    ion->nextthink = level.time + 3;
    ion->think = ionripper_sparks;
    ion->dmg = damage;
    ion->dmg_radius = 100;
    gi.linkentity (ion);

    if (self->client)
        check_dodge (self, ion->s.origin, dir, speed, 100);

    tr = gi.trace (self->s.origin, NULL, NULL, ion->s.origin, ion, MASK_SHOT);
    if (tr.fraction < 1.0)
    {
        VectorMA (ion->s.origin, -10, dir, ion->s.origin);
        ion->touch (ion, tr.ent, NULL, NULL);
    }

}

/*
========================
fire_flechette
========================
*/
static void flechette_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
    vec3_t dir;

    if (other == self->owner)
        return;

    if (surf && (surf->flags & SURF_SKY))
    {
        G_FreeEdict(self);
        return;
    }

    if (self->owner->client)
        PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

    if (other->takedamage)
    {
        T_Damage(other, self, self->owner, self->velocity, self->s.origin,
                 plane ? plane->normal : vec3_origin, self->dmg, self->dmg_radius,
                 DAMAGE_BULLET, MOD_MACHINEGUN);
    }
    else
    {
        if (!plane)
            VectorClear(dir);
        else
            VectorScale(plane->normal, 256, dir);

        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_FLECHETTE);
        gi.WritePosition(self->s.origin);
        gi.WriteDir(dir);
        gi.multicast(self->s.origin, MULTICAST_PVS);
    }

    G_FreeEdict(self);
}

void fire_flechette (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int kick)
{
    edict_t *flechette;

    VectorNormalize(dir);

    flechette = G_Spawn();
    VectorCopy(start, flechette->s.origin);
    VectorCopy(start, flechette->s.old_origin);
    vectoangles(dir, flechette->s.angles);
    VectorScale(dir, speed, flechette->velocity);
    flechette->movetype = MOVETYPE_FLYMISSILE;
    flechette->clipmask = MASK_SHOT;
    flechette->solid = SOLID_BBOX;
    flechette->s.renderfx = RF_FULLBRIGHT;
    VectorClear(flechette->mins);
    VectorClear(flechette->maxs);
    flechette->s.modelindex = gi.modelindex("models/proj/flechette/tris.md2");
    flechette->owner = self;
    flechette->touch = flechette_touch;
    flechette->nextthink = level.time + 8000 / speed;
    flechette->think = G_FreeEdict;
    flechette->dmg = damage;
    flechette->dmg_radius = kick;
    gi.linkentity(flechette);

    if (self->client)
        check_dodge(self, flechette->s.origin, dir, speed, 0);
}

/*
=================
fire_heat

Rogue Plasma Beam implementation (continuous hitscan beam).
=================
*/
static void fire_beams(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_beam, int mod)
{
    trace_t tr;
    vec3_t dir;
    vec3_t forward;
    vec3_t end;
    vec3_t water_start, endpoint;
    qboolean water = false, underwater = false;
    int content_mask = MASK_SHOT | MASK_WATER;
    vec3_t beam_endpt;

    vectoangles(aimdir, dir);
    AngleVectors(dir, forward, NULL, NULL);
    VectorMA(start, 8192, forward, end);

    if (gi.pointcontents(start) & MASK_WATER)
    {
        underwater = true;
        VectorCopy(start, water_start);
        content_mask &= ~MASK_WATER;
    }

    tr = gi.trace(start, NULL, NULL, end, self, content_mask);

    if (tr.contents & MASK_WATER)
    {
        water = true;
        VectorCopy(tr.endpos, water_start);

        if (!VectorCompare(start, tr.endpos))
        {
            gi.WriteByte(svc_temp_entity);
            gi.WriteByte(TE_HEATBEAM_SPARKS);
            gi.WritePosition(water_start);
            gi.WriteDir(tr.plane.normal);
            gi.multicast(tr.endpos, MULTICAST_PVS);
        }

        // Re-trace ignoring water.
        tr = gi.trace(water_start, NULL, NULL, end, self, MASK_SHOT);
    }
    VectorCopy(tr.endpos, endpoint);

    // Halve damage against submerged target.
    if (water)
        damage /= 2;

    if (!((tr.surface) && (tr.surface->flags & SURF_SKY)))
    {
        if (tr.fraction < 1.0)
        {
            if (tr.ent->takedamage)
            {
                T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, DAMAGE_ENERGY, mod);
            }
            else if ((!water) && tr.surface && tr.surface->name && strncmp(tr.surface->name, "sky", 3))
            {
                gi.WriteByte(svc_temp_entity);
                gi.WriteByte(TE_HEATBEAM_STEAM);
                gi.WritePosition(tr.endpos);
                gi.WriteDir(tr.plane.normal);
                gi.multicast(tr.endpos, MULTICAST_PVS);

                if (self->client)
                    PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
            }
        }
    }

    if (water || underwater)
    {
        vec3_t pos;

        VectorSubtract(tr.endpos, water_start, dir);
        VectorNormalize(dir);
        VectorMA(tr.endpos, -2, dir, pos);
        if (gi.pointcontents(pos) & MASK_WATER)
            VectorCopy(pos, tr.endpos);
        else
            tr = gi.trace(pos, NULL, NULL, water_start, tr.ent, MASK_WATER);

        VectorAdd(water_start, tr.endpos, pos);
        VectorScale(pos, 0.5, pos);

        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_BUBBLETRAIL2);
        gi.WritePosition(water_start);
        gi.WritePosition(tr.endpos);
        gi.multicast(pos, MULTICAST_PVS);
    }

    if (!underwater && !water)
        VectorCopy(tr.endpos, beam_endpt);
    else
        VectorCopy(endpoint, beam_endpt);

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(te_beam);
    gi.WriteShort(self - g_edicts);
    gi.WritePosition(start);
    gi.WritePosition(beam_endpt);
    gi.multicast(self->s.origin, MULTICAST_ALL);
}

void fire_heat(edict_t *self, vec3_t start, vec3_t aimdir, vec3_t offset, int damage, int kick, qboolean monster)
{
    (void)offset;

    if (monster)
        fire_beams(self, start, aimdir, damage, kick, TE_MONSTER_HEATBEAM, MOD_PHALANX);
    else
        fire_beams(self, start, aimdir, damage, kick, TE_HEATBEAM, MOD_PHALANX);
}


// RAFAEL
/*
=================
fire_heat
=================
*/
/*
void heat_think (edict_t *self)
{
	edict_t		*target = NULL;
	edict_t		*aquire = NULL;
	vec3_t		vec;
	vec3_t		oldang;
	int			len;
	int			oldlen = 0;

	VectorClear (vec);

	// aquire new target
	while (( target = findradius (target, self->s.origin, 1024)) != NULL)
	{

		if (self->owner == target)
			continue;
		if (!target->svflags & SVF_MONSTER)
			continue;
		if (!target->client)
			continue;
		if (target->health <= 0)
			continue;
		if (!visible (self, target))
			continue;

		// if we need to reduce the tracking cone
		/*
		{
			vec3_t	vec;
			float	dot;
			vec3_t	forward;

			AngleVectors (self->s.angles, forward, NULL, NULL);
			VectorSubtract (target->s.origin, self->s.origin, vec);
			VectorNormalize (vec);
			dot = DotProduct (vec, forward);

			if (dot > 0.6)
				continue;
		}
		*/

/*		if (!infront (self, target))
			continue;

		VectorSubtract (self->s.origin, target->s.origin, vec);
		len = VectorLength (vec);

		if (aquire == NULL || len < oldlen)
		{
			aquire = target;
			self->target_ent = aquire;
			oldlen = len;
		}
	}

	if (aquire != NULL)
	{
		VectorCopy (self->s.angles, oldang);
		VectorSubtract (aquire->s.origin, self->s.origin, vec);

		vectoangles (vec, self->s.angles);

		VectorNormalize (vec);
		VectorCopy (vec, self->movedir);
		VectorScale (vec, 500, self->velocity);
	}

	self->nextthink = level.time + 0.1;
}
*/
// RAFAEL
/*
void fire_heat (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage)
{
	edict_t *heat;

	heat = G_Spawn();
	VectorCopy (start, heat->s.origin);
	VectorCopy (dir, heat->movedir);
	vectoangles (dir, heat->s.angles);
	VectorScale (dir, speed, heat->velocity);
	heat->movetype = MOVETYPE_FLYMISSILE;
	heat->clipmask = MASK_SHOT;
	heat->solid = SOLID_BBOX;
	heat->s.effects |= EF_ROCKET;
	VectorClear (heat->mins);
	VectorClear (heat->maxs);
	heat->s.modelindex = gi.modelindex ("models/objects/rocket/tris.md2");
	heat->owner = self;
	heat->touch = rocket_touch;

	heat->nextthink = level.time + 0.1;
	heat->think = heat_think;

	heat->dmg = damage;
	heat->radius_dmg = radius_damage;
	heat->dmg_radius = damage_radius;
	heat->s.sound = gi.soundindex ("weapons/rockfly.wav");

	if (self->client)
		check_dodge (self, heat->s.origin, dir, speed);

	gi.linkentity (heat);
}
*/


// RAFAEL
/*
=================
	fire_plasma
=================
*/

void plasma_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
    vec3_t		origin;

    if (other == ent->owner)
        return;

    if (surf && (surf->flags & SURF_SKY))
    {
        G_FreeEdict (ent);
        return;
    }

    if (ent->owner->client)
        PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

    // calculate position for the explosion entity
    VectorMA (ent->s.origin, -0.02, ent->velocity, origin);

    if (other->takedamage)
    {
        T_Damage (other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, 0, MOD_PHALANX);
    }

    T_RadiusDamage(ent, ent->owner, ent->radius_dmg, other, ent->dmg_radius, MOD_PHALANX);

    gi.WriteByte (svc_temp_entity);
    gi.WriteByte (TE_PLASMA_EXPLOSION);
    gi.WritePosition (origin);
    gi.multicast (ent->s.origin, MULTICAST_PVS);

    G_FreeEdict (ent);
}


// RAFAEL
void fire_plasma (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage)
{
    edict_t *plasma;

    // calling entity made a sound, used to alert monsters
    self->lastsound = level.framenum;

    plasma = G_Spawn();
    VectorCopy (start, plasma->s.origin);
    VectorCopy (dir, plasma->movedir);
    vectoangles (dir, plasma->s.angles);
    VectorScale (dir, speed, plasma->velocity);
    plasma->movetype = MOVETYPE_FLYMISSILE;
    plasma->clipmask = MASK_SHOT;
    plasma->solid = SOLID_BBOX;

    VectorClear (plasma->mins);
    VectorClear (plasma->maxs);

    plasma->owner = self;
    plasma->touch = plasma_touch;
    plasma->nextthink = level.time + 8000/speed;
    plasma->think = G_FreeEdict;
    plasma->dmg = damage;
    plasma->radius_dmg = radius_damage;
    plasma->dmg_radius = damage_radius;
    plasma->s.sound = gi.soundindex ("weapons/rockfly.wav");

    plasma->s.modelindex = gi.modelindex ("sprites/s_photon.sp2");
    plasma->s.effects |= EF_PLASMA | EF_ANIM_ALLFAST;

    if (self->client)
        check_dodge (self, plasma->s.origin, dir, speed, damage_radius);

    gi.linkentity (plasma);


}


/*
=================
trap
=================
*/

// RAFAEL
extern void SP_item_foodcube (edict_t *best);
// RAFAEL
static void Trap_Think (edict_t *ent)
{
    edict_t	*target = NULL;
    edict_t	*best = NULL;
    vec3_t	vec;
    int		len, i;
    int		oldlen = 8000;
    vec3_t	forward, right, up;

    if (ent->timestamp < level.time)
    {
        BecomeExplosion1(ent);
        // note to self
        // cause explosion damage???
        return;
    }

    ent->nextthink = level.time + 0.1;

    if (!ent->groundentity)
        return;

    // ok lets do the blood effect
    if (ent->s.frame > 4)
    {
        if (ent->s.frame == 5)
        {
            if (ent->wait == 64)
                gi.sound(ent, CHAN_VOICE, gi.soundindex ("weapons/trapdown.wav"), 1, ATTN_IDLE, 0);

            ent->wait -= 2;
            ent->delay += level.time;

            for (i=0; i<3; i++)
            {

                best = G_Spawn();

                if (strcmp (ent->enemy->classname, "monster_gekk") == 0)
                {
                    best->s.modelindex = gi.modelindex ("models/objects/gekkgib/torso/tris.md2");
                    best->s.effects |= TE_GREENBLOOD;
                }
                else if (ent->mass > 200)
                {
                    best->s.modelindex = gi.modelindex ("models/objects/gibs/chest/tris.md2");
                    best->s.effects |= TE_BLOOD;
                }
                else
                {
                    best->s.modelindex = gi.modelindex ("models/objects/gibs/sm_meat/tris.md2");
                    best->s.effects |= TE_BLOOD;
                }

                AngleVectors (ent->s.angles, forward, right, up);

                RotatePointAroundVector( vec, up, right, ((360.0/3)* i)+ent->delay);
                VectorMA (vec, ent->wait/2, vec, vec);
                VectorAdd(vec, ent->s.origin, vec);
                VectorAdd(vec, forward, best->s.origin);

                best->s.origin[2] = ent->s.origin[2] + ent->wait;

                VectorCopy (ent->s.angles, best->s.angles);

                best->solid = SOLID_NOT;
                best->s.effects |= EF_GIB;
                best->takedamage = DAMAGE_YES;

                best->movetype = MOVETYPE_TOSS;
                best->svflags |= SVF_MONSTER;
                best->deadflag = DEAD_DEAD;

                VectorClear (best->mins);
                VectorClear (best->maxs);

                best->watertype = gi.pointcontents(best->s.origin);
                if (best->watertype & MASK_WATER)
                    best->waterlevel = 1;

                best->nextthink = level.time + 0.1;
                best->think = G_FreeEdict;
                gi.linkentity (best);
            }

            if (ent->wait < 19)
                ent->s.frame ++;

            return;
        }
        ent->s.frame ++;
        if (ent->s.frame == 8)
        {
            ent->nextthink = level.time + 1.0;
            ent->think = G_FreeEdict;

            best = G_Spawn ();
            SP_item_foodcube (best);
            VectorCopy (ent->s.origin, best->s.origin);
            best->s.origin[2]+= 16;
            best->velocity[2] = 400;
            best->count = ent->mass;
            gi.linkentity (best);
            return;
        }
        return;
    }

    ent->s.effects &= ~EF_TRAP;
    if (ent->s.frame >= 4)
    {
        ent->s.effects |= EF_TRAP;
        VectorClear (ent->mins);
        VectorClear (ent->maxs);

    }

    if (ent->s.frame < 4)
        ent->s.frame++;

    while ((target = findradius(target, ent->s.origin, 256)) != NULL)
    {
        if (target == ent)
            continue;
        if (!(target->svflags & SVF_MONSTER) && !target->client)
            continue;
        // if (target == ent->owner)
        //	continue;
        if (target->health <= 0)
            continue;
        if (!visible (ent, target))
            continue;
        if (!best)
        {
            best = target;
            continue;
        }
        VectorSubtract (ent->s.origin, target->s.origin, vec);
        len = VectorLength (vec);
        if (len < oldlen)
        {
            oldlen = len;
            best = target;
        }
    }

    // pull the enemy in
    if (best)
    {
        vec3_t	forward;

        if (best->groundentity)
        {
            best->s.origin[2] += 1;
            best->groundentity = NULL;
        }
        VectorSubtract (ent->s.origin, best->s.origin, vec);
        len = VectorLength (vec);
        if (best->client)
        {
            VectorNormalize (vec);
            VectorMA (best->velocity, 250, vec, best->velocity);
        }
        else
        {
            best->ideal_yaw = vectoyaw(vec);
//			M_ChangeYaw (best);
            AngleVectors (best->s.angles, forward, NULL, NULL);
            VectorScale (forward, 256, best->velocity);
        }

        gi.sound(ent, CHAN_VOICE, gi.soundindex ("weapons/trapsuck.wav"), 1, ATTN_IDLE, 0);

        if (len < 32)
        {
            if (best->mass < 400)
            {
                T_Damage (best, ent, ent->owner, vec3_origin, best->s.origin, vec3_origin, 100000, 1, 0, MOD_TRAP);
                ent->enemy = best;
                ent->wait = 64;
                VectorCopy (ent->s.origin, ent->s.old_origin);
                ent->timestamp = level.time + 30;
                if (deathmatch->value)
                    ent->mass = best->mass/4;
                else
                    ent->mass = best->mass/10;
                // ok spawn the food cube
                ent->s.frame = 5;
            }
            else
            {
                BecomeExplosion1(ent);
                // note to self
                // cause explosion damage???
                return;
            }

        }
    }
}

void fire_trap (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius, qboolean held)
{
    edict_t *trap;
    vec3_t dir;
    vec3_t forward, right, up;

    vectoangles(aimdir, dir);
    AngleVectors(dir, forward, right, up);

    trap = G_Spawn();
    VectorCopy(start, trap->s.origin);
    VectorScale(aimdir, speed, trap->velocity);
    VectorMA(trap->velocity, 200 + crandom() * 10.0, up, trap->velocity);
    VectorMA(trap->velocity, crandom() * 10.0, right, trap->velocity);
    VectorSet(trap->avelocity, 0, 300, 0);
    trap->movetype = MOVETYPE_BOUNCE;
    trap->clipmask = MASK_SHOT;
    trap->solid = SOLID_BBOX;
    VectorSet(trap->mins, -4, -4, 0);
    VectorSet(trap->maxs, 4, 4, 8);
    trap->s.modelindex = gi.modelindex("models/weapons/z_trap/tris.md2");
    trap->owner = self;
    trap->nextthink = level.time + 1.0;
    trap->think = Trap_Think;
    trap->dmg = damage;
    trap->dmg_radius = damage_radius;
    trap->classname = "htrap";
    trap->s.sound = gi.soundindex("weapons/traploop.wav");
    trap->spawnflags = held ? 3 : 1;

    if (timer <= 0.0)
        Grenade_Explode(trap);
    else
        gi.linkentity(trap);

    trap->timestamp = level.time + 30;
}

static void Prox_Explode(edict_t *ent)
{
    vec3_t origin;
    edict_t *owner = ent->teammaster;

    if (ent->teamchain && ent->teamchain->inuse && ent->teamchain->owner == ent)
    {
        G_FreeEdict(ent->teamchain);
        ent->teamchain = NULL;
    }

    if (!G_EntExists(owner))
        owner = ent->owner;
    if (!G_EntExists(owner))
        owner = ent;

    if (owner->client && !(owner->svflags & SVF_DEADMONSTER))
        PlayerNoise(owner, ent->s.origin, PNOISE_IMPACT);

    ent->takedamage = DAMAGE_NO;
    T_RadiusDamage(ent, owner, ent->dmg, ent, ent->dmg_radius, MOD_PROXY);

    VectorMA(ent->s.origin, -0.02, ent->velocity, origin);
    gi.WriteByte(svc_temp_entity);
    if (ent->waterlevel)
    {
        if (ent->groundentity)
            gi.WriteByte(TE_GRENADE_EXPLOSION_WATER);
        else
            gi.WriteByte(TE_ROCKET_EXPLOSION_WATER);
    }
    else
    {
        if (ent->groundentity)
            gi.WriteByte(TE_GRENADE_EXPLOSION);
        else
            gi.WriteByte(TE_ROCKET_EXPLOSION);
    }
    gi.WritePosition(origin);
    gi.multicast(ent->s.origin, MULTICAST_PHS);

    G_FreeEdict(ent);
}

static void prox_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
    self->takedamage = DAMAGE_NO;

    if (inflictor && inflictor->classname && !strcmp(inflictor->classname, "prox"))
    {
        self->think = Prox_Explode;
        self->nextthink = level.time + FRAMETIME;
        return;
    }

    Prox_Explode(self);
}

static void Prox_Field_Touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
    edict_t *prox = ent->owner;

    if (!(other->svflags & SVF_MONSTER) && !other->client)
        return;

    if (!G_EntExists(prox))
    {
        G_FreeEdict(ent);
        return;
    }

    if (other == prox || other == prox->teammaster)
        return;

    if (prox->think == Prox_Explode)
        return;

    gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/proxwarn.wav"), 1, ATTN_NORM, 0);
    prox->think = Prox_Explode;
    prox->nextthink = level.time + PROX_TIME_DELAY;
}

static void prox_seek(edict_t *ent)
{
    if (level.time > ent->wait)
    {
        Prox_Explode(ent);
        return;
    }

    ent->s.frame++;
    if (ent->s.frame > 13)
        ent->s.frame = 9;

    ent->think = prox_seek;
    ent->nextthink = level.time + 0.1;
}

static void prox_open(edict_t *ent)
{
    edict_t *search = NULL;

    if (ent->s.frame >= 9)
    {
        ent->s.sound = 0;
        ent->owner = NULL;

        if (ent->teamchain && ent->teamchain->inuse)
            ent->teamchain->touch = Prox_Field_Touch;

        while ((search = findradius(search, ent->s.origin, ent->dmg_radius + 10)) != NULL)
        {
            if (search == ent || search == ent->teammaster)
                continue;
            if (!search->classname)
                continue;
            if ((((search->svflags & SVF_MONSTER) || search->client) && (search->health > 0))
                && visible(ent, search))
            {
                gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/proxwarn.wav"), 1, ATTN_NORM, 0);
                Prox_Explode(ent);
                return;
            }
        }

        ent->wait = level.time + PROX_TIME_TO_LIVE;
        ent->think = prox_seek;
        ent->nextthink = level.time + 0.2;
        return;
    }

    if (ent->s.frame == 0)
        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/proxopen.wav"), 1, ATTN_NORM, 0);

    ent->s.frame++;
    ent->think = prox_open;
    ent->nextthink = level.time + 0.05;
}

static void prox_land(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
    edict_t *field;
    vec3_t dir;
    vec3_t land_point;

    if (surf && (surf->flags & SURF_SKY))
    {
        G_FreeEdict(ent);
        return;
    }

    if ((other->svflags & SVF_MONSTER) || other->client || other->takedamage)
    {
        if (other != ent->teammaster)
            Prox_Explode(ent);
        return;
    }

    if (!plane)
    {
        Prox_Explode(ent);
        return;
    }

    VectorMA(ent->s.origin, -10.0, plane->normal, land_point);
    if (gi.pointcontents(land_point) & (CONTENTS_SLIME | CONTENTS_LAVA))
    {
        Prox_Explode(ent);
        return;
    }

    field = G_Spawn();
    VectorCopy(ent->s.origin, field->s.origin);
    VectorClear(field->velocity);
    VectorClear(field->avelocity);
    VectorSet(field->mins, -PROX_BOUND_SIZE, -PROX_BOUND_SIZE, -PROX_BOUND_SIZE);
    VectorSet(field->maxs, PROX_BOUND_SIZE, PROX_BOUND_SIZE, PROX_BOUND_SIZE);
    field->movetype = MOVETYPE_NONE;
    field->solid = SOLID_TRIGGER;
    field->owner = ent;
    field->classname = "prox_field";
    gi.linkentity(field);

    VectorClear(ent->velocity);
    VectorClear(ent->avelocity);
    vectoangles(plane->normal, dir);
    dir[PITCH] += 90;
    VectorCopy(dir, ent->s.angles);

    ent->takedamage = DAMAGE_AIM;
    ent->movetype = MOVETYPE_NONE;
    ent->die = prox_die;
    ent->teamchain = field;
    ent->health = PROX_HEALTH;
    ent->nextthink = level.time + 0.05;
    ent->think = prox_open;
    ent->touch = NULL;
    ent->solid = SOLID_BBOX;

    gi.linkentity(ent);
}

void fire_prox(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float damage_radius)
{
    edict_t *prox;
    vec3_t dir;
    vec3_t forward, right, up;

    if (damage_radius < 64)
        damage_radius = 64;

    vectoangles(aimdir, dir);
    AngleVectors(dir, forward, right, up);

    prox = G_Spawn();
    VectorCopy(start, prox->s.origin);
    VectorScale(aimdir, speed, prox->velocity);
    VectorMA(prox->velocity, 200 + crandom() * 10.0, up, prox->velocity);
    VectorMA(prox->velocity, crandom() * 10.0, right, prox->velocity);
    VectorCopy(dir, prox->s.angles);
    prox->s.angles[PITCH] -= 90;
    prox->movetype = MOVETYPE_BOUNCE;
    prox->solid = SOLID_BBOX;
    prox->s.effects |= EF_GRENADE;
    prox->clipmask = MASK_SHOT | CONTENTS_LAVA | CONTENTS_SLIME;
    prox->s.renderfx |= RF_IR_VISIBLE;
    VectorSet(prox->mins, -6, -6, -6);
    VectorSet(prox->maxs, 6, 6, 6);
    prox->s.modelindex = gi.modelindex("models/weapons/g_prox/tris.md2");
    prox->owner = self;
    prox->teammaster = self;
    prox->touch = prox_land;
    prox->think = Prox_Explode;
    prox->nextthink = level.time + PROX_TIME_TO_LIVE;
    prox->dmg = damage;
    prox->dmg_radius = damage_radius;
    prox->classname = "prox";

    gi.linkentity(prox);
}

static void tesla_remove(edict_t *self)
{
    edict_t *cur, *next;
    edict_t *owner = self->teammaster;
    vec3_t origin;

    self->takedamage = DAMAGE_NO;

    cur = self->teamchain;
    while (cur)
    {
        next = cur->teamchain;
        G_FreeEdict(cur);
        cur = next;
    }
    self->teamchain = NULL;

    if (!G_EntExists(owner))
        owner = self->owner;
    if (!G_EntExists(owner))
        owner = self;

    if (self->radius_dmg > 0 && self->dmg_radius > 0)
        T_RadiusDamage(self, owner, self->radius_dmg, self, self->dmg_radius, MOD_LIGHTNING);

    VectorMA(self->s.origin, -0.02, self->velocity, origin);
    gi.WriteByte(svc_temp_entity);
    if (self->waterlevel)
    {
        if (self->groundentity)
            gi.WriteByte(TE_GRENADE_EXPLOSION_WATER);
        else
            gi.WriteByte(TE_ROCKET_EXPLOSION_WATER);
    }
    else
    {
        if (self->groundentity)
            gi.WriteByte(TE_GRENADE_EXPLOSION);
        else
            gi.WriteByte(TE_ROCKET_EXPLOSION);
    }
    gi.WritePosition(origin);
    gi.multicast(self->s.origin, MULTICAST_PHS);

    G_FreeEdict(self);
}

static void tesla_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
    tesla_remove(self);
}

static void tesla_blow(edict_t *self)
{
    self->radius_dmg = self->dmg * TESLA_EXPLOSION_DAMAGE_MULT;
    self->dmg_radius = TESLA_EXPLOSION_RADIUS;
    tesla_remove(self);
}

static void tesla_zap(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
}

static void tesla_think_active(edict_t *self)
{
    int i, num;
    edict_t *touch[MAX_EDICTS], *hit;
    edict_t *attacker = self->teammaster;
    vec3_t dir, start, normal;
    trace_t tr;

    if (!self->teamchain || !self->teamchain->inuse || level.time > self->air_finished)
    {
        tesla_remove(self);
        return;
    }

    if (!G_EntExists(attacker))
        attacker = self;

    VectorCopy(self->s.origin, start);
    start[2] += 16;

    num = gi.BoxEdicts(self->teamchain->absmin, self->teamchain->absmax, touch, MAX_EDICTS, AREA_SOLID);
    for (i = 0; i < num; i++)
    {
        if (!self->inuse)
            return;

        hit = touch[i];
        if (!hit || !hit->inuse || hit == self || hit->health < 1)
            continue;
        if (!(hit->svflags & SVF_MONSTER) && !hit->client && !hit->takedamage)
            continue;
        if (hit == attacker)
            continue;

        tr = gi.trace(start, vec3_origin, vec3_origin, hit->s.origin, self, MASK_SHOT);
        if (!(tr.fraction == 1.0f || tr.ent == hit))
            continue;

        VectorSubtract(hit->s.origin, start, dir);
        if (tr.fraction == 1.0f)
            VectorClear(normal);
        else
            VectorCopy(tr.plane.normal, normal);

        if ((hit->svflags & SVF_MONSTER) && !(hit->flags & (FL_FLY | FL_SWIM)))
            T_Damage(hit, self, attacker, dir, tr.endpos, normal, self->dmg, 0, 0, MOD_LIGHTNING);
        else
            T_Damage(hit, self, attacker, dir, tr.endpos, normal, self->dmg, TESLA_KNOCKBACK, 0, MOD_LIGHTNING);

        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_LIGHTNING);
        gi.WriteShort(hit - g_edicts);
        gi.WriteShort(self - g_edicts);
        gi.WritePosition(tr.endpos);
        gi.WritePosition(start);
        gi.multicast(start, MULTICAST_PVS);
    }

    self->think = tesla_think_active;
    self->nextthink = level.time + FRAMETIME;
}

static void tesla_activate(edict_t *self)
{
    edict_t *trigger;
    float radius = self->dmg_radius;

    if (gi.pointcontents(self->s.origin) & (CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_WATER))
    {
        tesla_blow(self);
        return;
    }

    if (radius < TESLA_DAMAGE_RADIUS_MIN)
        radius = TESLA_DAMAGE_RADIUS_MIN;

    trigger = G_Spawn();
    VectorCopy(self->s.origin, trigger->s.origin);
    VectorSet(trigger->mins, -radius, -radius, self->mins[2]);
    VectorSet(trigger->maxs, radius, radius, radius);
    trigger->movetype = MOVETYPE_NONE;
    trigger->solid = SOLID_TRIGGER;
    trigger->owner = self;
    trigger->touch = tesla_zap;
    trigger->classname = "tesla trigger";
    gi.linkentity(trigger);

    VectorClear(self->s.angles);
    if (deathmatch->value)
        self->owner = NULL;
    self->teamchain = trigger;
    self->think = tesla_think_active;
    self->nextthink = level.time + FRAMETIME;
    self->air_finished = level.time + TESLA_TIME_TO_LIVE;
}

static void tesla_think(edict_t *ent)
{
    if (gi.pointcontents(ent->s.origin) & (CONTENTS_SLIME | CONTENTS_LAVA))
    {
        tesla_remove(ent);
        return;
    }

    VectorClear(ent->s.angles);

    if (!ent->count)
    {
        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/teslaopen.wav"), 1, ATTN_NORM, 0);
        ent->count = 1;
    }

    ent->s.frame++;
    if (ent->s.frame > 14)
    {
        ent->s.frame = 14;
        ent->think = tesla_activate;
        ent->nextthink = level.time + 0.1;
        return;
    }

    if (ent->s.frame > 9)
    {
        if (ent->s.frame == 10)
        {
            if (ent->owner && ent->owner->client)
                PlayerNoise(ent->owner, ent->s.origin, PNOISE_WEAPON);
            ent->s.skinnum = 1;
        }
        else if (ent->s.frame == 12)
        {
            ent->s.skinnum = 2;
        }
        else if (ent->s.frame == 14)
        {
            ent->s.skinnum = 3;
        }
    }

    ent->think = tesla_think;
    ent->nextthink = level.time + 0.1;
}

static void tesla_lava(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
    vec3_t land_point;

    if (plane)
    {
        VectorMA(ent->s.origin, -20.0, plane->normal, land_point);
        if (gi.pointcontents(land_point) & (CONTENTS_SLIME | CONTENTS_LAVA))
        {
            tesla_blow(ent);
            return;
        }
    }

    if (level.time < ent->timestamp)
        return;
    ent->timestamp = level.time + 0.25f;

    if (random() > 0.5)
        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb1a.wav"), 1, ATTN_NORM, 0);
    else
        gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb2a.wav"), 1, ATTN_NORM, 0);
}

void fire_tesla(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float damage_radius)
{
    edict_t *tesla;
    vec3_t dir;
    vec3_t forward, right, up;

    if (damage_radius < TESLA_DAMAGE_RADIUS_MIN)
        damage_radius = TESLA_DAMAGE_RADIUS_MIN;

    vectoangles(aimdir, dir);
    AngleVectors(dir, forward, right, up);

    tesla = G_Spawn();
    VectorCopy(start, tesla->s.origin);
    VectorScale(aimdir, speed, tesla->velocity);
    VectorMA(tesla->velocity, 200 + crandom() * 10.0, up, tesla->velocity);
    VectorMA(tesla->velocity, crandom() * 10.0, right, tesla->velocity);
    VectorClear(tesla->s.angles);
    tesla->movetype = MOVETYPE_BOUNCE;
    tesla->solid = SOLID_BBOX;
    tesla->s.effects |= EF_GRENADE;
    tesla->s.renderfx |= RF_IR_VISIBLE;
    VectorSet(tesla->mins, -12, -12, 0);
    VectorSet(tesla->maxs, 12, 12, 20);
    tesla->s.modelindex = gi.modelindex("models/weapons/g_tesla/tris.md2");
    tesla->owner = self;
    tesla->teammaster = self;
    tesla->wait = level.time + TESLA_TIME_TO_LIVE;
    tesla->think = tesla_think;
    tesla->nextthink = level.time + TESLA_ACTIVATE_TIME;
    tesla->touch = tesla_lava;
    tesla->health = deathmatch->value ? 20 : 30;
    tesla->takedamage = DAMAGE_YES;
    tesla->die = tesla_die;
    tesla->dmg = damage;
    tesla->dmg_radius = damage_radius;
    tesla->radius_dmg = 0;
    tesla->classname = "tesla";
    tesla->clipmask = MASK_SHOT | CONTENTS_SLIME | CONTENTS_LAVA;

    gi.linkentity(tesla);
}
