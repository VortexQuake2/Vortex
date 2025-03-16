#include "g_local.h"


float vrx_increase_monster_damage_by_talent(edict_t *owner, float damage)
{
	float bonus = 0.1;
	if(owner && owner->client)
	{
		// oblation talent provides +10-20% dmg/lv
		if (pvm->value)
			bonus = 0.2;
        int talentLevel = vrx_get_talent_level(owner, TALENT_OBLATION);
		if (talentLevel > 0) damage *= 1 + bonus * talentLevel;
	}
	return damage;
}

//
// monster weapons
//

//FIXME mosnters should call these with a totally accurate direction
// and we can mess it up based on skill.  Spread should be for normal
// and we can tighten or loosen based on skill.  We could muck with
// the damages too, but I'm not sure that's such a good idea.
void monster_fire_bullet (edict_t *self, vec3_t start, vec3_t dir, int damage, int kick, int hspread, int vspread, int flashtype)
{
	float chance;

	// holy freeze reduces firing rate by 50%
	if (que_typeexists(self->curses, AURA_HOLYFREEZE))
	{
		if (random() <= 0.5)
			return;
	}

	// chill effect reduces attack rate/refire
	if (self->chill_time > level.time)
	{
		chance = 1 / (1 + CHILL_DEFAULT_BASE + CHILL_DEFAULT_ADDON * self->chill_level);
		if (random() > chance)
			return;
	}

	damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
	fire_bullet (self, start, dir, damage, kick, hspread, vspread, MOD_UNKNOWN);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}

static void debris_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
	G_FreeEdict(self);
}

void ThrowShell(edict_t* self, char* modelname, vec3_t start)
{
	edict_t* chunk;
	vec3_t	forward, right, target;

	chunk = G_Spawn();
	VectorCopy(start, chunk->s.origin);
	gi.setmodel(chunk, modelname);

	// projectile direction is forward of us, randomly offset to the left or right
	AngleVectors(self->s.angles, forward, right, NULL);
	VectorMA(start, 512, forward, target);
	VectorMA(target, crandom() * GetRandom(1, 64), right, target);
	VectorSubtract(target, start, forward);
	VectorNormalize(forward);

	// apply direction to object
	VectorMA(chunk->velocity, GetRandom(128, 256), forward, chunk->velocity);
	chunk->velocity[2] = GetRandom(128, 256);

	chunk->movetype = MOVETYPE_BOUNCE;
	chunk->solid = SOLID_NOT;
	chunk->avelocity[0] = random() * 600;
	chunk->avelocity[1] = random() * 600;
	chunk->avelocity[2] = random() * 600;
	chunk->think = G_FreeEdict;
	chunk->nextthink = level.time + GetRandom(2, 4);
	chunk->s.frame = 0;
	chunk->flags = 0;
	chunk->classname = "debris";
	chunk->takedamage = DAMAGE_YES;
	chunk->die = debris_die;
	gi.linkentity(chunk);
}

void monster_fire_20mm(edict_t* self, vec3_t start, vec3_t dir, int damage, int kick, int flashtype)
{
	float chance;

	// holy freeze reduces firing rate by 50%
	if (que_typeexists(self->curses, AURA_HOLYFREEZE))
	{
		if (random() <= 0.5)
			return;
	}

	// chill effect reduces attack rate/refire
	if (self->chill_time > level.time)
	{
		chance = 1 / (1 + CHILL_DEFAULT_BASE + CHILL_DEFAULT_ADDON * self->chill_level);
		if (random() > chance)
			return;
	}

	damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
	fire_20mm(self, start, dir, damage, kick);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);

	gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/sgun1.wav"), 1, ATTN_NORM, 0);

	if (vrx_spawn_nonessential_ent(self->s.origin))
		ThrowShell(self, "models/objects/shell1/tris.md2", start);
}

void monster_fire_shotgun (edict_t *self, vec3_t start, vec3_t aimdir, float damage, int kick, int hspread, int vspread, int count, int flashtype)
{
	float chance;

	// holy freeze reduces firing rate by 50%
	if (que_typeexists(self->curses, AURA_HOLYFREEZE))
	{
		if (random() <= 0.5)
			return;
	}

	// chill effect reduces attack rate/refire
	if (self->chill_time > level.time)
	{
		chance = 1 / (1 + CHILL_DEFAULT_BASE + CHILL_DEFAULT_ADDON * self->chill_level);
		if (random() > chance)
			return;
	}

	damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
	fire_shotgun (self, start, aimdir, damage, kick, hspread, vspread, count, MOD_UNKNOWN);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}

void monster_fire_blaster (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect, int proj_type, float duration, qboolean bounce, int flashtype)
{
	int mod;
	float chance;
	qboolean hyperblaster = false;

	// holy freeze reduces firing rate by 50%
	if (que_typeexists(self->curses, AURA_HOLYFREEZE))
	{
		if (random() <= 0.5)
			return;
	}

	// chill effect reduces attack rate/refire
	if (self->chill_time > level.time)
	{
		chance = 1 / (1 + CHILL_DEFAULT_BASE + CHILL_DEFAULT_ADDON * self->chill_level);
		if (random() > chance)
			return;
	}

	if (proj_type == BLASTER_PROJ_BLAST)
		mod = MOD_BLASTER;
	else
		mod = MOD_HYPERBLASTER;

	damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
	fire_blaster(self, start, dir, damage, speed, effect, proj_type, mod, duration, bounce);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}	

void monster_fire_grenade (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int flashtype)
{
	float	radius, chance;

	// holy freeze reduces firing rate by 50%
	if (que_typeexists(self->curses, AURA_HOLYFREEZE))
	{
		if (random() <= 0.5)
			return;
	}

	// chill effect reduces attack rate/refire
	if (self->chill_time > level.time)
	{
		chance = 1 / (1 + CHILL_DEFAULT_BASE + CHILL_DEFAULT_ADDON * self->chill_level);
		if (random() > chance)
			return;
	}

	radius = damage;

	// cap damage radius
	if (damage > 150)
		radius = 150;

	// cap speed
	if (speed > 1000)
		speed = 1000;

	damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
	fire_grenade (self, start, aimdir, damage, speed, 2.5, radius, damage);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}

void monster_fire_rocket (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype)
{
	float	radius, chance;

	// holy freeze reduces firing rate by 50%
	if (que_typeexists(self->curses, AURA_HOLYFREEZE))
	{
		if (random() <= 0.5)
			return;
	}

	// chill effect reduces attack rate/refire
	if (self->chill_time > level.time)
	{
		chance = 1 / (1 + CHILL_DEFAULT_BASE + CHILL_DEFAULT_ADDON * self->chill_level);
		if (random() > chance)
			return;
	}

	radius = damage;

	// cap damage radius
	if (damage > 125)
		radius = 125;

	// cap speed
	if (speed > 1000)
		speed = 1000;

	damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
	fire_rocket (self, start, dir, damage, speed, radius, damage);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}	

void monster_fire_railgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int flashtype)
{
	float chance;

	// holy freeze reduces firing rate by 50%
	if (que_typeexists(self->curses, AURA_HOLYFREEZE))
	{
		if (random() <= 0.5)
			return;
	}

	// chill effect reduces attack rate/refire
	if (self->chill_time > level.time)
	{
		chance = 1 / (1 + CHILL_DEFAULT_BASE + CHILL_DEFAULT_ADDON * self->chill_level);
		if (random() > chance)
			return;
	}

	damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
	fire_rail (self, start, aimdir, damage, kick);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}

void monster_fire_bfg (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int kick, float damage_radius, int flashtype)
{
	float chance;

	// holy freeze reduces firing rate by 50%
	if (que_typeexists(self->curses, AURA_HOLYFREEZE))
	{
		if (random() <= 0.5)
			return;
	}

	// chill effect reduces attack rate/refire
	if (self->chill_time > level.time)
	{
		chance = 1 / (1 + CHILL_DEFAULT_BASE + CHILL_DEFAULT_ADDON * self->chill_level);
		if (random() > chance)
			return;
	}

	damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
	fire_bfg (self, start, aimdir, damage, speed, damage_radius);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}

void fire_sword ( edict_t *self, vec3_t start, vec3_t aimdir, int damage, int length, int color);
void monster_fire_sword (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int flashtype)
{
	damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
	fire_sword (self, start, aimdir, damage, kick, 0xd2d3d2d3);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}

void fire_fireball(edict_t* self, vec3_t start, vec3_t aimdir, int damage, float damage_radius, int speed, int flames, int flame_damage);
void monster_fire_fireball(edict_t* self)
{
	int damage, radius, speed, flames, flame_damage;
	vec3_t	forward, start;

	if (!G_EntExists(self->enemy))
		return;

	float slvl = drone_damagelevel(self);

	damage = FIREBALL_INITIAL_DAMAGE + FIREBALL_ADDON_DAMAGE * slvl;
	damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
	radius = FIREBALL_INITIAL_RADIUS + FIREBALL_ADDON_RADIUS * slvl;
	flame_damage = FIREBALL_ADDON_FLAMEDMG * slvl;
	speed = FIREBALL_INITIAL_SPEED + FIREBALL_ADDON_SPEED * slvl;
	flames = FIREBALL_INITIAL_FLAMES + FIREBALL_ADDON_FLAMES * slvl;

	MonsterAim(self, M_PROJECTILE_ACC, speed, true, 0, forward, start);
	fire_fireball(self, start, forward, damage, radius, speed, flames, flame_damage);

	gi.sound(self, CHAN_ITEM, gi.soundindex("abilities/firecast.wav"), 1, ATTN_NORM, 0);
}

void fire_poison(edict_t* self, vec3_t start, vec3_t aimdir, int impact_dmg, float impact_rad, int speed, int poison_dmg, int poison_dur, qboolean cloud);
void monster_fire_poison(edict_t* self)
{
	int damage, radius, speed;
	float slvl, chance;
	vec3_t	forward, start;

	if (!G_EntExists(self->enemy))
		return;

	slvl = drone_damagelevel(self);

	damage = POISON_INITIAL_DAMAGE + POISON_ADDON_DAMAGE * slvl;
	damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
	radius = POISON_INITIAL_RADIUS + POISON_ADDON_RADIUS * slvl;
	speed = POISON_INITIAL_SPEED + POISON_ADDON_SPEED * slvl;

	MonsterAim(self, M_PROJECTILE_ACC, speed, true, 0, forward, start);

	// chance that a poison cloud spawns on impact increases with level
	chance = 0.02 * slvl;

	if (chance > random())
		fire_poison(self, start, forward, 0, radius, speed, damage, 10, true);
	else
		fire_poison(self, start, forward, 0, radius, speed, damage, 10, false);
}

void fire_icebolt(edict_t* self, vec3_t start, vec3_t aimdir, int damage, float damage_radius, int speed, int chillLevel, float chillDuration, float freezeDuration);
void monster_fire_icebolt(edict_t* self)
{
	int damage, radius, speed;
	float slvl, duration;
	vec3_t	forward, start;

	if (!G_EntExists(self->enemy))
		return;

	slvl = drone_damagelevel(self);

	damage = ICEBOLT_INITIAL_DAMAGE + ICEBOLT_ADDON_DAMAGE * slvl;
	damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
	radius = ICEBOLT_INITIAL_RADIUS + ICEBOLT_ADDON_RADIUS * slvl;
	speed = ICEBOLT_INITIAL_SPEED + ICEBOLT_ADDON_SPEED * slvl;
	duration = ICEBOLT_INITIAL_CHILL_DURATION + ICEBOLT_ADDON_CHILL_DURATION * slvl;

	MonsterAim(self, M_PROJECTILE_ACC, speed, true, 0, forward, start);
	fire_icebolt(self, start, forward, damage, radius, speed, (int)(2 * slvl), duration, 0);
	// Play sound effect
	gi.sound(self, CHAN_WEAPON, gi.soundindex("spells/coldcast.wav"), 1, ATTN_NORM, 0);
}

//
// Monster utility functions
//
/*
static void M_FliesOff (edict_t *self)
{
	self->s.effects &= ~EF_FLIES;
	self->s.sound = 0;
}

static void M_FliesOn (edict_t *self)
{
	self->s.effects |= EF_FLIES;
	self->s.sound = gi.soundindex ("infantry/inflies1.wav");
	self->think = M_FliesOff;
	self->nextthink = level.time + 60;
}

void M_FlyCheck (edict_t *self)
{
	if (self->waterlevel)
		return;

	if (random() > 0.5)
		return;

	self->think = M_FliesOn;
	self->nextthink = level.time + 5 + 10 * random();
}

void AttackFinished (edict_t *self, float time)
{
	self->monsterinfo.attack_finished = level.time + time;
}
*/

void M_CheckGround (edict_t *ent)
{
	vec3_t		point;
	trace_t		trace;

	if (ent->flags & (FL_SWIM|FL_FLY))
		return;

	if (ent->velocity[2] > 100)
	{
		ent->groundentity = NULL;
		return;
	}

// if the hull point one-quarter unit down is solid the entity is on ground
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] - 1;//0.25;

	trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, point, ent, MASK_MONSTERSOLID);

	// check steepness
	if ( trace.plane.normal[2] < 0.7 && !trace.startsolid)
	{
		ent->groundentity = NULL;
		return;
	}

//	ent->groundentity = trace.ent;
//	ent->groundentity_linkcount = trace.ent->linkcount;
//	if (!trace.startsolid && !trace.allsolid)
//		VectorCopy (trace.endpos, ent->s.origin);
	if (!trace.startsolid && !trace.allsolid)
	{
		//GHz START
		// monster wasn't previously on the ground (aireborne), so call touchdown function if we have one
		if (!ent->groundentity && ent->monsterinfo.touchdown)
			ent->monsterinfo.touchdown(ent);
		//GHz END
		VectorCopy(trace.endpos, ent->s.origin);
		ent->groundentity = trace.ent;
		ent->groundentity_linkcount = trace.ent->linkcount;
		ent->velocity[2] = 0;
	}
}

void M_CatagorizePosition (edict_t *ent)
{
	vec3_t		point;
	int			cont;

//
// get waterlevel
//
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] + ent->mins[2] + 1;	
	cont = gi.pointcontents (point);

	if (!(cont & MASK_WATER))
	{
		ent->waterlevel = 0;
		ent->watertype = 0;
		return;
	}

	ent->watertype = cont;
	ent->waterlevel = 1;
	point[2] += 26;
	cont = gi.pointcontents (point);
	if (!(cont & MASK_WATER))
		return;

	ent->waterlevel = 2;
	point[2] += 22;
	cont = gi.pointcontents (point);
	if (cont & MASK_WATER)
		ent->waterlevel = 3;
}


void M_WorldEffects (edict_t *ent)
{
	//int		dmg;
	
	if (ent->waterlevel == 0)
	{
		if (ent->flags & FL_INWATER)
		{	
			gi.sound (ent, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
			ent->flags &= ~FL_INWATER;
		}
		return;
	}

	if ((ent->watertype & CONTENTS_LAVA) && !(ent->flags & FL_IMMUNE_LAVA))
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 0.2;
			T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 10*ent->waterlevel, 0, 0, MOD_LAVA);
		}
	}
	if ((ent->watertype & CONTENTS_SLIME) && !(ent->flags & FL_IMMUNE_SLIME))
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 1;
			T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 4*ent->waterlevel, 0, 0, MOD_SLIME);
		}
	}
	
	if ( !(ent->flags & FL_INWATER) )
	{	
		if (ent->watertype & CONTENTS_LAVA)
			if (random() <= 0.5)
				gi.sound (ent, CHAN_BODY, gi.soundindex("player/lava1.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound (ent, CHAN_BODY, gi.soundindex("player/lava2.wav"), 1, ATTN_NORM, 0);
		else if (ent->watertype & CONTENTS_SLIME)
			gi.sound (ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		else if (ent->watertype & CONTENTS_WATER)
			gi.sound (ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);

		ent->flags |= FL_INWATER;
		ent->damage_debounce_time = 0;
	}
}


void M_droptofloor (edict_t *ent)
{
	vec3_t		end;
	trace_t		trace;

	ent->s.origin[2] += 1;
	VectorCopy (ent->s.origin, end);
	end[2] -= 256;
	
	trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);

	if (trace.fraction == 1 || trace.allsolid)
		return;

	VectorCopy (trace.endpos, ent->s.origin);

	gi.linkentity (ent);
	M_CheckGround (ent);
	M_CatagorizePosition (ent);
}

void EmpEffects (edict_t *ent);

void M_SetEffects (edict_t *ent)
{
	V_SetEffects(ent);
}

qboolean vrx_holdframe(edict_t* self)
{
	return self->monsterinfo.aiflags & AI_HOLD_FRAME || (self->health > 0 && que_typeexists(self->curses, CURSE_FROZEN));
}

void vrx_adjust_moveframe_scale(edict_t* self)
{
	float	temp;
	que_t* slot = NULL;

	//4.5 monster bonus flags
	if (self->monsterinfo.bonus_flags & BF_FANATICAL)
		self->monsterinfo.scale *= 2.0;
	if (self->monsterinfo.bonus_flags & BF_GHOSTLY)
		self->monsterinfo.scale *= 0.5;

	// is this monster slowed by the holyfreeze aura?
	slot = que_findtype(self->curses, slot, AURA_HOLYFREEZE);
	if (slot)
	{
		temp = 1 / (1 + 0.1 * slot->ent->owner->myskills.abilities[HOLY_FREEZE].current_level);
		if (temp < 0.25) temp = 0.25;
		self->monsterinfo.scale *= temp;
	}

	// chill effect slows monster movement rate
	if (self->chill_time > level.time)
		self->monsterinfo.scale *= 1 / (1 + CHILL_DEFAULT_BASE + CHILL_DEFAULT_ADDON * self->chill_level);

	// 3.5 weaken slows down target
	if ((slot = que_findtype(self->curses, NULL, WEAKEN)) != NULL)
	{
		temp = 1 / (1 + WEAKEN_SLOW_BASE + WEAKEN_SLOW_BONUS
			* slot->ent->owner->myskills.abilities[WEAKEN].current_level);
		self->monsterinfo.scale *= temp;
	}

	//caltrops
	if (self->slowed_time > level.time)
		self->monsterinfo.scale *= self->slowed_factor;
}

void M_MoveFrame_Reverse (edict_t* self)
{
	mmove_t* move;
	int		index;

	if (!self->inuse)
		return;

	move = self->monsterinfo.currentmove;

	// is the next frame between the first and last frame of currentmove?
	if ((self->monsterinfo.nextframe) && (self->monsterinfo.nextframe >= move->lastframe)
		&& (self->monsterinfo.nextframe <= move->firstframe))
	{
		self->s.frame = self->monsterinfo.nextframe;
		self->monsterinfo.nextframe = 0;
	}
	else
	{
		// have we reached the last frame of currentmove?
		if (self->s.frame == move->lastframe)
		{
			// does the currentmove have an end function?
			if (move->endfunc)
			{
				// call it
				move->endfunc(self);

				// regrab move, endfunc is very likely to change it
				move = self->monsterinfo.currentmove;

				// check for death
				if (self->svflags & SVF_DEADMONSTER)
					return;
			}
		}

		// is current frame not within the bounds of the first and last frame of currentmove?
		if (self->s.frame < move->lastframe || self->s.frame > move->firstframe)
		{
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
			self->s.frame = move->firstframe;
		}
		else
		{
			if (!vrx_holdframe(self))
			{
				self->s.frame--;
				if (self->s.frame < move->lastframe)
					self->s.frame = move->firstframe;
			}
		}
	}

	index = move->firstframe - self->s.frame;
	if (move->frame[index].aifunc)
		if (!vrx_holdframe(self))
		{
			self->monsterinfo.scale = 1.0;
			vrx_adjust_moveframe_scale(self);

			move->frame[index].aifunc(self, move->frame[index].dist * self->monsterinfo.scale);
		}
		else
		{
			// we're not going anywhere!
			move->frame[index].aifunc(self, 0);
		}

	if (move->frame[index].thinkfunc)
		move->frame[index].thinkfunc(self);
}

void M_MoveFrame (edict_t *self)
{
	mmove_t	*move;
	int		index;
//	int		frames;

//	edict_t *curse;


	if (!self->inuse)
		return;

	move = self->monsterinfo.currentmove;

	// if move frames are in reverse order, call reverse function
	if (move->firstframe > move->lastframe)
	{
		M_MoveFrame_Reverse(self);
		return;
	}

	// is the next frame between the first and last frame of currentmove?
	if ((self->monsterinfo.nextframe) && (self->monsterinfo.nextframe >= move->firstframe) 
		&& (self->monsterinfo.nextframe <= move->lastframe))
	{
		self->s.frame = self->monsterinfo.nextframe;
		self->monsterinfo.nextframe = 0;
	}
	else
	{
		// have we reached the last frame of currentmove?
		if (self->s.frame == move->lastframe)
		{
			// does the currentmove have an end function?
			if (move->endfunc)
			{
				// call it
				move->endfunc (self);

				// regrab move, endfunc is very likely to change it
				move = self->monsterinfo.currentmove;

				// check for death
				if (self->svflags & SVF_DEADMONSTER)
					return;
			}
		}

		// is current frame not within the bounds of the first and last frame of currentmove?
		if (self->s.frame < move->firstframe || self->s.frame > move->lastframe)
		{
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
			self->s.frame = move->firstframe;
		}
		else
		{
			if (!vrx_holdframe(self))
			{
				self->s.frame++;
				if (self->s.frame > move->lastframe)
					self->s.frame = move->firstframe;
			}
		}
	}

	index = self->s.frame - move->firstframe;
	if (move->frame[index].aifunc)
		if (!vrx_holdframe(self))
		{
			self->monsterinfo.scale = 1.0;
			vrx_adjust_moveframe_scale(self);
			
			move->frame[index].aifunc (self, move->frame[index].dist * self->monsterinfo.scale);
		}
		else
		{
			// we're not going anywhere!
			move->frame[index].aifunc (self, 0);
		}

	if (move->frame[index].thinkfunc)
		move->frame[index].thinkfunc (self);
}

static void M_FliesOff (edict_t *self)
{
	self->s.effects &= ~EF_FLIES;
	self->s.sound = 0;
}

static void M_FliesOn (edict_t *self)
{
	if (self->waterlevel)
		return;
	self->s.effects |= EF_FLIES;
	self->s.sound = gi.soundindex ("infantry/inflies1.wav");
	self->think = M_FliesOff;
	self->nextthink = level.time + 60;
}
void M_FlyCheck (edict_t *self)
{
	if (self->waterlevel)
		return;

	if (random() > 0.5)
		return;

	self->think = M_FliesOn;
	self->nextthink = level.time + 5 + 10 * random();
}

void AttackFinished (edict_t *self, float time)
{
	self->monsterinfo.attack_finished = level.time + time;
}