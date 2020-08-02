#include "g_local.h"
#include "../../gamemodes/ctf.h"


void detector_remove (edict_t *self)
{
	if (self->owner && self->owner->inuse)
	{
		self->owner->num_detectors--;
		safe_cprintf(self->owner, PRINT_HIGH, "%d/%d detectors remaining\n", self->owner->num_detectors, DETECTOR_MAX_COUNT);
	}

	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_NOCLIENT;
	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->deadflag = DEAD_DEAD;
}

void detector_removeall (edict_t *ent)
{
	edict_t *e = NULL;

	while((e = G_Find(e, FOFS(classname), "detector")) != NULL) 
	{
		if (e && e->owner && (e->owner == ent))
			detector_remove(e);
	}

	ent->num_detectors = 0;
}

edict_t *detector_findprojtarget (edict_t *self, edict_t *projectile)
{
	edict_t *target=NULL;

	// find enemy that is closest to the projectile
	while ((target = findclosestradius_targets(target, projectile, self->dmg_radius)) != NULL)
	{
		// valid target must be within range of the detector
		if (G_ValidTarget_Lite(self, target, true) && entdist(self, target) < self->dmg_radius)
			return target;
	}
	return NULL;
}

void ProjectileLockon (edict_t *proj)
{
	vec3_t forward, start;

	G_EntMidPoint(proj->enemy, start);
	VectorSubtract(start, proj->s.origin, forward);
	VectorNormalize(forward);
	VectorCopy (forward, proj->movedir);
	vectoangles (forward, proj->s.angles);
	VectorScale (forward, VectorLength(proj->velocity), proj->velocity);
}

void detector_findprojectile (edict_t *self, char *className)
{
	edict_t *e=NULL, *proj_target;

	while((e = G_Find(e, FOFS(classname), className)) != NULL)
	{
		// only use friendly projectiles
		if (e->owner && e->owner->inuse && !OnSameTeam(self->owner, e->owner))
			continue;

		// find a projectile that is within range of the detector
		if (entdist(self, e) > self->dmg_radius)
			continue;
	
		// this projectile already has a target that is within range
		if (e->enemy && e->enemy->inuse && entdist(self, e->enemy) < self->dmg_radius)
		{
			// lock-on to enemy
			ProjectileLockon(e);
			continue;
		}
		
		// find a new target for the projectile
		if ((proj_target = detector_findprojtarget(self, e)) != NULL)
		{
			e->enemy = proj_target;
			ProjectileLockon(e);
		}
	}
}

qboolean detector_findtarget (edict_t *self)
{
	qboolean	foundTarget=false;
	edict_t		*target=NULL;

	while ((target = findradius(target, self->s.origin, self->dmg_radius)) != NULL)
	{
		// sanity check
		if (!target || !target->inuse || !target->takedamage || target->solid == SOLID_NOT)
			continue;
		// don't target anything dead
		if (target->deadflag == DEAD_DEAD || target->health < 1)
			continue;
		// don't target spawning players
		if (target->client && (target->client->respawn_time > level.time))
			continue;
		// don't target players in chat-protect
		if (!ptr->value && target->client && (target->flags & FL_CHATPROTECT))
			continue;
		if (target->flags & FL_GODMODE)
			continue;
		// don't target spawning world monsters
		if (target->activator && !target->activator->client && (target->svflags & SVF_MONSTER) 
			&& (target->deadflag != DEAD_DEAD) && (target->nextthink-level.time > 2*FRAMETIME))
			continue;
		// don't target teammates
		if (OnSameTeam(self->owner, target))
			continue;
		// visiblity check
		if (!visible(self, target))
			continue;

		// flag them as detected
		target->flags |= FL_DETECTED;
		target->detected_time = level.time + DETECTOR_FLAG_DURATION;
		foundTarget = true;

		// decloak them
		if (target->client)
		{
			target->client->idle_frames = 0;
			target->client->cloaking = false;
		}

		target->svflags &= ~SVF_NOCLIENT;
	}

	return foundTarget;
}

void detector_effects (edict_t *self)
{
	// team colors
	self->s.effects |= EF_COLOR_SHELL;
	if (self->owner->teamnum == BLUE_TEAM)
		self->s.renderfx |= RF_SHELL_BLUE;
	else if (self->owner->teamnum == RED_TEAM)
		self->s.renderfx |= RF_SHELL_RED;
	else if (self->monsterinfo.attack_finished > level.time)
		self->s.renderfx |= RF_SHELL_YELLOW;
	else if (!(sf2qf(level.framenum) & 8))
		self->s.renderfx |= RF_SHELL_RED;
	else
		self->s.effects = self->s.renderfx = 0;
}

void detector_think (edict_t *self)
{
	qboolean expired=false;

	if (level.time > self->delay)
		expired = true;

	if (expired || !G_EntIsAlive(self->owner))
	{
		if (expired && self->owner && self->owner->inuse)
			safe_cprintf(self->owner, PRINT_HIGH, "A detector timed-out. (%d/%d)\n", self->owner->num_detectors, DETECTOR_MAX_COUNT);

		detector_remove(self);
		return;
	}

	if (detector_findtarget(self))
	{
		// play a sound to alert players
		if (level.time > self->sentrydelay)
		{
			gi.sound(self->owner, CHAN_VOICE, gi.soundindex("detector/alarm3.wav"), 1, ATTN_NORM, 0);
			self->sentrydelay = level.time + 1.0;
		}

		// glow for awhile
		self->monsterinfo.attack_finished = level.time + DETECTOR_GLOW_TIME;
	}

	detector_effects(self);

	// find projectiles that are within range and redirect them towards nearest enemy
	if (self->mtype == M_DETECTOR)
	{
		detector_findprojectile(self, "rocket");
		detector_findprojectile(self, "bolt");
		detector_findprojectile(self, "bfg blast");
		detector_findprojectile(self, "magicbolt");
		detector_findprojectile(self, "grenade");
		detector_findprojectile(self, "hgrenade");
		detector_findprojectile(self, "skull");
		detector_findprojectile(self, "spike");
		detector_findprojectile(self, "spikey");
		detector_findprojectile(self, "fireball");
		detector_findprojectile(self, "plasma bolt");
		detector_findprojectile(self, "acid");
	}

	self->nextthink = level.time + FRAMETIME;
}

void detector_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	safe_cprintf(self->owner, PRINT_HIGH, "A detector was destroyed. (%d/%d)\n", self->owner->num_detectors, DETECTOR_MAX_COUNT);
	detector_remove(self);
}

void BuildDetector (edict_t *self, vec3_t start, vec3_t forward, int slvl, float duration, int cost, float delay_mult)
{
	edict_t *detector;
	trace_t	tr;
	vec3_t	end;

	detector = G_Spawn();
	VectorSet (detector->mins, -4, -4, -4);
	VectorSet (detector->maxs, 4, 4, 4);
	VectorCopy(start, detector->s.origin);
	detector->movetype = MOVETYPE_NONE;
	detector->owner = self;
	detector->monsterinfo.level = slvl;

	detector->classname = "detector";
	detector->think = detector_think;
	detector->nextthink = level.time + FRAMETIME;
	detector->solid = SOLID_BBOX;
	detector->takedamage = DAMAGE_AIM;
	
	detector->die = detector_die;
	detector->clipmask = MASK_SHOT;

	//Talent: Alarm
	if (duration)
	{
		detector->mtype = M_DETECTOR;
		detector->health = detector->max_health = DETECTOR_INITIAL_HEALTH + DETECTOR_ADDON_HEALTH * slvl;
		detector->delay = level.time + duration;
		detector->dmg_radius = DETECTOR_INITIAL_RANGE + DETECTOR_ADDON_RANGE * slvl;
	}
	else
	{
		detector->mtype = M_ALARM;
		detector->flags |= FL_NOTARGET;
		detector->health = ALARM_INITIAL_HEALTH + ALARM_ADDON_HEALTH * slvl;
		detector->delay = level.time + 9999.0;
		detector->dmg_radius = ALARM_INITIAL_RANGE + ALARM_ADDON_RANGE * slvl;
	}

	detector->s.modelindex = gi.modelindex ("models/objects/detector/tris.md2");

	// get end position
	VectorMA(start, 64, forward, end);

	tr = gi.trace (start, detector->mins, detector->maxs, end, self, MASK_SOLID);

	// can't build on a sky brush
	if (tr.surface && (tr.surface->flags & SURF_SKY))
	{
		G_FreeEdict(detector);
		return;
	}

	if (tr.fraction == 1)
	{
		G_FreeEdict(detector);
		safe_cprintf(self, PRINT_HIGH, "Too far from wall.\n");
		return;
	}

	VectorCopy(tr.endpos, detector->s.origin);
	VectorCopy(tr.endpos, detector->s.old_origin);
	vectoangles(tr.plane.normal, detector->s.angles);
	detector->s.angles[PITCH] += 90;
	AngleCheck(&detector->s.angles[PITCH]);
	gi.linkentity(detector);

	self->num_detectors++;
	self->client->ability_delay = self->holdtime = level.time + DETECTOR_DELAY * delay_mult;
	self->client->pers.inventory[power_cube_index] -= cost;

	//  entity made a sound, used to alert monsters
	self->lastsound = level.framenum;
}

void Cmd_Detector_f (edict_t *ent)
{
	float skill_mult=1.0, cost_mult=1.0, delay_mult=1.0;
	int cost=DETECTOR_COST, talentLevel;
	vec3_t forward, start;

	if (Q_strcasecmp (gi.args(), "remove") == 0)
	{
		safe_cprintf(ent, PRINT_HIGH, "All detectors removed.\n");
		detector_removeall(ent);
		lasertrap_removeall(ent, true);//4.4 Talent: Alarm
		return;
	}

	//Talent: Rapid Assembly
    talentLevel = vrx_get_talent_level(ent, TALENT_RAPID_ASSEMBLY);
	if (talentLevel > 0)
		delay_mult -= 0.1 * talentLevel;
	//Talent: Precision Tuning
    else if ((talentLevel = vrx_get_talent_level(ent, TALENT_PRECISION_TUNING)) > 0)
	{
		cost_mult += PRECISION_TUNING_COST_FACTOR * talentLevel;
		delay_mult += PRECISION_TUNING_DELAY_FACTOR * talentLevel;
		skill_mult += PRECISION_TUNING_SKILL_FACTOR * talentLevel;
	}
	cost *= cost_mult;

	if (!V_CanUseAbilities(ent, DETECTOR, DETECTOR_COST, true))
		return;

	if (ent->num_detectors >= DETECTOR_MAX_COUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "You've reached the maximum number of detectors (%d)\n", DETECTOR_MAX_COUNT);
		return;
	}

	if (ctf->value && (CTF_DistanceFromBase(ent, NULL, CTF_GetEnemyTeam(ent->teamnum)) < CTF_BASE_DEFEND_RANGE))
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't build in enemy base!\n");
		return;
	}

	VectorCopy(ent->s.origin, start);
	start[2] += ent->viewheight - 8;
	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	//VectorMA(start, 24, forward, start);
	BuildDetector(ent, start, forward, (int)(ent->myskills.abilities[DETECTOR].current_level * skill_mult), DETECTOR_DURATION, cost, delay_mult);
}
