#include "g_local.h"


void caltrops_remove (edict_t *self)
{
	if (self->owner && self->owner->inuse)
		self->owner->num_caltrops--;

	G_FreeEdict(self);
}

void caltrops_removeall (edict_t *ent)
{
	edict_t *e = NULL;

	while((e = G_Find(e, FOFS(classname), "caltrops")) != NULL) 
	{
		if (e && e->owner && (e->owner == ent))
			caltrops_remove(e);
	}

	ent->num_caltrops = 0;
}

void caltrops_think (edict_t *self)
{
	if ((level.time > self->delay) || !G_EntIsAlive(self->owner))
	{
		caltrops_remove(self);
		return;
	}

	self->nextthink = level.time + FRAMETIME;
}

void caltrops_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (G_EntExists(other) && !OnSameTeam(self->owner, other))
	{
		// 50% slowed at level 10 for 5 seconds
		other->slowed_factor = 1 / (1 + CALTROPS_INITIAL_SLOW + (CALTROPS_ADDON_SLOW * self->monsterinfo.level));
		other->slowed_time = level.time + (CALTROPS_INITIAL_SLOWED_TIME + (CALTROPS_ADDON_SLOWED_TIME * self->monsterinfo.level));

		T_Damage(other, self, self->owner, self->velocity, self->s.origin, 
			plane->normal, self->dmg, self->dmg, DAMAGE_NO_KNOCKBACK, MOD_CALTROPS);

		gi.sound (other, CHAN_BODY, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);
		
		caltrops_remove(self);
	}
}

void ThrowCaltrops (edict_t *self, vec3_t start, vec3_t forward, int slevel, float duration)
{
	edict_t *caltrops;

	caltrops = G_Spawn();
	VectorCopy(start, caltrops->s.origin);
	caltrops->movetype = MOVETYPE_TOSS;
	caltrops->owner = self;
	caltrops->monsterinfo.level = slevel;
	caltrops->dmg = CALTROPS_INITIAL_DAMAGE + CALTROPS_ADDON_DAMAGE * slevel;
	caltrops->classname = "caltrops";
	caltrops->think = caltrops_think;
	caltrops->touch = caltrops_touch;
	caltrops->nextthink = level.time + FRAMETIME;
	caltrops->delay = level.time + duration;
	VectorSet(caltrops->maxs, 8, 8, 8);
	caltrops->s.angles[PITCH] = -90;
	caltrops->solid = SOLID_TRIGGER;
	caltrops->clipmask = MASK_SHOT;
	caltrops->s.modelindex = gi.modelindex ("models/spike/tris.md2");
	gi.linkentity(caltrops);

	VectorScale(forward, 200, caltrops->velocity);

	self->client->pers.inventory[power_cube_index] -= CALTROPS_COST;
	self->client->ability_delay = level.time + CALTROPS_DELAY;
	self->num_caltrops++;

	safe_cprintf(self, PRINT_HIGH, "Caltrops deployed: %d/%d\n", self->num_caltrops, CALTROPS_MAX_COUNT);

	//  entity made a sound, used to alert monsters
	self->lastsound = level.framenum;
}

void Cmd_Caltrops_f (edict_t *ent)
{
	vec3_t forward, start;

	if (!Q_strcasecmp(gi.args(), "remove"))
	{
		caltrops_removeall(ent);
		safe_cprintf(ent, PRINT_HIGH, "All caltrops removed.\n");
		return;
	}

	if (!V_CanUseAbilities(ent, CALTROPS, CALTROPS_COST, true))
		return;

	if (ent->num_caltrops >= CALTROPS_MAX_COUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "You've reached the maximum number of caltrops (%d).\n", CALTROPS_MAX_COUNT);
		return;
	}

	VectorCopy(ent->s.origin, start);
	start[2] += ent->viewheight - 8;
	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	VectorMA(start, 24, forward, start);
	ThrowCaltrops(ent, start, forward, ent->myskills.abilities[CALTROPS].current_level, CALTROPS_DURATION);
}