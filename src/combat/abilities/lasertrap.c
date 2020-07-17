#include "g_local.h"
#include "../../gamemodes/ctf.h"


void lasertrap_remove (edict_t *self)
{
	// reset counter
	if (self->activator && self->activator->inuse)
		self->activator->num_detectors--;

	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_NOCLIENT;
	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->deadflag = DEAD_DEAD;
}

void lasertrap_removeall (edict_t *ent, qboolean effect)
{
	edict_t *e;

	if (!ent || !ent->inuse)
		return;

	// remove all traps
	for (e=g_edicts ; e < &g_edicts[globals.num_edicts]; e++)
	{
		// find all laser traps that we own
		if (e && e->inuse && (e->mtype == M_ALARM) && e->activator 
			&& e->activator->inuse && e->activator == ent)
		{
			if (effect && e->solid != SOLID_NOT)
			{
				// explosion effect
				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_EXPLOSION1);
				gi.WritePosition (e->s.origin);
				gi.multicast (e->s.origin, MULTICAST_PVS);
			}

			lasertrap_remove(e);
		}
	}

	ent->num_detectors = 0; // reset counter
}

void lasertrap_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// explosion effect
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	lasertrap_remove(self);

	safe_cprintf(self->activator, PRINT_HIGH, "A laser trap was destroyed (%d remain).\n", self->activator->num_detectors);

}

qboolean lasertrap_validtarget (edict_t *self, edict_t *target, float dist, qboolean vis)
{
	vec3_t v;

	// basic check
	if (!G_ValidTarget(self, target, vis))
		return false;

	// check distance on a 2-D plane
	VectorSubtract(target->s.origin, self->s.origin, v);
	v[2] = 0;
	if (VectorLength(v) > dist)
		return false; // do not exceed maximum laser distance

	return true;
}

void lasertrap_findtarget (edict_t *self)
{
	edict_t *e=NULL;

	while ((e = findradius(e, self->s.origin, 256.0)) != NULL)//FIXME: should do a better search
	{
		if (!lasertrap_validtarget(self, e, LASERTRAP_RANGE, true))
			continue;
		self->enemy = e;
		break;
	}

	// if we are idle with no enemy, then become invisible to AI
	if (!self->enemy)
		self->flags |= FL_NOTARGET;
}

void lasertrap_firelaser (edict_t *self, vec3_t dir)
{
	vec3_t	start, end, forward;
	trace_t	tr;

	// trace ahead, stopping at any wall
	VectorCopy(self->s.origin, start);
	VectorMA(start, self->random, dir, end);
	tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SOLID);
	if (tr.fraction < 1)
	{
		// touched a wall, reduce maximum distance
		self->random = distance(tr.endpos, start);
		// too short, abort
		if (self->random < LASERTRAP_MINIMUM_RANGE)
		{
			lasertrap_remove(self);
			if (self->activator && self->activator->inuse)
				safe_cprintf(self->activator, PRINT_HIGH, "Lasertrap self-destructed. Too close to wall.\n");
			return;
		}
	}

	// trace down 32 units to the floor
	VectorCopy(tr.endpos, start);
	VectorCopy(tr.endpos, end);
	end[2] -= 32;
	tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SOLID);
	if (tr.fraction == 1.0) // too far down, abort
		return;

	// copy floor position as starting point, then trace up
	//tr.endpos[2]+=24;
	VectorCopy(tr.endpos, start);
	VectorCopy(tr.endpos, end);
	end[2] += 128;
	tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SHOT);

	// bfg laser effect
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BFG_LASER);
	gi.WritePosition (start);
	gi.WritePosition (tr.endpos);
	gi.multicast (self->s.origin, MULTICAST_PHS);

	// throw sparks
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_LASER_SPARKS);
	gi.WriteByte(1); // number of sparks
	gi.WritePosition(start);
	gi.WriteDir(vec3_origin);
	gi.WriteByte(209); // color
	gi.multicast(start, MULTICAST_PVS);

	// hurt any valid target that touches the laser
	if (G_ValidTarget(self, tr.ent, false))
	{
		// calculate attack vector
		VectorSubtract(tr.ent->s.origin, start, forward);// up
		VectorNormalize(forward);

		// hurt them
		T_Damage(tr.ent, self, self, forward, tr.endpos, vec3_origin, self->dmg, 0, DAMAGE_ENERGY, MOD_LASER_DEFENSE);//FIXME: change mod
	}
}

void lasertrap_attack (edict_t *self)
{
	vec3_t angles, forward, right;
	
	VectorCopy(self->s.angles, angles);
	AngleVectors(angles, forward, right, NULL);

	// activate first laser directly ahead
	lasertrap_firelaser(self, forward);

	// activate second laser to the right
	lasertrap_firelaser(self, right);

	// rotate angles 180 degrees
	angles[YAW] += 180;
	AngleCheck(&angles[YAW]);
	AngleVectors(angles, forward, right, NULL);

	// activate second laser behind
	lasertrap_firelaser(self, forward);

	// activate second laser to the left
	lasertrap_firelaser(self, right);
	
	self->flags &= ~FL_NOTARGET; // become visible to AI
	self->s.angles[YAW] += self->yaw_speed; // rotate
	self->random--;// slowly move closer to emitter

	// play a sound to alert players
	if (level.time > self->sentrydelay)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("detector/alarm3.wav"), 1, ATTN_NORM, 0);
		self->sentrydelay = level.time + 1.0;
	}
}

void lasertrap_effects (edict_t *self)
{
	edict_t *cl_ent;
	self->s.effects = self->s.renderfx = 0;

	// display shell if we have an enemy or after a delay
	if (self->enemy || !(level.framenum % 20))
	{
		// make sure
		if ((cl_ent = G_GetClient(self)) != NULL)
		{
			self->s.effects |= EF_COLOR_SHELL;

			// glow red if we are hostile against players or our activator is on the red team
			if (cl_ent->teamnum == RED_TEAM || ffa->value)
				self->s.renderfx = RF_SHELL_RED;
			else
				self->s.renderfx = RF_SHELL_BLUE;
		}
	}
}

void lasertrap_think (edict_t *self)
{
	// remove if owner dies or becomes invalid
	if (!G_EntIsAlive(self->activator))
	{
		lasertrap_remove(self);
		return;
	}

	// if enemy is valid and in-range, then attack
	if (lasertrap_validtarget(self, self->enemy, LASERTRAP_RANGE+8, false))
		lasertrap_attack(self);
	// if we had an enemy, reset
	else if (self->enemy)
	{
		self->enemy = NULL;
		self->random = 64;// reset maximum laser range
	}
	// find a new target
	else
		lasertrap_findtarget(self);

	lasertrap_effects(self);

	self->nextthink = level.time + FRAMETIME;
}

void lasertrap_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	V_Touch(ent, other, plane, surf);

	if (G_EntIsAlive(other) && other->client && OnSameTeam(ent, other) // a player on our team
		&& other->client->pers.inventory[power_cube_index] >= 5 // has power cubes
		&& ent->health < ent->max_health // need repair
		&& level.framenum > ent->monsterinfo.regen_delay1) // check delay
	{
		ent->health = ent->max_health;// restore health
		other->client->pers.inventory[power_cube_index] -= 5;
		safe_cprintf(other, PRINT_HIGH, "Laser trap repaired (%d/%dh).\n", ent->health, ent->max_health);
		gi.sound(other, CHAN_VOICE, gi.soundindex("weapons/repair.wav"), 1, ATTN_NORM, 0);
		ent->monsterinfo.regen_delay1 = (int)(level.framenum + 2 / FRAMETIME);// delay before we can rearm
	}
}

void ThrowLaserTrap (edict_t *self, vec3_t start, vec3_t aimdir, int skill_level)
{
	edict_t *trap;
	vec3_t	forward, right, up, dir;

	// get aiming angles
	vectoangles(aimdir, dir);
	// get directional vectors
	AngleVectors(dir, forward, right, up);

	trap = G_Spawn();
	VectorSet (trap->mins, -4, -4, -2);
	VectorSet (trap->maxs, 4, 4, 2);
	trap->movetype = MOVETYPE_TOSS;
	trap->activator = self;
	trap->monsterinfo.level = skill_level;

	trap->classname = "lasertrap";
	trap->think = lasertrap_think;
	trap->nextthink = level.time + FRAMETIME;
	trap->solid = SOLID_BBOX;
	trap->takedamage = DAMAGE_AIM;
	trap->touch = lasertrap_touch;
	
	trap->die = lasertrap_die;
	trap->clipmask = MASK_SHOT;
	trap->mtype = M_ALARM;//FIXME: change this?
	trap->random = LASERTRAP_RANGE;// maximum laser range
	trap->yaw_speed = LASERTRAP_YAW_SPEED;// rotation speed
	trap->health = trap->max_health = LASERTRAP_INITIAL_HEALTH + LASERTRAP_ADDON_HEALTH * skill_level;
	trap->dmg = LASERTRAP_INITIAL_DAMAGE + LASERTRAP_ADDON_DAMAGE * skill_level;
	trap->delay = level.time + 1.0; // activation time

	trap->s.modelindex = gi.modelindex ("models/objects/detector/tris.md2");

	VectorCopy (start, trap->s.origin);
	gi.linkentity (trap);

	// increment number of detectors/traps, set delay, and use power cubes
	self->num_detectors++;
	self->client->ability_delay = level.time + LASERTRAP_DELAY;
	self->client->pers.inventory[power_cube_index] -= LASERTRAP_COST;

	// throw
	VectorScale (aimdir, 400, trap->velocity);
	VectorMA (trap->velocity, 200 + crandom() * 10.0, up, trap->velocity);
	VectorMA (trap->velocity, crandom() * 10.0, right, trap->velocity);

	//  entity made a sound, used to alert monsters
	self->lastsound = level.framenum;
}


void Cmd_LaserTrap_f (edict_t *ent)
{
    int talentLevel = vrx_get_talent_level(ent, TALENT_ALARM);
	vec3_t	forward, start;

	if (Q_strcasecmp (gi.args(), "remove") == 0)
	{
		safe_cprintf(ent, PRINT_HIGH, "All detectors and laser traps removed.\n");
		detector_removeall(ent);
		lasertrap_removeall(ent, false);
		return;
	}

	if (!V_CanUseAbilities(ent, DETECTOR, LASERTRAP_COST, true))
		return;

	if (talentLevel < 1)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need to upgrade laser trap before you can use it.\n");
		return;
	}

	if (ent->num_detectors >= DETECTOR_MAX_COUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "You've reached the maximum number of laser traps.\n");
		return;
	}
	
	VectorCopy(ent->s.origin, start);
	start[2] += ent->viewheight - 8;
	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	ThrowLaserTrap(ent, start, forward, talentLevel);
}