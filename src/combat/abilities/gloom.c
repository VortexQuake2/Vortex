#include "g_local.h"
#include "../../gamemodes/ctf.h"


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
		
		self->movetype_prev = self->movetype;
		self->movetype_frame = (int)(level.framenum + 0.5 / FRAMETIME);
		self->movetype = MOVETYPE_STEP;

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

// touch function for all the gloom stuff
void organ_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	V_Touch(ent, other, plane, surf);

	// don't push or heal something that's already dead or invalid
	if (!ent || !ent->inuse || !ent->takedamage || ent->health < 1)
		return;

	V_Push(ent, other, plane, surf);

	if (G_EntIsAlive(other) && other->client && OnSameTeam(ent, other) 
		&& other->client->pers.inventory[power_cube_index] >= 5
		&& level.time > ent->lasthurt + 0.5 && ent->health < ent->max_health
		&& level.framenum > ent->monsterinfo.regen_delay1)
	{
		ent->health_cache += (int)(0.5 * ent->max_health);
		ent->monsterinfo.regen_delay1 = (int)(level.framenum + 1 / FRAMETIME);
		other->client->pers.inventory[power_cube_index] -= 5;
	}
}

// generic remove function for all the gloom stuff
void organ_remove (edict_t *self, qboolean refund)
{
	if (!self || !self->inuse || self->deadflag == DEAD_DEAD)
		return;

	if (self->mtype == M_COCOON)
	{
		// restore cocooned entity
		if (self->enemy && self->enemy->inuse)
		{
			self->enemy->movetype = self->count;
			self->enemy->svflags &= ~SVF_NOCLIENT;
			self->enemy->flags &= FL_COCOONED;//4.4
		}
	}

	if (self->activator && self->activator->inuse)
	{
		if (!self->monsterinfo.slots_freed)
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

			self->monsterinfo.slots_freed = true;
		}

		if (refund)
			self->activator->client->pers.inventory[power_cube_index] += (self->health / self->max_health) * self->monsterinfo.cost;
	}

	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_NOCLIENT;
	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->deadflag = DEAD_DEAD;
}

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
}

#define HEALER_INITIAL_HEALTH		100
#define HEALER_ADDON_HEALTH			40
#define HEALER_FRAMES_GROW_START	0
#define HEALER_FRAMES_GROW_END		15
#define HEALER_FRAMES_START			16
#define HEALER_FRAMES_END			26
#define HEALER_FRAME_DEAD			4
#define HEALER_COST					50
#define HEALER_DELAY				1.0

void healer_heal (edict_t *self, edict_t *other)
{
	float	value;
    //int	talentLevel = vrx_get_talent_level(self->activator, TALENT_SUPER_HEALER);
	qboolean regenerate = false;

	// (apple)
	// In the healer's case, checking for the health 
	// self->health >= 1 should be enough,
	// but used G_EntIsAlive for consistency.
	if (G_EntIsAlive(other) && G_EntIsAlive(self) && OnSameTeam(self, other) && other != self)
	{
		int frames = qf2sf(5000 / (15 * self->monsterinfo.level)); // idk how much would this change tbh

        value = 1.0f + 0.1f * vrx_get_talent_level(self->activator, TALENT_SUPER_HEALER);

		// regenerate health
		if (M_Regenerate(other, frames, qf2sf(1), value, true, false, false, &other->monsterinfo.regen_delay2))
			regenerate = true;

		// regenerate armor
		/*
		if (talentLevel > 0)
		{
			frames *= (3.0 - (0.4 * talentLevel)); // heals 100% armor in 5 seconds at talent level 5
			if (M_Regenerate(other, frames, 10,  1.0, false, true, false, &other->monsterinfo.regen_delay3))
				regenerate = true;
		}*/
		
		// play healer sound if health or armor was regenerated
		if (regenerate && (level.time > self->msg_time))
		{
			gi.sound(self, CHAN_AUTO, gi.soundindex("organ/healer1.wav"), 1, ATTN_NORM, 0);
			self->msg_time = level.time + 1.5;
		}
	}
}

void healer_attack (edict_t *self)
{
	edict_t *e=NULL;

	while ((e = findradius(e, self->s.origin, 96)) != NULL)
		healer_heal(self, e);
}

void healer_think (edict_t *self)
{
	if (!G_EntIsAlive(self->activator))
	{
		organ_remove(self, false);
		return;
	}

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

	if (level.time > self->lasthurt + 1.0)
		M_Regenerate(self, qf2sf(300), qf2sf(10), 1.0, true, false, false, &self->monsterinfo.regen_delay1);

	G_RunFrames(self, HEALER_FRAMES_START, HEALER_FRAMES_END, false);

	self->nextthink = level.time + FRAMETIME;
}

void healer_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
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

	if (self->health <= self->gib_health || organ_explode(self))
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
	e->movetype = MOVETYPE_TOSS;
	e->clipmask = MASK_MONSTERSOLID;
	e->mass = 500;
	e->classname = "healer";
	e->takedamage = DAMAGE_AIM;
	e->health = e->max_health = HEALER_INITIAL_HEALTH + HEALER_ADDON_HEALTH * skill_level;
	e->monsterinfo.level = skill_level;
	e->gib_health = -2 * BASE_GIB_HEALTH;
	e->die = healer_die;
	e->touch = V_Touch;
	VectorSet(e->mins, -28, -28, 0);
	VectorSet(e->maxs, 28, 28, 32);
	e->mtype = M_HEALER;
	ent->healer = e;

	return e;
}

void Cmd_Healer_f (edict_t *ent)
{
	edict_t *healer;
	vec3_t	start;

	if (ent->healer && ent->healer->inuse)
	{
		organ_remove(ent->healer, true);
		safe_cprintf(ent, PRINT_HIGH, "Healer removed\n");
		return;
	}

	if (!V_CanUseAbilities(ent, HEALER, HEALER_COST, true))
		return;

	healer = CreateHealer(ent, ent->myskills.abilities[HEALER].current_level);
	if (!G_GetSpawnLocation(ent, 100, healer->mins, healer->maxs, start))
	{
		ent->healer = NULL;
		G_FreeEdict(healer);
		return;
	}
	VectorCopy(start, healer->s.origin);
	VectorCopy(ent->s.angles, healer->s.angles);
	healer->s.angles[PITCH] = 0;
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
	int max = SPIKER_MAX_COUNT;
	int cur;

	if (!self->monsterinfo.slots_freed && self->activator && self->activator->inuse)
	{
		self->activator->num_spikers--;
		cur = self->activator->num_spikers;
		self->monsterinfo.slots_freed = true;
		
		if (PM_MonsterHasPilot(attacker))
			attacker = attacker->owner;

		if (attacker->client)
			safe_cprintf(self->activator, PRINT_HIGH, "Your spiker was killed by %s (%d/%d remain)\n", attacker->client->pers.netname, cur, max);
		else if (attacker->mtype)
			safe_cprintf(self->activator, PRINT_HIGH, "Your spiker was killed by a %s (%d/%d remain)\n", V_GetMonsterName(attacker), cur, max);
		else
			safe_cprintf(self->activator, PRINT_HIGH, "Your spiker was killed by a %s (%d/%d remain)\n", attacker->classname, cur, max);
	}

	if (self->health <= self->gib_health || organ_explode(self))
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
	gi.linkentity(self);
}

void spiker_attack (edict_t *self)
{
	float	dist;
	float	range=SPIKER_INITIAL_RANGE+SPIKER_ADDON_RANGE*self->monsterinfo.level;
	int		speed=SPIKER_INITIAL_SPEED+SPIKER_ADDON_SPEED*self->monsterinfo.level;
	vec3_t	forward, start, end;
	edict_t *e=NULL;

	if (self->monsterinfo.attack_finished > level.time)
		return;

	VectorCopy(self->s.origin, start);
	start[2] = self->absmax[2] - 16;

	while ((e = findradius(e, self->s.origin, range)) != NULL)
	{
		if (!G_ValidTarget(self, e, true))
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

		fire_spike(self, start, forward, self->dmg, 0, speed);
		
		//FIXME: only need to do this once
		self->monsterinfo.attack_finished = level.time + SPIKER_REFIRE_DELAY;
		self->s.frame = SPIKER_FRAMES_NOAMMO_START;
		gi.sound (self, CHAN_VOICE, gi.soundindex ("weapons/twang.wav"), 1, ATTN_NORM, 0);
	}	
}

void NearestNodeLocation (vec3_t start, vec3_t node_loc);
int FindPath(vec3_t start, vec3_t destination);
void spiker_think (edict_t *self)
{
	edict_t *e=NULL;

	if (!organ_checkowner(self))
		return;

	organ_restoreMoveType(self);

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
				if (!ConvertOwner(self->prev_owner, self, 0, false))
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

	gi.linkentity(self);
	
	self->nextthink = level.time + FRAMETIME;
}

void spiker_grow (edict_t *self)
{
	if (!organ_checkowner(self))
		return;

	organ_restoreMoveType(self);

	V_HealthCache(self, (int)(0.2 * self->max_health), 1);

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
	e->movetype = MOVETYPE_TOSS;
	e->clipmask = MASK_MONSTERSOLID;
	e->svflags |= SVF_MONSTER;//Note/Important/Hint: tells MOVETYPE_STEP physics to clip on any solid object (not just walls)
	e->mass = 500;
	e->classname = "spiker";
	e->takedamage = DAMAGE_AIM;
	e->max_health = SPIKER_INITIAL_HEALTH + SPIKER_ADDON_HEALTH * skill_level;
	e->health = 0.5*e->max_health;
	e->dmg = SPIKER_INITIAL_DAMAGE + SPIKER_ADDON_DAMAGE * skill_level;

	e->monsterinfo.level = skill_level;
	e->gib_health = -2.5 * BASE_GIB_HEALTH;
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
		safe_cprintf(ent, PRINT_HIGH, "You have reached the maximum amount of spikers (%d)\n", SPIKER_MAX_COUNT);
		return;
	}

	spiker = CreateSpiker(ent, ent->myskills.abilities[SPIKER].current_level);
	if (!G_GetSpawnLocation(ent, 100, spiker->mins, spiker->maxs, start))
	{
		ent->num_spikers--;
		G_FreeEdict(spiker);
		return;
	}
	VectorCopy(start, spiker->s.origin);
	VectorCopy(ent->s.angles, spiker->s.angles);
	spiker->s.angles[PITCH] = 0;
	gi.linkentity(spiker);
	spiker->monsterinfo.attack_finished = level.time + 2.0;
	spiker->monsterinfo.cost = cost;

	safe_cprintf(ent, PRINT_HIGH, "Spiker created (%d/%d)\n", ent->num_spikers, (int)SPIKER_MAX_COUNT);

	ent->client->ability_delay = level.time + SPIKER_DELAY;
	ent->client->pers.inventory[power_cube_index] -= cost;
	//ent->holdtime = level.time + SPIKER_DELAY;

	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}

#define OBSTACLE_INITIAL_HEALTH			0		
#define OBSTACLE_ADDON_HEALTH			145
#define OBSTACLE_INITIAL_DAMAGE			0
#define OBSTACLE_ADDON_DAMAGE			40	
#define OBSTACLE_COST					25
#define OBSTACLE_DELAY					0.5

#define OBSTACLE_FRAMES_GROW_START		0
#define OBSTACLE_FRAMES_GROW_END		9
#define OBSTACLE_FRAME_READY			12
#define OBSTACLE_FRAME_DEAD				13

void obstacle_dead (edict_t *self)
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

void obstacle_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int max = OBSTACLE_MAX_COUNT;
	int cur;

	if (!self->monsterinfo.slots_freed && self->activator && self->activator->inuse)
	{
		self->activator->num_obstacle--;
		cur = self->activator->num_obstacle;
		self->monsterinfo.slots_freed = true;
		
		if (PM_MonsterHasPilot(attacker))
			attacker = attacker->owner;

		if (attacker->client)
			safe_cprintf(self->activator, PRINT_HIGH, "Your obstacle was killed by %s (%d/%d remain)\n", attacker->client->pers.netname, cur, max);
		else if (attacker->mtype)
			safe_cprintf(self->activator, PRINT_HIGH, "Your obstacle was killed by a %s (%d/%d remain)\n", V_GetMonsterName(attacker), cur, max);
		else
			safe_cprintf(self->activator, PRINT_HIGH, "Your obstacle was killed by a %s (%d/%d remain)\n", attacker->classname, cur, max);
	}

	if (self->health <= self->gib_health || organ_explode(self))
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
	self->s.frame = OBSTACLE_FRAME_DEAD;
	self->movetype = MOVETYPE_TOSS;
	self->touch = V_Touch;
	self->maxs[2] = 16;
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
		// stick to the ground
		if (self->groundentity && self->groundentity == world)
			self->movetype = MOVETYPE_NONE;
		else
			self->movetype = MOVETYPE_STEP;

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

void obstacle_think (edict_t *self)
{
	if (!organ_checkowner(self))
		return;

	organ_restoreMoveType(self);
	obstacle_cloak(self);
	obstacle_move(self);

	V_HealthCache(self, (int)(0.2 * self->max_health), 1);

//	if (level.time > self->lasthurt + 1.0)
//		M_Regenerate(self, 300, 10, true, false, false, &self->monsterinfo.regen_delay1);

	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);

	self->nextthink = level.time + FRAMETIME;
}

void obstacle_grow (edict_t *self)
{
	if (!organ_checkowner(self))
		return;

	organ_restoreMoveType(self);

	V_HealthCache(self, (int)(0.2 * self->max_health), 1);

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

edict_t *CreateObstacle (edict_t *ent, int skill_level)
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
	e->movetype = MOVETYPE_TOSS;
	e->svflags |= SVF_MONSTER;
	e->clipmask = MASK_MONSTERSOLID;
	e->mass = 500;
	e->classname = "obstacle";
	e->takedamage = DAMAGE_AIM;
	e->max_health = OBSTACLE_INITIAL_HEALTH + OBSTACLE_ADDON_HEALTH * skill_level;
	e->health = 0.5*e->max_health;
	e->dmg = OBSTACLE_INITIAL_DAMAGE + OBSTACLE_ADDON_DAMAGE * skill_level;
    e->monsterinfo.nextattack = 100 - 9 * vrx_get_talent_level(ent, TALENT_PHANTOM_OBSTACLE);
	e->monsterinfo.level = skill_level;
	e->gib_health = -2.5 * BASE_GIB_HEALTH;
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
	vec3_t	start;

	if (Q_strcasecmp (gi.args(), "remove") == 0)
	{
		organ_removeall(ent, "obstacle", true);
		safe_cprintf(ent, PRINT_HIGH, "Obstacles removed\n");
		ent->num_obstacle = 0;
		return;
	}

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
		safe_cprintf(ent, PRINT_HIGH, "You have reached the maximum amount of obstacles (%d)\n", OBSTACLE_MAX_COUNT);
		return;
	}

	obstacle = CreateObstacle(ent, ent->myskills.abilities[OBSTACLE].current_level);
	if (!G_GetSpawnLocation(ent, 100, obstacle->mins, obstacle->maxs, start))
	{
		ent->num_obstacle--;
		G_FreeEdict(obstacle);
		return;
	}
	VectorCopy(start, obstacle->s.origin);
	VectorCopy(ent->s.angles, obstacle->s.angles);
	obstacle->s.angles[PITCH] = 0;
	gi.linkentity(obstacle);
	obstacle->monsterinfo.cost = cost;

	safe_cprintf(ent, PRINT_HIGH, "Obstacle created (%d/%d)\n", ent->num_obstacle,OBSTACLE_MAX_COUNT);

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

#define GASSER_RANGE					128
#define GASSER_REFIRE					5.0
#define GASSER_INITIAL_DAMAGE			0
#define GASSER_ADDON_DAMAGE				10
#define GASSER_INITIAL_HEALTH			100
#define GASSER_ADDON_HEALTH				40
#define GASSER_INITIAL_ATTACK_RANGE		100
#define GASSER_ADDON_ATTACK_RANGE		0
#define GASSER_COST						25
#define GASSER_DELAY					1.0

#define GASCLOUD_FRAMES_GROW_START		10
#define GASCLOUD_FRAMES_GROW_END		18
#define GASCLOUD_FRAMES_IDLE_START		19
#define GASCLOUD_FRAMES_IDLE_END		23

#define GASCLOUD_POISON_DURATION		10.0
#define GASCLOUD_POISON_FACTOR			0.1

void poison_think (edict_t *self)
{
	if (!G_EntIsAlive(self->enemy) || !G_EntIsAlive(self->activator) || (level.time > self->delay))
	{
		G_FreeEdict(self);
		return;
	}

	if (level.framenum >= self->monsterinfo.nextattack)
	{
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
	e->mtype = POISON;
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

void gascloud_attack (edict_t *self)
{
	que_t	*slot=NULL;
	edict_t *e=NULL;

	while ((e = findradius(e, self->s.origin, self->dmg_radius)) != NULL)
	{
		if (G_ValidTarget(self, e, true))
		{
			// otherwise, update the attack frequency to once per server frame
			if ((slot = que_findtype(e->curses, NULL, POISON)) != NULL)
			{
				slot->ent->random = 1;
				slot->ent->monsterinfo.nextattack = level.framenum + 1;
				slot->ent->delay = level.time + self->radius_dmg;
				slot->time = level.time + self->radius_dmg;
			}
			// if target is not already poisoned, create poison entity
			else
				CreatePoison(self->activator, e, (int)(GASCLOUD_POISON_FACTOR*self->dmg), self->radius_dmg, MOD_GAS);
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
	gi.linkentity(e);
}

void fire_acid (edict_t *self, vec3_t start, vec3_t aimdir, int projectile_damage, float radius, 
				int speed, int acid_damage, float acid_duration);

void gasser_acidattack (edict_t *self)
{
	float	dist;
	float	range=384;
	int		speed=600;
	vec3_t	forward, start, end;
	edict_t *e=NULL;

	if (self->monsterinfo.attack_finished > level.time)
		return;

	VectorCopy(self->s.origin, start);
	start[2] = self->absmax[2] - 16;

	while ((e = findradius(e, self->s.origin, range)) != NULL)
	{
		if (!G_ValidTarget(self, e, true))
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

		fire_acid(self, self->s.origin, forward, 200, 64, speed, 20, 10.0);
		
		//FIXME: only need to do this once
		self->monsterinfo.attack_finished = level.time + 2.0;
		self->s.frame = GASSER_FRAMES_ATTACK_END-2;
		//gi.sound (self, CHAN_VOICE, gi.soundindex ("weapons/twang.wav"), 1, ATTN_NORM, 0);
	}	
}

void gasser_attack (edict_t *self)
{
	vec3_t start;

	if (self->monsterinfo.attack_finished > level.time)
		return;

	VectorCopy(self->s.origin, start);
	start[2] = self->absmax[2] + 8;
	SpawnGasCloud(self, start, self->dmg, self->dmg_radius, 4.0);

	self->s.frame = GASSER_FRAMES_ATTACK_START+2;
	self->monsterinfo.attack_finished = level.time + GASSER_REFIRE;
}

qboolean gasser_findtarget (edict_t *self)
{
	edict_t *e=NULL;

	while ((e = findclosestradius_targets(e, self, GASSER_RANGE)) != NULL)
	{
		if (!G_ValidTarget_Lite(self, e, true))
			continue;

		self->enemy = e;
		return true;
	}
	self->enemy = NULL;
	return false;
}

void gasser_think (edict_t *self)
{
	if (!organ_checkowner(self))
		return;

	organ_restoreMoveType(self);

//	if (level.time > self->lasthurt + 1.0)
//		M_Regenerate(self, 300, 10, true, false, false, &self->monsterinfo.regen_delay1);

	V_HealthCache(self, (int)(0.2 * self->max_health), 1);

	if (gasser_findtarget(self))
		gasser_attack(self);
	//gasser_acidattack(self);

	if (self->s.frame < GASSER_FRAMES_ATTACK_END)
		G_RunFrames(self, GASSER_FRAMES_ATTACK_START, GASSER_FRAMES_ATTACK_END, false);
	else if (self->s.frame < GASSER_FRAMES_REARM_END && level.time > self->monsterinfo.attack_finished - 0.2)
		G_RunFrames(self, GASSER_FRAMES_REARM_START, GASSER_FRAMES_REARM_END, false);
	else if (level.time > self->monsterinfo.attack_finished)
	{
		gascloud_sparks(self, 1, 32);
		G_RunFrames(self, GASSER_FRAMES_IDLE_START, GASSER_FRAMES_IDLE_END, false);
	}

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

	self->nextthink = level.time + FRAMETIME;
}

void gasser_grow (edict_t *self)
{
	if (!organ_checkowner(self))
		return;

	organ_restoreMoveType(self);

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
		self->activator->num_gasser--;
		cur = self->activator->num_gasser;
		self->monsterinfo.slots_freed = true;
		
		if (PM_MonsterHasPilot(attacker))
			attacker = attacker->owner;

		if (attacker->client)
			safe_cprintf(self->activator, PRINT_HIGH, "Your gasser was killed by %s (%d/%d remain)\n", attacker->client->pers.netname, cur, max);
		else if (attacker->mtype)
			safe_cprintf(self->activator, PRINT_HIGH, "Your gasser was killed by a %s (%d/%d remain)\n", V_GetMonsterName(attacker), cur, max);
		else
			safe_cprintf(self->activator, PRINT_HIGH, "Your gasser was killed by a %s (%d/%d remain)\n", attacker->classname, cur, max);
	}

	if (self->health <= self->gib_health || organ_explode(self))
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
	gi.linkentity(self);
}

edict_t *CreateGasser (edict_t *ent, int skill_level)
{
	edict_t *e;

	e = G_Spawn();
	e->style = 1; //growing
	e->activator = ent;
	e->think = gasser_grow;
	e->nextthink = level.time + FRAMETIME;
	e->s.modelindex = gi.modelindex ("models/objects/organ/gas/tris.md2");
	e->s.renderfx |= RF_IR_VISIBLE;
	e->solid = SOLID_BBOX;
	e->movetype = MOVETYPE_TOSS;
	e->clipmask = MASK_MONSTERSOLID;
	e->svflags |= SVF_MONSTER;// tells physics to clip on any solid object (not just walls)
	e->mass = 500;
	e->classname = "gasser";
	e->takedamage = DAMAGE_AIM;
	e->max_health = GASSER_INITIAL_HEALTH + GASSER_ADDON_HEALTH * skill_level;
	e->health = 0.5*e->max_health;
	e->dmg = GASSER_INITIAL_DAMAGE + GASSER_ADDON_DAMAGE * skill_level;
	e->dmg_radius = GASSER_INITIAL_ATTACK_RANGE + GASSER_ADDON_ATTACK_RANGE * skill_level;

	e->monsterinfo.level = skill_level;
	e->gib_health = -1.25 * BASE_GIB_HEALTH;
	e->s.frame = GASSER_FRAMES_IDLE_START;
	e->die = gasser_die;
	e->touch = organ_touch;
	VectorSet(e->mins, -8, -8, -5);
	VectorSet(e->maxs, 8, 8, 15);
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

	if (ent->num_gasser >= GASSER_MAX_COUNT)
	{
		safe_cprintf(ent, PRINT_HIGH, "You have reached the maximum amount of gassers (%d)\n", GASSER_MAX_COUNT);
		return;
	}

	gasser = CreateGasser(ent, ent->myskills.abilities[GASSER].current_level);
	if (!G_GetSpawnLocation(ent, 100, gasser->mins, gasser->maxs, start))
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
		
		if (PM_MonsterHasPilot(attacker))
			attacker = attacker->owner;

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

	if (self->health <= self->gib_health || organ_explode(self))
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
		self->enemy->cocoon_owner = self->cocoon_owner;

		if (self->enemy->client)
			safe_cprintf(self->enemy, PRINT_HIGH, "You have gained a damage/defense bonus of +%.0f%c for %.0f seconds\n", 
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
	if (!G_ValidTargetEnt(other, true) || !OnSameTeam(ent, other))
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
}

void cocoon_think (edict_t *self)
{
	trace_t tr;

	if (!G_EntIsAlive(self->activator))
	{
		organ_remove(self, false);
		return;
	}

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
    int talentLevel = vrx_get_talent_level(ent, TALENT_PHANTOM_COCOON);

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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
	e->cocoon_owner = ent;
=======
	e->owner = ent;
>>>>>>> 3cab0a7... split exp in invasion; add bonus exp for assist actions
=======
	e->cocoon_owner = ent;
>>>>>>> 6a6b0b2... fix assist exp crashes
=======
	e->cocoon_owner = ent;
>>>>>>> 6a6b0b2437504a942b2fef0f3900f9866dba1d4c
	
	//Talent: Phantom Cocoon - frames before cloaking
	if (talentLevel > 0)
		e->monsterinfo.jumpdn = 50 - 8 * talentLevel;
	else
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
	if (!G_GetSpawnLocation(ent, 100, cocoon->mins, cocoon->maxs, start))
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

	safe_cprintf(ent, PRINT_HIGH, "Cocoon created\n");
	gi.sound(cocoon, CHAN_VOICE, gi.soundindex("organ/organe3.wav"), 1, ATTN_STATIC, 0);

	ent->client->pers.inventory[power_cube_index] -= COCOON_COST;
	ent->client->ability_delay = level.time + COCOON_DELAY;

	//  entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}