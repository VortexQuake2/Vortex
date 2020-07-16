#include "../../quake2/g_local.h"

qboolean hammer_return (edict_t *self)
{
	// only reverse course once
	if (self->style)
		return false;

	VectorNegate(self->movedir, self->movedir);
	VectorNegate(self->velocity, self->velocity);
	self->style = 1;
	return true;
}

void hammer_think1 (edict_t *self)
{
	vec3_t v;

	// remove hammer if activator dies
	if (!G_EntIsAlive(self->activator))
	{
		if (self->activator && self->activator->inuse)
			self->activator->num_hammers--;

		BecomeTE(self);
		return;
	}

	// make hammer spin
	self->s.angles[YAW] += GetRandom(36, 108);
	AngleCheck(&self->s.angles[YAW]);

	// swing back
	if (level.framenum >= self->monsterinfo.nextattack)
		self->style = 1;
	if (self->style)
	{
		VectorSubtract(self->activator->s.origin, self->s.origin, v);
		VectorNormalize(v);
		//VectorMA(self->velocity, 100.0, v, v);
		//VectorNormalize(v);
		VectorScale (v, 400, self->velocity);
	}

	self->nextthink = level.time + FRAMETIME;
}

void hammer_think (edict_t *self)
{
	vec3_t forward;

	// remove hammer if owner dies
	if (!G_EntIsAlive(self->owner))
	{
		BecomeTE(self);
		return;
	}

	// make hammer spin
	self->s.angles[YAW] += GetRandom(36, 108);
	AngleCheck(&self->s.angles[YAW]);

	// increase hammer velocity
	if (self->speed < 2000)
		self->speed += HAMMER_ADDON_SPEED;

	// reduce turning rate to make spiral bigger
	if (self->count > 9)
	{
		if (self->count <= 0.33*HAMMER_TURN_RATE)
		{
			if (!(sf2qf(level.framenum)%3))
				self->count--;
		}
		else if (self->count <= 0.5*HAMMER_TURN_RATE)
		{
			if (!(sf2qf(level.framenum)%2))
				self->count--;
		}
		else
			self->count--;
	}

	// modify trajectory and velocity
	self->move_angles[YAW] += self->count;
	AngleCheck(&self->move_angles[YAW]);
	AngleVectors(self->move_angles, forward, NULL, NULL);
	VectorScale (forward, self->speed , self->velocity);

	self->nextthink = level.time + FRAMETIME;
}

void hammer_touch1 (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	// touched a sky brush
	if (surf && (surf->flags & SURF_SKY))
	{
		// decrement counter
		if (self->activator && self->activator->inuse)
			self->activator->num_hammers--;

		G_FreeEdict(self);
		return;
	}

	// touched a non-brush entity
	if (G_EntExists(other))
	{
		// touched owner/activator
		if (self->activator && self->activator->inuse && other == self->activator)
		{
			// decrement counter
			if (self->activator && self->activator->inuse)
				self->activator->num_hammers--;

			// refund power cubes
			self->activator->client->pers.inventory[power_cube_index] += HAMMER_COST;

			G_FreeEdict(self);
			return;
		}
		// try to hurt anyone else
		else
			T_Damage(other, self, self->activator, vec3_origin, self->s.origin, 
				plane->normal, self->dmg, self->dmg, 0, MOD_HAMMER);
	}

	// explosion effect
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BFG_EXPLOSION);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PHS);

    gi.sound(self, CHAN_WEAPON, gi.soundindex("abilities/boom2.wav"), 1, ATTN_NORM, 0);

	// only bounce once
	if (self->style)
	{
		// decrement counter
		if (self->activator && self->activator->inuse)
			self->activator->num_hammers--;

		G_FreeEdict(self);
	}
	else
		self->style = 1;
}

void hammer_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}

	if (G_EntExists(other))
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, 
			plane->normal, self->dmg, self->dmg, 0, MOD_HAMMER);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BFG_EXPLOSION);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PHS);

    gi.sound(self, CHAN_WEAPON, gi.soundindex("abilities/boom2.wav"), 1, ATTN_NORM, 0);

	G_FreeEdict(self);
}

void SpawnBlessedHammer (edict_t *ent, int boomerang_level)
{
	edict_t *hammer;
	vec3_t	forward, right, offset, start;

	//Talent: Boomerang
	if (boomerang_level > 0)
	{
		// too many hammers out
		if (ent->num_hammers + 1 > boomerang_level)
			return;

		// set-up firing parameters
		AngleVectors (ent->client->v_angle, forward, right, NULL);
		VectorScale (forward, -3, ent->client->kick_origin);
		VectorSet(offset, 0, 7,  ent->viewheight-8);
		P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

		VectorMA(start, 40, forward, start);
	}
	else
	{
		// determine starting position
		AngleVectors(ent->s.angles, forward, right, NULL);
		VectorCopy(ent->s.origin, start);

		// adjust so hammer spirals around the owner
		VectorMA(start, 24, forward, start);
		VectorMA(start, 40, right, start);
	}

	// create hammer entity
	hammer = G_Spawn();
	VectorSet (hammer->mins, -8, -8, 0);
	VectorSet (hammer->maxs, 8, 8, 0);
	hammer->s.modelindex = gi.modelindex ("models/objects/masher/tris.md2");
	VectorCopy (start, hammer->s.origin);
	VectorCopy (start, hammer->s.old_origin);

	// set angles
	vectoangles (forward, hammer->s.angles);
	hammer->s.angles[ROLL] = crandom()*GetRandom(0, 20);
	VectorCopy(hammer->s.angles, hammer->move_angles); // controls spiral
	hammer->move_angles[PITCH] = 0;
	
	// set velocity
	VectorScale (forward, HAMMER_INITIAL_SPEED, hammer->velocity);

	hammer->clipmask = MASK_SHOT;
	hammer->solid = SOLID_BBOX;
	hammer->s.effects |= EF_COLOR_SHELL;
	hammer->s.renderfx |= (RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE);
    hammer->s.sound = gi.soundindex("abilities/hammerspin.wav");

	//Talent: Boomerang
	if (boomerang_level > 0)
	{
		hammer->activator = ent;
		hammer->touch = hammer_touch1;
		hammer->think = hammer_think1;
		ent->num_hammers++;

		// framenum when the projectile reverses course

		hammer->monsterinfo.nextattack = (int)(level.framenum + 1 / FRAMETIME);
		hammer->movetype = MOVETYPE_WALLBOUNCE;
	}
	else
	{
		hammer->owner = ent;
		hammer->touch = hammer_touch;
		hammer->think = hammer_think;
		hammer->count = HAMMER_TURN_RATE; // adjust this to make spiral bigger or smaller
		hammer->movetype = MOVETYPE_FLYMISSILE;
	}

	hammer->speed = HAMMER_INITIAL_SPEED;
	hammer->nextthink = level.time + FRAMETIME;
	hammer->dmg = HAMMER_INITIAL_DAMAGE+HAMMER_ADDON_DAMAGE*ent->myskills.abilities[HAMMER].current_level;
	hammer->classname = "hammer";	
	gi.linkentity (hammer);

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

    gi.sound(ent, CHAN_WEAPON, gi.soundindex("abilities/hammerlaunch.wav"), 1, ATTN_NORM, 0);

	ent->client->ability_delay = level.time + HAMMER_DELAY;
	ent->client->pers.inventory[power_cube_index] -= HAMMER_COST;

	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}

void Cmd_BlessedHammer_f (edict_t *ent)
{
	if (!G_CanUseAbilities(ent, ent->myskills.abilities[HAMMER].current_level, HAMMER_COST))
		return;
	if (ent->myskills.abilities[HAMMER].disable)
		return;

	SpawnBlessedHammer(ent, 0);
}

//Talent: Boomerang
void Cmd_Boomerang_f (edict_t *ent)
{
	int talentLevel;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[HAMMER].current_level, HAMMER_COST))
		return;
	if (ent->myskills.abilities[HAMMER].disable)
		return;

    talentLevel = vrx_get_talent_level(ent, TALENT_BOOMERANG);
	if (talentLevel < 1)
		return;

	SpawnBlessedHammer(ent, talentLevel);
}
