#include "g_local.h"

#define LASERPLATFORM_INITIAL_HEALTH		0
#define LASERPLATFORM_ADDON_HEALTH			200
#define LASERPLATFORM_INITIAL_DURATION		9999.9
#define LASERPLATFORM_ADDON_DURATION		0
#define LASERPLATFORM_MAX_COUNT				2
#define LASERPLATFORM_COST					10.0

void RemoveLaserPlatform (edict_t *laserplatform)
{
	if (laserplatform->activator && laserplatform->activator->inuse)
		laserplatform->activator->num_laserplatforms--;

	laserplatform->think = BecomeTE;
	laserplatform->deadflag = DEAD_DEAD;
	laserplatform->takedamage = DAMAGE_NO;
	laserplatform->nextthink = level.time + FRAMETIME;
	laserplatform->svflags |= SVF_NOCLIENT;
	laserplatform->solid = SOLID_NOT;
	//gi.linkentity(laserplatform);
}

void laserplatform_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	RemoveLaserPlatform(self);
}

void laserplatform_think (edict_t *self)
{
	if (!G_EntIsAlive(self->activator) || level.time > self->delay)
	{
		RemoveLaserPlatform(self);
		return;
	}

	self->s.renderfx = RF_SHELL_GREEN;
	
	if (level.time + 10.0 > self->delay && level.time >= self->monsterinfo.attack_finished)
	{

		self->s.renderfx |= RF_SHELL_RED|RF_SHELL_BLUE; // flash white
		self->monsterinfo.attack_finished = level.time + 1.0;
	}

	self->nextthink = level.time + FRAMETIME;
}

edict_t *CreateLaserPlatform (edict_t *ent, int skill_level)
{
	edict_t *e;

	e = G_Spawn();
	e->activator = ent;
	e->think = laserplatform_think;
	e->nextthink = level.time + FRAMETIME;
	e->s.modelindex = gi.modelindex ("models/items/armor/effect/tris.md2");
	e->s.angles[PITCH] += 90;
	e->solid = SOLID_BBOX;
	e->movetype = MOVETYPE_NONE;
	//e->svflags |= SVF_MONSTER;
	e->clipmask = MASK_MONSTERSOLID;
	e->mass = 500;
	e->classname = "laser_platform";
	e->delay = level.time + LASERPLATFORM_INITIAL_DURATION + LASERPLATFORM_ADDON_DURATION * skill_level;
	e->takedamage = DAMAGE_AIM;
	e->health = e->max_health = LASERPLATFORM_INITIAL_HEALTH + LASERPLATFORM_ADDON_HEALTH * skill_level;
	e->monsterinfo.level = skill_level;
	e->die = laserplatform_die;
	e->touch = V_Touch;
	e->s.effects |= EF_COLOR_SHELL|EF_SPHERETRANS;
	e->s.renderfx |= RF_SHELL_GREEN;
	VectorSet(e->mins, -28, -28, -18);
	VectorSet(e->maxs, 58, 28, -16);
	e->mtype = M_LASERPLATFORM;

	//ent->cocoon = e;
	ent->num_laserplatforms++;

	return e;
}

void RemoveOldestLaserPlatform (edict_t *ent)
{
	edict_t *e, *oldest=NULL;

	for (e=g_edicts ; e < &g_edicts[globals.num_edicts]; e++)
	{
		// find all laser platforms that we own
		if (e && e->inuse && (e->mtype == M_LASERPLATFORM) && e->activator && e->activator->inuse && e->activator == ent)
		{
			// update pointer to the platform with the earliest expiration time
			if (!oldest || e->delay < oldest->delay)
				oldest = e;
		}
	}

	// remove it
	if (oldest)
		RemoveLaserPlatform(oldest);
}

void RemoveAllLaserPlatforms (edict_t *ent)
{
	edict_t *e;

	for (e=g_edicts ; e < &g_edicts[globals.num_edicts]; e++)
	{
		// find all laser platforms that we own
		if (e && e->inuse && (e->mtype == M_LASERPLATFORM) && e->activator && e->activator->inuse && e->activator == ent)
			RemoveLaserPlatform(e);
	}
}

void Cmd_CreateLaserPlatform_f (edict_t *ent)
{	
	int		*cubes = &ent->client->pers.inventory[power_cube_index];
    int talentLevel = vrx_get_talent_level(ent, TALENT_LASER_PLATFORM);
	vec3_t	start;
	edict_t *laserplatform;

	if (talentLevel < 1)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need to upgrade laser platform talent before you can use it.\n");
		return;
	}

	if (*cubes < LASERPLATFORM_COST)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need %d more power cubes to use this ability.\n", (int)(LASERPLATFORM_COST - *cubes));
		return;
	}

	// if we have reached our max, then remove the oldest one
	if (ent->num_laserplatforms + 1 > LASERPLATFORM_MAX_COUNT)
		RemoveOldestLaserPlatform(ent);

	laserplatform = CreateLaserPlatform(ent, talentLevel);

	if (!G_GetSpawnLocation(ent, 100, laserplatform->mins, laserplatform->maxs, start))
	{
		RemoveLaserPlatform(laserplatform);
		return;
	}

	VectorCopy(start, laserplatform->s.origin);
	gi.linkentity(laserplatform);

	ent->client->pers.inventory[power_cube_index] -= LASERPLATFORM_COST;
}