#include "g_local.h"

void mirv_explode (edict_t *self)
{
    vec3_t  grenade1, grenade2, grenade3, grenade4;
    vec3_t  grenade5, grenade6, grenade7, grenade8;
    edict_t *e;

    // remove grenade if owner dies or becomes invalid
    if (!G_EntIsAlive(self->owner))
    {
        G_FreeEdict(self);
        return;
    }

    // explosion effect
    gi.WriteByte (svc_temp_entity);
    if (self->waterlevel)
    {
        if (self->groundentity)
            gi.WriteByte (TE_GRENADE_EXPLOSION_WATER);
        else
            gi.WriteByte (TE_ROCKET_EXPLOSION_WATER);
    }
    else
    {
        if (self->groundentity)
            gi.WriteByte (TE_GRENADE_EXPLOSION);
        else
            gi.WriteByte (TE_ROCKET_EXPLOSION);
    }
    gi.WritePosition (self->s.origin);// raise this a bit?
    gi.multicast (self->s.origin, MULTICAST_PVS);

    T_RadiusDamage(self, self->owner, self->dmg, NULL, self->dmg_radius, MOD_MIRV);

    // grenade may have been removed if owner was killed
    if (!self || !self->inuse || !self->owner || !self->owner->inuse)
        return;

    VectorSet(grenade1, 15, 15, 30);
    VectorSet(grenade2, 15, -15, 30);
    VectorSet(grenade3, -15, 15, 30);
    VectorSet(grenade4, -15, -15, 30);

    VectorSet(grenade5, 20, 20, 30);
    VectorSet(grenade6, 20, -20, 30);
    VectorSet(grenade7, -20, 20, 30);
    VectorSet(grenade8, -20, -20, 30);

    // raise starting point?
    e = fire_grenade2(self->owner, self->s.origin, grenade1, self->dmg, 10, 1.5, self->dmg_radius, self->radius_dmg, false);
    e->mtype = M_MIRV;
    e->touch = NULL;
    e = fire_grenade2(self->owner, self->s.origin, grenade2, self->dmg, 10, 1.5, self->dmg_radius, self->radius_dmg, false);
    e->mtype = M_MIRV;
    e->touch = NULL;
    e = fire_grenade2(self->owner, self->s.origin, grenade2, self->dmg, 10, 1.5, self->dmg_radius, self->radius_dmg, false);
    e->mtype = M_MIRV;
    e->touch = NULL;
    e = fire_grenade2(self->owner, self->s.origin, grenade3, self->dmg, 10, 1.5, self->dmg_radius, self->radius_dmg, false);
    e->mtype = M_MIRV;
    e->touch = NULL;
    e = fire_grenade2(self->owner, self->s.origin, grenade4, self->dmg, 10, 1.5, self->dmg_radius, self->radius_dmg, false);
    e->mtype = M_MIRV;
    e->touch = NULL;
    e = fire_grenade2(self->owner, self->s.origin, grenade5, self->dmg, 10, 1.5, self->dmg_radius, self->radius_dmg, false);
    e->mtype = M_MIRV;
    e->touch = NULL;
    e = fire_grenade2(self->owner, self->s.origin, grenade6, self->dmg, 10, 1.5, self->dmg_radius, self->radius_dmg, false);
    e->mtype = M_MIRV;
    e->touch = NULL;
    /*e = fire_grenade2(self->owner, self->s.origin, grenade7, self->dmg, 10, 1.5, self->dmg_radius, self->radius_dmg, false);
    e->mtype = M_MIRV;
    e->touch = NULL;
    e = fire_grenade2(self->owner, self->s.origin, grenade8, self->dmg, 10, 1.5, self->dmg_radius, self->radius_dmg, false);
    e->mtype = M_MIRV;
    e->touch = NULL;*/

    G_FreeEdict(self);
}

void mirv_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
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
        mirv_explode(ent);
}

void fire_mirv_grenade (edict_t *self, vec3_t start, vec3_t aimdir, int damage, float radius, int speed, float timer)
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
    grenade->s.modelindex = gi.modelindex ("models/objects/grenade/tris.md2");
    grenade->owner = self;
    grenade->touch = mirv_touch;
    grenade->think = mirv_explode;
    grenade->dmg = damage;
    grenade->radius_dmg = damage;
    grenade->dmg_radius = radius;
    grenade->classname = "mirv grenade";
    gi.linkentity (grenade);
    grenade->nextthink = level.time + timer;

    // adjust velocity
    VectorScale (aimdir, speed, grenade->velocity);
    VectorMA (grenade->velocity, 200 + crandom() * 10.0, up, grenade->velocity);
    VectorMA (grenade->velocity, crandom() * 10.0, right, grenade->velocity);
    VectorSet (grenade->avelocity, 300, 300, 300);
}

void Cmd_TossMirv (edict_t *ent)
{
    int		damage, cost;
    float	radius;
    vec3_t	forward, right, start, offset;

    //Talent: Bombardier - reduces grenade cost
    cost = MIRV_COST - vrx_get_talent_level(ent, TALENT_BOMBARDIER);

    if (!V_CanUseAbilities(ent, MIRV, MIRV_COST, true))
        return;

    damage = MIRV_INITIAL_DAMAGE + MIRV_ADDON_DAMAGE * ent->myskills.abilities[MIRV].current_level;
    radius = MIRV_INITIAL_RADIUS + MIRV_ADDON_RADIUS * ent->myskills.abilities[MIRV].current_level;

    // get starting position and forward vector
    AngleVectors (ent->client->v_angle, forward, right, NULL);
    VectorSet(offset, 0, 8,  ent->viewheight-8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    fire_mirv_grenade(ent, start, forward, damage, radius, 600, 2.0);

    ent->client->ability_delay = level.time + MIRV_DELAY;
    ent->client->pers.inventory[power_cube_index] -= cost;
}