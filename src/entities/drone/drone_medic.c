/*
==============================================================================

MEDIC

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_medic.h"

static int	sound_idle1;
static int	sound_pain1;
static int	sound_pain2;
static int	sound_die;
static int	sound_sight;
static int	sound_search;
static int	sound_hook_launch;
static int	sound_hook_hit;
static int	sound_hook_heal;
static int	sound_hook_retract;
static int	commander_sound_idle1;
static int	commander_sound_pain1;
static int	commander_sound_pain2;
static int	commander_sound_die;
static int	commander_sound_sight;
static int	commander_sound_hook_launch;
static int	commander_sound_hook_hit;
static int	commander_sound_hook_heal;
static int	commander_sound_hook_retract;
static int	commander_sound_spawn;

void drone_ai_run_slide(edict_t *self, float dist);
void mymedic_run(edict_t *self);
extern mmove_t mymedic_move_attackHyperBlaster;
extern mmove_t mymedic_move_attackBlaster;
extern mmove_t mymedic_move_attackCable;
extern mmove_t medic_commander_move_callReinforcements;
static qboolean mymedic_is_dodge_move(edict_t *self);

#define MEDIC_COMMANDER_SUMMON_COUNT	2
#define MEDIC_COMMANDER_SUMMON_COOLDOWN	8.0f
#define SPAWNGROW_LIFESPAN				1.0f

void mymedic_refire (edict_t *self);
void mymedic_heal (edict_t *self);
void medic_commander_attack(edict_t *self);

static qboolean medic_is_commander(edict_t *self)
{
	return self && self->mtype == M_MEDIC_COMMANDER;
}

static int medic_idle_sound(edict_t *self)
{
	return (medic_is_commander(self) && commander_sound_idle1) ? commander_sound_idle1 : sound_idle1;
}

static int medic_pain_sound1(edict_t *self)
{
	return (medic_is_commander(self) && commander_sound_pain1) ? commander_sound_pain1 : sound_pain1;
}

static int medic_pain_sound2(edict_t *self)
{
	return (medic_is_commander(self) && commander_sound_pain2) ? commander_sound_pain2 : sound_pain2;
}

static int medic_die_sound(edict_t *self)
{
	return (medic_is_commander(self) && commander_sound_die) ? commander_sound_die : sound_die;
}

static int medic_sight_sound(edict_t *self)
{
	return (medic_is_commander(self) && commander_sound_sight) ? commander_sound_sight : sound_sight;
}

static int medic_hook_launch_sound(edict_t *self)
{
	return (medic_is_commander(self) && commander_sound_hook_launch) ? commander_sound_hook_launch : sound_hook_launch;
}

static int medic_hook_retract_sound(edict_t *self)
{
	return (medic_is_commander(self) && commander_sound_hook_retract) ? commander_sound_hook_retract : sound_hook_retract;
}

static float medic_clampf(float value, float min_value, float max_value)
{
	if (value < min_value)
		return min_value;
	if (value > max_value)
		return max_value;
	return value;
}

static float medic_lerpf(float a, float b, float t)
{
	return a + (b - a) * t;
}

void mymedic_idle (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, medic_idle_sound(self), 1, ATTN_IDLE, 0);

}

mframe_t mymedic_frames_stand [] =
{
	drone_ai_stand, 0, mymedic_idle,	//12
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
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,	//101

};
mmove_t mymedic_move_stand = {FRAME_wait1, FRAME_wait90, mymedic_frames_stand, NULL};

void mymedic_stand (edict_t *self)
{
//	gi.dprintf("mymedic_stand()\n");
	self->monsterinfo.currentmove = &mymedic_move_stand;
}
mframe_t medic_frames_walk [] =
{
	drone_ai_walk, 6.2,	NULL,
	drone_ai_walk, 18.1,  NULL,
	drone_ai_walk, 1,		NULL,
	drone_ai_walk, 9,		NULL,
	drone_ai_walk, 10,	NULL,
	drone_ai_walk, 9,		NULL,
	drone_ai_walk, 11,	NULL,
	drone_ai_walk, 11.6,  NULL,
	drone_ai_walk, 2,		NULL,
	drone_ai_walk, 9.9,	NULL,
	drone_ai_walk, 14,	NULL,
	drone_ai_walk, 9.3,	NULL
};
mmove_t medic_move_walk = {FRAME_walk1, FRAME_walk12, medic_frames_walk, NULL};

void medic_walk (edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &medic_move_walk;
}

mframe_t mymedic_frames_run [] =
{
	drone_ai_run, 30,	NULL,	//102
	drone_ai_run, 30,	NULL,
	drone_ai_run, 30,	NULL,
	drone_ai_run, 30,	NULL,
	drone_ai_run, 30,	NULL,
	drone_ai_run, 30,	NULL	//107
	
};
mmove_t mymedic_move_run = {FRAME_run1, FRAME_run6, mymedic_frames_run, NULL};

static void mymedic_ai_dodge_slide(edict_t *self, float dist)
{
	if (!G_EntIsAlive(self->enemy) && G_EntIsAlive(self->monsterinfo.attacker))
		self->enemy = self->monsterinfo.attacker;
	if (!G_EntIsAlive(self->enemy))
		return;

	drone_ai_run_slide(self, dist);
}

mframe_t mymedic_frames_dodge_slide[] =
{
	mymedic_ai_dodge_slide, 18, NULL,
	mymedic_ai_dodge_slide, 22.5, NULL,
	mymedic_ai_dodge_slide, 25.4, NULL,
	mymedic_ai_dodge_slide, 23.4, NULL,
	mymedic_ai_dodge_slide, 24, NULL,
	mymedic_ai_dodge_slide, 35.6, NULL
};
mmove_t mymedic_move_dodge_slide = {FRAME_run1, FRAME_run6, mymedic_frames_dodge_slide, mymedic_run};

void mymedic_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &mymedic_move_stand;
	else
		self->monsterinfo.currentmove = &mymedic_move_run;
}

void mymedic_fire_blaster (edict_t *self)
{
	int		effect, damage, flash_number;
	const int speed = 2000; // speed: medic_blaster
	vec3_t	forward, start;
	qboolean bounce = false;

	if (!G_EntExists(self->enemy))
		return;
	
	if ((self->s.frame == FRAME_attack9) || (self->s.frame == FRAME_attack12))
	{
		effect = EF_BLASTER;
		bounce = true;
		flash_number = medic_is_commander(self) ? MZ2_MEDIC_BLASTER_2 : MZ2_MEDIC_BLASTER_1;
	}
	else
	{
		int frame_offset = self->s.frame - FRAME_attack19;

		if (frame_offset < 0)
			frame_offset = 0;
		else if (frame_offset > 11)
			frame_offset = 11;

		effect = (self->s.frame % 4) ? 0 : EF_HYPERBLASTER;
		flash_number = (medic_is_commander(self) ? MZ2_MEDIC_HYPERBLASTER2_1 : MZ2_MEDIC_HYPERBLASTER1_1) + frame_offset;
	}

	damage = M_HYPERBLASTER_DMG_BASE + M_HYPERBLASTER_DMG_ADDON * drone_damagelevel(self);
	if (M_HYPERBLASTER_DMG_MAX && damage > M_HYPERBLASTER_DMG_MAX)
		damage = M_HYPERBLASTER_DMG_MAX;


	MonsterAim(self, M_PROJECTILE_ACC, speed, false, flash_number, forward, start);
	if (medic_is_commander(self))
		monster_fire_blaster2(self, start, forward, damage, speed, effect, flash_number);
	else
		monster_fire_blaster(self, start, forward, damage, speed, effect, BLASTER_PROJ_BOLT, 2.0, bounce, flash_number);
}

void mymedic_fire_bolt (edict_t *self)
{
	int		min, max, damage;
	vec3_t	forward, start;

	min = 4 * drone_damagelevel(self); // dmg.min: medic_fire_bolt
	max = 50 + 25 * drone_damagelevel(self); // dmg.max: medic_fire_bolt

	damage = GetRandom(min, max);

	MonsterAim(self, M_PROJECTILE_ACC, 1500, false, MZ2_MEDIC_BLASTER_1, forward, start);
	// Keep the medic muzzle origin, but don't emit the muzzleflash packet here;
	// it can override the secondary blaster sound that should play per bolt.
	monster_fire_blaster(self, start, forward, damage, 1500, EF_BLASTER, BLASTER_PROJ_BLAST, 2.0, true, -1);

	gi.sound (self, CHAN_WEAPON, gi.soundindex("weapons/photon.wav"), 1, ATTN_NORM, 0);
}


void mymedic_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	//self->nextthink = 0;
	gi.linkentity (self);
	M_PrepBodyRemoval(self);
}

mframe_t medic_frames_pain_short[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
};
mmove_t medic_move_pain_short = { FRAME_paina1, FRAME_paina8, medic_frames_pain_short, mymedic_run };

mframe_t medic_frames_pain_long[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
};
mmove_t medic_move_pain_long = { FRAME_painb1, FRAME_painb15, medic_frames_pain_long, mymedic_run };

void medic_pain(edict_t* self, edict_t* other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	// we're already in a pain state
	if (self->monsterinfo.currentmove == &medic_move_pain_short ||
		self->monsterinfo.currentmove == &medic_move_pain_long ||
		mymedic_is_dodge_move(self))
		return;

	// monster players don't get pain state induced
	if (G_GetClient(self))
		return;

	// no pain in invasion hard mode
	if (invasion->value == 2)
		return;

	// if we're fidgeting, always go into pain state.
	if (random() <= (1.0f - self->monsterinfo.pain_chance) &&
		self->monsterinfo.currentmove != &medic_move_walk &&
		self->monsterinfo.currentmove != &mymedic_move_stand)
		return;

	if (random() < 0.5)
		gi.sound(self, CHAN_VOICE, medic_pain_sound1(self), 1, ATTN_NORM, 0);
	else {
		gi.sound(self, CHAN_VOICE, medic_pain_sound2(self), 1, ATTN_NORM, 0);
	}

	if (self->monsterinfo.currentmove == &medic_move_walk ||
		self->monsterinfo.currentmove == &mymedic_move_stand)
		self->monsterinfo.currentmove = &medic_move_pain_long;
	else
		self->monsterinfo.currentmove = &medic_move_pain_short;
}

mframe_t mymedic_frames_death [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t mymedic_move_death = {FRAME_death1, FRAME_death30, mymedic_frames_death, mymedic_dead};

void mymedic_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		if (vrx_spawn_nonessential_ent(self->s.origin))
		{
			for (n = 0; n < 2; n++)
				ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
			for (n = 0; n < 4; n++)
				ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
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

// regular death
	gi.sound (self, CHAN_VOICE, medic_die_sound(self), 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	self->monsterinfo.currentmove = &mymedic_move_death;

	if (self->activator && !self->activator->client)
	{
		self->activator->num_monsters_real--;
		// gi.bprintf(PRINT_HIGH, "releasing %p (%d)\n", self, self->activator->num_monsters_real);
	}
}

void mymedic_duck_down (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;
	if (!self->groundentity)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] = 0;
	self->takedamage = DAMAGE_YES;
	gi.linkentity (self);
}

void mymedic_duck_up (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] = 32;
	self->takedamage = DAMAGE_AIM;
	VectorClear(self->velocity);
	gi.linkentity (self);
}

void mymedic_duck_hold (edict_t *self)
{
	if (self->monsterinfo.pausetime > level.time)
		self->monsterinfo.nextframe = self->s.frame;
}


void mymedic_jump_takeoff (edict_t *self)
{
	vec3_t	v;

	gi.sound (self, CHAN_VOICE, medic_sight_sound(self), 1, ATTN_NORM, 0);
	VectorSubtract(self->monsterinfo.dir, self->s.origin, v);
	v[2] = 0;
	VectorNormalize(v);
	VectorScale(v, -200, self->velocity);
	self->velocity[2] = 400;
	self->monsterinfo.pausetime = level.time + 2.0; // maximum duration of jump
}

void mymedic_jump_hold (edict_t *self)
{
	vec3_t	v;

	if (G_EntExists(self->monsterinfo.attacker))
	{
		// face the attacker
		VectorSubtract(self->monsterinfo.attacker->s.origin, self->s.origin, v);
		self->ideal_yaw = vectoyaw(v);
		M_ChangeYaw(self);
	}
	// check for landing or jump timeout
	if (self->groundentity || (level.time > self->monsterinfo.pausetime))
	{
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		VectorClear(self->velocity);
	}
	else
	{
		// we're still in the air
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
	}
}

mframe_t mymedic_frames_duck [] =
{
	ai_move, -1, NULL,
	ai_move, -1, mymedic_duck_down,
	ai_move, -1, mymedic_duck_hold,
	ai_move, -1, NULL,
	ai_move, -1, NULL,
	ai_move, -1, NULL,
	ai_move, -1, NULL,
	ai_move, -1, NULL,
	ai_move, -1, NULL,
	ai_move, -1, NULL,
	ai_move, -1, NULL,
	ai_move, -1, NULL,
	ai_move, -1, mymedic_duck_up
};
mmove_t mymedic_move_duck = {FRAME_duck2, FRAME_duck14, mymedic_frames_duck, mymedic_run};

mframe_t mymedic_frames_leap [] =
{
	ai_move, 0,	mymedic_jump_takeoff,	//131
	ai_move, 0,	NULL,
	ai_move, 0,	NULL,
	ai_move, 0,	NULL,
	ai_move, 0,	NULL,
	ai_move, 0,	NULL,
	ai_move, 0,	mymedic_jump_hold,		//137
	/*
	ai_move, 0,	NULL,
	ai_move, 0,	NULL,
	ai_move, 0,	NULL,
	ai_move, 0,	NULL,
	ai_move, 0,	NULL,
	ai_move, 0,	NULL,
	ai_move, 0,	NULL,
	ai_move, 0,	NULL,
	ai_move, 0,	NULL
	*/
};
mmove_t mymedic_move_leap = {FRAME_duck1, FRAME_duck7, mymedic_frames_leap, mymedic_run};

void mymedic_leap (edict_t *self)
{
	if (self->groundentity)
		self->monsterinfo.currentmove = &mymedic_move_leap;
}

static qboolean mymedic_is_dodge_move(edict_t *self)
{
	return self->monsterinfo.currentmove == &mymedic_move_duck ||
		self->monsterinfo.currentmove == &mymedic_move_leap ||
		self->monsterinfo.currentmove == &mymedic_move_dodge_slide;
}

static qboolean mymedic_is_uninterruptible_attack(edict_t *self)
{
	return self->monsterinfo.currentmove == &mymedic_move_attackHyperBlaster ||
		self->monsterinfo.currentmove == &mymedic_move_attackCable ||
		self->monsterinfo.currentmove == &mymedic_move_attackBlaster ||
		self->monsterinfo.currentmove == &medic_commander_move_callReinforcements;
}

static qboolean mymedic_dodge_hit_low(edict_t *self, vec3_t dir)
{
	const float duck_height = self->absmax[2] - 33;

	return dir[2] > self->absmin[2] && dir[2] <= duck_height;
}

void mymedic_dodge (edict_t *self, edict_t *attacker, vec3_t dir, int radius)
{
	if (random() > 0.9)
		return;
	if (level.time < self->monsterinfo.dodge_time)
		return;
	if (!attacker)
		return;
	if (OnSameTeam(self, attacker))
		return;
	if (mymedic_is_dodge_move(self) || mymedic_is_uninterruptible_attack(self))
		return;

	self->monsterinfo.attacker = attacker;
	if (!G_EntIsAlive(self->enemy) && G_EntIsAlive(attacker))
		self->enemy = attacker;
	if (!radius)
	{
		if (!mymedic_dodge_hit_low(self, dir))
		{
			self->monsterinfo.pausetime = level.time + 0.5;
			self->monsterinfo.currentmove = &mymedic_move_duck;
			mymedic_duck_down(self);
		}
		else if (!(self->monsterinfo.aiflags & AI_STAND_GROUND))
		{
			self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
			self->monsterinfo.currentmove = &mymedic_move_dodge_slide;
		}
		else
		{
			self->monsterinfo.pausetime = level.time + 0.5;
			self->monsterinfo.currentmove = &mymedic_move_duck;
			mymedic_duck_down(self);
		}
		self->monsterinfo.dodge_time = level.time + 2.0;
	}
	else
	{
		mymedic_leap(self);
		self->monsterinfo.dodge_time = level.time + 3.0;
	}
}

void medic_checktarget (edict_t *self)
{
	// stop shooting at corpses!
	if (!self->enemy || !self->enemy->inuse || self->enemy->health < 1)
	{
		self->monsterinfo.currentmove = &mymedic_move_stand;
		return;
	}
}

mframe_t mymedic_frames_attackHyperBlaster [] =
{
	ai_charge, 0,	NULL,					// attack15
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	mymedic_fire_blaster,	// attack19
	ai_charge, 0,	mymedic_fire_blaster,
	ai_charge, 0,	mymedic_fire_blaster,
	ai_charge, 0,	mymedic_fire_blaster,
	ai_charge, 0,	mymedic_fire_blaster,
	ai_charge, 0,	mymedic_fire_blaster,
	ai_charge, 0,	mymedic_fire_blaster,
	ai_charge, 0,	mymedic_fire_blaster,
	ai_charge, 0,	mymedic_fire_blaster,
	ai_charge, 0,	mymedic_fire_blaster,
	ai_charge, 0,	mymedic_fire_blaster,
	ai_charge, 0,	mymedic_fire_blaster,	// attack30
	ai_charge, 0,	medic_checktarget,
	ai_charge, 0,	NULL,
	ai_charge, 2,	NULL,
	ai_charge, 3,	NULL
};
mmove_t mymedic_move_attackHyperBlaster = {FRAME_attack15, FRAME_attack34, mymedic_frames_attackHyperBlaster, mymedic_refire};

void mymedic_refire(edict_t* self)
{
	float dist = 9999;

	if (G_ValidTarget(self, self->enemy, true, true))
	{
		dist = entdist(self, self->enemy);

		if (random() <= 0.8 && dist <= 512)
		{
			// continue attack
			self->s.frame = FRAME_attack19;
			M_DelayNextAttack(self, 0, true);
			return;
		}
	}
	else
		self->enemy = NULL;

	// end attack
	mymedic_run(self);

	if (dist <= 128 || (self->monsterinfo.aiflags & AI_STAND_GROUND))
		M_DelayNextAttack(self, 0, true);
	else
		M_DelayNextAttack(self, (GetRandom(10, 20) * FRAMETIME), false);
}

void mymedic_continue(edict_t* self)
{
	if (M_ContinueAttack(self, &mymedic_move_attackHyperBlaster, NULL, 0, 512, 0.8))
		return;

	// end attack
	mymedic_run(self);
}

mframe_t mymedic_frames_attackBlaster [] =
{
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	mymedic_fire_bolt,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	mymedic_fire_bolt,	
	ai_charge, 0,	NULL,
	ai_charge, 0,	mymedic_continue
};
mmove_t mymedic_move_attackBlaster = {FRAME_attack1, FRAME_attack14, mymedic_frames_attackBlaster, mymedic_run};

void mymedic_hook_launch (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, medic_hook_launch_sound(self), 1, ATTN_NORM, 0);
}

void mymedic_hook_retract (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, medic_hook_retract_sound(self), 1, ATTN_NORM, 0);

	if (!self->enemy)
		return;
	//if ((self->svflags & SVF_MONSTER) && !(self->client))
	//	self->enemy->monsterinfo.aiflags &= ~AI_RESURRECTING;
}

void ED_CallSpawn (edict_t *ent);

static vec3_t	mymedic_cable_offsets[] =
{
	45.0,  -9.2, 15.5,
	48.4,  -9.7, 15.2,
	47.8,  -9.8, 15.8,
	47.3,  -9.3, 14.3,
	45.4, -10.1, 13.1,
	41.9, -12.7, 12.0,
	37.8, -15.8, 11.2,
	34.3, -18.4, 10.7,
	32.7, -19.7, 10.4,
	32.7, -19.7, 10.4
};

edict_t *CreateSpiker (edict_t *ent, int skill_level);
edict_t* CreateObstacle(edict_t* ent, int skill_level, int talent_level);
edict_t* CreateGasser(edict_t* ent, int skill_level, int talent_level);

void M_Reanimate (edict_t *ent, edict_t *target, int r_level, float r_modifier, qboolean printMsg)
{
	vec3_t	bmin, bmax;
	edict_t *e;

	// save the original monster's level
	// note: this isn't strictly needed for monsters since they don't apply any bonuses--it's mainly here for player-medics
	if (!target->monsterinfo.resurrected_level)
		target->monsterinfo.resurrected_level = target->monsterinfo.level;

	if (!strcmp(target->classname, "drone") && !(target->flags & FL_UNDEAD) && target->mtype != M_DECOY && target->mtype != M_GOLEM) // can't revive decoys, golem, and undead/skeletons
	{
		// if the summoner is a player, check for sufficient monster slots
		if (ent->client && (ent->num_monsters + target->monsterinfo.control_cost > MAX_MONSTERS))
			return;

		target->monsterinfo.level = r_level;
		M_SetBoundingBox(target->mtype, bmin, bmax);
		
		if (G_IsValidLocation(target, target->s.origin, bmin, bmax) && M_Initialize(ent, target, 0.0f))
		{
			//gi.dprintf("resurrect drone at %.1f\n", level.time);
			// restore this drone
			target->monsterinfo.slots_freed = false; // reset freed flag
			target->health = r_modifier*target->max_health;
			target->monsterinfo.power_armor_power = r_modifier*target->monsterinfo.max_armor;
			target->monsterinfo.resurrected_time = level.time + 10.0;
			target->activator = ent; // transfer ownership!
			target->nextthink = level.time + FRAMETIME;//1.0; note: don't delay think--this may cause undesired behavior (monster sliding)
			target->monsterinfo.resurrected_timeout = target->nextthink + MEDIC_RESURRECT_TIMEOUT; // resurrected monster dies when this timer is exceeded
			target->monsterinfo.attack_finished = level.time + 1.0;//delay attack--alternatively we can set the think func to drone_grow
			gi.linkentity(target);
			target->monsterinfo.stand(target);

			ent->num_monsters += target->monsterinfo.control_cost;
			ent->num_monsters_real++;
			// gi.bprintf(PRINT_HIGH, "adding %p (%d)\n", target, ent->num_monsters_real);

			// make sure invasion monsters hunt for navi
			if (invasion->value && !ent->client && ent->activator && !ent->activator->client)
			{
				target->monsterinfo.aiflags &= ~AI_STAND_GROUND;
				target->monsterinfo.aiflags |= AI_FIND_NAVI;
			}

			if (ent->client && printMsg)
				safe_cprintf(ent, PRINT_HIGH, "Resurrected a %s. (%d/%d)\n", target->classname, 
					ent->num_monsters, (int)MAX_MONSTERS);
		}
	}
	else if ((!strcmp(target->classname, "bodyque") || !strcmp(target->classname, "player")))
	{
		const int		random=GetRandom(1, 6);
		vec3_t	start;

		// if the summoner is a player, check for sufficient monster slots
		if (ent->client && (ent->num_monsters + 1 > MAX_MONSTERS))
			return;

		e = G_Spawn();

		VectorCopy(target->s.origin, start);

		// kill the corpse
		T_Damage(target, target, target, vec3_origin, target->s.origin,
			vec3_origin, 10000, 0, DAMAGE_NO_PROTECTION, 0);

		// random soldier type with different weapons
		switch (random)
		{
		case 1:
			// blaster
			e->mtype = M_SOLDIER;
			e->s.skinnum = 0;
			break;
		case 2:
			// rocket
			e->mtype = M_SOLDIERLT;
			e->s.skinnum = 4;
			break;
		case 3:
			// shotgun
			e->mtype = M_SOLDIERSS;
			e->s.skinnum = 2;
			break;
		case 4:
			// ripper
			e->mtype = M_SOLDIER_RIPPER;
			e->s.skinnum = 6;
			break;
		case 5:
			// blue blaster
			e->mtype = M_SOLDIER_BLUEBLASTER;
			e->s.skinnum = 8;
			break;
		case 6:
		default:
			// laser
			e->mtype = M_SOLDIER_LASER;
			e->s.skinnum = 10;
			break;
		}

		e->activator = ent;
		e->monsterinfo.level = r_level;
		M_Initialize(ent, e, 0.0f);
		e->health = r_modifier*e->max_health;
		e->monsterinfo.power_armor_power = r_modifier*e->monsterinfo.max_armor;
		e->monsterinfo.resurrected_time = level.time + 10.0;
		e->s.skinnum |= 1; // injured skin

		e->monsterinfo.stand(e);

		if (!G_IsValidLocation(target, start, e->mins, e->maxs))
		{
			start[2] += 24;
			if (!G_IsValidLocation(target, start, e->mins, e->maxs))
			{
				DroneList_Remove(e); // az: AAAAAAAARGH
				G_FreeEdict(e);
				return;
			}
		}

		VectorCopy(start, e->s.origin);
		gi.linkentity(e);
		e->nextthink = level.time + 1.0;
		e->monsterinfo.resurrected_timeout = e->nextthink + MEDIC_RESURRECT_TIMEOUT; // resurrected monster dies when this timer is exceeded
		ent->num_monsters += e->monsterinfo.control_cost;
		ent->num_monsters_real++;
		// gi.bprintf(PRINT_HIGH, "adding %p (%d)\n", e, ent->num_monsters_real);

		// make sure invasion monsters hunt for navi
		if (invasion->value && !ent->client && ent->activator && !ent->activator->client)
		{
			target->monsterinfo.aiflags &= ~AI_STAND_GROUND;
			target->monsterinfo.aiflags |= AI_FIND_NAVI;
		}

		if (ent->client && printMsg)
			safe_cprintf(ent, PRINT_HIGH, "Resurrected a soldier. (%d/%d)\n", ent->num_monsters, (int)MAX_MONSTERS);
	}
	else if (!strcmp(target->classname, "spiker"))
	{
		// if the summoner is a player, check for sufficient spiker slots
		if (ent->client && (ent->num_spikers + 1 > SPIKER_MAX_COUNT))
			return;

		e = CreateSpiker(ent, r_level);

		// make sure the new entity fits
		if (!G_IsValidLocation(target, target->s.origin, e->mins, e->maxs))
		{
			ent->num_spikers--;
			G_FreeEdict(e);
			return;
		}
		
		VectorCopy(target->s.angles, e->s.angles);
		e->s.angles[PITCH] = 0;
		e->monsterinfo.cost = target->monsterinfo.cost;
		e->health = r_modifier * e->max_health;
		e->monsterinfo.resurrected_time = level.time + 10.0;
		e->monsterinfo.resurrected_timeout = e->nextthink + MEDIC_RESURRECT_TIMEOUT; // resurrected spiker dies when this timer is exceeded
		e->s.frame = 4;
		VectorCopy(target->s.origin, e->s.origin);
		gi.linkentity(e);

		organ_remove(target, false);

		if (ent->client && printMsg)
			safe_cprintf(ent, PRINT_HIGH, "Resurrected a spiker. (%d/%d)\n", ent->num_spikers, SPIKER_MAX_COUNT);
	}
	else if (!strcmp(target->classname, "obstacle"))
	{
		// if the summoner is a player, check for sufficient obstacle slots
		if (ent->client && (ent->num_obstacle + 1 > OBSTACLE_MAX_COUNT))
			return;

		e = CreateObstacle(ent, r_level, vrx_get_talent_level(ent, TALENT_MAGNETISM));

		// make sure the new entity fits
		if (!G_IsValidLocation(target, target->s.origin, e->mins, e->maxs))
		{
			ent->num_obstacle--;
			G_FreeEdict(e);
			return;
		}
		
		VectorCopy(target->s.angles, e->s.angles);
		e->s.angles[PITCH] = 0;
		e->monsterinfo.cost = target->monsterinfo.cost;
		e->monsterinfo.resurrected_time = level.time + 10.0;
		e->monsterinfo.resurrected_timeout = e->nextthink + MEDIC_RESURRECT_TIMEOUT; // resurrected obstacle dies when this timer is exceeded
		e->health = r_modifier * e->max_health;
		e->s.frame = 6;
		VectorCopy(target->s.origin, e->s.origin);
		gi.linkentity(e);

		organ_remove(target, false);

		if (ent->client && printMsg)
			safe_cprintf(ent, PRINT_HIGH, "Resurrected an obstacle. (%d/%d)\n", ent->num_obstacle, OBSTACLE_MAX_COUNT);
	}
	else if (!strcmp(target->classname, "gasser"))
	{
		// if the summoner is a player, check for sufficient gasser slots
		if (ent->client && (ent->num_gasser + 1 > GASSER_MAX_COUNT))
			return;

		e = CreateGasser(ent, r_level, vrx_get_talent_level(ent, TALENT_SPITTING_GASSER));

		// make sure the new entity fits
		if (!G_IsValidLocation(target, target->s.origin, e->mins, e->maxs))
		{
			ent->num_gasser--;
			G_FreeEdict(e);
			return;
		}
		
		VectorCopy(target->s.angles, e->s.angles);
		e->s.angles[PITCH] = 0;
		e->monsterinfo.cost = target->monsterinfo.cost;
		e->monsterinfo.resurrected_time = level.time + 10.0;
		e->monsterinfo.resurrected_timeout = e->nextthink + MEDIC_RESURRECT_TIMEOUT; // resurrected gasser dies when this timer is exceeded
		e->health = r_modifier * e->max_health;
		e->s.frame = 0;
		VectorCopy(target->s.origin, e->s.origin);
		gi.linkentity(e);

		organ_remove(target, false);

		if (ent->client && printMsg)
			safe_cprintf(ent, PRINT_HIGH, "Resurrected a gasser. (%d/%d)\n", ent->num_gasser, GASSER_MAX_COUNT);
	}
		
}

void mymedic_cable_attack (edict_t *self)
{
	vec3_t	forward, right, start, offset, end;
	trace_t	tr;

	// need a valid target and activator
	if (!self || !self->inuse || !self->activator || !self->activator->inuse 
		|| !self->enemy || !self->enemy->inuse)
		return;

	// make sure target is still in range
	if (entdist(self, self->enemy) > 256)
		return;

	// get muzzle location
	AngleVectors(self->s.angles, forward, right, NULL);
	VectorCopy(mymedic_cable_offsets[self->s.frame - FRAME_attack42], offset);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
	// get end position
	//VectorCopy(self->enemy->s.origin, end);
	//end[2] = self->enemy->absmax[2]-8;
	G_EntMidPoint(self->enemy, end);

	tr = gi.trace (start, NULL, NULL, end, self, MASK_SHOT);
	if (tr.ent != self->enemy)
	{
		if (self->s.frame == 226)
		{
			// give up for awhile
			self->s.frame = 229;
			M_DelayNextAttack(self, (GetRandom(10, 20)*FRAMETIME), true);
			mymedic_hook_retract(self);
			
			// if our enemy is a corpse, destroy it
			if (self->enemy->health < 1)
			{
				T_Damage(self->enemy, self->enemy, self->enemy, vec3_origin, 
					self->enemy->s.origin, vec3_origin, 10000, 0, DAMAGE_NO_PROTECTION, 0);
			}
		}
		return; // cable is blocked
	}

	// cable effect
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_MEDIC_CABLE_ATTACK);
	gi.WriteShort (self - g_edicts);
	gi.WritePosition (start);
	gi.WritePosition (tr.endpos);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	// the target needs healing
	if (M_NeedRegen(self->enemy))
	{
		int frames = qf2sf(6000/(12*self->monsterinfo.level));

		//gi.dprintf("regenerate drone at %.1f\n", level.time);
		if (!frames)
			frames = 1;

		// remove all curses
		CurseRemove(self->enemy, 0, 0);

		//Give them a short period of curse immunity
		self->enemy->holywaterProtection = level.time + 2.0; //2 seconds immunity

		// heal them
		M_Regenerate(self->enemy, frames, 0, 1.0, true, true, false, &self->enemy->monsterinfo.regen_delay2);

		// hold monsters in-place
		if (self->enemy->svflags & SVF_MONSTER)
			self->enemy->holdtime = level.time + 0.2;
	}
	// the target is a dead monster and needs resurrection
	else if (self->enemy->health < 1)
	{
		M_Reanimate(self->activator, self->enemy, self->monsterinfo.level, 0.33, false);
	}
}

void mymedic_delay (edict_t *self)
{
	self->monsterinfo.attack_finished = level.time + random() + 1;
}

void mymedic_cable_continue (edict_t *self)
{
	// if target still needs healing, loop heal frames
	if (M_ValidMedicTarget(self, self->enemy) && (entdist(self, self->enemy) <= 256))
	{
		self->s.frame = 218;
		mymedic_cable_attack(self);
	}
}

mframe_t mymedic_frames_attackCable [] =
{
	ai_charge, 0,		NULL,					//209
	ai_charge, 0,		NULL,
	ai_charge, 0,		NULL,
	ai_charge, 0,		NULL,
	ai_charge, 0,		NULL,
	ai_charge, 0,		NULL,
	ai_charge, 0,		NULL,
	ai_charge, 0,		NULL,
	ai_charge, 0,		NULL,
	ai_charge, 0,		mymedic_hook_launch,	//218
	ai_charge, 0,		mymedic_cable_attack,
	ai_charge, 0,		mymedic_cable_attack,
	ai_charge, 0,		mymedic_cable_attack,
	ai_charge, 0,		mymedic_cable_attack,
	ai_charge, 0,		mymedic_cable_attack,
	ai_charge, 0,		mymedic_cable_attack,
	ai_charge, 0,		mymedic_cable_attack,
	ai_charge, 0,		mymedic_cable_attack,
	ai_charge, 0,		mymedic_cable_continue,	//227--loop from 218 to here
	ai_charge, 0,		NULL,
	ai_charge, 0,		mymedic_hook_retract,	//229
	ai_charge, 0,		NULL,
	ai_charge, 0,		NULL,
	ai_charge, 0,		NULL,
	ai_charge, 0,		NULL,
	ai_charge, 0,		NULL,
	ai_charge, 0,		NULL,
	ai_charge, 0,		mymedic_delay			//236
};
mmove_t mymedic_move_attackCable = {FRAME_attack33, FRAME_attack60, mymedic_frames_attackCable, mymedic_heal};

static void spawngrow_beam_think(edict_t *self);

static void spawngrow_beam_pos(edict_t *self, vec3_t pos)
{
	float theta;
	float phi;
	float radius;
	vec3_t dir;

	if (!self->owner || !self->owner->inuse)
	{
		VectorCopy(self->s.origin, pos);
		return;
	}

	theta = random() * 2.0f * (float)M_PI;
	phi = acosf(crandom());
	dir[0] = sinf(phi) * cosf(theta);
	dir[1] = sinf(phi) * sinf(theta);
	dir[2] = cosf(phi);

	radius = self->owner->s.scale;
	if (radius <= 0.0f)
		radius = 1.0f;
	radius *= 9.0f;

	VectorMA(self->owner->s.origin, radius, dir, pos);
}

static void spawngrow_think(edict_t *self)
{
	float t;
	int i;

	if (level.time >= self->timestamp)
	{
		if (self->target_ent && self->target_ent->inuse)
			G_FreeEdict(self->target_ent);
		G_FreeEdict(self);
		return;
	}

	for (i = 0; i < 3; i++)
		self->s.angles[i] += self->avelocity[i] * FRAMETIME;

	t = 1.0f - ((level.time - self->teleport_time) / self->wait);
	t = medic_clampf(t, 0.0f, 1.0f);
	self->s.scale = medic_clampf(medic_lerpf(self->decel, self->accel, t) / 16.0f, 0.001f, 16.0f);
	self->s.alpha = t * t;

	self->nextthink = level.time + FRAMETIME;
}

static void spawngrow_beam_think(edict_t *self)
{
	if (!self->owner || !self->owner->inuse)
	{
		G_FreeEdict(self);
		return;
	}

	spawngrow_beam_pos(self, self->s.old_origin);
	gi.linkentity(self);
	self->nextthink = level.time + FRAMETIME;
}

void SpawnGrow_Spawn(vec3_t startpos, float start_size, float end_size)
{
	edict_t *ent;
	edict_t *beam;

	ent = G_Spawn();
	VectorCopy(startpos, ent->s.origin);
	VectorSet(ent->s.angles, GetRandom(0, 359), GetRandom(0, 359), GetRandom(0, 359));
	VectorSet(ent->avelocity,
		(280.0f + random() * 80.0f) * 2.0f,
		(280.0f + random() * 80.0f) * 2.0f,
		(280.0f + random() * 80.0f) * 2.0f);

	ent->solid = SOLID_NOT;
	ent->movetype = MOVETYPE_NONE;
	ent->classname = "spawngro";
	ent->s.modelindex = gi.modelindex("models/items/spawngro3/tris.md2");
	ent->s.skinnum = 1;
	ent->s.renderfx |= RF_IR_VISIBLE | RF_TRANSLUCENT;
	ent->accel = start_size;
	ent->decel = end_size;
	ent->think = spawngrow_think;
	ent->s.scale = medic_clampf(start_size / 16.0f, 0.001f, 8.0f);
	ent->s.alpha = 1.0f;
	ent->teleport_time = level.time;
	ent->wait = SPAWNGROW_LIFESPAN;
	ent->timestamp = level.time + SPAWNGROW_LIFESPAN;
	ent->nextthink = level.time + FRAMETIME;
	gi.linkentity(ent);

	beam = ent->target_ent = G_Spawn();
	beam->solid = SOLID_NOT;
	beam->movetype = MOVETYPE_NONE;
	beam->s.modelindex = 1;
	beam->s.renderfx = RF_BEAM_LIGHTNING | RF_TRANSLUCENT;
	beam->s.frame = 1;
	beam->s.skinnum = 0x30303030;
	beam->classname = "spawngro_beam";
	beam->owner = ent;
	VectorCopy(ent->s.origin, beam->s.origin);
	spawngrow_beam_pos(beam, beam->s.old_origin);
	beam->think = spawngrow_beam_think;
	beam->nextthink = level.time + FRAMETIME;
	gi.linkentity(beam);
}

static void medic_commander_start_spawn(edict_t *self)
{
	if (commander_sound_spawn)
		gi.sound(self, CHAN_WEAPON, commander_sound_spawn, 1, ATTN_NORM, 0);

	self->monsterinfo.nextframe = FRAME_attack48;
}

static void medic_commander_cleanup_failed_spawn(edict_t *owner, edict_t *spawned)
{
	if (owner && owner->client)
		layout_remove_tracked_entity(&owner->client->layout, spawned);

	DroneList_Remove(spawned);
	AI_EnemyRemoved(spawned);
	G_FreeEdict(spawned);
}

static void medic_commander_setup_invasion_spawn(edict_t *spawned)
{
	if (!invasion->value)
		return;

	spawned->monsterinfo.aiflags &= ~AI_STAND_GROUND;
	spawned->monsterinfo.aiflags |= AI_FIND_NAVI;
	spawned->prev_navi = NULL;
	spawned->goalentity = NULL;
}

static qboolean medic_commander_valid_spawn_spot(edict_t *self, vec3_t mins, vec3_t maxs, vec3_t spot)
{
	if (G_IsValidLocation(self, spot, mins, maxs))
		return true;

	spot[2] += 24;
	if (G_IsValidLocation(self, spot, mins, maxs))
		return true;

	spot[2] -= 48;
	return G_IsValidLocation(self, spot, mins, maxs);
}

static qboolean medic_commander_find_spawn_spot(edict_t *self, vec3_t mins, vec3_t maxs, float side, vec3_t spot)
{
	vec3_t forward, right;

	AngleVectors(self->s.angles, forward, right, NULL);

	VectorCopy(self->s.origin, spot);
	VectorMA(spot, 96, forward, spot);
	VectorMA(spot, side, right, spot);
	spot[2] += 8;
	if (medic_commander_valid_spawn_spot(self, mins, maxs, spot))
		return true;

	VectorCopy(self->s.origin, spot);
	VectorMA(spot, -72, forward, spot);
	VectorMA(spot, side, right, spot);
	spot[2] += 8;
	if (medic_commander_valid_spawn_spot(self, mins, maxs, spot))
		return true;

	VectorCopy(self->s.origin, spot);
	VectorMA(spot, 64, forward, spot);
	VectorMA(spot, -side, right, spot);
	spot[2] += 8;
	return medic_commander_valid_spawn_spot(self, mins, maxs, spot);
}

static int medic_commander_random_soldier_type(void)
{
	static const int soldier_types[] =
	{
		M_SOLDIER,
		M_SOLDIERLT,
		M_SOLDIERSS,
		M_SOLDIER_RIPPER,
		M_SOLDIER_BLUEBLASTER,
		M_SOLDIER_LASER
	};

	return soldier_types[GetRandom(0, (int)(sizeof(soldier_types) / sizeof(soldier_types[0])) - 1)];
}

static qboolean medic_commander_spawn_soldier(edict_t *self, float side)
{
	edict_t *owner = self->activator;
	edict_t *gunner;
	vec3_t spot;

	if (!owner || !owner->inuse)
		return false;

	if (owner->client && owner->num_monsters + M_GUNNER_CONTROL_COST > MAX_MONSTERS)
		return false;

	gunner = G_Spawn();
	gunner->mtype = medic_commander_random_soldier_type();
	gunner->activator = owner;
	gunner->monsterinfo.level = self->monsterinfo.level;

	if (!M_Initialize(owner, gunner, 0.0f))
	{
		G_FreeEdict(gunner);
		return false;
	}

	if (!medic_commander_find_spawn_spot(self, gunner->mins, gunner->maxs, side, spot))
	{
		medic_commander_cleanup_failed_spawn(owner, gunner);
		return false;
	}

	gunner->monsterinfo.cost = 0;
	gunner->s.effects |= EF_PLASMA;
	VectorCopy(spot, gunner->s.origin);
	VectorCopy(spot, gunner->s.old_origin);
	VectorCopy(self->s.angles, gunner->s.angles);
	gunner->nextthink = level.time + FRAMETIME;
	gunner->monsterinfo.attack_finished = level.time + 1.0;
	medic_commander_setup_invasion_spawn(gunner);

	gi.linkentity(gunner);

	owner->num_monsters += gunner->monsterinfo.control_cost;
	owner->num_monsters_real++;

	if (G_ValidTarget(gunner, self->enemy, true, true))
	{
		gunner->enemy = self->enemy;
		if (gunner->monsterinfo.run)
			gunner->monsterinfo.run(gunner);
	}
	else if (gunner->monsterinfo.stand)
		gunner->monsterinfo.stand(gunner);

	return true;
}

static void medic_commander_spawngrows(edict_t *self)
{
	vec3_t mins, maxs, size, spot, effect_origin;
	float radius;
	int i;

	VectorSet(mins, -16, -16, -24);
	VectorSet(maxs, 16, 16, 32);
	VectorSubtract(maxs, mins, size);
	radius = VectorLength(size) * 0.5f;

	for (i = 0; i < MEDIC_COMMANDER_SUMMON_COUNT; i++)
	{
		float side = (i & 1) ? 56 : -56;

		if (!medic_commander_find_spawn_spot(self, mins, maxs, side, spot))
			continue;

		VectorAdd(mins, maxs, effect_origin);
		VectorAdd(spot, effect_origin, effect_origin);
		SpawnGrow_Spawn(effect_origin, radius, radius * 2.0f);
	}
}

static void medic_commander_finish_spawn(edict_t *self)
{
	int spawned = 0;
	int i;

	for (i = 0; i < MEDIC_COMMANDER_SUMMON_COUNT; i++)
	{
		float side = (i & 1) ? 56 : -56;

		if (medic_commander_spawn_soldier(self, side))
			spawned++;
	}

	if (spawned)
		self->monsterinfo.melee_finished = level.time + MEDIC_COMMANDER_SUMMON_COOLDOWN;
}

mframe_t medic_commander_frames_callReinforcements[] =
{
	ai_charge, 2,		NULL,							//209
	ai_charge, 3,		NULL,
	ai_charge, 5,		NULL,
	ai_charge, 4,		NULL,
	ai_charge, 5,		NULL,
	ai_charge, 5,		NULL,
	ai_charge, 6,		NULL,
	ai_charge, 4,		NULL,
	ai_charge, 0,		NULL,							//217
	ai_move, 0,			medic_commander_start_spawn,	//218
	ai_move, 0,			NULL,
	ai_move, 0,			NULL,
	ai_move, 0,			NULL,
	ai_move, 0,			NULL,
	ai_move, 0,			NULL,
	ai_move, 0,			NULL,							//224
	ai_charge, 0,		medic_commander_spawngrows,		//225
	ai_move, 0,			NULL,
	ai_move, 0,			NULL,
	ai_move, -15,		medic_commander_finish_spawn,	//228
	ai_move, -1.5,		NULL,
	ai_move, -1.2,		NULL,
	ai_move, -3,		NULL
};
mmove_t medic_commander_move_callReinforcements = {FRAME_attack33, FRAME_attack55, medic_commander_frames_callReinforcements, mymedic_run};

void drone_wakeallies (edict_t *self);
// search for nearby enemies, return true if one is found
// this is to make the medic stop healing if there is a higher priority target
// this function is the same as drone_findtarget(), except that it skips the medic check
qboolean mymedic_findenemy (edict_t *self)
{
	edict_t *target=NULL;

	while ((target = findclosestradius (target, self->s.origin, 1024)) != NULL)
	{
		if (!G_ValidTarget(self, target, true, true))
			continue;
		self->enemy = target;
		drone_wakeallies(self);
		return true;
	}
	return false;
}

void mymedic_heal (edict_t *self)
{
	// stop healing our target died, if they are fully healed, or
	// they have gone out of range while we are standing ground (can't reach them)
	if (!G_EntIsAlive(self->enemy) || !M_NeedRegen(self->enemy)
		|| ((self->monsterinfo.aiflags & AI_STAND_GROUND) 
		&& (entdist(self, self->enemy) > 256)))
	{
		self->enemy = NULL;
		mymedic_stand(self);
		return;
	}

	// continue healing if our target is still in range and
	// there are no enemies around
	if (OnSameTeam(self, self->enemy) && (entdist(self, self->enemy) <= 256)
		&& !mymedic_findenemy(self))
		self->monsterinfo.currentmove = &mymedic_move_attackCable;
	else
		mymedic_run(self);
}

void mymedic_attack(edict_t *self)
{
	float	dist, r;

	if (!self->enemy)
		return;
	if (!self->enemy->inuse)
		return;

	dist = entdist(self, self->enemy);
	r = random();

	if ((self->monsterinfo.aiflags & AI_MEDIC)
		&& ((self->enemy->health < 1 || OnSameTeam(self, self->enemy))))
	{
		if (dist <= 256)
			self->monsterinfo.currentmove = &mymedic_move_attackCable;
		return;
	}

	if (dist <= 256)
	{
		if (r <= 0.2)
			self->monsterinfo.currentmove = &mymedic_move_attackHyperBlaster;
		else
			self->monsterinfo.currentmove = &mymedic_move_attackBlaster;
	}
	else
	{
		if (r <= 0.3)
			self->monsterinfo.currentmove = &mymedic_move_attackBlaster;
		else
			self->monsterinfo.currentmove = &mymedic_move_attackHyperBlaster;
	}

	M_DelayNextAttack(self, 0, true);
}

void medic_commander_attack(edict_t *self)
{
	edict_t *owner = self->activator;
	float dist;

	if (!self->enemy || !self->enemy->inuse)
		return;

	dist = entdist(self, self->enemy);

	if (dist > 150
		&& owner && owner->inuse
		&& (!owner->client || owner->num_monsters + M_GUNNER_CONTROL_COST <= MAX_MONSTERS)
		&& level.time >= self->monsterinfo.melee_finished
		&& random() < 0.6)
	{
		self->monsterinfo.currentmove = &medic_commander_move_callReinforcements;
		M_DelayNextAttack(self, 0, true);
		return;
	}

	mymedic_attack(self);
}

void mymedic_melee (edict_t *self)
{
	// just here to keep monster from circle strafing
}

void mymedic_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, medic_sight_sound(self), 1, ATTN_NORM, 0);
}

/*QUAKED monster_medic (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void init_drone_medic (edict_t *self)
{
	sound_idle1 = gi.soundindex ("medic/idle.wav");
	sound_pain1 = gi.soundindex ("medic/medpain1.wav");
	sound_pain2 = gi.soundindex ("medic/medpain2.wav");
	sound_die = gi.soundindex ("medic/meddeth1.wav");
	sound_sight = gi.soundindex ("medic/medsght1.wav");
	sound_search = gi.soundindex ("medic/medsrch1.wav");
	sound_hook_launch = gi.soundindex ("medic/medatck2.wav");
	sound_hook_hit = gi.soundindex ("medic/medatck3.wav");
	sound_hook_heal = gi.soundindex ("medic/medatck4.wav");
	sound_hook_retract = gi.soundindex ("medic/medatck5.wav");

	gi.soundindex ("medic/medatck1.wav");

	self->monsterinfo.control_cost = M_MEDIC_CONTROL_COST;
	self->monsterinfo.cost = M_MEDIC_COST;
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex ("models/monsters/medic/tris.md2");
	VectorSet (self->mins, -24, -24, -24);
	VectorSet (self->maxs, 24, 24, 32);

	//if (self->activator && self->activator->client)
	self->health = M_MEDIC_INITIAL_HEALTH + M_MEDIC_ADDON_HEALTH*self->monsterinfo.level; // hlt: medic
	//else self->health = 200 + 20*self->monsterinfo.level;

	self->max_health = self->health;
	self->gib_health = -BASE_GIB_HEALTH;//-self->health;
	self->mass = 400;
	self->mtype = M_MEDIC;
	self->monsterinfo.aiflags |= AI_MEDIC; // use medic ai
	self->style = 1;// for blaster bolt

	self->monsterinfo.pain_chance = 0.3f;
	self->pain = medic_pain;
	self->die = mymedic_die;
//	self->touch = mymedic_touch;

	self->item = FindItemByClassname("item_adrenaline");

	self->monsterinfo.stand = mymedic_stand;
	self->monsterinfo.walk = medic_walk;
	self->monsterinfo.run = mymedic_run;
	self->monsterinfo.dodge = mymedic_dodge;
	self->monsterinfo.attack = mymedic_attack;
	self->monsterinfo.melee = mymedic_melee;
	self->monsterinfo.sight = mymedic_sight;
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;
//	self->monsterinfo.idle = mymedic_idle;
//	self->monsterinfo.search = mymedic_search;
//	self->monsterinfo.checkattack = mymedic_checkattack;
//	self->monsterinfo.control_cost = 1;
//	self->monsterinfo.cost = 150;

	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;
	//self->monsterinfo.melee = 1;

	//if (self->activator && self->activator->client)
		self->monsterinfo.power_armor_power = M_MEDIC_INITIAL_ARMOR + M_MEDIC_ADDON_ARMOR*self->monsterinfo.level; // pow: medic
	//else self->monsterinfo.power_armor_power = 200 + 40*self->monsterinfo.level;

	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;

	gi.linkentity (self);

	self->monsterinfo.currentmove = &mymedic_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

//	walkmonster_start (self);
	self->nextthink = level.time + 0.1;
	//self->activator->num_monsters += self->monsterinfo.control_cost;
}

void init_drone_medic_commander(edict_t *self)
{
	init_drone_medic(self);

	commander_sound_idle1 = gi.soundindex("medic_commander/medidle.wav");
	commander_sound_pain1 = gi.soundindex("medic_commander/medpain1.wav");
	commander_sound_pain2 = gi.soundindex("medic_commander/medpain2.wav");
	commander_sound_die = gi.soundindex("medic_commander/meddeth.wav");
	commander_sound_sight = gi.soundindex("medic_commander/medsght.wav");
	commander_sound_hook_launch = gi.soundindex("medic_commander/medatck2c.wav");
	commander_sound_hook_hit = gi.soundindex("medic_commander/medatck3a.wav");
	commander_sound_hook_heal = gi.soundindex("medic_commander/medatck4a.wav");
	commander_sound_hook_retract = gi.soundindex("medic_commander/medatck5a.wav");
	commander_sound_spawn = gi.soundindex("medic_commander/monsterspawn1.wav");
	gi.soundindex("tank/tnkatck3.wav");
	gi.modelindex("models/items/spawngro3/tris.md2");

	self->mtype = M_MEDIC_COMMANDER;
	self->s.skinnum = 2;
	self->mass = 600;
	self->yaw_speed = 40;
	self->health = M_MEDIC_COMMANDER_INITIAL_HEALTH + M_MEDIC_COMMANDER_ADDON_HEALTH * self->monsterinfo.level;
	self->max_health = self->health;
	self->monsterinfo.power_armor_power = M_MEDIC_COMMANDER_INITIAL_ARMOR + M_MEDIC_COMMANDER_ADDON_ARMOR * self->monsterinfo.level;
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
	self->monsterinfo.control_cost = M_TANK_CONTROL_COST;
	self->monsterinfo.cost = M_TANK_COST;
	self->monsterinfo.attack = medic_commander_attack;
	self->monsterinfo.melee_finished = level.time + 1.0;
}
