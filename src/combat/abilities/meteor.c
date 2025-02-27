#include "g_local.h"

void meteor_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t *e=NULL;

	// don't call this more than once
	if (self->solid == SOLID_NOT)
		return;

	self->solid = SOLID_NOT;

	// burn targets within explosion radius
	while ((e = findradius(e, self->s.origin, self->dmg_radius)) != NULL)
	{
		if (!G_ValidTarget(self, e, true, true))
			continue;
		burn_person(e, self->owner, (int)(self->dmg*0.1));
	}

	// deal radius damage
	T_RadiusDamage(self, self->owner, self->dmg, NULL, self->dmg_radius, MOD_METEOR);

	// create explosion effect
	gi.WriteByte (svc_temp_entity);
	if (self->waterlevel)
		gi.WriteByte (TE_ROCKET_EXPLOSION_WATER);
	else
		gi.WriteByte (TE_ROCKET_EXPLOSION);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PHS);

	// create flames near point of detonation
	SpawnFlames(self->owner, self->s.origin, 10, (int)(self->dmg*0.1), 100);

	// remove meteor entity
	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAMETIME;

    //gi.sound(self, CHAN_VOICE, gi.soundindex("abilities/meteorimpact.wav"), 1, ATTN_NORM, 0);
}

void meteor_think (edict_t *self)
{
	if (!G_EntIsAlive(self->owner) || (gi.pointcontents(self->s.origin) & MASK_SOLID))
	{
		self->think = G_FreeEdict;
		self->nextthink = level.time + FRAMETIME;
		self->touch = NULL;
		return;
	}

	self->nextthink = level.time + FRAMETIME;
}

void meteor_ready (edict_t *self)
{
	self->s.effects = EF_FLAG1|EF_COLOR_SHELL;
	self->s.renderfx = RF_SHELL_RED;
	// set meteor model
	self->s.modelindex = gi.modelindex ("models/flames/lavaball/tris.md2");
	// start moving
	VectorScale(self->movedir, self->speed, self->velocity);
	// set touch
	self->touch = meteor_touch;
	// switch to final think
	self->think = meteor_think;
	self->nextthink = level.time + FRAMETIME;
}

void fire_meteor (edict_t *self, vec3_t end, int damage, int radius, int speed)
{
	vec3_t start, forward;
	trace_t tr;
	edict_t *meteor;

	// calculate starting position at ceiling height
	VectorCopy(end, start);
	start[2] += METEOR_CEILING_HEIGHT;

	tr = gi.trace (end, NULL, NULL, start, NULL, MASK_SOLID);

	// abort if we get stuck or we don't have enough room
	if (tr.startsolid || fabs(start[2]-end[2]) < 64)
		return;

	// create meteor entity
	meteor = G_Spawn();
	meteor->speed = speed;
	//VectorScale (dir, speed, meteor->velocity);
	meteor->movetype = MOVETYPE_FLYMISSILE;
	meteor->clipmask = MASK_SHOT;
	meteor->solid = SOLID_BBOX;
	meteor->owner = self;
	meteor->think = meteor_ready;

	meteor->nextthink = level.time + 1.5;
	meteor->dmg = damage;
	meteor->dmg_radius = radius;
	meteor->classname = "meteor";

	// copy meteor starting origin
	VectorCopy (tr.endpos, meteor->s.origin);
	VectorCopy (tr.endpos, meteor->s.old_origin);
	gi.linkentity (meteor);

	// calculate vector to target
	VectorSubtract(end, start, forward);
	VectorNormalize(forward);

	vectoangles (forward, meteor->s.angles);
	VectorCopy(forward, meteor->movedir);

	// calling entity made a sound, used to alert monsters
	self->lastsound = level.framenum;

    gi.sound(meteor, CHAN_WEAPON, gi.soundindex("abilities/meteorlaunch_short.wav"), 1, ATTN_NORM, 0);
}

void MeteorAttack (edict_t *ent, int damage, int radius, int speed, float skill_mult, float cost_mult)
{
	//edict_t *meteor;
	vec3_t	start, end, offset, forward, right;
	trace_t	tr;

	damage *= skill_mult;

	// get start position for trace
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	// get end position for trace
	VectorMA(start, METEOR_RANGE, forward, end);

	tr = gi.trace (start, NULL, NULL, end, ent, MASK_SHOT);

	// make sure we're starting at the floor
	VectorCopy(tr.endpos, start);
	VectorCopy(start, end);
	end[2] -= 8192;
	tr = gi.trace (start, NULL, NULL, end, ent, MASK_SOLID);

	if (tr.fraction == 1)
	{
		safe_cprintf(ent, PRINT_HIGH, "Too far from target.\n");
		return;
	}

	VectorCopy(tr.endpos, end);

	if (distance(end, ent->s.origin) < radius+32)
		safe_cprintf(ent, PRINT_HIGH, "***** WARNING: METEOR TARGET TOO CLOSE! *****\n");

	/*
	
	// create meteor entity
	meteor = G_Spawn();
	meteor->speed = speed;
	//VectorScale (dir, speed, meteor->velocity);
	meteor->movetype = MOVETYPE_FLYMISSILE;
	meteor->clipmask = MASK_SHOT;
	meteor->solid = SOLID_BBOX;
	meteor->owner = ent;
	meteor->think = meteor_ready;
	meteor->nextthink = level.time + 2.0;
	meteor->dmg = damage;
	meteor->dmg_radius = radius;
	meteor->classname = "meteor";	

	// calculate starting position for meteor above target
	VectorCopy(end, start);
	start[2] += METEOR_CEILING_HEIGHT;
	tr = gi.trace (end, NULL, NULL, start, meteor, MASK_SOLID);
	VectorCopy(tr.endpos, start);

	if (tr.startsolid)
	{
		G_FreeEdict(meteor);
		return;
	}

	if (fabs(start[2]-end[2]) < 64)
	{
		safe_cprintf(ent, PRINT_HIGH, "Not enough room to spawn meteor.\n");
		G_FreeEdict(meteor);
		return;
	}

	// copy meteor starting origin
	VectorCopy (tr.endpos, meteor->s.origin);
	VectorCopy (tr.endpos, meteor->s.old_origin);
	gi.linkentity (meteor);

	// calculate vector to target
	VectorSubtract(end, start, forward);
	VectorNormalize(forward);

	vectoangles (forward, meteor->s.angles);
	VectorCopy(forward, meteor->movedir);
	*/

	fire_meteor(ent, end, damage, radius, speed);

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

    //gi.sound(meteor, CHAN_WEAPON, gi.soundindex("abilities/meteorlaunch_short.wav"), 1, ATTN_NORM, 0);
	ent->client->ability_delay = level.time + METEOR_DELAY/* * cost_mult*/;
	ent->client->pers.inventory[power_cube_index] -= METEOR_COST * cost_mult;

	// calling entity made a sound, used to alert monsters
	//ent->lastsound = level.framenum;
}

void Cmd_Meteor_f (edict_t *ent, float skill_mult, float cost_mult)
{
	int damage=METEOR_INITIAL_DMG+METEOR_ADDON_DMG*ent->myskills.abilities[METEOR].current_level;
	int speed=METEOR_INITIAL_SPEED+METEOR_ADDON_SPEED*ent->myskills.abilities[METEOR].current_level;
	int radius=METEOR_INITIAL_RADIUS+METEOR_ADDON_RADIUS*ent->myskills.abilities[METEOR].current_level;
	int	cost=METEOR_COST*cost_mult;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[METEOR].current_level, cost))
		return;
	if (ent->myskills.abilities[METEOR].disable)
		return;

	MeteorAttack(ent, damage, radius, speed, (skill_mult * vrx_get_synergy_mult(ent, METEOR)), cost_mult);

	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}
