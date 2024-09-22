/*
==============================================================================

BARON_FIRE

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/baron_fire.h"

static int sound_death1;
static int sound_death2;
static int sound_pain1;
static int sound_pain2;
static int sound_idle1;
static int sound_idle2;
static int sound_sight1;
static int sound_sight2;
static int sound_melee1;
static int sound_melee2;
static int sound_step1;
static int sound_step2;

void fire_meteor(edict_t* self, vec3_t end, int damage, int radius, int speed);

void baron_fire_idle(edict_t* self)
{
	if (random() > 0.5)
		gi.sound(self, CHAN_VOICE, sound_idle1, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_idle2, 1, ATTN_IDLE, 0);
}

mframe_t baron_fire_frames_stand[] =
{
	drone_ai_stand, 0, NULL, //FRAME_stand01
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL //FRAME_stand15
};
mmove_t	baron_fire_move_stand = { FRAME_stand01, FRAME_stand15, baron_fire_frames_stand, NULL };

void baron_fire_stand(edict_t* self)
{
	self->monsterinfo.currentmove = &baron_fire_move_stand;
}

void baron_fire_step1(edict_t* self)
{
	gi.sound(self, CHAN_BODY, sound_step1, 1, ATTN_IDLE, 0);
}

void baron_fire_step2(edict_t* self)
{
	gi.sound(self, CHAN_BODY, sound_step2, 1, ATTN_IDLE, 0);
}

mframe_t baron_fire_frames_walk[] =
{
	drone_ai_walk, 5, NULL,//FRAME_walk01
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, baron_fire_step2,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, baron_fire_step1,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL//FRAME_walk15
};
mmove_t baron_fire_move_walk = { FRAME_walk01, FRAME_walk15, baron_fire_frames_walk, NULL };

void baron_fire_walk(edict_t* self)
{
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &baron_fire_move_walk;
}

mframe_t baron_fire_frames_run[] =
{
	drone_ai_run, 25, baron_fire_step2,
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, NULL,//34
	drone_ai_run, 25, baron_fire_step1,//35
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, NULL
};

mmove_t baron_fire_move_run = { FRAME_run01, FRAME_run07, baron_fire_frames_run, NULL };

void baron_fire_run(edict_t* self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &baron_fire_move_stand;
	else
		self->monsterinfo.currentmove = &baron_fire_move_run;
}

void baron_fire_nextmove(edict_t* self)
{
	if (G_EntExists(self->enemy))
		baron_fire_run(self);
	else
		baron_fire_walk(self);
}

mframe_t baron_fire_frames_pain[] =
{
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL
};
mmove_t baron_fire_move_pain = { FRAME_pain101, FRAME_pain106, baron_fire_frames_pain, baron_fire_nextmove };

void baron_fire_pain(edict_t* self, edict_t* other, float kick, int damage)
{
	double rng = random();
	//if (self->health < (self->max_health / 2))
	//	self->s.skinnum = 1;

	if (self->health < 0.2 * self->max_health)
		self->s.skinnum = 2;
	else if (self->health < 0.5 * self->max_health)
		self->s.skinnum = 1;

	// we're already in a pain state
	if (self->monsterinfo.currentmove == &baron_fire_move_pain)
		return;

	// monster players don't get pain state induced
	if (G_GetClient(self))
		return;

	// no pain in invasion hard mode
	if (invasion->value == 2)
		return;

	// if we're fidgeting, always go into pain state.
	if (rng <= (1.0f - self->monsterinfo.pain_chance) &&
		self->monsterinfo.currentmove != &baron_fire_move_stand &&
		self->monsterinfo.currentmove != &baron_fire_move_walk)
		return;

	if (random() > 0.5)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_IDLE, 0);

	//gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &baron_fire_move_pain;
}

void baron_fire_check_landing(edict_t* self)
{
	//gi.dprintf("%d: baron_fire_check_landing %d\n", (int)(level.framenum), self->s.frame);

	// hold this frame unless we're on the ground or jump has timed out
	if (self->s.frame == FRAME_jump05 && !self->groundentity && self->monsterinfo.pausetime > level.time)
	{
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
		return;
	}
	
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
}

void baron_fire_jump(edict_t* self)
{
	int	speed = 1000;
	vec3_t	forward, start;

	//gi.dprintf("%d: baron_fire_jump %d\n", (int)(level.framenum), self->s.frame);
	//gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);

	self->lastsound = level.framenum;

	self->groundentity = NULL;

	MonsterAim(self, -1, speed, false, 0, forward, start);
	self->velocity[0] += forward[0] * speed;
	self->velocity[1] += forward[1] * speed;
	self->velocity[2] += forward[2] * speed;

	//self->touch = mutant_jump_touch;
	self->monsterinfo.pausetime = level.time + 2; // maximum duration of jump
	self->velocity[2] += 300;

	//self->s.frame = FRAME_attack05; // skip to airborne jump frame
	//self->monsterinfo.aiflags |= AI_HOLD_FRAME; // hold this frame
}

mframe_t baron_fire_frames_jump[] =
{
	ai_charge, 0, baron_fire_jump,//43
	ai_charge, 0, baron_fire_check_landing,
	ai_charge, 0, baron_fire_check_landing,
	ai_charge, 0, baron_fire_check_landing,//46
	ai_charge, 0, baron_fire_check_landing,//47
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL//58
};
mmove_t baron_fire_move_jump = { FRAME_jump01, FRAME_jump16, baron_fire_frames_jump, baron_fire_run };

void baron_fire_meteor(edict_t* self)
{
	int damage, radius, speed;
	float slvl = drone_damagelevel(self);

	if (!G_EntExists(self->enemy))
		return;

	damage = METEOR_INITIAL_DMG + METEOR_ADDON_DMG * slvl;
	radius = METEOR_INITIAL_RADIUS + METEOR_ADDON_RADIUS * slvl;
	speed = METEOR_INITIAL_SPEED + METEOR_ADDON_SPEED * slvl;

	// miss the attack if we are cursed/confused
	if (que_typeexists(self->curses, CURSE) && rand() > 0.2)
		fire_meteor(self, self->s.origin, damage, radius, speed); // meteor drops on his head!
	else
		fire_meteor(self, self->enemy->s.origin, damage, radius, speed);

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TELEPORT_EFFECT);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);
}

mframe_t baron_fire_frames_crush[] =
{
	ai_charge, 0, baron_fire_meteor,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,//97
	ai_charge, 0, NULL,//98
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL //107
};
mmove_t baron_fire_move_crush = { FRAME_crush01, FRAME_crush23, baron_fire_frames_crush, baron_fire_run };

void flame_think(edict_t* self)
{
	if (self->style != 1 && self->s.frame > 20)
		G_RunFrames(self, 21, 32, false); // burning frames
	else
		self->s.frame++; // ignite frames

	if (!G_EntIsAlive(self->owner) || (self->delay < level.time) || self->waterlevel || self->s.frame == 39)
	{
		G_FreeEdict(self);
		return;
	}
	self->nextthink = level.time + FRAMETIME;
}

void flame_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	//int damage;
	//float slvl;

	if (other == self)
		return;

	if (G_EntExists(other) && !OnSameTeam(self, other))
	{
		/*
		slvl = drone_damagelevel(self->owner);
		// 100% of health over 5 seconds at level 10
		damage = other->max_health * (0.1 + 0.01 * slvl);
		if (damage < 20)
			damage = 20;
		if (damage > 1000)
			damage = 1000;
		*/
		// set them on fire
		burn_person(other, self->owner, sf2qf(self->dmg_radius));

		// hurt them
		if (level.framenum >= self->monsterinfo.nextattack)
		{
			T_Damage(other, self, self->owner, vec3_origin, self->s.origin,
				plane->normal, self->dmg, 0, DAMAGE_NO_KNOCKBACK, MOD_BURN);
			self->monsterinfo.nextattack = level.framenum + 1;
		}
		// actively hurting something--don't free us yet!
		return;
	}
	if (!self->style)
	{
		self->style = 1;//mark for removal
		self->s.frame = 33;//begin flare frames
	}
}

void circle_of_flames(edict_t* self, int count, int damage, int burn_damage, float speed)
{
	int			i;
	float		deg;
	edict_t*	flame;
	vec3_t		org, start, forward, angles;

	deg = 360 / count;

	VectorCopy(self->s.angles, angles);
	VectorCopy(self->s.origin, org);
	org[2] = self->absmin[2] + 1;

	//gi.dprintf("count %d damage %d speed %.0f deg %.1f\n", count, damage, speed, deg);

	for (i = 0; i < count; i++)
	{
		angles[YAW] = i * deg;
		//gi.dprintf("firing flame at %.1f degrees\n", angles[YAW]);
		AngleVectors(angles, forward, NULL, NULL);
		VectorMA(org, self->maxs[1], forward, start);
		// spawn flame entity
		flame = G_Spawn();
		VectorCopy(start, flame->s.origin);
		VectorCopy(start, flame->s.old_origin);
		flame->movetype = MOVETYPE_FLYMISSILE;//MOVETYPE_TOSS;//MOVETYPE_FLYMISSILE;
		flame->clipmask = MASK_SHOT;
		flame->solid = SOLID_TRIGGER;
		flame->s.modelindex = gi.modelindex("models/fire/tris.md2");
		flame->s.sound = gi.soundindex("weapons/bfg__l1a.wav");
		flame->owner = self;
		flame->touch = flame_touch;
		flame->think = flame_think;
		flame->dmg = damage;
		flame->dmg_radius = burn_damage;
		flame->s.frame = 15;//21-32
		flame->classname = "flame";
		flame->delay = level.time + 10.0;
		gi.linkentity(flame);
		flame->nextthink = level.time + FRAMETIME;
		VectorScale(forward, speed, flame->velocity);
	}
}

void fire_baron_cof_attack(edict_t* self)
{
	int damage, burn;
	float slvl = drone_damagelevel(self);

	damage = METEOR_INITIAL_DMG + METEOR_ADDON_DMG * slvl;
	burn = 0.1 * damage;

	circle_of_flames(self, 16, damage, burn, 350);
}

mframe_t baron_fire_frames_swipe[] =
{
	ai_charge, 0, fire_baron_cof_attack,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t baron_fire_move_swipe = { FRAME_swipe01, FRAME_swipe15, baron_fire_frames_swipe, baron_fire_run };

void fire_fireball(edict_t* self, vec3_t start, vec3_t aimdir, int damage, float damage_radius, int speed, int flames, int flame_damage);

void baron_fire_fireball(edict_t* self)
{
	int damage, radius, speed, flames, flame_damage;
	vec3_t	forward, start;

	if (!G_EntExists(self->enemy))
		return;

	float slvl = drone_damagelevel(self);

	damage = FIREBALL_INITIAL_DAMAGE + FIREBALL_ADDON_DAMAGE * slvl;
	radius = FIREBALL_INITIAL_RADIUS + FIREBALL_ADDON_RADIUS * slvl;
	flame_damage = FIREBALL_ADDON_FLAMEDMG * slvl;
	speed = FIREBALL_INITIAL_SPEED + FIREBALL_ADDON_SPEED * slvl;
	flames = FIREBALL_INITIAL_FLAMES + FIREBALL_ADDON_FLAMES * slvl;

	MonsterAim(self, M_PROJECTILE_ACC, speed, true, 0, forward, start);
	fire_fireball(self, start, forward, damage, radius, speed, flames, flame_damage);

	gi.sound(self, CHAN_ITEM, gi.soundindex("abilities/firecast.wav"), 1, ATTN_NORM, 0);
}

mframe_t baron_fire_frames_backhand[] =
{
	ai_charge, 0, NULL,//74
	ai_charge, 0, NULL,
	ai_charge, 0, baron_fire_fireball,
	ai_charge, 0, baron_fire_fireball,
	ai_charge, 0, baron_fire_fireball,//78
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t baron_fire_move_backhand = { FRAME_backhand01, FRAME_backhand11, baron_fire_frames_backhand, baron_fire_run };

void baron_fire_attack_sound(edict_t* self)
{
	// prevent attack sound from conflicting with sight sound
	if (self->haste_time > level.time)
		return;

	if (random() > 0.5)
		gi.sound(self, CHAN_VOICE, sound_melee1, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_melee2, 1, ATTN_IDLE, 0);
}

void baron_fire_stand_attack(edict_t* self)
{
	float	zDelta = fabs(self->absmin[2] - self->enemy->absmin[2]);
	float	dist = entdist(self, self->enemy);
	float	r = random();

	if (dist <= 512)
	{
		if (r <= 0.8 && zDelta <= 8)
			self->monsterinfo.currentmove = &baron_fire_move_swipe;//circle of fire attack
		else
			self->monsterinfo.currentmove = &baron_fire_move_backhand;//fireball attack
	}
	else
		self->monsterinfo.currentmove = &baron_fire_move_crush;//meteor attack
	self->monsterinfo.attack_finished = level.time + 2.0;

}

void baron_fire_move_attack(edict_t* self)
{
	float	zDelta = fabs(self->absmin[2] - self->enemy->absmin[2]);
	float	dist = entdist(self, self->enemy);
	float	r = random();

	if (dist <= 256)
	{
		if (r <= 0.8 && zDelta <= 8)
			self->monsterinfo.currentmove = &baron_fire_move_swipe;//circle of fire attack
		else
			self->monsterinfo.currentmove = &baron_fire_move_backhand;//fireball attack
	}
	if (dist <= 512)
	{
		if (r <= 0.8 && zDelta <= 8)
			self->monsterinfo.currentmove = &baron_fire_move_swipe;//circle of fire attack
		else if (random() <= 0.6)
		{
			self->monsterinfo.currentmove = &baron_fire_move_jump;//jump attack
			return;
		}
		else
			self->monsterinfo.currentmove = &baron_fire_move_backhand;//fireball attack
	}
	else
		self->monsterinfo.currentmove = &baron_fire_move_crush;//meteor attack
	//self->monsterinfo.attack_finished = level.time + GetRandom(2, 4);
	M_DelayNextAttack(self, (float)GetRandom(0,2), true);
}

void baron_fire_attack(edict_t* self)
{
	baron_fire_attack_sound(self);

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		baron_fire_stand_attack(self);
	else
		baron_fire_move_attack(self);
}

void baron_fire_sight(edict_t* self, edict_t* other)
{
	if (random() > 0.5)
		gi.sound(self, CHAN_VOICE, sound_sight1, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_sight2, 1, ATTN_IDLE, 0);
	self->haste_time = level.time + 1.0;// timer to delay attack sounds
	//baron_fire_attack(self);
}

void baron_fire_dead(edict_t* self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	//self->nextthink = 0;
	gi.linkentity(self);
	M_PrepBodyRemoval(self);
}

mframe_t baron_fire_frames_death[] =
{
	ai_move, 0,	 NULL,//FRAME_death01
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL //FRAME_death25
};
mmove_t baron_fire_move_death = { FRAME_death01, FRAME_death25, baron_fire_frames_death, baron_fire_dead };

void baron_fire_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
	int		n;

	M_Notify(self);

#ifdef OLD_NOLAG_STYLE
	// reduce lag by removing the entity right away
	if (nolag->value)
	{
		M_Remove(self, false, true);
		return;
	}
#endif

	// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n = 0; n < 2; n++)
			ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n = 0; n < 4; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		//ThrowHead (self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		//self->deadflag = DEAD_DEAD;
#ifdef OLD_NOLAG_STYLE
		M_Remove(self, false, false);
#else
		if (nolag->value)
			M_Remove(self, false, true);
		else
			M_Remove(self, false, false);
#endif
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	//DroneList_Remove(self);

	// regular death
	if (random() > 0.5)
		gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_death2, 1, ATTN_IDLE, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &baron_fire_move_death;
	DroneList_Remove(self);
}

void baron_fire_melee(edict_t* self)
{

}

void init_baron_fire(edict_t* self)
{
	
	sound_death1 = gi.soundindex("baron/death.wav");
	sound_death2 = gi.soundindex("baron/death2.wav");
	sound_pain1 = gi.soundindex("baron/pain1.wav");
	sound_pain2 = gi.soundindex("baron/pain2.wav");
	sound_idle1 = gi.soundindex("baron/idle1.wav");
	sound_idle2 = gi.soundindex("baron/idle2.wav");
	sound_sight1 = gi.soundindex("baron/sight1.wav");
	sound_sight2 = gi.soundindex("baron/sight1.wav");
	sound_melee1 = gi.soundindex("baron/melee1.wav");
	sound_melee2 = gi.soundindex("baron/melee2.wav");
	sound_step1 = gi.soundindex("baron/step.wav");
	sound_step2 = gi.soundindex("baron/stepdistant.wav");
	
	self->monsterinfo.control_cost = 4;
	self->monsterinfo.cost = M_DEFAULT_COST;
	self->s.modelindex = gi.modelindex("models/monsters/baron_fire/tris.md2");
	VectorSet(self->mins, -40, -40, -24);
	VectorSet(self->maxs, 40, 40, 82);

	//if (self->activator && self->activator->client)
	self->health = M_BARON_FIRE_INITIAL_HEALTH + M_BARON_FIRE_ADDON_HEALTH * self->monsterinfo.level;
	//else self->health = 100 + 30*self->monsterinfo.level;

	self->max_health = self->health;
	self->gib_health = -BASE_GIB_HEALTH;
	self->mass = 200;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.jumpup = 64;

	self->die = baron_fire_die;

	self->monsterinfo.melee = baron_fire_melee;
	self->monsterinfo.idle = baron_fire_idle;
	self->monsterinfo.sight = baron_fire_sight;
	self->monsterinfo.stand = baron_fire_stand;
	self->monsterinfo.run = baron_fire_run;
	//self->monsterinfo.dodge = baron_fire_dodge;
	self->monsterinfo.attack = baron_fire_attack;
	self->monsterinfo.walk = baron_fire_walk;
	self->monsterinfo.pain_chance = 0.1f;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;


	self->pain = baron_fire_pain;

	//K03 Begin
	//self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;

	//if (self->activator && self->activator->client)
	self->monsterinfo.power_armor_power = M_BARON_FIRE_INITIAL_ARMOR + M_BARON_FIRE_ADDON_ARMOR * self->monsterinfo.level;
	//else self->monsterinfo.power_armor_power = 100 + 50*self->monsterinfo.level;

	//self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
	self->mtype = M_BARON_FIRE;
	//K03 End

	gi.linkentity(self);

	self->monsterinfo.currentmove = &baron_fire_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

	//	walkmonster_start (self);
	self->nextthink = level.time + FRAMETIME;
}
