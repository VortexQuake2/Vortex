#include "../../quake2/g_local.h"

void mirrored_remove (edict_t *self)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	if (self->activator && self->activator->inuse)
	{
		if (self->activator->mirror1 == self)
			self->activator->mirror1 = NULL;
			
		else if (self->activator->mirror2 == self)
			self->activator->mirror2 = NULL;
	}

	self->think = BecomeTE;
	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_NOCLIENT;
	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->deadflag = DEAD_DEAD;
}

void mirrored_removeall (edict_t *ent)
{
	if (ent->mirror1)
		mirrored_remove(ent->mirror1);
	if (ent->mirror2)
		mirrored_remove(ent->mirror2);
}

void UnGhostMirror (edict_t *self)
{
	self->svflags &= ~SVF_NOCLIENT;
	self->solid = SOLID_BBOX;
	self->takedamage = DAMAGE_YES;
}

void GhostMirror (edict_t *self)
{
	self->svflags |= SVF_NOCLIENT;
	self->solid = SOLID_NOT;
	self->takedamage = DAMAGE_NO;
}

qboolean UpdateMirroredEntity (edict_t *self, float dist)
{
	vec3_t	angles;
	vec3_t	forward, right, start, end;
	trace_t	tr;
	edict_t *target;
	
	if (!self->activator || !self->activator->inuse || !self->activator->client)
		return false;

	if (PM_PlayerHasMonster(self->activator))
	{
		target = self->activator->owner;
		self->owner = target;
	}
	else
	{
		target = self->activator;
		self->owner = self->activator;
	}

	// copy current animation frame
	self->s.frame = target->s.frame;

	if (level.framenum >= self->count)
	{
		// copy owner's bounding box
		VectorCopy(target->mins, self->mins);
		VectorCopy(target->maxs, self->maxs);

		// copy yaw for position calculation
		VectorCopy(target->s.angles, angles);
		angles[YAW] = target->s.angles[YAW];
		angles[PITCH] = 0;
		angles[ROLL] = 0;

		AngleVectors(angles, NULL, right, NULL);

		// calculate starting position
		if (self->activator->mtype == MORPH_FLYER || self->activator->mtype == MORPH_CACODEMON)
		{
			// trace sideways
			VectorMA(self->owner->s.origin, dist, right, end);
			tr = gi.trace(self->owner->s.origin, NULL, NULL, end, target, MASK_SOLID);
		}
		else
		{
			VectorCopy(target->s.origin, start);
			VectorCopy(start, end);
			end[2] += self->maxs[2];
			// trace up
			tr = gi.trace(start, NULL, NULL, end, target, MASK_SOLID);
			VectorMA(tr.endpos, dist, right, end);
			// trace sideways
			tr = gi.trace(start, NULL, NULL, end, target, MASK_SOLID);
			VectorCopy(tr.endpos, start);
			VectorCopy(start, end);
			end[2] = target->absmin[2];
			// trace down
			tr = gi.trace(start, self->mins, self->maxs, end, target, MASK_SHOT);
			// no floor
			if ((tr.fraction == 1.0) && target->groundentity)
				return false;
			
			
		}

		VectorCopy(tr.endpos, end);
		// check final position
		tr = gi.trace(end, self->mins, self->maxs, end, target, MASK_SHOT);
		if (tr.fraction < 1 && (tr.ent != self->owner->mirror1) && (tr.ent != self->owner->mirror2))
			return false;
		
		// update position
		VectorCopy(end, self->s.origin);
		gi.linkentity(self);

		// copy everything from our owner
		self->model = target->model;
		self->s.skinnum = target->s.skinnum;
		self->s.modelindex = target->s.modelindex;
		self->s.modelindex2 = target->s.modelindex2;
		self->s.effects = target->s.effects;
		self->s.renderfx = target->s.renderfx;

		// set angles to point where our owner is pointing
		VectorCopy(target->s.origin, start);
		start[2] += self->activator->viewheight - 8;
		AngleVectors(self->activator->client->v_angle, forward, NULL, NULL);
		VectorMA(start, 8192, forward, end);
		tr = gi.trace (start, NULL, NULL, end, self, MASK_SHOT);
		VectorSubtract(tr.endpos, start, forward);
		vectoangles(forward, self->s.angles);
		self->s.angles[PITCH] = target->s.angles[PITCH];

		self->count = (int)(level.framenum + 0.1 / FRAMETIME);
	}

	// we have a valid position for this entity, so make it visible
	UnGhostMirror(self);
	return true;
}

#define		MIRROR_POSITION_MIDDLE		1
#define		MIRROR_POSITION_LEFT		2
#define		MIRROR_POSITION_RIGHT		3

qboolean MirroredEntitiesExist (edict_t *ent)
{
	//return (G_EntExists(ent->mirror1) && G_EntExists(ent->mirror2));
	return ent->mirror1 && ent->mirror1->inuse && ent->mirror2 && ent->mirror2->inuse;
}

void UpdateMirroredEntities (edict_t *ent)
{
	int		i, pos;
	float	dist;

	if (!MirroredEntitiesExist(ent))
		return;

	// randomize desired position to fool opponents
	if (!(level.framenum% (int)(5 / FRAMETIME)))
		ent->mirroredPosition = GetRandom(1, 3);

	pos = ent->mirroredPosition;

	// calculate distance from owner
	if (PM_PlayerHasMonster(ent))
		dist = (2*ent->owner->maxs[1])+8;
	else
		dist = (2*ent->maxs[1])+8;

	for (i=0; i<3; i++)
	{
		// try middle position
		if ((pos == MIRROR_POSITION_MIDDLE) && UpdateMirroredEntity(ent->mirror1, dist)
			&& UpdateMirroredEntity(ent->mirror2, -dist))
			return;
		// try left position
		else if ((pos == MIRROR_POSITION_LEFT) && UpdateMirroredEntity(ent->mirror1, -dist)
			&& UpdateMirroredEntity(ent->mirror2, -2*dist))
			return;
		// try right
		else if ((pos == MIRROR_POSITION_RIGHT) && UpdateMirroredEntity(ent->mirror1, dist)
			&& UpdateMirroredEntity(ent->mirror2, 2*dist))
			return;
		pos++;
		if (pos > MIRROR_POSITION_RIGHT)
			pos = MIRROR_POSITION_MIDDLE;
		ent->mirroredPosition = pos;
	}

	// we couldn't find a valid position, so hide them
	GhostMirror(ent->mirror1);
	GhostMirror(ent->mirror2);
}

void mirrored_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->activator && self->activator->inuse && self->deadflag != DEAD_DEAD)
	{
		if (self->activator->client)
			safe_cprintf(self->activator, PRINT_HIGH, "Your decoy was killed.\n");
		mirrored_removeall(self->activator);
	}
}

void mirrored_think (edict_t *self)
{
	if (!G_EntIsAlive(self->activator))
	{
		mirrored_remove(self);
		return;
	}

	self->nextthink = level.time + FRAMETIME;

}

char *V_GetClassSkin (edict_t *ent);

edict_t *MirrorEntity (edict_t *ent)
{
	edict_t *e;

	e = G_Spawn();
	e->svflags |= SVF_MONSTER;
	e->classname = "mirrored";
	e->activator = e->owner = ent;
	e->takedamage = DAMAGE_YES;
	e->health = e->max_health = MIRROR_INITIAL_HEALTH + MIRROR_ADDON_HEALTH * ent->myskills.level;
	e->mass = 200;
	e->clipmask = MASK_MONSTERSOLID;
	e->movetype = MOVETYPE_NONE;
	e->s.renderfx |= RF_IR_VISIBLE;
//	e->flags |= FL_CHASEABLE;
	e->solid = SOLID_BBOX;
	e->think = mirrored_think;
	e->die = mirrored_die;
	e->mtype = M_MIRROR;
	e->touch = V_Touch;
	VectorCopy(ent->mins, e->mins);
	VectorCopy(ent->maxs, e->maxs);
	e->nextthink = level.time + FRAMETIME;

	return e;
}