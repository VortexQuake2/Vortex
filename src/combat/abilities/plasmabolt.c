#include "g_local.h"


void plasmabolt_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	// remove plasmabolt if owner dies or becomes invalid or if we touch a sky brush or if we bounced too much
	if (!G_EntIsAlive(ent->owner) || (surf && (surf->flags & SURF_SKY)) || (ent->style >= 10))
	{
		G_FreeEdict(ent);
		return;
	}

	// if this is the first impact, set expiration timer
	if (!ent->style)
	{
		ent->style++; // increment the number of bounces
		ent->delay = level.time + ent->random;
	}
	
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BFG_EXPLOSION);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

    gi.sound(ent, CHAN_VOICE, gi.soundindex(va("abilities/largefireimpact%d.wav", GetRandom(1, 3))), 1, ATTN_NORM, 0);

	if (G_EntExists(other))
	{
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, ent->dmg, 0, MOD_PLASMABOLT);
		T_RadiusDamage(ent, ent->owner, ent->dmg, other, ent->dmg_radius, MOD_PLASMABOLT);
		G_FreeEdict(ent);
		return;
	}

	T_RadiusDamage(ent, ent->owner, ent->dmg, NULL, ent->dmg_radius, MOD_PLASMABOLT);
}

void plasmabolt_think (edict_t *self)
{
	// owner must be alive
	if (!G_EntIsAlive(self->owner) || (self->delay < level.time)) 
	{
		BecomeTE(self);
		return;
	}

	self->nextthink = level.time + FRAMETIME;
}

void fire_plasmabolt (edict_t *self, vec3_t start, vec3_t aimdir, int damage, float damage_radius, int speed, float duration)
{
	edict_t	*bolt;
	vec3_t	dir;
	vec3_t	forward;

	//  entity made a sound, used to alert monsters
	self->lastsound = level.framenum;

	// get aiming angles
	vectoangles(aimdir, dir);
	// get directional vectors
	AngleVectors(dir, forward, NULL, NULL);

	// spawn plasmabolt entity
	bolt = G_Spawn();
	VectorCopy (start, bolt->s.origin);
	bolt->movetype = MOVETYPE_WALLBOUNCE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= EF_BFG | EF_ANIM_ALLFAST;
	bolt->s.modelindex = gi.modelindex ("sprites/s_bfg1.sp2");
	bolt->owner = self;
	bolt->touch = plasmabolt_touch;
	bolt->think = plasmabolt_think;
	bolt->dmg_radius = damage_radius;
	bolt->dmg = damage;
	bolt->classname = "plasma bolt";
	bolt->random = duration;
	bolt->delay = level.time + 10.0;
	gi.linkentity(bolt);
	bolt->nextthink = level.time + FRAMETIME;
	VectorCopy(dir, bolt->s.angles);

	// adjust velocity
	VectorScale (aimdir, speed, bolt->velocity);
}

void Cmd_Plasmabolt_f (edict_t *ent)
{
	int		slvl = ent->myskills.abilities[PLASMA_BOLT].current_level;
	int		damage, speed, duration;
	float	radius;
	vec3_t	forward, right, start, offset;

	if (!V_CanUseAbilities(ent, PLASMA_BOLT, PLASMABOLT_COST, true))
		return;

	damage = PLASMABOLT_INITIAL_DAMAGE + PLASMABOLT_ADDON_DAMAGE * slvl;
	radius = PLASMABOLT_INITIAL_RADIUS + PLASMABOLT_ADDON_RADIUS * slvl;
	speed = PLASMABOLT_INITIAL_SPEED + PLASMABOLT_ADDON_SPEED * slvl;
	duration = PLASMABOLT_INITIAL_DURATION + PLASMABOLT_ADDON_DURATION * slvl;

	// get starting position and forward vector
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_plasmabolt(ent, start, forward, damage, radius, speed, duration);

	if (pvm->value)
		ent->client->ability_delay = level.time + PLASMABOLT_DELAY;
	else
		ent->client->ability_delay = level.time + PLASMABOLT_DELAY_PVP;

	ent->client->pers.inventory[power_cube_index] -= PLASMABOLT_COST;

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

    gi.sound(ent, CHAN_ITEM, gi.soundindex("abilities/holybolt2.wav"), 1, ATTN_NORM, 0);
}