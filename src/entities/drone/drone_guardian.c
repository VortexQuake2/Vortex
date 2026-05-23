/*
==============================================================================

GUARDIAN / miniguardian

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_guardian.h"

static int sound_step;
static int sound_charge;
static int sound_spin_loop;
static int sound_laser;
static int sound_pew;
static int sound_pain;

static constexpr float GUARDIAN_INVASION_SCALE = 0.45f;
static constexpr float GUARDIAN_INVASION_MOVE_SCALE = 1.75f;
static constexpr float GUARDIAN_SIGHT_ACK_CHANCE = 0.30f;

void guardian_run(edict_t *self);

static float guardian_scale(edict_t *self)
{
	return (self->s.scale > 0.0f) ? self->s.scale : 1.0f;
}

static void guardian_ai_walk(edict_t *self, float dist)
{
	drone_ai_walk(self, invasion->value ? dist * GUARDIAN_INVASION_MOVE_SCALE : dist);
}

static void guardian_ai_run(edict_t *self, float dist)
{
	drone_ai_run(self, invasion->value ? dist * GUARDIAN_INVASION_MOVE_SCALE : dist);
}

static void guardian_project_flash(edict_t *self, int flash, vec3_t forward, vec3_t right, vec3_t start)
{
	vec3_t offset;

	VectorCopy(monster_flash_offset[flash], offset);
	if (guardian_scale(self) != 1.0f)
		VectorScale(offset, guardian_scale(self), offset);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
}

static void guardian_project_laser_frame_origin(edict_t *self, vec3_t forward, vec3_t right, vec3_t start)
{
	vec3_t offset;

	if (self->s.frame & 1)
		VectorSet(offset, 125, -70, 60);
	else
		VectorSet(offset, 112, -62, 60);

	if (guardian_scale(self) != 1.0f)
		VectorScale(offset, guardian_scale(self), offset);

	G_ProjectSource(self->s.origin, offset, forward, right, start);
}

static void guardian_project_laser_origin(edict_t *self, qboolean secondary, vec3_t forward, vec3_t right, vec3_t start)
{
	vec3_t offset;

	if (secondary)
		VectorSet(offset, 112, -62, 60);
	else
		VectorSet(offset, 125, -70, 60);

	if (guardian_scale(self) != 1.0f)
		VectorScale(offset, guardian_scale(self), offset);

	G_ProjectSource(self->s.origin, offset, forward, right, start);
}

static qboolean guardian_has_origin_shot(edict_t *self, vec3_t start)
{
	return M_MonsterHasClearShotFrom(self, start);
}

static qboolean guardian_has_laser_shot(edict_t *self, qboolean secondary)
{
	vec3_t forward, right, start;

	AngleVectors(self->s.angles, forward, right, NULL);
	guardian_project_laser_origin(self, secondary, forward, right, start);
	return guardian_has_origin_shot(self, start);
}

static qboolean guardian_has_blaster_shot(edict_t *self)
{
	if (self->mtype == M_MINIGUARDIAN)
		return M_MonsterHasClearShotFromFlash(self, MZ2_SOLDIER_RIPPER_8);

	return M_MonsterHasClearShotFromFlash(self, MZ2_GUARDIAN_BLASTER);
}

static qboolean guardian_has_grenade_or_laser_shot(edict_t *self)
{
	if (self->mtype == M_MINIGUARDIAN)
		return guardian_has_laser_shot(self, false) || guardian_has_laser_shot(self, true);

	return guardian_has_laser_shot(self, false) || guardian_has_laser_shot(self, true);
}

static qboolean guardian_has_rocket_shot(edict_t *self, float offset)
{
	vec3_t forward, right, up, start;

	AngleVectors(self->s.angles, forward, right, up);
	VectorCopy(self->s.origin, start);
	VectorMA(start, -8 * guardian_scale(self), forward, start);
	VectorMA(start, offset * guardian_scale(self), right, start);
	VectorMA(start, 50 * guardian_scale(self), up, start);
	return guardian_has_origin_shot(self, start);
}

static void guardian_footstep(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step, 1, ATTN_NORM, 0);
}

static void guardian_sight(edict_t *self, edict_t *other)
{
	(void)other;

	if (random() < GUARDIAN_SIGHT_ACK_CHANCE)
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
}

mframe_t guardian_frames_stand[] =
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
	drone_ai_stand, 0, NULL
};
mmove_t guardian_move_stand = {FRAME_idle1, FRAME_idle52, guardian_frames_stand, NULL};

void guardian_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &guardian_move_stand;
}

mframe_t guardian_frames_walk[] =
{
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, guardian_footstep,
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, NULL,
	guardian_ai_walk, 8, guardian_footstep,
	guardian_ai_walk, 8, NULL
};
mmove_t guardian_move_walk = {FRAME_walk1, FRAME_walk19, guardian_frames_walk, NULL};

void guardian_walk(edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &guardian_move_walk;
}

mframe_t guardian_frames_run[] =
{
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, guardian_footstep,
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, NULL,
	guardian_ai_run, 8, guardian_footstep,
	guardian_ai_run, 8, NULL
};
mmove_t guardian_move_run = {FRAME_walk1, FRAME_walk19, guardian_frames_run, NULL};

void guardian_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &guardian_move_stand;
	else
		self->monsterinfo.currentmove = &guardian_move_run;
}

mframe_t guardian_frames_pain1[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t guardian_move_pain1 = {FRAME_pain1_1, FRAME_pain1_8, guardian_frames_pain1, guardian_run};

void guardian_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	(void)other;
	(void)kick;

	if (level.time < self->pain_debounce_time)
		return;
	if (damage <= 75 && random() > 0.2)
		return;
	if ((self->s.frame >= FRAME_atk1_spin1 && self->s.frame <= FRAME_atk1_spin15) ||
		(self->s.frame >= FRAME_atk2_fire1 && self->s.frame <= FRAME_atk2_fire4) ||
		(self->s.frame >= FRAME_kick_in1 && self->s.frame <= FRAME_kick_in13))
		return;
	if (G_GetClient(self) || invasion->value == 2)
		return;

	self->pain_debounce_time = level.time + 3.0;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &guardian_move_pain1;
	self->s.sound = 0;
}

mframe_t guardian_frames_atk1_out[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t guardian_move_atk1_out = {FRAME_atk1_out1, FRAME_atk1_out3, guardian_frames_atk1_out, guardian_run};

void guardian_atk1_finish(edict_t *self)
{
	self->monsterinfo.currentmove = &guardian_move_atk1_out;
	self->s.sound = 0;
}

void guardian_atk1_charge(edict_t *self)
{
	self->s.sound = sound_spin_loop;

	if (self->mtype == M_GUARDIAN)
		gi.sound(self, CHAN_WEAPON, sound_charge, 1, ATTN_NORM, 0);
}

void guardian_fire_blaster(edict_t *self)
{
	vec3_t forward, right, start;
	int damage, speed, effect;

	if (!G_EntExists(self->enemy))
		return;

	if (self->mtype == M_MINIGUARDIAN)
	{
		damage = M_IONRIPPER_DMG_BASE + M_IONRIPPER_DMG_ADDON * drone_damagelevel(self);
		if (M_IONRIPPER_DMG_MAX && damage > M_IONRIPPER_DMG_MAX)
			damage = M_IONRIPPER_DMG_MAX;
		speed = M_IONRIPPER_SPEED_BASE + M_IONRIPPER_SPEED_ADDON * drone_damagelevel(self);
		if (M_IONRIPPER_SPEED_MAX && speed > M_IONRIPPER_SPEED_MAX)
			speed = M_IONRIPPER_SPEED_MAX;
		MonsterAim(self, M_PROJECTILE_ACC, speed, false, MZ2_SOLDIER_RIPPER_8, forward, start);
		if (!M_MonsterHasClearShotFrom(self, start))
			return;

		monster_fire_ionripper(self, start, forward, damage, speed, EF_IONRIPPER, MZ2_SOLDIER_RIPPER_8);
	}
	else
	{
		damage = M_HYPERBLASTER_DMG_BASE + M_HYPERBLASTER_DMG_ADDON * drone_damagelevel(self);
		if (M_HYPERBLASTER_DMG_MAX && damage > M_HYPERBLASTER_DMG_MAX)
			damage = M_HYPERBLASTER_DMG_MAX;

		speed = 1100;
		effect = (self->s.frame % 4) ? 0 : EF_HYPERBLASTER;
		AngleVectors(self->s.angles, forward, right, NULL);
		guardian_project_flash(self, MZ2_GUARDIAN_BLASTER, forward, right, start);
		MonsterAim(self, M_PROJECTILE_ACC, speed, false, -1, forward, start);
		if (!M_MonsterHasClearShotFrom(self, start))
			return;

		monster_fire_blaster(self, start, forward, damage, speed, effect, BLASTER_PROJ_BOLT, 2.0, true, MZ2_GUARDIAN_BLASTER);
	}

	if (G_EntExists(self->enemy) && self->s.frame == FRAME_atk1_spin12 && self->timestamp > level.time && visible(self, self->enemy))
		self->s.frame = FRAME_atk1_spin5 - 1;
}

mframe_t guardian_frames_atk1_spin[] =
{
	ai_charge, 0, guardian_atk1_charge,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, guardian_fire_blaster,
	ai_charge, 0, guardian_fire_blaster,
	ai_charge, 0, guardian_fire_blaster,
	ai_charge, 0, guardian_fire_blaster,
	ai_charge, 0, guardian_fire_blaster,
	ai_charge, 0, guardian_fire_blaster,
	ai_charge, 0, guardian_fire_blaster,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t guardian_move_atk1_spin = {FRAME_atk1_spin1, FRAME_atk1_spin15, guardian_frames_atk1_spin, guardian_atk1_finish};

void guardian_atk1(edict_t *self)
{
	self->monsterinfo.currentmove = &guardian_move_atk1_spin;
	self->timestamp = level.time + 0.65 + random() * 1.5;
}

mframe_t guardian_frames_atk1_in[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t guardian_move_atk1_in = {FRAME_atk1_in1, FRAME_atk1_in3, guardian_frames_atk1_in, guardian_atk1};

mframe_t guardian_frames_atk2_out[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, guardian_footstep,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t guardian_move_atk2_out = {FRAME_atk2_out1, FRAME_atk2_out7, guardian_frames_atk2_out, guardian_run};

void guardian_atk2_out(edict_t *self)
{
	self->monsterinfo.currentmove = &guardian_move_atk2_out;
}

static int guardian_grenade_flash(edict_t *self)
{
	return (self->s.frame & 1) ? MZ2_SUPERTANK_GRENADE_1 : MZ2_SUPERTANK_GRENADE_2;
}

void guardian_grenade(edict_t *self)
{
	vec3_t forward, right, start;
	int damage, speed, flash_number;

	if (!G_EntExists(self->enemy))
		return;

	damage = M_GRENADELAUNCHER_DMG_BASE + M_GRENADELAUNCHER_DMG_ADDON * drone_damagelevel(self);
	if (M_GRENADELAUNCHER_DMG_MAX && damage > M_GRENADELAUNCHER_DMG_MAX)
		damage = M_GRENADELAUNCHER_DMG_MAX;
	speed = M_GRENADELAUNCHER_SPEED_BASE + M_GRENADELAUNCHER_SPEED_ADDON * drone_damagelevel(self);
	if (M_GRENADELAUNCHER_SPEED_MAX && speed > M_GRENADELAUNCHER_SPEED_MAX)
		speed = M_GRENADELAUNCHER_SPEED_MAX;

	flash_number = guardian_grenade_flash(self);
	if (self->mtype == M_MINIGUARDIAN)
	{
		AngleVectors(self->s.angles, forward, right, NULL);
		guardian_project_laser_frame_origin(self, forward, right, start);
		flash_number = -1;
	}
	MonsterAim(self, M_PROJECTILE_ACC, speed, true, flash_number, forward, start);
	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	monster_fire_grenade(self, start, forward, damage, speed, flash_number);
}

void guardian_laser_fire(edict_t *self)
{
	int damage;

	if (!G_EntExists(self->enemy))
		return;

	gi.sound(self, CHAN_WEAPON, sound_laser, 1, ATTN_NORM, 0);
	damage = M_DABEAM_DMG_BASE + M_DABEAM_DMG_ADDON * drone_damagelevel(self);
	if (M_DABEAM_DMG_MAX && damage > M_DABEAM_DMG_MAX)
		damage = M_DABEAM_DMG_MAX;
	if (self->mtype == M_GUARDIAN)
		damage += 10 + 2 * drone_damagelevel(self);
	if (!guardian_has_laser_shot(self, self->s.frame & 1))
		return;

	monster_fire_dabeam(self, damage, self->s.frame & 1, NULL);
}

void guardian_fire_attack(edict_t *self)
{
	if (self->mtype == M_MINIGUARDIAN)
		guardian_grenade(self);
	else
		guardian_laser_fire(self);
}

mframe_t guardian_frames_atk2_fire[] =
{
	ai_charge, 0, guardian_fire_attack,
	ai_charge, 0, guardian_fire_attack,
	ai_charge, 0, guardian_fire_attack,
	ai_charge, 0, guardian_fire_attack
};
mmove_t guardian_move_atk2_fire = {FRAME_atk2_fire1, FRAME_atk2_fire4, guardian_frames_atk2_fire, guardian_atk2_out};

void guardian_atk2(edict_t *self)
{
	self->monsterinfo.currentmove = &guardian_move_atk2_fire;
}

mframe_t guardian_frames_atk2_in[] =
{
	ai_charge, 0, guardian_footstep,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, guardian_footstep,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, guardian_footstep,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t guardian_move_atk2_in = {FRAME_atk2_in1, FRAME_atk2_in12, guardian_frames_atk2_in, guardian_atk2};

void guardian_fire_rocket(edict_t *self, float offset)
{
	vec3_t forward, right, up, start, dir;
	int damage, speed;

	if (!G_EntExists(self->enemy))
		return;

	AngleVectors(self->s.angles, forward, right, up);
	VectorCopy(self->s.origin, start);
	VectorMA(start, -8 * guardian_scale(self), forward, start);
	VectorMA(start, offset * guardian_scale(self), right, start);
	VectorMA(start, 50 * guardian_scale(self), up, start);
	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	speed = M_ROCKETLAUNCHER_SPEED_BASE + M_ROCKETLAUNCHER_SPEED_ADDON * drone_damagelevel(self);
	if (M_ROCKETLAUNCHER_SPEED_MAX && speed > M_ROCKETLAUNCHER_SPEED_MAX)
		speed = M_ROCKETLAUNCHER_SPEED_MAX;
	speed *= 0.9f;
	if (speed < 200)
		speed = 200;

	damage = M_ROCKETLAUNCHER_DMG_BASE + M_ROCKETLAUNCHER_DMG_ADDON * drone_damagelevel(self);
	if (M_ROCKETLAUNCHER_DMG_MAX && damage > M_ROCKETLAUNCHER_DMG_MAX)
		damage = M_ROCKETLAUNCHER_DMG_MAX;

	VectorScale(forward, 0.35f, dir);
	VectorMA(dir, 0.95f, up, dir);
	VectorNormalize(dir);
	monster_fire_heat(self, start, dir, damage, speed, MZ2_GUARDIAN_BLASTER, 0.18f);
	gi.sound(self, CHAN_WEAPON, sound_pew, 1, 0.5f, 0);
}

void guardian_fire_rocket_l(edict_t *self)
{
	guardian_fire_rocket(self, -14.0f);
}

void guardian_fire_rocket_r(edict_t *self)
{
	guardian_fire_rocket(self, 14.0f);
}

mframe_t guardian_frames_rocket[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, guardian_fire_rocket_l,
	ai_charge, 0, NULL,
	ai_charge, 0, guardian_fire_rocket_l,
	ai_charge, 0, NULL,
	ai_charge, 0, guardian_fire_rocket_r,
	ai_charge, 0, NULL,
	ai_charge, 0, guardian_fire_rocket_r,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, guardian_fire_rocket_l,
	ai_charge, 0, NULL,
	ai_charge, 0, guardian_fire_rocket_l,
	ai_charge, 0, NULL,
	ai_charge, 0, guardian_fire_rocket_r,
	ai_charge, 0, NULL,
	ai_charge, 0, guardian_fire_rocket_r,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t guardian_move_rocket = {FRAME_turnl_1, FRAME_turnr_11, guardian_frames_rocket, guardian_run};

void guardian_kick(edict_t *self)
{
	vec3_t aim;

	if (!G_EntExists(self->enemy))
	{
		self->monsterinfo.melee_finished = level.time + 1.0;
		return;
	}

	VectorSet(aim, MELEE_DISTANCE, 0, -80);
	if (self->mtype == M_GUARDIAN)
		aim[0] = 160;
	if (!fire_hit(self, aim, (self->mtype == M_GUARDIAN) ? 85 : 30, 700))
		self->monsterinfo.melee_finished = level.time + ((self->mtype == M_GUARDIAN) ? 3.5 : 1.0);
}

mframe_t guardian_frames_kick[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, guardian_footstep,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, guardian_kick,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, guardian_footstep,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t guardian_move_kick = {FRAME_kick_in1, FRAME_kick_in13, guardian_frames_kick, guardian_run};

void guardian_attack(edict_t *self)
{
	float dist;
	qboolean can_blaster;
	qboolean can_atk2;
	qboolean can_rocket;

	if (!G_EntExists(self->enemy))
		return;

	dist = entdist(self, self->enemy);
	can_blaster = guardian_has_blaster_shot(self);
	can_atk2 = guardian_has_grenade_or_laser_shot(self);
	can_rocket = guardian_has_rocket_shot(self, -14.0f) || guardian_has_rocket_shot(self, 14.0f);

	if (self->mtype == M_GUARDIAN && self->monsterinfo.melee_finished < level.time && dist < 160)
		self->monsterinfo.currentmove = &guardian_move_kick;
	else if (self->mtype != M_GUARDIAN && self->monsterinfo.melee_finished < level.time && dist < 120)
		self->monsterinfo.currentmove = &guardian_move_kick;
	else if (can_rocket && self->mtype == M_GUARDIAN && dist > 300 && self->count <= 0 && random() < 0.25)
	{
		self->monsterinfo.currentmove = &guardian_move_rocket;
		self->count = 6;
	}
	else if (can_atk2 && (dist > 512 || (self->mtype == M_GUARDIAN && dist > 300 && random() < 0.5)))
		self->monsterinfo.currentmove = &guardian_move_atk2_in;
	else if (can_blaster)
		self->monsterinfo.currentmove = &guardian_move_atk1_in;
	else if (can_atk2)
		self->monsterinfo.currentmove = &guardian_move_atk2_in;
	else
		return;

	if (self->mtype == M_GUARDIAN && self->count > 0)
		self->count--;

	M_DelayNextAttack(self, 0, true);
}

void guardian_explode(edict_t *self)
{
	vec3_t org;

	VectorCopy(self->s.origin, org);
	org[0] += crandom() * self->maxs[0];
	org[1] += crandom() * self->maxs[1];
	org[2] += 24 + random() * self->maxs[2];

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1_BIG);
	gi.WritePosition(org);
	gi.multicast(self->s.origin, MULTICAST_PVS);
}

void guardian_dead(edict_t *self)
{
	int n;

	for (n = 0; n < 3; n++)
		guardian_explode(self);
	vrx_throw_drone_gibs(self, 125);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	self->svflags |= SVF_DEADMONSTER;
	M_Remove(self, false, false);
}

mframe_t guardian_frames_deathboss[] =
{
	ai_move, 0, guardian_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, guardian_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, guardian_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, guardian_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, guardian_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, guardian_explode
};
mmove_t guardian_move_deathboss = {FRAME_death1, FRAME_death26, guardian_frames_deathboss, guardian_dead};

mframe_t guardian_frames_death[] =
{
	ai_move, 0, guardian_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, guardian_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, guardian_explode
};
mmove_t guardian_move_death = {FRAME_death1, FRAME_death11, guardian_frames_death, guardian_dead};

void guardian_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	(void)inflictor;
	(void)attacker;
	(void)damage;
	(void)point;

	M_Notify(self);

	if (self->deadflag == DEAD_DEAD)
		return;

	self->s.sound = 0;
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_NO;

	if (self->mtype == M_GUARDIAN)
		self->monsterinfo.currentmove = &guardian_move_deathboss;
	else
		self->monsterinfo.currentmove = &guardian_move_death;

	DroneList_Remove(self);

	if (self->activator && !self->activator->client)
		self->activator->num_monsters_real--;
}

void init_drone_guardian(edict_t *self)
{
	qboolean miniguardian = (self->mtype == M_MINIGUARDIAN);

	sound_step = gi.soundindex("zortemp/step.wav");
	sound_charge = gi.soundindex("weapons/hyprbu1a.wav");
	sound_spin_loop = gi.soundindex("weapons/hyprbl1a.wav");
	sound_laser = gi.soundindex("weapons/laser2.wav");
	sound_pew = gi.soundindex("weapons/rocklf1a.wav");
	sound_pain = gi.soundindex("zortemp/ack.wav");

	if (!self->mtype)
		self->mtype = M_GUARDIAN;

	self->s.modelindex = gi.modelindex("models/monsters/guardian/tris.md2");
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->monsterinfo.control_cost = miniguardian ? M_TANK_CONTROL_COST : M_JORG_CONTROL_COST;
	self->monsterinfo.cost = miniguardian ? 175 : 500;

	if (miniguardian)
	{
		self->s.skinnum = 2;
		self->s.scale = 0.4f;
		self->monsterinfo.scale = MODEL_SCALE * 0.4f;
		VectorSet(self->mins, -38, -38, -26);
		VectorSet(self->maxs, 38, 38, 25);
		self->health = M_MINIGUARDIAN_INITIAL_HEALTH + M_MINIGUARDIAN_ADDON_HEALTH * self->monsterinfo.level;
		M_SetMonsterArmor(self, M_MINIGUARDIAN_INITIAL_ARMOR + M_MINIGUARDIAN_ADDON_ARMOR * self->monsterinfo.level);
		self->mass = 340;
	}
	else
	{
		if (invasion->value)
		{
			self->s.scale = GUARDIAN_INVASION_SCALE;
			self->monsterinfo.scale = MODEL_SCALE * GUARDIAN_INVASION_SCALE;
			VectorSet(self->mins, -44, -44, -30);
			VectorSet(self->maxs, 44, 44, 45);
		}
		else
		{
			self->monsterinfo.scale = MODEL_SCALE;
			VectorSet(self->mins, -96, -96, -66);
			VectorSet(self->maxs, 96, 96, 62);
		}
		self->health = M_GUARDIAN_INITIAL_HEALTH + M_GUARDIAN_ADDON_HEALTH * self->monsterinfo.level;
		M_SetMonsterArmor(self, M_GUARDIAN_INITIAL_ARMOR + M_GUARDIAN_ADDON_ARMOR * self->monsterinfo.level);
		self->mass = 850;
	}

	self->max_health = self->health;
	self->gib_health = miniguardian ? -3 * BASE_GIB_HEALTH : -6 * BASE_GIB_HEALTH;
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;

	self->pain = guardian_pain;
	self->die = guardian_die;
	self->monsterinfo.stand = guardian_stand;
	self->monsterinfo.walk = guardian_walk;
	self->monsterinfo.run = guardian_run;
	self->monsterinfo.attack = guardian_attack;
	self->monsterinfo.sight = guardian_sight;
	self->monsterinfo.currentmove = &guardian_move_stand;

	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);

	if (!miniguardian && !invasion->value)
		G_PrintGreenText(va("A level %d guardian has spawned!", self->monsterinfo.level));
}
