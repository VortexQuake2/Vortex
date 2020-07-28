#include "g_local.h"

void spikegren_remove (edict_t *self)
{
	if (self->owner && self->owner->inuse)
	{
		self->owner->num_spikegrenades--;
		safe_cprintf(self->owner, 
				     PRINT_HIGH, 
					 "%d/%d spike grenades remaining\n", 
					 self->owner->num_spikegrenades, 
					 (int)SPIKEGRENADE_MAX_COUNT);
	}

	G_FreeEdict(self);
}

void spikegren_removeall (edict_t *ent)
{
	edict_t *e = NULL;

	while((e = G_Find(e, FOFS(classname), "spikegren")) != NULL) 
	{
		if (e && e->owner && (e->owner == ent))
			spikegren_remove(e);
	}

	ent->num_spikegrenades = 0;
}

void spikey_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if ((surf && (surf->flags & SURF_SKY)) || !self->creator || !self->creator->inuse)
	{
		G_FreeEdict(self);
		return;
	}

	if (other->takedamage)
	{
		T_Damage(other, self, self->creator, self->velocity, self->s.origin, 
			plane->normal, self->dmg, self->dmg, 0, MOD_SPIKEGRENADE);

		gi.sound (other, CHAN_WEAPON, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);
	}

	G_FreeEdict(self);
}

void fire_spikey (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t	*bolt;

	// create bolt
	bolt = G_Spawn();
	bolt->s.modelindex = gi.modelindex ("models/objects/spike/tris.md2");
	VectorCopy (start, bolt->s.origin);
	VectorCopy (start, bolt->s.old_origin);
	vectoangles (dir, bolt->s.angles);
	VectorScale (dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	//bolt->owner = self;
	bolt->creator = self;
	bolt->touch = spikey_touch;
	bolt->nextthink = level.time + 5;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	bolt->classname = "spikey";
	gi.linkentity (bolt);
}

void spikegren_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (surf && (surf->flags & SURF_SKY))
	{
		spikegren_remove(self);
		return;
	}
}

void spikegren_explode (edict_t *self)
{
	int		speed, i;
	vec3_t	forward, start;

	if ((level.time > self->delay) || !G_EntIsAlive(self->owner))
	{
		spikegren_remove(self);
		return;
	}

	if (level.time > self->monsterinfo.attack_finished)
	{
		speed = SPIKEGRENADE_INITIAL_SPEED + SPIKEGRENADE_ADDON_SPEED * self->monsterinfo.level;

		G_EntMidPoint(self, start);

		for (i = 0; i < SPIKEGRENADE_TURNS; i++)
		{
			self->s.angles[YAW] += 45;
			AngleCheck(&self->s.angles[YAW]);

			AngleVectors(self->s.angles, forward, NULL, NULL);
			fire_spikey(self->owner, start, forward, self->dmg, speed);
		}

		self->monsterinfo.attack_finished = level.time + SPIKEGRENADE_TURN_DELAY;
	}

	self->s.angles[YAW] += SPIKEGRENADE_TURN_DEGREES;
	AngleCheck(&self->s.angles[YAW]);

	self->nextthink = level.time + FRAMETIME;
}

void spikegren_think (edict_t *self)
{
	if (!G_EntIsAlive(self->owner) || gi.pointcontents(self->s.origin) & MASK_SOLID)
	{
		spikegren_remove(self);
		return;
	}

	if (self->velocity[0] == 0 && self->velocity[1] == 0 && self->velocity[2] == 0)
	{
		trace_t	tr;
		vec3_t	end;

	//	gi.dprintf("%.1f %.1f %.1f\n", self->mins[0], self->mins[1], self->mins[2]);
	//	gi.dprintf("%.1f %.1f %.1f\n", self->maxs[0], self->maxs[1], self->maxs[2]);

		self->delay = level.time + self->delay;	// start the timer
		self->think = spikegren_explode;

		// move into position
		VectorCopy(self->s.origin, end);
		end[2] += 32;
		tr = gi.trace(self->s.origin, NULL, NULL, end, self, MASK_SHOT);
		VectorCopy(tr.endpos, self->s.origin);
		gi.linkentity(self);
		
		// hold in-place
		self->movetype = MOVETYPE_NONE;
	}

	M_SetEffects(self);//4.5

	self->nextthink = level.time + FRAMETIME;
}

void spikegren_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// explosion effect
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	safe_cprintf(self->owner, PRINT_HIGH, "Spike grenade destroyed.\n");

	spikegren_remove(self);
}

void ThrowSpikeGrenade (edict_t *self, vec3_t start, vec3_t forward, int slevel, float duration)
{
	edict_t *grenade;

	//  entity made a sound, used to alert monsters
	self->lastsound = level.framenum;

	grenade = G_Spawn();
	VectorSet (grenade->mins, -10, -10, -16);
	VectorSet (grenade->maxs, 10, 10, 4);

	// don't get stuck in a floor/wall
	if (!G_IsValidLocation(self, start, grenade->mins, grenade->maxs))
	{
		G_FreeEdict(self);
		return;
	}

	VectorCopy(start, grenade->s.origin);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->health = grenade->max_health = 100;//4.4
	grenade->takedamage = DAMAGE_AIM;//4.4
	grenade->die = spikegren_die;//4.4
	grenade->owner = self;
	grenade->monsterinfo.level = slevel;
	grenade->mtype = M_SPIKE_GRENADE;
	grenade->dmg = SPIKEGRENADE_INITIAL_DAMAGE + SPIKEGRENADE_ADDON_DAMAGE * slevel;

	grenade->classname = "spikegren";
	grenade->think = spikegren_think;
	grenade->touch = spikegren_touch;
	grenade->nextthink = level.time + FRAMETIME;
	grenade->delay = duration;
	grenade->solid = SOLID_BBOX;//SOLID_TRIGGER;
	grenade->clipmask = MASK_SHOT;
	grenade->s.modelindex = gi.modelindex ("models/items/ammo/grenades/medium/tris.md2");
	gi.linkentity(grenade);

	VectorScale(forward, 400, grenade->velocity);
	self->num_spikegrenades++;
}

void Cmd_SpikeGrenade_f (edict_t *ent)
{
	int cost;
	vec3_t forward, start;

	if (ent->num_spikegrenades >= SPIKEGRENADE_MAX_COUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "You've reached the maximum number of spike grenades (%d)\n", (int)SPIKEGRENADE_MAX_COUNT);
		return;
	}

	//Talent: Bombardier - reduces grenade cost
    cost = SPIKEGRENADE_COST - vrx_get_talent_level(ent, TALENT_BOMBARDIER);

	if (!V_CanUseAbilities(ent, SPIKE_GRENADE, cost, true))
		return;

	VectorCopy(ent->s.origin, start);
	start[2] += ent->viewheight - 8;
	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	VectorMA(start, 24, forward, start);

	// don't allow them to throw through thin walls
	if (!G_IsClearPath(ent, MASK_SHOT, ent->s.origin, start))
		return;

	ThrowSpikeGrenade(ent, start, forward, ent->myskills.abilities[SPIKE_GRENADE].current_level, SPIKEGRENADE_DURATION);

	ent->client->pers.inventory[power_cube_index] -= cost;
	ent->client->ability_delay = level.time + SPIKEGRENADE_DELAY;
}