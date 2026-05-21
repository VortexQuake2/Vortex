/*
==============================================================================

STALKER

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_rogue_stalker.h"

static int sound_pain;
static int sound_die;
static int sound_sight;
static int sound_punch_hit1;
static int sound_punch_hit2;
static int sound_idle;

#define STALKER_CEILING_NONE 0
#define STALKER_CEILING_ON 1
#define STALKER_CEILING_JUMPING 2
#define STALKER_CEILING_TRACE_DIST 256
#define STALKER_CEILING_JUMP_SPEED 550
#define STALKER_CEILING_MIN_SPEED 360

void drone_ai_stand(edict_t *self, float dist);
void drone_ai_run(edict_t *self, float dist);
void drone_ai_run_slide(edict_t *self, float dist);
void drone_ai_walk(edict_t *self, float dist);

static void stalker_stand(edict_t *self);
static void stalker_run(edict_t *self);
static void stalker_set_floor(edict_t *self);
static void stalker_jump_wait_land(edict_t *self);
extern mmove_t stalker_move_jump_straightup;
static mmove_t stalker_move_dodge_run;

static void stalker_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void stalker_idle_noise(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 0.5, ATTN_IDLE, 0);
}

mframe_t stalker_frames_stand[] =
{
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, stalker_idle_noise,
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
mmove_t stalker_move_stand = { FRAME_idle01, FRAME_idle21, stalker_frames_stand, stalker_stand };

mframe_t stalker_frames_idle2[] =
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
	drone_ai_stand, 0, NULL
};
mmove_t stalker_move_idle2 = { FRAME_idle201, FRAME_idle213, stalker_frames_idle2, stalker_stand };

static void stalker_idle(edict_t *self)
{
	if (random() < 0.35f)
		self->monsterinfo.currentmove = &stalker_move_stand;
	else
		self->monsterinfo.currentmove = &stalker_move_idle2;
}

static void stalker_stand(edict_t *self)
{
	if (random() < 0.25f)
		self->monsterinfo.currentmove = &stalker_move_stand;
	else
		self->monsterinfo.currentmove = &stalker_move_idle2;
}

mframe_t stalker_frames_walk[] =
{
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 10, NULL
};
mmove_t stalker_move_walk = { FRAME_walk01, FRAME_walk08, stalker_frames_walk, stalker_run };

static void stalker_walk(edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &stalker_move_walk;
}

static void stalker_reactivate(edict_t *self);
static void stalker_false_death(edict_t *self);

static mframe_t stalker_frames_reactivate[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
static mmove_t stalker_move_false_death_end = { FRAME_reactive01, FRAME_reactive04, stalker_frames_reactivate, stalker_run };

static void stalker_reactivate(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_STAND_GROUND;
	self->monsterinfo.currentmove = &stalker_move_false_death_end;
}

static void stalker_heal(edict_t *self)
{
	if (skill->value >= 3)
		self->health += 3;
	else if (skill->value >= 2)
		self->health += 2;
	else
		self->health++;

	self->s.skinnum = self->health < (self->max_health / 2);

	if (self->health >= self->max_health)
	{
		self->health = self->max_health;
		stalker_reactivate(self);
	}
}

static mframe_t stalker_frames_false_death[] =
{
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal
};
static mmove_t stalker_move_false_death = { FRAME_twitch01, FRAME_twitch10, stalker_frames_false_death, stalker_false_death };

static void stalker_false_death(edict_t *self)
{
	self->monsterinfo.currentmove = &stalker_move_false_death;
}

static mframe_t stalker_frames_false_death_start[] =
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
static mmove_t stalker_move_false_death_start = { FRAME_death01, FRAME_death09, stalker_frames_false_death_start, stalker_false_death };

static void stalker_false_death_start(edict_t *self)
{
	stalker_set_floor(self);
	self->monsterinfo.aiflags |= AI_STAND_GROUND;
	self->monsterinfo.currentmove = &stalker_move_false_death_start;
}

mframe_t stalker_frames_run[] =
{
	drone_ai_run, 24, NULL,
	drone_ai_run, 28, NULL,
	drone_ai_run, 32, NULL,
	drone_ai_run, 28, NULL
};
mmove_t stalker_move_run = { FRAME_run01, FRAME_run04, stalker_frames_run, NULL };

static void stalker_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &stalker_move_stand;
	else
		self->monsterinfo.currentmove = &stalker_move_run;
}

static qboolean stalker_dodge_allowed(edict_t *self)
{
	return self && self->health > 0 && self->deadflag == DEAD_NO;
}

static qboolean stalker_ceiling_allowed(edict_t *self)
{
	return stalker_dodge_allowed(self) && !invasion->value;
}

static qboolean stalker_on_ceiling(edict_t *self)
{
	return self->style == STALKER_CEILING_ON;
}

static qboolean stalker_find_ceiling(edict_t *self, float max_dist, float *ceiling_z)
{
	trace_t tr;
	vec3_t end;

	VectorCopy(self->s.origin, end);
	end[2] += max_dist;
	tr = gi.trace(self->s.origin, self->mins, self->maxs, end, self, MASK_MONSTERSOLID);

	if (tr.fraction == 1.0 || !(tr.contents & CONTENTS_SOLID) || tr.ent != world)
		return false;
	if (tr.plane.normal[2] > -0.7)
		return false;

	if (ceiling_z)
		*ceiling_z = tr.endpos[2] + self->maxs[2];
	return true;
}

static void stalker_attach_ceiling(edict_t *self, float ceiling_z)
{
	self->style = STALKER_CEILING_ON;
	self->flags |= FL_FLY;
	self->gravity = 0;
	self->groundentity = NULL;
	self->s.angles[ROLL] = 180;
	self->s.origin[2] = ceiling_z - self->maxs[2] - 1;
	VectorClear(self->velocity);
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	gi.linkentity(self);
}

static void stalker_set_floor(edict_t *self)
{
	self->style = STALKER_CEILING_NONE;
	self->flags &= ~FL_FLY;
	self->gravity = 1.0;
	self->s.angles[ROLL] = 0;
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
}

static void stalker_drop_from_ceiling(edict_t *self)
{
	stalker_set_floor(self);
	self->groundentity = NULL;
	self->velocity[2] = -300;
	gi.linkentity(self);
}

static void stalker_abort_ceiling_jump(edict_t *self)
{
	stalker_set_floor(self);
	self->gravity = 1.0;
	self->flags &= ~FL_FLY;
	self->monsterinfo.dodge_time = level.time + 2.0f;
	if (self->groundentity)
		VectorClear(self->velocity);
	else if (self->velocity[2] > 0)
		self->velocity[2] = 0;
	stalker_run(self);
}

static qboolean stalker_is_jump_move(edict_t *self)
{
	return self->monsterinfo.currentmove == &stalker_move_jump_straightup ||
		self->style == STALKER_CEILING_JUMPING;
}

static qboolean stalker_is_dodge_move(edict_t *self)
{
	return stalker_is_jump_move(self) ||
		self->monsterinfo.currentmove == &stalker_move_dodge_run;
}

static void stalker_ceiling_prethink(edict_t *self)
{
	float ceiling_z;

	if (self->style == STALKER_CEILING_JUMPING)
	{
		stalker_jump_wait_land(self);
		return;
	}

	if (!stalker_on_ceiling(self))
		return;

	if (!stalker_ceiling_allowed(self))
	{
		stalker_set_floor(self);
		return;
	}

	if (stalker_find_ceiling(self, 96, &ceiling_z))
		stalker_attach_ceiling(self, ceiling_z);
	else
		stalker_drop_from_ceiling(self);
}

static void stalker_jump_straightup(edict_t *self)
{
	float ceiling_z;

	if (!stalker_ceiling_allowed(self))
		return;

	if (stalker_on_ceiling(self))
	{
		stalker_set_floor(self);
		self->velocity[2] = -300;
		return;
	}

	if (!self->groundentity)
		return;

	if (stalker_find_ceiling(self, STALKER_CEILING_TRACE_DIST, &ceiling_z))
	{
		self->pos1[2] = ceiling_z;
		self->flags |= FL_FLY;
		self->gravity = 0;
		self->velocity[2] = STALKER_CEILING_JUMP_SPEED;
	}
	else
	{
		stalker_abort_ceiling_jump(self);
		return;
	}

	self->style = STALKER_CEILING_JUMPING;
	self->s.origin[2] += 1;
	self->groundentity = NULL;
	self->velocity[0] += crandom() * 5;
	self->velocity[1] += crandom() * 5;
	self->monsterinfo.pausetime = level.time + 1.3;
	self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void stalker_jump_wait_land(edict_t *self)
{
	float ceiling_z;

	if (self->style != STALKER_CEILING_JUMPING)
		return;

	if (!stalker_ceiling_allowed(self))
	{
		stalker_set_floor(self);
		return;
	}

	if (stalker_find_ceiling(self, STALKER_CEILING_TRACE_DIST, &ceiling_z))
		self->pos1[2] = ceiling_z;

	if (self->pos1[2] && self->s.origin[2] + self->maxs[2] >= self->pos1[2] - 12)
	{
		stalker_attach_ceiling(self, self->pos1[2]);
		return;
	}

	if (self->pos1[2])
	{
		self->flags |= FL_FLY;
		self->gravity = 0;
		if (self->velocity[2] < STALKER_CEILING_MIN_SPEED)
			self->velocity[2] = STALKER_CEILING_MIN_SPEED;
	}

	if (self->groundentity || level.time > self->monsterinfo.pausetime)
	{
		stalker_abort_ceiling_jump(self);
		return;
	}

		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void stalker_jump_wait_land_ai(edict_t *self, float dist)
{
	ai_move(self, dist);
	stalker_jump_wait_land(self);
}

mframe_t stalker_frames_jump_straightup[] =
{
	ai_move, 1, stalker_jump_straightup,
	stalker_jump_wait_land_ai, 1, NULL,
	ai_move, -1, NULL,
	ai_move, -1, NULL
};
mmove_t stalker_move_jump_straightup = { FRAME_jump04, FRAME_jump07, stalker_frames_jump_straightup, stalker_run };

static qboolean stalker_start_ceiling_jump(edict_t *self, float cooldown)
{
	float ceiling_z;

	if (!stalker_ceiling_allowed(self) || stalker_on_ceiling(self) || !self->groundentity)
		return false;
	if (stalker_is_dodge_move(self) || level.time < self->monsterinfo.dodge_time)
		return false;
	if (!stalker_find_ceiling(self, STALKER_CEILING_TRACE_DIST, &ceiling_z))
	{
		self->monsterinfo.dodge_time = level.time + cooldown;
		return false;
	}

	self->monsterinfo.dodge_time = level.time + cooldown;
	self->pos1[2] = ceiling_z;
	self->monsterinfo.currentmove = &stalker_move_jump_straightup;
	stalker_jump_straightup(self);
	return true;
}

static void stalker_ai_dodge_slide(edict_t *self, float dist)
{
	if (!G_EntIsAlive(self->enemy) && G_EntIsAlive(self->monsterinfo.attacker))
		self->enemy = self->monsterinfo.attacker;
	if (!G_EntIsAlive(self->enemy))
		return;

	drone_ai_run_slide(self, dist);
}

mframe_t stalker_frames_dodge_run[] =
{
	stalker_ai_dodge_slide, 13, NULL,
	stalker_ai_dodge_slide, 17, NULL,
	stalker_ai_dodge_slide, 21, NULL,
	stalker_ai_dodge_slide, 18, NULL
};
static mmove_t stalker_move_dodge_run = { FRAME_run01, FRAME_run04, stalker_frames_dodge_run, stalker_run };

static qboolean stalker_start_dodge_slide(edict_t *self, edict_t *attacker, vec3_t dir)
{
	vec3_t right, diff;

	if (!self->groundentity || stalker_is_dodge_move(self))
		return false;
	if (!G_EntIsAlive(self->enemy) && G_EntIsAlive(attacker))
		self->enemy = attacker;
	if (!G_EntIsAlive(self->enemy))
		return false;

	AngleVectors(self->s.angles, NULL, right, NULL);
	VectorSubtract(dir, self->s.origin, diff);
	if (VectorLength(diff) > 1)
		self->monsterinfo.lefty = DotProduct(right, diff) >= 0;
	else
		self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;

	self->monsterinfo.currentmove = &stalker_move_dodge_run;
	self->monsterinfo.dodge_time = level.time + 0.4f + random() * 1.2f;
	return true;
}

static void stalker_fire(edict_t *self)
{
	int damage, speed;
	vec3_t forward, right, start, target, dir, offset;

	if (!G_EntExists(self->enemy))
		return;

	damage = M_BLASTER2_DMG_BASE + M_BLASTER2_DMG_ADDON * drone_damagelevel(self);
	if (M_BLASTER2_DMG_MAX && damage > M_BLASTER2_DMG_MAX)
		damage = M_BLASTER2_DMG_MAX;
	speed = M_BLASTER2_SPEED_BASE + M_BLASTER2_SPEED_ADDON * drone_damagelevel(self);
	if (M_BLASTER2_SPEED_MAX && speed > M_BLASTER2_SPEED_MAX)
		speed = M_BLASTER2_SPEED_MAX;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorCopy(monster_flash_offset[MZ2_STALKER_BLASTER], offset);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	VectorCopy(self->enemy->s.origin, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, dir);
	VectorNormalize(dir);

	monster_fire_blaster2(self, start, dir, damage, speed, EF_BLASTER, MZ2_STALKER_BLASTER);
}

mframe_t stalker_frames_shoot[] =
{
	drone_ai_run, 10, NULL,
	drone_ai_run, 10, stalker_fire,
	drone_ai_run, 12, stalker_fire,
	drone_ai_run, 12, stalker_fire
};
mmove_t stalker_move_shoot = { FRAME_run01, FRAME_run04, stalker_frames_shoot, stalker_run };

static void stalker_attack(edict_t *self)
{
	if (stalker_on_ceiling(self))
	{
		// sometimes come back down during combat
		if (entdist(self, self->enemy) < 96 || random() < 0.25f)
		{
			stalker_set_floor(self);
			self->velocity[2] = -300;
			stalker_run(self);
			M_DelayNextAttack(self, 0.6, true);
			return;
		}

		// otherwise keep fighting from the ceiling
		self->monsterinfo.currentmove = &stalker_move_shoot;
		M_DelayNextAttack(self, 0.4, true);
		return;
	}

	if (stalker_ceiling_allowed(self) && !stalker_on_ceiling(self) && self->groundentity &&
		level.time > self->monsterinfo.melee_finished && random() < 0.33f)
	{
		if (stalker_start_ceiling_jump(self, 3.0))
		{
			self->monsterinfo.melee_finished = level.time + 3.0;
			M_DelayNextAttack(self, 1.0, true);
			return;
		}
	}

	self->monsterinfo.currentmove = &stalker_move_shoot;
	M_DelayNextAttack(self, 0.4, true);
}

static void stalker_dodge(edict_t *self, edict_t *attacker, vec3_t dir, int radius)
{
	float ceiling_z;
	float eta;

	if (!stalker_dodge_allowed(self) || !self->groundentity)
		return;
	if (!attacker || OnSameTeam(self, attacker))
		return;
	if (level.time < self->monsterinfo.dodge_time || stalker_is_dodge_move(self))
		return;

	if (!G_EntIsAlive(self->enemy) && G_EntIsAlive(attacker))
		self->enemy = attacker;
	self->monsterinfo.attacker = attacker;

	eta = self->monsterinfo.eta - level.time;
	if ((eta < FRAMETIME) || (eta > 5.0f))
		return;

	if (stalker_ceiling_allowed(self) &&
		stalker_find_ceiling(self, STALKER_CEILING_TRACE_DIST, &ceiling_z) &&
		stalker_start_ceiling_jump(self, 1.0f + random() * 1.5f))
		return;

	if (radius || random() < 0.35f)
	{
		if (stalker_start_ceiling_jump(self, 1.0f + random() * 1.5f))
			return;
	}

	stalker_start_dodge_slide(self, attacker, dir);
}

static void stalker_swing_attack(edict_t *self)
{
	int damage;

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self);
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	if (M_MeleeAttack(self, self->enemy, 80, damage, 160))
		gi.sound(self, CHAN_WEAPON, (random() < 0.5) ? sound_punch_hit1 : sound_punch_hit2, 1, ATTN_NORM, 0);
}

mframe_t stalker_frames_swing_l[] =
{
	ai_charge, 0, NULL,
	ai_charge, 4, NULL,
	ai_charge, 5, stalker_swing_attack,
	ai_charge, 5, NULL,
	ai_charge, 5, stalker_swing_attack,
	ai_charge, 4, NULL,
	ai_charge, 2, NULL,
	ai_charge, 0, NULL
};
mmove_t stalker_move_swing_l = { FRAME_attack01, FRAME_attack08, stalker_frames_swing_l, stalker_run };

mframe_t stalker_frames_swing_r[] =
{
	ai_charge, 0, NULL,
	ai_charge, 6, stalker_swing_attack,
	ai_charge, 5, NULL,
	ai_charge, 5, stalker_swing_attack,
	ai_charge, 0, NULL
};
mmove_t stalker_move_swing_r = { FRAME_attack11, FRAME_attack15, stalker_frames_swing_r, stalker_run };

static void stalker_melee(edict_t *self)
{
	if (stalker_on_ceiling(self))
	{
		if (!G_ValidTarget(self, self->enemy, true, true) || entdist(self, self->enemy) > 96)
		{
			self->monsterinfo.melee_finished = level.time + 0.5;
			stalker_run(self);
			return;
		}

		if (random() < 0.5)
			self->monsterinfo.currentmove = &stalker_move_swing_l;
		else
			self->monsterinfo.currentmove = &stalker_move_swing_r;
		return;
	}

	if (!G_ValidTarget(self, self->enemy, true, true) || entdist(self, self->enemy) > 96)
	{
		self->monsterinfo.melee_finished = level.time + 0.5;
		stalker_run(self);
		return;
	}

	if (random() < 0.5)
		self->monsterinfo.currentmove = &stalker_move_swing_l;
	else
		self->monsterinfo.currentmove = &stalker_move_swing_r;
}

mframe_t stalker_frames_pain[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t stalker_move_pain = { FRAME_pain01, FRAME_pain04, stalker_frames_pain, stalker_run };

static void stalker_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (self->monsterinfo.currentmove == &stalker_move_false_death_end ||
		self->monsterinfo.currentmove == &stalker_move_false_death_start)
		return;

	if (self->monsterinfo.currentmove == &stalker_move_false_death)
	{
		stalker_reactivate(self);
		return;
	}

	if (self->health > 0 && self->health < (self->max_health / 4) &&
		self->groundentity && !stalker_on_ceiling(self) && random() < 0.30f)
	{
		stalker_false_death_start(self);
		return;
	}

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (invasion->value == 2)
		return;

	if (self->style == STALKER_CEILING_JUMPING && damage > 10)
	{
		stalker_abort_ceiling_jump(self);
		self->monsterinfo.currentmove = &stalker_move_pain;
		return;
	}

	if (stalker_is_dodge_move(self))
		return;

	if (damage > 10 && random() < 0.5 && stalker_start_ceiling_jump(self, 3.0))
		return;

	self->monsterinfo.currentmove = &stalker_move_pain;
}

static void stalker_dead(edict_t *self)
{
	VectorSet(self->mins, -28, -28, -18);
	VectorSet(self->maxs, 28, 28, -4);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
	M_PrepBodyRemoval(self);
}

mframe_t stalker_frames_death[] =
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
mmove_t stalker_move_death = { FRAME_death01, FRAME_death09, stalker_frames_death, stalker_dead };

static void stalker_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	M_Notify(self);
	stalker_set_floor(self);
	self->prethink = NULL;
	self->movetype = MOVETYPE_TOSS;

#ifdef OLD_NOLAG_STYLE
	if (nolag->value)
	{
		M_Remove(self, false, true);
		return;
	}
#endif

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
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
	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	vrx_update_drone_death_skin(self);
	self->monsterinfo.currentmove = &stalker_move_death;

	if (self->activator && !self->activator->client)
		self->activator->num_monsters_real--;
}

void init_drone_stalker(edict_t *self)
{
	sound_pain = gi.soundindex("stalker/pain.wav");
	sound_die = gi.soundindex("stalker/death.wav");
	sound_sight = gi.soundindex("stalker/sight.wav");
	sound_punch_hit1 = gi.soundindex("stalker/melee1.wav");
	sound_punch_hit2 = gi.soundindex("stalker/melee2.wav");
	sound_idle = gi.soundindex("stalker/idle.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/stalker/tris.md2");
	gi.modelindex("models/monsters/stalker/gibs/bodya.md2");
	gi.modelindex("models/monsters/stalker/gibs/bodyb.md2");
	gi.modelindex("models/monsters/stalker/gibs/claw.md2");
	gi.modelindex("models/monsters/stalker/gibs/foot.md2");
	gi.modelindex("models/monsters/stalker/gibs/head.md2");
	gi.modelindex("models/monsters/stalker/gibs/leg.md2");

	VectorSet(self->mins, -28, -28, -18);
	VectorSet(self->maxs, 28, 28, 18);

	self->health = M_STALKER_INITIAL_HEALTH + M_STALKER_ADDON_HEALTH * self->monsterinfo.level;
	self->max_health = self->health;
	self->gib_health = -125;
	self->mass = 250;
	self->mtype = M_STALKER;

	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
	self->monsterinfo.power_armor_power = M_STALKER_INITIAL_ARMOR + M_STALKER_ADDON_ARMOR * self->monsterinfo.level;
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
	self->monsterinfo.control_cost = M_BERSERKER_CONTROL_COST;
	self->monsterinfo.cost = M_DEFAULT_COST;

	self->item = FindItemByClassname("ammo_cells");

	self->pain = stalker_pain;
	self->die = stalker_die;
	self->monsterinfo.stand = stalker_stand;
	self->monsterinfo.walk = stalker_walk;
	self->monsterinfo.run = stalker_run;
	self->monsterinfo.dodge = stalker_dodge;
	self->monsterinfo.attack = stalker_attack;
	self->monsterinfo.melee = stalker_melee;
	self->monsterinfo.sight = stalker_sight;
	self->monsterinfo.idle = stalker_idle;
	self->prethink = stalker_ceiling_prethink;
	self->monsterinfo.pain_chance = 0.3f;
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &stalker_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;
	self->nextthink = level.time + FRAMETIME;
}
