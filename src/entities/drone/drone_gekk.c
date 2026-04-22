/*
==============================================================================

GEKK

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_xatrix_gekk.h"

static int sound_swing;
static int sound_hit;
static int sound_hit2;
static int sound_speet;
static int sound_death;
static int sound_pain1;
static int sound_sight;
static int sound_search;
static int sound_step1;
static int sound_step2;
static int sound_step3;
static int sound_thud;

void drone_ai_stand(edict_t *self, float dist);
void drone_ai_run(edict_t *self, float dist);
void drone_ai_walk(edict_t *self, float dist);
void drone_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);

static void gekk_stand(edict_t *self);
static void gekk_walk(edict_t *self);
static void gekk_run(edict_t *self);
static void gekk_land_to_water(edict_t *self);
static void gekk_water_to_land(edict_t *self);
static void gekk_swim_loop(edict_t *self);
static void gekk_check_landing(edict_t *self);
static void gekk_stop_skid(edict_t *self);

extern mmove_t gekk_move_standunderwater;
extern mmove_t gekk_move_swim_loop;
extern mmove_t gekk_move_swim_start;
extern mmove_t gekk_move_leapatk;

extern void fire_acid(edict_t *self, vec3_t start, vec3_t aimdir, int projectile_damage, float radius,
	int speed, int acid_damage, float acid_duration, int gas_damage, float gas_radius, float gas_duration);

static void gekk_set_leap_cooldown(edict_t *self, float base, float extra)
{
	float cooldown = level.time + base + random() * extra;

	if (self->monsterinfo.dodge_time < cooldown)
		self->monsterinfo.dodge_time = cooldown;
}

static void gekk_step(edict_t *self)
{
	const float r = random();

	if (r < 0.33)
		gi.sound(self, CHAN_BODY, sound_step1, 1, ATTN_NORM, 0);
	else if (r < 0.66)
		gi.sound(self, CHAN_BODY, sound_step2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_BODY, sound_step3, 1, ATTN_NORM, 0);
}

static void gekk_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void gekk_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_IDLE, 0);
}

static qboolean gekk_should_swim(edict_t *self)
{
	return self->waterlevel >= WATER_WAIST;
}

static void gekk_set_water_bounds(edict_t *self)
{
	self->flags |= FL_SWIM;
	self->yaw_speed = 10;
	self->viewheight = 10;
	VectorSet(self->mins, -18, -18, -24);
	VectorSet(self->maxs, 18, 18, 16);
	gi.linkentity(self);
}

static void gekk_set_land_bounds(edict_t *self)
{
	self->flags &= ~FL_SWIM;
	self->yaw_speed = 20;
	self->viewheight = 25;
	VectorSet(self->mins, -18, -18, -24);
	VectorSet(self->maxs, 18, 18, 24);
	gi.linkentity(self);
}

mframe_t gekk_frames_stand[] =
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
	drone_ai_stand, 0, NULL
};
mmove_t gekk_move_stand = { FRAME_stand_01, FRAME_stand_39, gekk_frames_stand, gekk_stand };

static void gekk_stand(edict_t *self)
{
	if (gekk_should_swim(self))
	{
		gekk_set_water_bounds(self);
		self->monsterinfo.currentmove = &gekk_move_standunderwater;
	}
	else
	{
		if (self->flags & FL_SWIM)
			gekk_set_land_bounds(self);
		self->monsterinfo.currentmove = &gekk_move_stand;
	}
}

mframe_t gekk_frames_walk[] =
{
	drone_ai_walk, 8, gekk_step,
	drone_ai_walk, 9, NULL,
	drone_ai_walk, 12, NULL,
	drone_ai_walk, 11, gekk_step,
	drone_ai_walk, 9, NULL,
	drone_ai_walk, 8, NULL
};
mmove_t gekk_move_walk = { FRAME_run_01, FRAME_run_06, gekk_frames_walk, gekk_walk };

static void gekk_walk(edict_t *self)
{
	if (gekk_should_swim(self))
	{
		gekk_land_to_water(self);
		return;
	}
	if (self->flags & FL_SWIM)
		gekk_set_land_bounds(self);

	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &gekk_move_walk;
}

mframe_t gekk_frames_run[] =
{
	drone_ai_run, 18, gekk_step,
	drone_ai_run, 24, NULL,
	drone_ai_run, 28, NULL,
	drone_ai_run, 22, gekk_step,
	drone_ai_run, 20, NULL,
	drone_ai_run, 18, NULL
};
mmove_t gekk_move_run = { FRAME_run_01, FRAME_run_06, gekk_frames_run, gekk_run };

static void gekk_run(edict_t *self)
{
	if (gekk_should_swim(self))
	{
		gekk_land_to_water(self);
		return;
	}
	if (self->flags & FL_SWIM)
		gekk_set_land_bounds(self);

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &gekk_move_stand;
	else
		self->monsterinfo.currentmove = &gekk_move_run;
}

static int gekk_melee_damage(edict_t *self)
{
	int damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self);

	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	return damage;
}

static void gekk_bite(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
	M_MeleeAttack(self, self->enemy, 80, gekk_melee_damage(self), 120);
}

static void gekk_claw(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, (random() < 0.5) ? sound_hit : sound_hit2, 1, ATTN_NORM, 0);
	M_MeleeAttack(self, self->enemy, 96, gekk_melee_damage(self), 180);
}

static void gekk_swim_check(edict_t *self)
{
	if (!gekk_should_swim(self) && (self->flags & FL_SWIM))
		gekk_water_to_land(self);
}

static void gekk_swim_loop(edict_t *self)
{
	if (!gekk_should_swim(self))
	{
		gekk_water_to_land(self);
		return;
	}

	if (G_EntExists(self->enemy) && self->enemy->waterlevel < WATER_WAIST && random() < 0.3f)
	{
		gekk_water_to_land(self);
		return;
	}

	gekk_set_water_bounds(self);
	self->monsterinfo.currentmove = &gekk_move_swim_loop;
}

mframe_t gekk_frames_standunderwater[] =
{
	drone_ai_stand, 14, NULL,
	drone_ai_stand, 14, NULL,
	drone_ai_stand, 14, NULL,
	drone_ai_stand, 14, NULL,
	drone_ai_stand, 16, NULL,
	drone_ai_stand, 16, NULL,
	drone_ai_stand, 16, NULL,
	drone_ai_stand, 18, NULL,
	drone_ai_stand, 18, NULL,
	drone_ai_stand, 18, NULL,
	drone_ai_stand, 20, NULL,
	drone_ai_stand, 20, NULL,
	drone_ai_stand, 22, NULL,
	drone_ai_stand, 22, NULL,
	drone_ai_stand, 24, NULL,
	drone_ai_stand, 24, NULL,
	drone_ai_stand, 26, NULL,
	drone_ai_stand, 26, NULL,
	drone_ai_stand, 24, NULL,
	drone_ai_stand, 24, NULL,
	drone_ai_stand, 22, NULL,
	drone_ai_stand, 22, NULL,
	drone_ai_stand, 22, NULL,
	drone_ai_stand, 22, NULL,
	drone_ai_stand, 22, NULL,
	drone_ai_stand, 22, NULL,
	drone_ai_stand, 22, NULL,
	drone_ai_stand, 22, NULL,
	drone_ai_stand, 18, NULL,
	drone_ai_stand, 18, NULL,
	drone_ai_stand, 18, NULL,
	drone_ai_stand, 18, gekk_swim_check
};
mmove_t gekk_move_standunderwater = { FRAME_swim_01, FRAME_swim_32, gekk_frames_standunderwater, gekk_stand };

mframe_t gekk_frames_swim[] =
{
	drone_ai_run, 14, NULL,
	drone_ai_run, 14, NULL,
	drone_ai_run, 14, NULL,
	drone_ai_run, 14, NULL,
	drone_ai_run, 16, NULL,
	drone_ai_run, 16, NULL,
	drone_ai_run, 16, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 24, NULL,
	drone_ai_run, 24, NULL,
	drone_ai_run, 26, NULL,
	drone_ai_run, 26, NULL,
	drone_ai_run, 24, NULL,
	drone_ai_run, 24, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 18, gekk_swim_check
};
mmove_t gekk_move_swim_loop = { FRAME_swim_01, FRAME_swim_32, gekk_frames_swim, gekk_swim_loop };

mframe_t gekk_frames_swim_start[] =
{
	drone_ai_run, 14, NULL,
	drone_ai_run, 14, NULL,
	drone_ai_run, 14, NULL,
	drone_ai_run, 14, NULL,
	drone_ai_run, 16, NULL,
	drone_ai_run, 16, NULL,
	drone_ai_run, 16, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 18, gekk_claw,
	drone_ai_run, 18, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 24, gekk_claw,
	drone_ai_run, 24, NULL,
	drone_ai_run, 26, NULL,
	drone_ai_run, 26, NULL,
	drone_ai_run, 24, NULL,
	drone_ai_run, 24, NULL,
	drone_ai_run, 22, gekk_bite,
	drone_ai_run, 22, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 22, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 18, gekk_swim_check
};
mmove_t gekk_move_swim_start = { FRAME_swim_01, FRAME_swim_32, gekk_frames_swim_start, gekk_swim_loop };

static void gekk_land_to_water(edict_t *self)
{
	gekk_set_water_bounds(self);
	self->monsterinfo.currentmove = &gekk_move_swim_start;
}

static void gekk_water_to_land(edict_t *self)
{
	vec3_t dir;

	gekk_set_land_bounds(self);
	if (G_EntExists(self->enemy))
	{
		VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
		dir[2] = 0;
		VectorNormalize(dir);
		VectorScale(dir, 450, self->velocity);
		self->velocity[2] = 320;
		self->groundentity = NULL;
		self->monsterinfo.attack_finished = level.time + 1.5;
		self->monsterinfo.currentmove = &gekk_move_leapatk;
		return;
	}

	self->monsterinfo.currentmove = self->goalentity ? &gekk_move_run : &gekk_move_stand;
}

mframe_t gekk_frames_attack[] =
{
	ai_charge, 8, NULL,
	ai_charge, 12, NULL,
	ai_charge, 12, gekk_bite,
	ai_charge, 14, NULL,
	ai_charge, 14, NULL,
	ai_charge, 12, gekk_bite,
	ai_charge, 12, NULL,
	ai_charge, 10, NULL,
	ai_charge, 8, NULL,
	ai_charge, 8, NULL,
	ai_charge, 6, gekk_claw,
	ai_charge, 6, NULL,
	ai_charge, 6, NULL,
	ai_charge, 6, NULL,
	ai_charge, 6, gekk_claw,
	ai_charge, 6, NULL,
	ai_charge, 6, NULL,
	ai_charge, 4, NULL,
	ai_charge, 4, NULL,
	ai_charge, 2, NULL,
	ai_charge, 0, NULL
};
mmove_t gekk_move_attack = { FRAME_attack_01, FRAME_attack_21, gekk_frames_attack, gekk_run };

mframe_t gekk_frames_attack1[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, gekk_claw,
	ai_charge, 0, NULL,
	ai_charge, 0, gekk_claw,
	ai_charge, 0, NULL,
	ai_charge, 0, gekk_claw,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t gekk_move_attack1 = { FRAME_clawatk3_01, FRAME_clawatk3_09, gekk_frames_attack1, gekk_run };

mframe_t gekk_frames_attack2[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, gekk_claw,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, gekk_claw,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t gekk_move_attack2 = { FRAME_clawatk5_01, FRAME_clawatk5_09, gekk_frames_attack2, gekk_run };

static void gekk_melee(edict_t *self)
{
	if (gekk_should_swim(self))
	{
		gekk_land_to_water(self);
		self->monsterinfo.melee_finished = level.time + 1.0;
		return;
	}
	if (self->flags & FL_SWIM)
		gekk_set_land_bounds(self);

	if (!G_ValidTarget(self, self->enemy, true, true) || entdist(self, self->enemy) > 120)
	{
		self->monsterinfo.melee_finished = level.time + 0.5;
		gekk_run(self);
		return;
	}

	gi.sound(self, CHAN_WEAPON, sound_swing, 1, ATTN_NORM, 0);
	if (random() < 0.5)
		self->monsterinfo.currentmove = &gekk_move_attack1;
	else
		self->monsterinfo.currentmove = &gekk_move_attack2;
}

static qboolean gekk_can_leap(edict_t *self)
{
	vec3_t v;
	float distance;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return false;
	if (!self->groundentity)
		return false;
	if (self->monsterinfo.melee_finished > level.time)
		return false;
	if (self->monsterinfo.dodge_time > level.time)
		return false;
	if (!visible(self, self->enemy))
		return false;

	VectorSubtract(self->s.origin, self->enemy->s.origin, v);
	v[2] = 0;
	distance = VectorLength(v);

	if (distance < 100)
	{
		if (self->absmax[2] <= self->enemy->absmin[2])
			return false;
	}
	else if (self->absmin[2] + 125 < self->enemy->absmin[2])
		return false;

	if (self->waterlevel > 1 && random() < 0.2f)
		return false;

	return true;
}

static qboolean gekk_use_high_leap(edict_t *self)
{
	vec3_t v;
	float distance;

	if (!G_EntExists(self->enemy))
		return false;
	if (self->absmin[2] + 125 < self->enemy->absmin[2])
		return false;

	VectorSubtract(self->s.origin, self->enemy->s.origin, v);
	v[2] = 0;
	distance = VectorLength(v);
	if (distance < 100)
		return false;

	return random() >= (self->waterlevel > 1 ? 0.2f : 0.9f);
}

static void gekk_jump_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (self->health <= 0)
	{
		self->touch = drone_touch;
		return;
	}

	if (other && other->takedamage)
		return;

	if (!self->groundentity)
	{
		self->velocity[0] = 0;
		self->velocity[1] = 0;
		if (self->velocity[2] > -200)
			self->velocity[2] = -200;
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		self->monsterinfo.nextframe = FRAME_leapatk_11;
		gekk_set_leap_cooldown(self, 4.0, 2.0);
	}

	self->touch = drone_touch;
}

static void gekk_jump_takeoff(edict_t *self)
{
	vec3_t forward;

	if (!G_EntExists(self->enemy))
		return;

	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
	self->lastsound = level.framenum;
	self->s.origin[2] += 1;
	self->groundentity = NULL;
	AngleVectors(self->s.angles, forward, NULL, NULL);
	if (gekk_use_high_leap(self))
	{
		VectorScale(forward, 700, self->velocity);
		self->velocity[2] = 250;
	}
	else
	{
		VectorScale(forward, 250, self->velocity);
		self->velocity[2] = 400;
	}
	self->monsterinfo.pausetime = level.time + 2.0;
	self->monsterinfo.aiflags |= AI_HOLD_FRAME;
	self->touch = gekk_jump_touch;
	gekk_set_leap_cooldown(self, 1.0, 0.5);
}

static void gekk_stop_skid(edict_t *self)
{
	if (self->groundentity)
	{
		VectorClear(self->velocity);
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		self->touch = drone_touch;
	}
}

static void gekk_check_landing(edict_t *self)
{
	if (self->groundentity || (self->waterlevel > 1) || (level.time > self->monsterinfo.pausetime))
	{
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		self->monsterinfo.melee_finished = level.time + 1.5;
		self->monsterinfo.attack_finished = level.time + 1.0;
		self->monsterinfo.nextframe = FRAME_leapatk_11;
		self->touch = drone_touch;
		gekk_set_leap_cooldown(self, 1.5, 1.0);
		if (!self->groundentity && self->velocity[2] > -200)
			self->velocity[2] = -200;
		if (self->waterlevel > 1)
			gekk_land_to_water(self);
		return;
	}

	self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void gekk_spit(edict_t *self)
{
	int acid_level, damage, speed;
	float radius;
	vec3_t forward, right, start, target, dir, offset;

	if (!G_EntExists(self->enemy))
		return;

	acid_level = drone_damagelevel(self);
	if (acid_level > 15)
		acid_level = 15;

	damage = ACID_INITIAL_DAMAGE + ACID_ADDON_DAMAGE * acid_level;
	speed = ACID_INITIAL_SPEED + ACID_ADDON_SPEED * acid_level;
	radius = ACID_INITIAL_RADIUS + ACID_ADDON_RADIUS * acid_level;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorSet(offset, -18, -1, 24);
	G_ProjectSource(self->s.origin, offset, forward, right, start);

	VectorCopy(self->enemy->s.origin, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, dir);
	VectorNormalize(dir);

	gi.sound(self, CHAN_WEAPON, sound_speet, 1, ATTN_NORM, 0);
	fire_acid(self, start, dir, damage, radius, speed, (int)(0.1 * damage), ACID_DURATION, 0, 0, 0);
}

mframe_t gekk_frames_leapatk[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 6, gekk_jump_takeoff,
	ai_charge, 6, NULL,
	ai_charge, 0, NULL,
	ai_charge, 28, NULL,
	ai_charge, 24, NULL,
	ai_charge, 32, NULL,
	ai_charge, 36, gekk_check_landing,
	ai_charge, 12, gekk_stop_skid,
	ai_charge, 20, gekk_stop_skid,
	ai_charge, -1, gekk_stop_skid,
	ai_charge, 3, gekk_stop_skid,
	ai_charge, 1, gekk_stop_skid,
	ai_charge, 2, gekk_stop_skid,
	ai_charge, 1, gekk_stop_skid,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t gekk_move_leapatk = { FRAME_leapatk_01, FRAME_leapatk_19, gekk_frames_leapatk, gekk_run };

mframe_t gekk_frames_spit[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, gekk_spit,
	ai_charge, 0, NULL
};
mmove_t gekk_move_spit = { FRAME_spit_01, FRAME_spit_07, gekk_frames_spit, gekk_run };

static void gekk_attack(edict_t *self)
{
	if (gekk_should_swim(self))
	{
		if (G_EntExists(self->enemy) && self->enemy->waterlevel < WATER_WAIST)
			gekk_water_to_land(self);
		else
			gekk_land_to_water(self);
		self->monsterinfo.melee_finished = level.time + 1.0;
		M_DelayNextAttack(self, 1.0 + random(), true);
		return;
	}
	if (self->flags & FL_SWIM)
		gekk_set_land_bounds(self);

	if (!G_EntExists(self->enemy))
		return;

	if (gekk_can_leap(self) && random() >= 0.35f)
	{
		self->monsterinfo.currentmove = &gekk_move_leapatk;
		self->monsterinfo.melee_finished = level.time + 3.0;
	}
	else
	{
		self->monsterinfo.currentmove = &gekk_move_spit;
		self->monsterinfo.melee_finished = level.time + 0.5;
	}
	M_DelayNextAttack(self, 1.0 + random(), true);
}

mframe_t gekk_frames_pain[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t gekk_move_pain = { FRAME_pain_01, FRAME_pain_06, gekk_frames_pain, gekk_run };

mframe_t gekk_frames_pain1[] =
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
	ai_move, 0, NULL
};
mmove_t gekk_move_pain1 = { FRAME_pain3_01, FRAME_pain3_11, gekk_frames_pain1, gekk_run };

mframe_t gekk_frames_pain2[] =
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
	ai_move, 0, NULL
};
mmove_t gekk_move_pain2 = { FRAME_pain4_01, FRAME_pain4_13, gekk_frames_pain2, gekk_run };

static void gekk_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0;
	gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);

	if (skill->value == 3)
		return;

	if (self->waterlevel >= 2)
		self->monsterinfo.currentmove = &gekk_move_pain;
	else if (random() < 0.5)
		self->monsterinfo.currentmove = &gekk_move_pain1;
	else
		self->monsterinfo.currentmove = &gekk_move_pain2;
}

static void gekk_dead(edict_t *self)
{
	self->flags &= ~FL_SWIM;
	VectorSet(self->mins, -18, -18, -24);
	VectorSet(self->maxs, 18, 18, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
	M_PrepBodyRemoval(self);
}

static void gekk_shrink(edict_t *self)
{
	self->maxs[2] = -8;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

mframe_t gekk_frames_death1[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, -7, gekk_shrink,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t gekk_move_death1 = { FRAME_death1_01, FRAME_death1_10, gekk_frames_death1, gekk_dead };

static void gekk_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

	M_Notify(self);

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		if (vrx_spawn_nonessential_ent(self->s.origin))
		{
			ThrowGib(self, "models/objects/gekkgib/pelvis/tris.md2", damage, GIB_ORGANIC);
			for (n = 0; n < 2; n++)
				ThrowGib(self, "models/objects/gekkgib/arm/tris.md2", damage, GIB_ORGANIC);
			ThrowGib(self, "models/objects/gekkgib/torso/tris.md2", damage, GIB_ORGANIC);
			ThrowGib(self, "models/objects/gekkgib/claw/tris.md2", damage, GIB_ORGANIC);
			ThrowHead(self, "models/objects/gekkgib/head/tris.md2", damage, GIB_ORGANIC);
		}
		M_Remove(self, false, false);
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	DroneList_Remove(self);
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &gekk_move_death1;

	if (self->activator && !self->activator->client)
		self->activator->num_monsters_real--;
}

void init_drone_gekk(edict_t *self)
{
	sound_swing = gi.soundindex("gek/gk_atck1.wav");
	sound_hit = gi.soundindex("gek/gk_atck2.wav");
	sound_hit2 = gi.soundindex("gek/gk_atck3.wav");
	sound_speet = gi.soundindex("gek/gk_atck4.wav");
	sound_death = gi.soundindex("gek/gk_deth1.wav");
	sound_pain1 = gi.soundindex("gek/gk_pain1.wav");
	sound_sight = gi.soundindex("gek/gk_sght1.wav");
	sound_search = gi.soundindex("gek/gk_idle1.wav");
	sound_step1 = gi.soundindex("gek/gk_step1.wav");
	sound_step2 = gi.soundindex("gek/gk_step2.wav");
	sound_step3 = gi.soundindex("gek/gk_step3.wav");
	sound_thud = gi.soundindex("mutant/thud1.wav");
	gi.soundindex("gek/loogie_hit.wav");
	gi.soundindex("weapons/rocklx1a.wav");
	gi.modelindex("models/objects/loogy/tris.md2");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/gekk/tris.md2");
	VectorSet(self->mins, -18, -18, -24);
	VectorSet(self->maxs, 18, 18, 24);
	self->viewheight = 25;

	gi.modelindex("models/objects/gekkgib/pelvis/tris.md2");
	gi.modelindex("models/objects/gekkgib/arm/tris.md2");
	gi.modelindex("models/objects/gekkgib/torso/tris.md2");
	gi.modelindex("models/objects/gekkgib/claw/tris.md2");
	gi.modelindex("models/objects/gekkgib/leg/tris.md2");
	gi.modelindex("models/objects/gekkgib/head/tris.md2");

	self->health = M_GEKK_INITIAL_HEALTH + M_GEKK_ADDON_HEALTH * self->monsterinfo.level;
	self->max_health = self->health;
	self->gib_health = -30;
	self->mass = 300;
	self->mtype = M_GEKK;

	self->monsterinfo.power_armor_power = M_GEKK_INITIAL_ARMOR + M_GEKK_ADDON_ARMOR * self->monsterinfo.level;
	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
	self->monsterinfo.control_cost = M_MUTANT_CONTROL_COST;
	self->monsterinfo.cost = M_MUTANT_COST;
	self->monsterinfo.jumpdn = 256;
	self->monsterinfo.jumpup = 88;

	self->item = FindItemByClassname("ammo_cells");

	self->pain = gekk_pain;
	self->die = gekk_die;
	self->monsterinfo.stand = gekk_stand;
	self->monsterinfo.walk = gekk_walk;
	self->monsterinfo.run = gekk_run;
	self->monsterinfo.attack = gekk_attack;
	self->monsterinfo.melee = gekk_melee;
	self->monsterinfo.sight = gekk_sight;
	self->monsterinfo.idle = gekk_search;
	self->monsterinfo.pain_chance = 0.2f;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &gekk_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;
	self->nextthink = level.time + FRAMETIME;
}
