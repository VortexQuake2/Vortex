/*
==============================================================================

GOLEM

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/golem.h"

#define GOLEM_BACKHAND_RANGE		512 // range for backhand attack (throw rocks)
#define GOLEM_AOE_ATTACK_RANGE		200 // range for punch/avalanche/stun attacks
#define GOLEM_MULTISTRIKE_FOV		45 // arc degrees for multistrike attack

static int sound_death;
static int sound_pain1;
static int sound_pain2;

static int sound_sight;
static int sound_melee1;
static int sound_melee2;

static int sound_idle1;
static int sound_idle2;
static int sound_idle3;
static int sound_idle4;
static int sound_step1;
static int sound_step2;
static int sound_step3;
static int sound_step4;

static int sound_stone_hit1;
static int sound_stone_hit2;
static int sound_stone_hit3;
static int sound_stone_hit4;
static int sound_stone_hit5;
static int sound_stone_hit6;
static int sound_stone_hit7;
static int sound_stone_hit8;
static int sound_stone_hit9;

static int sound_swing1;
static int sound_swing2;

static int sound_stun1;
static int sound_stun2;
static int sound_stun3;

static int sound_rock_storm;

static int sound_spawn;

static int sound_deepthud;

void golem_stone_hit(edict_t* self)
{
	switch (GetRandom(1, 9))
	{
	case 1: gi.sound(self, CHAN_WEAPON, sound_stone_hit1, 1, ATTN_NORM, 0); break;
	case 2: gi.sound(self, CHAN_WEAPON, sound_stone_hit2, 1, ATTN_NORM, 0); break;
	case 3: gi.sound(self, CHAN_WEAPON, sound_stone_hit3, 1, ATTN_NORM, 0); break;
	case 4: gi.sound(self, CHAN_WEAPON, sound_stone_hit4, 1, ATTN_NORM, 0); break;
	case 5: gi.sound(self, CHAN_WEAPON, sound_stone_hit5, 1, ATTN_NORM, 0); break;
	case 6: gi.sound(self, CHAN_WEAPON, sound_stone_hit6, 1, ATTN_NORM, 0); break;
	case 7: gi.sound(self, CHAN_WEAPON, sound_stone_hit7, 1, ATTN_NORM, 0); break;
	case 8: gi.sound(self, CHAN_WEAPON, sound_stone_hit8, 1, ATTN_NORM, 0); break;
	case 9: gi.sound(self, CHAN_WEAPON, sound_stone_hit9, 1, ATTN_NORM, 0); break;
	}
}

void golem_idle(edict_t* self)
{
	switch (GetRandom(1, 4))
	{
	case 1: gi.sound(self, CHAN_VOICE, sound_idle1, 1, ATTN_NORM, 0); break;
	case 2: gi.sound(self, CHAN_VOICE, sound_idle2, 1, ATTN_NORM, 0); break;
	case 3: gi.sound(self, CHAN_VOICE, sound_idle3, 1, ATTN_NORM, 0); break;
	case 4: gi.sound(self, CHAN_VOICE, sound_idle4, 1, ATTN_NORM, 0); break;
	}
}

void golem_stun(edict_t* self)
{
	switch (GetRandom(1, 3))
	{
	case 1: gi.sound(self, CHAN_WEAPON, sound_stun1, 1, ATTN_NORM, 0); break;
	case 2: gi.sound(self, CHAN_WEAPON, sound_stun2, 1, ATTN_NORM, 0); break;
	case 3: gi.sound(self, CHAN_WEAPON, sound_stun3, 1, ATTN_NORM, 0); break;
	}
}

mframe_t golem_frames_stand[] =
{
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
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL
};
mmove_t	golem_move_stand = { FRAME_stand01, FRAME_stand22, golem_frames_stand, NULL };

void golem_stand(edict_t* self)
{
	self->monsterinfo.currentmove = &golem_move_stand;
}

void golem_step(edict_t* self)
{
	switch (GetRandom(1, 4))
	{
	case 1: gi.sound(self, CHAN_BODY, sound_step1, 1, ATTN_NORM, 0); break;
	case 2: gi.sound(self, CHAN_BODY, sound_step2, 1, ATTN_NORM, 0); break;
	case 3: gi.sound(self, CHAN_BODY, sound_step3, 1, ATTN_NORM, 0); break;
	case 4: gi.sound(self, CHAN_BODY, sound_step4, 1, ATTN_NORM, 0); break;
	}
}

mframe_t golem_frames_walk[] =
{
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, golem_step,//touchdown
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, golem_step//touchdown
};
mmove_t golem_move_walk = { FRAME_walk01, FRAME_walk34, golem_frames_walk, NULL };

void golem_walk(edict_t* self)
{
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &golem_move_walk;
}

mframe_t golem_frames_run[] =
{
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL, //87 move start
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL, // 95 move stop
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, golem_step,// 97 touchdown
	drone_ai_run, 15, NULL, // 98 move start
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL, // 107 move stop
	drone_ai_run, 15, golem_step// 108 touchdown
};

mmove_t golem_move_run = { FRAME_run01, FRAME_run24, golem_frames_run, NULL };

void golem_run(edict_t* self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &golem_move_stand;
	else
		self->monsterinfo.currentmove = &golem_move_run;
}

void golem_heal(edict_t* self)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
	self->health_cache += (int)(self->monsterinfo.level * 0.10 * self->max_health) + 1;
	self->monsterinfo.regen_delay1 = level.framenum + (int)(1 / FRAMETIME);
}

mframe_t golem_frames_flex[] =
{
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
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, golem_heal,//peak
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t	golem_move_flex = { FRAME_flex01, FRAME_flex24, golem_frames_flex, golem_run };

void golem_flex(edict_t* self)
{
	self->monsterinfo.currentmove = &golem_move_flex;
}

void golem_pain_nextmove(edict_t* self)
{
	if (G_EntExists(self->enemy))
		golem_run(self);
	else
		golem_walk(self);
}

mframe_t golem_frames_pain[] =
{
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL
};
mmove_t golem_move_pain = { FRAME_pain101, FRAME_pain116, golem_frames_pain, golem_pain_nextmove };

void golem_pain(edict_t* self, edict_t* other, float kick, int damage)
{
	double rng = random();
	//if (self->health < (self->max_health / 2))
	//	self->s.skinnum = 1;

	//if (self->health < 0.2 * self->max_health)
	//	self->s.skinnum = 2;
	//else if (self->health < 0.5 * self->max_health)
	//	self->s.skinnum = 1;

	// we're already in a pain state
	if (self->monsterinfo.currentmove == &golem_move_pain)
		return;

	// monster players don't get pain state induced
	if (G_GetClient(self))
		return;

	// no pain in invasion hard mode
	if (invasion->value == 2)
		return;

	// if we're fidgeting, always go into pain state.
	if (rng <= (1.0f - self->monsterinfo.pain_chance) &&
		self->monsterinfo.currentmove != &golem_move_stand &&
		self->monsterinfo.currentmove != &golem_move_walk)
		return;

	if (random() > 0.5)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_IDLE, 0);

	//gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &golem_move_pain;
}

void fire_rocks(edict_t* self, vec3_t start, vec3_t aimdir, int damage, float speed);

// drops rocks on nearby enemies
void golem_avalanche(edict_t* self)
{
	int damage = 50 * self->monsterinfo.level;
	float speed = 650;
	vec3_t start, end;
	edict_t* e = NULL;
	trace_t tr;

	while ((e = findradius(e, self->s.origin, GOLEM_AOE_ATTACK_RANGE)) != NULL)
	{
		if (!G_ValidTarget(self, e, true, true))
			continue;

		// calculate ending position at ceiling height
		VectorCopy(e->s.origin, start);
		VectorCopy(start, end);
		end[2] += METEOR_CEILING_HEIGHT;

		tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SOLID);

		// abort if we get stuck or we don't have enough room
		if (tr.startsolid || fabs(start[2] - end[2]) < 64)
			return;
		fire_rocks(self, tr.endpos, tv(0, 0, -1), damage, speed);

	}

	gi.sound(self, CHAN_VOICE, sound_rock_storm, 1, ATTN_NORM, 0);
}

// stuns nearby enemies
void golem_stunattack(edict_t* self)
{
	qboolean found_target = false;
	float stuntime = EMP_INITIAL_TIME + EMP_ADDON_TIME * self->monsterinfo.level;
	edict_t* e = NULL;

	while ((e = findradius(e, self->s.origin, GOLEM_AOE_ATTACK_RANGE)) != NULL)
	{
		if (!G_ValidTarget(self, e, true, true))
			continue;
		vrx_stun(self, e, stuntime);
		found_target = true;
		
	}

	if (found_target)
		golem_stun(e);
	gi.sound(self, CHAN_VOICE, sound_deepthud, 1, ATTN_NORM, 0);
}

// targets and strikes at multiple nearby enemies in a relatively narrow arc in front of the golem
void golem_multistrike(edict_t* self)
{
	edict_t *e = NULL, *hit;
	qboolean hit_something = false;

	int damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self);
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;
	damage *= 3;// because it's the equivalent of a boss!

	while ((e = findradius(e, self->s.origin, GOLEM_AOE_ATTACK_RANGE)) != NULL)
	{
		if (!G_ValidTarget(self, e, true, false))
			continue;
		if (!infov(self, e, GOLEM_MULTISTRIKE_FOV))
			continue;
		if (M_MeleeAttack(self, e, GOLEM_AOE_ATTACK_RANGE, damage, 100))
			hit_something = true;
	}

	if (hit_something)
	{
		// play a sound
		golem_stone_hit(self);
	}
	else
	{
		// play weapon swing sound
		if (random() > 0.5)
			gi.sound(self, CHAN_WEAPON, sound_swing1, 1, ATTN_NORM, 0);
		else
			gi.sound(self, CHAN_WEAPON, sound_swing2, 1, ATTN_NORM, 0);
	}

}

void golem_restrike(edict_t* self)
{
	if (G_EntIsAlive(self->enemy) && infov(self, self->enemy, GOLEM_MULTISTRIKE_FOV) && entdist(self, self->enemy) < GOLEM_AOE_ATTACK_RANGE)
		self->s.frame = FRAME_punch10;
}

void golem_delay(edict_t* self)
{
	self->monsterinfo.attack_finished = level.time + random();
}

mframe_t golem_frames_punch[] =
{
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 0, NULL,//116
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, golem_step,//119 touchdown
	ai_charge, 0, golem_multistrike,//120 hit
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,//122 retraction
	ai_charge, 0, NULL,
	ai_charge, 0, golem_restrike,
	ai_charge, 15, NULL,//125
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, golem_delay,
	ai_charge, 15, golem_step
};
mmove_t golem_move_punch = { FRAME_punch01, FRAME_punch25, golem_frames_punch, golem_run };

void golem_rebash(edict_t* self)
{
	if (G_EntIsAlive(self->enemy) && entdist(self, self->enemy) < GOLEM_AOE_ATTACK_RANGE && random() <= 0.9)
		self->s.frame = FRAME_bash09;
}

mframe_t golem_frames_bash[] =
{
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, golem_step,//141 peak
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, golem_avalanche,//145 hit
	ai_charge, 0, golem_rebash,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, golem_delay
};
mmove_t golem_move_bash = { FRAME_bash01, FRAME_bash23, golem_frames_bash, golem_run };

mframe_t golem_frames_stomp[] =
{
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
	ai_charge, 0, golem_stunattack,//hit
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
	ai_charge, 0, golem_delay
};
mmove_t golem_move_stomp = { FRAME_stomp01, FRAME_stomp25, golem_frames_stomp, golem_run };

void golem_rebackhand(edict_t* self)
{
	if (G_EntIsAlive(self->enemy) && visible(self, self->enemy) && entdist(self, self->enemy) < GOLEM_BACKHAND_RANGE && random() <= 0.9)
		self->s.frame = FRAME_backhand09;
}

mframe_t golem_frames_backhand[] =
{
	ai_charge, 15, NULL,//228
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, golem_step,//235
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, monster_fire_rocks,//242 hit
	ai_charge, 0, monster_fire_rocks,
	ai_charge, 0, monster_fire_rocks,
	ai_charge, 0, golem_rebackhand,//245
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, NULL,
	ai_charge, 15, golem_delay
};
mmove_t golem_move_backhand = { FRAME_backhand01, FRAME_backhand23, golem_frames_backhand, golem_run };

void golem_backhand_attack(edict_t* self)
{
	self->monsterinfo.currentmove = &golem_move_backhand;//projectile attack
	//self->monsterinfo.attack_finished = level.time + 2.0;
	M_DelayNextAttack(self, random(), true);
}

void golem_attack_sound(edict_t* self)
{
	// prevent attack sound from conflicting with sight sound
	if (self->haste_time > level.time)
		return;

	if (random() > 0.5)
		gi.sound(self, CHAN_VOICE, sound_melee1, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_melee2, 1, ATTN_IDLE, 0);
}

// returns the number of enemies within radius of the golem
// used to aid attack method selection
void golem_nearbyenemies(edict_t* self, float radius, int *num, int *in_front, int *stunned)
{
	edict_t* e = NULL;

	while ((e = findradius(e, self->s.origin, radius)) != NULL)
	{
		if (!G_ValidTarget(self, e, true, true))
			continue;
		*num = *num + 1;
		if (infov(self, e, GOLEM_MULTISTRIKE_FOV))
			*in_front = *in_front + 1;
		if (e->empeffect_time > level.time)
			*stunned = *stunned + 1;
	}

	//gi.dprintf("%s: r: %.0f num: %d in: %d st: %d\n", __func__, radius, *num, *in_front, *stunned);
}
// choose attack:
// 1. backhand - medium-range attack fires 3 rock projectiles in rapid succession
// 2. punch - short-range attack causing massive damage to enemies in 45 degree arc in front of the golem
// 3. stomp - short-range attack that stuns enemies around the golem
// 4. bash - short-range attack that drops rock projectiles over enemies around the golem
void golem_attack(edict_t* self)
{
	int nearby, in_front, stunned;
	float range = entdist(self, self->enemy);

	// Talent: Golem Mastery - allows golem to self-heal when his health is low
	int talentLevel = vrx_get_talent_level(self->activator, TALENT_GOLEM_MASTERY);
	float healthFrac = (float)self->health / self->max_health;
	if (talentLevel > 1 && healthFrac < 0.5 && level.framenum > self->monsterinfo.regen_delay1)
	{
		//gi.dprintf("%s: healthFrac %.1f\n", __func__, healthFrac);
		float chance = (1 - healthFrac) + 0.1;
		if (chance > random())
			golem_flex(self);
		return;
	}

	nearby = in_front = stunned = 0; // initialize
	golem_nearbyenemies(self, GOLEM_AOE_ATTACK_RANGE, &nearby, &in_front, &stunned);
	//gi.dprintf("%s: r: %.0f num: %d in: %d st: %d\n", __func__, (float)GOLEM_AOE_ATTACK_RANGE, nearby, in_front, stunned);

	// no enemies within short range
	if (nearby < 1)
	{
		// golem has only one ranged attack--use it if enemy is within range
		if (range <= GOLEM_BACKHAND_RANGE)
		{
			golem_backhand_attack(self);
			golem_attack_sound(self);
		}
		return;
	}

	float r = random();

	// calculate number of enemies in the periphery
	int peripheral = nearby - in_front;

	// more enemies in front of the golem
	if (in_front > peripheral)
	{
		if (stunned < 1 && r > 0.5)
			self->monsterinfo.currentmove = &golem_move_stomp; // stun attack
		else
			self->monsterinfo.currentmove = &golem_move_punch;
	}
	else // more enemies in the periphery (sides and back)
	{
		if (stunned < 1 && r > 0.5) // no stunned enemies?
			self->monsterinfo.currentmove = &golem_move_stomp; // stun attack
		else
			self->monsterinfo.currentmove = &golem_move_bash; // avalanche attack
	}

	golem_attack_sound(self);
	//M_DelayNextAttack(self, random(), true);
}

void golem_sight(edict_t* self, edict_t* other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
	self->haste_time = level.time + 1.0;// timer to delay attack sounds
}

void golem_dead(edict_t* self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	//self->nextthink = 0;
	gi.linkentity(self);
	M_PrepBodyRemoval(self);
}

mframe_t golem_frames_death[] =
{
	ai_move, 0,	 NULL,
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
	ai_move, 0,	 NULL
};
mmove_t golem_move_death = { FRAME_death01, FRAME_death22, golem_frames_death, golem_dead };

void golem_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
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
		if (vrx_spawn_nonessential_ent(self->s.origin))
		{
			//FIXME: throw rocks instead?
			for (n = 0; n < 2; n++)
				ThrowGib(self, "models/proj/proj_golemrock1/tris.md2", damage, GIB_ORGANIC);
			for (n = 0; n < 4; n++)
				ThrowGib(self, "models/objects/debris2/tris.md2", damage, GIB_METALLIC);
			//ThrowHead (self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		}
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

	DroneList_Remove(self);

	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &golem_move_death;
	self->s.skinnum = 1;// lights out!
}

void golem_melee(edict_t* self)
{

}

void golem_set_bbox(vec3_t mins, vec3_t maxs)
{
	VectorSet(mins, -38, -38, -24);
	VectorSet(maxs, 38, 38, 82);
}

typedef void (*thinkfunc)(edict_t* self);
void aura_create(edict_t* ent, int aura_type, int aura_level, float duration, float radius, thinkfunc aura_think);
void thorns_think(edict_t* self);

void init_golem(edict_t* self)
{
	sound_pain1 = gi.soundindex("golem/pain1.wav");
	sound_pain2 = gi.soundindex("golem/pain2.wav");
	sound_melee1 = gi.soundindex("golem/attack3.wav");
	sound_melee2 = gi.soundindex("golem/attack4.wav");
	sound_sight = gi.soundindex("golem/sight.wav");
	sound_death = gi.soundindex("golem/death.wav");
	sound_idle1 = gi.soundindex("golem/idle1a.wav");
	sound_idle2 = gi.soundindex("golem/idle1b.wav");
	sound_idle1 = gi.soundindex("golem/idle2a.wav");
	sound_idle2 = gi.soundindex("golem/idle2b.wav");
	sound_step1 = gi.soundindex("golem/claywalk1.wav");
	sound_step2 = gi.soundindex("golem/claywalk2.wav");
	sound_step3 = gi.soundindex("golem/claywalk3.wav");
	sound_step4 = gi.soundindex("golem/claywalk4.wav");
	sound_stone_hit1 = gi.soundindex("golem/stonehit1.wav");
	sound_stone_hit2 = gi.soundindex("golem/stonehit2.wav");
	sound_stone_hit3 = gi.soundindex("golem/stonehit3.wav");
	sound_stone_hit4 = gi.soundindex("golem/stonehit4.wav");
	sound_stone_hit5 = gi.soundindex("golem/stonehit5.wav");
	sound_stone_hit6 = gi.soundindex("golem/stonehit6.wav");
	sound_stone_hit7 = gi.soundindex("golem/stonehit7.wav");
	sound_stone_hit8 = gi.soundindex("golem/stonehit8.wav");
	sound_stone_hit9 = gi.soundindex("golem/stonehit9.wav");
	sound_swing1 = gi.soundindex("golem/staff blunt03.wav");
	sound_swing2 = gi.soundindex("golem/staff blunt04.wav");
	sound_stun1 = gi.soundindex("golem/stun1.wav");
	sound_stun2 = gi.soundindex("golem/stun2.wav");
	sound_stun3 = gi.soundindex("golem/stun3.wav");
	sound_rock_storm = gi.soundindex("golem/rockstorm.wav");
	sound_spawn = gi.soundindex("golem/spawnclay.wav");
	sound_deepthud = gi.soundindex("golem/deepthud.wav");

	//self->monsterinfo.level = 10;//TESTING ONLY!!!!!!!!
	self->monsterinfo.control_cost = 3;
	self->monsterinfo.cost = GOLEM_COST;
	self->s.modelindex = gi.modelindex("models/monsters/golem/tris.md2");
	//VectorSet(self->mins, -38, -38, -24);
	//VectorSet(self->maxs, 38, 38, 82);
	golem_set_bbox(self->mins, self->maxs);
	//if (self->activator && self->activator->client)
	self->health = GOLEM_INITIAL_HEALTH + GOLEM_ADDON_HEALTH * self->monsterinfo.level;
	//else self->health = 100 + 30*self->monsterinfo.level;

	self->max_health = self->health;
	self->gib_health = -GOLEM_ADDON_HEALTH;
	self->mass = 500;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.jumpup = 64;

	self->die = golem_die;

	self->monsterinfo.melee = golem_melee;
	self->monsterinfo.idle = golem_idle;
	self->monsterinfo.sight = golem_sight;
	self->monsterinfo.stand = golem_stand;
	self->monsterinfo.run = golem_run;
	//self->monsterinfo.dodge = golem_dodge;
	self->monsterinfo.attack = golem_attack;
	self->monsterinfo.walk = golem_walk;
	self->pain = golem_pain;
	self->monsterinfo.pain_chance = 0.1f;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;
	self->flags |= FL_NO_KNOCKBACK; // no knocking around a stone giant!


	self->pain = golem_pain;

	//K03 Begin
	//self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;

	//if (self->activator && self->activator->client)
	self->monsterinfo.power_armor_power = M_BARON_FIRE_INITIAL_ARMOR + M_BARON_FIRE_ADDON_ARMOR * self->monsterinfo.level;
	//else self->monsterinfo.power_armor_power = 100 + 50*self->monsterinfo.level;

	//self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
	self->mtype = M_GOLEM;
	//K03 End

	gi.linkentity(self);

	self->monsterinfo.currentmove = &golem_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

	//	walkmonster_start (self);
	self->nextthink = level.time + FRAMETIME;

	// Talent: Golem Mastery - allows golem to activate thorns aura
	int talentLevel = vrx_get_talent_level(self->activator, TALENT_GOLEM_MASTERY);
	if (talentLevel > 0)
		aura_create(self, AURA_THORNS, (2*talentLevel), DEFAULT_AURA_DURATION, 256, thorns_think);
}

// create and initialize the golem entity
qboolean create_golem(edict_t* ent, vec3_t start, int skill_level)
{
	edict_t* e = G_Spawn();
	e->activator = ent;
	e->monsterinfo.level = skill_level;
	e->mtype = M_GOLEM;
	//e->s.skinnum = type;

	if (!M_Initialize(ent, e, 0) || !G_IsValidLocation(NULL, start, e->mins, e->maxs))
	{
		DroneList_Remove(e);
		layout_remove_tracked_entity(&ent->client->layout, e);
		AI_EnemyRemoved(e);
		G_FreeEdict(e);
		return false;
	}

	e->monsterinfo.currentmove = &golem_move_stand;
	VectorCopy(start, e->s.origin);
	gi.linkentity(e);
	e->nextthink = level.time + FRAMETIME;
	e->monsterinfo.attack_finished = level.time + 1.0;// don't attack immediately
	gi.sound(e, CHAN_VOICE, sound_spawn, 1, ATTN_NORM, 0); // play a sound

	ent->num_golems++;
	safe_cprintf(ent, PRINT_HIGH, "Raised a %s. (%d/%d)\n", V_GetMonsterName(e), ent->num_golems, (int)GOLEM_MAX);
	return true;
}

// calculate spawn location for golem and then try to create it
void spawn_golem(edict_t* ent)
{
	int slvl = ent->myskills.abilities[GOLEM].current_level;
	vec3_t forward, right, start, offset, mins, maxs;

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TELEPORT_EFFECT);
	gi.WritePosition(ent->s.origin);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->idle_frames = 0; // disable cp/cloak on caster

	// get view origin
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	golem_set_bbox(mins, maxs);

	if (!G_GetSpawnLocation(ent, 64, mins, maxs, start, NULL, PROJECT_HITBOX_FAR, false))
		return;

	if (create_golem(ent, start, slvl))
	{
		ent->client->pers.inventory[power_cube_index] -= GOLEM_COST;
		ent->client->ability_delay = level.time + 1.0;

		//  entity made a sound, used to alert monsters
		ent->lastsound = level.framenum;
	}
}

void Cmd_Golem_f(edict_t* ent)
{
	int type;
	char* s;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called %s\n", ent->client->pers.netname, __func__);

	if (ent->deadflag == DEAD_DEAD)
		return;

	s = gi.args();

	if (!Q_strcasecmp(s, "help"))
	{
		safe_cprintf(ent, PRINT_HIGH, "golem [remove|command|follow me]\n");
		return;
	}

	if (!Q_strcasecmp(s, "command"))
	{
		MonsterCommand(ent);
		return;
	}

	if (!Q_strcasecmp(s, "follow me"))
	{
		MonsterFollowMe(ent);
		return;
	}

	if (ent->myskills.abilities[GOLEM].disable)
		return;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[GOLEM].current_level, 0))
		return;

	if (!Q_strcasecmp(s, "remove"))
	{
		RemoveDrone(ent);
		return;
	}

	if (ent->client->pers.inventory[power_cube_index] < GOLEM_COST)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need more power cubes to use this ability.\n");
		return;
	}
	if (ent->num_golems >= GOLEM_MAX)
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't create any more golems.\n");
		return;
	}

	spawn_golem(ent);
}