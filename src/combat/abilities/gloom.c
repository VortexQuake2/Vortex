#include "g_local.h"
#include "../../gamemodes/ctf.h"

static int poison_sound1;
static int poison_sound2;
static int poison_sound3;
static int poison_sound4;
static int sound_poisoned;
static int sound_poisonnova;
static int sound_gas;

// az's note for anyone who looks at this in the future: for organ_touch
void V_Push (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	float maxvel = 300;

	// our activator or ally can push us
	if (other && other->inuse && other->client && self->activator && self->activator->inuse 
		&& (other == self->activator || IsAlly(self->activator, other)))
	{
		vec3_t forward, right, offset, start;

		// don't push if we are standing on this entity
		if (other->groundentity && other->groundentity == self)
			return;
		
		if (self->movetype != MOVETYPE_STEP) // needed by obstacle which uses MOVETYPE_NONE to stay glued to the ground
		{
			self->movetype_prev = self->movetype;
			self->movetype_frame = (int)(level.framenum + 0.5 / FRAMETIME);
			self->movetype = MOVETYPE_STEP;
			//self->touch_debounce_time = level.time + 0.5;
			//gi.dprintf("%f %f\n", level.time, self->touch_debounce_time);
		}

		AngleVectors (other->client->v_angle, forward, right, NULL);
		VectorScale (forward, -3, other->client->kick_origin);
		VectorSet(offset, 0, 7,  other->viewheight-8);
		P_ProjectSource (other->client, other->s.origin, offset, forward, right, start);

		VectorMA(self->velocity, 50, forward, self->velocity);

		// cap maximum velocity
		if (self->velocity[0] > maxvel)
			self->velocity[0] = maxvel;
		if (self->velocity[0] < -maxvel)
			self->velocity[0] = -maxvel;
		if (self->velocity[1] > maxvel)
			self->velocity[1] = maxvel;
		if (self->velocity[1] < -maxvel)
			self->velocity[1] = -maxvel;
		if (self->velocity[2] > maxvel)
			self->velocity[2] = maxvel;
		if (self->velocity[2] < -maxvel)
			self->velocity[2] = -maxvel;
	}
}

qboolean vrx_attach_wall (edict_t* ent, edict_t* other, cplane_t* plane)
{
	vec3_t angles;
	float pitch, save;

	if (ent->mtype != M_GASSER && ent->mtype != M_HEALER)
		return false; // not an entity that can be attached to the wall
	if (ent->groundentity)
	{
		if (ent->s.angles[PITCH] == 90)
		{
			//gi.dprintf("touched ground, flip upright\n");
			// flip upright
			ent->s.angles[PITCH] = 0;
			//save = ent->mins[2];
			//ent->mins[2] = ent->maxs[2] * -1;
			//ent->maxs[2] = save * -1;
			// adjust the position
			//ent->s.origin[2] -= ent->maxs[2];
			gi.linkentity(ent);
		}
		return false; // on the ground
	}
	if (ent->s.angles[PITCH] == 90)
		return false; // already flipped
	if (!plane)
		return false; // not touching a plane

	// calculate the pitch angle of the plane, 270 = ceiling, 90 = floor, 0 = wall
	vectoangles(plane->normal, angles);
	pitch = angles[PITCH] * -1;

	// touched a wall owned by worldspawn
	//FIXME: we probably want to periodically verify if the wall still exists or moved, and flip over otherwise
	if (other && other->inuse && other == world && pitch == 0)
	{
		// flip over
		//gi.dprintf("touched wall, flip over\n");
		// adjust the position
		ent->s.origin[2] += ent->maxs[2];
		// flip the model over 90 degrees
		ent->s.angles[PITCH] = 90;
		ent->s.angles[YAW] = angles[YAW];
		ent->movetype = MOVETYPE_NONE;
		// flip the bbox
		//save = ent->mins[2];
		//ent->mins[2] = ent->maxs[2] * -1;
		//ent->maxs[2] = save * -1;
		gi.linkentity(ent);
		return true;
	}
	return false;
}

qboolean vrx_attach_ceiling(edict_t* ent, edict_t *other, cplane_t *plane)
{
	vec3_t angles;
	float pitch, save;

	if (ent->mtype != M_SPIKER && ent->mtype != M_OBSTACLE && ent->mtype != M_HEALER)
		return false; // not an entity that can be attached to the ceiling
	if (ent->groundentity)
	{
		if (ent->s.angles[PITCH] == 180)
		{
			//gi.dprintf("touched ground, flip upright\n");
			// flip upright
			ent->s.angles[PITCH] = 0;
			save = ent->mins[2];
			ent->mins[2] = ent->maxs[2] * -1;
			ent->maxs[2] = save * -1;
			// adjust the position
			ent->s.origin[2] -= ent->maxs[2];
			gi.linkentity(ent);
		}
		return false; // on the ground
	}
	if (ent->s.angles[PITCH] == 180)
		return false; // already flipped
	if (!plane)
		return false; // not touching a plane

	// calculate the pitch angle of the plane, 270 = ceiling, 90 = floor, 0 = wall
	vectoangles(plane->normal, angles);
	pitch = angles[PITCH] * -1;

	// touched a ceiling owned by worldspawn
	//FIXME: we probably want to periodically verify if the ceiling still exists or moved, and flip over otherwise
	if (other && other->inuse && other == world && pitch == 270)
	{
		// flip over
		//gi.dprintf("touched ceiling, flip over\n");
		// adjust the position
		ent->s.origin[2] += ent->maxs[2];
		// flip the model over 180 degrees
		ent->s.angles[PITCH] = 180;
		ent->movetype = MOVETYPE_NONE;
		// flip the bbox
		save = ent->mins[2];
		ent->mins[2] = ent->maxs[2] * -1;
		ent->maxs[2] = save * -1;
		gi.linkentity(ent);
		return true;
	}
	return false;
}

// touch function for all the gloom stuff
void organ_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	/*
	if (ent->groundentity)
		gi.dprintf("%s touched the ground\n", ent->classname);
	else if (other && other->inuse)
		gi.dprintf("%s touching %s\n", ent->classname, other->classname);
	else
		gi.dprintf("%s not touching the ground\n", ent->classname);
	if (plane)
	{
		float pitch, yaw, roll;
		vec3_t angles;
		vectoangles(plane->normal, angles);
		gi.dprintf("touching plane pitch %.0f yaw %.0f roll %.0f\n", angles[PITCH], angles[YAW], angles[ROLL]);
	}
	*/
	vrx_attach_wall(ent, other, plane);
	vrx_attach_ceiling(ent, other, plane);
	V_Touch(ent, other, plane, surf);

	// don't push or heal something that's already dead or invalid
	if (!ent || !ent->inuse || !ent->takedamage || ent->health < 1)
		return;

	// allow organ to be pushed if it's not flipped (attached to ceiling or wall)
	if (ent->s.angles[PITCH] != 180 && ent->s.angles[PITCH] != 90)
		V_Push(ent, other, plane, surf);

	if (G_EntIsAlive(other) && other->client && OnSameTeam(ent, other) 
		&& other->client->pers.inventory[power_cube_index] >= 5
		&& level.time > ent->lasthurt + 0.5 && ent->health < ent->max_health
		&& level.framenum > ent->monsterinfo.regen_delay1)
	{
		ent->health_cache += (int)(0.5 * ent->max_health) + 1;
		ent->monsterinfo.regen_delay1 = (int)(level.framenum + 1 / FRAMETIME);
		other->client->pers.inventory[power_cube_index] -= 5;
	}
}

void organ_death_message(edict_t* self, edict_t* attacker, int max)
{
	int qty;

	// don't need to print a message if the revived organ expired
	if (attacker && attacker->inuse && attacker == world && level.time >= self->monsterinfo.resurrected_timeout)
		return;

	if (self->mtype == M_SPIKER)
		qty = self->activator->num_spikers;
	else if (self->mtype == M_GASSER)
		qty = self->activator->num_gasser;
	else if (self->mtype == M_OBSTACLE)
		qty = self->activator->num_obstacle;
	else if (self->mtype == M_SPIKEBALL)
		qty = self->activator->num_spikeball;

	char buf[100];
	buf[0] = 0;

	strcat(buf, va("Your %s was killed", V_GetMonsterKind(self->mtype)));

	if (attacker && attacker->inuse)
	{
		if (PM_MonsterHasPilot(attacker))
			attacker = attacker->owner;

		if (attacker->client)
			strcat(buf, va(" by %s", attacker->client->pers.netname));
		else if (attacker->mtype)
			strcat(buf, va(" by a %s", V_GetMonsterName(attacker)));
		else if (attacker->classname)
			strcat(buf, va(" by a %s", attacker->classname));
	}

	if (max > 1)
		strcat(buf, va(" (%d/%d remain)", qty, max));
		
	safe_cprintf(self->activator, PRINT_HIGH, "%s\n", buf);
	
}

void organ_death_cleanup(edict_t* self, edict_t* attacker, int* organCounter, int max, qboolean message)
{
	if (organCounter)
	{
		*organCounter -= 1;
	}
	else
	{
		if (self->mtype == M_OBSTACLE)
			self->activator->num_obstacle--;
		else if (self->mtype == M_SPIKER)
			self->activator->num_spikers--;
		else if (self->mtype == M_HEALER)
			self->activator->healer = NULL;
		else if (self->mtype == M_COCOON)
			self->activator->cocoon = NULL;
		else if (self->mtype == M_GASSER)
			self->activator->num_gasser--;
		else if (self->mtype == M_SPIKEBALL)
			self->activator->num_spikeball--;
	}
	self->monsterinfo.slots_freed = true;

	self->s.effects &= ~(EF_PLASMA | EF_SPHERETRANS); // remove transparency from growth/pickup

	if (self->mtype == M_COCOON)
	{
		// restore cocooned entity
		if (self->enemy && self->enemy->inuse)
		{
			self->enemy->movetype = self->count;
			self->enemy->svflags &= ~SVF_NOCLIENT;
			self->enemy->flags &= FL_COCOONED;
		}
	}

	if (self->activator->client)
	{
		layout_remove_tracked_entity(&self->activator->client->layout, self);

		// stop tracking this previously picked up entity and drop it
		vrx_clear_pickup_ent(self->activator->client, self);

		self->flags &= ~FL_PICKUP;
		// note: owner isn't cleared because it's possible the organ is clipping the player's bbox

		if (message)
			organ_death_message(self, attacker, max);
	}
	AI_EnemyRemoved(self);
}

// generic remove function for all the gloom stuff
void organ_remove (edict_t *self, qboolean refund)
{
	if (!self || !self->inuse || self->deadflag == DEAD_DEAD)
		return;

	if (!self->monsterinfo.slots_freed && self->activator && self->activator->inuse)
	{
		organ_death_cleanup(self, NULL, NULL, SPIKER_MAX_COUNT, false);

		if (refund && self->activator->client)
			self->activator->client->pers.inventory[power_cube_index] += (self->health / self->max_health) * self->monsterinfo.cost;
	}

	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_NOCLIENT;
	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->deadflag = DEAD_DEAD;
}

// removes organ if activator becomes invalid
// returns false if the organ was removed, true if not
qboolean organ_checkowner (edict_t *self)
{
	qboolean remove = false;

	// make sure activator exists
	if (!self->activator || !self->activator->inuse)
		remove = true;
	// make sure player activator is alive
	else if (self->activator->client && (self->activator->health < 1 
		|| (self->activator->client->respawn_time > level.time) 
		|| self->activator->deadflag == DEAD_DEAD))
		remove = true;

	if (remove)
	{
		organ_remove(self, false);
		return false;
	}

	return true;
}

// kills organ if it expires
// return false if the organ died, true if not
qboolean organ_checkrevived(edict_t* self)
{
	//gi.dprintf("%s: %d %.1f %.1f\n", __func__, self->monsterinfo.resurrected_level, self->monsterinfo.resurrected_timeout, (float)level.time);
	// resurrected organ has expired
	if (self->monsterinfo.resurrected_level && level.time > self->monsterinfo.resurrected_timeout)
	{
		// kill it!
		self->health = 0;
		self->die(self, world, world, 0, vec3_origin);
		return false;
	}
	return true;
}

// used to restore a prevous movetype (MOVETYPE_TOSS or MOVETYPE_NONE) needed for V_Push to work (MOVETYPE_STEP)
void organ_restoreMoveType (edict_t *self)
{
	if (self->movetype_prev && level.framenum >= self->movetype_frame)
	{
		self->movetype = self->movetype_prev;
		self->movetype_prev = 0;
	}
}

void organ_removeall (edict_t *ent, char *classname, qboolean refund)
{
	edict_t *e = NULL;

	while((e = G_Find(e, FOFS(classname), classname)) != NULL) 
	{
		if (e && e->activator && e->activator->inuse && (e->activator == ent) && !RestorePreviousOwner(e))
			organ_remove(e, refund);
	}
}

//Talent: Exploding Bodies
/*
qboolean organ_explode (edict_t *self)
{
    int damage, radius, talentLevel = vrx_get_talent_level(self->activator, TALENT_EXPLODING_BODIES);

	if (talentLevel < 1 || self->style)
		return false;

	// cause the damage
	damage = radius = 100 * talentLevel;
	if (radius > 200)
		radius = 200;
	T_RadiusDamage (self, self, damage, self, radius, MOD_CORPSEEXPLODE);

    gi.sound(self, CHAN_VOICE, gi.soundindex(va("abilities/corpse_explode%d.wav", GetRandom(1, 6))), 1, ATTN_NORM, 0);
	return true;
}*/

#define HEALER_FRAMES_GROW_START	0
#define HEALER_FRAMES_GROW_END		15
#define HEALER_FRAMES_START			16
#define HEALER_FRAMES_END			26
#define HEALER_FRAME_DEAD			4


qboolean healer_heal (edict_t *self, edict_t *other)
{
	float	value;
    //int	talentLevel = vrx_get_talent_level(self->activator, TALENT_SUPER_HEALER);
	qboolean regenerate = false;

	// (apple)
	// In the healer's case, checking for the health 
	// self->health >= 1 should be enough,
	// but used G_EntIsAlive for consistency.
	//if (G_EntIsAlive(other) && G_EntIsAlive(self) && OnSameTeam(self, other) && other != self)
	//{
		int frames = qf2sf(5000 / (15 * self->monsterinfo.level)); // idk how much would this change tbh

        value = 1.0f + 0.1f * vrx_get_talent_level(self->activator, TALENT_SUPER_HEALER);

		// regenerate health
		if (M_Regenerate(other, frames, qf2sf(1), value, true, false, false, &other->monsterinfo.regen_delay2)) {
			regenerate = true;
			other->heal_exp_owner = self->creator;
			if ( other->heal_exp_time < level.time )
				other->heal_exp_time = level.time;
			if ( other->heal_exp_time < level.time + 60.0 ) {
				other->heal_exp_time += 30.0;
			}
		}

		// regenerate armor
		/*
		if (talentLevel > 0)
		{
			frames *= (3.0 - (0.4 * talentLevel)); // heals 100% armor in 5 seconds at talent level 5
			if (M_Regenerate(other, frames, 10,  1.0, false, true, false, &other->monsterinfo.regen_delay3))
				regenerate = true;
		}*/
		
		// play healer sound if health or armor was regenerated
		if (regenerate)
		{
			if (level.time > self->msg_time)
			{
				gi.sound(self, CHAN_AUTO, gi.soundindex("organ/healer1.wav"), 1, ATTN_NORM, 0);
				self->msg_time = level.time + 1.5;
			}
			return true;
		}
		return false;
	//}
}

qboolean healer_validtarget(edict_t* self, edict_t* target)
{
	float velocity;

	if (target == self)
		return false;

	//if (!G_EntExists(target))
	if (!G_EntIsAlive(target))
		return false;

	// don't target players with invulnerability
	if (target->client && (target->client->invincible_framenum > level.framenum))
		return false;

	// don't target spawning players
	if (target->client && (target->client->respawn_time > level.time))
		return false;

	// don't target players in chat-protect
	if (target->client && ((target->flags & FL_CHATPROTECT) || (target->flags & FL_GODMODE)))
		return false;

	// don't target frozen players
	//if (que_typeexists(target->curses, CURSE_FROZEN))
	//	return false;

	// target must be visible
	if (!visible(self, target))
		return false;

	// make sure target is within sight range
	if (entdist(self, target) > self->monsterinfo.sight_range)
		return false;

	// make sure we have an unobstructed path to target
	//if (!G_ClearShot(self, NULL, target))
	//	return false;

	if (target->movetype != MOVETYPE_NONE)
	{
		velocity = VectorLength(target->velocity);
		// make sure target is touching the ground and isn't moving
		if (!target->groundentity || velocity > 1)
		{
			//gi.dprintf("groundentity: %s velocity: %f\n", target->groundentity ? "true" : "false", velocity);
			return false;
		}
	}

	// make sure target is alive!
	//if (target->deadflag == DEAD_DEAD || target->health < 1)
		//return false;

	// target needs to be on our team
	if (OnSameTeam(self, target) < 2)
		return false;

	return true;
}

//void G_Spawn_Trails(int type, vec3_t start, vec3_t endpos);
void G_Spawn_Splash(int type, int count, int color, vec3_t start, vec3_t movdir, vec3_t origin);

void healer_healeffects(edict_t* self, edict_t *target)
{
	vec3_t		start, end, forward;//, angles;
	trace_t		tr;
	// start and end position of effect is slightly above the floor
	VectorCopy(self->s.origin, start);
	start[2] = self->absmin[2] + 4;
	VectorCopy(target->s.origin, end);
	end[2] = target->absmin[2] + 4;

	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);

	// move effect slightly to the right or left
	VectorSubtract(start, end, forward);
	VectorNormalize(forward);
	vectoangles(forward, forward);
	AngleVectors(forward, NULL, forward, NULL);
	VectorMA(tr.endpos, crandom() * target->maxs[1], forward, end);

	G_Spawn_Splash(TE_LASER_SPARKS, 6, GetRandom(200, 217), end, forward, end);

}
void healer_attack (edict_t *self)
{
	edict_t*	target =NULL;
	float		range = self->monsterinfo.sight_range;
	//vec3_t		start, end, forward;//, angles;
	//trace_t		tr;

	while ((target = findradius(target, self->s.origin, range)) != NULL)
	{
		if (!healer_validtarget(self, target))
			continue;

		if (healer_heal(self, target))
		{
			healer_healeffects(self, target);
			/*
			// start and end position of effect is slightly above the floor
			VectorCopy(self->s.origin, start);
			start[2] = self->absmin[2] + 4;
			VectorCopy(target->s.origin, end);
			end[2] = target->absmin[2] + 4;

			tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);

			// move effect slightly to the right or left
			VectorSubtract(start, end, forward);
			VectorNormalize(forward);
			vectoangles(forward, forward);
			AngleVectors(forward, NULL, forward, NULL);
			VectorMA(tr.endpos, crandom()*target->maxs[1], forward, end);

			G_Spawn_Splash(TE_LASER_SPARKS, 6, GetRandom(200, 217), end, forward, end);*/
		}
	}
}

void healer_think (edict_t *self)
{
	if (!G_EntIsAlive(self->activator))
	{
		organ_remove(self, false);
		return;
	}

	vrx_set_pickup_owner(self); // sets owner when this entity is picked up, making it non-solid to the player

	// if position has been updated, check for ground entity
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	// don't slide
	if (self->groundentity)
		VectorClear(self->velocity);
	
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);

	if (!que_typeexists(self->curses, CURSE_FROZEN))
	{
		if (level.time > self->lasthurt + 1.0)
		{
			healer_attack(self);
			M_Regenerate(self, qf2sf(300), qf2sf(10), 1.0, true, false, false, &self->monsterinfo.regen_delay1);
		}
	}

	G_RunFrames(self, HEALER_FRAMES_START, HEALER_FRAMES_END, false);

	self->nextthink = level.time + FRAMETIME;
}

void healer_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	//vrx_attach_wall(ent, other, plane);
	vrx_attach_ceiling(ent, other, plane);
	V_Touch(ent, other, plane, surf);
	healer_heal(ent, other);	
}

void healer_grow (edict_t *self)
{
	if (!G_EntIsAlive(self->activator))
	{
		organ_remove(self, false);
		return;
	}

	vrx_set_pickup_owner(self); // sets owner when this entity is picked up, making it non-solid to the player

	// if position has been updated, check for ground entity
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	// don't slide
	if (self->groundentity)
		VectorClear(self->velocity);
	
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);

	self->nextthink = level.time + FRAMETIME;

	if (self->s.frame == HEALER_FRAMES_GROW_END)
	{
		self->think = healer_think;
		self->touch = healer_touch;
		self->style = 0;// done growing
		return;
	}

	if (self->s.frame == HEALER_FRAMES_GROW_START)
		gi.sound(self, CHAN_VOICE, gi.soundindex("organ/organe3.wav"), 1, ATTN_STATIC, 0);

	G_RunFrames(self, HEALER_FRAMES_GROW_START, HEALER_FRAMES_GROW_END, false);
}

void healer_dead (edict_t *self)
{
	if (level.time > self->delay)
	{
		organ_remove(self, false);
		return;
	}

	if (level.time == self->delay - 5)
		self->s.effects |= EF_PLASMA;
	else if (level.time == self->delay - 2)
		self->s.effects |= EF_SPHERETRANS;

	self->nextthink = level.time + FRAMETIME;
}

void healer_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

	if (!self->monsterinfo.slots_freed && self->activator && self->activator->inuse)
	{
		self->activator->healer = NULL;
		self->monsterinfo.slots_freed = true;
	
		if (PM_MonsterHasPilot(attacker))
			attacker = attacker->owner;

		if (attacker->client)
			safe_cprintf(self->activator, PRINT_HIGH, "Your healer was killed by %s\n", attacker->client->pers.netname);
		else if (attacker->mtype)
			safe_cprintf(self->activator, PRINT_HIGH, "Your healer was killed by a %s\n", V_GetMonsterName(attacker));
		else
			safe_cprintf(self->activator, PRINT_HIGH, "Your healer was killed by a %s\n", attacker->classname);
	}

	if (self->health <= self->gib_health /* || organ_explode(self)*/)
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < 2; n++)
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n= 0; n < 4; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		organ_remove(self, false);
		return;
	}

	if (self->deadflag == DEAD_DYING)
		return;

	self->think = healer_dead;
	self->deadflag = DEAD_DYING;
	self->delay = level.time + 20.0;
	self->nextthink = level.time + FRAMETIME;
	self->s.frame = HEALER_FRAME_DEAD;
	self->movetype = MOVETYPE_TOSS;
	self->maxs[2] = 4;
	gi.linkentity(self);
}

edict_t *CreateHealer (edict_t *ent, int skill_level)
{
	edict_t *e;

	e = G_Spawn();
	e->style = 1; //growing
	e->activator = ent;
	e->think = healer_grow;
	e->nextthink = level.time + FRAMETIME;
	e->s.modelindex = gi.modelindex ("models/objects/organ/healer/tris.md2");
	e->s.renderfx |= RF_IR_VISIBLE;
	e->solid = SOLID_BBOX;
	e->movetype = MOVETYPE_STEP;//MOVETYPE_TOSS;
	e->svflags |= SVF_MONSTER;
	e->clipmask = MASK_MONSTERSOLID;
	e->mass = 500;
	e->classname = "healer";
	e->takedamage = DAMAGE_AIM;
	e->health = e->max_health = HEALER_INITIAL_HEALTH + HEALER_ADDON_HEALTH * skill_level;
	e->monsterinfo.level = skill_level;
	e->monsterinfo.sight_range = 256;
	e->gib_health = -2 * BASE_GIB_HEALTH;
	e->die = healer_die;
	e->touch = V_Touch;
	VectorSet(e->mins, -28, -28, 0);
	VectorSet(e->maxs, 28, 28, 32);
	e->mtype = M_HEALER;
	e->creator = ent;
	ent->healer = e;

	return e;
}

void Cmd_Healer_f (edict_t *ent)
{
	edict_t *healer;
	vec3_t	start;

	if (vrx_toggle_pickup(ent, M_HEALER, 150)) // search for entity to pick up, or drop the one we're holding
		return;

	if (ent->healer && ent->healer->inuse)
	{
		organ_remove(ent->healer, true);
		safe_cprintf(ent, PRINT_HIGH, "Healer removed\n");
		return;
	}

	if (!V_CanUseAbilities(ent, HEALER, HEALER_COST, true))
		return;

	healer = CreateHealer(ent, ent->myskills.abilities[HEALER].current_level);
	if (!G_GetSpawnLocation(ent, 100, healer->mins, healer->maxs, start, NULL, PROJECT_HITBOX_FAR, false))
	{
		ent->healer = NULL;
		G_FreeEdict(healer);
		return;
	}
	VectorCopy(start, healer->s.origin);
	VectorCopy(ent->s.angles, healer->s.angles);
	healer->s.angles[PITCH] = 0;

	if (ent->client)
	{
		layout_add_tracked_entity(&ent->client->layout, healer);
		AI_EnemyAdded(healer);
		// pick it up!
		ent->client->pickup = healer;
	}

	gi.linkentity(healer);
	healer->monsterinfo.cost = HEALER_COST;

	ent->client->ability_delay = level.time + HEALER_DELAY;
	ent->client->pers.inventory[power_cube_index] -= HEALER_COST;

	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}

#define SPIKER_FRAMES_GROW_START		0
#define SPIKER_FRAMES_GROW_END			12
#define SPIKER_FRAMES_NOAMMO_START		14
#define SPIKER_FRAMES_NOAMMO_END		17
#define SPIKER_FRAMES_REARM_START		19
#define SPIKER_FRAMES_REARM_END			23
#define SPIKER_FRAME_READY				13
#define SPIKER_FRAME_DEAD				18

void spiker_dead (edict_t *self)
{
	if (level.time > self->delay)
	{
		organ_remove(self, false);
		return;
	}

	if (level.time == self->delay - 5)
		self->s.effects |= EF_PLASMA;
	else if (level.time == self->delay - 2)
		self->s.effects |= EF_SPHERETRANS;

	self->nextthink = level.time + FRAMETIME;
}

void spiker_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (!self->monsterinfo.slots_freed && self->activator && self->activator->inuse)
	{
		organ_death_cleanup(self, attacker, &self->activator->num_spikers, SPIKER_MAX_COUNT, true);
	}

	if (self->health <= self->gib_health /* || organ_explode(self)*/)
	{
		int n;

		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < 2; n++)
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n= 0; n < 4; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		organ_remove(self, false);
		return;
	}

	if (self->deadflag == DEAD_DYING)
		return;

	self->think = spiker_dead;
	self->deadflag = DEAD_DYING;
	self->delay = level.time + 20.0;
	self->nextthink = level.time + FRAMETIME;
	self->s.frame = SPIKER_FRAME_DEAD;
	self->movetype = MOVETYPE_TOSS;
	self->maxs[2] = 16;

	// flip spiker upright
	self->mins[2] = 0;
	self->s.angles[PITCH] = 0;

	gi.linkentity(self);
}

void spiker_attack (edict_t *self)
{
	float	dist, chance = 0.05 * self->light_level; // Talent: Deadly Spikes gives 5% chance/level to stun
	float	range=SPIKER_INITIAL_RANGE+SPIKER_ADDON_RANGE*self->monsterinfo.level;
	int		speed=SPIKER_INITIAL_SPEED+SPIKER_ADDON_SPEED*self->monsterinfo.level;
	vec3_t	forward, start, end;
	edict_t *e=NULL;

	if (self->monsterinfo.attack_finished > level.time)
		return;

	if (que_typeexists(self->curses, CURSE_FROZEN))
		return;

	VectorCopy(self->s.origin, start);
	if (self->s.angles[PITCH] == 180) // flipped spiker
		start[2] = self->absmin[2] + 16;
	else
		start[2] = self->absmax[2] - 16;

	while ((e = findradius(e, self->s.origin, range)) != NULL)
	{
		if (!G_ValidTarget(self, e, true, true))
			continue;
		
		// copy target location
		G_EntMidPoint(e, end);

		// calculate distance to target
		dist = distance(e->s.origin, start);

		// move our target point based on projectile and enemy velocity
		VectorMA(end, (float)dist/speed, e->velocity, end);
				
		// calculate direction vector to target
		VectorSubtract(end, start, forward);
		VectorNormalize(forward);

		// Deadly Spikes talent provides a chance to shoot a spike that will stun enemies
		if (chance > random())
			fire_spike(self, start, forward, self->dmg, 0.5, speed);
		else
			fire_spike(self, start, forward, self->dmg, 0, speed);
		
		//FIXME: only need to do this once
		self->monsterinfo.attack_finished = level.time + SPIKER_REFIRE_DELAY;
		self->s.frame = SPIKER_FRAMES_NOAMMO_START;
		gi.sound (self, CHAN_VOICE, gi.soundindex ("weapons/twang.wav"), 1, ATTN_NORM, 0);
	}	
}

//void NearestNodeLocation (vec3_t start, vec3_t node_loc);
//int FindPath(vec3_t start, vec3_t destination);
void spiker_think (edict_t *self)
{
	edict_t *e=NULL;

	if (!organ_checkowner(self) || !organ_checkrevived(self))
		return;

	vrx_set_pickup_owner(self); // sets owner when this entity is picked up, making it non-solid to the player

	// don't attack while picked up
	if (self->flags & FL_PICKUP)
		self->monsterinfo.attack_finished = level.time + 1.0;

	//organ_restoreMoveType(self);

	if (self->removetime > 0)
	{
		qboolean converted=false;

		if (self->flags & FL_CONVERTED)
			converted = true;

		if (level.time > self->removetime)
		{
			// if we were converted, try to convert back to previous owner
			if (converted && self->prev_owner && self->prev_owner->inuse)
			{
				if (!ConvertOwner(self->prev_owner, self, 0, false, false))
				{
					organ_remove(self, false);
					return;
				}
			}
			else
			{
				organ_remove(self, false);
				return;
			}
		}
		// warn the converted monster's current owner
		else if (converted && self->activator && self->activator->inuse && self->activator->client 
			&& (level.time > self->removetime-5) && !(level.framenum % (int)(1 / FRAMETIME)))
				safe_cprintf(self->activator, PRINT_HIGH, "%s conversion will expire in %.0f seconds\n", 
					V_GetMonsterName(self), self->removetime-level.time);	
	}

	// try to attack something
	spiker_attack(self);

	V_HealthCache(self, (int)(0.2 * self->max_health), 1);

	//if (level.time > self->lasthurt + 1.0) 
	//	M_Regenerate(self, 300, 10, true, false, false, &self->monsterinfo.regen_delay1);

	// run animation frames
	if (self->monsterinfo.attack_finished - 0.5 > level.time)
	{
		if (self->s.frame != SPIKER_FRAME_READY)
			G_RunFrames(self, SPIKER_FRAMES_NOAMMO_START, SPIKER_FRAMES_NOAMMO_END, false);
	}
	else
	{
		if (self->s.frame != SPIKER_FRAME_READY && self->s.frame != SPIKER_FRAMES_REARM_END)
			G_RunFrames(self, SPIKER_FRAMES_REARM_START, SPIKER_FRAMES_REARM_END, false);
		else
			self->s.frame = SPIKER_FRAME_READY;
	}

	// if position has been updated, check for ground entity
	// note: this is checked every frame if using MOVETYPE_STEP, whereas MOVETYPE_TOSS may require gi.linkentity to be run on velocity/position changes
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		//gi.dprintf("%s ground position updated\n", self->classname);
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	// ground friction to prevent excessive sliding
	if (self->groundentity)
	{
		self->velocity[0] *= 0.5;
		self->velocity[1] *= 0.5;
	}
	//else
	//	gi.dprintf("spiker not touching the ground\n");
	
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);

	//gi.linkentity(self);
	
	self->nextthink = level.time + FRAMETIME;
}

void spiker_grow (edict_t *self)
{
	if (!organ_checkowner(self) || !organ_checkrevived(self))
		return;

	vrx_set_pickup_owner(self); // sets owner when this entity is picked up, making it non-solid to the player

	//organ_restoreMoveType(self);

	V_HealthCache(self, (int)(0.2 * self->max_health), 1);

	// if position has been updated, check for ground entity
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		//gi.dprintf("%s ground position updated\n", self->classname);
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	// don't slide
	if (self->groundentity)
		VectorClear(self->velocity);
	
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);

	self->s.effects |= EF_PLASMA;

	self->nextthink = level.time + FRAMETIME;

	if (self->s.frame == SPIKER_FRAMES_GROW_END && self->health >= self->max_health)
	{
		self->s.frame = SPIKER_FRAME_READY;
		self->think = spiker_think;
		self->style = 0;//done growing
		return;
	}

	if (self->s.frame == SPIKER_FRAMES_GROW_START)
		gi.sound(self, CHAN_VOICE, gi.soundindex("organ/organe3.wav"), 1, ATTN_NORM, 0);

	if (self->s.frame != SPIKER_FRAMES_GROW_END)
		G_RunFrames(self, SPIKER_FRAMES_GROW_START, SPIKER_FRAMES_GROW_END, false);
}

edict_t *CreateSpiker (edict_t *ent, int skill_level)
{
	edict_t *e;

	e = G_Spawn();
	e->style = 1; //growing
	e->activator = ent;
	e->think = spiker_grow;
	e->nextthink = level.time + FRAMETIME;
	e->s.modelindex = gi.modelindex ("models/objects/organ/spiker/tris.md2");
	e->s.renderfx |= RF_IR_VISIBLE;
	e->solid = SOLID_BBOX;
	e->movetype = MOVETYPE_STEP;
	e->clipmask = MASK_MONSTERSOLID;
	e->svflags |= SVF_MONSTER;//Note/Important/Hint: tells MOVETYPE_STEP physics to clip on any solid object (not just walls)
	e->monsterinfo.touchdown = M_Touchdown;
	e->mass = 500;
	e->classname = "spiker";
	e->takedamage = DAMAGE_AIM;
	e->max_health = SPIKER_INITIAL_HEALTH + SPIKER_ADDON_HEALTH * skill_level;
	e->health = 0.5*e->max_health;
	e->monsterinfo.level = skill_level;
	e->light_level = vrx_get_talent_level(ent, TALENT_DEADLY_SPIKES); // Talent: Deadly Spikes
	e->dmg = (SPIKER_INITIAL_DAMAGE + SPIKER_ADDON_DAMAGE * skill_level) * vrx_get_synergy_mult(ent, SPIKER);
	//e->dmg *= 1.0 + (0.1 * e->light_level); // damage increase from Deadly Spikes talent
	e->gib_health = -1.5 * BASE_GIB_HEALTH;
	e->die = spiker_die;
	e->touch = organ_touch;
	VectorSet(e->mins, -24, -24, 0);
	VectorSet(e->maxs, 24, 24, 48);
	e->mtype = M_SPIKER;

	ent->num_spikers++;

	return e;
}

void Cmd_Spiker_f (edict_t *ent)
{
	int		cost = SPIKER_COST;
	edict_t *spiker;
	vec3_t	start;

	if (Q_strcasecmp (gi.args(), "remove") == 0)
	{
		organ_removeall(ent, "spiker", true);
		safe_cprintf(ent, PRINT_HIGH, "Spikers removed\n");
		ent->num_spikers = 0;
		return;
	}

	if (vrx_toggle_pickup(ent, M_SPIKER, 128)) // search for entity to pick up, or drop the one we're holding
		return;

	if (ctf->value && (CTF_DistanceFromBase(ent, NULL, CTF_GetEnemyTeam(ent->teamnum)) < CTF_BASE_DEFEND_RANGE))
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't build in enemy base!\n");
		return;
	}

	// cost is doubled if you are a flyer or cacodemon below skill level 5
	if ((ent->mtype == MORPH_FLYER && ent->myskills.abilities[FLYER].current_level < 5) 
		|| (ent->mtype == MORPH_CACODEMON && ent->myskills.abilities[CACODEMON].current_level < 5))
		cost *= 2;

	if (!V_CanUseAbilities(ent, SPIKER, cost, true))
		return;

	if (ent->num_spikers >= SPIKER_MAX_COUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "You have reached the maximum amount of spikers (%d)\n", (int)SPIKER_MAX_COUNT);
		return;
	}

	spiker = CreateSpiker(ent, ent->myskills.abilities[SPIKER].current_level);
	if (!vrx_position_player_summonable(ent, spiker, 100))// move spiker into position in front of us
	{
		ent->num_spikers--;
		return;
	}

	spiker->monsterinfo.attack_finished = level.time + 2.0;
	spiker->monsterinfo.cost = cost;

	if (ent->client)
	{
		layout_add_tracked_entity(&ent->client->layout, spiker);
		AI_EnemyAdded(spiker);
		// pick it up!
		ent->client->pickup = spiker;
	}

	safe_cprintf(ent, PRINT_HIGH, "Spiker created (%d/%d)\n", ent->num_spikers, (int)SPIKER_MAX_COUNT);

	ent->client->ability_delay = level.time + SPIKER_DELAY;
	ent->client->pers.inventory[power_cube_index] -= cost;
	//ent->holdtime = level.time + SPIKER_DELAY;

	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}

#define OBSTACLE_FRAMES_GROW_START		0
#define OBSTACLE_FRAMES_GROW_END		9
#define OBSTACLE_FRAME_READY			12
#define OBSTACLE_FRAME_DEAD				13

void obstacle_dead (edict_t *self)
{
	if (level.time > self->delay)
	{
		//gi.dprintf("remove dead obstacle %.1f!\n", level.time);
		organ_remove(self, false);
		return;
	}

	if (level.time == self->delay - 5)
		self->s.effects |= EF_PLASMA;
	else if (level.time == self->delay - 2)
		self->s.effects |= EF_SPHERETRANS;

	self->nextthink = level.time + FRAMETIME;
}

void obstacle_return(edict_t* self)
{
	//gi.dprintf("obstacle_return\n");

	VectorCopy(self->move_origin, self->s.origin);
	VectorCopy(self->move_origin, self->s.old_origin);
	gi.linkentity(self);
	self->movetype = MOVETYPE_NONE;
	self->s.event = EV_PLAYER_TELEPORT;
	VectorClear(self->velocity);
}

void obstacle_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int max = OBSTACLE_MAX_COUNT;
	int cur;
	qboolean flipped = false;

	//gi.dprintf("obstacle_die %d %d\n", self->health, self->gib_health);

	if (self->s.angles[PITCH] == 180)
		flipped = true;

	if (flipped && self->movetype != MOVETYPE_NONE)
		obstacle_return(self);

	if (!self->monsterinfo.slots_freed && self->activator && self->activator->inuse)
	{
		self->activator->num_obstacle--;
		cur = self->activator->num_obstacle;
		self->monsterinfo.slots_freed = true;
		AI_EnemyRemoved(self);

		if (PM_MonsterHasPilot(attacker))
			attacker = attacker->owner;

		if (self->activator->client)
			layout_remove_tracked_entity(&self->activator->client->layout, self);

		if (attacker->client) {
			safe_cprintf(self->activator, PRINT_HIGH, "Your obstacle was killed by %s (%d/%d remain)\n", attacker->client->pers.netname, cur, max);
		}
		else if (attacker->mtype)
			safe_cprintf(self->activator, PRINT_HIGH, "Your obstacle was killed by a %s (%d/%d remain)\n", V_GetMonsterName(attacker), cur, max);
		else
			safe_cprintf(self->activator, PRINT_HIGH, "Your obstacle was killed by a %s (%d/%d remain)\n", attacker->classname, cur, max);
	}

	if (self->health <= self->gib_health /* || organ_explode(self)*/)
	{
		int n;

		//gi.dprintf("gib obstacle\n");
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < 2; n++)
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n= 0; n < 4; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		organ_remove(self, false);
		return;
	}

	if (self->deadflag == DEAD_DYING)
		return;

	self->think = obstacle_dead;
	self->deadflag = DEAD_DYING;
	self->delay = level.time + 20.0;
	//gi.dprintf("time: %.1f delay %.1f\n", (level.time), self->delay);
	self->nextthink = level.time + FRAMETIME;
	self->s.frame = OBSTACLE_FRAME_DEAD;
	if (!flipped)
	{
		// don't make tossable if we're stuck to the ceiling
		self->movetype = MOVETYPE_TOSS;
		// shorten the bounding box
		self->maxs[2] = 16;
	}
	else
	{
		self->mins[2] = -16;
	}
	self->touch = V_Touch;

	gi.linkentity(self);
}

void obstacle_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	organ_touch(self, other, plane, surf);

	if (other && other->inuse && other->takedamage && !OnSameTeam(self->activator, other) 
		&& (level.framenum >= self->monsterinfo.jumpup))
	{
		T_Damage(other, self, self, self->velocity, self->s.origin, 
			plane->normal, self->dmg, 0, 0, MOD_OBSTACLE);
		self->monsterinfo.jumpup = level.framenum + 1;
	}
	else if (other && other->inuse && other->takedamage)
	{
		self->svflags &= ~SVF_NOCLIENT;
		self->monsterinfo.idle_frames = 0;
	}
}

void obstacle_cloak (edict_t *self)
{
	self->flags |= FL_NOTARGET; // monsters won't target the obstacle until it hurts them
	// already cloaked
	if (self->svflags & SVF_NOCLIENT)
	{
		// random chance for obstacle to become uncloaked for a frame
		//if (level.time > self->lasthbshot && random() <= 0.05)
		if (!(level.framenum % (int)(5 / FRAMETIME)) && random() > 0.5)
		{
			self->svflags &= ~SVF_NOCLIENT;
			//self->lasthbshot = level.time + 1.0;
		}
		return;
	}

	// cloak after idling for awhile
	if (self->monsterinfo.idle_frames >= self->monsterinfo.nextattack)
		self->svflags |= SVF_NOCLIENT;
}

void obstacle_move (edict_t *self)
{
	// check for ground entity
	if (!M_CheckBottom(self) || self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	// check for movement/sliding
	if (!(int)self->velocity[0] && !(int)self->velocity[1] && !(int)self->velocity[2])
	{
		// obstacle not attached to the ceiling
		if (self->s.angles[PITCH] != 180)
		{
			// stick to the ground
			if (self->groundentity && self->groundentity == world && level.time > self->touch_debounce_time)
			{
				//gi.dprintf("obstacle sticking to the ground %f %f\n", level.time, self->touch_debounce_time);
				self->movetype = MOVETYPE_NONE;
			}
			else
				self->movetype = MOVETYPE_STEP;
		}

		// increment idle frames
		self->monsterinfo.idle_frames++;
	}
	else
	{
		// de-cloak
		self->svflags &= ~SVF_NOCLIENT;
		// reset idle frames
		self->monsterinfo.idle_frames = 0;
	}

	// ground friction to prevent excessive sliding
	if (self->groundentity)
	{
		self->velocity[0] *= 0.5;
		self->velocity[1] *= 0.5;
	}
}

qboolean obstacle_attack_clear(edict_t* self, edict_t* target)
{
	// no entity blocking our path
	if (!target || !target->inuse)// || !target->takedamage)
		return true;
	// an indestructable entity is blocking us
	if (!target->takedamage)
		return false;
	// a teammate is blocking us
	if (OnSameTeam(self, target))
		return false;
	// an enemy is in our path--keep falling!
	return true;
}

qboolean magmine_findtarget(edict_t* self, float range, int pull);
void magmine_throwsparks(edict_t* self);

void obstacle_attack(edict_t* self)
{
	vec3_t end;
	trace_t	tr;

	if (self->light_level > 0)
	{
		if (magmine_findtarget(self, self->monsterinfo.sight_range, self->count))
		{
			magmine_throwsparks(self);
			self->monsterinfo.idle_frames = 0;
		}
	}

	// must be flipped
	if (self->s.angles[PITCH] != 180)
		return;

	// obstacle is currently attached to ceiling, so look for a valid target to fall on
	if (self->movetype == MOVETYPE_NONE)
	{
		VectorCopy(self->s.origin, end);
		VectorCopy(end, self->move_origin); // save current position
		end[2] -= 8192;
		tr = gi.trace(self->s.origin, self->mins, self->maxs, end, self, MASK_SHOT);
		if (G_ValidTarget(self, tr.ent, false, true))
			self->movetype = MOVETYPE_STEP;
	}
	else
	{
		// obstacle is falling
		// trace to the floor to determine the floor Z height where we would make contact
		VectorCopy(self->s.origin, end);
		end[2] -= 8192;
		tr = gi.trace(self->s.origin, self->mins, self->maxs, end, self, MASK_SOLID);
		// have we reached the floor?
		if (self->s.origin[2] == tr.endpos[2])
		{
			// we've hit the ground, so move back to the starting position
			obstacle_return(self);
		}
		else
		{
			// floor position, i.e. the lowest point we can go before touching the ground
			VectorCopy(tr.endpos, end); 
			// trace again looking for something to hurt
			tr = gi.trace(self->s.origin, self->mins, self->maxs, end, self, MASK_SHOT);
			// are we touching it?
			if (self->s.origin[2] == tr.endpos[2])
			{
				// is path to floor obstructed by something that we can't hurt?
				if (!obstacle_attack_clear(self, tr.ent))
				{
					// return to starting position
					obstacle_return(self);
				}
				else
				{
					// hurt it!
					obstacle_touch(self, tr.ent, &tr.plane, tr.surface);
				}
			}
		}

	}
}

void obstacle_think (edict_t *self)
{
	if (!organ_checkowner(self) || !organ_checkrevived(self))
		return;

	vrx_set_pickup_owner(self); // sets owner when this entity is picked up, making it non-solid to the player
	organ_restoreMoveType(self); // needed to restore movetype from MOVETYPE_NONE to MOVETYPE_STEP after being pushed!

	if (!que_typeexists(self->curses, CURSE_FROZEN))
	{
		obstacle_cloak(self);
		obstacle_attack(self);
		obstacle_move(self);
		V_HealthCache(self, (int)(0.2 * self->max_health), 1);
	}

//	if (level.time > self->lasthurt + 1.0)
//		M_Regenerate(self, 300, 10, true, false, false, &self->monsterinfo.regen_delay1);

	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);

	self->nextthink = level.time + FRAMETIME;
}

void obstacle_grow (edict_t *self)
{
	if (!organ_checkowner(self) || !organ_checkrevived(self))
		return;
	//gi.dprintf("movetype: %d pitch: %.0f\n", self->movetype, self->s.angles[PITCH]);
	//organ_restoreMoveType(self);

	vrx_set_pickup_owner(self); // sets owner when this entity is picked up, making it non-solid to the player

	V_HealthCache(self, (int)(0.2 * self->max_health), 1);

	// if position has been updated, check for ground entity
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	// don't slide
	if (self->groundentity && level.time > self->touch_debounce_time)
		VectorClear(self->velocity);
	
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);

	self->s.effects |= EF_PLASMA;

	self->nextthink = level.time + FRAMETIME;

	if (self->s.frame == OBSTACLE_FRAMES_GROW_END && self->health >= self->max_health)
	{
		self->style = 0;//done growing
		self->s.frame = OBSTACLE_FRAME_READY;
		self->think = obstacle_think;
		self->touch = obstacle_touch;
		return;
	}

	if (self->s.frame == OBSTACLE_FRAMES_GROW_START)
		gi.sound(self, CHAN_VOICE, gi.soundindex("organ/organe3.wav"), 1, ATTN_NORM, 0);

	if (self->s.frame != OBSTACLE_FRAMES_GROW_END)
		G_RunFrames(self, OBSTACLE_FRAMES_GROW_START, OBSTACLE_FRAMES_GROW_END, false);
}

edict_t *CreateObstacle (edict_t *ent, int skill_level, int talent_level)
{
	edict_t *e;

	e = G_Spawn();
	e->style = 1; //growing
	e->activator = ent;
	e->think = obstacle_grow;
	e->nextthink = level.time + FRAMETIME;
	e->s.modelindex = gi.modelindex ("models/objects/organ/obstacle/tris.md2");
	e->s.renderfx |= RF_IR_VISIBLE;
	e->solid = SOLID_BBOX;
	e->movetype = MOVETYPE_STEP;
	e->svflags |= SVF_MONSTER;
	e->clipmask = MASK_MONSTERSOLID;
	e->mass = 500;
	e->classname = "obstacle";
	e->takedamage = DAMAGE_AIM;
	e->max_health = OBSTACLE_INITIAL_HEALTH + OBSTACLE_ADDON_HEALTH * skill_level;
	e->health = 0.5*e->max_health;
	e->dmg = OBSTACLE_INITIAL_DAMAGE + OBSTACLE_ADDON_DAMAGE * skill_level;
	e->monsterinfo.nextattack = 100;// -9 * vrx_get_talent_level(ent, TALENT_PHANTOM_OBSTACLE);
	e->monsterinfo.level = skill_level;
	if (talent_level)
	{
		e->light_level = talent_level; // Talent: Magnetism
		e->monsterinfo.sight_range = (0.2 * MAGMINE_RANGE) * talent_level; // range
		e->count = MAGMINE_DEFAULT_PULL + (2 * MAGMINE_ADDON_PULL * talent_level); // pull
		e->radius_dmg = e->dmg; // for bot AI hazard detection
		e->dmg_radius = e->monsterinfo.sight_range; // for bot AI hazard detection
		//gi.dprintf("magnetism: level: %d range: %.0f pull: %d\n", e->light_level, e->monsterinfo.sight_range, e->style);
	}
	e->gib_health = -2 * BASE_GIB_HEALTH;
	e->die = obstacle_die;
	e->touch = organ_touch;
	VectorSet(e->mins, -16, -16, 0);
	VectorSet(e->maxs, 16, 16, 40);
	e->mtype = M_OBSTACLE;

	ent->num_obstacle++;

	return e;
}

void Cmd_Obstacle_f (edict_t *ent)
{
	int		cost = OBSTACLE_COST;
	edict_t *obstacle;
	vec3_t	start, angles;

	if (Q_strcasecmp (gi.args(), "remove") == 0)
	{
		organ_removeall(ent, "obstacle", true);
		safe_cprintf(ent, PRINT_HIGH, "Obstacles removed\n");
		ent->num_obstacle = 0;
		return;
	}

	if (vrx_toggle_pickup(ent, M_OBSTACLE, 128)) // search for entity to pick up, or drop the one we're holding
		return;

	if (ctf->value && (CTF_DistanceFromBase(ent, NULL, CTF_GetEnemyTeam(ent->teamnum)) < CTF_BASE_DEFEND_RANGE))
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't build in enemy base!\n");
		return;
	}

	// cost is doubled if you are a flyer or cacodemon below skill level 5
	if ((ent->mtype == MORPH_FLYER && ent->myskills.abilities[FLYER].current_level < 5) 
		|| (ent->mtype == MORPH_CACODEMON && ent->myskills.abilities[CACODEMON].current_level < 5))
		cost *= 2;

	if (!V_CanUseAbilities(ent, OBSTACLE, cost, true))
		return;

	if (ent->num_obstacle >= OBSTACLE_MAX_COUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "You have reached the maximum amount of obstacles (%d)\n", (int)OBSTACLE_MAX_COUNT);
		return;
	}

	obstacle = CreateObstacle(ent, ent->myskills.abilities[OBSTACLE].current_level, vrx_get_talent_level(ent, TALENT_MAGNETISM));
	if (!G_GetSpawnLocation(ent, 100, obstacle->mins, obstacle->maxs, start, angles, PROJECT_HITBOX_FAR, false))
	{
		ent->num_obstacle--;
		G_FreeEdict(obstacle);
		return;
	}

	// calculate angles from plane.normal of wall
	vectoangles(angles, angles);
	AngleCheck(&angles[PITCH]);// 90 = ceiling, 270 = floor
	if (angles[PITCH] == 90)
	{
		VectorCopy(angles, obstacle->s.angles);
		obstacle->movetype = MOVETYPE_NONE;
		// flip obstacle over
		obstacle->s.angles[PITCH] = 180;
		// flip bounding box
		obstacle->mins[2] = -40;
		obstacle->maxs[2] = 0;
		// move starting position Z height
		start[2] += 40;
	}
	else
	{
		VectorCopy(ent->s.angles, obstacle->s.angles);
		obstacle->s.angles[PITCH] = 0;
	}

	//AngleCheck(&angles[YAW]);
	//AngleCheck(&angles[ROLL]);
	//gi.dprintf("angles %.0f %.0f %.0f\n", angles[PITCH], angles[YAW], angles[ROLL]);

	VectorCopy(start, obstacle->s.origin);

	if (ent->client)
	{
		layout_add_tracked_entity(&ent->client->layout, obstacle);
		AI_EnemyAdded(obstacle);
		// pick it up!
		ent->client->pickup = obstacle;
	}

	gi.linkentity(obstacle);
	obstacle->monsterinfo.cost = cost;

	safe_cprintf(ent, PRINT_HIGH, "Obstacle created (%d/%d)\n", ent->num_obstacle,(int)OBSTACLE_MAX_COUNT);

	ent->client->pers.inventory[power_cube_index] -= cost;
	ent->client->ability_delay = level.time + OBSTACLE_DELAY;
	//ent->holdtime = level.time + OBSTACLE_DELAY;

	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}

#define GASSER_FRAMES_ATTACK_START		1	
#define GASSER_FRAMES_ATTACK_END		6
#define GASSER_FRAMES_REARM_START		7
#define GASSER_FRAMES_REARM_END			8
#define GASSER_FRAMES_IDLE_START		9
#define GASSER_FRAMES_IDLE_END			12
#define GASSER_FRAME_DEAD				6

#define GASCLOUD_FRAMES_GROW_START		10
#define GASCLOUD_FRAMES_GROW_END		18
#define GASCLOUD_FRAMES_IDLE_START		19
#define GASCLOUD_FRAMES_IDLE_END		23

void poison_think (edict_t *self)
{
	if (!G_EntIsAlive(self->enemy) || !G_EntIsAlive(self->activator) || (level.time > self->delay))
	{
		//gi.dprintf("poison did %d damage\n", self->dmg_counter);
		G_FreeEdict(self);
		return;
	}

	if (level.framenum >= self->monsterinfo.nextattack)
	{
		//self->dmg_counter += self->dmg;
		T_Damage(self->enemy, self, self->activator, vec3_origin, self->enemy->s.origin, vec3_origin, self->dmg, 0, 0, self->style);
		self->monsterinfo.nextattack = level.framenum + floattoint(self->random);
		self->random *= 1.25;
	}

	self->nextthink = level.time + FRAMETIME;
}

void CreatePoison (edict_t *ent, edict_t *targ, int damage, float duration, int meansOfDeath)
{
	edict_t *e;

	e = G_Spawn();
	e->activator = ent;
	e->solid = SOLID_NOT;
	e->movetype = MOVETYPE_NOCLIP;
	e->svflags |= SVF_NOCLIENT;
	e->classname = "poison";
	e->delay = level.time + duration;
	e->owner = e->enemy = targ;
	e->random = 1; // starting refire delay (in frames)
	e->dmg = damage;
	e->mtype = e->atype = POISON;
	e->style = meansOfDeath;
	e->think = poison_think;
	e->nextthink = level.time + FRAMETIME;
	VectorCopy(targ->s.origin, e->s.origin);
	gi.linkentity(e);

	if (!que_addent(targ->curses, e, duration))
		G_FreeEdict(e);
}

void gascloud_sparks (edict_t *self, int num, int radius)
{
	int		i;
	vec3_t	start;

	// 0 = black, 8 = grey, 15 = white, 16 = light brown, 20 = brown, 57 = light orange, 66 = orange/red, 73 = maroon
	// 106 = pink, 113 = light blue, 119 = blue, 123 = dark blue, 200 = pale green, 205 = dark green, 209 = bright green
	// 217 = white, 220 = yellow, 226 = orange, 231 = red/orange, 240 = red, 243 = dark blue

	for (i=0; i<num; i++)
	{
		VectorCopy(self->s.origin, start);
		start[0] += crandom() * GetRandom(0, radius);
		start[1] += crandom() * GetRandom(0, radius);
		start[2] += crandom() * GetRandom(0, radius);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_LASER_SPARKS);
		gi.WriteByte(1); // number of sparks
		gi.WritePosition(start);
		gi.WriteDir(vec3_origin);
		gi.WriteByte(GetRandom(200, 209)); // color
		gi.multicast(start, MULTICAST_PVS);
	}
}

void gascloud_runframes (edict_t *self)
{
	if (level.time > self->delay - 0.8)
	{
		G_RunFrames(self, GASCLOUD_FRAMES_GROW_START, GASCLOUD_FRAMES_GROW_END, true);
		self->s.effects |= EF_SPHERETRANS;
	}
	else if (self->s.frame < GASCLOUD_FRAMES_GROW_END)
		G_RunFrames(self, GASCLOUD_FRAMES_GROW_START, GASCLOUD_FRAMES_GROW_END, false);
}

void poison_curse_sound(edict_t* self)
{
	switch (GetRandom(1, 4))
	{
	case 1: gi.sound(self, CHAN_ITEM, poison_sound1, 1.0, ATTN_NORM, 0); break;
	case 2: gi.sound(self, CHAN_ITEM, poison_sound2, 1.0, ATTN_NORM, 0); break;
	case 3: gi.sound(self, CHAN_ITEM, poison_sound3, 1.0, ATTN_NORM, 0); break;
	case 4: gi.sound(self, CHAN_ITEM, poison_sound4, 1.0, ATTN_NORM, 0); break;
	}
}

void poison_target(edict_t* ent, edict_t* target, int damage, float duration, int meansOfdeath, qboolean stack)
{
	que_t* slot = NULL;

	// is there an existing poison curse?
	//if ((slot = que_findtype(target->curses, NULL, POISON)) != NULL)
	while ((slot = que_findtype(target->curses, slot, POISON)) != NULL)
	{
		if (!stack) // stacking of poison curses is not allowed, so refresh the curse instead
		{
			slot->ent->random = 1; // initial refire delay for next attack
			slot->ent->monsterinfo.nextattack = level.framenum + 1; // next attack server frame
			slot->ent->delay = level.time + duration;
			slot->time = level.time + duration;
			return;
		}
		else if (slot->time - level.time >= 9)
			return; // only allow 1 poison curse per second
		//gi.dprintf("prev poison time %.1f\n", slot->time - level.time);
	}
	// if target is not already poisoned, create poison entity

	CreatePoison(ent, target, damage, duration, meansOfdeath);
	poison_curse_sound(target);
	//gi.dprintf("poisoned at %.1f\n", level.time);


	//gi.sound(target, CHAN_ITEM, gi.soundindex("curses/poison.wav"), 1, ATTN_NORM, 0);
	
}

void gascloud_attack (edict_t *self)
{
	que_t	*slot=NULL;
	edict_t *e=NULL;

	while ((e = findradius(e, self->s.origin, self->dmg_radius)) != NULL)
	{
		if (G_ValidTarget(self, e, true, true))
		{
			poison_target(self->activator, e, (int)(GASCLOUD_POISON_FACTOR * self->dmg), self->radius_dmg, MOD_GAS, true);
			T_Damage(e, self, self->activator, vec3_origin, e->s.origin, vec3_origin, (int)((1.0-GASCLOUD_POISON_FACTOR)*self->dmg), 0, 0, MOD_GAS);
		}
		else if (e && e->inuse && e->takedamage && visible(self, e) && !OnSameTeam(self, e))
			T_Damage(e, self, self->activator, vec3_origin, e->s.origin, vec3_origin, self->dmg, 0, 0, MOD_GAS);
	}
}

void gascloud_move (edict_t *self)
{
	trace_t tr;
	vec3_t	start;

	VectorCopy(self->s.origin, start);
	start[2]++;
	tr = gi.trace(self->s.origin, NULL, NULL, start, NULL, MASK_SOLID);
	if (tr.fraction == 1.0)
	{
		self->s.origin[2]++;
		gi.linkentity(self);
	}
}

void gascloud_think (edict_t *self)
{
	if (!G_EntIsAlive(self->activator) || level.time > self->delay)
	{
		G_FreeEdict(self);
		return;
	}
	
	gascloud_sparks(self, 3, (int)self->dmg_radius);
	gascloud_attack(self);
	gascloud_move(self);
	gascloud_runframes(self);
	
	self->nextthink = level.time + FRAMETIME;	
}

void SpawnGasCloud (edict_t *ent, vec3_t start, int damage, float radius, float duration)
{
	edict_t *e;

	// initialize sound
	poison_sound1 = gi.soundindex("curses/green1.wav");
	poison_sound2 = gi.soundindex("curses/green2.wav");
	poison_sound3 = gi.soundindex("curses/green3.wav");
	poison_sound4 = gi.soundindex("curses/green4.wav");

	// spawn gascloud entity
	e = G_Spawn();
	e->activator = ent->activator;
	e->solid = SOLID_NOT;
	e->movetype = MOVETYPE_NOCLIP;
	e->classname = "gas cloud";
	e->delay = level.time + duration;
	e->radius_dmg = GASCLOUD_POISON_DURATION; // poison effect duration
	e->dmg = damage;
	e->dmg_radius = radius;
	e->think = gascloud_think;
	e->s.modelindex = gi.modelindex ("models/objects/smokexp/tris.md2");
	e->s.skinnum = 1;
	e->s.effects |= EF_PLASMA;
	e->nextthink = level.time + FRAMETIME;
	VectorCopy(start, e->s.origin);
	VectorCopy(ent->s.angles, e->s.angles);
	e->s.angles[PITCH] = 0;//always upright
	gi.linkentity(e);
}

void poisonball_remove(edict_t* self)
{
	// remove poisonball entity next frame
	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAMETIME;
}

//FIXME: these effects don't appear to be working on impact
void poisonball_explode_effects(edict_t* self, cplane_t* plane)
{
	int i;
	vec3_t forward;

	for (i = 0; i < 5; i++)
	{
		if (plane)
		{
			VectorCopy(plane->normal, forward);
		}
		else
		{
			VectorNegate(self->velocity, forward);
			VectorNormalize(forward);
		}

		// randomize aiming vector
		forward[YAW] += 0.5 * crandom();
		forward[PITCH] += 0.5 * crandom();
		
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_LASER_SPARKS);
		gi.WriteByte(1); // number of sparks
		gi.WritePosition(self->s.origin);
		gi.WriteDir(forward);
		gi.WriteByte(205); // color
		gi.multicast(self->s.origin, MULTICAST_PVS);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_LASER_SPARKS);
		gi.WriteByte(1); // number of sparks
		gi.WritePosition(self->s.origin);
		gi.WriteDir(forward);
		gi.WriteByte(209); // color
		gi.multicast(self->s.origin, MULTICAST_PVS);
	}
}

void tempent_ball_touch(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	// remove if we touch a sky brush
	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}
}

void tempent_ball_think(edict_t* self)
{
	// remove if entity times out
	if (level.time > self->delay)
	{
		G_FreeEdict(self);
		return;
	}

	G_RunFrames(self, 0, 7, false);

	self->nextthink = level.time + FRAMETIME;
}

void tempent_ball_explode(vec3_t start, int skinnum, vec3_t dir, float speed, float delay)
{
	edict_t* ball;
	vec3_t angles;

	// spawn ball entity
	ball = G_Spawn();
	VectorCopy(start, ball->s.origin);
	ball->movetype = MOVETYPE_FLYMISSILE;
	ball->clipmask = MASK_SHOT;
	ball->solid = SOLID_BBOX;
	ball->s.effects |= EF_PLASMA;
	ball->s.modelindex = gi.modelindex("models/proj/minoball/tris.md2");
	ball->s.skinnum = skinnum;
	//poison->owner = self;
	ball->touch = tempent_ball_touch;
	ball->think = tempent_ball_think;
	ball->delay = level.time + delay;
	ball->classname = "te_ball_ex";
	gi.linkentity(ball);
	ball->nextthink = level.time + FRAMETIME;

	// get angles
	vectoangles(dir, angles);
	VectorCopy(dir, ball->s.angles);
	// get directional vectors
	//AngleVectors(dir, NULL, NULL, up);
	// adjust velocity
	VectorScale(dir, speed, ball->velocity);
}

void poisonball_explode(edict_t* self, cplane_t* plane)
{
	vec3_t	up;
	edict_t* e=NULL;

	// find targets
	while ((e = findradius(e, self->s.origin, self->dmg_radius)) != NULL)
	{
		if (!G_ValidTarget(self, e, true, true))
			continue;
		// poison target
		poison_target(self->owner, e, self->dmg, 10.0, MOD_GAS, true);
		// apply impact damage, if any
		if (self->radius_dmg)
			T_Damage(e, self, self->owner, vec3_origin, e->s.origin, vec3_origin, self->radius_dmg, 0, 0, MOD_GAS);
	}

	// spawn a gas cloud on impact?
	if (self->sucking)
		SpawnGasCloud(self->owner, self->s.origin, self->dmg, 128, 4.0);

	// impact particle effect
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BLASTER2);
	gi.WritePosition(self->s.origin);
	if (!plane)
		gi.WriteDir(vec3_origin);
	else
		gi.WriteDir(plane->normal);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	// create temporary entity that slowly moves up
	AngleVectors(self->s.angles, NULL, NULL, up);
	tempent_ball_explode(self->s.origin, 1, up, 25, 0.7);

	poisonball_remove(self);
	self->exploded = true;
}

void poisonball_touch(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	if (ent->exploded)
		return;

	// remove poisonball if owner dies or becomes invalid or if we touch a sky brush
	if (!G_EntIsAlive(ent->owner) || (surf && (surf->flags & SURF_SKY)))
	{
		poisonball_remove(ent);
		return;
	}

	if (other == ent->owner)
		return;

	//gi.sound(ent, CHAN_VOICE, gi.soundindex(va("abilities/largefireimpact%d.wav", GetRandom(1, 3))), 1, ATTN_NORM, 0);
	gi.sound(ent, CHAN_ITEM, sound_poisoned, 1, ATTN_NORM, 0);
	poisonball_explode(ent, plane);
}

void poisonball_think(edict_t* self)
{
	//int i;
	//vec3_t start, forward;

	if (level.time > self->delay || self->waterlevel)
	{
		G_FreeEdict(self);
		return;
	}

	// 0 = black, 8 = grey, 15 = white, 16 = light brown, 20 = brown, 57 = light orange, 66 = orange/red, 73 = maroon
	// 106 = pink, 113 = light blue, 119 = blue, 123 = dark blue, 200 = pale green, 205 = dark green, 209 = bright green
	// 217 = white, 220 = yellow, 226 = orange, 231 = red/orange, 240 = red, 243 = dark blue
	/*
	VectorCopy(self->s.origin, start);
	VectorCopy(self->velocity, forward);
	VectorNormalize(forward);

	// create a trail behind the fireball
	for (i = 0; i < 6; i++)
	{

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_LASER_SPARKS);
		gi.WriteByte(1); // number of sparks
		gi.WritePosition(start);
		gi.WriteDir(forward);
		gi.WriteByte(205); // color
		gi.multicast(start, MULTICAST_PVS);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_LASER_SPARKS);
		gi.WriteByte(1); // number of sparks
		gi.WritePosition(start);
		gi.WriteDir(forward);
		gi.WriteByte(209); // color
		gi.multicast(start, MULTICAST_PVS);

		VectorMA(start, -16, forward, start);
	}*/

	self->nextthink = level.time + FRAMETIME;
}

void fire_poison(edict_t* self, vec3_t start, vec3_t aimdir, int impact_dmg, float impact_rad, int speed, int poison_dmg, int poison_dur, qboolean cloud)
{
	edict_t* poison;
	vec3_t	dir;
	vec3_t	forward;

	// initialize sound
	sound_poisoned = gi.soundindex("abilities/poisoned.wav");
	sound_poisonnova = gi.soundindex("abilities/poisoned.wav");

	//  entity made a sound, used to alert monsters
	self->lastsound = level.framenum;

	// get aiming angles
	vectoangles(aimdir, dir);
	// get directional vectors
	AngleVectors(dir, forward, NULL, NULL);

	// spawn grenade entity
	poison = G_Spawn();
	VectorCopy(start, poison->s.origin);
	poison->movetype = MOVETYPE_FLYMISSILE;
	poison->clipmask = MASK_SHOT;
	poison->solid = SOLID_BBOX;
	poison->s.effects |= EF_PLASMA|EF_BLASTER|EF_TRACKER|EF_COLOR_SHELL;
	//poison->s.effects |= (EF_PLASMA | EF_COLOR_SHELL);
	poison->s.renderfx |= RF_SHELL_GREEN;
	//poison->s.modelindex = gi.modelindex("models/proj/minopoison/tris.md2");
	poison->s.modelindex = gi.modelindex("models/objects/spore/tris.md2");
	poison->s.skinnum = 1;
	poison->owner = self;
	poison->activator = G_GetSummoner(self); // needed for OnSameTeam() checks
	poison->touch = poisonball_touch;
	poison->think = poisonball_think;
	poison->dmg_radius = impact_rad; // radius of impact
	poison->dmg = poison_dmg; // poison damage over time from poison curse
	poison->radius_dmg = impact_dmg; // instantaneous damage caused on impact
	poison->sucking = cloud; // spawn poison cloud on immpact?
	poison->classname = "poisonball";
	poison->delay = level.time + 10.0;
	gi.linkentity(poison);
	poison->nextthink = level.time + FRAMETIME;
	VectorCopy(dir, poison->s.angles);

	// adjust velocity
	VectorScale(aimdir, speed, poison->velocity);
	//poison->velocity[2] += 150;

	gi.sound(poison, CHAN_WEAPON, sound_poisonnova, 1, ATTN_NORM, 0);
}


void fire_acid (edict_t *self, vec3_t start, vec3_t aimdir, int projectile_damage, float radius, 
				int speed, int acid_damage, float acid_duration);

void gasser_acidattack (edict_t *self)
{
	float	dist;
	//float	range=self->monsterinfo.sight_range;
	int		speed= ACID_INITIAL_SPEED;
	vec3_t	forward, start, end;
	edict_t *e=NULL;

	if (self->monsterinfo.attack_finished > level.time)
		return;

	VectorCopy(self->s.origin, start);
	start[2] = self->absmax[2] + 1;// -16;

	e = self->enemy;
	//while ((e = findradius(e, self->s.origin, range)) != NULL)
	//{
	if (!G_ValidTarget(self, e, true, true))
		return;
		//	continue;
		
		// copy target location
		G_EntMidPoint(e, end);

		// calculate distance to target
		dist = distance(e->s.origin, start);

		// move our target point based on projectile and enemy velocity
		VectorMA(end, (float)dist/speed, e->velocity, end);
				
		// calculate direction vector to target
		VectorSubtract(end, start, forward);
		VectorNormalize(forward);

		fire_acid(self, start, forward, self->radius_dmg, ACID_INITIAL_RADIUS, speed, (0.1*self->radius_dmg), ACID_DURATION);
		
		//FIXME: only need to do this once
		self->monsterinfo.attack_finished = level.time + (3.0 - (0.5 * self->light_level)); // talent level reduces attack delay
		self->s.frame = GASSER_FRAMES_ATTACK_END-2;
		//gi.sound (self, CHAN_VOICE, gi.soundindex ("weapons/twang.wav"), 1, ATTN_NORM, 0);
	//}	
}

void gasser_attack (edict_t *self)
{
	vec3_t start;

	if (self->monsterinfo.attack_finished > level.time)
		return;

	VectorCopy(self->s.origin, start);
	start[2] = self->absmax[2] + 8;
	SpawnGasCloud(self, start, self->dmg, self->dmg_radius, 4.0);
	//gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/poisonloopmedium2.wav"), 1, ATTN_NORM, 0);
	gi.sound(self, CHAN_VOICE, sound_gas, 1, ATTN_NORM, 0);

	self->s.frame = GASSER_FRAMES_ATTACK_START+2;
	self->monsterinfo.attack_finished = level.time + GASSER_REFIRE;
}

qboolean gasser_findtarget (edict_t *self)
{
	edict_t *e=NULL;

	while ((e = findclosestradius_targets(e, self, self->monsterinfo.sight_range)) != NULL)
	{
		if (!G_ValidTarget_Lite(self, e, true))
			continue;
		//gi.dprintf("gasser found a target\n");
		self->enemy = e;
		return true;
	}
	//gi.dprintf("can't find a target %.0f\n", self->monsterinfo.sight_range);
	self->enemy = NULL;
	return false;
}

void gasser_think (edict_t *self)
{
	if (!organ_checkowner(self) || !organ_checkrevived(self))
		return;

	vrx_set_pickup_owner(self); // sets owner when this entity is picked up, making it non-solid to the player

	// don't attack while picked up
	if (self->flags & FL_PICKUP)
		self->monsterinfo.attack_finished = level.time + 1.0;

	//organ_restoreMoveType(self);

//	if (level.time > self->lasthurt + 1.0)
//		M_Regenerate(self, 300, 10, true, false, false, &self->monsterinfo.regen_delay1);

	if (!que_typeexists(self->curses, CURSE_FROZEN))
	{
		V_HealthCache(self, (int)(0.2 * self->max_health), 1);

		if (level.time > self->monsterinfo.attack_finished && gasser_findtarget(self))
		{
			//gi.dprintf("gasser wants to attack\n");
			if (entdist(self, self->enemy) < self->dmg_radius)
				gasser_attack(self);
			else if (self->light_level)
				gasser_acidattack(self);
		}
		//else
			//gi.dprintf("cant attack\n");

		if (self->s.frame < GASSER_FRAMES_ATTACK_END)
			G_RunFrames(self, GASSER_FRAMES_ATTACK_START, GASSER_FRAMES_ATTACK_END, false);
		else if (self->s.frame < GASSER_FRAMES_REARM_END && level.time > self->monsterinfo.attack_finished - 0.2)
			G_RunFrames(self, GASSER_FRAMES_REARM_START, GASSER_FRAMES_REARM_END, false);
		else if (level.time > self->monsterinfo.attack_finished)
		{
			gascloud_sparks(self, 1, 32);
			G_RunFrames(self, GASSER_FRAMES_IDLE_START, GASSER_FRAMES_IDLE_END, false);
		}
	}

	// if position has been updated, check for ground entity
	// note: this is checked every frame if using MOVETYPE_STEP, whereas MOVETYPE_TOSS may require gi.linkentity to be run on velocity/position changes
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		//gi.dprintf("position updated, check for ground\n");
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	// ground friction to prevent excessive sliding
	if (self->groundentity)
	{
		self->velocity[0] *= 0.5;
		self->velocity[1] *= 0.5;
	}
	
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);
	//gi.linkentity(self);

	self->nextthink = level.time + FRAMETIME;
}

void gasser_grow (edict_t *self)
{
	if (!organ_checkowner(self) || !organ_checkrevived(self))
		return;

	vrx_set_pickup_owner(self); // sets owner when this entity is picked up, making it non-solid to the player

	//organ_restoreMoveType(self);

	V_HealthCache(self, (int)(0.2 * self->max_health), 1);

	// if position has been updated, check for ground entity
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	// ground friction to prevent excessive sliding
	if (self->groundentity)
	{
		self->velocity[0] *= 0.5;
		self->velocity[1] *= 0.5;
	}
	
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);

	self->s.effects |= EF_PLASMA;

	if (self->health >= self->max_health)
	{
		self->style = 0;//done growing
		self->think = gasser_think;
	}

	self->nextthink = level.time + FRAMETIME;
}

void gasser_dead (edict_t *self)
{
	if (level.time > self->delay)
	{
		organ_remove(self, false);
		return;
	}

	if (level.time == self->delay - 5)
		self->s.effects |= EF_PLASMA;
	else if (level.time == self->delay - 2)
		self->s.effects |= EF_SPHERETRANS;

	self->nextthink = level.time + FRAMETIME;
}

void gasser_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int max = GASSER_MAX_COUNT;
	int cur;

	if (!self->monsterinfo.slots_freed && self->activator && self->activator->inuse)
	{
		organ_death_cleanup(self, attacker, &self->activator->num_gasser, GASSER_MAX_COUNT, true);
	}

	if (self->health <= self->gib_health /* || organ_explode(self)*/)
	{
		int n;

		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < 2; n++)
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n= 0; n < 4; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		organ_remove(self, false);
		return;
	}

	if (self->deadflag == DEAD_DYING)
		return;

	self->think = gasser_dead;
	self->deadflag = DEAD_DYING;
	self->delay = level.time + 20.0;
	self->nextthink = level.time + FRAMETIME;
	self->s.frame = GASSER_FRAME_DEAD;
	self->movetype = MOVETYPE_TOSS;

	// flip gasser upright
	self->maxs[2] = 6;
	self->s.angles[PITCH] = 0;

	gi.linkentity(self);
}

edict_t *CreateGasser (edict_t *ent, int skill_level, int talent_level)
{
	float synergy_bonus = vrx_get_synergy_mult(ent, GASSER);
	edict_t *e;

	// initialize sound
	sound_gas = gi.soundindex("weapons/gas1.wav");

	e = G_Spawn();
	e->style = 1; //growing
	e->activator = ent;
	e->think = gasser_grow;
	e->nextthink = level.time + FRAMETIME;
	e->s.modelindex = gi.modelindex ("models/objects/organ/gas/tris.md2");
	e->s.renderfx |= RF_IR_VISIBLE;
	e->solid = SOLID_BBOX;
	e->monsterinfo.touchdown = M_Touchdown;
	e->movetype = MOVETYPE_STEP;
	e->clipmask = MASK_MONSTERSOLID;
	e->svflags |= SVF_MONSTER;// tells physics to clip on any solid object (not just walls)
	e->mass = 500;
	e->classname = "gasser";
	e->takedamage = DAMAGE_AIM;
	e->max_health = GASSER_INITIAL_HEALTH + GASSER_ADDON_HEALTH * skill_level;
	e->health = 0.5*e->max_health;
	e->dmg = (GASSER_INITIAL_DAMAGE + GASSER_ADDON_DAMAGE * skill_level) * synergy_bonus;
	e->dmg_radius = GASSER_INITIAL_ATTACK_RANGE + GASSER_ADDON_ATTACK_RANGE * skill_level;
	e->monsterinfo.level = skill_level;

	e->light_level = talent_level; // Talent: Spitting Gasser
	if (talent_level)
	{
		e->radius_dmg = (ACID_INITIAL_DAMAGE + ACID_ADDON_DAMAGE * skill_level) * synergy_bonus;
		e->monsterinfo.sight_range = GASSER_ACID_RANGE;
	}
	else
		e->monsterinfo.sight_range = GASSER_RANGE;

	e->gib_health = -1.25 * BASE_GIB_HEALTH;
	e->s.frame = GASSER_FRAMES_IDLE_START;
	e->die = gasser_die;
	e->touch = organ_touch;
	VectorSet(e->mins, -6, -6, -5);
	VectorSet(e->maxs, 6, 6, 15);
	e->mtype = M_GASSER;

	ent->num_gasser++;

	return e;
}

void Cmd_Gasser_f (edict_t *ent)
{
	int		cost = GASSER_COST;
	edict_t *gasser;
	vec3_t	start;

	if (Q_strcasecmp (gi.args(), "remove") == 0)
	{
		organ_removeall(ent, "gasser", true);
		safe_cprintf(ent, PRINT_HIGH, "Gassers removed\n");
		ent->num_gasser = 0;
		return;
	}

	if (vrx_toggle_pickup(ent, M_GASSER, 128)) // search for entity to pick up, or drop the one we're holding
		return;

	if (ctf->value && (CTF_DistanceFromBase(ent, NULL, CTF_GetEnemyTeam(ent->teamnum)) < CTF_BASE_DEFEND_RANGE))
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't build in enemy base!\n");
		return;
	}

	// cost is doubled if you are a flyer or cacodemon below skill level 5
	if ((ent->mtype == MORPH_FLYER && ent->myskills.abilities[FLYER].current_level < 5) 
		|| (ent->mtype == MORPH_CACODEMON && ent->myskills.abilities[CACODEMON].current_level < 5))
		cost *= 2;

	if (!V_CanUseAbilities(ent, GASSER, cost, true))
		return;

	if (ent->num_gasser >= (int)GASSER_MAX_COUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "You have reached the maximum amount of gassers (%d)\n", (int)GASSER_MAX_COUNT);
		return;
	}

	gasser = CreateGasser(ent, ent->myskills.abilities[GASSER].current_level, vrx_get_talent_level(ent, TALENT_SPITTING_GASSER)); // Talent: Spitting Gasser
	if (!G_GetSpawnLocation(ent, 100, gasser->mins, gasser->maxs, start, NULL, PROJECT_HITBOX_FAR, false))
	{
		ent->num_gasser--;
		G_FreeEdict(gasser);
		return;
	}
	VectorCopy(start, gasser->s.origin);
	VectorCopy(ent->s.angles, gasser->s.angles);
	gasser->s.angles[PITCH] = 0;
	gi.linkentity(gasser);
	gasser->monsterinfo.attack_finished = level.time + 1.0;
	gasser->monsterinfo.cost = cost;

	if (ent->client)
	{
		layout_add_tracked_entity(&ent->client->layout, gasser);
		AI_EnemyAdded(gasser);
		// pick it up!
		ent->client->pickup = gasser;
	}

	safe_cprintf(ent, PRINT_HIGH, "Gasser created (%d/%d)\n", ent->num_gasser, (int)GASSER_MAX_COUNT);

	ent->client->pers.inventory[power_cube_index] -= cost;
	ent->client->ability_delay = level.time + GASSER_DELAY;
	//ent->holdtime = level.time + GASSER_DELAY;

	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}


#define COCOON_FRAMES_IDLE_START	73//36
#define COCOON_FRAMES_IDLE_END		87//50
#define COCOON_FRAME_STANDBY		89//52
#define COCOON_FRAMES_GROW_START	90//53
#define COCOON_FRAMES_GROW_END		108//72

void cocoon_dead (edict_t *self)
{
	if (level.time > self->delay)
	{
		organ_remove(self, false);
		return;
	}

	if (level.time == self->delay - 5)
		self->s.effects |= EF_PLASMA;
	else if (level.time == self->delay - 2)
		self->s.effects |= EF_SPHERETRANS;

	self->nextthink = level.time + FRAMETIME;
}

void cocoon_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (!self->monsterinfo.slots_freed && self->activator && self->activator->inuse)
	{
		self->activator->cocoon = NULL;
		self->monsterinfo.slots_freed = true;
		AI_EnemyRemoved(self);

		if (PM_MonsterHasPilot(attacker))
			attacker = attacker->owner;

		if (self->activator->client)
			layout_remove_tracked_entity(&self->activator->client->layout, self);

		if (attacker->client)
			safe_cprintf(self->activator, PRINT_HIGH, "Your cocoon was killed by %s\n", attacker->client->pers.netname);
		else if (attacker->mtype)
			safe_cprintf(self->activator, PRINT_HIGH, "Your cocoon was killed by a %s\n", V_GetMonsterName(attacker));
		else
			safe_cprintf(self->activator, PRINT_HIGH, "Your cocoon was killed by a %s\n", attacker->classname);
	}

	// restore cocooned entity
	if (self->enemy && self->enemy->inuse && !self->deadflag)
	{
		self->enemy->movetype = self->count;
		self->enemy->svflags &= ~SVF_NOCLIENT;
		self->enemy->flags &= FL_COCOONED;

		if (self->enemy->client)
		{
			self->enemy->holdtime = level.time + 0.5;
			self->enemy->client->ability_delay = level.time + 0.5;
		}
	}

	if (self->health <= self->gib_health /* || organ_explode(self)*/)
	{
		int n;

		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < 2; n++)
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n= 0; n < 4; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		organ_remove(self, false);
		return;
	}

	if (self->deadflag == DEAD_DYING)
		return;

	self->think = cocoon_dead;
	self->deadflag = DEAD_DYING;
	self->delay = level.time + 20.0;
	self->nextthink = level.time + FRAMETIME;
	self->s.frame = COCOON_FRAME_STANDBY;
	self->movetype = MOVETYPE_TOSS;
	self->maxs[2] = 0;
	gi.linkentity(self);
}

void cocoon_cloak (edict_t *self)
{
	// cloak disabled or not upgraded
	if (self->monsterinfo.jumpdn == -1)
		return;

	// don't cloak if we're holding the flag carrier!
	if (self->enemy->client && self->enemy->client->pers.inventory[flag_index])
		return;

	// already cloaked
	if (self->svflags & SVF_NOCLIENT)
		return;
	
	// cloak after jumpdn frames of attacking
	if (self->monsterinfo.jumpup >= self->monsterinfo.jumpdn)
		self->svflags |= SVF_NOCLIENT;
}

void cocoon_attack (edict_t *self)
{
	int		frames;
	float	time;
	vec3_t	start;

	if (que_typeexists(self->curses, CURSE_FROZEN))
		return;

	if (!G_EntIsAlive(self->enemy))
	{
		if (self->enemy)
			self->enemy = NULL;
		return;
	}
	
	cocoon_cloak(self);

	if (level.framenum >= self->monsterinfo.nextattack)
	{
		int		heal;
		float	duration = COCOON_INITIAL_TIME + COCOON_ADDON_TIME * self->monsterinfo.level;
		float	factor = COCOON_INITIAL_FACTOR + COCOON_ADDON_FACTOR * self->monsterinfo.level;

		// give them a damage/defense bonus for awhile
		self->enemy->cocoon_time = level.time + duration;
		self->enemy->cocoon_factor = factor;
		self->enemy->cocoon_owner = self->creator;

		if (self->creator && self->creator->client)
			self->creator->client->layout.dirty = true;

		if (self->enemy->client && !self->enemy->ai.is_bot)
			gi.cprintf(self->enemy, PRINT_HIGH, "You have gained a damage/defense bonus of +%.0f%c for %.0f seconds\n",
				(factor * 100) - 100, '%', duration); 
		
		//4.4 give some health
		heal = self->enemy->max_health * (0.25 + (0.075 * self->monsterinfo.level));
		if (self->enemy->health < self->enemy->max_health)
		{
			self->enemy->health += heal;
			if (self->enemy->health > self->enemy->max_health)
				self->enemy->health = self->enemy->max_health;
		}
		
		
		//Talent: Phantom Cocoon - decloak when entity emerges
		self->svflags &= ~SVF_NOCLIENT; 
		self->monsterinfo.jumpup = 0;

		self->enemy->svflags &= ~SVF_NOCLIENT;
		self->enemy->movetype = self->count;
		self->enemy->flags &= ~FL_COCOONED;//4.4
	//	self->owner = self->enemy;
		self->enemy = NULL;
		self->s.frame = COCOON_FRAME_STANDBY;
		self->maxs[2] = 0;//shorten bbox
		self->touch = V_Touch;
		return;
	}

	if (!(level.framenum % (int)(sv_fps->value)) && self->enemy->client)
		safe_cprintf(self->enemy, PRINT_HIGH, "You will emerge from the cocoon in %d second(s)\n", 
			(int)((self->monsterinfo.nextattack - level.framenum) * FRAMETIME));

	time = level.time + FRAMETIME;

	// hold target in-place
	if (!strcmp(self->enemy->classname, "drone"))
	{
		self->enemy->monsterinfo.pausetime = time;
		self->enemy->monsterinfo.stand(self->enemy);
	}
	else
		self->enemy->holdtime = time;

	// keep morphed players from shooting
	if (PM_MonsterHasPilot(self->enemy))
	{
		self->enemy->owner->client->ability_delay = time;
		self->enemy->owner->monsterinfo.attack_finished = time;
	}
	else
	{
		// no using abilities!
		if (self->enemy->client)
			self->enemy->client->ability_delay = time;
		//if(strcmp(self->enemy->classname, "spiker") != 0)
		if (self->enemy->mtype == M_SPIKER)
			self->enemy->monsterinfo.attack_finished = time;
	}
	
	// move position
	VectorCopy(self->s.origin, start);
	start[2] += fabsf(self->enemy->mins[2]) + 1;
	VectorCopy(start, self->enemy->s.origin);
	gi.linkentity(self->enemy);
	
	// hide them
	self->enemy->svflags |= SVF_NOCLIENT;

	frames = COCOON_INITIAL_DURATION + COCOON_ADDON_DURATION * self->monsterinfo.level;
	if (frames < COCOON_MINIMUM_DURATION)
		frames = COCOON_MINIMUM_DURATION;

	/*if (M_Regenerate(self->enemy, frames, 1, true, true, false, &self->monsterinfo.regen_delay2) && (level.time > self->msg_time))
	{
		gi.sound(self, CHAN_AUTO, gi.soundindex("organ/healer1.wav"), 1, ATTN_NORM, 0);
		self->msg_time = level.time + 1.5;
	}
	*/

	self->monsterinfo.jumpup++;//Talent: Phantom Cocoon - keep track of attack frames
}

void cocoon_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int frames;

	V_Touch(ent, other, plane, surf);
	
	if (!ent->groundentity || ent->groundentity != world)
		return;
	if (level.framenum < ent->monsterinfo.nextattack)
		return;
	if (!G_ValidTargetEnt(ent, other, true) || !OnSameTeam(ent, other))
		return;
	if (other->mtype == M_SPIKEBALL)//4.4
		return;
	if (other->movetype == MOVETYPE_NONE)
		return;

	ent->enemy = other;

	frames = COCOON_INITIAL_DURATION + COCOON_ADDON_DURATION * ent->monsterinfo.level;
	if (frames < COCOON_MINIMUM_DURATION)
		frames = COCOON_MINIMUM_DURATION;
	ent->monsterinfo.nextattack = level.framenum + sf2qf(frames);

	// don't let them move (or fall out of the map)
	ent->count = other->movetype;
	other->movetype = MOVETYPE_NONE;
	other->flags |= FL_COCOONED;//4.4

	if (other->client)
		safe_cprintf(other, PRINT_HIGH, "You have been cocooned for %d seconds\n", (int)(frames / 10));

	if (ent->activator && ent->activator->client)
	{
		ent->activator->client->layout.dirty = true;
	}
}

void cocoon_think (edict_t *self)
{
	trace_t tr;

	if (!G_EntIsAlive(self->activator))
	{
		organ_remove(self, false);
		return;
	}

	if (!organ_checkrevived(self))
		return;

	cocoon_attack(self);

	if (level.time > self->lasthurt + 1.0)
		M_Regenerate(self, qf2sf(300), qf2sf(10),  1.0, true, false, false, &self->monsterinfo.regen_delay1);

	// if position has been updated, check for ground entity
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	// don't slide
	if (self->groundentity)
		VectorClear(self->velocity);
	
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);

	self->nextthink = level.time + FRAMETIME;

	if (self->s.frame == COCOON_FRAME_STANDBY)
	{
		vec3_t maxs;

		VectorCopy(self->maxs, maxs);
		maxs[2] = 40;

		tr = gi.trace(self->s.origin, self->mins, maxs, self->s.origin, self, MASK_SHOT);
		if (tr.fraction == 1.0)
		{
			//self->touch = cocoon_touch;
			self->maxs[2] = 40;
			self->s.frame++;
		}
		return;
	}
	else if (self->s.frame > COCOON_FRAME_STANDBY && self->s.frame < COCOON_FRAMES_GROW_END)
	{
		G_RunFrames(self, COCOON_FRAMES_GROW_START, COCOON_FRAMES_GROW_END, false);
	}
	else if (self->s.frame == COCOON_FRAMES_GROW_END)
	{
		self->style = 0; //done growing
		self->touch = cocoon_touch;
		self->s.frame = COCOON_FRAMES_IDLE_START;
	}
	else
		G_RunFrames(self, COCOON_FRAMES_IDLE_START, COCOON_FRAMES_IDLE_END, false);
}

edict_t *CreateCocoon (edict_t *ent, int skill_level)
{
	//Talent: Phantom Cocoon
    //int talentLevel = vrx_get_talent_level(ent, TALENT_PHANTOM_COCOON);

	edict_t *e;

	e = G_Spawn();
	e->style = 1; //growing
	e->activator = ent;
	e->think = cocoon_think;
	e->nextthink = level.time + FRAMETIME;
	e->s.modelindex = gi.modelindex ("models/objects/cocoon/tris.md2");
	e->s.renderfx |= RF_IR_VISIBLE;
	e->solid = SOLID_BBOX;
	e->movetype = MOVETYPE_TOSS;
	e->svflags |= SVF_MONSTER;
	e->clipmask = MASK_MONSTERSOLID;
	e->mass = 500;
	e->classname = "cocoon";
	e->takedamage = DAMAGE_AIM;
	e->health = e->max_health = COCOON_INITIAL_HEALTH + COCOON_ADDON_HEALTH * skill_level;
	e->monsterinfo.level = skill_level;
	e->creator = ent;

	//Talent: Phantom Cocoon - frames before cloaking
	//if (talentLevel > 0)
	//	e->monsterinfo.jumpdn = 50 - 8 * talentLevel;
	//else
		e->monsterinfo.jumpdn = -1; // cloak disabled

	e->gib_health = -2 * BASE_GIB_HEALTH;
	e->s.frame = COCOON_FRAME_STANDBY;
	e->die = cocoon_die;
	e->touch = V_Touch;
	VectorSet(e->mins, -50, -50, -40);
	VectorSet(e->maxs, 50, 50, 40);
	e->mtype = M_COCOON;

	ent->cocoon = e;

	return e;
}

void Cmd_Cocoon_f (edict_t *ent)
{
	edict_t *cocoon;
	vec3_t	start;

	if (ent->cocoon && ent->cocoon->inuse)
	{
		organ_remove(ent->cocoon, true);
		safe_cprintf(ent, PRINT_HIGH, "Cocoon removed\n");
		return;
	}

	if (!V_CanUseAbilities(ent, COCOON, COCOON_COST, true))
		return;

	cocoon = CreateCocoon(ent, ent->myskills.abilities[COCOON].current_level);
	if (!G_GetSpawnLocation(ent, 150, cocoon->mins, cocoon->maxs, start, NULL, PROJECT_HITBOX_FAR, false))
	{
		ent->cocoon = NULL;
		G_FreeEdict(cocoon);
		return;
	}
	VectorCopy(start, cocoon->s.origin);
	VectorCopy(ent->s.angles, cocoon->s.angles);
	cocoon->s.angles[PITCH] = 0;
	gi.linkentity(cocoon);
	cocoon->monsterinfo.cost = COCOON_COST;

	if (ent->client)
		layout_add_tracked_entity(&ent->client->layout, cocoon);
	AI_EnemyAdded(cocoon);

	safe_cprintf(ent, PRINT_HIGH, "Cocoon created\n");
	gi.sound(cocoon, CHAN_VOICE, gi.soundindex("organ/organe3.wav"), 1, ATTN_STATIC, 0);

	ent->client->pers.inventory[power_cube_index] -= COCOON_COST;
	ent->client->ability_delay = level.time + COCOON_DELAY;

	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}

void spikeball_findtarget (edict_t *self)
{
    edict_t *e=NULL;

    if (!G_ValidTarget(self, self->enemy, true, true))
    {
        while ((e = findclosestradius_targets(e, self, self->dmg_radius)) != NULL)
        {
            if (!G_ValidTarget_Lite(self, e, true))
                continue;

            // ignore enemies that are too far from our owner
            // FIXME: we may want to add a hurt function so that we can retaliate if we are attacked out of range
            if (entdist(self->activator, e) > SPIKEBALL_MAX_DIST)
                continue;

            self->enemy = e;
           //	gi.sound (self, CHAN_VOICE, gi.soundindex("brain/brnpain1.wav"), 1, ATTN_NORM, 0);
            return;
        }

        self->enemy = NULL;
    }
}

void spikeball_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
    if (other && other->inuse && other->takedamage && !OnSameTeam(ent, other) && (level.time > ent->monsterinfo.attack_finished))
    {
		if (T_Damage(other, ent, ent, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, 0, MOD_UNKNOWN))
		{
			if (random() > 0.5)
				gi.sound(ent, CHAN_WEAPON, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound(ent, CHAN_WEAPON, gi.soundindex("brain/melee3.wav"), 1, ATTN_NORM, 0);
		}

        ent->monsterinfo.attack_finished = level.time + 0.5;
    }

    V_Touch(ent, other, plane, surf);
}

qboolean spikeball_recall (edict_t *self)
{
    // we shouldn't be here if recall is disabled, we have a target, our activator is visible
    // or we have teleported recently
    if (/*!self->activator->spikeball_recall || self->enemy ||*/ visible(self, self->activator)
                                                                 || self->monsterinfo.teleport_delay > level.time)
        return false;

    TeleportNearArea(self, self->activator->s.origin, 128, false);
    self->monsterinfo.teleport_delay = level.time + 1;//0.0;
    return true;
}

void spikeball_move (edict_t *self)
{
    vec3_t	start, forward, end, goalpos;
    trace_t	tr;
    float	max_velocity = 350;

    if (self->monsterinfo.attack_finished > level.time)
        return;

    // are we following the player's crosshairs?
    if (self->activator->spikeball_follow)
    {
		vec3_t v1 = {0}, v2 = {0};
        G_GetSpawnLocation(self->activator, 8192, v1, v2, goalpos, NULL, PROJECT_HITBOX_FAR, false);
    }
        // we have no enemy
    else if (!self->enemy)
    {
        if (entdist(self, self->activator) > 128)
        {
            // move towards player
            if (visible(self, self->activator))
            {
                VectorCopy(self->activator->s.origin, goalpos);
                self->monsterinfo.search_frames = 0;

            }
            else
            {
                // try to teleport near player after 3 seconds of losing sight
                if (self->monsterinfo.search_frames > 30)
                    spikeball_recall(self);
                else
                    self->monsterinfo.search_frames++;
                return;
            }
        }
        else
            return;
    }

    // use the enemy's position as a starting point if we are not following
    if (self->enemy && !self->spikeball_follow)
    {
        // enemy is too far from activator, abort if activator is visible or we can teleport
        if (entdist(self->activator, self->enemy) > SPIKEBALL_MAX_DIST
            && (visible(self, self->activator) || spikeball_recall(self)))
        {
            self->enemy = NULL;
            return;
        }

        VectorCopy(self->enemy->s.origin, goalpos);
    }


    // vector to target
    VectorSubtract(goalpos, self->s.origin, forward);
    VectorNormalize(forward);

    // accelerate towards target
    VectorMA(self->velocity, self->monsterinfo.lefty, forward, forward);
    self->velocity[0] = forward[0];
    self->velocity[1] = forward[1];

    // calculate starting position and ending position on a 2-D plane
    VectorCopy(self->s.origin, start);
    VectorAdd(self->s.origin, self->velocity, end);
    tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SHOT);
    VectorCopy(tr.endpos, end);
    start[2] = end[2] = goalpos[2] = 0;

    // if we are moving away from our target, slow down
    if (distance(goalpos, end) > distance(goalpos, start))
    {
        self->velocity[0] *= 0.8;
        self->velocity[1] *= 0.8;
    }

    // cap maximum x and y velocity
    if (self->velocity[0] > max_velocity)
        self->velocity[0] = max_velocity;
    if (self->velocity[0] < -max_velocity)
        self->velocity[0] = -max_velocity;
    if (self->velocity[1] > max_velocity)
        self->velocity[1] = max_velocity;
    if (self->velocity[1] < -max_velocity)
        self->velocity[1] = -max_velocity;

//	gi.dprintf("%.0f %.0f %.0f\n", self->velocity[0], self->velocity[1], self->velocity[2]);
}

void spikeball_idle_friction (edict_t *self)
{
    if (!self->enemy && !self->spikeball_follow)
    {
        self->velocity[0] *= 0.8;
        self->velocity[1] *= 0.8;

        if (VectorLength(self->velocity) < 10)
        {
            VectorClear(self->velocity);
            return;
        }

        if (level.time > self->msg_time)
        {
            gi.sound (self, CHAN_VOICE, gi.soundindex(va("spore/neutral%d.wav", GetRandom(1, 6))), 1, ATTN_NORM, 0);
            self->msg_time = level.time + GetRandom(2, 10);
        }
    }
}

void spikeball_effects(edict_t *self)
{
    if (self->enemy)
    {
        self->s.effects |= EF_COLOR_SHELL;
        self->s.renderfx |= RF_SHELL_RED;
    }
    else
    {
        self->s.effects &= ~EF_COLOR_SHELL;
        self->s.renderfx &= ~RF_SHELL_RED;
    }
}

void spikeball_think (edict_t *self)
{
    if (!G_EntIsAlive(self->owner) || level.time > self->delay)
    {
        organ_remove(self, false);
        return;
    }
    else if (self->removetime > 0)
    {
        qboolean converted=false;

        if (self->flags & FL_CONVERTED)
            converted = true;

        if (level.time > self->removetime)
        {
            // if we were converted, try to convert back to previous owner
            if (converted && self->prev_owner && self->prev_owner->inuse)
            {
                if (!ConvertOwner(self->prev_owner, self, 0, false, false))
                {
                    organ_remove(self, false);
                    return;
                }
            }
            else
            {
                organ_remove(self, false);
                return;
            }
        }
            // warn the converted monster's current owner
        else if (converted && self->activator && self->activator->inuse && self->activator->client
                 && (level.time > self->removetime-5) && !(level.framenum % (int)(1 / FRAMETIME)))
            safe_cprintf(self->activator, PRINT_HIGH, "%s conversion will expire in %.0f seconds\n",
                         V_GetMonsterName(self), self->removetime-level.time);
    }

    if (!M_Upkeep(self, 1.3 / FRAMETIME, 1))
        return;

	
    spikeball_effects(self);
	if (level.time > self->holdtime && !que_typeexists(self->curses, CURSE_FROZEN))
	{
		spikeball_findtarget(self);
		//spikeball_recall(self);
		spikeball_move(self);
	}
    spikeball_idle_friction(self);

    self->nextthink = level.time + FRAMETIME;
}

void spikeball_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
    if (!self->monsterinfo.slots_freed && self->activator && self->activator->inuse)
    {
        int cur;

        self->activator->num_spikeball--;
        cur = self->activator->num_spikeball;
        self->monsterinfo.slots_freed = true;

        if (PM_MonsterHasPilot(attacker))
            attacker = attacker->owner;

        if (attacker->client)
            safe_cprintf(self->activator, PRINT_HIGH, "Your spore was killed by %s (%d remain)\n", attacker->client->pers.netname, cur);
        else if (attacker->mtype)
            safe_cprintf(self->activator, PRINT_HIGH, "Your spore was killed by a %s (%d remain)\n", V_GetMonsterName(attacker), cur);
        else
            safe_cprintf(self->activator, PRINT_HIGH, "Your spore was killed by a %s (%d remain)\n", attacker->classname, cur);
    }

    organ_remove(self, false);
}

void fire_spikeball (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int throw_speed, int ball_speed, float radius, int duration, int health, int skill_level)
{
    int		cost = SPIKEBALL_COST;
    edict_t	*grenade;
    vec3_t	dir;
    vec3_t	forward, right, up;

    // calling entity made a sound, used to alert monsters
    self->lastsound = level.framenum;

    // cost is doubled if you are a flyer or cacodemon below skill level 5
    if ((self->mtype == MORPH_FLYER && self->myskills.abilities[FLYER].current_level < 5)
        || (self->mtype == MORPH_CACODEMON && self->myskills.abilities[CACODEMON].current_level < 5))
        cost *= 2;

    // get aiming angles
    vectoangles(aimdir, dir);
    // get directional vectors
    AngleVectors(dir, forward, right, up);

    // spawn grenade entity
    grenade = G_Spawn();
    VectorCopy (start, grenade->s.origin);
    grenade->takedamage = DAMAGE_AIM;
    grenade->die = spikeball_die;
    grenade->health = grenade->max_health = health;
    grenade->movetype = MOVETYPE_BOUNCE;
//	grenade->svflags |= SVF_MONSTER; // tell physics to clip on more than just walls
    grenade->clipmask = MASK_SHOT;
    grenade->solid = SOLID_BBOX;
    grenade->s.effects |= EF_GIB;
    grenade->s.modelindex = gi.modelindex ("models/objects/sspore/tris.md2");
    grenade->owner = grenade->activator = self;
    grenade->touch = spikeball_touch;
    grenade->think = spikeball_think;
    grenade->dmg = damage;
    grenade->mtype = M_SPIKEBALL;
    grenade->monsterinfo.level = skill_level;
    grenade->dmg_radius = radius;
    grenade->monsterinfo.attack_finished = level.time + 2.0;
    grenade->monsterinfo.lefty = ball_speed;
    grenade->delay = level.time + duration;
    grenade->classname = "spikeball";
    VectorSet(grenade->maxs, 8, 8, 8);
    VectorSet(grenade->mins, -8, -8, -8);

	if (self->client)
		layout_add_tracked_entity(&self->client->layout, grenade);
	AI_EnemyAdded(grenade);

    gi.linkentity (grenade);
    grenade->nextthink = level.time + 1.0;
    grenade->monsterinfo.cost = cost;

    // adjust velocity
    VectorScale (aimdir, throw_speed, grenade->velocity);
    VectorMA (grenade->velocity, 200 + crandom() * 10.0, up, grenade->velocity);
    VectorMA (grenade->velocity, crandom() * 10.0, right, grenade->velocity);
    VectorSet (grenade->avelocity, 300, 300, 300);

    self->num_spikeball++;
}

void organ_removeall (edict_t *ent, char *classname, qboolean refund);

void organ_kill(edict_t *ent, char *classname)
{
    edict_t *e = NULL;

    while ((e = G_Find(e, FOFS(classname), classname)) != NULL)
    {
        if (e && e->activator && e->activator->inuse && (e->activator == ent) && !RestorePreviousOwner(e))
            T_Damage(e, ent, ent, vec3_origin, e->s.origin, vec3_origin, e->health / 2, 0, DAMAGE_NO_PROTECTION, MOD_LIGHTNING_STORM);
    }
}

void Cmd_TossSpikeball (edict_t *ent)
{
    int		talentLevel;
    int		cost = SPIKEBALL_COST, max_count = SPIKEBALL_MAX_COUNT;
    int		health = SPIKEBALL_INITIAL_HEALTH + SPIKEBALL_ADDON_HEALTH * ent->myskills.abilities[SPORE].current_level;
    int		damage = SPIKEBALL_INITIAL_DAMAGE + SPIKEBALL_ADDON_DAMAGE * ent->myskills.abilities[SPORE].current_level;
    float	duration = SPIKEBALL_INITIAL_DURATION + SPIKEBALL_ADDON_DURATION * ent->myskills.abilities[SPORE].current_level;
    float	range = SPIKEBALL_INITIAL_RANGE + SPIKEBALL_ADDON_RANGE * ent->myskills.abilities[SPORE].current_level;
    vec3_t	forward, right, start, offset;

    if (ent->num_spikeball > 0 && Q_strcasecmp (gi.args(), "move") == 0)
    {
        // toggle movement setting
        if (ent->spikeball_follow)
        {
            ent->spikeball_follow = false;
            safe_cprintf(ent, PRINT_HIGH, "Spores will follow targets\n");
        }
        else
        {
            ent->spikeball_follow = true;
            safe_cprintf(ent, PRINT_HIGH, "Spores will follow your crosshairs\n");
        }
        return;
    }

    if (Q_strcasecmp(gi.args(), "kill") == 0 && ent->myskills.administrator) {
        organ_kill(ent, "spikeball");
        return;
    }
/*
	if (ent->num_spikeball > 0 && Q_strcasecmp (gi.args(), "recall") == 0)
	{
		// toggle recall setting
		if (ent->spikeball_recall)
		{
			ent->spikeball_recall = false;
			safe_cprintf(ent, PRINT_HIGH, "Spores recall disabled\n");
		}
		else
		{
			ent->spikeball_recall = true;
			safe_cprintf(ent, PRINT_HIGH, "Spores recall enabled\n");
		}
		return;
	}
*/
    if (Q_strcasecmp (gi.args(), "help") == 0)
    {
        safe_cprintf(ent, PRINT_HIGH, "syntax: spore [move|remove]\n");
        return;
    }

    if (Q_strcasecmp (gi.args(), "remove") == 0)
    {
        organ_removeall(ent, "spikeball", true);
        safe_cprintf(ent, PRINT_HIGH, "Spores removed\n");
        ent->num_spikeball = 0;
        return;
    }

    // cost is doubled if you are a flyer or cacodemon below skill level 5
    if ((ent->mtype == MORPH_FLYER && ent->myskills.abilities[FLYER].current_level < 5)
        || (ent->mtype == MORPH_CACODEMON && ent->myskills.abilities[CACODEMON].current_level < 5))
        cost *= 2;

    if (!V_CanUseAbilities(ent, SPORE, cost, true))
        return;

    //Talent: Swarming
    if ((talentLevel = vrx_get_talent_level(ent, TALENT_SWARMING)) > 0)
    {
        // max_count *= 1.0 + 0.2 * talentLevel;
        damage *= 1.0 + 0.15 * talentLevel; // Only increase damage.
    }

    if (ent->num_spikeball >= max_count)
    {
        safe_cprintf(ent, PRINT_HIGH, "You have reached the maximum amount of spores (%d)\n", ent->num_spikeball);
        return;
    }

    // get starting position and forward vector
    AngleVectors (ent->client->v_angle, forward, right, NULL);
    VectorSet(offset, 0, 8,  ent->viewheight-8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    fire_spikeball(ent, start, forward, damage, 600, 100, range, duration, health, ent->myskills.abilities[SPORE].current_level);

    ent->client->ability_delay = level.time + SPIKEBALL_DELAY;

    ent->client->pers.inventory[power_cube_index] -= cost;
}

void acid_sparks (vec3_t org, int num, float radius)
{
    int		i;
    vec3_t	start;

    // 0 = black, 8 = grey, 15 = white, 16 = light brown, 20 = brown, 57 = light orange, 66 = orange/red, 73 = maroon
    // 106 = pink, 113 = light blue, 119 = blue, 123 = dark blue, 200 = pale green, 205 = dark green, 209 = bright green
    // 217 = white, 220 = yellow, 226 = orange, 231 = red/orange, 240 = red, 243 = dark blue

    for (i=0; i<num; i++)
    {
        //VectorCopy(self->s.origin, start);
        VectorCopy(org, start);
        start[0] += crandom() * GetRandom(0, (int) radius);
        start[1] += crandom() * GetRandom(0, (int) radius);
        //start[2] += crandom() * GetRandom(0, (int) self->dmg_radius);

        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_LASER_SPARKS);
        gi.WriteByte(1); // number of sparks
        gi.WritePosition(start);
        gi.WriteDir(vec3_origin);
        //gi.WriteByte(GetRandom(200, 209)); // color
        if (random() <= 0.33)
            gi.WriteByte(205);
        else
            gi.WriteByte(209);
        gi.multicast(start, MULTICAST_PVS);
    }
}

void CreatePoison (edict_t *ent, edict_t *targ, int damage, float duration, int meansOfDeath);

void acid_explode (edict_t *self)
{
    int amt, max_amt;
    vec3_t start;
    edict_t *e=NULL;

    if (!G_EntIsAlive(self->owner))
    {
        G_FreeEdict(self);
        return;
    }

    while ((e = findradius(e, self->s.origin, self->dmg_radius)) != NULL)
    {
        // acid heals alien summons beyond their normal max health
        if (OnSameTeam(self->owner, e) && (e->mtype == M_SPIKER
                                           || e->mtype == M_OBSTACLE || e->mtype == M_GASSER
                                           || e->mtype == M_HEALER || e->mtype == M_COCOON))
        {
            amt = 0.1 * self->dmg;
            max_amt = 2 * e->max_health;

            // 2x health maximum
            if (e->health + amt < max_amt)
                e->health += amt;
            else if (e->health < max_amt)
                e->health = max_amt;
        }

        if (e && e->inuse && e->takedamage && e->solid != SOLID_NOT && !OnSameTeam(self->owner, e))
        {
            T_Damage(e, self, self->owner, vec3_origin, e->s.origin, vec3_origin, self->dmg, 0, 0, MOD_ACID);
            CreatePoison(self->owner, e, self->radius_dmg, self->delay, MOD_ACID);
        }
    }

    VectorCopy(self->s.origin, start);
    start[2] += 8;
    acid_sparks(start, 20, self->dmg_radius);

    gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/acid.wav"), 1, ATTN_NORM, 0);

    G_FreeEdict(self);
}


void acid_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
    // disappear when touching a sky brush
    if (surf && (surf->flags & SURF_SKY))
    {
        G_FreeEdict (ent);
        return;
    }

	if (other == ent->owner)
		return;

    acid_explode(ent);
}

void fire_acid (edict_t *self, vec3_t start, vec3_t aimdir, int projectile_damage, float radius,
                int speed, int acid_damage, float acid_duration)
{
    edict_t	*grenade;
    vec3_t	dir;
    vec3_t	forward, right, up;

    // calling entity made a sound, used to alert monsters
    self->lastsound = level.framenum;

    // get aiming angles
    vectoangles(aimdir, dir);
    // get directional vectors
    AngleVectors(dir, forward, right, up);

    // spawn grenade entity
    grenade = G_Spawn();
    VectorCopy (start, grenade->s.origin);
    grenade->movetype = MOVETYPE_TOSS;
    grenade->clipmask = MASK_SHOT;
    grenade->solid = SOLID_BBOX;
    grenade->s.modelindex = gi.modelindex ("models/objects/spore/tris.md2");
    grenade->s.skinnum = 1;
    grenade->owner = self;
    grenade->touch = acid_touch;
    grenade->think = acid_explode;
    grenade->dmg = projectile_damage;
    grenade->radius_dmg = acid_damage;
    grenade->dmg_radius = radius;
    grenade->delay = acid_duration;
    grenade->classname = "acid";
    gi.linkentity (grenade);
    grenade->nextthink = level.time + 10.0;

    // adjust velocity
    VectorScale (aimdir, speed, grenade->velocity);
    VectorMA (grenade->velocity, 200 + crandom() * 10.0, up, grenade->velocity);
    VectorMA (grenade->velocity, crandom() * 10.0, right, grenade->velocity);
    VectorSet (grenade->avelocity, 300, 300, 300);
}

//#define ACID_INITIAL_DAMAGE		50
//#define ACID_ADDON_DAMAGE		15
//#define ACID_INITIAL_SPEED		600
//#define ACID_ADDON_SPEED		0
//#define ACID_INITIAL_RADIUS		64
//#define ACID_ADDON_RADIUS		0
//#define ACID_DURATION			10.0
//#define ACID_DELAY				0.2
//#define ACID_COST				20

void Cmd_FireAcid_f (edict_t *ent)
{
	int		gasser_level = ent->myskills.abilities[GASSER].current_level;
	int		acid_level = ent->myskills.abilities[ACID].current_level;
    int		damage = ACID_INITIAL_DAMAGE + ACID_ADDON_DAMAGE * acid_level;
	int		speed = ACID_INITIAL_SPEED + ACID_ADDON_SPEED * acid_level;
    float	radius = ACID_INITIAL_RADIUS + ACID_ADDON_RADIUS * acid_level;
	//float	synergy_bonus = 1.0 + ACID_GASSER_SYNERGY_BONUS * gasser_level; // synergy bonus from gasser
    vec3_t	forward, right, start, offset;

    if (!V_CanUseAbilities(ent, ACID, ACID_COST, true))
        return;

	damage *= vrx_get_synergy_mult(ent, ACID);

    // get starting position and forward vector
    AngleVectors (ent->client->v_angle, forward, right, NULL);
    VectorSet(offset, 0, 8,  ent->viewheight-8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    fire_acid(ent, start, forward, damage, radius, speed, (int)(0.1 * damage), ACID_DURATION);

    ent->client->ability_delay = level.time + ACID_DELAY;
    ent->client->pers.inventory[power_cube_index] -= ACID_COST;
}
