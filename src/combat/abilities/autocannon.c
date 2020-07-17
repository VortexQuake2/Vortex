#include "g_local.h"

#define AUTOCANNON_FRAME_ATTACK_START	7
#define AUTOCANNON_FRAME_ATTACK_END		17//20
#define AUTOCANNON_FRAME_SLEEP_START	28
#define AUTOCANNON_FRAME_SLEEP_END		38

#define AUTOCANNON_STATUS_READY			0
#define AUTOCANNON_STATUS_ATTACK		1


void autocannon_remove (edict_t *self, char *message)
{
	if (self->deadflag == DEAD_DEAD)
		return; // don't call more than once

	if (self->creator)
	{
		self->creator->num_autocannon--; // decrement counter

		if (self->creator->inuse && message)
			safe_cprintf(self->creator, PRINT_HIGH, message);
	}

	// mark for removal
	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->deadflag = DEAD_DEAD;
	self->think = BecomeExplosion1;
	self->nextthink = level.time + FRAMETIME;
	gi.unlinkentity(self);
}

void RemoveAutoCannons (edict_t *ent)
{
	edict_t *scan = NULL;

	while((scan = G_Find(scan, FOFS(classname), "autocannon")) != NULL)
	{
		// remove all autocannons owned by this entity
		if (scan && scan->inuse && scan->creator && scan->creator->client && (scan->creator == ent))
			autocannon_remove(scan, NULL);
	}

	ent->num_autocannon = 0; // make sure the counter is zeroed out
}

void autocannon_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	char *s;

	if (self->deadflag == DEAD_DEAD)
		return; // don't call more than once

	if (attacker->client)
		s = va("Your autocannon was destroyed by %s (%d/%d).\n", attacker->client->pers.netname, 
			self->creator->num_autocannon-1, AUTOCANNON_MAX_UNITS);
	else
		s = va("Your autocannon was destroyed (%d/%d).\n", self->creator->num_autocannon-1, AUTOCANNON_MAX_UNITS);
	autocannon_remove(self, s);
}

void mzfire_think (edict_t *self)
{
	int		attack_frame=AUTOCANNON_FRAME_ATTACK_START;
	float	dist=10;
	vec3_t	start, forward;

	if (!self->owner || !self->owner->inuse || (level.framenum > self->count))
	{
		G_FreeEdict(self);
		return;
	}

	if (self->owner->s.frame == attack_frame)
		dist = 2;
	else if (self->owner->s.frame == attack_frame+1)
		dist = 0;
	else if (self->owner->s.frame == attack_frame+2)
		dist = -6;

	// move into position
	VectorCopy(self->owner->s.origin, start);
	start[2] += self->owner->viewheight;
	AngleVectors(self->owner->s.angles, forward, NULL, NULL);
	VectorMA(start, self->owner->maxs[0]+dist, forward, start);
	VectorCopy(start, self->s.origin);
	gi.linkentity(self);

	// modify angles
	vectoangles(forward, forward);
	forward[PITCH] += 90;
	AngleCheck(&forward[PITCH]);
	VectorCopy(forward, self->s.angles);
	

	// play frame sequence
	G_RunFrames(self, 4, 15, false);

	self->nextthink = level.time + FRAMETIME;
}

void MuzzleFire (edict_t *self)
{
	edict_t *fire;

	// create the fire ent
	fire = G_Spawn();
	fire->movetype = MOVETYPE_NONE;
	fire->owner = self;
	fire->count = (int)(level.framenum + 0.2 / FRAMETIME); // duration
	fire->classname = "mzfire";
	fire->think = mzfire_think;
	fire->nextthink = level.time + FRAMETIME;
	fire->s.modelindex = gi.modelindex ("models/fire/tris.md2");
}

void autocannon_attack (edict_t *self)
{
	if (level.time > self->delay)
	{
		vec3_t	start, end, forward, angles;
		trace_t	tr;

		if (!self->light_level)
		{
			gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			self->delay = level.time + AUTOCANNON_ATTACK_DELAY;
			return;
		}

		MuzzleFire(self);

		//  entity made a sound, used to alert monsters
		self->lastsound = level.framenum;

		// trace directly ahead of autocannon
		VectorCopy(self->s.origin, start);
		start[2] += self->viewheight;
		VectorCopy(self->s.angles, angles);//4.5
		angles[PITCH] = self->random;//4.5
		AngleVectors(angles, forward, NULL, NULL);
		VectorMA(start, AUTOCANNON_RANGE, forward, end);
		tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);

		// fire if an enemy crosses its path
		if (G_EntExists(tr.ent))
			T_Damage (tr.ent, self, self->creator, forward, tr.endpos, tr.plane.normal, 
				self->dmg, self->dmg, DAMAGE_PIERCING, MOD_AUTOCANNON);
		
		// throw debris at impact point
		if (random() < 0.5)
			ThrowDebris (self, "models/objects/debris2/tris.md2", 3, tr.endpos);
		if (random() < 0.5)
			ThrowDebris (self, "models/objects/debris2/tris.md2", 3, tr.endpos);

		gi.sound (self, CHAN_WEAPON, gi.soundindex("weapons/sgun1.wav"), 1, ATTN_NORM, 0);

		self->style = AUTOCANNON_STATUS_ATTACK;
		self->delay = level.time + AUTOCANNON_ATTACK_DELAY;
		self->light_level--; // reduce ammo
	}
}

void autocannon_runframes (edict_t *self)
{
	if (self->style == AUTOCANNON_STATUS_ATTACK)
	{
		G_RunFrames(self, AUTOCANNON_FRAME_ATTACK_START, AUTOCANNON_FRAME_ATTACK_END, false);
		if (self->s.frame == AUTOCANNON_FRAME_ATTACK_END)
		{
			self->s.frame = 0; // reset frame
			self->style = AUTOCANNON_STATUS_READY; // reset status
		}
	}
	else
		self->s.frame = 0;
}

void autocannon_aim (edict_t *self)
{
	float	angle;
	vec3_t v;

	if (self->wait > level.time)
	{
		VectorCopy(self->move_origin, v);
		// 0.1=5-6 degrees/frame, 0.2=11-12 degrees/frame, 0.3=17-18 degrees/frame
		VectorScale(v, 0.1, v);
		VectorAdd(v, self->movedir, v);
		VectorNormalize(v);
		VectorCopy(v, self->movedir);
		vectoangles(v, self->s.angles);

		angle = self->s.angles[PITCH];
		AngleCheck(&angle);
		
		if (angle > 270)
		{
			if (angle < 350)
				self->s.angles[PITCH] = 350;
		}
		else if (angle > 10)
			self->s.angles[PITCH] = 30;

		//gi.dprintf("pitch=%d\n", (int)self->s.angles[PITCH]);
	}
	
}

qboolean autocannon_checkstatus (edict_t *self)
{
	vec3_t	end;
	trace_t tr;

	// must have live owner
	if (!G_EntIsAlive(self->creator))
		return false;

	// must be on firm ground
	VectorCopy(self->s.origin, end);
	end[2] -= fabsf(self->mins[2])+1;
	tr = gi.trace(self->s.origin, self->mins, self->maxs, end, NULL, MASK_SOLID);
	if (tr.fraction == 1.0)
		return false;

	return true; // everything is OK
}

qboolean autocannon_effects (edict_t *self)
{
	self->s.effects = self->s.renderfx = 0;

	if (!(level.framenum % (int)sv_fps->value) && self->light_level < 1)
	{
		self->s.effects |= EF_COLOR_SHELL;
		self->s.renderfx |= RF_SHELL_YELLOW;
		return true;
	}

	if (!(sf2qf(level.framenum) % 20) && self->light_level < 2)
	{
		self->s.effects |= EF_COLOR_SHELL;
		self->s.renderfx |= RF_SHELL_YELLOW;
		return true;
	}

	return false;
}

void autocannon_think (edict_t *self)
{
	vec3_t	angles;//4.5 aiming angles are different than model/gun angles
	vec3_t	start, end, forward;
	trace_t tr;

	if (!autocannon_checkstatus(self))
	{
		autocannon_remove(self, NULL);
		return;
	}

	if (self->holdtime > level.time)
	{
		if (!autocannon_effects(self))
			M_SetEffects(self);

		self->nextthink = level.time + FRAMETIME;
		return;
	}

	// low ammo warning
	if ((level.framenum % (int)(5 / FRAMETIME)) == 0)
	{
	if (self->light_level < 1)
		safe_cprintf(self->creator, PRINT_HIGH, "***AutoCannon is out of ammo. Re-arm with more shells.***\n");
	else if (self->light_level < 2)
		safe_cprintf(self->creator, PRINT_HIGH, "AutoCannon is low on ammo. Re-arm with more shells.\n");
	}

//	M_Regenerate(self, AUTOCANNON_REGEN_FRAMES, 10, true, false, false);
	M_ChangeYaw(self);

	if (!autocannon_effects(self))
		M_SetEffects(self);

	M_WorldEffects(self);
	//autocannon_aim(self);
	autocannon_runframes(self);

	// trace directly ahead of autocannon
	VectorCopy(self->s.origin, start);
	start[2] += self->viewheight;
	VectorCopy(self->s.angles, angles);//4.5
	angles[PITCH] = self->random;//4.5
	AngleVectors(angles, forward, NULL, NULL);
	VectorMA(start, AUTOCANNON_RANGE, forward, end);
	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);

	if (self->wait > level.time)
	{
		// draw aiming laser
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BFG_LASER);
		gi.WritePosition (start);
		gi.WritePosition (tr.endpos);
		gi.multicast (start, MULTICAST_PHS);
	}

	
	// fire if an enemy crosses its path
	if (G_EntIsAlive(tr.ent) && !OnSameTeam(self, tr.ent))
		autocannon_attack(self);

	self->nextthink = level.time + FRAMETIME;
}

void autocannon_buildthink (edict_t *self)
{
	// play sleep frames in reverse, gun is waking up
	G_RunFrames(self, AUTOCANNON_FRAME_SLEEP_START, AUTOCANNON_FRAME_SLEEP_END, true);
	if (self->s.frame == AUTOCANNON_FRAME_SLEEP_START)
	{
		if (!autocannon_checkstatus(self))
		{
			autocannon_remove(self, NULL);
			return;
		}

		self->think = autocannon_think;
		self->movetype = MOVETYPE_NONE; // lock-down

		//self->s.frame = AUTOCANNON_FRAME_ATTACK_START+2;//REMOVE THIS
	}

	self->nextthink = level.time + FRAMETIME;
}

void autocannon_status (edict_t *self, edict_t *other)
{
	safe_cprintf(other, PRINT_HIGH, "AutoCannon Status: Health (%d/%d) Ammo (%d/%d)\n", self->health, 
		self->max_health, self->light_level, self->count);

	self->sentrydelay = level.time + AUTOCANNON_TOUCH_DELAY;
}

void autocannon_reload (edict_t *self, edict_t *other)
{
	int required_ammo = self->count-self->light_level;
	int	*player_ammo = &other->client->pers.inventory[shell_index];

	// full ammo
	if (required_ammo < 1)
		return;
	// player has no ammo
	if (*player_ammo < 1)
		return;
	
	// player has enough ammo
	if (*player_ammo >= required_ammo)
	{
		self->light_level += required_ammo;
		*player_ammo -= required_ammo;
	}
	// player has less ammo than we require
	else
	{
		self->light_level += *player_ammo;
		*player_ammo = 0;
	}

	self->sentrydelay = level.time + AUTOCANNON_TOUCH_DELAY;
	gi.sound(other, CHAN_ITEM, gi.soundindex("plats/pt1_strt.wav"), 1, ATTN_STATIC, 0);
}

void autocannon_repair (edict_t *self, edict_t *other)
{
	int *powercubes = &other->client->pers.inventory[power_cube_index];

	// need to be hurt
	if (self->health >= self->max_health)
		return;

	// need powercubes to perform repairs
	if (*powercubes < 1)
		return;

	// if the player has more than enough pc's to fill up the autocannon
	if (*powercubes * (1/AUTOCANNON_REPAIR_COST) + self->health > self->max_health)
	{
		// spend just enough pc's to repair it
		*powercubes -= (self->max_health - self->health) * AUTOCANNON_REPAIR_COST;
		self->health = self->max_health;
	}
	// use all the player's pc's to repair as much as possible
	else 
	{
		self->health += *powercubes * (1/AUTOCANNON_REPAIR_COST);
		*powercubes = 0;
	}

	self->sentrydelay = level.time + AUTOCANNON_TOUCH_DELAY;
	gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/repair.wav"), 1, ATTN_NORM, 0);
}

void autocannon_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	// sanity check, only a teammate can repair/reload
	if (!other->client || !OnSameTeam(ent, other))
		return;

	// activate aiming laser
	ent->wait = level.time + 1.0;

	// time delay before next repair/reload can be performed
	if (ent->sentrydelay > level.time)
		return;

	autocannon_repair(ent, other);
	autocannon_reload(ent, other);
	autocannon_status(ent, other);
}

void CreateAutoCannon (edict_t *ent, int cost, float skill_mult, float delay_mult)
{
	int		talentLevel;
	float	ammo_mult=1.0;
	vec3_t	forward, start, end, angles;
	trace_t	tr;
	edict_t *cannon;

	cannon = G_Spawn();
	cannon->creator = ent;
	cannon->think = autocannon_buildthink;
	cannon->nextthink = level.time + AUTOCANNON_START_DELAY;
	cannon->s.modelindex = gi.modelindex ("models/des/tris.md2");
	//cannon->s.effects |= EF_PLASMA;
	cannon->s.renderfx |= RF_IR_VISIBLE;
	cannon->solid = SOLID_BBOX;
	cannon->movetype = MOVETYPE_TOSS;
	cannon->clipmask = MASK_MONSTERSOLID;
	cannon->deadflag = DEAD_NO;
	cannon->svflags &= ~SVF_DEADMONSTER;
	// NO one liked AI chasing
	//cannon->flags |= FL_CHASEABLE;
	cannon->mass = 300;
	cannon->classname = "autocannon";
	cannon->takedamage = DAMAGE_AIM;
	cannon->monsterinfo.level = ent->myskills.abilities[AUTOCANNON].current_level * skill_mult;
	cannon->light_level = AUTOCANNON_START_AMMO; // current ammo

	//Talent: Storage Upgrade
    talentLevel = vrx_get_talent_level(ent, TALENT_STORAGE_UPGRADE);
	ammo_mult += 0.2 * talentLevel;

	cannon->count = AUTOCANNON_INITIAL_AMMO+AUTOCANNON_ADDON_AMMO*cannon->monsterinfo.level; // max ammo
	cannon->count *= ammo_mult;

	cannon->health = AUTOCANNON_INITIAL_HEALTH+AUTOCANNON_ADDON_HEALTH*cannon->monsterinfo.level;
	cannon->dmg = AUTOCANNON_INITIAL_DAMAGE+AUTOCANNON_ADDON_DAMAGE*cannon->monsterinfo.level;
	cannon->gib_health = -100;
	cannon->viewheight = 10;
	cannon->max_health = cannon->health;
	cannon->die = autocannon_die;
	cannon->touch = autocannon_touch;
	VectorSet(cannon->mins, -28, -28, -24);
	VectorSet(cannon->maxs, 28, 28, 24);
	cannon->s.skinnum = 4;
	cannon->mtype = M_AUTOCANNON;
	cannon->yaw_speed = AUTOCANNON_YAW_SPEED;
	cannon->s.angles[YAW] = ent->s.angles[YAW];
	cannon->ideal_yaw = ent->s.angles[YAW];
	cannon->s.frame = AUTOCANNON_FRAME_SLEEP_END; // cannon starts out retracted

	// starting position for autocannon
	VectorCopy(ent->s.angles, angles);
	angles[PITCH] = 0;
	AngleVectors(angles, forward, NULL, NULL);
	VectorMA(ent->s.origin, ent->maxs[1]+cannon->maxs[1]+18, forward, start);

	// check for ground
	VectorCopy(start, end);
	end[2] += cannon->mins[2]+1;
	tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SHOT);
	if (tr.fraction < 1)
	{
		// move up from ground
		VectorCopy(tr.endpos, start);
		start[2] += fabsf(cannon->mins[2]) + 1;
	}

	// make sure station doesn't spawn in a solid
	tr = gi.trace(start, cannon->mins, cannon->maxs, start, NULL, MASK_SHOT);
	if (tr.contents & MASK_SHOT)
	{
		safe_cprintf (ent, PRINT_HIGH, "Can't build autocannon there.\n");
		G_FreeEdict(cannon);
		return;
	}
	
	// move into position
	VectorCopy(tr.endpos, cannon->s.origin);
	VectorCopy(cannon->s.origin, cannon->s.old_origin);
	gi.linkentity(cannon);

	ent->client->ability_delay = level.time + AUTOCANNON_DELAY * delay_mult;
	ent->holdtime = level.time + AUTOCANNON_BUILD_TIME * delay_mult;
	ent->client->pers.inventory[power_cube_index] -= cost;
	ent->num_autocannon++; // increment counter

	safe_cprintf(ent, PRINT_HIGH, "Built %d/%d autocannons.\n", ent->num_autocannon, AUTOCANNON_MAX_UNITS);
}

#define AUTOCANNON_AIM_NEAREST	0
#define AUTOCANNON_AIM_ALL		1

void autocannon_setaim (edict_t *ent, edict_t *cannon)
{
	vec3_t	forward, right, start, offset, end;
	trace_t	tr;

	// get start position for trace
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	// get end position for trace
	VectorMA(start, 8192, forward, end);

	// get vector to the point client is aiming at and convert to angles
	tr = gi.trace (start, NULL, NULL, end, ent, MASK_SOLID);
	VectorSubtract(tr.endpos, cannon->s.origin, forward);

	//VectorNormalize(forward);
	//VectorCopy(forward, cannon->move_origin); // vector to target

	vectoangles(forward, forward);

	// modify pitch angle, but don't go too far
	AngleCheck(&forward[PITCH]);
	if (forward[PITCH] > 270)
	{
		if (forward[PITCH] < 330)
			forward[PITCH] = 330;
	}
	else if (forward[PITCH] > 30)
		forward[PITCH] = 30;
	cannon->random = forward[PITCH];//4.5 this is our aiming pitch

	// modify pitch angle, but don't go too far
	//AngleCheck(&forward[PITCH]);
	if (forward[PITCH] > 270)
	{
		if (forward[PITCH] < 350)
			forward[PITCH] = 350;
	}
	else if (forward[PITCH] > 10)
		forward[PITCH] = 10;
	cannon->s.angles[PITCH] = forward[PITCH];// this is our gun/model pitch

	// copy angles to autocannon
	cannon->move_angles[YAW] = forward[YAW];
	cannon->ideal_yaw = forward[YAW];
	AngleCheck(&cannon->move_angles[YAW]);
	AngleCheck(&cannon->ideal_yaw);
	
	cannon->wait = level.time + 5.0;
}

void Cmd_AutoCannonAim_f (edict_t *ent, int option)
{
	edict_t *e=NULL;

	if (option == AUTOCANNON_AIM_NEAREST)
	{
		while ((e = findclosestradius (e, ent->s.origin, 512)) != NULL)
		{
			if (!e->inuse)
				continue;
			if (e->mtype != M_AUTOCANNON)
				continue;
			if (e->creator && e->creator->inuse && e->creator == ent)
			{
				autocannon_setaim(ent, e);
				break;
			}
		}
	}
	else if (option == AUTOCANNON_AIM_ALL)
	{
		while((e = G_Find(e, FOFS(classname), "autocannon")) != NULL)
		{
			if (e && e->inuse && e->creator && e->creator->client && (e->creator==ent))
				autocannon_setaim(ent, e);
		}
	}

	safe_cprintf(ent, PRINT_HIGH, "Aiming autocannon...\n");
}

void Cmd_AutoCannon_f (edict_t *ent)
{
	int talentLevel, cost=AUTOCANNON_COST;
	float skill_mult=1.0, cost_mult=1.0, delay_mult=1.0;//Talent: Rapid Assembly & Precision Tuning
	char	*arg;

	arg = gi.args();

	if (!Q_strcasecmp(arg, "remove"))
	{
		RemoveAutoCannons(ent);
		return;
	}

	if (!Q_strcasecmp(arg, "aim"))
	{
		Cmd_AutoCannonAim_f(ent,0);
		return;
	}

	if (!Q_strcasecmp(arg, "aimall"))
	{
		Cmd_AutoCannonAim_f(ent,1);
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

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[AUTOCANNON].current_level, AUTOCANNON_COST))
		return;
	if (ent->myskills.abilities[AUTOCANNON].disable)
		return;

	if (ent->num_autocannon >= AUTOCANNON_MAX_UNITS)
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't build any more autocannons.\n");
		return;
	}

	CreateAutoCannon(ent, cost, skill_mult, delay_mult);
}