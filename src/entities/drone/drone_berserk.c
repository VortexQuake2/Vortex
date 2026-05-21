/*
==============================================================================

BERSERK

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_berserk.h"


static int sound_pain;
static int sound_die;
static int sound_idle;
static int sound_punch;
static int sound_sight;
static int sound_search;
static int sound_thud;
static int sound_explod;
static int sound_jump;

static void berserk_ai_dodge_slide(edict_t *self, float dist);
static void berserk_duck_up(edict_t *self);
static void berserk_slam_touchdown(edict_t *self);
void berserk_melee(edict_t *self);
extern mmove_t berserk_move_attack_strike;

#define BERSERK_CLOSE_MELEE_RANGE	MELEE_DISTANCE
#define BERSERK_RUN_ATTACK_RANGE	500.0f
#define BERSERK_SLAM_MIN_RANGE		150.0f
#define BERSERK_SLAM_COOLDOWN		5.0f
#define BERSERK_SLAM_TIMEOUT		3.0f
#define BERSERK_SLAM_RADIUS			165.0f
#define BERSERK_SLAM_KICK			300.0f


void berserk_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void berserk_search (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}


void berserk_fidget (edict_t *self);
mframe_t berserk_frames_stand [] =
{
	drone_ai_stand, 0, berserk_fidget,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL
};
mmove_t berserk_move_stand = {FRAME_stand1, FRAME_stand5, berserk_frames_stand, NULL};


void berserk_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &berserk_move_stand;
}

mframe_t berserk_frames_stand_fidget [] =
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
	drone_ai_stand, 0, NULL
};
mmove_t berserk_move_stand_fidget = {FRAME_standb1, FRAME_standb20, berserk_frames_stand_fidget, berserk_stand};

void berserk_fidget (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;
	if (random() > 0.15)
		return;

	self->monsterinfo.currentmove = &berserk_move_stand_fidget;
	gi.sound (self, CHAN_WEAPON, sound_idle, 1, ATTN_IDLE, 0);
}

mframe_t berserk_frames_walk [] =
{
	drone_ai_walk, 9.1, NULL,
	drone_ai_walk, 6.3, NULL,
	drone_ai_walk, 4.9, NULL,
	drone_ai_walk, 6.7, NULL,
	drone_ai_walk, 6.0, NULL,
	drone_ai_walk, 8.2, NULL,
	drone_ai_walk, 7.2, NULL,
	drone_ai_walk, 6.1, NULL,
	drone_ai_walk, 4.9, NULL,
	drone_ai_walk, 4.7, NULL,
	drone_ai_walk, 4.7, NULL,
	drone_ai_walk, 4.8, NULL
};
mmove_t berserk_move_walk = {FRAME_walkc1, FRAME_walkc11, berserk_frames_walk, NULL};

void berserk_walk (edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &berserk_move_walk;
}

mframe_t berserk_frames_run1 [] =
{
	drone_ai_run, 35, NULL,
	drone_ai_run, 35, NULL,
	drone_ai_run, 35, NULL,
	drone_ai_run, 35, NULL,
	drone_ai_run, 35, NULL,
	drone_ai_run, 35, NULL
};
mmove_t berserk_move_run1 = {FRAME_run1, FRAME_run6, berserk_frames_run1, NULL};

void berserk_run (edict_t *self)
{
	berserk_duck_up(self);

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &berserk_move_stand;
	else
		self->monsterinfo.currentmove = &berserk_move_run1;
}

void berserk_attack_spike (edict_t *self)
{
	int damage;

	if (!G_EntExists(self->enemy))
		return;

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self); // dmg: berserker_attack_spike
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	M_MeleeAttack(self, self->enemy, 96, damage, 400);
	//FIXME: add bleed curse
}

void berserk_swing (edict_t *self)
{
	//gi.dprintf("played sound at %d on frame %d\n", level.framenum, self->s.frame);
	gi.sound (self, CHAN_WEAPON, sound_punch, 1, ATTN_NORM, 0); // doesn't make noises on static on repro - testing
}

mframe_t berserk_frames_attack_spike [] =
{
	ai_charge, 0, berserk_swing,
	ai_charge, 0, berserk_attack_spike,
	ai_charge, 0, berserk_attack_spike,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t berserk_move_attack_spike = {FRAME_att_c1, FRAME_att_c8, berserk_frames_attack_spike, berserk_run};

void berserk_attack_club (edict_t *self)
{
	int		damage;

	if (!G_EntExists(self->enemy))
		return;

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self); // dmg: berserker_attack_club
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	M_MeleeAttack(self, self->enemy, 96, damage, 400);
}

mframe_t berserk_frames_attack_club [] =
{	
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, berserk_swing,
	ai_charge, 0, NULL,
	ai_charge, 0, berserk_attack_club,
	ai_charge, 0, berserk_attack_club,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t berserk_move_attack_club = {FRAME_att_c9, FRAME_att_c20, berserk_frames_attack_club, berserk_run};

static void berserk_run_attack_speed(edict_t *self)
{
	if (!G_EntExists(self->enemy))
	{
		berserk_run(self);
		return;
	}

	if (entdist(self, self->enemy) <= BERSERK_CLOSE_MELEE_RANGE)
		self->monsterinfo.nextframe = self->s.frame + 6;
}

static void berserk_run_swing(edict_t *self)
{
	berserk_swing(self);
	self->monsterinfo.melee_finished = level.time + 0.6f;
}

mframe_t berserk_frames_runattack1 [] =
{	
	drone_ai_run, 21, berserk_run_attack_speed,
	drone_ai_run, 11, berserk_run_attack_speed,
	drone_ai_run, 21, berserk_run_attack_speed,
	drone_ai_run, 25, berserk_run_attack_speed,
	drone_ai_run, 18, berserk_run_attack_speed,
	drone_ai_run, 19, berserk_run_attack_speed,
	drone_ai_run, 21, NULL,
	drone_ai_run, 11, NULL,
	drone_ai_run, 21, NULL,
	drone_ai_run, 25, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 19, NULL,
	drone_ai_run, 21, berserk_run_swing,
	drone_ai_run, 11, NULL,
	drone_ai_run, 21, NULL,
	drone_ai_run, 25, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 19, berserk_attack_club
};
mmove_t berserk_move_runattack1 = {FRAME_r_att1, FRAME_r_att18, berserk_frames_runattack1, berserk_run};


static int berserk_slam_damage_value(edict_t *self)
{
	int damage;

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self); // dmg: berserker_attack_strike
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	return damage;
}

static float berserk_clamp_float(float value, float min_value, float max_value)
{
	if (value < min_value)
		return min_value;
	if (value > max_value)
		return max_value;
	return value;
}

static void berserk_closest_point_to_box(vec3_t point, edict_t *ent, vec3_t out)
{
	out[0] = berserk_clamp_float(point[0], ent->absmin[0], ent->absmax[0]);
	out[1] = berserk_clamp_float(point[1], ent->absmin[1], ent->absmax[1]);
	out[2] = berserk_clamp_float(point[2], ent->absmin[2], ent->absmax[2]);
}

static void berserk_slam_origin(edict_t *self, vec3_t origin)
{
	vec3_t forward, right, offset;
	trace_t tr;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorSet(offset, 20, -14.3f, -21);
	G_ProjectSource(self->s.origin, offset, forward, right, origin);
	tr = gi.trace(self->s.origin, NULL, NULL, origin, self, MASK_SOLID);
	VectorCopy(tr.endpos, origin);
}

static void berserk_slam_effect(vec3_t origin)
{
	vec3_t up;

	VectorSet(up, 0, 0, 1);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BERSERK_SLAM);
	gi.WritePosition(origin);
	gi.WriteDir(up);
	gi.multicast(origin, MULTICAST_PHS);
}

static void berserk_attack_strike(edict_t *self)
{
	int damage;
	trace_t tr;
	edict_t *other = NULL;
	vec3_t closest, damage_origin, dir, v;

	if (self->monsterinfo.lefty)
		return;
	self->monsterinfo.lefty = 1;

	damage = berserk_slam_damage_value(self);

	gi.sound(self, CHAN_WEAPON, sound_thud, 1, ATTN_NORM, 0);
	gi.sound(self, CHAN_AUTO, sound_explod, 0.75f, ATTN_NORM, 0);
	berserk_slam_origin(self, damage_origin);
	berserk_slam_effect(damage_origin);

	while ((other = findradius(other, damage_origin, BERSERK_SLAM_RADIUS * 2.0f)) != NULL)
	{
		float amount, distance, points;
		vec3_t point;

		if (!G_ValidTarget(self, other, true, true))
			continue;
		if (!CanDamage(other, self))
			continue;
		// miss the attack if we are cursed/confused
		if (que_typeexists(self->curses, CURSE) && rand() > 0.2)
			continue;

		berserk_closest_point_to_box(damage_origin, other, closest);
		VectorSubtract(closest, damage_origin, v);
		distance = VectorLength(v);
		amount = 1.0f - (distance / BERSERK_SLAM_RADIUS);
		if (amount <= 0)
			continue;
		amount *= amount;
		points = damage * amount;
		if (points < 1)
			points = 1;

		VectorSubtract(other->s.origin, damage_origin, dir);
		if (VectorNormalize(dir) == 0)
			VectorSet(dir, 0, 0, 1);
		VectorCopy(damage_origin, point);
		point[2] = other->absmin[2];

		tr = gi.trace(damage_origin, NULL, NULL, other->s.origin, self, (MASK_PLAYERSOLID | MASK_MONSTERSOLID));
		T_Damage(other, self, self, dir, point, tr.plane.normal, (int)points,
			(int)(BERSERK_SLAM_KICK * amount), 0, MOD_TANK_PUNCH);
		if (other->inuse && other->client && other->velocity[2] < 270)
			other->velocity[2] = 270;
	}
}

static void berserk_high_gravity(edict_t *self)
{
	self->gravity = self->velocity[2] < 0 ? 2.25f : 5.25f;
}

static qboolean berserk_can_slam(edict_t *self, float dist)
{
	return G_ValidTarget(self, self->enemy, true, true) &&
		self->groundentity && level.time >= self->timestamp &&
		dist > BERSERK_SLAM_MIN_RANGE;
}

static void berserk_finish_slam(edict_t *self, qboolean damage)
{
	self->monsterinfo.aiflags &= ~(AI_HOLD_FRAME | AI_DUCKED);
	self->monsterinfo.touchdown = NULL;
	self->gravity = 1.0f;
	VectorClear(self->velocity);
	self->monsterinfo.attack_finished = level.time + 0.6f;
	self->monsterinfo.melee_finished = level.time + 0.6f;
	if (damage)
		berserk_attack_strike(self);
	self->s.frame = FRAME_slam18;
	gi.linkentity(self);
}

static void berserk_slam_touchdown(edict_t *self)
{
	if (self->health <= 0)
	{
		self->monsterinfo.touchdown = NULL;
		return;
	}
	if (self->monsterinfo.currentmove == &berserk_move_attack_strike)
		berserk_finish_slam(self, true);
}

static void berserk_jump_takeoff(edict_t *self)
{
	float dist, speed;
	vec3_t dir, forward;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
	dist = VectorLength(dir);
	if (dist < 1)
		dist = 1;
	self->s.angles[YAW] = vectoyaw(dir);
	AngleVectors(self->s.angles, forward, NULL, NULL);

	speed = dist * 1.95f;
	if (speed < 350)
		speed = 350;
	else if (speed > 1200)
		speed = 1200;

	self->s.origin[2] += 1;
	VectorScale(forward, speed, self->velocity);
	self->velocity[2] = 400;
	self->groundentity = NULL;
	self->monsterinfo.aiflags |= AI_DUCKED;
	self->monsterinfo.attack_finished = level.time + BERSERK_SLAM_TIMEOUT;
	self->monsterinfo.touchdown = berserk_slam_touchdown;
	self->monsterinfo.lefty = 0;
	berserk_high_gravity(self);
	gi.linkentity(self);
}

static void berserk_check_landing(edict_t *self)
{
	berserk_high_gravity(self);

	if (self->groundentity)
	{
		berserk_finish_slam(self, true);
		return;
	}

	if (level.time > self->monsterinfo.attack_finished)
	{
		berserk_finish_slam(self, false);
		return;
	}

	self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

mframe_t berserk_frames_attack_strike [] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_move, 0, berserk_jump_takeoff,
	ai_move, 0, berserk_high_gravity,
	ai_move, 0, berserk_check_landing,
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
	
mmove_t berserk_move_attack_strike = {FRAME_slam1, FRAME_slam23, berserk_frames_attack_strike, berserk_run};

static void berserk_start_slam(edict_t *self)
{
	self->timestamp = level.time + BERSERK_SLAM_COOLDOWN;
	self->monsterinfo.lefty = 0;
	self->monsterinfo.attack_finished = level.time + BERSERK_SLAM_TIMEOUT;
	self->monsterinfo.melee_finished = level.time + 0.6f;
	gi.sound(self, CHAN_WEAPON, sound_jump, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &berserk_move_attack_strike;
}

void berserk_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity (self);
	M_PrepBodyRemoval(self);
}

static void berserk_shrink(edict_t *self)
{
	self->maxs[2] = 0;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

mframe_t berserk_frames_death1 [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, berserk_shrink,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
	
};
mmove_t berserk_move_death1 = {FRAME_death1, FRAME_death13, berserk_frames_death1, berserk_dead};


mframe_t berserk_frames_death2 [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, berserk_shrink,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t berserk_move_death2 = {FRAME_deathc1, FRAME_deathc8, berserk_frames_death2, berserk_dead};

mframe_t berserk_frames_pain_short[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t berserk_move_pain_short = { FRAME_painc1, FRAME_painc4, berserk_frames_pain_short, berserk_run };

mframe_t berserk_frames_pain_long[] =
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
};
mmove_t berserk_move_pain_long = { FRAME_painb1, FRAME_painb20, berserk_frames_pain_long, berserk_run };

#define BERSERK_SCALE(self)             ((self)->s.scale > 0 ? (self)->s.scale : 1.0f)
#define BERSERK_STAND_MAX_Z_SCALED(self) (32.0f * BERSERK_SCALE(self))
#define BERSERK_DUCK_MAX_Z_SCALED(self)  (0.0f  * BERSERK_SCALE(self))

static void berserk_duck_down(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;
	if (!self->groundentity)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] = BERSERK_DUCK_MAX_Z_SCALED(self);
	self->takedamage = DAMAGE_YES;
	gi.linkentity(self);
}

static void berserk_duck_hold(edict_t *self)
{
	if (self->monsterinfo.pausetime > level.time)
		self->monsterinfo.nextframe = self->s.frame;
}

static void berserk_duck_up(edict_t *self)
{
	vec3_t oldmaxs;
	trace_t tr;

	if (!(self->monsterinfo.aiflags & AI_DUCKED) && self->maxs[2] == BERSERK_STAND_MAX_Z_SCALED(self))
		return;

	VectorCopy(self->maxs, oldmaxs);
	self->maxs[2] = BERSERK_STAND_MAX_Z_SCALED(self);

	tr = gi.trace(self->s.origin, self->mins, self->maxs, self->s.origin, self, MASK_MONSTERSOLID);

	if (tr.startsolid || tr.allsolid)
	{
		VectorCopy(oldmaxs, self->maxs);
		self->monsterinfo.aiflags |= AI_DUCKED;
		return;
	}

	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity(self);
}

mframe_t berserk_frames_dodge_slide[] =
{
	berserk_ai_dodge_slide, 21, NULL,
	berserk_ai_dodge_slide, 11, NULL,
	berserk_ai_dodge_slide, 21, NULL,
	berserk_ai_dodge_slide, 25, NULL,
	berserk_ai_dodge_slide, 18, NULL,
	berserk_ai_dodge_slide, 19, NULL
};
mmove_t berserk_move_dodge_slide = { FRAME_run1, FRAME_run6, berserk_frames_dodge_slide, berserk_run };

mframe_t berserk_frames_dodge_duck[] =
{
	ai_move, 21, berserk_duck_down,
	ai_move, 28, NULL,
	ai_move, 20, NULL,
	ai_move, 12, NULL,
	ai_move, 7, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, berserk_duck_hold,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, berserk_duck_up,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t berserk_move_dodge_duck = { FRAME_fall2, FRAME_fall18, berserk_frames_dodge_duck, berserk_run };

mframe_t berserk_frames_dodge_duck_short[] =
{
	ai_move, 0, berserk_duck_down,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, berserk_duck_hold,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, berserk_duck_up,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t berserk_move_dodge_duck_short = { FRAME_duck1, FRAME_duck10, berserk_frames_dodge_duck_short, berserk_run };

static qboolean berserk_is_dodge_move(edict_t *self)
{
	return self->monsterinfo.currentmove == &berserk_move_dodge_slide ||
		self->monsterinfo.currentmove == &berserk_move_dodge_duck ||
		self->monsterinfo.currentmove == &berserk_move_dodge_duck_short;
}

static void berserk_ai_dodge_slide(edict_t *self, float dist)
{
	if (!G_EntIsAlive(self->enemy) && G_EntIsAlive(self->monsterinfo.attacker))
		self->enemy = self->monsterinfo.attacker;
	if (!G_EntIsAlive(self->enemy))
		return;

	drone_ai_dodge_slide(self, dist);
}

void berserk_dodge(edict_t *self, edict_t *attacker, vec3_t dir, int radius)
{
	if (level.time < self->monsterinfo.dodge_time)
		return;
	if (!attacker)
		return;
	if (OnSameTeam(self, attacker))
		return;
	if (berserk_is_dodge_move(self) ||
		self->monsterinfo.currentmove == &berserk_move_attack_strike ||
		self->monsterinfo.currentmove == &berserk_move_pain_long)
		return;
	if (!self->groundentity)
		return;

	if (!G_EntIsAlive(self->enemy))
	{
		if (!G_EntIsAlive(attacker))
			return;
		self->enemy = attacker;
	}
	self->monsterinfo.attacker = attacker;

	if (random() > 0.5f)
		return;

	if (radius || random() < 0.05f)
	{
		self->monsterinfo.pausetime = level.time + 0.5f;

		if (radius && random() < 0.5f)
		{
			self->monsterinfo.currentmove = &berserk_move_dodge_duck_short;
			berserk_duck_down(self);
			self->monsterinfo.dodge_time = level.time + 1.0f;
		}
		else
		{
			self->monsterinfo.currentmove = &berserk_move_dodge_duck;
			berserk_duck_down(self);
			self->monsterinfo.dodge_time = level.time + 1.7f;
		}

		return;
	}

	if (random() < 0.25f)
	{
		self->monsterinfo.pausetime = level.time + 0.5f;
		self->monsterinfo.currentmove = &berserk_move_dodge_duck_short;
		berserk_duck_down(self);
		self->monsterinfo.dodge_time = level.time + 1.0f;
		return;
	}

	drone_set_dodge_side(self, dir);
	self->monsterinfo.currentmove = &berserk_move_dodge_slide;
	self->monsterinfo.dodge_time = level.time + 0.4f + random() * 1.6f;
}

void berserk_pain(edict_t* self, edict_t* other, float kick, int damage)
{
	const double rng = random();
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	// we're already in a pain state
	if (self->monsterinfo.currentmove == &berserk_move_pain_long ||
		self->monsterinfo.currentmove == &berserk_move_pain_short ||
		berserk_is_dodge_move(self))
		return;

	// monster players don't get pain state induced
	if (G_GetClient(self))
		return;

	// no pain in invasion hard mode
	if (invasion->value == 2)
		return;

	// if we're fidgeting, always go into pain state.
	if (rng <= (1.0f - self->monsterinfo.pain_chance) &&
		self->monsterinfo.currentmove != &berserk_move_stand &&
		self->monsterinfo.currentmove != &berserk_move_stand_fidget &&
		self->monsterinfo.currentmove != &berserk_move_walk)
		return;

	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (self->monsterinfo.currentmove == &berserk_move_stand ||
		self->monsterinfo.currentmove == &berserk_move_stand_fidget ||
		self->monsterinfo.currentmove == &berserk_move_walk)
		self->monsterinfo.currentmove = &berserk_move_pain_long;
	else
		self->monsterinfo.currentmove = &berserk_move_pain_short;
}

void berserk_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	M_Notify(self);

#ifdef OLD_NOLAG_STYLE
    if (nolag->value)
	{
		M_Remove(self, false, true);
		return;
	}
#endif



	if (self->health <= self->gib_health)
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		vrx_throw_drone_gibs(self, damage);
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
	gi.sound (self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.touchdown = NULL;
	self->touch = NULL;
	self->gravity = 1.0f;
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	vrx_update_drone_death_skin(self);

	if (damage >= 50)
		self->monsterinfo.currentmove = &berserk_move_death1;
	else
		self->monsterinfo.currentmove = &berserk_move_death2;

	if (self->activator && !self->activator->client)
	{
		self->activator->num_monsters_real--;
		// gi.bprintf(PRINT_HIGH, "releasing %p (%d)\n", self, self->activator->num_monsters_real);
	}
}

void berserk_attack (edict_t *self)
{
	const float	dist = entdist(self, self->enemy);

	if (!G_EntExists(self->enemy))
		return;

	if (self->monsterinfo.melee_finished <= level.time && dist <= MELEE_DISTANCE)
	{
		berserk_melee(self);
		return;
	}

	if (berserk_can_slam(self, dist) && random() <= 0.5f)
	{
		berserk_start_slam(self);
		return;
	}

	if (!(self->monsterinfo.aiflags & AI_STAND_GROUND) &&
		self->monsterinfo.currentmove == &berserk_move_run1 &&
		dist <= BERSERK_RUN_ATTACK_RANGE)
	{
		self->monsterinfo.currentmove = &berserk_move_runattack1;
		if (self->s.frame >= FRAME_run1 && self->s.frame <= FRAME_run6)
			self->monsterinfo.nextframe = FRAME_r_att1 + (self->s.frame - FRAME_run1);
		self->monsterinfo.attack_finished = level.time + 0.6f;
	}
}

static void berserk_choose_melee_attack(edict_t *self)
{
	if (random() <= 0.5f)
		self->monsterinfo.currentmove = &berserk_move_attack_spike;
	else
		self->monsterinfo.currentmove = &berserk_move_attack_club;

	self->monsterinfo.melee_finished = level.time + 0.6f;
	self->monsterinfo.attack_finished = level.time + 0.6f;
}

void berserk_melee (edict_t *self)
{
	float dist;

	if (!G_EntExists(self->enemy))
		return;

	dist = entdist(self, self->enemy);
	if (self->monsterinfo.melee_finished > level.time)
		return;

	if (dist <= MELEE_DISTANCE)
		berserk_choose_melee_attack(self);
	else if (berserk_can_slam(self, dist) && random() <= 0.5f)
		berserk_start_slam(self);
	else
		return;
}

/*QUAKED monster_berserk (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void init_drone_berserk (edict_t *self)
{
	sound_pain  = gi.soundindex ("berserk/berpain2.wav");
	sound_die   = gi.soundindex ("berserk/berdeth2.wav");
	sound_idle  = gi.soundindex ("berserk/beridle1.wav");
	sound_punch = gi.soundindex ("berserk/attack.wav");
	sound_search = gi.soundindex ("berserk/bersrch1.wav");
	sound_sight = gi.soundindex ("berserk/sight.wav");
	sound_thud = gi.soundindex("mutant/thud1.wav");
	sound_explod = gi.soundindex("world/explod2.wav");
	sound_jump = gi.soundindex("berserk/jump.wav");

	self->s.modelindex = gi.modelindex("models/monsters/berserk/tris.md2");

	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 32);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->health = self->max_health = M_BERSERKER_INITIAL_HEALTH + M_BERSERKER_ADDON_HEALTH * self->monsterinfo.level; // hlt: berserker
	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	self->monsterinfo.power_armor_power = self->monsterinfo.max_armor = M_BERSERKER_INITIAL_ARMOR + M_BERSERKER_ADDON_ARMOR * self->monsterinfo.level; // pow: berserker
	self->gib_health = -0.6 * BASE_GIB_HEALTH;
	self->mass = 250;
	self->monsterinfo.control_cost = M_BERSERKER_CONTROL_COST;
	self->monsterinfo.cost = M_DEFAULT_COST;//FIXME
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.touchdown = NULL;
	self->monsterinfo.lefty = 0;
	self->gravity = 1.0f;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;
	self->mtype = M_BERSERK;

	self->pain = berserk_pain;
	self->monsterinfo.pain_chance = 0.15f;

	self->die = berserk_die;

	self->monsterinfo.stand = berserk_stand;
	self->monsterinfo.walk = berserk_walk;
	self->monsterinfo.run = berserk_run;
	self->monsterinfo.dodge = berserk_dodge;
	self->monsterinfo.attack = berserk_attack;
	self->monsterinfo.melee = berserk_melee;
	self->monsterinfo.sight = berserk_sight;
	self->monsterinfo.idle = berserk_search;

	self->monsterinfo.currentmove = &berserk_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

	gi.linkentity (self);

	//self->nextthink = level.time + FRAMETIME;
}
