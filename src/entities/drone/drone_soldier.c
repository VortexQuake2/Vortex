/*
==============================================================================

SOLDIER

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_soldier.h"

static int	sound_idle;
static int	sound_sight1;
static int	sound_sight2;
static int	sound_pain_light;
static int	sound_pain;
static int	sound_pain_ss;
static int	sound_death_light;
static int	sound_death;
static int	sound_death_ss;
static int	sound_cock;

static mmove_t m_soldier_move_attack5;
static mmove_t m_soldier_move_trip;
static mmove_t m_soldier_move_duck;

static const int soldier_blaster_flash[] =
{
	MZ2_SOLDIER_BLASTER_1,
	MZ2_SOLDIER_BLASTER_2,
	MZ2_SOLDIER_BLASTER_3,
	MZ2_SOLDIER_BLASTER_4,
	MZ2_SOLDIER_BLASTER_5,
	MZ2_SOLDIER_BLASTER_6,
	MZ2_SOLDIER_BLASTER_7,
	MZ2_SOLDIER_BLASTER_8,
	MZ2_SOLDIER_BLASTER_9
};

static const int soldier_shotgun_flash[] =
{
	MZ2_SOLDIER_SHOTGUN_1,
	MZ2_SOLDIER_SHOTGUN_2,
	MZ2_SOLDIER_SHOTGUN_3,
	MZ2_SOLDIER_SHOTGUN_4,
	MZ2_SOLDIER_SHOTGUN_5,
	MZ2_SOLDIER_SHOTGUN_6,
	MZ2_SOLDIER_SHOTGUN_7,
	MZ2_SOLDIER_SHOTGUN_8,
	MZ2_SOLDIER_SHOTGUN_9
};

static const int soldier_machinegun_flash[] =
{
	MZ2_SOLDIER_MACHINEGUN_1,
	MZ2_SOLDIER_MACHINEGUN_2,
	MZ2_SOLDIER_MACHINEGUN_3,
	MZ2_SOLDIER_MACHINEGUN_4,
	MZ2_SOLDIER_MACHINEGUN_5,
	MZ2_SOLDIER_MACHINEGUN_6,
	MZ2_SOLDIER_MACHINEGUN_7,
	MZ2_SOLDIER_MACHINEGUN_8,
	MZ2_SOLDIER_MACHINEGUN_9
};

static const int soldier_ripper_flash[] =
{
	MZ2_SOLDIER_RIPPER_1,
	MZ2_SOLDIER_RIPPER_2,
	MZ2_SOLDIER_RIPPER_3,
	MZ2_SOLDIER_RIPPER_4,
	MZ2_SOLDIER_RIPPER_5,
	MZ2_SOLDIER_RIPPER_6,
	MZ2_SOLDIER_RIPPER_7,
	MZ2_SOLDIER_RIPPER_8,
	MZ2_SOLDIER_RIPPER_9
};

static const int soldier_hyper_flash[] =
{
	MZ2_SOLDIER_HYPERGUN_1,
	MZ2_SOLDIER_HYPERGUN_2,
	MZ2_SOLDIER_HYPERGUN_3,
	MZ2_SOLDIER_HYPERGUN_4,
	MZ2_SOLDIER_HYPERGUN_5,
	MZ2_SOLDIER_HYPERGUN_6,
	MZ2_SOLDIER_HYPERGUN_7,
	MZ2_SOLDIER_HYPERGUN_8,
	MZ2_SOLDIER_HYPERGUN_9
};

void m_soldier_stand (edict_t *self);
void m_soldier_run (edict_t *self);
void m_soldier_runandshoot_continue (edict_t *self);
void drone_ai_run_slide(edict_t *self, float dist);

static qboolean soldier_uses_light_voice(edict_t *self)
{
	return self->mtype == M_SOLDIER ||
		self->mtype == M_SOLDIER_RIPPER;
}

static int soldier_pain_sound(edict_t *self)
{
	if (soldier_uses_light_voice(self))
		return sound_pain_light;
	if (self->mtype == M_SOLDIERSS)
		return sound_pain;
	return sound_pain_ss;
}

static int soldier_death_sound(edict_t *self)
{
	if (soldier_uses_light_voice(self))
		return sound_death_light;
	if (self->mtype == M_SOLDIERSS)
		return sound_death;
	return sound_death_ss;
}

void m_soldier_idle (edict_t *self)
{
	if (random() > 0.8)
		gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void m_soldier_cock (edict_t *self)
{
	if (self->s.frame == FRAME_stand322)
		gi.sound (self, CHAN_WEAPON, sound_cock, 1, ATTN_IDLE, 0);
	else
		gi.sound (self, CHAN_WEAPON, sound_cock, 1, ATTN_NORM, 0);
}

mframe_t m_soldier_frames_stand1 [] =
{
	drone_ai_stand, 0, m_soldier_idle,
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
	drone_ai_stand, 0, NULL
};
mmove_t m_soldier_move_stand1 = {FRAME_stand101, FRAME_stand130, m_soldier_frames_stand1, m_soldier_stand};

mframe_t m_soldier_frames_stand3 [] =
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
	drone_ai_stand, 0, m_soldier_cock,
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
mmove_t m_soldier_move_stand3 = {FRAME_stand301, FRAME_stand339, m_soldier_frames_stand3, m_soldier_stand};

void m_soldier_stand (edict_t *self)
{
	if (random() > 0.5)
		self->monsterinfo.currentmove = &m_soldier_move_stand1;
	else
		self->monsterinfo.currentmove = &m_soldier_move_stand3;
}

mframe_t m_soldier_frames_run [] =
{
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, NULL
};
mmove_t m_soldier_move_run = {FRAME_run03, FRAME_run08, m_soldier_frames_run, NULL};

static void m_soldier_ai_dodge_slide(edict_t *self, float dist)
{
	if (!G_EntIsAlive(self->enemy) && G_EntIsAlive(self->monsterinfo.attacker))
		self->enemy = self->monsterinfo.attacker;
	if (!G_EntIsAlive(self->enemy))
		return;

	drone_ai_run_slide(self, dist);
}

mframe_t m_soldier_frames_dodge_slide[] =
{
	m_soldier_ai_dodge_slide, 10, NULL,
	m_soldier_ai_dodge_slide, 11, NULL,
	m_soldier_ai_dodge_slide, 11, NULL,
	m_soldier_ai_dodge_slide, 16, NULL,
	m_soldier_ai_dodge_slide, 10, NULL,
	m_soldier_ai_dodge_slide, 15, NULL
};
mmove_t m_soldier_move_dodge_slide = {FRAME_run03, FRAME_run08, m_soldier_frames_dodge_slide, m_soldier_run};

void m_soldier_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		m_soldier_stand(self);
	else
		self->monsterinfo.currentmove = &m_soldier_move_run;
}

static int soldier_flash_from_table(const int *flashes, size_t count, int flash_number)
{
	if (flash_number < 0 || flash_number >= (int)count)
		flash_number = 7;
	return flashes[flash_number];
}

static void soldier_project_raw_flash_origin(edict_t *self, int flash, vec3_t forward, vec3_t start)
{
	vec3_t right;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash], forward, right, start);
}

static void soldier_fireblaster_flash_ex(edict_t* self, int flash, qboolean raw_origin)
{
	int		damage, speed;
	vec3_t	forward, start;

	if (!G_EntExists(self->enemy))
		return;

	damage = M_BLASTER_DMG_BASE + M_BLASTER_DMG_ADDON * drone_damagelevel(self);
	if (damage > M_BLASTER_DMG_MAX && M_BLASTER_DMG_MAX)
		damage = M_BLASTER_DMG_MAX;

	speed = M_BLASTER_SPEED_BASE + M_BLASTER_SPEED_ADDON * drone_damagelevel(self);
	if (M_BLASTER_SPEED_MAX && speed > M_BLASTER_SPEED_MAX)
		speed = M_BLASTER_SPEED_MAX;

	if (raw_origin)
	{
		soldier_project_raw_flash_origin(self, flash, forward, start);
		MonsterAim(self, M_PROJECTILE_ACC, speed, false, -1, forward, start);
	}
	else
		MonsterAim(self, M_PROJECTILE_ACC, speed, false, flash, forward, start);
	monster_fire_blaster(self, start, forward, damage, speed, EF_BLASTER, BLASTER_PROJ_BOLT, 2.0, true, flash);
}

static void soldier_fireblaster_flash(edict_t* self, int flash)
{
	soldier_fireblaster_flash_ex(self, flash, false);
}

void soldier_fireblaster(edict_t* self)
{
	soldier_fireblaster_flash(self, MZ2_SOLDIER_BLASTER_8);
}

static void soldier_firerocket_flash_ex(edict_t* self, int flash, qboolean raw_origin)
{
	int		damage, speed;
	vec3_t	forward, start;

	if (!G_EntExists(self->enemy))
		return;

	damage = M_ROCKETLAUNCHER_DMG_BASE + M_ROCKETLAUNCHER_DMG_ADDON * drone_damagelevel(self);
	if (M_ROCKETLAUNCHER_DMG_MAX && damage > M_ROCKETLAUNCHER_DMG_MAX)
		damage = M_ROCKETLAUNCHER_DMG_MAX;
	speed = M_ROCKETLAUNCHER_SPEED_BASE + M_ROCKETLAUNCHER_SPEED_ADDON * drone_damagelevel(self);
	if (M_ROCKETLAUNCHER_SPEED_MAX && speed > M_ROCKETLAUNCHER_SPEED_MAX)
		speed = M_ROCKETLAUNCHER_SPEED_MAX;

	if (raw_origin)
	{
		soldier_project_raw_flash_origin(self, flash, forward, start);
		MonsterAim(self, M_PROJECTILE_ACC, speed, true, -1, forward, start);
	}
	else
		MonsterAim(self, M_PROJECTILE_ACC, speed, true, flash, forward, start);
	monster_fire_rocket(self, start, forward, damage, speed, flash);
}

static void soldier_firerocket_flash(edict_t* self, int flash)
{
	soldier_firerocket_flash_ex(self, flash, false);
}

void soldier_firerocket(edict_t* self)
{
	soldier_firerocket_flash(self, MZ2_SOLDIER_BLASTER_8);
}

static void soldier_fireshotgun_flash(edict_t* self, int flash)
{
	int		damage;
	vec3_t	forward, start;

	if (!G_EntExists(self->enemy))
		return;

	damage = M_SHOTGUN_DMG_BASE + M_SHOTGUN_DMG_ADDON * drone_damagelevel(self);
	if (M_SHOTGUN_DMG_MAX && damage > M_SHOTGUN_DMG_MAX)
		damage = M_SHOTGUN_DMG_MAX;
	MonsterAim(self, M_HITSCAN_INSTANT_ACC, 0, false, flash, forward, start);
	monster_fire_shotgun(self, start, forward, damage, 15, 375, 375, 10, flash);
}

void soldier_fireshotgun(edict_t* self)
{
	soldier_fireshotgun_flash(self, MZ2_SOLDIER_SHOTGUN_8);
}

static void soldier_fireionripper_ex(edict_t* self, int flash_number, qboolean raw_origin)
{
	int		damage, speed;
	int		flash;
	vec3_t	forward, start;

	if (!G_EntExists(self->enemy))
		return;

	flash = soldier_flash_from_table(soldier_ripper_flash,
		sizeof(soldier_ripper_flash) / sizeof(soldier_ripper_flash[0]), flash_number);
	damage = IONRIPPER_INITIAL_DAMAGE + IONRIPPER_ADDON_DAMAGE * drone_damagelevel(self);
	speed = IONRIPPER_INITIAL_SPEED + IONRIPPER_ADDON_SPEED * drone_damagelevel(self);

	if (raw_origin)
	{
		soldier_project_raw_flash_origin(self, flash, forward, start);
		MonsterAim(self, M_PROJECTILE_ACC, speed, false, -1, forward, start);
	}
	else
		MonsterAim(self, M_PROJECTILE_ACC, speed, false, flash, forward, start);
	monster_fire_ionripper(self, start, forward, damage, speed, EF_IONRIPPER, flash);
}

void soldier_fireionripper(edict_t* self, int flash_number)
{
	soldier_fireionripper_ex(self, flash_number, false);
}

static void soldier_fireblueblaster_ex(edict_t* self, int flash_number, qboolean raw_origin)
{
	int		damage, speed;
	int		flash;
	vec3_t	forward, start;

	if (!G_EntExists(self->enemy))
		return;

	flash = soldier_flash_from_table(soldier_hyper_flash,
		sizeof(soldier_hyper_flash) / sizeof(soldier_hyper_flash[0]), flash_number);
	damage = M_HYPERBLASTER_DMG_BASE + M_HYPERBLASTER_DMG_ADDON * drone_damagelevel(self);
	if (M_HYPERBLASTER_DMG_MAX && damage > M_HYPERBLASTER_DMG_MAX)
		damage = M_HYPERBLASTER_DMG_MAX;

	speed = HYPERBLASTER_INITIAL_SPEED + HYPERBLASTER_ADDON_SPEED * drone_damagelevel(self);
	if (speed < 400)
		speed = 400;

	if (raw_origin)
	{
		soldier_project_raw_flash_origin(self, flash, forward, start);
		MonsterAim(self, M_PROJECTILE_ACC, speed, false, -1, forward, start);
	}
	else
		MonsterAim(self, M_PROJECTILE_ACC, speed, false, flash, forward, start);
	monster_fire_blueblaster(self, start, forward, damage, speed, EF_BLUEHYPERBLASTER, flash);
}

void soldier_fireblueblaster(edict_t* self, int flash_number)
{
	soldier_fireblueblaster_ex(self, flash_number, false);
}

static void soldier_laser_update(edict_t *laser)
{
	edict_t *self;
	vec3_t forward, right, up, start, offset, target;
	qboolean do_damage;

	self = laser->owner;
	if (!self || !self->inuse || !G_EntExists(self->enemy))
	{
		laser->spawnflags |= DABEAM_SPAWNED;
		return;
	}

	if (self->radius_dmg <= 0 || self->radius_dmg > MZ2_LAST)
		self->radius_dmg = MZ2_SOLDIER_MACHINEGUN_4;

	AngleVectors(self->s.angles, forward, right, up);
	VectorCopy(self->s.origin, start);
	VectorCopy(monster_flash_offset[self->radius_dmg], offset);
	VectorMA(start, offset[0], forward, start);
	VectorMA(start, offset[1], right, start);
	VectorMA(start, offset[2] + 6, up, start);

	G_EntMidPoint(self->enemy, target);
	VectorSubtract(target, start, forward);
	VectorNormalize(forward);

	VectorCopy(start, laser->s.origin);
	VectorCopy(forward, laser->movedir);

	do_damage = !(laser->spawnflags & DABEAM_SPAWNED);
	dabeam_update(laser, do_damage);
	laser->spawnflags |= DABEAM_SPAWNED;
}

void soldier_firelaser(edict_t* self, int flash_number)
{
	int damage;

	if (!G_EntExists(self->enemy))
		return;

	self->radius_dmg = flash_number;
	damage = M_DABEAM_DMG_BASE + M_DABEAM_DMG_ADDON * drone_damagelevel(self);
		if (M_DABEAM_DMG_MAX && damage > M_DABEAM_DMG_MAX)
		damage = M_DABEAM_DMG_MAX;
	monster_fire_dabeam(self, damage, false, soldier_laser_update);
}

void m_soldier_fire (edict_t *self)
{
	if (!G_EntExists(self->enemy))
		return;

	if (self->mtype == M_SOLDIER)
		soldier_fireblaster(self);
	else if (self->mtype == M_SOLDIERLT)
		soldier_firerocket(self);
	else if (self->mtype == M_SOLDIERSS)
		soldier_fireshotgun(self);
	else if (self->mtype == M_SOLDIER_RIPPER)
		soldier_fireionripper(self, 7);
	else if (self->mtype == M_SOLDIER_BLUEBLASTER)
		soldier_fireblueblaster(self, 7);
	else if (self->mtype == M_SOLDIER_LASER)
		soldier_firelaser(self, MZ2_SOLDIER_MACHINEGUN_4);
}

void m_soldier_hyperripper_run_fire(edict_t *self)
{
	if (self->mtype == M_SOLDIER_RIPPER)
		soldier_fireionripper(self, 7);
	else if (self->mtype == M_SOLDIER_BLUEBLASTER)
		soldier_fireblueblaster(self, 7);
}

mframe_t m_soldier_frames_runandshoot [] =
{
	drone_ai_run, 25, NULL,	//109
	drone_ai_run,  25, NULL,
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, m_soldier_fire, //112
	drone_ai_run, 25, m_soldier_hyperripper_run_fire,
	drone_ai_run, 25, m_soldier_hyperripper_run_fire,
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, m_soldier_fire,	//117
	drone_ai_run,  25, NULL,
	drone_ai_run, 25, m_soldier_cock,	//119
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, NULL,
	drone_ai_run, 25, m_soldier_runandshoot_continue	//122
};
mmove_t m_soldier_move_runandshoot = {FRAME_runs01, FRAME_runs14, m_soldier_frames_runandshoot, NULL};

void m_soldier_runandshoot_continue (edict_t* self)
{
	if (M_ContinueAttack(self, &m_soldier_move_runandshoot, NULL, 0, 512, 0.9))
		return;

	// end attack
	self->monsterinfo.currentmove = &m_soldier_move_run;
}

void m_soldier_runandshoot (edict_t* self)
{
	self->monsterinfo.currentmove = &m_soldier_move_runandshoot;
}

void m_soldier_attack1_refire1 (edict_t* self)
{
	// continue firing if the enemy is still close, or we are standing ground
	if (G_ValidTarget(self, self->enemy, true, true) && (random() <= 0.9)
		&& ((entdist(self, self->enemy) <= 512) || (self->monsterinfo.aiflags & AI_STAND_GROUND)))
		self->s.frame = FRAME_attak102;

	M_DelayNextAttack(self, 0, true);
}

void m_soldier_endattack1 (edict_t* self)
{
	if (entdist(self, self->enemy) > 128 && !(self->monsterinfo.aiflags & AI_STAND_GROUND))
		M_DelayNextAttack(self, (GetRandom(10, 20) * FRAMETIME), false);
	m_soldier_run(self);
}

mframe_t m_soldier_frames_attack1 [] =
{
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  m_soldier_fire,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  m_soldier_attack1_refire1,
	ai_charge, 0,  NULL,
	ai_charge, 0,  m_soldier_cock,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL
};
mmove_t m_soldier_move_attack1 = {FRAME_attak101, FRAME_attak112, m_soldier_frames_attack1, m_soldier_endattack1};

void m_soldier_endattack_laser(edict_t* self)
{
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	self->monsterinfo.pausetime = 0;
	if (self->beam && self->beam->inuse)
	{
		self->beam->prethink = NULL;
		self->beam->nextthink = level.time + FRAMETIME;
	}
	m_soldier_run(self);
}

void m_soldier_firelaser_frame(edict_t* self)
{
	if (!G_EntExists(self->enemy) || !visible(self, self->enemy))
	{
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		return;
	}

	if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
		self->monsterinfo.pausetime = level.time + 0.3 + random() * 0.8;

	soldier_firelaser(self, MZ2_SOLDIER_MACHINEGUN_4);

	if (level.time < self->monsterinfo.pausetime)
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
	else
	{
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		self->monsterinfo.pausetime = 0;
		if (self->beam && self->beam->inuse)
		{
			self->beam->prethink = NULL;
			self->beam->nextthink = level.time + FRAMETIME;
		}
	}
}

mframe_t m_soldier_frames_attack_laser[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, m_soldier_firelaser_frame,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t m_soldier_move_attack_laser = {FRAME_attak401, FRAME_attak406, m_soldier_frames_attack_laser, m_soldier_endattack_laser};

void m_soldier_attack(edict_t* self)
{
	if (self->mtype == M_SOLDIER_LASER)
	{
		self->monsterinfo.currentmove = &m_soldier_move_attack_laser;
		M_DelayNextAttack(self, 0, true);
		return;
	}

	if ((self->mtype == M_SOLDIER_RIPPER || self->mtype == M_SOLDIER_BLUEBLASTER)
		&& !(self->monsterinfo.aiflags & AI_STAND_GROUND))
	{
		self->monsterinfo.currentmove = &m_soldier_move_runandshoot;
		M_DelayNextAttack(self, 0, true);
		return;
	}

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.currentmove = &m_soldier_move_attack1;
		return;
	}

	if ((entdist(self, self->enemy) < 128) && (random() <= 0.8))
		self->monsterinfo.currentmove = &m_soldier_move_attack1;
	else
		self->monsterinfo.currentmove = &m_soldier_move_runandshoot;

	M_DelayNextAttack(self, 0, true);
}

void m_soldier_duck_down (edict_t *self)
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

void m_soldier_duck_up (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] = 32;
	self->takedamage = DAMAGE_AIM;
	VectorClear(self->velocity);
	gi.linkentity (self);
}

void m_soldier_duck_hold (edict_t *self)
{
	if (self->monsterinfo.pausetime > level.time)
		self->monsterinfo.nextframe = self->s.frame;
}

static void m_soldier_start_duck_dodge(edict_t *self, float hold_time)
{
	self->monsterinfo.nextattack = 0;
	self->monsterinfo.pausetime = level.time + hold_time;
	self->monsterinfo.currentmove = &m_soldier_move_duck;
	m_soldier_duck_down(self);
	self->monsterinfo.dodge_time = level.time + 1.25f;
}

mframe_t m_soldier_frames_duck [] =
{
	ai_move, 5, m_soldier_duck_down,
	ai_move, -1, m_soldier_duck_hold,
	ai_move, 1, NULL,
	ai_move, 0, m_soldier_duck_up,
	ai_move, 5, NULL
};
static mmove_t m_soldier_move_duck = {FRAME_duck01, FRAME_duck05, m_soldier_frames_duck, m_soldier_run};

void m_soldier_jump_takeoff (edict_t *self)
{
	vec3_t	v;

	gi.sound (self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0);
	VectorSubtract(self->monsterinfo.dir, self->s.origin, v);
	v[2] = 0;
	VectorNormalize(v);
	VectorScale(v, -200, self->velocity);
	self->velocity[2] = 400;
	self->monsterinfo.pausetime = level.time + 2.0; // maximum duration of jump
}

void m_soldier_jump_forward_takeoff(edict_t *self, float forward_velocity, float up_velocity)
{
	vec3_t forward;

	gi.sound(self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0);
	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorScale(forward, forward_velocity, self->velocity);
	self->velocity[2] = up_velocity;
	self->groundentity = NULL;
	self->monsterinfo.pausetime = level.time + 2.0;
}

void m_soldier_jump_attack_takeoff(edict_t *self)
{
	m_soldier_jump_forward_takeoff(self, 250, 250);
}

void m_soldier_jump2_takeoff(edict_t *self)
{
	m_soldier_jump_forward_takeoff(self, 200, 350);
}

void m_soldier_jump_hold (edict_t *self)
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

mframe_t m_soldier_frames_jump [] =
{
	ai_move, 0, m_soldier_jump2_takeoff,
	ai_move, 0,	m_soldier_jump_hold,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL
};
mmove_t m_soldier_move_jump = {FRAME_duck01, FRAME_duck05, m_soldier_frames_jump, m_soldier_run};

static qboolean m_soldier_prone_shoot_ok(edict_t *self)
{
	vec3_t forward, diff;

	if (!G_EntIsAlive(self->enemy))
		return false;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(self->enemy->s.origin, self->s.origin, diff);
	diff[2] = 0;
	if (VectorNormalize(diff) == 0)
		return false;

	return DotProduct(forward, diff) >= 0.80f;
}

static void m_soldier_stand_up(edict_t *self)
{
	if (self->monsterinfo.nextattack > 0 && m_soldier_prone_shoot_ok(self))
	{
		self->monsterinfo.nextattack--;
		self->monsterinfo.currentmove = &m_soldier_move_attack5;
		self->monsterinfo.nextframe = FRAME_attak504;
		return;
	}

	self->monsterinfo.nextattack = 0;
	self->monsterinfo.currentmove = &m_soldier_move_trip;
	self->monsterinfo.nextframe = FRAME_runt08;
}

static void m_soldier_ai_prone_move(edict_t *self, float dist)
{
	ai_move(self, dist);

	if (!m_soldier_prone_shoot_ok(self))
		m_soldier_stand_up(self);
}

static void m_soldier_fire_prone(edict_t *self)
{
	const int prone_flash = 8;

	if (self->mtype == M_SOLDIER)
		soldier_fireblaster_flash_ex(self, soldier_flash_from_table(soldier_blaster_flash,
			sizeof(soldier_blaster_flash) / sizeof(soldier_blaster_flash[0]), prone_flash), true);
	else if (self->mtype == M_SOLDIERLT)
		soldier_firerocket_flash_ex(self, soldier_flash_from_table(soldier_blaster_flash,
			sizeof(soldier_blaster_flash) / sizeof(soldier_blaster_flash[0]), prone_flash), true);
	else if (self->mtype == M_SOLDIERSS)
		soldier_fireshotgun_flash(self, soldier_flash_from_table(soldier_shotgun_flash,
			sizeof(soldier_shotgun_flash) / sizeof(soldier_shotgun_flash[0]), prone_flash));
	else if (self->mtype == M_SOLDIER_RIPPER)
		soldier_fireionripper_ex(self, prone_flash, true);
	else if (self->mtype == M_SOLDIER_BLUEBLASTER)
		soldier_fireblueblaster_ex(self, prone_flash, true);
	else if (self->mtype == M_SOLDIER_LASER)
		soldier_firelaser(self, soldier_flash_from_table(soldier_machinegun_flash,
			sizeof(soldier_machinegun_flash) / sizeof(soldier_machinegun_flash[0]), prone_flash));
}

static void m_soldier_start_prone_dodge(edict_t *self)
{
	self->monsterinfo.nextattack = GetRandom(1, 2);
	self->monsterinfo.currentmove = &m_soldier_move_attack5;
	m_soldier_duck_down(self);
	self->monsterinfo.dodge_time = level.time + 2.25f;
}

mframe_t m_soldier_frames_attack5 [] =
{
	ai_move, 18, m_soldier_duck_down,
	ai_move, 11, NULL,
	ai_move, 0, NULL,
	m_soldier_ai_prone_move, 0, NULL,
	m_soldier_ai_prone_move, 0, NULL,
	m_soldier_ai_prone_move, 0, m_soldier_fire_prone,
	m_soldier_ai_prone_move, 0, m_soldier_fire_prone,
	m_soldier_ai_prone_move, 0, m_soldier_fire_prone
};
static mmove_t m_soldier_move_attack5 = {FRAME_attak501, FRAME_attak508, m_soldier_frames_attack5, m_soldier_stand_up};

static void m_soldier_check_prone(edict_t *self)
{
	if (m_soldier_prone_shoot_ok(self))
		self->monsterinfo.currentmove = &m_soldier_move_attack5;
}

mframe_t m_soldier_frames_trip [] =
{
	ai_move, 10, NULL,
	ai_move, 2, m_soldier_check_prone,
	ai_move, 18, m_soldier_duck_down,
	ai_move, 11, NULL,
	ai_move, 9, NULL,
	ai_move, -11, NULL,
	ai_move, -2, NULL,
	ai_move, 0, NULL,
	ai_move, 6, NULL,
	ai_move, -5, NULL,
	ai_move, 0, NULL,
	ai_move, 1, NULL,
	ai_move, 0, NULL,
	ai_move, 0, m_soldier_duck_up,
	ai_move, 3, NULL,
	ai_move, 2, NULL,
	ai_move, -1, NULL,
	ai_move, 2, NULL,
	ai_move, 0, NULL
};

static void m_soldier_end_trip(edict_t *self)
{
	self->monsterinfo.nextattack = 0;
	m_soldier_run(self);
}

static mmove_t m_soldier_move_trip = {FRAME_runt01, FRAME_runt19, m_soldier_frames_trip, m_soldier_end_trip};

void m_soldier_jump (edict_t *self)
{
	if (self->groundentity)
	{
		if (random() < 0.5 && m_soldier_prone_shoot_ok(self))
			m_soldier_start_prone_dodge(self);
		else
		{
			self->monsterinfo.nextattack = random() < 0.5f ? 1 : 0;
			self->monsterinfo.currentmove = &m_soldier_move_trip;
		}
	}
}

static qboolean m_soldier_is_dodge_move(edict_t *self)
{
	return self->monsterinfo.currentmove == &m_soldier_move_duck ||
		self->monsterinfo.currentmove == &m_soldier_move_jump ||
		self->monsterinfo.currentmove == &m_soldier_move_attack5 ||
		self->monsterinfo.currentmove == &m_soldier_move_trip ||
		self->monsterinfo.currentmove == &m_soldier_move_dodge_slide;
}

static qboolean m_soldier_dodge_hit_low(edict_t *self, vec3_t dir)
{
	const float duck_height = self->absmax[2] - 33;

	return dir[2] > self->absmin[2] && dir[2] <= duck_height;
}

void m_soldier_dodge (edict_t *self, edict_t *attacker, vec3_t dir, int radius)
{
	if (random() > 0.9)
		return;
	if (level.time < self->monsterinfo.dodge_time)
		return;
	if (!attacker)
		return;
	if (OnSameTeam(self, attacker))
		return;
	if (m_soldier_is_dodge_move(self))
		return;

	self->monsterinfo.attacker = attacker;
	if (!G_EntIsAlive(self->enemy) && G_EntIsAlive(attacker))
		self->enemy = attacker;
	if (!radius)
	{
		if (!m_soldier_dodge_hit_low(self, dir))
		{
			if (self->groundentity && m_soldier_prone_shoot_ok(self) &&
				(self->monsterinfo.currentmove == &m_soldier_move_runandshoot || random() < 0.45f))
			{
				m_soldier_start_prone_dodge(self);
			}
			else
			{
				m_soldier_start_duck_dodge(self, 1.10f);
			}
			return;
		}

		if (!(self->monsterinfo.aiflags & AI_STAND_GROUND))
		{
			self->monsterinfo.nextattack = 0;
			self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
			self->monsterinfo.currentmove = &m_soldier_move_dodge_slide;
			self->monsterinfo.dodge_time = level.time + 1.35f;
		}
		else
		{
			m_soldier_start_duck_dodge(self, 1.10f);
		}
	}
	else
	{
		m_soldier_jump(self);
		self->monsterinfo.dodge_time = level.time + 2.5;
	}
}

void m_soldier_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity (self);
	M_PrepBodyRemoval(self);
}

static void m_soldier_death_shrink(edict_t *self)
{
	self->svflags |= SVF_DEADMONSTER;
	self->maxs[2] = 0;
	gi.linkentity(self);
}


mframe_t soldier_frames_pain_short1[] =
{
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0, NULL,
	ai_move, 0,	 NULL,
};
mmove_t soldier_move_pain_short1 = { FRAME_pain101, FRAME_pain105, soldier_frames_pain_short1, m_soldier_run };

mframe_t soldier_frames_pain_short2[] =
{
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,

	ai_move, 0,  NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
};
mmove_t soldier_move_pain_short2 = { FRAME_pain201, FRAME_pain207, soldier_frames_pain_short2, m_soldier_run };

mframe_t soldier_frames_pain_long1[] =
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
	ai_move, 0,	 NULL,

	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
};
mmove_t soldier_move_pain_long1 = { FRAME_pain301, FRAME_pain318, soldier_frames_pain_long1, m_soldier_run };

mframe_t soldier_frames_pain_long2[] =
{
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
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,

	ai_move, 0,	 NULL,
};
mmove_t soldier_move_pain_long2 = { FRAME_pain401, FRAME_pain417, soldier_frames_pain_long2, m_soldier_run };

void soldier_pain(edict_t* self, edict_t* other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;
	else
		self->s.skinnum &= ~1;

	// we're already in a pain state
	if (self->monsterinfo.currentmove == &soldier_move_pain_long1 ||
		self->monsterinfo.currentmove == &soldier_move_pain_long2 ||
		self->monsterinfo.currentmove == &soldier_move_pain_short1 || 
		self->monsterinfo.currentmove == &soldier_move_pain_short2 ||
		m_soldier_is_dodge_move(self))
		return;

	// monster players don't get pain state induced
	if (G_GetClient(self))
		return;

	// no pain in invasion hard mode
	if (invasion->value == 2)
		return;

	if (level.time >= self->pain_debounce_time)
	{
		gi.sound(self, CHAN_VOICE, soldier_pain_sound(self), 1, ATTN_NORM, 0);
		self->pain_debounce_time = level.time + 0.5;
	}

	// if we're fidgeting, always go into pain state.
	if (random() <= (1.0f - self->monsterinfo.pain_chance) &&
		self->monsterinfo.currentmove != &m_soldier_move_stand1 &&
		self->monsterinfo.currentmove != &m_soldier_move_stand3)
		return;

	if (self->monsterinfo.currentmove == &m_soldier_move_stand1 ||
		self->monsterinfo.currentmove == &m_soldier_move_stand3) {
		if (random() < 0.5)
			self->monsterinfo.currentmove = &soldier_move_pain_long1;
		else
			self->monsterinfo.currentmove = &soldier_move_pain_long2;
	}
	else {
		if (random() < 0.5)
			self->monsterinfo.currentmove = &soldier_move_pain_short1;
		else
			self->monsterinfo.currentmove = &soldier_move_pain_short2;
	}
}

mframe_t m_soldier_frames_death1 [] =
{
	ai_move, 0,   NULL,
	ai_move, -10, NULL,
	ai_move, -10, NULL,
	ai_move, -10, m_soldier_death_shrink,
	ai_move, -5,  NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   m_soldier_fire,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   m_soldier_fire,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};
mmove_t m_soldier_move_death1 = {FRAME_death101, FRAME_death136, m_soldier_frames_death1, m_soldier_dead};

mframe_t m_soldier_frames_death2 [] =
{
	ai_move, -5,  NULL,
	ai_move, -5,  NULL,
	ai_move, -5,  NULL,
	ai_move, 0,   m_soldier_death_shrink,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};
mmove_t m_soldier_move_death2 = {FRAME_death201, FRAME_death235, m_soldier_frames_death2, m_soldier_dead};

mframe_t m_soldier_frames_death3 [] =
{
	ai_move, -5,  NULL,
	ai_move, -5,  NULL,
	ai_move, -5,  NULL,
	ai_move, 0,   m_soldier_death_shrink,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
};
mmove_t m_soldier_move_death3 = {FRAME_death301, FRAME_death345, m_soldier_frames_death3, m_soldier_dead};

mframe_t m_soldier_frames_death4 [] =
{
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   m_soldier_death_shrink,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};
mmove_t m_soldier_move_death4 = {FRAME_death401, FRAME_death453, m_soldier_frames_death4, m_soldier_dead};

mframe_t m_soldier_frames_death5 [] =
{
	ai_move, -5,  NULL,
	ai_move, -5,  NULL,
	ai_move, -5,  NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   m_soldier_death_shrink,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};
mmove_t m_soldier_move_death5 = {FRAME_death501, FRAME_death524, m_soldier_frames_death5, m_soldier_dead};

mframe_t m_soldier_frames_death6 [] =
{
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   m_soldier_death_shrink,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};
mmove_t m_soldier_move_death6 = {FRAME_death601, FRAME_death610, m_soldier_frames_death6, m_soldier_dead};

void m_soldier_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

	// notify the owner that the monster is dead
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

	// regular death
	gi.sound (self, CHAN_VOICE, soldier_death_sound(self), 1, ATTN_NORM, 0);
	self->takedamage = DAMAGE_YES;
	self->deadflag = DEAD_DEAD;
	vrx_update_drone_death_skin(self);

	if (self->monsterinfo.currentmove == &m_soldier_move_trip ||
		self->monsterinfo.currentmove == &m_soldier_move_attack5)
	{
		self->monsterinfo.currentmove = &m_soldier_move_death4;
		self->monsterinfo.nextframe = FRAME_death413;
		m_soldier_death_shrink(self);
	}
	else
	{
		n = GetRandom(1, 6);
		switch (n)
		{
		case 1: self->monsterinfo.currentmove = &m_soldier_move_death1; break;
		case 2: self->monsterinfo.currentmove = &m_soldier_move_death2; break;
		case 3: self->monsterinfo.currentmove = &m_soldier_move_death3; break;
		case 4: self->monsterinfo.currentmove = &m_soldier_move_death4; break;
		case 5: self->monsterinfo.currentmove = &m_soldier_move_death5; break;
		case 6: self->monsterinfo.currentmove = &m_soldier_move_death6; break;
		}
	}

	DroneList_Remove(self);

	if (self->activator && !self->activator->client)
	{
		self->activator->num_monsters_real--;
		// gi.bprintf(PRINT_HIGH, "releasing %p (%d)\n", self, self->activator->num_monsters_real);
	}
}

void init_drone_soldier (edict_t *self)
{
	// NOTE: monster's pain, think, and touch functions are set-up elsewhere

	self->s.modelindex = gi.modelindex ("models/monsters/soldier/tris.md2");
	self->monsterinfo.scale = MODEL_SCALE;
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 32);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	sound_idle =	gi.soundindex ("soldier/solidle1.wav");
	sound_sight1 =	gi.soundindex ("soldier/solsght1.wav");
	sound_sight2 =	gi.soundindex ("soldier/solsrch1.wav");
	sound_cock =	gi.soundindex ("infantry/infatck3.wav");
	sound_pain_light = gi.soundindex ("soldier/solpain2.wav");
	sound_pain = gi.soundindex ("soldier/solpain1.wav");
	sound_pain_ss = gi.soundindex ("soldier/solpain3.wav");
	sound_death_light = gi.soundindex ("soldier/soldeth2.wav");
	sound_death = gi.soundindex ("soldier/soldeth1.wav");
	sound_death_ss = gi.soundindex ("soldier/soldeth3.wav");

	self->mass = 100;

	//4.2 don't override previous mtype
	if (!self->mtype)
		self->mtype = M_SOLDIER;

	self->monsterinfo.control_cost = M_SOLDIERLT_CONTROL_COST;
	self->monsterinfo.cost = 50;

	// set health
	if (self->mtype == M_SOLDIER_RIPPER)
		self->health = M_SOLDIER_RIPPER_INITIAL_HEALTH + M_SOLDIER_RIPPER_ADDON_HEALTH * self->monsterinfo.level;
	else if (self->mtype == M_SOLDIER_BLUEBLASTER)
		self->health = M_SOLDIER_BLUEBLASTER_INITIAL_HEALTH + M_SOLDIER_BLUEBLASTER_ADDON_HEALTH * self->monsterinfo.level;
	else if (self->mtype == M_SOLDIER_LASER)
		self->health = M_SOLDIER_LASER_INITIAL_HEALTH + M_SOLDIER_LASER_ADDON_HEALTH * self->monsterinfo.level;
	else
		self->health = M_SOLDIER_INITIAL_HEALTH+M_SOLDIER_ADDON_HEALTH*self->monsterinfo.level; // hlt: soldier
	self->max_health = self->health;
	self->gib_health = -1.5 * BASE_GIB_HEALTH;

	// set armor
	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	if (self->mtype == M_SOLDIER_RIPPER)
		self->monsterinfo.power_armor_power = M_SOLDIER_RIPPER_INITIAL_ARMOR + M_SOLDIER_RIPPER_ADDON_ARMOR * self->monsterinfo.level;
	else if (self->mtype == M_SOLDIER_BLUEBLASTER)
		self->monsterinfo.power_armor_power = M_SOLDIER_BLUEBLASTER_INITIAL_ARMOR + M_SOLDIER_BLUEBLASTER_ADDON_ARMOR * self->monsterinfo.level;
	else if (self->mtype == M_SOLDIER_LASER)
		self->monsterinfo.power_armor_power = M_SOLDIER_LASER_INITIAL_ARMOR + M_SOLDIER_LASER_ADDON_ARMOR * self->monsterinfo.level;
	else
		self->monsterinfo.power_armor_power = M_SOLDIER_INITIAL_ARMOR+M_SOLDIER_ADDON_ARMOR*self->monsterinfo.level; // pow: soldier
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;

	if (self->mtype == M_SOLDIER_RIPPER)
	{
		self->s.skinnum = 6;
		self->count = self->s.skinnum - 6;
	}
	else if (self->mtype == M_SOLDIER_BLUEBLASTER)
	{
		self->s.skinnum = 8;
		self->count = self->s.skinnum - 6;
	}
	else if (self->mtype == M_SOLDIER_LASER)
	{
		self->s.skinnum = 10;
		self->count = self->s.skinnum - 6;
	}

	// set AI jump parameters
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.jumpup = 64;

	// extremely sensitive to pain!
	self->monsterinfo.pain_chance = 0.4f;
	self->pain = soldier_pain;
	self->die = m_soldier_die;

	self->monsterinfo.stand = m_soldier_stand;
	self->monsterinfo.run = m_soldier_run;
	self->monsterinfo.dodge = m_soldier_dodge;
	self->monsterinfo.attack = m_soldier_attack;

	gi.linkentity (self);
	self->nextthink = level.time + FRAMETIME;
	self->monsterinfo.currentmove = &m_soldier_move_stand1;
}
