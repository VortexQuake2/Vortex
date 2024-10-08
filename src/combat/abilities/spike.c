#include "g_local.h"

void spike_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (G_ValidTarget(self->owner, other, false, true))
	{
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, 
			plane->normal, self->dmg, self->dmg, 0, MOD_SPIKE);

		gi.sound (other, CHAN_WEAPON, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);

		// only stun if the entity is alive and they haven't been stunned too recently
		if (G_EntIsAlive(other) && (level.time > (other->holdtime + 1.0)))
		{
			// stun them
			if (other->client)
			{
				other->client->ability_delay = level.time + self->dmg_radius;
				other->holdtime = level.time + self->dmg_radius;
			}
			else
			{
				// FIXME: this method of stunning works; however it may cause undesired behavior (monsters sliding)
				// one possible fix would be to have a temporary stunned_think set while stunned that handles sliding, worldeffects, etc
				// and switches back to the prev_think after the stun is complete
				other->nextthink = level.time + self->dmg_radius;
			}
		}

		
	}

	G_FreeEdict(self);
}

void fire_spike (edict_t *self, vec3_t start, vec3_t dir, int damage, float stun_length, int speed)
{
	edict_t	*bolt;

	// calling entity made a sound, used to alert monsters
	self->lastsound = level.framenum;

	// create bolt
	bolt = G_Spawn();
//	VectorSet (bolt->mins, -4, -4, 4);
//	VectorSet (bolt->maxs, 4, 4, 4);
	bolt->s.modelindex = gi.modelindex ("models/objects/spike/tris.md2");
	VectorCopy (start, bolt->s.origin);
	VectorCopy (start, bolt->s.old_origin);
	vectoangles (dir, bolt->s.angles);
	VectorScale (dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->owner = self;
	bolt->touch = spike_touch;
	bolt->nextthink = level.time + 3;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	bolt->dmg_radius = stun_length;
	bolt->classname = "spike";
	gi.linkentity (bolt);
}

void SpikeAttack (edict_t *ent)
{
	int spike_level = ent->myskills.abilities[SPIKE].current_level;
	int spiker_level = ent->myskills.abilities[SPIKER].current_level;
	int		i, move, damage;
	float	delay;
	float synergy_bonus = 1.0 + SPIKE_SPIKER_SYNERGY_BONUS * spiker_level;
	vec3_t	angles, v, org;
	vec3_t	offset, forward, right, start;

	// get starting position and forward vector
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	VectorCopy(start, org);

	AngleVectors(ent->s.angles, NULL, right, NULL);

	// ready angles
	VectorCopy(forward, v);
	vectoangles(forward, angles);
	move = SPIKE_FOV/SPIKE_SHOTS;

	// calculate damage and stun length
	damage = (SPIKE_INITIAL_DMG + SPIKE_ADDON_DMG * spike_level) * synergy_bonus;
	//damage *= 1.0 + 0.1 * vrx_get_talent_level(ent, TALENT_DEADLY_SPIKES); // Talent Deadly Spikes - increases spike damage

	delay = SPIKE_STUN_ADDON * ent->myskills.abilities[SPIKE].current_level;
	if (delay < SPIKE_STUN_MIN)
		delay = SPIKE_STUN_MIN;
	else if (delay > SPIKE_STUN_MAX)
		delay = SPIKE_STUN_MAX;

	// fire spikes spaced evenly across horizontal plane
	for (i=1; i<SPIKE_SHOTS+1; i++)
	{
		if (i < SPIKE_SHOTS/2)
		{
			angles[YAW]-=move;
			// move this spike ahead of the last one to avoid collisions
			VectorMA(org, 1, forward, org);
		}
		else if (i > SPIKE_SHOTS/2)
		{
			angles[YAW]+=move;
			VectorMA(org, -1, forward, org);
		}
		else
		{
			vectoangles(forward, angles);
			VectorCopy(start, org);
		}

		ValidateAngles(angles);
		AngleVectors(angles, v, NULL, NULL);
		fire_spike(ent, org, v, damage, delay, SPIKE_SPEED);
	}

	ent->client->pers.inventory[power_cube_index] -= SPIKE_COST;
	ent->client->ability_delay = level.time + SPIKE_DELAY;

	gi.sound (ent, CHAN_WEAPON, gi.soundindex("brain/brnatck2.wav"), 1, ATTN_NORM, 0);
}

void Cmd_Spike_f (edict_t *ent)
{
	if (!G_CanUseAbilities(ent, ent->myskills.abilities[SPIKE].current_level, SPIKE_COST))
		return;
	if (ent->myskills.abilities[SPIKE].disable)
		return;
	SpikeAttack(ent);
}