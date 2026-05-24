/*
==============================================================================

flyer

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_flyer.h"

qboolean visible (const edict_t *self, const edict_t *other);

static int	nextmove;			// Used for start/stop frames

static int	sound_sight;
static int	sound_idle;
static int	sound_pain1;
static int	sound_pain2;
static int	sound_slash;
static int	sound_sproing;
static int	sound_die;
static int	sound_laser;

static constexpr int FLYER_ROCKET_MIN_SPEED = 850;
static constexpr float FLYER_ROCKET_ATTACK_CHANCE = 0.24f;
static constexpr float FLYER_ROCKET_BONUS_ATTACK_CHANCE = 0.30f;
static constexpr float FLYER_ROCKET_REFIRE_CHANCE = 0.06f;
static constexpr float FLYER_ROCKET_STRAFE_RANGE = 768.0f;
static constexpr float FLYER_ROCKET_STRAFE_PROBE = 320.0f;
static constexpr float FLYER_ROCKET_STRAFE_PIN_TIME = 0.75f;
static constexpr float FLYER_ROCKET_AIM_ACCURACY = 0.98f;
static constexpr float FLYER_LASER_ATTACK_CHANCE = 0.28f;
static constexpr float FLYER_LASER_MIN_RANGE = 150.0f;
static constexpr float FLYER_LASER_MAX_RANGE = 400.0f;
static constexpr float FLYER_LASER_SWEEP_SIDE = 42.0f;
static constexpr float FLYER_LASER_SWEEP_UP = 18.0f;
static constexpr float FLYER_LASER_SWEEP_SPEED = 8.0f;
static constexpr float FLYER_LASER_AIM_BLEND = 0.30f;
static constexpr float FLYER_LASER_FAST_AIM_BLEND = 0.18f;
static constexpr float FLYER_LASER_FAST_TARGET_SPEED = 180.0f;
static constexpr float FLYER_LASER_AIM_RESET_TIME = 0.45f;
static constexpr float FLYER_RANGED_MIN_DISTANCE = 45.0f;
static constexpr float FLYER_RANGED_MAX_DISTANCE = 200.0f;
static constexpr float FLYER_MELEE_APPROACH_RANGE = 225.0f;
static constexpr float FLYER_MELEE_Z_TOLERANCE = 72.0f;


void flyer_check_melee(edict_t *self);
void flyer_loop_melee (edict_t *self);
void flyer_melee (edict_t *self);
void flyer_setstart (edict_t *self);
void flyer_stand (edict_t *self);
void flyer_nextmove (edict_t *self);
static void flyer_attack_finished(edict_t *self);
void flyer_rocket(edict_t *self);
void flyer_reattack_rocket(edict_t *self);
void flyer_laser_warn(edict_t *self);
void flyer_laser_on(edict_t *self);
void flyer_laser_off(edict_t *self);
void flyer_recharge(edict_t *self);

static void flyer_set_fly_parameters(edict_t *self, qboolean melee)
{
	if (melee)
	{
		self->monsterinfo.fly_pinned = false;
		self->monsterinfo.fly_thrusters = true;
		self->monsterinfo.fly_position_time = 0.0f;
		self->monsterinfo.fly_acceleration = 20.0f;
		self->monsterinfo.fly_speed = 210.0f;
		self->monsterinfo.fly_min_distance = 0.0f;
		self->monsterinfo.fly_max_distance = 10.0f;
		return;
	}

	self->monsterinfo.fly_thrusters = false;
	self->monsterinfo.fly_acceleration = 15.0f;
	self->monsterinfo.fly_speed = 165.0f;
	self->monsterinfo.fly_min_distance = FLYER_RANGED_MIN_DISTANCE;
	self->monsterinfo.fly_max_distance = FLYER_RANGED_MAX_DISTANCE;
}

static int flyer_melee_damage(edict_t *self)
{
	int damage;

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self);
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;
	return damage;
}

static void flyer_restore_ranged_hover(edict_t *self, float range)
{
	flyer_set_fly_parameters(self, false);
	self->monsterinfo.fly_pinned = false;
	if (range < self->monsterinfo.fly_min_distance)
		self->monsterinfo.fly_position_time = 0.0f;
}

static qboolean flyer_drive_side_strafe(edict_t *self, float probe, float side_speed, float forward_speed,
	float pin_time, float flip_chance)
{
	vec3_t	forward, right, check_end;
	trace_t	tr;
	float	strafe_direction;
	float	vertical_velocity;
	qboolean choose_new_side;

	if (!G_ValidTarget(self, self->enemy, true, true) || !visible(self, self->enemy))
		return false;

	AngleVectors(self->s.angles, forward, right, NULL);
	choose_new_side = !self->monsterinfo.fly_pinned || self->monsterinfo.fly_position_time <= level.time;
	if (choose_new_side && flip_chance > 0.0f && random() < flip_chance)
		self->monsterinfo.lefty = !self->monsterinfo.lefty;
	strafe_direction = self->monsterinfo.lefty ? -1.0f : 1.0f;

	VectorMA(self->s.origin, probe * strafe_direction, right, check_end);
	tr = gi.trace(self->s.origin, NULL, NULL, check_end, self, MASK_MONSTERSOLID);
	if (tr.fraction < 1.0f)
	{
		strafe_direction *= -1.0f;
		VectorMA(self->s.origin, probe * strafe_direction, right, check_end);
		tr = gi.trace(self->s.origin, NULL, NULL, check_end, self, MASK_MONSTERSOLID);
		if (tr.fraction < 1.0f)
			return false;
	}

	vertical_velocity = self->velocity[2];
	VectorScale(forward, forward_speed, self->velocity);
	VectorMA(self->velocity, strafe_direction * side_speed, right, self->velocity);
	self->velocity[2] = vertical_velocity;
	self->monsterinfo.lefty = (strafe_direction < 0.0f);
	self->monsterinfo.attack_state = AS_SLIDING;
	self->monsterinfo.fly_pinned = true;
	VectorCopy(check_end, self->monsterinfo.fly_ideal_position);
	if (forward_speed)
		VectorMA(self->monsterinfo.fly_ideal_position, forward_speed * 0.45f, forward, self->monsterinfo.fly_ideal_position);
	self->monsterinfo.fly_position_time = level.time + pin_time;
	return true;
}

static qboolean flyer_melee_z_ok(edict_t *self)
{
	vec3_t self_mid, enemy_mid;

	if (!G_EntExists(self->enemy))
		return false;

	G_EntMidPoint(self, self_mid);
	G_EntMidPoint(self->enemy, enemy_mid);
	return fabsf(enemy_mid[2] - self_mid[2]) <= FLYER_MELEE_Z_TOLERANCE;
}

static qboolean flyer_melee_clear_path(edict_t *self)
{
	vec3_t start, end;

	if (!G_EntExists(self->enemy))
		return false;

	G_EntMidPoint(self, start);
	G_EntMidPoint(self->enemy, end);
	return G_ClearPath(self, self->enemy, MASK_MONSTERSOLID, start, end);
}

static qboolean flyer_melee_ready(edict_t *self)
{
	return M_MonsterMeleeReady(self)
		&& flyer_melee_z_ok(self)
		&& nearfov(self, self->enemy, 0, 100);
}

static qboolean flyer_melee_approach_ready(edict_t *self, float range)
{
	if (self->monsterinfo.bonus_flags)
		return false;
	if (self->monsterinfo.melee_finished > level.time)
		return false;
	if (!G_ValidTarget(self, self->enemy, true, true))
		return false;
	if (range > FLYER_MELEE_APPROACH_RANGE)
		return false;
	if (!flyer_melee_z_ok(self))
		return false;
	if (!nearfov(self, self->enemy, 0, 100))
		return false;
	if (!flyer_melee_clear_path(self))
		return false;
	return true;
}

static qboolean flyer_predict_rocket_aim(edict_t *self, vec3_t start, int speed, vec3_t aim)
{
	vec3_t forward, target, direct, predicted, predicted_dir;
	trace_t tr;
	float dist, time, skill_error, miss_time;

	if (!G_EntExists(self->enemy))
		return false;

	VectorCopy(self->enemy->s.origin, target);
	if (self->enemy->client)
		target[2] += self->enemy->viewheight;
	else
		target[2] += 22.0f;

	VectorSubtract(target, start, direct);
	dist = VectorNormalize(direct);
	if (dist <= 1.0f)
		return false;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	if (DotProduct(direct, forward) < FLYER_ROCKET_AIM_ACCURACY)
		return false;

	time = (speed > 0) ? dist / speed : 0.0f;
	skill_error = skill ? 3.0f - skill->value : 2.0f;
	if (skill_error < 0.0f)
		skill_error = 0.0f;
	miss_time = random() * (skill_error / 3.0f) - random() * (0.05f * skill_error);

	VectorMA(self->enemy->s.origin, time - miss_time, self->enemy->velocity, predicted);
	if (self->enemy->client)
		predicted[2] += self->enemy->viewheight;
	else
		predicted[2] += 22.0f;

	VectorSubtract(predicted, start, predicted_dir);
	if (!VectorNormalize(predicted_dir) || DotProduct(direct, predicted_dir) < 0.0f)
		VectorCopy(target, predicted);
	else
	{
		tr = gi.trace(start, NULL, NULL, predicted, self, MASK_SOLID);
		if (tr.fraction < 0.9f && tr.ent != self->enemy)
			VectorCopy(target, predicted);
	}

	VectorSubtract(predicted, start, aim);
	if (!VectorNormalize(aim))
		VectorCopy(direct, aim);
	return true;
}


void flyer_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void flyer_idle (edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;
	gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void flyer_pop_blades (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_sproing, 1, ATTN_NORM, 0);
}


mframe_t flyer_frames_stand [] =
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
	drone_ai_stand, 0, NULL
};
mmove_t	flyer_move_stand = {FRAME_stand01, FRAME_stand45, flyer_frames_stand, NULL};


mframe_t flyer_frames_walk [] =
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
	drone_ai_walk, 5, NULL
};
mmove_t	flyer_move_walk = {FRAME_stand01, FRAME_stand45, flyer_frames_walk, NULL};

mframe_t flyer_frames_run [] =
{
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL
};
mmove_t	flyer_move_run = {FRAME_stand01, FRAME_stand45, flyer_frames_run, NULL};

void flyer_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &flyer_move_stand;
	else
		self->monsterinfo.currentmove = &flyer_move_run;
}

void flyer_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &flyer_move_walk;
}

void flyer_stand (edict_t *self)
{
		self->monsterinfo.currentmove = &flyer_move_stand;
}

mframe_t flyer_frames_start [] =
{
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	flyer_nextmove
};
mmove_t flyer_move_start = {FRAME_start01, FRAME_start06, flyer_frames_start, NULL};

mframe_t flyer_frames_stop [] =
{
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	flyer_nextmove
};
mmove_t flyer_move_stop = {FRAME_stop01, FRAME_stop07, flyer_frames_stop, NULL};

void flyer_stop (edict_t *self)
{
		self->monsterinfo.currentmove = &flyer_move_stop;
}

void flyer_start (edict_t *self)
{
		self->monsterinfo.currentmove = &flyer_move_start;
}


static void flyer_checkstrafe(edict_t *self)
{
	float	strafe_speed;
	float	forward_speed;
	float	range;

	if (!G_ValidTarget(self, self->enemy, true, true) || !visible(self, self->enemy))
		return;

	range = entdist(self, self->enemy);

	strafe_speed = 520.0f + random() * 220.0f;
	if (range < 260.0f)
		forward_speed = -220.0f;
	else if (range > FLYER_ROCKET_STRAFE_RANGE)
		forward_speed = 240.0f;
	else
		forward_speed = 190.0f;
	if (!flyer_drive_side_strafe(self, FLYER_ROCKET_STRAFE_PROBE, strafe_speed,
		forward_speed, FLYER_ROCKET_STRAFE_PIN_TIME, 0.25f))
		return;
	self->monsterinfo.pausetime = level.time + 0.75f + random() * 0.55f;
}

static void flyer_ai_rocket_strafe(edict_t *self, float dist)
{
	flyer_checkstrafe(self);
	ai_charge(self, dist);
}

void flyer_rocket(edict_t *self)
{
	vec3_t	start, forward;
	int		damage, speed;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	if (self->delay > level.time)
		return;

	damage = M_ROCKETLAUNCHER_DMG_BASE + M_ROCKETLAUNCHER_DMG_ADDON * drone_damagelevel(self);
	if (M_ROCKETLAUNCHER_DMG_MAX && damage > M_ROCKETLAUNCHER_DMG_MAX)
		damage = M_ROCKETLAUNCHER_DMG_MAX;

	speed = M_ROCKETLAUNCHER_SPEED_BASE + M_ROCKETLAUNCHER_SPEED_ADDON * drone_damagelevel(self);
	if (M_ROCKETLAUNCHER_SPEED_MAX && speed > M_ROCKETLAUNCHER_SPEED_MAX)
		speed = M_ROCKETLAUNCHER_SPEED_MAX;
	if (speed < FLYER_ROCKET_MIN_SPEED)
		speed = FLYER_ROCKET_MIN_SPEED;

	VectorCopy(self->s.origin, start);
	if (!flyer_predict_rocket_aim(self, start, speed, forward))
		return;
	if (!M_MonsterHasClearShotFrom(self, start))
	{
		M_MonsterBlockedShot(self, 0.35f);
		return;
	}

	monster_fire_rocket(self, start, forward, damage, speed, MZ2_TURRET_ROCKET);
	self->delay = level.time + 0.22f + random() * 0.23f;
}

void flyer_reattack_rocket(edict_t *self)
{
	if (G_ValidTarget(self, self->enemy, true, true) && self->delay <= level.time
		&& random() < FLYER_ROCKET_REFIRE_CHANCE)
	{
		flyer_checkstrafe(self);
		flyer_rocket(self);
		self->monsterinfo.nextframe = FRAME_rollr03;
		return;
	}

	self->monsterinfo.attack_finished = level.time + 0.65f + random() * 0.45f;
}

mframe_t flyer_frames_rollright [] =
{
		flyer_ai_rocket_strafe, 3, NULL,
		flyer_ai_rocket_strafe, 3, NULL,
		flyer_ai_rocket_strafe, 3, NULL,
		flyer_ai_rocket_strafe, 3, flyer_rocket,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		flyer_ai_rocket_strafe, 3, flyer_reattack_rocket
};
mmove_t flyer_move_rollright = {FRAME_rollr01, FRAME_rollr09, flyer_frames_rollright, flyer_run};

mframe_t flyer_frames_rollleft [] =
{
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
mmove_t flyer_move_rollleft = {FRAME_rollf01, FRAME_rollf09, flyer_frames_rollleft, NULL};

mframe_t flyer_frames_pain3 [] =
{	
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL
};
mmove_t flyer_move_pain3 = {FRAME_pain301, FRAME_pain304, flyer_frames_pain3, flyer_run};

mframe_t flyer_frames_pain2 [] =
{
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL
};
mmove_t flyer_move_pain2 = {FRAME_pain201, FRAME_pain204, flyer_frames_pain2, flyer_run};

mframe_t flyer_frames_pain1 [] =
{
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
mmove_t flyer_move_pain1 = {FRAME_pain101, FRAME_pain109, flyer_frames_pain1, flyer_run};

mframe_t flyer_frames_defense [] = 
{
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,		// Hold this frame
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL
};
mmove_t flyer_move_defense = {FRAME_defens01, FRAME_defens06, flyer_frames_defense, NULL};

mframe_t flyer_frames_bankright [] =
{
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL
};
mmove_t flyer_move_bankright = {FRAME_bankr01, FRAME_bankr07, flyer_frames_bankright, NULL};

mframe_t flyer_frames_bankleft [] =
{
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL
};
mmove_t flyer_move_bankleft = {FRAME_bankl01, FRAME_bankl07, flyer_frames_bankleft, NULL};		


void flyer_fire (edict_t *self, int flash_number)
{
	vec3_t	start, forward, right, offset;
	int		effect, damage;

	if ((self->s.frame == FRAME_attak204) || (self->s.frame == FRAME_attak207) || (self->s.frame == FRAME_attak210))
		effect = EF_HYPERBLASTER;
	else
		effect = 0;

	damage = M_HYPERBLASTER_DMG_BASE + M_HYPERBLASTER_DMG_ADDON * drone_damagelevel(self);
	if (M_HYPERBLASTER_DMG_MAX && damage > M_HYPERBLASTER_DMG_MAX)
		damage = M_HYPERBLASTER_DMG_MAX;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorCopy(monster_flash_offset[flash_number], offset);
	if (self->s.scale)
		VectorScale(offset, self->s.scale, offset);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
	MonsterAim(self, M_PROJECTILE_ACC, 2000, false, -1, forward, start);
	if (!M_MonsterHasClearShotFrom(self, start))
	{
		M_MonsterBlockedShot(self, 0.35f);
		return;
	}
	monster_fire_blaster(self, start, forward, damage, 2000, effect, BLASTER_PROJ_BOLT, 2.0, false, flash_number);
}

void flyer_fireleft (edict_t *self)
{
	flyer_fire (self, MZ2_FLYER_BLASTER_1);
}

void flyer_fireright (edict_t *self)
{
	flyer_fire (self, MZ2_FLYER_BLASTER_2);
}

static void flyer_reattack_blaster(edict_t *self)
{
	if (flyer_melee_ready(self))
	{
		flyer_attack_finished(self);
		return;
	}

	if (G_EntExists(self->enemy) && visible(self, self->enemy) && random() < 0.55f)
	{
		self->monsterinfo.nextframe = FRAME_attak204;
		return;
	}

	flyer_attack_finished(self);
}

mframe_t flyer_frames_attack3[] =
{
		ai_charge, 10, NULL,
		ai_charge, 10, NULL,
		ai_charge, 10, NULL,
		ai_charge, 10, flyer_fireleft,			// left gun
		ai_charge, 10, flyer_fireright,		// right gun
		ai_charge, 10, flyer_fireleft,			// left gun
		ai_charge, 10, flyer_fireright,		// right gun
		ai_charge, 10, flyer_fireleft,			// left gun
		ai_charge, 10, flyer_fireright,		// right gun
		ai_charge, 10, flyer_fireleft,			// left gun
		ai_charge, 10, flyer_fireright,		// right gun
		ai_charge, 10, NULL,
		ai_charge, 10, NULL,
		ai_charge, 10, NULL,
		ai_charge, -15, flyer_reattack_blaster,
		ai_charge, 10, NULL,
		ai_charge, 10, NULL
};
static void flyer_attack_finished(edict_t *self)
{
	self->monsterinfo.attack_finished = level.time + 0.8f + random() * 0.6f;
	flyer_run(self);
}

mmove_t flyer_move_attack3 = { FRAME_attak201, FRAME_attak217, flyer_frames_attack3, flyer_attack_finished };

mframe_t flyer_frames_attack2 [] =
{
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, -10, flyer_fireleft,			// left gun
		ai_charge, -10, flyer_fireright,		// right gun
		ai_charge, -10, flyer_fireleft,			// left gun
		ai_charge, -10, flyer_fireright,		// right gun
		ai_charge, -10, flyer_fireleft,			// left gun
		ai_charge, -10, flyer_fireright,		// right gun
		ai_charge, -10, flyer_fireleft,			// left gun
		ai_charge, -10, flyer_fireright,		// right gun
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, -15, flyer_reattack_blaster,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL
};
mmove_t flyer_move_attack2 = {FRAME_attak201, FRAME_attak217, flyer_frames_attack2, flyer_attack_finished};


static void flyer_slash(edict_t *self, float side)
{
	vec3_t	aim;

	gi.sound (self, CHAN_WEAPON, sound_slash, 1, ATTN_NORM, 0);

	if (!G_ValidTarget(self, self->enemy, true, true)
		|| entdist(self, self->enemy) > MELEE_DISTANCE
		|| !flyer_melee_z_ok(self))
	{
		self->monsterinfo.melee_finished = level.time + 1.5f;
		return;
	}

	VectorSet (aim, MELEE_DISTANCE, side, 0);
	if (!fire_hit(self, aim, flyer_melee_damage(self), 0))
		self->monsterinfo.melee_finished = level.time + 1.5f;
}

void flyer_slash_left (edict_t *self)
{
	flyer_slash(self, self->mins[0]);
}

void flyer_slash_right (edict_t *self)
{
	flyer_slash(self, self->maxs[0]);
}

mframe_t flyer_frames_start_melee [] =
{
		ai_charge, 0, flyer_pop_blades,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL
};
mmove_t flyer_move_start_melee = {FRAME_attak101, FRAME_attak106, flyer_frames_start_melee, flyer_loop_melee};

mframe_t flyer_frames_end_melee [] =
{
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL
};
mmove_t flyer_move_end_melee = {FRAME_attak119, FRAME_attak121, flyer_frames_end_melee, flyer_run};

mframe_t flyer_frames_loop_melee [] =
{
		ai_charge, 0, NULL,		// Loop Start
		ai_charge, 0, NULL,
		ai_charge, 0, flyer_slash_left,		// Left Wing Strike
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, flyer_slash_right,	// Right Wing Strike
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL		// Loop Ends
		
};
mmove_t flyer_move_loop_melee = {FRAME_attak107, FRAME_attak118, flyer_frames_loop_melee, flyer_check_melee};

void flyer_check_melee(edict_t *self)
{
	if (!G_ValidTarget(self, self->enemy, true, true) ||
		(!visible(self, self->enemy) && !M_MonsterHasCombatSight(self, self->enemy)) ||
		!flyer_melee_z_ok(self))
	{
		self->monsterinfo.nextattack = 0;
		self->monsterinfo.attack_state = AS_STRAIGHT;
		flyer_set_fly_parameters(self, false);
		self->monsterinfo.currentmove = &flyer_move_end_melee;
		M_DelayNextAttack(self, 0.2f, true);
		return;
	}

	if (flyer_melee_ready(self)
		&& self->monsterinfo.nextattack < 3
		&& random() <= 0.8f)
	{
		self->monsterinfo.nextattack++;
		self->monsterinfo.attack_state = AS_MELEE;
		flyer_set_fly_parameters(self, true);
		self->monsterinfo.currentmove = &flyer_move_loop_melee;
		return;
	}

	self->monsterinfo.nextattack = 0;
	self->monsterinfo.attack_state = AS_STRAIGHT;
	flyer_set_fly_parameters(self, false);
	self->monsterinfo.currentmove = &flyer_move_end_melee;
	M_DelayNextAttack(self, 0.2f, true);
}

void flyer_loop_melee (edict_t *self)
{
	float range;

	if (!G_ValidTarget(self, self->enemy, true, true) ||
		(!visible(self, self->enemy) && !M_MonsterHasCombatSight(self, self->enemy)) ||
		!flyer_melee_z_ok(self) ||
		!nearfov(self, self->enemy, 0, 100))
	{
		self->monsterinfo.nextattack = 0;
		self->monsterinfo.attack_state = AS_STRAIGHT;
		flyer_set_fly_parameters(self, false);
		self->monsterinfo.currentmove = &flyer_move_end_melee;
		M_DelayNextAttack(self, 0.2f, true);
		return;
	}

	range = entdist(self, self->enemy);
	if (range > FLYER_MELEE_APPROACH_RANGE || !flyer_melee_clear_path(self))
	{
		self->monsterinfo.nextattack = 0;
		self->monsterinfo.attack_state = AS_STRAIGHT;
		flyer_set_fly_parameters(self, false);
		self->monsterinfo.currentmove = &flyer_move_end_melee;
		M_DelayNextAttack(self, 0.2f, true);
		return;
	}

/*	if (random() <= 0.5)	
		self->monsterinfo.currentmove = &flyer_move_attack1;
	else */
	self->monsterinfo.nextattack = 0;
	self->monsterinfo.attack_state = AS_MELEE;
	flyer_set_fly_parameters(self, true);
	self->monsterinfo.currentmove = &flyer_move_loop_melee;
}

static vec3_t flyer_left_laser_offset = { 14.1f, -13.4f, -7.0f };
static vec3_t flyer_right_laser_offset = { 14.1f, 13.4f, -7.0f };

static qboolean flyer_laser_target(edict_t *self, vec3_t target)
{
	vec3_t	actual_target, delta;
	float	blend;
	qboolean update_turn = false;
	qboolean first_laser_frame;

	G_EntMidPoint(self->enemy, actual_target);
	first_laser_frame = (!self->target_ent || !self->target_ent->inuse)
		&& (!self->beam || !self->beam->inuse)
		&& (!self->beam2 || !self->beam2->inuse);

	if (first_laser_frame || self->timestamp <= 0.0f || level.time - self->timestamp > FLYER_LASER_AIM_RESET_TIME)
	{
		VectorCopy(actual_target, self->pos1);
		update_turn = true;
	}
	else if (self->timestamp < level.time)
	{
		blend = (VectorLength(self->enemy->velocity) >= FLYER_LASER_FAST_TARGET_SPEED)
			? FLYER_LASER_FAST_AIM_BLEND
			: FLYER_LASER_AIM_BLEND;
		VectorSubtract(actual_target, self->pos1, delta);
		VectorMA(self->pos1, blend, delta, self->pos1);
		update_turn = true;
	}

	self->timestamp = level.time;
	VectorCopy(self->pos1, target);
	return update_turn;
}

static qboolean flyer_laser_aim(edict_t *self)
{
	vec3_t	target, dir, angles;
	qboolean update_turn;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return false;

	update_turn = flyer_laser_target(self, target);
	VectorSubtract(target, self->s.origin, dir);
	if (VectorLength(dir) < 1.0f)
		return false;

	vectoangles(dir, angles);
	if (update_turn)
	{
		self->ideal_yaw = angles[YAW];
		M_ChangeYaw(self);
		self->s.angles[PITCH] = angles[PITCH];
	}

	return true;
}

void flyer_laser_warn(edict_t *self)
{
	if (!flyer_laser_aim(self))
	{
		flyer_laser_off(self);
		return;
	}

	// Vortex does not have misc_flare or the Remaster mynoise/mynoise2
	// flare controller fields yet. Restore this telegraph block after those
	// entities/fields exist.
	/*
	edict_t *left_flare = G_Spawn();
	if (left_flare)
	{
		left_flare->classname = "misc_flare";
		left_flare->owner = self;
		left_flare->s.skinnum = 0xFF0000FF;
		left_flare->spawnflags = 9;
		ED_CallSpawn(left_flare);
		gi.linkentity(left_flare);
	}

	edict_t *right_flare = G_Spawn();
	if (right_flare)
	{
		right_flare->classname = "misc_flare";
		right_flare->owner = self;
		right_flare->s.skinnum = 0xFF0000FF;
		right_flare->spawnflags = 9;
		ED_CallSpawn(right_flare);
		gi.linkentity(right_flare);
	}
	*/
}

static void flyer_project_laser_start(edict_t *self, vec3_t offset, vec3_t forward, vec3_t right, vec3_t up, vec3_t start)
{
	AngleVectors(self->s.angles, forward, right, up);
	VectorCopy(self->s.origin, start);
	VectorMA(start, offset[0], forward, start);
	VectorMA(start, offset[1], right, start);
	VectorMA(start, offset[2], up, start);
}

static void flyer_laser_update(edict_t *laser, vec3_t offset, float phase_offset)
{
	edict_t *self = laser->owner;
	vec3_t	start, forward, right, up, dir, target;
	float	phase;

	if (!self || !self->inuse || !G_ValidTarget(self, self->enemy, true, true))
	{
		laser->spawnflags |= DABEAM_SPAWNED;
		return;
	}

	flyer_project_laser_start(self, offset, forward, right, up, start);
	G_EntMidPoint(self->enemy, target);
	phase = level.time * FLYER_LASER_SWEEP_SPEED + phase_offset + (float)(self - g_edicts) * 0.17f;
	VectorMA(target, sinf(phase) * FLYER_LASER_SWEEP_SIDE, right, target);
	VectorMA(target, cosf(phase * 0.7f) * FLYER_LASER_SWEEP_UP, up, target);
	VectorSubtract(target, start, dir);
	if (!VectorNormalize(dir))
		VectorCopy(forward, dir);

	VectorCopy(start, laser->s.origin);
	VectorCopy(dir, laser->movedir);
	gi.linkentity(laser);
	dabeam_update(laser, false);
}

void flyer_left_laser_update(edict_t *laser)
{
	flyer_laser_update(laser, flyer_left_laser_offset, 0.0f);
}

void flyer_right_laser_update(edict_t *laser)
{
	flyer_laser_update(laser, flyer_right_laser_offset, M_PI);
}

void flyer_laser_on(edict_t *self)
{
	int damage;
	qboolean starting_beams;

	if (!flyer_laser_aim(self))
	{
		flyer_laser_off(self);
		return;
	}

	damage = M_DABEAM_DMG_BASE + M_DABEAM_DMG_ADDON * drone_damagelevel(self);
	if (M_DABEAM_DMG_MAX && damage > M_DABEAM_DMG_MAX)
		damage = M_DABEAM_DMG_MAX;

	starting_beams = (!self->beam || !self->beam->inuse || !self->beam2 || !self->beam2->inuse);
	if ((!self->beam || !self->beam->inuse) && (!self->beam2 || !self->beam2->inuse))
		gi.sound(self, CHAN_WEAPON, sound_laser, 1, ATTN_NORM, 0);
	monster_fire_dabeam(self, starting_beams ? 0 : damage, false, flyer_left_laser_update);
	monster_fire_dabeam(self, starting_beams ? 0 : damage, true, flyer_right_laser_update);
}

void flyer_laser_off(edict_t *self)
{
	if (self->target_ent && self->target_ent->inuse && self->target_ent->owner == self)
		G_FreeEdict(self->target_ent);
	self->target_ent = NULL;

	if (self->beam && self->beam->inuse)
		G_FreeEdict(self->beam);
	self->beam = NULL;

	if (self->beam2 && self->beam2->inuse)
		G_FreeEdict(self->beam2);
	self->beam2 = NULL;

	self->monsterinfo.fly_pinned = false;
	self->monsterinfo.fly_position_time = 0.0f;
	self->timestamp = 0.0f;
	VectorClear(self->pos1);
}

mframe_t flyer_frames_laser_right [] =
{
		ai_charge, 0, flyer_laser_warn,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, flyer_laser_on,
		ai_charge, 0, flyer_laser_on,
		ai_charge, 0, flyer_laser_on,
		ai_charge, 0, flyer_laser_on,
		ai_charge, 0, flyer_laser_on,
		ai_charge, 0, flyer_laser_on,
		ai_charge, 0, flyer_laser_on,
		ai_charge, 0, flyer_laser_on,
		ai_charge, 0, flyer_laser_on,
		ai_charge, 0, flyer_laser_on,
		ai_charge, 0, flyer_laser_on,
		ai_charge, 0, flyer_laser_on
};
mmove_t flyer_move_laser_right = {FRAME_attak201, FRAME_attak217, flyer_frames_laser_right, flyer_recharge};

static void flyer_laser_recharge_done(edict_t *self)
{
	flyer_laser_off(self);
	flyer_attack_finished(self);
}

mframe_t flyer_frames_laser_recharge [] =
{
		ai_charge, 0, NULL,
		ai_charge, 0, flyer_laser_off,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL
};
mmove_t flyer_move_laser_recharge = {FRAME_defens01, FRAME_defens06, flyer_frames_laser_recharge, flyer_laser_recharge_done};

void flyer_recharge(edict_t *self)
{
	self->monsterinfo.currentmove = &flyer_move_laser_recharge;
}


void flyer_attack (edict_t *self)
{
	float range;
	qboolean use_rocket_attack;

	flyer_set_fly_parameters(self, false);

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	range = entdist(self, self->enemy);

	if (flyer_melee_approach_ready(self, range)
		&& random() > (range / FLYER_MELEE_APPROACH_RANGE) * 0.35f)
	{
		self->monsterinfo.attack_state = AS_MELEE;
		self->monsterinfo.nextattack = 0;
		flyer_set_fly_parameters(self, true);
		self->monsterinfo.currentmove = &flyer_move_start_melee;
		return;
	}

	flyer_restore_ranged_hover(self, range);

	if (range > FLYER_LASER_MIN_RANGE && range < FLYER_LASER_MAX_RANGE && random() < FLYER_LASER_ATTACK_CHANCE)
	{
		self->monsterinfo.attack_state = AS_STRAIGHT;
		self->monsterinfo.currentmove = &flyer_move_laser_right;
		return;
	}

/*	if (random() <= 0.5)	
		self->monsterinfo.currentmove = &flyer_move_attack1;
	else */
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.attack_state = AS_STRAIGHT;
		self->monsterinfo.currentmove = &flyer_move_attack3;
	}
	else
	{
		use_rocket_attack = random() < ((self->monsterinfo.bonus_flags)
			? FLYER_ROCKET_BONUS_ATTACK_CHANCE
			: FLYER_ROCKET_ATTACK_CHANCE);
		if (use_rocket_attack && self->delay <= level.time)
		{
			self->monsterinfo.attack_state = AS_SLIDING;
			self->monsterinfo.lefty = random() < 0.5f;
			flyer_checkstrafe(self);
			self->monsterinfo.currentmove = &flyer_move_rollright;
		}
		else if (random() < 0.5f)
		{
			self->monsterinfo.attack_state = AS_STRAIGHT;
			self->monsterinfo.currentmove = &flyer_move_attack2;
		}
		else
		{
			if (random() <= 0.5f)
				self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
			self->monsterinfo.attack_state = AS_SLIDING;
			self->monsterinfo.currentmove = &flyer_move_attack3;
		}
	}

	if (!self->monsterinfo.fly_pinned
		&& range >= self->monsterinfo.fly_min_distance
		&& random() < 0.5f
		&& visible(self, self->enemy))
	{
		self->monsterinfo.fly_pinned = true;
		if (self->monsterinfo.fly_position_time < level.time + 1.7f)
			self->monsterinfo.fly_position_time = level.time + 1.7f;
		if (random() < 0.5f)
			VectorMA(self->s.origin, random(), self->velocity, self->monsterinfo.fly_ideal_position);
		else
			VectorAdd(self->monsterinfo.fly_ideal_position, self->enemy->s.origin, self->monsterinfo.fly_ideal_position);
	}
}

void flyer_setstart (edict_t *self)
{
	nextmove = ACTION_run;
	self->monsterinfo.currentmove = &flyer_move_start;
}

void flyer_nextmove (edict_t *self)
{
	if (nextmove == ACTION_attack1)
		self->monsterinfo.currentmove = &flyer_move_start_melee;
	else if (nextmove == ACTION_attack2)
		self->monsterinfo.currentmove = &flyer_move_attack2;
	else if (nextmove == ACTION_run)
		self->monsterinfo.currentmove = &flyer_move_run;
}

void flyer_melee (edict_t *self)
{
	if (!flyer_melee_ready(self))
		return;

	self->monsterinfo.attack_state = AS_MELEE;
	self->monsterinfo.nextattack = 0;
	flyer_set_fly_parameters(self, true);
	self->monsterinfo.currentmove = &flyer_move_start_melee;
}

void flyer_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	int		n;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	if (self->monsterinfo.currentmove == &flyer_move_laser_right ||
		self->monsterinfo.currentmove == &flyer_move_laser_recharge ||
		self->monsterinfo.currentmove == &flyer_move_rollright)
		return;

	self->pain_debounce_time = level.time + 3;
	if (invasion->value == 2)
		return;

	n = rand() % 3;
	if (n == 0)
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &flyer_move_pain1;
	}
	else if (n == 1)
	{
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &flyer_move_pain2;
	}
	else
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &flyer_move_pain3;
	}
}


void flyer_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	gi.sound (self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	flyer_laser_off(self);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	M_Notify(self);
	vrx_throw_drone_gibs(self, 55);
	M_Remove(self, false, false);
}
	

/*QUAKED monster_flyer (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void init_drone_flyer (edict_t *self)
{
	sound_sight = gi.soundindex ("flyer/flysght1.wav");
	sound_idle = gi.soundindex ("flyer/flysrch1.wav");
	sound_pain1 = gi.soundindex ("flyer/flypain1.wav");
	sound_pain2 = gi.soundindex ("flyer/flypain2.wav");
	sound_slash = gi.soundindex ("flyer/flyatck2.wav");
	sound_sproing = gi.soundindex ("flyer/flyatck1.wav");
	sound_die = gi.soundindex ("flyer/flydeth1.wav");
	sound_laser = gi.soundindex ("weapons/laser2.wav");

	gi.soundindex ("flyer/flyatck3.wav");

	self->s.modelindex = gi.modelindex ("models/monsters/flyer/tris.md2");
	gi.imageindex ("models/monsters/flyer/pain.pcx");
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 8);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->s.sound = gi.soundindex ("flyer/flyidle1.wav");

	self->health = M_FLYER_INITIAL_HEALTH + M_FLYER_ADDON_HEALTH*self->monsterinfo.level;
	self->mass = 50;

	self->mtype = M_FLYER;
	self->flags |= FL_FLY;
	self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
	self->monsterinfo.fly_buzzard = true;
	flyer_set_fly_parameters(self, false);
	self->max_health = self->health;
	M_SetMonsterArmor(self, M_FLYER_INITIAL_ARMOR + M_FLYER_ADDON_ARMOR*self->monsterinfo.level);
	self->monsterinfo.control_cost = M_FLYER_CONTROL_COST;
	self->monsterinfo.cost = M_FLYER_COST;
	self->item = FindItemByClassname("ammo_cells");

	self->pain = flyer_pain;
	self->die = flyer_die;

	self->monsterinfo.stand = flyer_stand;
	self->monsterinfo.walk = flyer_walk;
	self->monsterinfo.run = flyer_run;
	self->monsterinfo.attack = flyer_attack;
	self->monsterinfo.melee = flyer_melee;
	self->monsterinfo.sight = flyer_sight;
	self->monsterinfo.idle = flyer_idle;

	gi.linkentity (self);

	self->monsterinfo.currentmove = &flyer_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;

	//flymonster_start (self);
}
